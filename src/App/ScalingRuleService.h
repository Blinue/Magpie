#pragma once
#include "ScalingRule.h"
#include "AppSettings.h"
#include "StrUtils.h"


namespace winrt::Magpie::App {

class ScalingRuleService {
public:
	static ScalingRuleService& Get() {
		static ScalingRuleService instance;
		return instance;
	}

	ScalingRule& GetRuleForWindow(HWND hWnd);

	ScalingRule* GetScalingRule(std::wstring_view name) {
		auto it = _rulesMap.find(name);
		if (it == _rulesMap.end()) {
			return nullptr;
		}

		return it->second;
	}

	ScalingRule& GetDefaultScalingRule() {
		return AppSettings::Get().DefaultScalingRule();
	}

private:
	ScalingRuleService() = default;

	std::unordered_map<std::wstring, ScalingRule*, StrUtils::StringHash<wchar_t>, std::equal_to<>> _rulesMap;
};

}
