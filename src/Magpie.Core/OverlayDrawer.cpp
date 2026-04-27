#include "pch.h"
#include "Logger.h"
#include "OverlayDrawer.h"
#include "ScalingOptions.h"
#include "ScalingWindow.h"
#include "D3D12Context.h"
#include "StrHelper.h"
#include <parallel_hashmap/phmap.h>
#include <imgui.h>

namespace Magpie {

//static const char* COLOR_INDICATOR = "■";
//static const wchar_t COLOR_INDICATOR_W = L'■';

static const float CORNER_ROUNDING = 6;

//static const char* TOOLBAR_WINDOW_ID = "toolbar";
static const char* PROFILER_WINDOW_ID = "profiler";

static void SetDefaultWindowOptions(
	phmap::flat_hash_map<std::string, OverlayWindowOption>& windowOptions
) noexcept {
	if (!windowOptions.contains(PROFILER_WINDOW_ID)) {
		// 右侧竖直居中
		windowOptions.emplace(PROFILER_WINDOW_ID, OverlayWindowOption{
			.hArea = 2,
			.vArea = 1,
			.hPos = 60.0f,
			.vPos = 0.5f
		});
	}
}

bool OverlayDrawer::Initialize(D3D12Context& d3d12Context, OverlayOptions& overlayOptions) noexcept {
	_d3d12Context = &d3d12Context;
	_overlayOptions = &overlayOptions;

	SetDefaultWindowOptions(overlayOptions.windows);

	if (!_imguiImpl.Initialize(d3d12Context)) {
		Logger::Get().Error("ImGuiImpl::Initialize 失败");
		return false;
	}

	_dpiScale = GetDpiForWindow(ScalingWindow::Get().Handle()) / float(USER_DEFAULT_SCREEN_DPI);

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.PopupRounding = style.WindowRounding = CORNER_ROUNDING;
	// 由于我们是按需渲染，显示 tooltip 时不要有延迟
	style.HoverFlagsForTooltipMouse = ImGuiHoveredFlags_DelayNone;
	style.FrameBorderSize = 1;
	style.FrameRounding = 2;
	style.WindowMinSize = ImVec2(10, 10);
	style.ScaleAllSizes(_dpiScale);

	ImGui::GetIO().Fonts->AddFontDefault();

	// 获取硬件信息
	DXGI_ADAPTER_DESC desc{};
	HRESULT hr = d3d12Context.GetDXGIAdapter()->GetDesc(&desc);
	if (SUCCEEDED(hr)) {
		_hardwareInfo.gpuName = StrHelper::UTF16ToUTF8(desc.Description);
	} else {
		Logger::Get().ComError("IDXGIAdapter::GetDesc 失败", hr);
	}

	return true;
}

void OverlayDrawer::OnResizingChanged(bool value) noexcept {
	_imguiImpl.OnResizingChanged(value);
}

void OverlayDrawer::OnResized(const RECT& rendererRect, const RECT& destRect) noexcept {
	_imguiImpl.OnResized(rendererRect, destRect);
}

void OverlayDrawer::OnMovingChanged(bool value) noexcept {
	_imguiImpl.OnMovingChanged(value);
}

void OverlayDrawer::OnMoved(const RECT& rendererRect, const RECT& destRect) noexcept {
	_imguiImpl.OnMoved(rendererRect, destRect);
}

void OverlayDrawer::OnCursorCapturedOnForegroundChanged(bool value) noexcept {
	_imguiImpl.OnCursorCapturedOnForegroundChanged(value);
}

HRESULT OverlayDrawer::Draw(GraphicsContext& /*graphicsContext*/, POINT cursorPos, uint32_t /*fps*/) noexcept {
	// 所有窗口都不可见则跳过 ImGui 绘制
	/*if (!_AnyVisibleWindow()) {
		return;
	}*/

	// 为了符合 Fitts 法则，鼠标在工具栏上时稍微下移逻辑位置使得在上边缘可以选中工具栏按钮
	float fittsLawAdjustment = 0;
	/*const char* hoveredWindowId = _imguiImpl.GetHoveredWindowId();
	if (hoveredWindowId && hoveredWindowId == std::string_view(TOOLBAR_WINDOW_ID)) {
		fittsLawAdjustment = 4 * _dpiScale;
	}*/

	_imguiImpl.NewFrame(cursorPos, _overlayOptions->windows, fittsLawAdjustment, _dpiScale);

#ifdef _DEBUG
	ImGui::ShowDemoWindow(&_isDemoWindowVisible);
#endif

	ImGui::EndFrame();

	_imguiImpl.Draw();

	return S_OK;
}

bool OverlayDrawer::_AnyVisibleWindow() const noexcept {
	bool result = _isToolbarVisible || _isProfilerVisible;
#ifdef _DEBUG
	result = result || _isDemoWindowVisible;
#endif
	return result;
}

}
