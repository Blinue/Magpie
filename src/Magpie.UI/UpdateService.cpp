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

IAsyncAction UpdateService::CheckForUpdatesAsync() {
	rapidjson::Document doc;

	HttpClient httpClient;
	try {
		IBuffer buffer = co_await httpClient.GetBufferAsync(
			Uri(L"https://raw.githubusercontent.com/Blinue/Magpie/dev/version.json"));

		doc.Parse((const char*)buffer.data(), buffer.Length());
	} catch (const hresult_error& e) {
		Logger::Get().ComError(
			StrUtils::Concat("检查更新失败：", StrUtils::UTF16ToUTF8(e.message())),
			e.code()
		);
		_result = UpdateResult::NetworkError;
		co_return;
	}

	if (!doc.IsObject()) {
		Logger::Get().Error("根元素不是对象");
		_result = UpdateResult::UnknownError;
		co_return;
	}

	auto root = doc.GetObj();
	auto versionNode = root.FindMember("version");
	if (versionNode == root.end()) {
		Logger::Get().Error("找不到 version 成员");
		_result = UpdateResult::UnknownError;
		co_return;
	}
	if (!versionNode->value.IsString()) {
		Logger::Get().Error("version 成员不是字符串");
		_result = UpdateResult::UnknownError;
		co_return;
	}

	Version remoteVersion;
	if(!remoteVersion.Parse(versionNode->value.GetString())) {
		Logger::Get().Error("解析版本号失败");
		_result = UpdateResult::UnknownError;
		co_return;
	}

	if (remoteVersion <= MAGPIE_VERSION) {
		_result = UpdateResult::NoUpdate;
		co_return;
	}

	_result = UpdateResult::Available;

	/*auto binaryNode = root.FindMember("binary");
	if (binaryNode == root.end()) {
		Logger::Get().Error("找不到 binary 成员");
		_result = UpdateResult::UnknownError;
		co_return;
	}
	if (!binaryNode->value.IsObject()) {
		Logger::Get().Error("binary 成员不是对象");
		_result = UpdateResult::UnknownError;
		co_return;
	}
	auto binaryObj = binaryNode->value.GetObj();
	auto x64Node = binaryObj.FindMember("x64");
	if (x64Node == binaryObj.end()) {
		Logger::Get().Error("找不到 x64 成员");
		_result = UpdateResult::UnknownError;
		co_return;
	}
	if (!x64Node->value.IsString()) {
		Logger::Get().Error("x64 成员不是字符串");
		_result = UpdateResult::UnknownError;
		co_return;
	}

	const std::wstring x64BinaryUrl = StrUtils::UTF8ToUTF16(x64Node->value.GetString());
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

}
