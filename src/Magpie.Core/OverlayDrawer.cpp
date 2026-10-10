#include "pch.h"
#include "OverlayDrawer.h"
#include "D3D12Context.h"
#include "LocalizationService.h"
#include "Logger.h"
#include "OverlayHelper.h"
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

static const std::string& GetLocalizedString(const std::wstring_view& key) noexcept {
	static phmap::flat_hash_map<std::wstring_view, std::string> cache;

	if (auto it = cache.find(key); it != cache.end()) {
		return it->second;
	}

	LocalizationService& ls = LocalizationService::Get();
	return cache[key] = StrHelper::UTF16ToUTF8(ls.GetLocalizedString(key));
}

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
	_destRect = destRect;

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

	style.FontSizeBase = 18;
	style.FontScaleDpi = _dpiScale;

	// ImGui 被设计为工作在 sRGB 空间中，在线性空间混合时有透明度太高的问题
	for (ImVec4& color : style.Colors) {
		color.w = std::pow(color.w, 1.0f / 2.2f);
	}

	if (!_BuildFonts()) {
		Logger::Get().Error("_BuildFonts 失败");
		return false;
	}

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
	_isResizing = value;
	_imguiImpl.OnResizingChanged(value);
}

void OverlayDrawer::OnResized(const RECT& rendererRect, const RECT& destRect) noexcept {
	_destRect = destRect;
	_imguiImpl.OnResized(rendererRect, destRect);
}

void OverlayDrawer::OnMovingChanged(bool value) noexcept {
	_isMoving = value;
	_imguiImpl.OnMovingChanged(value);
}

void OverlayDrawer::OnMoved(const RECT& rendererRect, const RECT& destRect) noexcept {
	_destRect = destRect;
	_imguiImpl.OnMoved(rendererRect, destRect);
}

void OverlayDrawer::OnCursorCapturedOnForegroundChanged(bool value) noexcept {
	_imguiImpl.OnCursorCapturedOnForegroundChanged(value);
}

HRESULT OverlayDrawer::Draw(
	GraphicsContext& graphicsContext,
	bool isCursorOnRenderer,
	POINT cursorPos,
	uint32_t fps,
	uint64_t frameFenceValue,
	uint64_t completedFenceValue
) noexcept {
	bool hasInput = std::exchange(_hasMouseInput, false);

	// 所有窗口都不可见则跳过 ImGui 绘制
	if (!_AnyVisibleWindow()) {
		return S_OK;
	}

	_imguiImpl.PrepareNewFrame(isCursorOnRenderer, cursorPos, TOOLBAR_WINDOW_ID, 4 * _dpiScale);

	const bool wasCursorOnCaptionArea = _isCursorOnCaptionArea;

	for (int i = (hasInput ? 2 : 1); i >= 0; --i) {
		_imguiImpl.NewFrame(_overlayOptions->windows, _dpiScale);

		// 防止 ID 冲突
		int itemId = 0;

		if (_isToolbarVisible && _DrawToolbar(fps, cursorPos, itemId)) {
			++i;
		}

#ifdef _DEBUG
		if (_isDemoWindowVisible) {
			ImGui::ShowDemoWindow(&_isDemoWindowVisible);
		}
#endif

		ImGui::EndFrame();
	}

	if (wasCursorOnCaptionArea != _isCursorOnCaptionArea) {
		ScalingWindow::Get().OnCursorOnOverlayCaptionAreaChanged(_isCursorOnCaptionArea);
	}

	HRESULT hr = _imguiImpl.Draw(graphicsContext, frameFenceValue, completedFenceValue);
	if (FAILED(hr)) {
		Logger::Get().ComError("ImGuiImpl::Draw 失败", hr);
		return hr;
	}

	return S_OK;
}

void OverlayDrawer::OnColorInfoChanged(const ColorInfo& colorInfo) noexcept {
	_imguiImpl.OnColorInfoChanged(colorInfo);
}

