#pragma once
#include "ScalingModesViewModel.g.h"
#include "WinRTUtils.h"
#include "ScalingModesService.h"

namespace winrt::Magpie::App::implementation {

struct ScalingModesViewModel : ScalingModesViewModelT<ScalingModesViewModel>,
                               wil::notify_property_changed_base<ScalingModesViewModel> {
	ScalingModesViewModel();

	void Export() const noexcept;

	void Import() {
		_Import(false);
	}

	void ImportLegacy() {
		_Import(true);
	}

	bool ShowErrorMessage() const noexcept {
		return _showErrorMessage;
	}

	void ShowErrorMessage(bool value) {
		_showErrorMessage = value;
		RaisePropertyChanged(L"ShowErrorMessage");
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

	int NewScalingModeCopyFrom() const noexcept {
		return _newScalingModeCopyFrom;
	}

	void NewScalingModeCopyFrom(int value) noexcept {
		_newScalingModeCopyFrom = value;
		RaisePropertyChanged(L"NewScalingModeCopyFrom");
	}
	
	bool IsAddButtonEnabled() const noexcept {
		return !_newScalingModeName.empty();
	}

	void AddScalingMode();

private:
	fire_and_forget _AddScalingModes(bool isInitialExpanded = false);

	void _ScalingModesService_Added(EffectAddedWay way);

	void _ScalingModesService_Moved(uint32_t index, bool isMoveUp);

	void _ScalingModesService_Removed(uint32_t index);

	void _Import(bool legacy);

	IObservableVector<IInspectable> _scalingModes = single_threaded_observable_vector<IInspectable>();

	WinRTUtils::EventRevoker _scalingModeAddedRevoker;
	WinRTUtils::EventRevoker _scalingModeMovedRevoker;
	WinRTUtils::EventRevoker _scalingModeRemovedRevoker;

	hstring _newScalingModeName;
	IVector<IInspectable> _newScalingModeCopyFromList{ nullptr };
	int _newScalingModeCopyFrom = 0;

	bool _showErrorMessage = false;
	bool _addingScalingModes = false;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ScalingModesViewModel : ScalingModesViewModelT<ScalingModesViewModel, implementation::ScalingModesViewModel> {
};

}
