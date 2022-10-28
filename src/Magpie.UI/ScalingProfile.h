#pragma once
#include "pch.h"
#include <Magpie.Core.h>


namespace winrt::Magpie::UI {

enum class CursorScaling {
	x0_5,
	x0_75,
	NoScaling,
	x1_25,
	x1_5,
	x2,
	Source,
	Custom
};

// 默认规则 Name、PathRule、ClassNameRule 均为空
struct ScalingProfile {
	void Copy(const ScalingProfile& other) noexcept {
		scalingMode = other.scalingMode;
		cursorScaling = other.cursorScaling;
		customCursorScaling = other.customCursorScaling;
		isCroppingEnabled = other.isCroppingEnabled;
		cropping = other.cropping;
		captureMode = other.captureMode;
		graphicsAdapter = other.graphicsAdapter;
		multiMonitorUsage = other.multiMonitorUsage;
		cursorInterpolationMode = other.cursorInterpolationMode;
		flags = other.flags;
	}

	DEFINE_FLAG_ACCESSOR(IsDisableWindowResizing, ::Magpie::Core::MagFlags::DisableWindowResizing, flags)
	DEFINE_FLAG_ACCESSOR(Is3DGameMode, ::Magpie::Core::MagFlags::Is3DGameMode, flags)
	DEFINE_FLAG_ACCESSOR(IsShowFPS, ::Magpie::Core::MagFlags::ShowFPS, flags)
	DEFINE_FLAG_ACCESSOR(IsVSync, ::Magpie::Core::MagFlags::VSync, flags)
	DEFINE_FLAG_ACCESSOR(IsTripleBuffering, ::Magpie::Core::MagFlags::TripleBuffering, flags)
	DEFINE_FLAG_ACCESSOR(IsReserveTitleBar, ::Magpie::Core::MagFlags::ReserveTitleBar, flags)
	DEFINE_FLAG_ACCESSOR(IsAdjustCursorSpeed, ::Magpie::Core::MagFlags::AdjustCursorSpeed, flags)
	DEFINE_FLAG_ACCESSOR(IsDrawCursor, ::Magpie::Core::MagFlags::DrawCursor, flags)
	DEFINE_FLAG_ACCESSOR(IsDisableDirectFlip, ::Magpie::Core::MagFlags::DisableDirectFlip, flags)

	std::wstring name;

	// 若为打包应用，PathRule 存储 AUMID
	std::wstring pathRule;
	std::wstring classNameRule;

	CursorScaling cursorScaling = CursorScaling::NoScaling;
	float customCursorScaling = 1.0;

	::Magpie::Core::Cropping cropping{};
	// -1 表示原样
	int scalingMode = -1;
	::Magpie::Core::CaptureMode captureMode = ::Magpie::Core::CaptureMode::GraphicsCapture;
	uint32_t graphicsAdapter = 0;
	::Magpie::Core::MultiMonitorUsage multiMonitorUsage = ::Magpie::Core::MultiMonitorUsage::Nearest;
	::Magpie::Core::CursorInterpolationMode cursorInterpolationMode = ::Magpie::Core::CursorInterpolationMode::Nearest;

	uint32_t flags = ::Magpie::Core::MagFlags::VSync 
		| ::Magpie::Core::MagFlags::AdjustCursorSpeed
		| ::Magpie::Core::MagFlags::DrawCursor;

	bool isPackaged = false;
	bool isCroppingEnabled = false;
};

}
