#include "pch.h"
#include "ScalingProfileViewModel.h"
#if __has_include("ScalingProfileViewModel.g.cpp")
#include "ScalingProfileViewModel.g.cpp"
#endif
#include "AppSettings.h"
#include "ScalingProfile.h"


namespace winrt::Magpie::App::implementation {

ScalingProfileViewModel::ScalingProfileViewModel(uint32_t profileId) 
	: _profile(profileId == 0 ? AppSettings::Get().DefaultScalingProfile() : AppSettings::Get().ScalingProfiles()[profileId - 1]) {
}

hstring ScalingProfileViewModel::Name() const noexcept {
	return hstring(_profile.Name().empty() ? L"默认" : _profile.Name());
}

void ScalingProfileViewModel::Name(const hstring& value) {
	if (_profile.Name() == value) {
		return;
	}

	_profile.Name(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Name"));
}

int32_t ScalingProfileViewModel::CaptureMode() const noexcept {
	return (int32_t)_profile.MagSettings().CaptureMode();
}

void ScalingProfileViewModel::CaptureMode(int32_t value) {
	if (value < 0) {
		return;
	}

	Magpie::Runtime::CaptureMode captureMode = (Magpie::Runtime::CaptureMode)value;
	if (_profile.MagSettings().CaptureMode() == captureMode) {
		return;
	}

	_profile.MagSettings().CaptureMode(captureMode);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CaptureMode"));
}

bool ScalingProfileViewModel::Is3DGameMode() const noexcept {
	return _profile.MagSettings().Is3DGameMode();
}

void ScalingProfileViewModel::Is3DGameMode(bool value) {
	if (_profile.MagSettings().Is3DGameMode() == value) {
		return;
	}

	_profile.MagSettings().Is3DGameMode(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Is3DGameMode"));
}

int32_t ScalingProfileViewModel::MultiMonitorUsage() const noexcept {
	return (int32_t)_profile.MagSettings().MultiMonitorUsage();
}

void ScalingProfileViewModel::MultiMonitorUsage(int32_t value) {
	if (value < 0) {
		return;
	}

	Magpie::Runtime::MultiMonitorUsage multiMonitorUsage = (Magpie::Runtime::MultiMonitorUsage)value;
	if (_profile.MagSettings().MultiMonitorUsage() == multiMonitorUsage) {
		return;
	}

	_profile.MagSettings().MultiMonitorUsage(multiMonitorUsage);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"MultiMonitorUsage"));
}

}
