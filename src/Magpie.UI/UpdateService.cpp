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

using namespace winrt;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;

namespace winrt::Magpie::UI {

fire_and_forget UpdateService::CheckForUpdatesAsync() {
	_Status(UpdateStatus::Checking);

	rapidjson::Document doc;

	HttpClient httpClient;
	try {
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
			_Status(UpdateStatus::ErrorWhileChecking, UpdateError::Network);
		}
		
		co_return;
	}

	if (!doc.IsObject()) {
		Logger::Get().Error("根元素不是对象");
		_Status(UpdateStatus::ErrorWhileChecking, UpdateError::Logical);
		co_return;
	}

	auto root = doc.GetObj();
	auto versionNode = root.FindMember("version");
	if (versionNode == root.end()) {
		Logger::Get().Error("找不到 version 成员");
		_Status(UpdateStatus::ErrorWhileChecking, UpdateError::Logical);
		co_return;
	}
	if (!versionNode->value.IsString()) {
		Logger::Get().Error("version 成员不是字符串");
		_Status(UpdateStatus::ErrorWhileChecking, UpdateError::Logical);
		co_return;
	}

	Version remoteVersion;
	if(!remoteVersion.Parse(versionNode->value.GetString())) {
		Logger::Get().Error("解析版本号失败");
		_Status(UpdateStatus::ErrorWhileChecking, UpdateError::Logical);
		co_return;
	}

	if (remoteVersion <= MAGPIE_VERSION) {
		_Status(UpdateStatus::NoUpdate);
		co_return;
	}

	auto tagNode = root.FindMember("tag");
	if (tagNode == root.end()) {
		Logger::Get().Error("找不到 tag 成员");
		_Status(UpdateStatus::ErrorWhileChecking, UpdateError::Logical);
		co_return;
	}
	if (!tagNode->value.IsString()) {
		Logger::Get().Error("tag 成员不是字符串");
		_Status(UpdateStatus::ErrorWhileChecking, UpdateError::Logical);
		co_return;
	}
	_tag = StrUtils::UTF8ToUTF16(tagNode->value.GetString());
	if (_tag.empty()) {
		Logger::Get().Error("tag 成员为空");
		_Status(UpdateStatus::ErrorWhileChecking, UpdateError::Logical);
		co_return;
	}

	auto binaryNode = root.FindMember("binary");
	if (binaryNode == root.end()) {
		Logger::Get().Error("找不到 binary 成员");
		_Status(UpdateStatus::ErrorWhileChecking, UpdateError::Logical);
		co_return;
	}
	if (!binaryNode->value.IsObject()) {
		Logger::Get().Error("binary 成员不是对象");
		_Status(UpdateStatus::ErrorWhileChecking, UpdateError::Logical);
		co_return;
	}
	auto binaryObj = binaryNode->value.GetObj();
	auto x64Node = binaryObj.FindMember("x64");
	if (x64Node == binaryObj.end()) {
		Logger::Get().Error("找不到 x64 成员");
		_Status(UpdateStatus::ErrorWhileChecking, UpdateError::Logical);
		co_return;
	}
	if (!x64Node->value.IsString()) {
		Logger::Get().Error("x64 成员不是字符串");
		_Status(UpdateStatus::ErrorWhileChecking, UpdateError::Logical);
		co_return;
	}

	_binaryUrl = StrUtils::UTF8ToUTF16(x64Node->value.GetString());
	if (_binaryUrl.empty()) {
		Logger::Get().Error("x64 成员为空");
		_Status(UpdateStatus::ErrorWhileChecking, UpdateError::Logical);
		co_return;
	}

	_Status(UpdateStatus::Available);

	/*const std::wstring x64BinaryUrl = StrUtils::UTF8ToUTF16(x64Node->value.GetString());
	try {
		auto progressOp1 = httpClient.TryGetInputStreamAsync(Uri(x64BinaryUrl));
		uint64_t totalBytes = 0;
		progressOp1.Progress([&totalBytes](const auto&, const HttpProgress& progress) {
			if (std::optional<uint64_t> totalBytesToReceive = progress.TotalBytesToReceive) {
				totalBytes = *totalBytesToReceive;
			}
		});
		HttpGetInputStreamResult httpResult = co_await progressOp1;

		wchar_t curDir[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, curDir);
		StorageFolder curFolder = co_await StorageFolder::GetFolderFromPathAsync(curDir);
		StorageFolder updateFolder = co_await curFolder.CreateFolderAsync(
			CommonSharedConstants::UPDATE_DIR, CreationCollisionOption::OpenIfExists);

		StorageFile updatePkg = co_await updateFolder.CreateFileAsync(
			L"update.zip",
			CreationCollisionOption::ReplaceExisting
		);
		IRandomAccessStream fileStream = co_await updatePkg.OpenAsync(FileAccessMode::ReadWrite);

		auto progressOp2 = httpResult.ResponseMessage().Content().WriteToStreamAsync(fileStream);
		if (totalBytes > 0) {
			progressOp2.Progress([totalBytes](const auto&, uint64_t readed) {
				OutputDebugString(fmt::format(L"{}\n", readed * 100.0 / totalBytes).c_str());
			});
		}
		co_await progressOp2;

		fileStream.Close();
	} catch (const hresult_error& e) {
		_result = UpdateResult::NetworkError;
		OutputDebugString(e.message().c_str());
		co_return;
	}

	co_await resume_background();
	
	std::wstring updatePkg = CommonSharedConstants::UPDATE_DIR + L"\\update.zip"s;
	int ec = zip_extract(
		StrUtils::UTF16ToUTF8(updatePkg).c_str(),
		StrUtils::UTF16ToUTF8(CommonSharedConstants::UPDATE_DIR).c_str(),
		nullptr,
		nullptr
	);
	if (ec < 0) {
		_result = UpdateResult::UnknownError;
		co_return;
	}

	DeleteFile(updatePkg.c_str());
	*/
}

void UpdateService::LeavingAboutPage() {
	if (_status == UpdateStatus::NoUpdate || _status == UpdateStatus::ErrorWhileChecking) {
		_Status(UpdateStatus::Pending);
	}
}

void UpdateService::ClosingMainWindow() {
	LeavingAboutPage();

	if (_status == UpdateStatus::Available) {
		_Status(UpdateStatus::Pending);
	}
}

void UpdateService::Cancel() {
	switch (_status) {
	case UpdateStatus::Pending:
		break;
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
		break;
	default:
		break;
	}
}

void UpdateService::_Status(UpdateStatus value, UpdateError error) {
	if (_status == value) {
		return;
	}

	_status = value;
	_error = error;
	_statusChangedEvent(value);
}

}
