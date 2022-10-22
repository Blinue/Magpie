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
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ShowErrorMessage"));
	}

	IVector<IInspectable> DownscalingEffects() const noexcept {
		return _downscalingEffects;
	}

	int DownscalingEffectIndex() const noexcept {
		return _downscalingEffectIndex;
	}

	void DownscalingEffectIndex(int value);

	bool DownscalingEffectHasParameters() noexcept;

	Magpie::UI::EffectParametersViewModel DownscalingEffectParameters() const noexcept {
		// 默认构造表示降采样效果参数
		// 每次调用都返回一个新的实例，因为此时降采样效果已更改
		return {};
	}

	Animation::TransitionCollection ScalingModesListTransitions() const noexcept {
		return _scalingModesListTransitions;
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
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"NewScalingModeCopyFrom"));
	}
	
	bool IsAddButtonEnabled() const noexcept {
		return !_newScalingModeName.empty();
	}

	void AddScalingMode();

private:
	fire_and_forget _AddScalingModes();

	void _ScalingModesService_Added();

	void _ScalingModesService_Moved(uint32_t index, bool isMoveUp);

	void _ScalingModesService_Removed(uint32_t index);

	void _Import(bool legacy);

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	Animation::TransitionCollection _scalingModesListTransitions;

	IVector<IInspectable> _downscalingEffects{ nullptr };
	// (FullName, 小写 DisplayName)
	std::vector<std::pair<std::wstring, std::wstring>> _downscalingEffectNames;
	IObservableVector<IInspectable> _scalingModes = single_threaded_observable_vector<IInspectable>();

	WinRTUtils::EventRevoker _scalingModeAddedRevoker;
	WinRTUtils::EventRevoker _scalingModeMovedRevoker;
	WinRTUtils::EventRevoker _scalingModeRemovedRevoker;

	hstring _newScalingModeName;
	IVector<IInspectable> _newScalingModeCopyFromList{ nullptr };
	int _newScalingModeCopyFrom = 0;

	int _downscalingEffectIndex = 0;
	bool _showErrorMessage = false;

	bool _addingScalingModes = false;
	bool _scalingModesInitialized = false;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ScalingModesViewModel : ScalingModesViewModelT<ScalingModesViewModel, implementation::ScalingModesViewModel> {
};

}
