#include "pch.h"
#include "ScalingModeEffectItem.h"
#if __has_include("ScalingModeEffectItem.g.cpp")
#include "ScalingModeEffectItem.g.cpp"
#endif
#include <Magpie.Core.h>
#include "ScalingModesService.h"
#include "EffectsService.h"
#include "EffectHelper.h"
#include "AppSettings.h"
#include "Logger.h"
#include "ScalingMode.h"
#include "StrUtils.h"
#include "CommonSharedConstants.h"

using namespace ::Magpie::Core;
namespace MagpieCore = ::Magpie::Core;

namespace winrt::Magpie::App::implementation {

ScalingModeEffectItem::ScalingModeEffectItem(uint32_t scalingModeIdx, uint32_t effectIdx) 
	: _scalingModeIdx(scalingModeIdx), _effectIdx(effectIdx)
{
	EffectOption& data = _Data();

	_effectInfo = EffectsService::Get().GetEffect(data.name);

	if (_effectInfo) {
		_name = EffectHelper::GetDisplayName(data.name);
		_parametersViewModel = EffectParametersViewModel(scalingModeIdx, effectIdx);
	} else {
		ResourceLoader resourceLoader =
			ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
		_name = StrUtils::Concat(
			resourceLoader.GetString(L"ScalingModes_Description_UnknownEffect"),
			L" (",
			data.name,
			L")"
		);
	}
}

void ScalingModeEffectItem::ScalingModeIdx(uint32_t value) noexcept {
	_scalingModeIdx = value;

	if (_parametersViewModel) {
		_parametersViewModel.ScalingModeIdx(value);
	}
}

void ScalingModeEffectItem::EffectIdx(uint32_t value) noexcept {
	_effectIdx = value;

	if (_parametersViewModel) {
		_parametersViewModel.EffectIdx(value);
	}
}

bool ScalingModeEffectItem::CanScale() const noexcept {
	return _effectInfo && _effectInfo->CanScale();
}

bool ScalingModeEffectItem::HasParameters() const noexcept {
	return _effectInfo && !_effectInfo->params.empty();
}

IVector<IInspectable> ScalingModeEffectItem::ScalingTypes() noexcept {
	using Windows::ApplicationModel::Resources::ResourceLoader;
	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	
	return single_threaded_vector(std::vector<IInspectable>{
		Magpie::App::ScalingType(
			resourceLoader.GetString(L"ScalingModes_ScaleFlyout_Type_Factor"),
			resourceLoader.GetString(L"ScalingModes_ScaleFlyout_Type_Factor_Description")
		),
		Magpie::App::ScalingType(
			resourceLoader.GetString(L"ScalingModes_ScaleFlyout_Type_Fit"),
			resourceLoader.GetString(L"ScalingModes_ScaleFlyout_Type_Fit_Description")
		),
		Magpie::App::ScalingType(
			resourceLoader.GetString(L"ScalingModes_ScaleFlyout_Type_Absolute"),
			resourceLoader.GetString(L"ScalingModes_ScaleFlyout_Type_Absolute_Description")
		),
		Magpie::App::ScalingType(
			resourceLoader.GetString(L"ScalingModes_ScaleFlyout_Type_Fill"),
			resourceLoader.GetString(L"ScalingModes_ScaleFlyout_Type_Fill_Description")
		),
	});
}

int ScalingModeEffectItem::ScalingType() const noexcept {
	return (int)_Data().scalingType;
}

static SIZE GetMonitorSize() noexcept {
	// 使用距离主窗口最近的显示器
	HWND hwndMain = (HWND)Application::Current().as<App>().HwndMain();
	HMONITOR hMonitor = MonitorFromWindow(hwndMain, MONITOR_DEFAULTTONEAREST);
	if (!hMonitor) {
		Logger::Get().Win32Error("MonitorFromWindow 失败");
		return { 400,300 };
	}

	MONITORINFO mi{ .cbSize = sizeof(mi) };
	if (!GetMonitorInfo(hMonitor, &mi)) {
		Logger::Get().Win32Error("GetMonitorInfo 失败");
		return { 400,300 };
	}
	
	return {
		mi.rcMonitor.right - mi.rcMonitor.left,
		mi.rcMonitor.bottom - mi.rcMonitor.top
	};
}

void ScalingModeEffectItem::ScalingType(int value) {
	if (value < 0) {
		return;
	}

	EffectOption& data = _Data();
	const MagpieCore::ScalingType scalingType = (MagpieCore::ScalingType)value;
	if (data.scalingType == scalingType) {
		return;
	}

	if (data.scalingType == MagpieCore::ScalingType::Absolute) {
		data.scale = { 1.0f,1.0f };
		RaisePropertyChanged(L"ScalingFactorX");
		RaisePropertyChanged(L"ScalingFactorY");
	} else if (scalingType == MagpieCore::ScalingType::Absolute) {
		SIZE monitorSize = GetMonitorSize();
		data.scale = { (float)monitorSize.cx,(float)monitorSize.cy };

		RaisePropertyChanged(L"ScalingPixelsX");
		RaisePropertyChanged(L"ScalingPixelsY");
	}

	data.scalingType = scalingType;
	RaisePropertyChanged(L"ScalingType");
	RaisePropertyChanged(L"IsShowScalingFactors");
	RaisePropertyChanged(L"IsShowScalingPixels");

	AppSettings::Get().SaveAsync();
}

bool ScalingModeEffectItem::IsShowScalingFactors() const noexcept {
	MagpieCore::ScalingType scalingType = _Data().scalingType;
	return scalingType == MagpieCore::ScalingType::Normal || scalingType == MagpieCore::ScalingType::Fit;
}

bool ScalingModeEffectItem::IsShowScalingPixels() const noexcept {
	return _Data().scalingType == MagpieCore::ScalingType::Absolute;
}

double ScalingModeEffectItem::ScalingFactorX() const noexcept {
	return _Data().scale.first;
}

void ScalingModeEffectItem::ScalingFactorX(double value) {
	EffectOption& data = _Data();
	if (data.scalingType != MagpieCore::ScalingType::Normal && data.scalingType != MagpieCore::ScalingType::Fit) {
		return;
	}

	// 用户将 NumberBox 清空时会传入 nan
	if (!std::isnan(value) && value + std::numeric_limits<float>::epsilon() > 1e-4) {
		data.scale.first = (float)value;
	}
	
	RaisePropertyChanged(L"ScalingFactorX");
	AppSettings::Get().SaveAsync();
}

double ScalingModeEffectItem::ScalingFactorY() const noexcept {
	return _Data().scale.second;
}

void ScalingModeEffectItem::ScalingFactorY(double value) {
	EffectOption& data = _Data();
	if (data.scalingType != MagpieCore::ScalingType::Normal && data.scalingType != MagpieCore::ScalingType::Fit) {
		return;
	}

	if (!std::isnan(value) && value + std::numeric_limits<float>::epsilon() > 1e-4) {
		data.scale.second = (float)value;
	}

	RaisePropertyChanged(L"ScalingFactorY");
	AppSettings::Get().SaveAsync();
}

double ScalingModeEffectItem::ScalingPixelsX() const noexcept {
	return _Data().scale.first;
}

void ScalingModeEffectItem::ScalingPixelsX(double value) {
	EffectOption& data = _Data();
	if (data.scalingType != MagpieCore::ScalingType::Absolute) {
		return;
	}

	if (!std::isnan(value) && value > 0.5) {
		data.scale.first = (float)std::round(value);
	}

	RaisePropertyChanged(L"ScalingPixelsX");
	AppSettings::Get().SaveAsync();
}

double ScalingModeEffectItem::ScalingPixelsY() const noexcept {
	return _Data().scale.second;
}

void ScalingModeEffectItem::ScalingPixelsY(double value) {
	EffectOption& data = _Data();
	if (data.scalingType != MagpieCore::ScalingType::Absolute) {
		return;
	}

	if (!std::isnan(value) && value > 0.5) {
		data.scale.second = (float)std::round(value);
	}

	RaisePropertyChanged(L"ScalingPixelsY");
	AppSettings::Get().SaveAsync();
}

void ScalingModeEffectItem::Remove() {
	Removed.invoke(*this, _effectIdx);
}

bool ScalingModeEffectItem::CanMove() const noexcept {
	const ScalingMode& mode = ScalingModesService::Get().GetScalingMode(_scalingModeIdx);
	return mode.effects.size() > 1 && Win32Utils::IsProcessElevated();
}

bool ScalingModeEffectItem::CanMoveUp() const noexcept {
	return _effectIdx > 0;
}

bool ScalingModeEffectItem::CanMoveDown() const noexcept {
	const ScalingMode& mode = ScalingModesService::Get().GetScalingMode(_scalingModeIdx);
	return _effectIdx + 1 < (uint32_t)mode.effects.size();
}

void ScalingModeEffectItem::MoveUp() noexcept {
	Moved.invoke(*this, true);
}

void ScalingModeEffectItem::MoveDown() noexcept {
	Moved.invoke(*this, false);
}

void ScalingModeEffectItem::RefreshMoveState() {
	RaisePropertyChanged(L"CanMove");
	RaisePropertyChanged(L"CanMoveUp");
	RaisePropertyChanged(L"CanMoveDown");
}

EffectOption& ScalingModeEffectItem::_Data() noexcept {
	return ScalingModesService::Get().GetScalingMode(_scalingModeIdx).effects[_effectIdx];
}

const EffectOption& ScalingModeEffectItem::_Data() const noexcept {
	return ScalingModesService::Get().GetScalingMode(_scalingModeIdx).effects[_effectIdx];
}

}
