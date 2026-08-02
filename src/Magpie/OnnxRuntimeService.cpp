#include "pch.h"
#include "OnnxRuntimeService.h"
#include "App.h"
#include "Logger.h"
#include "StrHelper.h"
#include "Win32Helper.h"
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Web.Http.h>

using namespace winrt;
// App 在 winrt::Magpie::implementation 里，而本文件位于 namespace Magpie
// App lives in winrt::Magpie::implementation; this file is in namespace Magpie,
// so without this the name resolves to Magpie::App and does not exist.
using namespace winrt::Magpie::implementation;
using namespace Windows::Storage::Streams;
using namespace Windows::Web::Http;

namespace Magpie {

// 运行时已经作为 release 资源发布，直接取用，不再另行分发。
// 注意运行时分散在两个资源里：onnxruntime.dll 和 DirectML.dll 在主包的
// third_party\ 下，TensorRT/CUDA 那几个大文件在单独的 ext 包里。
//
// The runtime is already published as release assets, so it is fetched from
// there rather than redistributed again. It is split across two of them:
// onnxruntime.dll and DirectML.dll live under third_party\ inside the main
// package, while the large TensorRT/CUDA libraries are in the ext package.
struct RuntimePackage {
	const wchar_t* url;
	const wchar_t* fileName;
	// 主包里带 third_party\ 前缀，需要剥掉一层
	// The main package nests them under third_party\, so strip one level.
	const wchar_t* subDir;
	// 已存在则跳过，重试时不必重新下载近 1 GB
	// Skipped when already present, so a retry does not refetch ~1 GB.
	const wchar_t* probe;
	uint64_t approxBytes;
};

static constexpr RuntimePackage PACKAGES[] = {
	{
		L"https://github.com/Blinue/Magpie/releases/download/onnx-preview2/Magpie-onnx-preview2-x64.zip",
		L"onnx-core.zip",
		L"third_party",
		L"onnxruntime.dll",
		26ull * 1024 * 1024
	},
	{
		L"https://github.com/Blinue/Magpie/releases/download/onnx-preview2/ext-tensorrt-x64.7z",
		L"onnx-ext.7z",
		nullptr,
		L"nvinfer_10.dll",
		973ull * 1024 * 1024
	}
};

std::wstring OnnxRuntimeService::_ThirdPartyDir() const noexcept {
	return (Win32Helper::GetExePath().parent_path() / L"third_party").wstring();
}

bool OnnxRuntimeService::IsInstalled() const noexcept {
	const std::wstring probe = _ThirdPartyDir() + L"\\onnxruntime.dll";
	return Win32Helper::FileExists(probe.c_str());
}

void OnnxRuntimeService::_Status(OnnxRuntimeStatus value) {
	if (_status == value) {
		return;
	}

	_status = value;
	StatusChanged.Invoke(value);
}

bool OnnxRuntimeService::_Extract(
	const std::wstring& archivePath,
	const std::wstring& destDir,
	const wchar_t* subDir
) noexcept {
	// tar.exe 一定在 System32，用绝对路径避免 PATH 被劫持
	// tar.exe always lives in System32; use the absolute path so a hijacked
	// PATH cannot substitute something else.
	wchar_t systemDir[MAX_PATH];
	const UINT len = GetSystemDirectory(systemDir, MAX_PATH);
	if (len == 0 || len >= MAX_PATH) {
		Logger::Get().Win32Error("GetSystemDirectory 失败");
		return false;
	}

	const std::wstring tarPath = StrHelper::Concat(systemDir, L"\\tar.exe");
	if (!Win32Helper::FileExists(tarPath.c_str())) {
		Logger::Get().Error("找不到 tar.exe / tar.exe is not present");
		return false;
	}

	// bsdtar 能按路径挑选条目，--strip-components 去掉外层目录
	// bsdtar can select entries by path; --strip-components drops the wrapper
	// directory so the files land directly in destDir.
	std::wstring cmdLine = StrHelper::Concat(
		L"\"", tarPath, L"\" -xf \"", archivePath, L"\" -C \"", destDir, L"\"");
	if (subDir) {
		cmdLine += StrHelper::Concat(L" --strip-components=1 ", subDir);
	}

	STARTUPINFO si{ .cb = sizeof(si), .dwFlags = STARTF_USESHOWWINDOW, .wShowWindow = SW_HIDE };
	wil::unique_process_information pi;
	if (!CreateProcess(nullptr, cmdLine.data(), nullptr, nullptr, FALSE,
		CREATE_NO_WINDOW, nullptr, nullptr, &si, pi.addressof())) {
		Logger::Get().Win32Error("CreateProcess(tar.exe) 失败");
		return false;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD exitCode = 1;
	GetExitCodeProcess(pi.hProcess, &exitCode);
	if (exitCode != 0) {
		Logger::Get().Error(fmt::format("tar.exe 解压失败，退出码 {}", exitCode));
		return false;
	}

	return true;
}

fire_and_forget OnnxRuntimeService::DownloadAndInstall() {
	if (_status == OnnxRuntimeStatus::Downloading ||
		_status == OnnxRuntimeStatus::Extracting) {
		co_return;
	}

	_cancelled = false;
	_downloadProgress = 0;

	const std::wstring thirdPartyDir = _ThirdPartyDir();

	if (!Win32Helper::DirExists(thirdPartyDir.c_str()) &&
		!Win32Helper::CreateDir(thirdPartyDir, true)) {
		Logger::Get().Win32Error("创建 third_party 失败");
		_Status(OnnxRuntimeStatus::Error);
		co_return;
	}

	// 已经就位的包不再下载，重试时只补缺的那个
	// Skip packages already on disk so a retry only fetches what is missing.
	uint64_t plannedBytes = 0;
	bool needed[std::size(PACKAGES)]{};
	for (size_t i = 0; i < std::size(PACKAGES); ++i) {
		const std::wstring probe = thirdPartyDir + L"\\" + PACKAGES[i].probe;
		needed[i] = !Win32Helper::FileExists(probe.c_str());
		if (needed[i]) {
			plannedBytes += PACKAGES[i].approxBytes;
		}
	}

	_Status(OnnxRuntimeStatus::Downloading);

	// 下载留在 UI 线程：WinRT 的异步等待会回到同一个上下文，而状态事件最终
	// 会触碰 XAML，从线程池线程触发会抛 "marshalled for a different thread"。
	// 只有阻塞的解压切到后台，并且在报告状态前切回来。
	//
	// The download stays on the UI thread: the awaits below resume on the same
	// context, and the status events end up touching XAML, so firing them from
	// a thread-pool thread throws. Only the blocking extract goes to the
	// background, and every exit path returns here before reporting.
	bool ok = false;
	bool cancelled = false;
	bool failed = false;
	// 跨多个包累计，进度条按总量走
	// Accumulated across packages so the bar tracks the whole job.
	uint64_t doneBytes = 0;

	try {
		for (size_t i = 0; i < std::size(PACKAGES) && !cancelled && !failed; ++i) {
			if (!needed[i]) {
				continue;
			}

			const RuntimePackage& pkg = PACKAGES[i];
			const std::wstring archivePath =
				StrHelper::Concat(thirdPartyDir, L"\\", pkg.fileName);

			HttpClient httpClient;
			auto requestProgressOp = httpClient.GetInputStreamAsync(Uri(pkg.url));
			IInputStream httpStream = co_await requestProgressOp;

			{
				wil::unique_hfile file(
					CreateFile2(archivePath.c_str(), GENERIC_WRITE, 0, CREATE_ALWAYS, nullptr));
				if (!file) {
					Logger::Get().Win32Error("创建下载文件失败");
					failed = true;
				} else {
					Buffer buffer(64 * 1024);
					// 总量接近 1 GB，必须用 64 位计数
					// Close to 1 GB in total, so the counter has to be 64-bit.
					uint64_t pkgBytes = 0;

					while (true) {
						IBuffer resultBuffer = co_await httpStream.ReadAsync(
							buffer, buffer.Capacity(), InputStreamOptions::Partial);

						if (_cancelled) {
							httpStream.Close();
							cancelled = true;
							break;
						}

						const uint32_t bufferSize = resultBuffer.Length();
						if (bufferSize == 0) {
							break;
						}

						if (!WriteFile(file.get(), resultBuffer.data(), bufferSize, nullptr, nullptr)) {
							Logger::Get().Win32Error("WriteFile 失败");
							failed = true;
							break;
						}

						pkgBytes += bufferSize;
						if (plannedBytes > 0) {
							_downloadProgress = std::min(1.0,
								(doneBytes + pkgBytes) / (double)plannedBytes);
							DownloadProgressChanged.Invoke(_downloadProgress);
						}
					}

					doneBytes += pkgBytes;
				}
			}

			if (cancelled || failed) {
				DeleteFile(archivePath.c_str());
				break;
			}

			_Status(OnnxRuntimeStatus::Extracting);

			// 解压会阻塞（WaitForSingleObject），必须切到后台
			// Extraction blocks on WaitForSingleObject, so it leaves the UI
			// thread here and returns below before anything is reported.
			co_await resume_background();
			const bool extracted = _Extract(archivePath, thirdPartyDir, pkg.subDir);
			DeleteFile(archivePath.c_str());
			co_await App::Get().Dispatcher();

			if (!extracted) {
				failed = true;
				break;
			}

			_Status(OnnxRuntimeStatus::Downloading);
		}

		if (!cancelled && !failed) {
			ok = IsInstalled();
			if (!ok) {
				Logger::Get().Error("解压后仍找不到 onnxruntime.dll");
			}
		}
	} catch (const hresult_error& e) {
		Logger::Get().Error(StrHelper::Concat(
			"下载运行时失败 / failed to download the runtime: ",
			StrHelper::UTF16ToUTF8(e.message())));
	}

	// 清掉可能残留的半个压缩包
	// Drop any half-written archive left behind.
	for (const RuntimePackage& pkg : PACKAGES) {
		DeleteFile(StrHelper::Concat(thirdPartyDir, L"\\", pkg.fileName).c_str());
	}

	// 可能仍在后台线程上（解压途中失败或抛异常），报告状态前必须切回
	// May still be on a background thread if the extract failed or threw, so
	// come back before touching anything the UI is bound to. Harmless when we
	// are already on the UI thread.
	co_await App::Get().Dispatcher();

	if (cancelled) {
		_downloadProgress = 0;
		_Status(OnnxRuntimeStatus::NotInstalled);
	} else {
		_Status(ok ? OnnxRuntimeStatus::Installed : OnnxRuntimeStatus::Error);
	}
}

}
