#include "pch.h"
#include "UpdateService.h"
#include <rapidjson/document.h>
#include "JsonHelper.h"


namespace winrt::Magpie::UI {

IAsyncAction UpdateService::CheckForUpdateAsync() {
	HttpClient httpClient;
	Windows::Storage::Streams::IBuffer buffer = co_await httpClient.GetBufferAsync(
		Uri(L"https://raw.githubusercontent.com/Blinue/Magpie/dev/version.json"));

	rapidjson::Document doc;
	doc.Parse((char*)buffer.data(), buffer.Length());

	if (!doc.IsObject()) {
		co_return;
	}

	auto root = doc.GetObj();
	auto versionNode = root.FindMember("version");

}

}
