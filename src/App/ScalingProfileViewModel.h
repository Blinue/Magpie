#pragma once
#include "ScalingProfileViewModel.g.h"


namespace winrt::Magpie::App {
class ScalingProfile;
}

namespace winrt::Magpie::App::implementation {

struct ScalingProfileViewModel : ScalingProfileViewModelT<ScalingProfileViewModel> {
	ScalingProfileViewModel(uint32_t profileId);

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	hstring Name() const noexcept;
	void Name(const hstring& value);

	int32_t CaptureMode() const noexcept;
	void CaptureMode(int32_t value);

	bool Is3DGameMode() const noexcept;
	void Is3DGameMode(bool value);

	int32_t MultiMonitorUsage() const noexcept;
	void MultiMonitorUsage(int32_t value);

private:
	event<PropertyChangedEventHandler> _propertyChangedEvent;
	ScalingProfile& _profile;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ScalingProfileViewModel : ScalingProfileViewModelT<ScalingProfileViewModel, implementation::ScalingProfileViewModel> {
};

}
