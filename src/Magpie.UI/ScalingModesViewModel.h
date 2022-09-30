#pragma once
#include "ScalingModesViewModel.g.h"
#include <WinRTUtils.h>


namespace winrt::Magpie::UI::implementation {

struct ScalingModesViewModel : ScalingModesViewModelT<ScalingModesViewModel> {
	ScalingModesViewModel();

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	IObservableVector<IInspectable> ScalingModes() const noexcept {
		return _scalingModes;
	}

	void PrepareForAdd();

	hstring NewScalingModeName() const noexcept {
		return _newScalingModeName;
	}

	void NewScalingModeName(const hstring& value) noexcept;

	IVector<IInspectable> NewScalingModeCopyFromList() const noexcept {
		return _newScalingModeCopyFromList;
	}

	int32_t NewScalingModeCopyFrom() const noexcept {
		return _newScalingModeCopyFrom;
	}

	void NewScalingModeCopyFrom(int32_t value) noexcept {
		_newScalingModeCopyFrom = value;
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"NewScalingModeCopyFrom"));
	}
	
	bool IsAddButtonEnabled() const noexcept {
		return !_newScalingModeName.empty();
	}

	void AddScalingMode();

private:
	void _ScalingModesService_Reordered(uint32_t index, bool isMoveUp);

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	IObservableVector<IInspectable> _scalingModes{ nullptr };

	WinRTUtils::EventRevoker _scalingModesReorderdRevoker;

	hstring _newScalingModeName;
	IVector<IInspectable> _newScalingModeCopyFromList{ nullptr };
	int32_t _newScalingModeCopyFrom = 0;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ScalingModesViewModel : ScalingModesViewModelT<ScalingModesViewModel, implementation::ScalingModesViewModel> {
};

}
