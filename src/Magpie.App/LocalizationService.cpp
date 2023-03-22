#include "pch.h"
#include "LocalizationService.h"
#include "AppSettings.h"
#include <winrt/Windows.System.UserProfile.h>
#include "StrUtils.h"
#include <bcp47mrm.h>

#pragma comment(lib, "bcp47mrm.lib")

using namespace winrt;
using namespace Windows::ApplicationModel::Resources;
using namespace Windows::ApplicationModel::Resources::Core;

namespace winrt::Magpie::App {

void LocalizationService::EarlyInitialize() {
	// 非打包应用默认使用“Windows 显示语言”，这里自行切换至“首选语言”
	std::wstring userLanguages;
	for (const hstring& language : UserProfile::GlobalizationPreferences::Languages()) {
		userLanguages += language;
		userLanguages += L'\0';
	}
	// 要求双空结尾
	userLanguages += L'\0';

	double bestScore = 0.0;
	// 没有支持的语言则回落到英语
	std::wstring_view bestLanguage = L"en-US";
	for (const std::wstring& language : SupportedLanguages()) {
		double score = 0.0;
		HRESULT hr = GetDistanceOfClosestLanguageInList(language.c_str(), userLanguages.data(), 0, &score);
		if (FAILED(hr)) {
			continue;
		}

		if (score > bestScore) {
			bestScore = score;
			bestLanguage = language;

			if (score == 1.0) {
				break;
			}
		}
	}

	ResourceContext::SetGlobalQualifierValue(L"Language", bestLanguage);
}

void LocalizationService::Initialize() {
	AppSettings& settings = AppSettings::Get();

	int language = settings.Language();
	if (language >= 0) {
		ResourceContext::SetGlobalQualifierValue(L"Language", SupportedLanguages()[language]);
	}
}

const std::vector<std::wstring>& LocalizationService::SupportedLanguages() noexcept {
	static std::vector<std::wstring> languages;
	if (!languages.empty()) {
		return languages;
	}

	// 从资源文件中查找所有支持的语言
	ResourceMap resourceMap = ResourceManager::Current().MainResourceMap().GetSubtree(L"Resources");
	auto candidates = (*resourceMap.begin()).Value().Candidates();
	// 将 candidates 中的内容读取到 std::vector 中，直接枚举会导致崩溃，可能因为它是异步加载的
	std::vector<ResourceCandidate> candidatesVec(candidates.Size(), nullptr);
	candidatesVec.resize(candidates.GetMany(0, candidatesVec), nullptr);

	for (const ResourceCandidate& candidate : candidatesVec) {
		std::wstring language(candidate.GetQualifierValue(L"Language"));
		// 转换为小写
		StrUtils::ToLowerCase(language);
		languages.push_back(std::move(language));
	}

	// 以字典顺序排序
	std::sort(languages.begin(), languages.end());
	return languages;
}

}
