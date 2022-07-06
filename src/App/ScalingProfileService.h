#pragma once
#include "ScalingProfile.h"
#include "StrUtils.h"


namespace winrt::Magpie::App {

class ScalingProfileService {
public:
	static ScalingProfileService& Get() {
		static ScalingProfileService instance;
		return instance;
	}

	ScalingProfile& GetProfileForWindow(HWND hWnd);

	ScalingProfile* GetScalingProfile(std::wstring_view name) {
		auto it = _rulesMap.find(name);
		if (it == _rulesMap.end()) {
			return nullptr;
		}

		return it->second;
	}

	ScalingProfile& GetDefaultScalingProfile();

private:
	ScalingProfileService() = default;

	std::unordered_map<std::wstring, ScalingProfile*, StrUtils::StringHash<wchar_t>, std::equal_to<>> _rulesMap;
};

}
