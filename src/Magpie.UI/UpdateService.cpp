#include "pch.h"
#include "UpdateService.h"
#include <rapidjson/document.h>
#include "JsonHelper.h"
#include "Logger.h"
#include "StrUtils.h"

using namespace winrt;
using namespace Windows::Storage::Streams;


namespace winrt::Magpie::UI {

IAsyncAction UpdateService::CheckForUpdateAsync() {
	rapidjson::Document doc;

	try {
		HttpClient httpClient;
		IBuffer buffer = co_await httpClient.GetBufferAsync(
			Uri(L"https://raw.githubusercontent.com/Blinue/Magpie/dev/version.json"));

		doc.Parse((const char*)buffer.data(), buffer.Length());
	} catch (const hresult_error& e) {
		Logger::Get().ComError(
			StrUtils::Concat("检查更新失败：", StrUtils::UTF16ToUTF8(e.message())),
			e.code()
		);
		co_return;
	}

	if (!doc.IsObject()) {
		_result = UpdateResult::UnknownError;
		co_return;
	}

	auto root = doc.GetObj();
	auto versionNode = root.FindMember("version");

}

}
