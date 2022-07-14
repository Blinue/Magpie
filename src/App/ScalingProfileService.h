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

	ScalingProfile& GetDefaultScalingProfile();

private:
	ScalingProfileService() = default;
};

}
