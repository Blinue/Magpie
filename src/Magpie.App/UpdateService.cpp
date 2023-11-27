#include "pch.h"
#include "UpdateService.h"
#include <rapidjson/document.h>
#include "JsonHelper.h"
#include "Logger.h"
#include "StrUtils.h"
#include "Version.h"
#include "AppSettings.h"
#include "Win32Utils.h"
#include "CommonSharedConstants.h"
#include <zip/zip.h>
#include <filesystem>
#include <winrt/Windows.Security.Cryptography.Core.h>

using namespace winrt;
using namespace Windows::Security::Cryptography::Core;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::System::Threading;

namespace winrt::Magpie::App {

static constexpr Version MAGPIE_VERSION(
#ifdef MAGPIE_VERSION_MAJOR
	MAGPIE_VERSION_MAJOR, MAGPIE_VERSION_MINOR, MAGPIE_VERSION_PATCH
#else
	0, 0, 0
#endif
);

void UpdateService::Initialize() noexcept {
#ifndef MAGPIE_VERSION_TAG
	// 只有正式版本才能检查更新
	return;
#endif

	_dispatcher = CoreWindow::GetForCurrentThread().Dispatcher();
	
	AppSettings& settings = AppSettings::Get();
	if (settings.IsAutoCheckForUpdates()) {
		_StartTimer();
		// 启动时检查一次
		_Timer_Tick(nullptr);
	}
	settings.IsAutoCheckForUpdatesChanged([this](bool value) {
		if (value) {
			_StartTimer();
		} else {
			_StopTimer();
		}
	});
}

fire_and_forget UpdateService::CheckForUpdatesAsync(bool isAutoUpdate) {
	if (_status == UpdateStatus::Checking) {
		co_return;
	}

	_Status(UpdateStatus::Checking);

	rapidjson::Document doc;

	try {
		HttpClient httpClient;
		IBuffer buffer = co_await httpClient.GetBufferAsync(
			Uri(AppSettings::Get().IsCheckForPreviewUpdates()
			? L"https://raw.githubusercontent.com/Blinue/Magpie/dev/version.json"
			: L"https://raw.githubusercontent.com/Blinue/Magpie/main/version.json"));

		doc.Parse((const char*)buffer.data(), buffer.Length());
	} catch (const hresult_error& e) {
		Logger::Get().ComError(
			StrUtils::Concat("检查更新失败：", StrUtils::UTF16ToUTF8(e.message())),
			e.code()
		);

		if (e.code() == HTTP_E_STATUS_NOT_FOUND) {
			// 404 表示没有更新可用
			_Status(UpdateStatus::NoUpdate);
		} else {
			_Status(UpdateStatus::ErrorWhileChecking);
		}
		
		co_return;
	}

	if (!doc.IsObject()) {
		Logger::Get().Error("根元素不是对象");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}

	auto root = doc.GetObj();
	auto versionNode = root.FindMember("version");
	if (versionNode == root.end()) {
		Logger::Get().Error("找不到 version 成员");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	if (!versionNode->value.IsString()) {
		Logger::Get().Error("version 成员不是字符串");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}

	Version remoteVersion;
	if(!remoteVersion.Parse(versionNode->value.GetString())) {
		Logger::Get().Error("解析版本号失败");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}

	if (remoteVersion <= MAGPIE_VERSION) {
		_Status(UpdateStatus::NoUpdate);
		co_return;
	}

	auto tagNode = root.FindMember("tag");
	if (tagNode == root.end()) {
		Logger::Get().Error("找不到 tag 成员");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	if (!tagNode->value.IsString()) {
		Logger::Get().Error("tag 成员不是字符串");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	_tag = StrUtils::UTF8ToUTF16(tagNode->value.GetString());
	if (_tag.empty()) {
		Logger::Get().Error("tag 成员为空");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}

	auto binaryNode = root.FindMember("binary");
	if (binaryNode == root.end()) {
		Logger::Get().Error("找不到 binary 成员");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	if (!binaryNode->value.IsObject()) {
		Logger::Get().Error("binary 成员不是对象");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	auto binaryObj = binaryNode->value.GetObj();
	auto x64Node = binaryObj.FindMember("x64");
	if (x64Node == binaryObj.end()) {
		Logger::Get().Error("找不到 x64 成员");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	if (!x64Node->value.IsObject()) {
		Logger::Get().Error("x64 成员不是对象");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	auto x64Obj = x64Node->value.GetObj();

	auto urlNode = x64Obj.FindMember("url");
	if (urlNode == x64Obj.end()) {
		Logger::Get().Error("找不到 url 成员");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	if (!urlNode->value.IsString()) {
		Logger::Get().Error("url 成员不是字符串");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	_binaryUrl = StrUtils::UTF8ToUTF16(urlNode->value.GetString());
	if (_binaryUrl.empty()) {
		Logger::Get().Error("url 成员为空");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}

	auto hashNode = x64Obj.FindMember("hash");
	if (hashNode == x64Obj.end()) {
		Logger::Get().Error("找不到 hash 成员");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	if (!hashNode->value.IsString()) {
		Logger::Get().Error("hash 成员不是字符串");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	_binaryHash = StrUtils::UTF8ToUTF16(hashNode->value.GetString());
	if (_binaryHash.empty()) {
		Logger::Get().Error("hash 成员为空");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}

	_Status(UpdateStatus::Available);
	if (isAutoUpdate) {
		IsShowOnHomePage(true);
	}
}

static std::wstring Md5ToHex(const uint8_t* data) {
	static wchar_t oct2Hex[16] = {
		L'0',L'1',L'2',L'3',L'4',L'5',L'6',L'7',
		L'8',L'9',L'a',L'b',L'c',L'd',L'e',L'f'
	};

	std::wstring result(32, 0);
	wchar_t* pResult = &result[0];

	for (int i = 0; i < 16; ++i) {
		uint8_t b = data[i];
		*pResult++ = oct2Hex[(b >> 4) & 0xf];
		*pResult++ = oct2Hex[b & 0xf];
	}

	return result;
}

fire_and_forget UpdateService::DownloadAndInstall() {
	assert(_status == UpdateStatus::Available || _status == UpdateStatus::ErrorWhileDownloading);
	_Status(UpdateStatus::Downloading);

	_downloadProgress = 0;
	_downloadCancelled = false;
	_downloadProgressChangedEvent(_downloadProgress);

	// 删除 update 文件夹
	std::filesystem::remove_all(CommonSharedConstants::UPDATE_DIR);

	// 下载新版
	try {
		wchar_t curDir[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, curDir);
		StorageFolder curFolder = co_await StorageFolder::GetFolderFromPathAsync(curDir);

		std::wstring updateDir(CommonSharedConstants::UPDATE_DIR, StrUtils::StrLen(CommonSharedConstants::UPDATE_DIR) - 1);
		StorageFolder updateFolder = co_await curFolder.CreateFolderAsync(
			updateDir.c_str(), CreationCollisionOption::OpenIfExists);

		StorageFile updatePkg = co_await updateFolder.CreateFileAsync(
			L"update.zip",
			CreationCollisionOption::ReplaceExisting
		);

		HttpClient httpClient;
		auto requestProgressOp = httpClient.GetInputStreamAsync(Uri(_binaryUrl));
		double totalBytes = 0;
		requestProgressOp.Progress([&totalBytes](const auto&, const HttpProgress& progress) {
			if (std::optional<uint64_t> totalBytesToReceive = progress.TotalBytesToReceive) {
				totalBytes = (double)*totalBytesToReceive;
			}
		});
		IInputStream httpStream = co_await requestProgressOp;

		if (_downloadCancelled) {
			_Status(UpdateStatus::Available);
			co_return;
		}

		IRandomAccessStream fileStream = co_await updatePkg.OpenAsync(FileAccessMode::ReadWrite);
		HashAlgorithmProvider hashAlgProvider = HashAlgorithmProvider::OpenAlgorithm(HashAlgorithmNames::Md5());
		CryptographicHash hasher = hashAlgProvider.CreateHash();

		Buffer buffer(64 * 1024);
		uint32_t bytesReceived = 0;
		while (true) {
			IBuffer resultBuffer = co_await httpStream.ReadAsync(buffer, buffer.Capacity(), InputStreamOptions::Partial);

			if (_downloadCancelled) {
				httpStream.Close();
				co_await fileStream.FlushAsync();
				fileStream.Close();
				_Status(UpdateStatus::Available);
				co_return;
			}

			uint32_t size = resultBuffer.Length();
			if (size == 0) {
				break;
			}

			bytesReceived += size;
			if (totalBytes >= 1e-6) {
				_downloadProgress = bytesReceived / totalBytes;
				_downloadProgressChangedEvent(_downloadProgress);
			}

			hasher.Append(buffer);

			co_await fileStream.WriteAsync(resultBuffer);
		}

		co_await fileStream.FlushAsync();
		fileStream.Close();

		// 检查哈希
		IBuffer hash = hasher.GetValueAndReset();
		assert(hash.Length() == 16);
		if (Md5ToHex(hash.data()) != _binaryHash) {
			Logger::Get().Error("下载失败：哈希不匹配");
			_Status(UpdateStatus::ErrorWhileDownloading);
			co_return;
		}
	} catch (const hresult_error& e) {
		Logger::Get().Error(StrUtils::Concat("下载失败：", StrUtils::UTF16ToUTF8(e.message())));
		_Status(UpdateStatus::ErrorWhileDownloading);
		co_return;
	}

	// 后台解压 zip
	CoreDispatcher dispatcher = CoreWindow::GetForCurrentThread().Dispatcher();
	co_await resume_background();

	std::wstring updatePkg = CommonSharedConstants::UPDATE_DIR + L"update.zip"s;
	// kuba-zip 内部使用 UTF-8 编码
	int ec = zip_extract(
		StrUtils::UTF16ToUTF8(updatePkg).c_str(),
		StrUtils::UTF16ToUTF8(CommonSharedConstants::UPDATE_DIR).c_str(),
		nullptr,
		nullptr
	);
	if (ec < 0) {
		Logger::Get().Error(fmt::format("解压失败，错误代码：{}", ec));
		co_await dispatcher;
		_Status(UpdateStatus::ErrorWhileDownloading);
		co_return;
	}

	DeleteFile(updatePkg.c_str());

	std::wstring magpieExePath = StrUtils::Concat(CommonSharedConstants::UPDATE_DIR, L"Magpie.exe");
	std::wstring updaterExePath = StrUtils::Concat(CommonSharedConstants::UPDATE_DIR, L"Updater.exe");
	if (!Win32Utils::FileExists(magpieExePath.c_str()) || !Win32Utils::FileExists(updaterExePath.c_str())) {
		Logger::Get().Error("未找到 Magpie.exe 或 Updater.exe");
		co_await dispatcher;
		_Status(UpdateStatus::ErrorWhileDownloading);
		co_return;
	}

	co_await dispatcher;

	if (_downloadCancelled) {
		_Status(UpdateStatus::Available);
		co_return;
	}

	_Status(UpdateStatus::Installing);

	// 再转入后台安装更新
	co_await resume_background();

	// 安装更新流程
	// ----------------------------------------------------
	// Magpie.exe
	// 1. 下载并解压新版本至 update 文件夹
	// 2. 将 Updater.exe 复制到根目录
	// 3. 运行 Updater.exe 然后退出
	// 
	// Updater.exe
	// 1. 等待 Magpie.exe 退出
	// 2. 删除旧版本文件，复制新版本文件，删除 update 文件夹
	// 3. 启动 Magpie.exe 然后退出

	MoveFileEx(updaterExePath.c_str(), L"Updater.exe", MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);

	SHELLEXECUTEINFO execInfo{};
	execInfo.cbSize = sizeof(execInfo);
	execInfo.lpFile = L"Updater.exe";
	std::wstring curVersion = MAGPIE_VERSION.ToString();
	execInfo.lpParameters = curVersion.c_str();
	execInfo.lpVerb = L"open";
	// 调用 ShellExecuteEx 后立即退出，因此应该指定 SEE_MASK_NOASYNC
	execInfo.fMask = SEE_MASK_NOASYNC;

	if (!ShellExecuteEx(&execInfo)) {
		Logger::Get().Win32Error("ShellExecuteEx 失败");
	}

	Application::Current().as<App>().Quit();
}

void UpdateService::EnteringAboutPage() {
	if (_status == UpdateStatus::NoUpdate || _status == UpdateStatus::ErrorWhileChecking) {
		_Status(UpdateStatus::Pending);
	}
}

void UpdateService::ClosingMainWindow() {
	EnteringAboutPage();

	if (_status == UpdateStatus::Available) {
		_Status(UpdateStatus::Pending);
	}
}

void UpdateService::Cancel() {
	switch (_status) {
	case UpdateStatus::Pending:
	case UpdateStatus::Checking:
	case UpdateStatus::Installing:
		// 不支持取消
		assert(false);
		break;
	case UpdateStatus::NoUpdate:
	case UpdateStatus::ErrorWhileChecking:
	case UpdateStatus::Available:
	case UpdateStatus::ErrorWhileDownloading:
		_Status(UpdateStatus::Pending);
		break;
	case UpdateStatus::Downloading:
		_downloadCancelled = true;
		break;
	default:
		break;
	}
}

void UpdateService::_Status(UpdateStatus value) {
	if (_status == value) {
		return;
	}

	_status = value;
	_statusChangedEvent(value);
}

fire_and_forget UpdateService::_Timer_Tick(ThreadPoolTimer const& timer) {
	if (timer) {
		co_await _dispatcher;
	}

	AppSettings& settings = AppSettings::Get();
	
	using namespace std::chrono;
	system_clock::time_point now = system_clock::now();
	// 自动检查更新的间隔为 1 天
	if (duration_cast<days>(now - settings.UpdateCheckDate()).count() < 1) {
		co_return;
	}

	settings.UpdateCheckDate(now);
	CheckForUpdatesAsync(true);
}

void UpdateService::_StartTimer() {
	// 每小时检查一次剩余时间
	_timer = ThreadPoolTimer::CreatePeriodicTimer(
		{ this, &UpdateService::_Timer_Tick },
		1h
	);
}

void UpdateService::_StopTimer() {
	_timer.Cancel();
	_timer = nullptr;
}

}
