#pragma once
#include <Magpie.Core.h>

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
struct Profile {
	void Copy(const Profile& other) noexcept {
		// 不复制自动缩放选项
		scalingMode = other.scalingMode;
		cursorScaling = other.cursorScaling;
		customCursorScaling = other.customCursorScaling;
		isCroppingEnabled = other.isCroppingEnabled;
		cropping = other.cropping;
		captureMethod = other.captureMethod;
		graphicsCard = other.graphicsCard;
		multiMonitorUsage = other.multiMonitorUsage;
		cursorInterpolationMode = other.cursorInterpolationMode;
		flags = other.flags;
	}

	DEFINE_FLAG_ACCESSOR(IsDisableWindowResizing, ::Magpie::Core::MagFlags::DisableWindowResizing, flags)
	DEFINE_FLAG_ACCESSOR(Is3DGameMode, ::Magpie::Core::MagFlags::Is3DGameMode, flags)
	DEFINE_FLAG_ACCESSOR(IsShowFPS, ::Magpie::Core::MagFlags::ShowFPS, flags)
	DEFINE_FLAG_ACCESSOR(IsVSync, ::Magpie::Core::MagFlags::VSync, flags)
	DEFINE_FLAG_ACCESSOR(IsTripleBuffering, ::Magpie::Core::MagFlags::TripleBuffering, flags)
	DEFINE_FLAG_ACCESSOR(IsCaptureTitleBar, ::Magpie::Core::MagFlags::CaptureTitleBar, flags)
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
	::Magpie::Core::CaptureMethod captureMethod = ::Magpie::Core::CaptureMethod::GraphicsCapture;
	// -1 表示默认，大于等于 0 为图形适配器的索引
	int graphicsCard = 0;
	::Magpie::Core::MultiMonitorUsage multiMonitorUsage = ::Magpie::Core::MultiMonitorUsage::Closest;
	::Magpie::Core::CursorInterpolationMode cursorInterpolationMode = ::Magpie::Core::CursorInterpolationMode::NearestNeighbor;

	uint32_t flags = ::Magpie::Core::MagFlags::VSync 
		| ::Magpie::Core::MagFlags::AdjustCursorSpeed
		| ::Magpie::Core::MagFlags::DrawCursor;

	bool isPackaged = false;
	bool isCroppingEnabled = false;
	bool isAutoScale = false;
};

}
