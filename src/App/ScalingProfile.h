#pragma once
#include "pch.h"
#include <Runtime.h>


namespace winrt::Magpie::App {

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
		CursorScaling = other.CursorScaling;
		CustomCursorScaling = other.CustomCursorScaling;
		IsCroppingEnabled = other.IsCroppingEnabled;
		Cropping = other.Cropping;
		CaptureMode = other.CaptureMode;
		GraphicsAdapter = other.GraphicsAdapter;
		MultiMonitorUsage = other.MultiMonitorUsage;
		CursorInterpolationMode = other.CursorInterpolationMode;
		Flags = other.Flags;
	}

	DEFINE_MAGFLAG_ACCESSOR(IsDisableWindowResizing)
	DEFINE_MAGFLAG_ACCESSOR(Is3DGameMode)
	DEFINE_MAGFLAG_ACCESSOR(IsShowFPS)
	DEFINE_MAGFLAG_ACCESSOR(IsVSync)
	DEFINE_MAGFLAG_ACCESSOR(IsTripleBuffering)
	DEFINE_MAGFLAG_ACCESSOR(IsReserveTitleBar)
	DEFINE_MAGFLAG_ACCESSOR(IsAdjustCursorSpeed)
	DEFINE_MAGFLAG_ACCESSOR(IsDrawCursor)
	DEFINE_MAGFLAG_ACCESSOR(IsDisableDirectFlip)

	std::wstring Name;

	// 若为打包应用，PathRule 存储 AUMID
	std::wstring PathRule;
	std::wstring ClassNameRule;
	bool IsPackaged = false;
	
	CursorScaling CursorScaling = CursorScaling::NoScaling;
	float CustomCursorScaling = 1.0;

	bool IsCroppingEnabled = false;
	::Magpie::Runtime::Cropping Cropping{};
	::Magpie::Runtime::CaptureMode CaptureMode = ::Magpie::Runtime::CaptureMode::GraphicsCapture;
	uint32_t GraphicsAdapter = 0;
	::Magpie::Runtime::MultiMonitorUsage MultiMonitorUsage = ::Magpie::Runtime::MultiMonitorUsage::Nearest;
	::Magpie::Runtime::CursorInterpolationMode CursorInterpolationMode = ::Magpie::Runtime::CursorInterpolationMode::Nearest;

	uint32_t Flags = ::Magpie::Runtime::MagFlags::IsVSync 
		| ::Magpie::Runtime::MagFlags::IsAdjustCursorSpeed
		| ::Magpie::Runtime::MagFlags::IsDrawCursor;
};

}