void OverlayDrawer::MessageHandler(UINT msg, WPARAM wParam, LPARAM) noexcept {
	if (_AnyVisibleWindow()) {
		if (_imguiImpl.MessageHandler(msg, wParam)) {
			_hasMouseInput = true;
		}
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
	const bool isWin11 = Win32Helper::GetOSVersion().IsWin11();

	ImFontAtlas& fontAtlas = *ImGui::GetIO().Fonts;
	fontAtlas.Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight | ImFontAtlasFlags_NoMouseCursors;

	ImFontConfig fontConfig;

	const std::string systemFontsFolder = StrHelper::UTF16ToUTF8(GetSystemFontsFolder());

	std::string segUIPath = systemFontsFolder + (isWin11 ? "\\SegUIVar.ttf" : "\\segoeui.ttf");
	strcpy_s(fontConfig.Name, "ui");
	// 不设置 ImGuiIO::FontDefault 时默认字体是第一个创建的字体
	fontAtlas.AddFontFromFileTTF(segUIPath.c_str(), 0.0f, &fontConfig);
	
	{
		fontConfig.MergeMode = true;
		
		// 一些语言需要加载额外的字体:
		// 简体中文 -> Microsoft YaHei UI (我们把它作为所有语言的回退字体)
		// 繁体中文 -> Microsoft JhengHei UI
		// 日语 -> Yu Gothic UI
		// 韩语/朝鲜语 -> Malgun Gothic
		// 泰米尔语 -> Nirmala UI
		// 参见 https://learn.microsoft.com/en-us/windows/apps/design/style/typography#fonts-for-non-latin-languages
		std::string fontPath;
		bool needFallback = true;

		std::wstring_view language = LocalizationService::Get().GetLanguage();
		if (language == L"zh-hant") {
			// msjh.ttc: 0 是 Microsoft JhengHei，1 是 Microsoft JhengHei UI
			fontPath = systemFontsFolder + "\\msjh.ttc";
			fontConfig.FontNo = 1;
			needFallback = false;
		} else if (language == L"ja") {
			// YuGothM.ttc: 0 是 Yu Gothic Medium，1 是 Yu Gothic UI
			fontPath = systemFontsFolder + "\\YuGothM.ttc";
			fontConfig.FontNo = 1;
		} else if (language == L"ko") {
			fontPath = systemFontsFolder + "\\malgun.ttf";
		} else if (language == L"ta") {
			fontPath = systemFontsFolder + "\\Nirmala.ttf";
		}

		if (!fontPath.empty()) {
			fontAtlas.AddFontFromFileTTF(fontPath.c_str(), 0.0f, &fontConfig);
		}

		// 使用 Microsoft YaHei UI 作为回退字体
		if (needFallback) {
			// msyh.ttc: 0 是微软雅黑，1 是 Microsoft YaHei UI
			fontPath = systemFontsFolder + "\\msyh.ttc";
			fontConfig.FontNo = 1;
			fontAtlas.AddFontFromFileTTF(fontPath.c_str(), 0.0f, &fontConfig);
		}
	}

	std::string iconFontPath = systemFontsFolder +
		(isWin11 ? "\\SegoeIcons.ttf" : "\\segmdl2.ttf");
	strcpy_s(fontConfig.Name, "icon");
	fontConfig.MergeMode = false;
	fontConfig.FontNo = 0;
	_iconFont = fontAtlas.AddFontFromFileTTF(iconFontPath.c_str(), 0.0f, &fontConfig);

	// 等宽数字字体
	strcpy_s(fontConfig.Name, "mono-numbers");
	fontConfig.GlyphMinAdvanceX = ImGui::GetStyle().FontSizeBase * 0.42f;
	fontConfig.GlyphMaxAdvanceX = fontConfig.GlyphMinAdvanceX;
	_fontMonoNumbers = fontAtlas.AddFontFromFileTTF(
		segUIPath.c_str(), ImGui::GetStyle().FontSizeBase, &fontConfig);

	return true;
}

bool OverlayDrawer::_AnyVisibleWindow() const noexcept {
	bool result = _isToolbarVisible || _isProfilerVisible;
#ifdef _DEBUG
	result = result || _isDemoWindowVisible;
#endif
	return result;
}

static std::string IconLabel(ImWchar iconChar) noexcept {
	const wchar_t text[] = { iconChar, L'\0' };
	return StrHelper::UTF16ToUTF8(text);
}

bool OverlayDrawer::_DrawToolbar(uint32_t fps, POINT cursorPos, int& /*itemId*/) noexcept {
	bool needRedraw = false;

	const float windowWidth = 360 * _dpiScale;
	ImGui::SetNextWindowSize({ windowWidth, (CORNER_ROUNDING + 31) * _dpiScale });
	ImGui::SetNextWindowPos(
		ImVec2((ImGui::GetIO().DisplaySize.x - windowWidth) / 2, -CORNER_ROUNDING * _dpiScale));

	_lastToolbarAlpha = _CalcToolbarAlpha(cursorPos);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, _lastToolbarAlpha);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImU32)ImColor(15, 15, 15, 230));
	const ImVec2 originalWindowPadding = ImGui::GetStyle().WindowPadding;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 6 * _dpiScale,0.0f });

	_isToolbarItemActive = false;

	if (ImGui::Begin(StrHelper::Concat("##", TOOLBAR_WINDOW_ID).c_str(), nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse
	)) {
		// 通过工具栏拖拽缩放窗口时不要更新 _isCursorOnCaptionArea
		if (!_isResizing && !_isMoving) {
			// 鼠标被 ImGui 捕获时禁止拖拽缩放窗口
			_isCursorOnCaptionArea = !ImGui::IsAnyMouseDown();
			if (_isCursorOnCaptionArea) {
				// 检查鼠标是否被其他窗口遮挡
				_isCursorOnCaptionArea = _imguiImpl.GetHoveredWindowId() == TOOLBAR_WINDOW_ID;
			}
		}

		ImGui::SetCursorPosY((CORNER_ROUNDING + 3) * _dpiScale);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, originalWindowPadding);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4 * _dpiScale,4 * _dpiScale });
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4 * _dpiScale);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4 * _dpiScale, 0.0f });
		// 禁用仅为阻止交互，不应有视觉改变
		ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, { 0,0,0,0 });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.118f, 0.533f, 0.894f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.118f, 0.533f, 0.894f, 0.8f });

		auto drawToggleButton = [&](bool& value, ImWchar icon, const char* tooltip) {
			bool stylePushed = value;
			if (stylePushed) {
				ImGui::PushStyleColor(ImGuiCol_Button, { 0.118f, 0.533f, 0.894f, 0.8f });
			}

			ImGui::PushFont(_iconFont, 16.0f);
			if (ImGui::Button(IconLabel(icon).c_str())) {
				value = !value;
				needRedraw = true;
			}

			ImGui::PopFont();

			const bool isItemHovered = ImGui::IsItemHovered();

			if (isItemHovered) {
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(tooltip);
				ImGui::EndTooltip();
			}

			if (isItemHovered || ImGui::IsItemActive()) {
				_isCursorOnCaptionArea = false;
				_isToolbarItemActive = true;
			}

			if (stylePushed) {
				ImGui::PopStyleColor();
			}
		};

		auto drawButton = [&](ImWchar icon, const char* tooltip, const char* description = nullptr) {
			ImGui::PushFont(_iconFont, 16.0f);
			const bool clicked = ImGui::Button(IconLabel(icon).c_str());
			ImGui::PopFont();

			const bool isItemHovered = ImGui::IsItemHovered();

			if (isItemHovered) {
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(tooltip);
				if (description) {
					ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f,1.0f,1.0f,0.6f });
					ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 0.9f);
					ImGui::TextUnformatted(description);
					ImGui::PopFont();
					ImGui::PopStyleColor();
				}
				ImGui::EndTooltip();
			}

			if (isItemHovered || ImGui::IsItemActive()) {
				_isCursorOnCaptionArea = false;
				_isToolbarItemActive = true;
			}

			return clicked;
		};

		const std::string& pinStr = GetLocalizedString(L"Overlay_Toolbar_Pin");
		drawToggleButton(_isToolbarPinned, OverlayHelper::SegoeIcons::Pinned, pinStr.c_str());
		ImGui::SameLine();
		const std::string& profilerStr = GetLocalizedString(L"Overlay_Toolbar_Profiler");
		drawToggleButton(_isProfilerVisible, OverlayHelper::SegoeIcons::Diagnostic, profilerStr.c_str());
