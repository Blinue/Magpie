#include "pch.h"
#include "ScalingModeItem.h"
#if __has_include("ScalingModeItem.g.cpp")
#include "ScalingModeItem.g.cpp"
#endif
#include "ScalingMode.h"
#include "StrHelper.h"
#include "XamlHelper.h"
#include "AppSettings.h"
#include "EffectsService.h"
#include "EffectHelper.h"
#include "CommonSharedConstants.h"
#include "App.h"
#include "ScalingModeEffectItem.h"
#include "Win32Helper.h"
#include "RootPage.h"

using namespace ::Magpie;

namespace winrt::Magpie::implementation {

static ScalingModeEffectItem& GetEffectItemImpl(const IInspectable& item) noexcept {
	return *get_self<ScalingModeEffectItem>(item.as<winrt::Magpie::ScalingModeEffectItem>());
}

ScalingModeItem::ScalingModeItem(uint32_t index, bool isInitialExpanded)
	: _index(index), _isInitialExpanded(isInitialExpanded)
{
	{
		std::vector<IInspectable> linkedProfiles;
		const Profile& defaultProfile = AppSettings::Get().DefaultProfile();
		if (defaultProfile.scalingMode == (int)index) {
			hstring defaults = ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID)
				.GetString(L"Root_Defaults/Content");
			linkedProfiles.push_back(box_value(defaults));
		}
		for (const Profile& profile : AppSettings::Get().Profiles()) {
			if (profile.scalingMode == (int)index) {
				linkedProfiles.push_back(box_value(profile.name));
			}
		}
		_linkedProfiles = single_threaded_vector(std::move(linkedProfiles));
	}

	_scalingModeAddedRevoker = ScalingModesService::Get().ScalingModeAdded(
		auto_revoke, std::bind_front(&ScalingModeItem::_ScalingModesService_Added, this));
	_scalingModeMovedRevoker = ScalingModesService::Get().ScalingModeMoved(
		auto_revoke, std::bind_front(&ScalingModeItem::_ScalingModesService_Moved, this));
	_scalingModeRemovedRevoker = ScalingModesService::Get().ScalingModeRemoved(
		auto_revoke, std::bind_front(&ScalingModeItem::_ScalingModesService_Removed, this));

	ScalingMode& data = _Data();
	{
		std::vector<IInspectable> effects;
		effects.reserve(data.effects.size());
		for (uint32_t i = 0; i < data.effects.size(); ++i) {
			effects.push_back(*_CreateScalingModeEffectItem(_index, i));
		}
		_effects = single_threaded_observable_vector(std::move(effects));
	}
	_effects.VectorChanged({ this, &ScalingModeItem::_Effects_VectorChanged });
}

void ScalingModeItem::_Index(uint32_t value) noexcept {
	_index = value;
	for (const IInspectable& item : _effects) {
		GetEffectItemImpl(item).ScalingModeIdx(value);
	}
	RaisePropertyChanged(L"CanMoveUp");
	RaisePropertyChanged(L"CanMoveDown");
}

void ScalingModeItem::_ScalingModesService_Added(EffectAddedWay) {
	if (_index + 2 == ScalingModesService::Get().GetScalingModeCount()) {
		RaisePropertyChanged(L"CanMoveDown");
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
		RaisePropertyChanged(L"CanMoveUp");
		RaisePropertyChanged(L"CanMoveDown");
	}
}

void ScalingModeItem::_Effects_VectorChanged(IObservableVector<IInspectable> const&, IVectorChangedEventArgs const& args) {
	if (!_isMovingEffects) {
		RaisePropertyChanged(L"Description");
		RaisePropertyChanged(L"CanReorderEffects");
		RaisePropertyChanged(L"IsShowMoveButtons");
		return;
	}
	
	// 移动元素时先删除再插入
	if (args.CollectionChange() == CollectionChange::ItemRemoved) {
		_movingFromIdx = args.Index();
		return;
	}
	
	assert(args.CollectionChange() == CollectionChange::ItemInserted);
	uint32_t movingToIdx = args.Index();

	std::vector<EffectOption>& effects = _Data().effects;
	EffectOption removedEffect = std::move(effects[_movingFromIdx]);
	effects.erase(effects.begin() + _movingFromIdx);
	effects.emplace(effects.begin() + movingToIdx, std::move(removedEffect));

	uint32_t minIdx, maxIdx;
	if (_movingFromIdx < movingToIdx) {
		minIdx = _movingFromIdx;
		maxIdx = movingToIdx;
	} else {
		minIdx = movingToIdx;
		maxIdx = _movingFromIdx;
	}
	
	for (uint32_t i = minIdx; i <= maxIdx; ++i) {
		GetEffectItemImpl(_effects.GetAt(i)).EffectIdx(i);
	}

	RaisePropertyChanged(L"Description");
	AppSettings::Get().SaveAsync();
}

void ScalingModeItem::_ScalingModeEffectItem_Removed(uint32_t index) {
	std::vector<EffectOption>& effects = _Data().effects;
	effects.erase(effects.begin() + index);

	_isMovingEffects = false;
	_effects.RemoveAt(index);
	_isMovingEffects = true;

	for (uint32_t i = index; i < effects.size(); ++i) {
		GetEffectItemImpl(_effects.GetAt(i)).EffectIdx(i);
	}

	if (index > 0) {
		GetEffectItemImpl(_effects.GetAt(index - 1)).RefreshMoveState();
	}
	if (index < _effects.Size()) {
		GetEffectItemImpl(_effects.GetAt(index)).RefreshMoveState();
	}

	RaisePropertyChanged(L"HasUnkownEffects");

	AppSettings::Get().SaveAsync();
}

