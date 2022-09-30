#pragma once
#include "ScalingModeItem.g.h"


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

	uint32_t Index() const noexcept {
		return _index;
	}

	// 重新排序时被调用
	void Index(uint32_t value) noexcept {
		_index = value;
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

private:
	ScalingMode* _Data() noexcept;
	const ScalingMode* _Data() const noexcept;

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	uint32_t _index = 0;
	IObservableVector<IInspectable> _effects{ nullptr };

	hstring _renameText;
	std::wstring_view _trimedRenameText;
	bool _isRenameButtonEnabled = false;
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ScalingModeItem : ScalingModeItemT<ScalingModeItem, implementation::ScalingModeItem> {
};

}