#ifdef _DEBUG
		ImGui::SameLine();
		const std::string& demoStr = GetLocalizedString(L"Overlay_Toolbar_Demo");
		drawToggleButton(_isDemoWindowVisible, OverlayHelper::SegoeIcons::Design, demoStr.c_str());
#endif
		ImGui::SameLine();
		const std::string& screenshotStr = GetLocalizedString(L"Overlay_Toolbar_TakeScreenshot");
		const std::string& screenshotDescStr = GetLocalizedString(L"Overlay_Toolbar_TakeScreenshot_Description");

		// 提示文字追加快捷键
		const OverlayOptions& overlayOptions = ScalingWindow::Get().Options().overlayOptions;
		std::string screenshotButtonStr =
			StrHelper::Concat(screenshotStr, " (", overlayOptions.takeScreenshotShortcut, ")");

		if (drawButton(OverlayHelper::SegoeIcons::Camera, screenshotButtonStr.c_str(), screenshotDescStr.c_str())) {
			ScalingWindow::Get().TakeScreenshot();
		}
		// 截图按钮右键菜单
		/*if (ImGui::BeginPopupContextItem()) {
			_isCursorOnCaptionArea = false;
			_isToolbarItemActive = true;

			ImGui::SeparatorText(GetLocalizedString(L"Overlay_Toolbar_TakeScreenshot_PopupTitle").c_str());
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f * _dpiScale);
			ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 6.0f * _dpiScale);

			const std::vector<const EffectDesc*>& effectDescs =
				ScalingWindow::Get().Renderer().ActiveEffectDescs();
			const uint32_t effectCount = (uint32_t)effectDescs.size();

			const bool isDeveloperMode = ScalingWindow::Get().Options().IsDeveloperMode();
			for (uint32_t i = 0; i < effectCount; ++i) {
				const EffectDesc& effectDesc = *effectDescs[i];
				std::string_view effectName = GetEffectDisplayName(effectDesc);

				if (isDeveloperMode && effectDesc.passes.size() > 1) {
					// 开发者模式允许保存任意通道的输出
					ImGui::PushID(itemId++);
					if (ImGui::BeginMenu(effectName.data())) {
						const uint32_t passCount = (uint32_t)effectDesc.passes.size();
						for (uint32_t j = 0; j < passCount; ++j) {
							const EffectPassDesc& passDesc = effectDesc.passes[j];
							const uint32_t outputCount = (uint32_t)passDesc.outputs.size();

							if (outputCount == 1) {
								ImGui::PushID(itemId++);
								if (ImGui::MenuItem(passDesc.desc.c_str())) {
									ScalingWindow::Get().Renderer().TakeScreenshot(i, j);
								}
								ImGui::PopID();
							} else {
								ImGui::PushID(itemId++);
								if (ImGui::BeginMenu(passDesc.desc.c_str())) {
									for (uint32_t k = 0; k < outputCount; ++k) {
										ImGui::PushID(itemId++);
										if (ImGui::MenuItem(effectDesc.textures[passDesc.outputs[k]].name.c_str())) {
											ScalingWindow::Get().Renderer().TakeScreenshot(i, j, k);
										}
										ImGui::PopID();
									}

									ImGui::EndMenu();
								}
								ImGui::PopID();
							}
						}

						ImGui::EndMenu();
					}
					ImGui::PopID();
				} else {
					ImGui::PushID(itemId++);
					if (ImGui::MenuItem(effectName.data())) {
						ScalingWindow::Get().Renderer().TakeScreenshot(i);
					}
					ImGui::PopID();
				}
			}

			ImGui::PopStyleVar();
			ImGui::EndPopup();
		}*/
		ImGui::SameLine(0, 0);

		const float contentRegionMax = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;

		// 居中绘制 FPS
		{
			std::string fpsNumberStr = StrHelper::ToString(fps);
			float cursorPosY = (CORNER_ROUNDING + 1) * _dpiScale;
			
			ImGui::PushFont(_fontMonoNumbers, 0.0f);
			float fpsTextWidth = ImGui::CalcTextSize(fpsNumberStr.c_str()).x;
			ImGui::PopFont();
			fpsTextWidth += ImGui::CalcTextSize(" FPS").x;

			ImGui::SetCursorPosX((contentRegionMax - fpsTextWidth) / 2);
			ImGui::SetCursorPosY(cursorPosY);
			ImGui::PushFont(_fontMonoNumbers, 0.0f);
			ImGui::TextUnformatted(fpsNumberStr.c_str());
			ImGui::PopFont();
			ImGui::SameLine(0, 0);
			
			ImGui::SetCursorPosY(cursorPosY);
			ImGui::TextUnformatted(" FPS");
			ImGui::SameLine(0, 0);
		}
		
		ImGui::SetCursorPosY((CORNER_ROUNDING + 3) * _dpiScale);

		// 源窗口支持最小化时才显示最小化按钮
		const HWND hwndSrc = ScalingWindow::Get().SrcHandle();
		const bool canSrcMinimized = GetWindowStyle(hwndSrc) & WS_MINIMIZEBOX;
		ImGui::SetCursorPosX(contentRegionMax - ((canSrcMinimized ? 3 : 2) * 28 - 4) * _dpiScale);

		if (canSrcMinimized) {
			const std::string& minimizeStr = GetLocalizedString(L"Overlay_Toolbar_Minimize");
			const ImWchar icon = Win32Helper::GetOSVersion().IsWin11() ?
				OverlayHelper::SegoeIcons::CheckboxIndeterminate : OverlayHelper::SegoeIcons::Remove;
			if (drawButton(icon, minimizeStr.c_str())) {
				// 模拟通过标题栏最小化，失败则回落到 ShowWindow
				if (!PostMessage(hwndSrc, WM_SYSCOMMAND, SC_MINIMIZE, 0)) {
					ShowWindowAsync(hwndSrc, SW_SHOWMINIMIZED);
				}
			}
			ImGui::SameLine();
		}

		const bool isWindowedMode = ScalingWindow::Get().Options().IsWindowedMode();

		{
			const ImWchar icon = isWindowedMode ?
				OverlayHelper::SegoeIcons::FullScreen : OverlayHelper::SegoeIcons::Favicon;
			const std::string& switchScalingStr = GetLocalizedString(
				isWindowedMode ? L"Overlay_Toolbar_SwitchToFullscreen" : L"Overlay_Toolbar_SwitchToWindowed");

			// 提示文字追加快捷键
			const std::string& scaleShortcut =
				isWindowedMode ? overlayOptions.scaleShortcut : overlayOptions.windowedModeScaleShortcut;
			std::string switchScalingButtonStr = StrHelper::Concat(switchScalingStr, " (", scaleShortcut, ")");

			if (drawButton(icon, switchScalingButtonStr.c_str())) {
				ScalingWindow::Dispatcher().TryEnqueue([]() {
					ScalingWindow::Get().ToggleScaling(!ScalingWindow::Get().Options().IsWindowedMode());
				});
			}
		}
		ImGui::SameLine();

		// 和主窗口保持一致 (#C42B1C)
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.769f, 0.169f, 0.11f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.769f, 0.169f, 0.11f, 0.8f });

		const std::string& closeStr = GetLocalizedString(L"Overlay_Toolbar_Close");
		const std::string& closeDescStr = GetLocalizedString(L"Overlay_Toolbar_Close_Description");

		// 提示文字追加快捷键
		const std::string& closeShortcut =
			isWindowedMode ? overlayOptions.windowedModeScaleShortcut : overlayOptions.scaleShortcut;
		std::string closeButtonStr =
			StrHelper::Concat(closeStr, " (", closeShortcut, ")");

		if (drawButton(OverlayHelper::SegoeIcons::Cancel, closeButtonStr.c_str(), closeDescStr.c_str())) {
			ScalingWindow::Dispatcher().TryEnqueue([]() {
				ScalingWindow::Get().Stop();
			});
		}
		/*if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
			ScalingWindow::Dispatcher().TryEnqueue([this, runId(ScalingWindow::RunId())]() {
				if (runId == ScalingWindow::RunId()) {
					ToolbarState(ToolbarState::Off);
					ScalingWindow::Get().Renderer().Render(true);
				}
			});
		}*/

		ImGui::PopStyleColor(5);
		ImGui::PopStyleVar(6);
	} else {
		_isCursorOnCaptionArea = false;
	}
	ImGui::End();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);

	return needRedraw;
}

