#pragma once

// Windows
#include <SDKDDKVer.h>
#include <Windows.h>
#include <windowsx.h>

// 避免 C++/WinRT 头文件的警告
#undef GetCurrentTime

// DirectX
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>

// 标准库
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <functional>
#include <span>
#include <filesystem>
#include <chrono>

// WIL
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <wil/filesystem.h>
#define WIL_ENABLE_EXCEPTIONS
// 防止编译失败
#define RESOURCE_SUPPRESS_STL
#include <wil/stl.h>
#undef RESOURCE_SUPPRESS_STL
#undef WIL_ENABLE_EXCEPTIONS

// C++/WinRT
#include <unknwn.h>
#include <restrictederrorinfo.h>
#include <hstring.h>
#include <winrt/base.h>
#include <winrt/Windows.ApplicationModel.Resources.h>
#include <winrt/Windows.ApplicationModel.Resources.Core.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.System.h>

namespace winrt {
using namespace Windows::ApplicationModel::Resources;
using namespace Windows::ApplicationModel::Resources::Core;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Metadata;
using namespace Windows::Graphics::Display;
using namespace Windows::System;
}

// fmt
#include <fmt/format.h>
#include <fmt/xchar.h>

#include "CommonDefines.h"

namespace Magpie {

static constexpr uint32_t MAX_CAPTURE_DIRTY_RECT_COUNT = 4;
static constexpr uint32_t DUP_FRAME_DISPATCH_BLOCK_SIZE = 16;
static constexpr uint32_t SCENE_REFERRED_SDR_WHITE_LEVEL = 80;

enum class ComponentState {
	NoError,
	DeviceLost,
	Error
};

struct ColorInfo {
	winrt::AdvancedColorKind kind = winrt::AdvancedColorKind::StandardDynamicRange;

	// HDR 模式下最低亮度，1.0 表示 80nit
	float minLuminance = 0.0f;
	// HDR 模式下最高亮度，1.0 表示 80nit
	float maxLuminance = 1.0f;
	// HDR 模式下 SDR 内容亮度，1.0 表示 80nit
	float sdrWhiteLevel = 1.0f;

	// 默认为 sRGB 原色和白点
	winrt::Point redPrimary = { 0.64f, 0.33f };
	winrt::Point greenPrimary = { 0.3f, 0.6f };
	winrt::Point bluePrimary = { 0.15f, 0.06f };
	winrt::Point whitePoint = { 0.3127f, 0.329f };

	bool operator==(const ColorInfo& other) const = default;
};

}