void ScalingModeItem::_ScalingModeEffectItem_Moved(ScalingModeEffectItem& sender, bool isUp) {
	uint32_t idx = sender.EffectIdx();

	if (isUp) {
		assert(idx > 0);
		IInspectable prev = _effects.GetAt(idx - 1);
		// 状态更新由 _Effects_VectorChanged 处理
		_effects.RemoveAt(idx - 1);
		_effects.InsertAt(idx, prev);

		GetEffectItemImpl(prev).RefreshMoveState();
	} else {
		assert(idx + 1 < _effects.Size());
		IInspectable next = _effects.GetAt(idx + 1);
		_effects.RemoveAt(idx + 1);
		_effects.InsertAt(idx, next);

		GetEffectItemImpl(next).RefreshMoveState();
	}
	sender.RefreshMoveState();
}

com_ptr<ScalingModeEffectItem> ScalingModeItem::_CreateScalingModeEffectItem(uint32_t scalingModeIdx, uint32_t effectIdx) {
	auto item = make_self<ScalingModeEffectItem>(scalingModeIdx, effectIdx);
	item->Removed(std::bind_front(&ScalingModeItem::_ScalingModeEffectItem_Removed, this));
	item->Moved(std::bind_front(&ScalingModeItem::_ScalingModeEffectItem_Moved, this));
	return item;
}

void ScalingModeItem::AddEffect(const hstring& fullName) {
	EffectOption& effect = _Data().effects.emplace_back();
	effect.name = fullName;

	const EffectInfo* effectInfo = EffectsService::Get().GetEffect(fullName);
	assert(effectInfo);
	if (effectInfo->CanScale()) {
		// 支持缩放的效果默认等比缩放到充满屏幕
		effect.scalingType = ::Magpie::ScalingType::Fit;
	}

	auto item = _CreateScalingModeEffectItem(_index, (uint32_t)_Data().effects.size() - 1);
	_isMovingEffects = false;
	_effects.Append(*item);
	_isMovingEffects = true;

	uint32_t size = _effects.Size();
	GetEffectItemImpl(_effects.GetAt(size - 1)).RefreshMoveState();
	if (size > 1) {
		GetEffectItemImpl(_effects.GetAt(size - 2)).RefreshMoveState();
	}

	AppSettings::Get().SaveAsync();
}

hstring ScalingModeItem::Name() const noexcept {
	if (_index == std::numeric_limits<uint32_t>::max()) {
		// 特殊情况下被删除后依然可能被获取 Name 和 Description，可能是 ListView 的 bug
		return {};
	}

	return hstring(_Data().name);
}

void ScalingModeItem::Name(const hstring& value) noexcept {
	_Data().name = value;
	AppSettings::Get().SaveAsync();
}

hstring ScalingModeItem::Description() const noexcept {
	if (_index == std::numeric_limits<uint32_t>::max()) {
		return {};
	}

	std::wstring result;
	for (const EffectOption& effect : _Data().effects) {
		if (!result.empty()) {
			result.append(L" > ");
		}

		if (const EffectInfo* effectInfo = EffectsService::Get().GetEffect(effect.name)) {
			result += EffectHelper::GetDisplayName(effect.name);
		} else {
			ResourceLoader resourceLoader =
				ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
			result += L'(';
			result += resourceLoader.GetString(L"ScalingModes_Description_UnknownEffect");
			result += L')';
		}
	}
	return hstring(result);
}

bool ScalingModeItem::HasUnkownEffects() const noexcept {
	for (const EffectOption& effect : _Data().effects) {
		if (!EffectsService::Get().GetEffect(effect.name)) {
			return true;
		}
	}
	return false;
}

void ScalingModeItem::RenameText(const hstring& value) noexcept {
	_renameText = value;
	RaisePropertyChanged(L"RenameText");

	_trimedRenameText = value;
	StrHelper::Trim(_trimedRenameText);
	bool newEnabled = !_trimedRenameText.empty() && _trimedRenameText != _Data().name;
	if (_isRenameButtonEnabled != newEnabled) {
		_isRenameButtonEnabled = newEnabled;
		RaisePropertyChanged(L"IsRenameButtonEnabled");
	}
}

void ScalingModeItem::RenameFlyout_Opening() {
	RenameText(hstring(_Data().name));
	RaisePropertyChanged(L"RenameTextBoxSelectionStart");
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

	// Flyout 没有 IsOpen 可供绑定，只能用变通方法关闭
	XamlHelper::ClosePopups(App::Get().RootPage()->XamlRoot());

	_Data().name = _trimedRenameText;
	RaisePropertyChanged(L"Name");

	AppSettings::Get().SaveAsync();
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

bool ScalingModeItem::CanReorderEffects() const noexcept {
	// 管理员身份下不支持拖拽排序
	return _effects.Size() > 1 && !Win32Helper::IsProcessElevated();
}

bool ScalingModeItem::IsShowMoveButtons() const noexcept {
	return _effects.Size() > 1 && Win32Helper::IsProcessElevated();
}

void ScalingModeItem::Remove() {
	ScalingModesService::Get().RemoveScalingMode(_index);
	_index = std::numeric_limits<uint32_t>::max();
}

ScalingMode& ScalingModeItem::_Data() noexcept {
	return ScalingModesService::Get().GetScalingMode(_index);
}

const ScalingMode& ScalingModeItem::_Data() const noexcept {
	return ScalingModesService::Get().GetScalingMode(_index);
}

}
