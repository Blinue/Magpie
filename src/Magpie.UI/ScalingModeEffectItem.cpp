#include "pch.h"
#include "ScalingModeEffectItem.h"
#if __has_include("ScalingModeEffectItem.g.cpp")
#include "ScalingModeEffectItem.g.cpp"
#endif
#include <Magpie.Core.h>
#include "ScalingModesService.h"
#include "EffectsService.h"


namespace Core = ::Magpie::Core;
using namespace Magpie::Core;


static std::wstring_view GetEffectDisplayName(std::wstring_view fullName) {
	size_t delimPos = fullName.find_last_of(L'\\');
	return delimPos != std::wstring::npos ? fullName.substr(delimPos + 1) : fullName;
}

namespace winrt::Magpie::UI::implementation {

ScalingModeEffectItem::ScalingModeEffectItem(uint32_t scalingModeIdx, uint32_t effectIdx) 
	: _scalingModeIdx(scalingModeIdx), _effectIdx(effectIdx)
{
	const std::wstring& name = _Data().name;
	_name = GetEffectDisplayName(name);
	_effectInfo = EffectsService::Get().GetEffect(name);
}

bool ScalingModeEffectItem::CanScale() const noexcept {
	return _effectInfo && _effectInfo->canScale;
}

bool ScalingModeEffectItem::HasParameters() const noexcept {
	return _effectInfo && !_effectInfo->params.empty();
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

	MONITORINFO mi{};
	mi.cbSize = sizeof(mi);
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
	const Core::ScalingType scalingType = (Core::ScalingType)value;
	if (data.scalingType == scalingType) {
		return;
	}

	if (data.scalingType == Core::ScalingType::Absolute) {
		data.scale = { 1.0f,1.0f };
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ScalingFactorX"));
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ScalingFactorY"));
	} else if (scalingType == Core::ScalingType::Absolute) {
		SIZE monitorSize = GetMonitorSize();
		data.scale = { (float)monitorSize.cx,(float)monitorSize.cy };

		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ScalingPixelsX"));
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ScalingPixelsY"));
	}

	data.scalingType = scalingType;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ScalingType"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsShowScalingFactors"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsShowScalingPixels"));
}

bool ScalingModeEffectItem::IsShowScalingFactors() const noexcept {
	Core::ScalingType scalingType = _Data().scalingType;
	return scalingType == Core::ScalingType::Normal || scalingType == Core::ScalingType::Fit;
}

bool ScalingModeEffectItem::IsShowScalingPixels() const noexcept {
	return _Data().scalingType == Core::ScalingType::Absolute;
}

double ScalingModeEffectItem::ScalingFactorX() const noexcept {
	return _Data().scale.first;
}

void ScalingModeEffectItem::ScalingFactorX(double value) {
	EffectOption& data = _Data();
	if (data.scalingType != Core::ScalingType::Normal && data.scalingType != Core::ScalingType::Fit) {
		return;
	}

	// 用户将 NumberBox 清空时会传入 nan
	if (!std::isnan(value) && value + std::numeric_limits<float>::epsilon() > 1e-4) {
		data.scale.first = (float)value;
	}
	
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ScalingFactorX"));
}

double ScalingModeEffectItem::ScalingFactorY() const noexcept {
	return _Data().scale.second;
}

void ScalingModeEffectItem::ScalingFactorY(double value) {
	EffectOption& data = _Data();
	if (data.scalingType != Core::ScalingType::Normal && data.scalingType != Core::ScalingType::Fit) {
		return;
	}

	if (!std::isnan(value) && value + std::numeric_limits<float>::epsilon() > 1e-4) {
		data.scale.second = (float)value;
	}

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ScalingFactorY"));
}

double ScalingModeEffectItem::ScalingPixelsX() const noexcept {
	return _Data().scale.first;
}

void ScalingModeEffectItem::ScalingPixelsX(double value) {
	EffectOption& data = _Data();
	if (data.scalingType != Core::ScalingType::Absolute) {
		return;
	}

	if (!std::isnan(value) && value > 0.5) {
		data.scale.first = (float)std::round(value);
	}

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ScalingPixelsX"));
}

double ScalingModeEffectItem::ScalingPixelsY() const noexcept {
	return _Data().scale.second;
}

void ScalingModeEffectItem::ScalingPixelsY(double value) {
	EffectOption& data = _Data();
	if (data.scalingType != Core::ScalingType::Absolute) {
		return;
	}

	if (!std::isnan(value) && value > 0.5) {
		data.scale.second = (float)std::round(value);
	}

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ScalingPixelsY"));
}

void ScalingModeEffectItem::Remove() {
	_removedEvent(*this, _effectIdx);
}

EffectOption& ScalingModeEffectItem::_Data() noexcept {
	return ScalingModesService::Get().GetScalingMode(_scalingModeIdx).effects[_effectIdx];
}

const EffectOption& ScalingModeEffectItem::_Data() const noexcept {
	return ScalingModesService::Get().GetScalingMode(_scalingModeIdx).effects[_effectIdx];
}

}
