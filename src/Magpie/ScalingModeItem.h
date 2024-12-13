#pragma once
#include "ScalingModeItem.g.h"
#include "WinRTHelper.h"
#include "ScalingModesService.h"

namespace winrt::Magpie {
struct ScalingMode;
}

namespace winrt::Magpie::implementation {

struct ScalingModeItem : ScalingModeItemT<ScalingModeItem>,
                         wil::notify_property_changed_base<ScalingModeItem> {
	ScalingModeItem(uint32_t index, bool isInitialExpanded);

	void AddEffect(const hstring& fullName);

	bool IsInitialExpanded() const noexcept {
		return _isInitialExpanded;
	}

	hstring Name() const noexcept;

	void Name(const hstring& value) noexcept;

	hstring Description() const noexcept;

	bool HasUnkownEffects() const noexcept;

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

	int RenameTextBoxSelectionStart() {
		return _renameText.size();
	}

	void RenameButton_Click();

	bool CanMoveUp() const noexcept;

	bool CanMoveDown() const noexcept;

	void MoveUp() noexcept;

	void MoveDown() noexcept;

	bool CanReorderEffects() const noexcept;

	bool IsShowMoveButtons() const noexcept;

	void Remove();

	IVector<IInspectable> LinkedProfiles() const noexcept {
		return _linkedProfiles;
	}

	bool IsInUse() const noexcept {
		return _linkedProfiles.Size() > 0;
	}

private:
	void _Index(uint32_t value) noexcept;

	void _ScalingModesService_Added(EffectAddedWay);

	void _ScalingModesService_Moved(uint32_t index, bool isMoveUp);

	void _ScalingModesService_Removed(uint32_t index);

	void _Effects_VectorChanged(IObservableVector<IInspectable> const&, IVectorChangedEventArgs const& args);

	void _ScalingModeEffectItem_Removed(IInspectable const&, uint32_t index);

	void _ScalingModeEffectItem_Moved(ScalingModeEffectItem const& sender, bool isUp);

	ScalingModeEffectItem _CreateScalingModeEffectItem(uint32_t scalingModeIdx, uint32_t effectIdx);

	ScalingMode& _Data() noexcept;
	const ScalingMode& _Data() const noexcept;

	uint32_t _index = 0;
	IObservableVector<IInspectable> _effects{ nullptr };
	
	uint32_t _movingFromIdx = 0;

	WinRTHelper::EventRevoker _scalingModeAddedRevoker;
	WinRTHelper::EventRevoker _scalingModeMovedRevoker;
	WinRTHelper::EventRevoker _scalingModeRemovedRevoker;

	hstring _renameText;
	std::wstring_view _trimedRenameText;
	
	IVector<IInspectable> _linkedProfiles{ nullptr };

	bool _isMovingEffects = true;
	bool _isRenameButtonEnabled = false;
	bool _isInitialExpanded = false;
};

}

namespace winrt::Magpie::factory_implementation {

struct ScalingModeItem : ScalingModeItemT<ScalingModeItem, implementation::ScalingModeItem> {
};

}
