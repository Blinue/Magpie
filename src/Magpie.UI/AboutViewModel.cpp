#include "pch.h"
#include "AboutViewModel.h"
#if __has_include("AboutViewModel.g.cpp")
#include "AboutViewModel.g.cpp"
#endif
#include "Version.h"
#include "UpdateService.h"
#include "AppSettings.h"

namespace winrt::Magpie::UI::implementation {

hstring AboutViewModel::Version() const noexcept {
	return MAGPIE_TAG_W;
}

Uri AboutViewModel::ReleaseNotesLink() const noexcept {
	return Uri(StrUtils::ConcatW(L"https://github.com/Blinue/Magpie/releases/tag/", MAGPIE_TAG_W));
}

fire_and_forget AboutViewModel::CheckForUpdates() {
	_isCheckingForUpdates = true;
	_updateStatus = -1;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsCheckingForUpdates"));

	co_await UpdateService::Get().CheckForUpdatesAsync();

	_isCheckingForUpdates = false;
	_updateStatus = (int)UpdateService::Get().GetResult();
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsCheckingForUpdates"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsNetworkError"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsUnknownError"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsNoUpdate"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsAvailable"));

	if (IsAvailable()) {
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"UpdateReleaseNotesLink"));
	}
}

bool AboutViewModel::IsCheckForPreviewUpdates() const noexcept {
	return AppSettings::Get().IsCheckForPreviewUpdates();
}

void AboutViewModel::IsCheckForPreviewUpdates(bool value) noexcept {
	AppSettings::Get().IsCheckForPreviewUpdates(value);
}

bool AboutViewModel::IsAutoCheckForUpdates() const noexcept {
	return AppSettings::Get().IsAutoCheckForUpdates();
}

void AboutViewModel::IsAutoCheckForUpdates(bool value) noexcept {
	AppSettings::Get().IsAutoCheckForUpdates(value);
}

bool AboutViewModel::IsNetworkError() const noexcept {
	return _updateStatus == (int)UpdateResult::NetworkError;
}

bool AboutViewModel::IsUnknownError() const noexcept {
	return _updateStatus == (int)UpdateResult::UnknownError;
}

bool AboutViewModel::IsNoUpdate() const noexcept {
	return _updateStatus == (int)UpdateResult::NoUpdate;
}

bool AboutViewModel::IsAvailable() const noexcept {
	return _updateStatus == (int)UpdateResult::Available;
}

Uri AboutViewModel::UpdateReleaseNotesLink() const noexcept {
	if (!IsAvailable()) {
		return nullptr;
	}

	return Uri(StrUtils::ConcatW(L"https://github.com/Blinue/Magpie/releases/tag/",
		UpdateService::Get().Tag()));
}

}
