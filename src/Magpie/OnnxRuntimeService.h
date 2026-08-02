#pragma once
#include "Event.h"

namespace Magpie {

enum class OnnxRuntimeStatus {
	// third_party\ 里没有运行时
	NotInstalled,
	Downloading,
	Extracting,
	Installed,
	Error
};

// AI 放大所需的运行时（onnxruntime / TensorRT / CUDA / DirectML）解压后约 2.4 GB，
// 不随程序分发，因此按需下载到 third_party\。
//
// The runtime needed for AI upscaling is ~2.4 GB unpacked - far too large to
// ship with Magpie - so it is fetched on demand into third_party\. Without it
// the app runs normally and simply reports that AI upscaling is unavailable
// (see PinThirdPartyRuntimes in main.cpp), so this is purely additive.
class OnnxRuntimeService {
public:
	static OnnxRuntimeService& Get() noexcept {
		static OnnxRuntimeService instance;
		return instance;
	}

	OnnxRuntimeService(const OnnxRuntimeService&) = delete;
	OnnxRuntimeService(OnnxRuntimeService&&) = delete;

	// 以 third_party\onnxruntime.dll 是否存在为准
	// Presence of third_party\onnxruntime.dll is the test; a partial extract
	// would leave it missing, so a failed install does not read as complete.
	bool IsInstalled() const noexcept;

	OnnxRuntimeStatus Status() const noexcept {
		return _status;
	}

	// 0 ~ 1，仅在 Downloading 时有意义
	double DownloadProgress() const noexcept {
		return _downloadProgress;
	}

	winrt::fire_and_forget DownloadAndInstall();

	void Cancel() noexcept {
		_cancelled = true;
	}

	Event<OnnxRuntimeStatus> StatusChanged;
	Event<double> DownloadProgressChanged;

private:
	OnnxRuntimeService() = default;

	void _Status(OnnxRuntimeStatus value);

	// 用系统自带的 tar.exe 解压。它是 bsdtar/libarchive，原生支持 7z，
	// 这样就不必为了一个下载功能引入 LZMA 依赖。
	// Extraction goes through the in-box tar.exe: it is bsdtar/libarchive with
	// liblzma and reads 7z natively, which avoids taking an LZMA dependency
	// just to unpack one download.
	bool _Extract(const std::wstring& archivePath, const std::wstring& destDir,
		const wchar_t* subDir) noexcept;

	std::wstring _ThirdPartyDir() const noexcept;

	OnnxRuntimeStatus _status = OnnxRuntimeStatus::NotInstalled;
	double _downloadProgress = 0;
	bool _cancelled = false;
};

}
