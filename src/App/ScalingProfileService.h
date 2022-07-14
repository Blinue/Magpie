#pragma once
#include "ScalingProfile.h"
#include "WinRTUtils.h"


namespace winrt::Magpie::App {

class ScalingProfileService {
public:
	static ScalingProfileService& Get() {
		static ScalingProfileService instance;
		return instance;
	}

	bool TestNewProfile(bool isPackaged, std::wstring_view pathOrAumid, std::wstring_view className);

	bool AddProfile(bool isPackaged, std::wstring_view pathOrAumid, std::wstring_view className, std::wstring_view name);

	event_token ProfileAdded(delegate<ScalingProfile&> const& handler) {
		return _profileAddedEvent.add(handler);
	}

	WinRTUtils::EventRevoker ProfileAdded(auto_revoke_t, delegate<ScalingProfile&> const& handler) {
		event_token token = ProfileAdded(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			ProfileAdded(token);
		});
	}

	void ProfileAdded(event_token const& token) {
		_profileAddedEvent.remove(token);
	}

	ScalingProfile& GetProfileForWindow(HWND hWnd);

	ScalingProfile& GetDefaultScalingProfile();

private:
	ScalingProfileService() = default;

	event<delegate<ScalingProfile&>> _profileAddedEvent;
};

}
