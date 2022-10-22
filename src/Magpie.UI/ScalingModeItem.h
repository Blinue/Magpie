#pragma once
#include "ScalingModeItem.g.h"
#include "WinRTUtils.h"


namespace winrt::Magpie::UI {
struct ScalingMode;
}

namespace winrt::Magpie::UI::implementation {

struct ScalingModeItem : ScalingModeItemT<ScalingModeItem> {
	ScalingModeItem(uint32_t index);

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	void AddEffect(const hstring& fullName);

	hstring Name() const noexcept;

	void Name(const hstring& value) noexcept;

	hstring Description() const noexcept;

	IObservableVector<IInspectable> Effects() const noexcept {
		return _effects;
	}

	hstring RenameText() const noexcept {
		return _renameText;
	}

	void RenameText(const hstring& value) noexcept;

	bool IsRenameButtonEnabled() const noexcept {
		return _isRenameButtonEnabled;
	}

	void RenameFlyout_Opening();

	void RenameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args);

	int32_t RenameTextBoxSelectionStart() {
		return _renameText.size();
	}

	void RenameButton_Click();

	bool CanMoveUp() const noexcept;

	bool CanMoveDown() const noexcept;

	void MoveUp() noexcept;

	void MoveDown() noexcept;

	bool CanReorderEffects() const noexcept {
		return _effects.Size() > 1;
	}

	void Remove();

	IVector<IInspectable> LinkedProfiles() const noexcept {
		return _linkedProfiles;
	}

	bool IsInUse() const noexcept {
		return _linkedProfiles.Size() > 0;
	}

private:
	void _Index(uint32_t value) noexcept;

	void _ScalingModesService_Added();

	void _ScalingModesService_Moved(uint32_t index, bool isMoveUp);

	void _ScalingModesService_Removed(uint32_t index);

	void _Effects_VectorChanged(IObservableVector<IInspectable> const&, IVectorChangedEventArgs const& args);

	void _ScalingModeEffectItem_Removed(IInspectable const&, uint32_t index);

	ScalingMode& _Data() noexcept;
	const ScalingMode& _Data() const noexcept;

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	uint32_t _index = 0;
	IObservableVector<IInspectable> _effects{ nullptr };
	bool _isMovingEffects = true;
	uint32_t _movingFromIdx = 0;

	WinRTUtils::EventRevoker _scalingModeAddedRevoker;
	WinRTUtils::EventRevoker _scalingModeMovedRevoker;
	WinRTUtils::EventRevoker _scalingModeRemovedRevoker;

	hstring _renameText;
	std::wstring_view _trimedRenameText;
	bool _isRenameButtonEnabled = false;
	IVector<IInspectable> _linkedProfiles{ nullptr };
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ScalingModeItem : ScalingModeItemT<ScalingModeItem, implementation::ScalingModeItem> {
};

}
