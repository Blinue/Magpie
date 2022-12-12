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
#include <winrt/Windows.Networking.BackgroundTransfer.h>

using namespace winrt;
using namespace Windows::Networking::BackgroundTransfer;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;

namespace winrt::Magpie::UI {

IAsyncAction UpdateService::CheckForUpdateAsync() {
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

	if (!AppSettings::Get().IsAutoDownloadUpdates()) {
		_result = UpdateResult::Available;
		co_return;
	}

	auto binaryNode = root.FindMember("binary");
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
	auto progressOp = httpClient.GetInputStreamAsync(Uri(x64BinaryUrl));
	progressOp.Progress([](const auto&, const HttpProgress& progress) {
		std::optional<uint64_t> totalBytes = progress.TotalBytesToReceive;
		if (!totalBytes.has_value()) {
			return;
		}

		OutputDebugString(fmt::format(L"{}\n", progress.BytesReceived * 100.0 / totalBytes.value()).c_str());
	});
	IInputStream stream = co_await progressOp;

	Buffer buffer(1024);
	wchar_t curDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, curDir);
	StorageFolder updateFoler = co_await StorageFolder::GetFolderFromPathAsync(
		StrUtils::ConcatW(curDir, L"\\", CommonSharedConstants::UPDATE_DIR)
	);
	StorageFile updatePkg = co_await updateFoler.CreateFileAsync(
		L"update.zip",
		CreationCollisionOption::ReplaceExisting
	);
	IRandomAccessStream fileStream = co_await updatePkg.OpenAsync(FileAccessMode::ReadWrite);
	
	while (true) {
		IBuffer resultBuffer = co_await stream.ReadAsync(buffer, buffer.Capacity(), InputStreamOptions::Partial);
		if (resultBuffer.Length() == 0) {
			break;
		}

		co_await fileStream.WriteAsync(resultBuffer);
	}
}

}
