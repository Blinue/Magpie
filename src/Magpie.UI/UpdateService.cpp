#include "pch.h"
#include "UpdateService.h"
#include <rapidjson/document.h>
#include "JsonHelper.h"

using namespace winrt;
using namespace Windows::Storage::Streams;


namespace winrt::Magpie::UI {

IAsyncAction UpdateService::CheckForUpdateAsync() {
	HttpClient httpClient;
	IBuffer buffer = co_await httpClient.GetBufferAsync(
		Uri(L"https://raw.githubusercontent.com/Blinue/Magpie/dev/version.json"));

	rapidjson::Document doc;
	doc.Parse((const char*)buffer.data(), buffer.Length());

	if (!doc.IsObject()) {
		_result = UpdateResult::UnknownError;
		co_return;
	}

	auto root = doc.GetObj();
	auto versionNode = root.FindMember("version");

}

}
