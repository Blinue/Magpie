#include "pch.h"
#include "ScalingModeItem.h"
#if __has_include("ScalingModeItem.g.cpp")
#include "ScalingModeItem.g.cpp"
#endif
#include "ScalingMode.h"
#include "ScalingModesService.h"
#include "StrUtils.h"
#include "XamlUtils.h"
#include "AppSettings.h"

using namespace Magpie::Core;


static std::wstring_view GetEffectDisplayName(std::wstring_view fullName) {
	size_t delimPos = fullName.find_last_of(L'\\');
	return delimPos != std::wstring::npos ? fullName.substr(delimPos + 1) : fullName;
}

namespace winrt::Magpie::UI::implementation {

ScalingModeItem::ScalingModeItem(uint32_t index) {
	_index = index;
	ScalingMode* data = _Data();

	{
		std::vector<IInspectable> effects;
		effects.reserve(data->effects.size());
		for (const EffectOption& effect : data->effects) {
			effects.push_back(box_value(GetEffectDisplayName(effect.name)));
		}
		_effects = single_threaded_observable_vector(std::move(effects));
	}

	{
		std::vector<IInspectable> linkedProfiles;
		const ScalingProfile& defaultProfile = AppSettings::Get().DefaultScalingProfile();
		if (defaultProfile.scalingMode == (int)index) {
			linkedProfiles.push_back(box_value(L"默认"));
		}
		for (const ScalingProfile& profile : AppSettings::Get().ScalingProfiles()) {
			if (profile.scalingMode == (int)index) {
				linkedProfiles.push_back(box_value(profile.name));
			}
		}
		_linkedProfiles = single_threaded_vector(std::move(linkedProfiles));
	}

	_scalingModeAddedRevoker = ScalingModesService::Get().ScalingModeAdded(
		auto_revoke, { this, &ScalingModeItem::_ScalingModesService_Added });
	_scalingModeMovedRevoker = ScalingModesService::Get().ScalingModeMoved(
		auto_revoke, { this, &ScalingModeItem::_ScalingModesService_Moved });
	_scalingModeRemovedRevoker = ScalingModesService::Get().ScalingModeRemoved(
		auto_revoke, { this, &ScalingModeItem::_ScalingModesService_Removed });
}

void ScalingModeItem::_Index(uint32_t value) noexcept {
	_index = value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveUp"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveDown"));
}

void ScalingModeItem::_ScalingModesService_Added() {
	if (_index + 2 == ScalingModesService::Get().GetScalingModeCount()) {
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveDown"));
	}
}

void ScalingModeItem::_ScalingModesService_Moved(uint32_t index, bool isMoveUp) {
	uint32_t targetIndex = isMoveUp ? index - 1 : index + 1;
	if (_index == index) {
		_Index(targetIndex);
	} else if (_index == targetIndex) {
		_Index(index);
	}
}

void ScalingModeItem::_ScalingModesService_Removed(uint32_t index) {
	if (_index > index) {
		_Index(_index - 1);
	} else {
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveUp"));
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanMoveDown"));
	}
}

void ScalingModeItem::AddEffect(const hstring& fullName) {
	EffectOption& effect = _Data()->effects.emplace_back();
	effect.name = fullName;
	_effects.Append(box_value(GetEffectDisplayName(fullName)));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CanReorderEffects"));
}

hstring ScalingModeItem::Name() const noexcept {
	return hstring(_Data()->name);
}

void ScalingModeItem::Name(const hstring& value) noexcept {
	_Data()->name = value;
}

hstring ScalingModeItem::Description() const noexcept {
	std::wstring result;
	for (const EffectOption& effect : _Data()->effects) {
		if (!result.empty()) {
			result.append(L" > ");
		}

		size_t delimPos = effect.name.find_last_of(L'\\');
		if (delimPos == std::wstring::npos) {
			result += effect.name;
		} else {
			result += effect.name.substr(delimPos + 1);
		}
	}
	return hstring(result);
}

void ScalingModeItem::RenameText(const hstring& value) noexcept {
	_renameText = value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"RenameText"));

	_trimedRenameText = value;
	StrUtils::Trim(_trimedRenameText);
	bool newEnabled = !_trimedRenameText.empty() && _trimedRenameText != _Data()->name;
	if (_isRenameButtonEnabled != newEnabled) {
		_isRenameButtonEnabled = newEnabled;
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsRenameButtonEnabled"));
	}
}

void ScalingModeItem::RenameFlyout_Opening() {
	RenameText(hstring(_Data()->name));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"RenameTextBoxSelectionStart"));
}

void ScalingModeItem::RenameTextBox_KeyDown(IInspectable const&, Input::KeyRoutedEventArgs const& args) {
	if (args.Key() == VirtualKey::Enter) {
		RenameButton_Click();
	}
}

void ScalingModeItem::RenameButton_Click() {
	if (!_isRenameButtonEnabled) {
		return;
	}

	XamlUtils::CloseXamlPopups(Application::Current().as<App>().MainPage().XamlRoot());

	_Data()->name = _trimedRenameText;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Name"));
}

bool ScalingModeItem::CanMoveUp() const noexcept {
	return _index != 0;
}

bool ScalingModeItem::CanMoveDown() const noexcept {
	return _index + 1 < ScalingModesService::Get().GetScalingModeCount();
}

void ScalingModeItem::MoveUp() noexcept {
	ScalingModesService::Get().MoveScalingMode(_index, true);
}

void ScalingModeItem::MoveDown() noexcept {
	ScalingModesService::Get().MoveScalingMode(_index, false);
}

void ScalingModeItem::Remove() {
	ScalingModesService::Get().RemoveScalingMode(_index);
}

ScalingMode* ScalingModeItem::_Data() noexcept {
	return &ScalingModesService::Get().GetScalingMode(_index);
}

const ScalingMode* ScalingModeItem::_Data() const noexcept {
	return &ScalingModesService::Get().GetScalingMode(_index);
}

}