float OverlayDrawer::_CalcToolbarAlpha(POINT cursorPos) const noexcept {
	if (_isResizing || _isMoving) {
		// 调整缩放窗口大小时不能按需渲染，所以不要改变工具栏透明度
		return _lastToolbarAlpha;
	}

	// 鼠标被工具栏中的按钮捕获时不要隐藏工具栏
	if (_isToolbarPinned || _isToolbarItemActive) {
		return 1.0f;
	}

	std::optional<ImVec4> windowRect = _imguiImpl.GetWindowRect(TOOLBAR_WINDOW_ID);
	if (!windowRect) {
		return 0.0f;
	}

	// 为了裁掉圆角，顶部有一部分在屏幕外
	windowRect->y = 0.0f;

	// ImGui::GetIO().MousePos 在调整缩放窗口大小或鼠标被前台窗口捕获时不是真实位置，这里应重新计算
	const float cursorX = float(cursorPos.x - _destRect.left);
	const float cursorY = float(cursorPos.y - _destRect.top);

	// 计算离边或角最短的距离
	float dist = 0;
	if (cursorX < windowRect->x) {
		if (cursorY < windowRect->y) {
			dist = std::hypot(windowRect->x - cursorX, windowRect->y - cursorY);
		} else if (cursorY > windowRect->w) {
			dist = std::hypot(windowRect->x - cursorX, cursorY - windowRect->w);
		} else {
			dist = windowRect->x - cursorX;
		}
	} else if (cursorX > windowRect->z) {
		if (cursorY < windowRect->y) {
			dist = std::hypot(cursorX - windowRect->z, windowRect->y - cursorY);
		} else if (cursorY > windowRect->w) {
			dist = std::hypot(cursorX - windowRect->z, cursorY - windowRect->w);
		} else {
			dist = cursorX - windowRect->z;
		}
	} else {
		if (cursorY < windowRect->y) {
			dist = windowRect->y - cursorY;
		} else if (cursorY > windowRect->w) {
			dist = cursorY - windowRect->w;
		} else {
			dist = 0;
		}
	}
	dist /= _dpiScale;

	return (40.0f - std::clamp(dist - 10.0f, 0.0f, 40.0f)) / 40.0f;
}

}
