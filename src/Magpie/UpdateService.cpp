#include "pch.h"
#include "UpdateService.h"
#include <rapidjson/document.h>
#include "JsonHelper.h"
#include "Logger.h"
#include "StrHelper.h"
#include "Version.h"
#include "AppSettings.h"
#include "Win32Helper.h"
#include "CommonSharedConstants.h"
#include <zip/zip.h>
#include <bcrypt.h>
#include <wil/resource.h>	// 再次包含以激活 CNG 相关包装器
#include "App.h"
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace ::Magpie;
using namespace winrt::Magpie::implementation;
using namespace winrt;
using namespace Windows::Storage::Streams;
using namespace Windows::System::Threading;
using namespace Windows::Web::Http;

namespace Magpie {

static constexpr Version MAGPIE_VERSION(
#ifdef MAGPIE_VERSION_MAJOR
	MAGPIE_VERSION_MAJOR, MAGPIE_VERSION_MINOR, MAGPIE_VERSION_PATCH
#else
	0, 0, 0
#endif
);

static constexpr uint32_t MD5_HASH_LENGTH = 16;

void UpdateService::Initialize() noexcept {
	// 只有发布版本能检查更新
#ifdef MAGPIE_VERSION_TAG
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

	App::Get().MainWindow().Destroyed([this]() {
		_MainWindow_Closed();
	});
#endif
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
			StrHelper::Concat("检查更新失败: ", StrHelper::UTF16ToUTF8(e.message())),
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
	_tag = StrHelper::UTF8ToUTF16(tagNode->value.GetString());
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
	const char* platform =
#ifdef _M_X64
	"x64";
#elif defined(_M_ARM64)
	"ARM64";
#else
	static_assert(false, "不支持的架构")
#endif
	auto platformNode = binaryObj.FindMember(platform);
	if (platformNode == binaryObj.end()) {
		Logger::Get().Error(StrHelper::Concat("找不到 ", platform, "成员"));
		// 还不支持此架构
		_Status(UpdateStatus::NoUpdate);
		co_return;
	}
	if (!platformNode->value.IsObject()) {
		Logger::Get().Error(StrHelper::Concat(platform, " 成员不是对象"));
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	auto platformObj = platformNode->value.GetObj();

	auto urlNode = platformObj.FindMember("url");
	if (urlNode == platformObj.end()) {
		Logger::Get().Error("找不到 url 成员");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	if (!urlNode->value.IsString()) {
		Logger::Get().Error("url 成员不是字符串");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	_binaryUrl = StrHelper::UTF8ToUTF16(urlNode->value.GetString());
	if (_binaryUrl.empty()) {
		Logger::Get().Error("url 成员为空");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}

	auto hashNode = platformObj.FindMember("hash");
	if (hashNode == platformObj.end()) {
		Logger::Get().Error("找不到 hash 成员");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	if (!hashNode->value.IsString()) {
		Logger::Get().Error("hash 成员不是字符串");
		_Status(UpdateStatus::ErrorWhileChecking);
		co_return;
	}
	_binaryHash = StrHelper::UTF8ToUTF16(hashNode->value.GetString());
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

	for (uint32_t i = 0; i < MD5_HASH_LENGTH; ++i) {
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
	DownloadProgressChanged.Invoke(_downloadProgress);

	// 清空 update 文件夹
	HRESULT hr = wil::RemoveDirectoryRecursiveNoThrow(
		CommonSharedConstants::UPDATE_DIR, wil::RemoveDirectoryOptions::KeepRootDirectory);
	if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) {
		if (!CreateDirectory(CommonSharedConstants::UPDATE_DIR, nullptr)) {
			Logger::Get().ComError("创建 update 文件夹失败", hr);
			_Status(UpdateStatus::ErrorWhileDownloading);
			co_return;
		}
	} else if (FAILED(hr)) {
		Logger::Get().ComError("RemoveDirectoryRecursiveNoThrow 失败", hr);
		_Status(UpdateStatus::ErrorWhileDownloading);
		co_return;
	}
	
	std::wstring updatePkgPath = StrHelper::Concat(CommonSharedConstants::UPDATE_DIR, L"update.zip");
	wil::unique_hfile updatePkg(
		CreateFile2(updatePkgPath.c_str(), GENERIC_WRITE, 0, CREATE_ALWAYS, nullptr));
	if (!updatePkg) {
		Logger::Get().ComError("打开 update.zip 失败", hr);
		_Status(UpdateStatus::ErrorWhileDownloading);
		co_return;
	}

	// 下载新版
	try {
		HttpClient httpClient;
		auto requestProgressOp = httpClient.GetInputStreamAsync(Uri(_binaryUrl));
		// 如果文件大小未知，则 totalBytes 为 0
		uint64_t totalBytes = 0;
		requestProgressOp.Progress([&totalBytes](const auto&, const HttpProgress& progress) {
			if (std::optional<uint64_t> totalBytesToReceive = progress.TotalBytesToReceive) {
				totalBytes = *totalBytesToReceive;
			}
		});
		IInputStream httpStream = co_await requestProgressOp;

		if (_downloadCancelled) {
			_Status(UpdateStatus::Available);
			co_return;
		}
		
		// 用于计算 MD5
		wil::unique_bcrypt_algorithm hAlg;
		NTSTATUS status = BCryptOpenAlgorithmProvider(hAlg.put(), BCRYPT_MD5_ALGORITHM, nullptr, 0);
		if (status != STATUS_SUCCESS) {
			Logger::Get().NTError("BCryptOpenAlgorithmProvider 失败", status);
			_Status(UpdateStatus::ErrorWhileDownloading);
			co_return;
		}

		ULONG result;

#ifdef _DEBUG
		uint32_t hashLen = 0;
		BCryptGetProperty(hAlg.get(), BCRYPT_HASH_LENGTH,
			(PUCHAR)&hashLen, sizeof(hashLen), &result, 0);
		assert(hashLen == MD5_HASH_LENGTH);
#endif

		uint32_t hashObjSize = 0;
		status = BCryptGetProperty(hAlg.get(), BCRYPT_OBJECT_LENGTH,
			(PUCHAR)&hashObjSize, sizeof(hashObjSize), &result, 0);
		if (status != STATUS_SUCCESS) {
			Logger::Get().NTError("BCryptGetProperty 失败", status);
			_Status(UpdateStatus::ErrorWhileDownloading);
			co_return;
		}

		std::unique_ptr<uint8_t[]> hashObj = std::make_unique<uint8_t[]>(hashObjSize);

		wil::unique_bcrypt_hash hHash;
		status = BCryptCreateHash(
			hAlg.get(), hHash.put(), hashObj.get(), hashObjSize, NULL, 0, 0);
		if (status != STATUS_SUCCESS) {
			Logger::Get().NTError("BCryptCreateHash 失败", status);
			_Status(UpdateStatus::ErrorWhileDownloading);
			co_return;
		}

		Buffer buffer(64 * 1024);
		uint32_t bytesReceived = 0;
		while (true) {
			IBuffer resultBuffer = co_await httpStream.ReadAsync(buffer, buffer.Capacity(), InputStreamOptions::Partial);

			if (_downloadCancelled) {
				httpStream.Close();
				_Status(UpdateStatus::Available);
				co_return;
			}

			uint32_t bufferSize = resultBuffer.Length();
			if (bufferSize == 0) {
				break;
			}

			if (totalBytes > 0) {
				bytesReceived += bufferSize;
				_downloadProgress = bytesReceived / (double)totalBytes;
				DownloadProgressChanged.Invoke(_downloadProgress);
			}

			const uint8_t* bufferData = resultBuffer.data();

			status = BCryptHashData(hHash.get(), (PUCHAR)bufferData, bufferSize, 0);
			if (status != STATUS_SUCCESS) {
				Logger::Get().NTError("BCryptHashData 失败", status);
				_Status(UpdateStatus::ErrorWhileDownloading);
				co_return;
			}
			
			if (!WriteFile(updatePkg.get(), bufferData, bufferSize, nullptr, nullptr)) {
				Logger::Get().Win32Error("WriteFile 失败");
				_Status(UpdateStatus::ErrorWhileDownloading);
				co_return;
			}
		}

		// 检查哈希
		std::array<uint8_t, MD5_HASH_LENGTH> hash{};
		status = BCryptFinishHash(hHash.get(), hash.data(), (ULONG)hash.size(), 0);
		if (status != STATUS_SUCCESS) {
			Logger::Get().NTError("BCryptFinishHash 失败", status);
			_Status(UpdateStatus::ErrorWhileDownloading);
			co_return;
		}

		if (Md5ToHex(hash.data()) != _binaryHash) {
			Logger::Get().Error("下载失败: 哈希不匹配");
			_Status(UpdateStatus::ErrorWhileDownloading);
			co_return;
		}
	} catch (const hresult_error& e) {
		Logger::Get().Error(StrHelper::Concat("下载失败: ", StrHelper::UTF16ToUTF8(e.message())));
		_Status(UpdateStatus::ErrorWhileDownloading);
		co_return;
	}

	updatePkg.reset();

	// 后台解压 zip
	co_await resume_background();

	// kuba-zip 内部使用 UTF-8 编码
	int ec = zip_extract(
		StrHelper::UTF16ToUTF8(updatePkgPath).c_str(),
		StrHelper::UTF16ToUTF8(CommonSharedConstants::UPDATE_DIR).c_str(),
		nullptr,
		nullptr
	);
	if (ec < 0) {
		Logger::Get().Error(fmt::format("解压失败，错误代码: {}", ec));
		co_await App::Get().Dispatcher();
		_Status(UpdateStatus::ErrorWhileDownloading);
		co_return;
	}

	DeleteFile(updatePkgPath.c_str());

	std::wstring magpieExePath = StrHelper::Concat(CommonSharedConstants::UPDATE_DIR, L"Magpie.exe");
	std::wstring updaterExePath = StrHelper::Concat(CommonSharedConstants::UPDATE_DIR, L"Updater.exe");
	if (!Win32Helper::FileExists(magpieExePath.c_str()) || !Win32Helper::FileExists(updaterExePath.c_str())) {
		Logger::Get().Error("未找到 Magpie.exe 或 Updater.exe");
		co_await App::Get().Dispatcher();
		_Status(UpdateStatus::ErrorWhileDownloading);
		co_return;
	}

	co_await App::Get().Dispatcher();

	if (_downloadCancelled) {
		_Status(UpdateStatus::Available);
		co_return;
	}

	_Status(UpdateStatus::Installing);

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

	std::wstring curVersion = MAGPIE_VERSION.ToString<wchar_t>();
	// 调用 ShellExecuteEx 后立即退出，因此应该指定 SEE_MASK_NOASYNC
	SHELLEXECUTEINFO execInfo{
		.cbSize = sizeof(execInfo),
		.fMask = SEE_MASK_NOASYNC,
		.lpVerb = L"open",
		.lpFile = L"Updater.exe",
		.lpParameters = curVersion.c_str()
	};
	if (!ShellExecuteEx(&execInfo)) {
		Logger::Get().Win32Error("ShellExecuteEx 失败");
	}

	App::Get().Quit();
}

void UpdateService::EnteringAboutPage() {
	if (_status == UpdateStatus::NoUpdate || _status == UpdateStatus::ErrorWhileChecking) {
		_Status(UpdateStatus::Pending);
	}
}

void UpdateService::_MainWindow_Closed() {
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
	StatusChanged.Invoke(value);
}

fire_and_forget UpdateService::_Timer_Tick(ThreadPoolTimer const& timer) {
	if (timer) {
		co_await App::Get().Dispatcher();
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
