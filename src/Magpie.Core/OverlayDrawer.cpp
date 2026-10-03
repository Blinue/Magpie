#include "pch.h"
#include "OverlayDrawer.h"
#include "D3D12Context.h"
#include "LocalizationService.h"
#include "Logger.h"
#include "ScalingOptions.h"
#include "ScalingWindow.h"
#include "StrHelper.h"
#include "Win32Helper.h"
#include <imgui.h>
#include <parallel_hashmap/phmap.h>
#include <ShlObj.h>

namespace Magpie {

//static const char* COLOR_INDICATOR = "■";
//static const wchar_t COLOR_INDICATOR_W = L'■';

static const float CORNER_ROUNDING = 6;

static const char* TOOLBAR_WINDOW_ID = "toolbar";
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

bool OverlayDrawer::Initialize(
	D3D12Context& d3d12Context,
	OverlayOptions& overlayOptions,
	const RECT& rendererRect,
	const RECT& destRect,
	const ColorInfo& colorInfo
) noexcept {
	_d3d12Context = &d3d12Context;
	_overlayOptions = &overlayOptions;

	SetDefaultWindowOptions(overlayOptions.windows);

	if (!_imguiImpl.Initialize(d3d12Context, rendererRect, destRect, colorInfo)) {
		Logger::Get().Error("ImGuiImpl::Initialize 失败");
		return false;
	}

	_dpiScale = ScalingWindow::Get().GetDpi() / float(USER_DEFAULT_SCREEN_DPI);

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.PopupRounding = style.WindowRounding = CORNER_ROUNDING;
	// 由于我们是按需渲染，显示 tooltip 时不要有延迟
	style.HoverFlagsForTooltipMouse = ImGuiHoveredFlags_DelayNone;
	style.FrameBorderSize = 1;
	style.FrameRounding = 2;
	style.WindowMinSize = ImVec2(10, 10);
	style.ScaleAllSizes(_dpiScale);
	style.FontScaleDpi = _dpiScale;

	if (!_BuildFonts()) {
		Logger::Get().Error("_BuildFonts 失败");
		return false;
	}

	ImGui::GetIO().FontDefault = _uiFont;

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

HRESULT OverlayDrawer::Draw(
	GraphicsContext& graphicsContext,
	POINT cursorPos,
	uint32_t /*fps*/,
	uint64_t frameFenceValue,
	uint64_t completedFenceValue
) noexcept {
	// 所有窗口都不可见则跳过 ImGui 绘制
	if (!_AnyVisibleWindow()) {
		return S_OK;
	}

	// 为了符合 Fitts 法则，鼠标在工具栏上时稍微下移逻辑位置使得在上边缘可以选中工具栏按钮
	float fittsLawAdjustment = 0;
	const char* hoveredWindowId = _imguiImpl.GetHoveredWindowId();
	if (hoveredWindowId && hoveredWindowId == std::string_view(TOOLBAR_WINDOW_ID)) {
		fittsLawAdjustment = 4 * _dpiScale;
	}

	_imguiImpl.NewFrame(cursorPos, _overlayOptions->windows, fittsLawAdjustment, _dpiScale);

#ifdef _DEBUG
	ImGui::ShowDemoWindow(&_isDemoWindowVisible);
#endif

	ImGui::EndFrame();

	_imguiImpl.Draw(graphicsContext, frameFenceValue, completedFenceValue);

	return S_OK;
}

void OverlayDrawer::OnColorInfoChanged(const ColorInfo& colorInfo) noexcept {
	_imguiImpl.OnColorInfoChanged(colorInfo);
}

void OverlayDrawer::MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	if (_AnyVisibleWindow()) {
		_imguiImpl.MessageHandler(msg, wParam, lParam);
	}
}

static const std::wstring& GetSystemFontsFolder() noexcept {
	static std::wstring result;

	if (result.empty()) {
		wil::unique_cotaskmem_string fontsFolder;
		HRESULT hr = SHGetKnownFolderPath(FOLDERID_Fonts, 0, NULL, fontsFolder.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("SHGetKnownFolderPath 失败", hr);
			return result;
		}

		result = fontsFolder.get();
	}

	return result;
}

bool OverlayDrawer::_BuildFonts() noexcept {
	ImFontAtlas& fontAtlas = *ImGui::GetIO().Fonts;
	fontAtlas.Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight | ImFontAtlasFlags_NoMouseCursors;

	std::string uiFontPath = StrHelper::UTF16ToUTF8(GetSystemFontsFolder());
	if (Win32Helper::GetOSVersion().IsWin11()) {
		uiFontPath += "\\SegUIVar.ttf";
	} else {
		uiFontPath += "\\segoeui.ttf";
	}

	/*std::vector<uint8_t> uiFontData;
	if (!Win32Helper::ReadFile(uiFontPath.c_str(), uiFontData)) {
		Logger::Get().Error("读取字体文件失败");
		return false;
	}*/

	_uiFont = fontAtlas.AddFontFromFileTTF(uiFontPath.c_str());

	// 构建 ImFontAtlas 前 ranges 不能析构，因为 ImGui 只保存了指针
	/*SmallVector<ImWchar> uiRanges = _BuildFontUI(uiFontData);
	_BuildFontIcons(iconFontPath.c_str());

	if (!fontAtlas.Build()) {
		Logger::Get().Error("构建 ImFontAtlas 失败");
		return false;
	}


	if (!_imguiImpl.BuildFonts()) {
		Logger::Get().Error("构建字体失败");
		return false;
	}*/

	// ImGui::GetIO().Fonts->AddFontDefault();

	return true;
}

bool OverlayDrawer::_AnyVisibleWindow() const noexcept {
	bool result = _isToolbarVisible || _isProfilerVisible;
#ifdef _DEBUG
	result = result || _isDemoWindowVisible;
#endif
	return result;
}

}
