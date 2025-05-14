#include "pch.h"
#include "OverlayDrawer.h"
#include "DeviceResources.h"
#include "Renderer.h"
#include "StepTimer.h"
#include "Logger.h"
#include "StrHelper.h"
#include "Win32Helper.h"
#include "FrameSourceBase.h"
#include "CommonSharedConstants.h"
#include "EffectDesc.h"
#include "OverlayHelper.h"
#include "ImGuiFontsCacheManager.h"
#include "ScalingWindow.h"
#include <ShlObj.h>
#include "CursorManager.h"

using namespace std::chrono;

namespace Magpie {

static const char* COLOR_INDICATOR = "■";
static const wchar_t COLOR_INDICATOR_W = L'■';

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

bool OverlayDrawer::Initialize(DeviceResources& deviceResources, OverlayOptions& overlayOptions) noexcept {
	_overlayOptions = &overlayOptions;
	SetDefaultWindowOptions(overlayOptions.windows);

	if (!_imguiImpl.Initialize(deviceResources)) {
		Logger::Get().Error("初始化 ImGuiImpl 失败");
		return false;
	}

	_dpiScale = GetDpiForWindow(ScalingWindow::Get().Handle()) / float(USER_DEFAULT_SCREEN_DPI);

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.PopupRounding = style.WindowRounding = CORNER_ROUNDING * _dpiScale;
	// 由于我们是按需渲染，显示 tooltip 时不要有延迟
	style.HoverFlagsForTooltipMouse = ImGuiHoveredFlags_DelayNone;
	style.FrameBorderSize = 1;
	style.FrameRounding = 2;
	style.WindowMinSize = ImVec2(10, 10);
	style.ScaleAllSizes(_dpiScale);

	if (!_BuildFonts()) {
		Logger::Get().Error("_BuildFonts 失败");
		return false;
	}

	// 将 _fontUI 设为默认字体
	ImGui::GetIO().FontDefault = _fontUI;

	// 获取硬件信息
	DXGI_ADAPTER_DESC desc{};
	HRESULT hr = deviceResources.GetGraphicsAdapter()->GetDesc(&desc);
	_hardwareInfo.gpuName = SUCCEEDED(hr) ? StrHelper::UTF16ToUTF8(desc.Description) : "UNAVAILABLE";

	UpdateAfterActiveEffectsChanged();
	return true;
}

void OverlayDrawer::Draw(
	uint32_t count,
	uint32_t fps,
	const SmallVector<float>& effectTimings,
	POINT drawOffset
) noexcept {
	// 所有窗口都不可见则跳过 ImGui 绘制
	if (!AnyVisibleWindow()) {
		return;
	}

	_lastFPS = fps;

	if (_isFirstFrame) {
		// 刚显示时需连续渲染两帧才能显示
		_isFirstFrame = false;
		++count;
	}

	const bool oldProfilerVisible = _isProfilerVisible;

	// 很多时候需要多次渲染避免呈现中间状态，但最多只渲染 10 次
	for (int i = 0; i < 10; ++i) {
		// 为了符合 Fitts 法则，鼠标在工具栏上时稍微下移逻辑位置使得在上边缘可以选中工具栏按钮
		float fittsLawAdjustment = 0;
		const char* hoveredWindowId = _imguiImpl.GetHoveredWindowId();
		if (hoveredWindowId && hoveredWindowId == std::string_view(TOOLBAR_WINDOW_ID)) {
			fittsLawAdjustment = 4 * _dpiScale;
		}

		_imguiImpl.NewFrame(_overlayOptions->windows, fittsLawAdjustment, _dpiScale);

		bool needRedraw = false;

		if (_isToolbarVisible && _DrawToolbar(fps)) {
			needRedraw = true;
		}

		if (_isProfilerVisible && _DrawProfiler(effectTimings, fps)) {
			needRedraw = true;
		}
			
		if (needRedraw) {
			++count;
		}

#ifdef _DEBUG
		if (_isDemoWindowVisible) {
			ImGui::ShowDemoWindow(&_isDemoWindowVisible);
		}
#endif
		
		// 中间状态不应执行渲染，因此调用 EndFrame 而不是 Render
		ImGui::EndFrame();
		
		if (--count == 0) {
			break;
		}
	}
	
	_imguiImpl.Draw(drawOffset);

	if (_isProfilerVisible != oldProfilerVisible) {
		Renderer& renderer = ScalingWindow::Get().Renderer();
		if (_isProfilerVisible) {
			renderer.StartProfile();
		} else {
			renderer.StopProfile();
		}
	}

	_ClearStatesIfNoVisibleWindow();
}

ToolbarState OverlayDrawer::ToolbarState() const noexcept {
	if (!_isToolbarVisible) {
		return ToolbarState::Off;
	} else {
		return _isToolbarPinned ? ToolbarState::AlwaysShow : ToolbarState::AutoHide;
	}
}

void OverlayDrawer::ToolbarState(Magpie::ToolbarState value) noexcept {
	if (ToolbarState() == value) {
		return;
	}

	if (value == ToolbarState::Off) {
		_isToolbarVisible = false;
		_ClearStatesIfNoVisibleWindow();
	} else if (value == ToolbarState::AlwaysShow) {
		_isToolbarVisible = true;
		_isToolbarPinned = true;
	} else {
		_isToolbarVisible = true;
		_isToolbarPinned = false;
	}
}

bool OverlayDrawer::AnyVisibleWindow() const noexcept {
	bool result = _isToolbarVisible || _isProfilerVisible;
#ifdef _DEBUG
	result = result || _isDemoWindowVisible;
#endif
	return result;
}

void OverlayDrawer::MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	if (AnyVisibleWindow()) {
		_imguiImpl.MessageHandler(msg, wParam, lParam);
	}
}

bool OverlayDrawer::NeedRedraw(uint32_t fps) const noexcept {
	if (!AnyVisibleWindow()) {
		return false;
	}

	if (_CalcToolbarAlpha() != _lastToolbarAlpha) {
		return true;
	}

	return _lastFPS != fps && (_lastToolbarAlpha > 1e-6f || _isProfilerVisible);
}

void OverlayDrawer::UpdateAfterActiveEffectsChanged() noexcept {
	const std::vector<const EffectDesc*>& effectDescs =
		ScalingWindow::Get().Renderer().ActiveEffectDescs();
	_timelineColors = OverlayHelper::GenerateTimelineColors(effectDescs);

	uint32_t passCount = 0;
	for (const EffectDesc* info : effectDescs) {
		passCount += (uint32_t)info->passes.size();
	}

	// 清空旧时间数据
	_effectTimingsStatistics.clear();
	_effectTimingsStatistics.resize(passCount);
	_lastestAvgEffectTimings.clear();
	_lastestAvgEffectTimings.resize(passCount);
	_lastUpdateTime = {};
}

static const std::wstring& GetAppLanguage() noexcept {
	static std::wstring language;
	if (language.empty()) {
		winrt::ResourceContext resourceContext = winrt::ResourceContext::GetForViewIndependentUse();
		language = resourceContext.QualifierValues().Lookup(L"Language");
		StrHelper::ToLowerCase(language);
	}
	return language;
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
	const std::wstring& language = GetAppLanguage();
	ImFontAtlas& fontAtlas = *ImGui::GetIO().Fonts;

	auto buildFontAtlas = [&]() {
		fontAtlas.Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight | ImFontAtlasFlags_NoMouseCursors;

		std::wstring uiFontPath = GetSystemFontsFolder();
		std::string iconFontPath = StrHelper::UTF16ToUTF8(uiFontPath);
		if (Win32Helper::GetOSVersion().IsWin11()) {
			uiFontPath += L"\\SegUIVar.ttf";
			iconFontPath += "\\SegoeIcons.ttf";
		} else {
			uiFontPath += L"\\segoeui.ttf";
			iconFontPath += "\\segmdl2.ttf";
		}

		std::vector<uint8_t> uiFontData;
		if (!Win32Helper::ReadFile(uiFontPath.c_str(), uiFontData)) {
			Logger::Get().Error("读取字体文件失败");
			return false;
		}

		// 构建 ImFontAtlas 前 ranges 不能析构，因为 ImGui 只保存了指针
		SmallVector<ImWchar> uiRanges = _BuildFontUI(language, uiFontData);
		_BuildFontIcons(iconFontPath.c_str());

		if (!fontAtlas.Build()) {
			Logger::Get().Error("构建 ImFontAtlas 失败");
			return false;
		}

		return true;
	};

	if (ScalingWindow::Get().Options().IsFontCacheDisabled()) {
		if (!buildFontAtlas()) {
			return false;
		}
	} else {
		const uint32_t dpi = (uint32_t)std::lroundf(_dpiScale * USER_DEFAULT_SCREEN_DPI);
		if (ImGuiFontsCacheManager::Get().Load(language, dpi, fontAtlas)) {
			_fontUI = fontAtlas.Fonts[0];
			_fontMonoNumbers = fontAtlas.Fonts[1];
			_fontIcons = fontAtlas.Fonts[2];
		} else {
			if (!buildFontAtlas()) {
				return false;
			}

			ImGuiFontsCacheManager::Get().Save(language, dpi, fontAtlas);
		}
	}

	if (!_imguiImpl.BuildFonts()) {
		Logger::Get().Error("构建字体失败");
		return false;
	}

	return true;
}

template <size_t SIZE>
static void SetGlyphRanges(SmallVector<ImWchar>& uiRanges, const ImWchar (&ranges)[SIZE]) noexcept {
	uiRanges.assign(std::begin(ranges), std::end(ranges));
}

// 指针重载，但不能直接使用指针
template <typename T, typename = std::enable_if_t<std::is_same_v<T, const ImWchar*>>>
static void SetGlyphRanges(SmallVector<ImWchar>& uiRanges, T ranges) noexcept {
	// 删除末尾的 0
	for (const ImWchar* range = ranges; *range; ++range) {
		uiRanges.push_back(*range);
	}
}

SmallVector<ImWchar> OverlayDrawer::_BuildFontUI(
	std::wstring_view language,
	const std::vector<uint8_t>& fontData
) noexcept {
	ImFontAtlas& fontAtlas = *ImGui::GetIO().Fonts;

	std::string extraFontPath;
	const ImWchar* extraRanges = nullptr;
	int extraFontNo = 0;
	
	SmallVector<ImWchar> ranges;
	if (language == L"en-us") {
		SetGlyphRanges(ranges, OverlayHelper::BASIC_LATIN_RANGES);
	} else if (language == L"ru" || language == L"uk") {
		SetGlyphRanges(ranges, fontAtlas.GetGlyphRangesCyrillic());
	} else if (language == L"tr" || language == L"hu" || language == L"pl") {
		SetGlyphRanges(ranges, OverlayHelper::EXTENDED_LATIN_RANGES);
	} else if (language == L"vi") {
		SetGlyphRanges(ranges, fontAtlas.GetGlyphRangesVietnamese());
	} else if (language == L"ka" && !Win32Helper::GetOSVersion().IsWin11()) {
		// Win10 中格鲁吉亚语无需加载额外字体
		SetGlyphRanges(ranges, OverlayHelper::GEORGIAN_RANGES);
	} else {
		// Basic Latin 使用默认字体
		SetGlyphRanges(ranges, OverlayHelper::BASIC_LATIN_RANGES);

		// 一些语言需要加载额外的字体:
		// 简体中文 -> Microsoft YaHei UI
		// 繁体中文 -> Microsoft JhengHei UI
		// 日语 -> Yu Gothic UI
		// 韩语/朝鲜语 -> Malgun Gothic
		// 泰米尔语 -> Nirmala UI
		// 格鲁吉亚语 -> Segoe UI (仅限 Win11)
		// 参见 https://learn.microsoft.com/en-us/windows/apps/design/style/typography#fonts-for-non-latin-languages
		if (language == L"zh-hans") {
			// msyh.ttc: 0 是微软雅黑，1 是 Microsoft YaHei UI
			extraFontPath = StrHelper::Concat(StrHelper::UTF16ToUTF8(GetSystemFontsFolder()), "\\msyh.ttc");
			extraFontNo = 1;
			extraRanges = OverlayHelper::GetGlyphRangesChineseSimplifiedOfficial();
		} else if (language == L"zh-hant") {
			// msjh.ttc: 0 是 Microsoft JhengHei，1 是 Microsoft JhengHei UI
			extraFontPath = StrHelper::Concat(StrHelper::UTF16ToUTF8(GetSystemFontsFolder()), "\\msjh.ttc");
			extraFontNo = 1;
			extraRanges = OverlayHelper::GetGlyphRangesChineseTraditionalOfficial();
		} else if (language == L"ja") {
			// YuGothM.ttc: 0 是 Yu Gothic Medium，1 是 Yu Gothic UI
			extraFontPath = StrHelper::Concat(StrHelper::UTF16ToUTF8(GetSystemFontsFolder()), "\\YuGothM.ttc");
			extraFontNo = 1;
			extraRanges = fontAtlas.GetGlyphRangesJapanese();
		} else if (language == L"ka") {
			assert(Win32Helper::GetOSVersion().IsWin11());
			// Win11 中的 Segoe UI Variable 不包含格鲁吉亚字母，需额外加载 Segoe UI
			extraFontPath = StrHelper::Concat(StrHelper::UTF16ToUTF8(GetSystemFontsFolder()), "\\segoeui.ttf");
			extraRanges = OverlayHelper::EXTRA_GEORGIAN_RANGES;
		} else if (language == L"ko") {
			extraFontPath = StrHelper::Concat(StrHelper::UTF16ToUTF8(GetSystemFontsFolder()), "\\malgun.ttf");
			extraRanges = fontAtlas.GetGlyphRangesKorean();
		} else if (language == L"ta") {
			extraFontPath = StrHelper::Concat(StrHelper::UTF16ToUTF8(GetSystemFontsFolder()), "\\Nirmala.ttf");
			extraRanges = OverlayHelper::EXTRA_TAMIL_RANGES;
		}
	}

	ranges.push_back(COLOR_INDICATOR_W);
	ranges.push_back(COLOR_INDICATOR_W);
	ranges.push_back(0);

	ImFontConfig config;
	// fontData 需要多次使用，我们自己读取并管理生命周期
	config.FontDataOwnedByAtlas = false;

	const float fontSize = 18 * _dpiScale;

	//////////////////////////////////////////////////////////
	// 
	// ranges (+ extraRanges) -> _fontUI
	// 
	//////////////////////////////////////////////////////////


#ifdef _DEBUG
	std::char_traits<char>::copy(config.Name, "_fontUI", std::size(config.Name));
#endif

	_fontUI = fontAtlas.AddFontFromMemoryTTF(
		(void*)fontData.data(), (int)fontData.size(), fontSize, &config, ranges.data());

	if (extraRanges) {
		assert(Win32Helper::FileExists(StrHelper::UTF8ToUTF16(extraFontPath).c_str()));

		// 在 MergeMode 下已有字符会跳过而不是覆盖
		config.MergeMode = true;
		config.FontNo = extraFontNo;
		// 额外字体数据由 ImGui 管理，初始化完成后释放
		config.FontDataOwnedByAtlas = true;
		fontAtlas.AddFontFromFileTTF(extraFontPath.c_str(), fontSize, &config, extraRanges);
		config.FontDataOwnedByAtlas = false;
		config.FontNo = 0;
		config.MergeMode = false;
	}

	//////////////////////////////////////////////////////////
	//
	// NUMBER_RANGES + NOT_NUMBER_RANGES -> _fontMonoNumbers
	//
	//////////////////////////////////////////////////////////

#ifdef _DEBUG
	std::char_traits<char>::copy(config.Name, "_fontMonoNumbers", std::size(config.Name));
#endif

	// 等宽的数字字符
	config.GlyphMinAdvanceX = config.GlyphMaxAdvanceX = fontSize * 0.42f;
	_fontMonoNumbers = fontAtlas.AddFontFromMemoryTTF(
		(void*)fontData.data(), (int)fontData.size(), fontSize, &config, OverlayHelper::NUMBER_RANGES);

	// 其他不等宽的字符
	config.MergeMode = true;
	config.GlyphMinAdvanceX = 0;
	config.GlyphMaxAdvanceX = std::numeric_limits<float>::max();
	fontAtlas.AddFontFromMemoryTTF(
		(void*)fontData.data(), (int)fontData.size(), fontSize, &config, OverlayHelper::NOT_NUMBER_RANGES);

	return ranges;
}

void OverlayDrawer::_BuildFontIcons(const char* fontPath) noexcept {
	ImFontConfig config;
#ifdef _DEBUG
	std::char_traits<char>::copy(config.Name, "_fontIcons", std::size(config.Name));
#endif

	const float fontSize = 16 * _dpiScale;
	_fontIcons = ImGui::GetIO().Fonts->AddFontFromFileTTF(
		fontPath, fontSize, &config, OverlayHelper::ICON_RANGES);
}

static std::string_view GetEffectDisplayName(const EffectDesc& effectDesc) noexcept {
	auto delimPos = effectDesc.name.find_last_of('\\');
	if (delimPos == std::string::npos) {
		return effectDesc.name;
	} else {
		return std::string_view(effectDesc.name.begin() + delimPos + 1, effectDesc.name.end());
	}
}

bool OverlayDrawer::_DrawTimingItem(
	int& itemId,
	const char* text,
	const ImColor* color,
	float time,
	bool isExpanded
) const noexcept {
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	const std::string timeStr = fmt::format("{:.3f} ms", time);
	const float timeWidth = _fontMonoNumbers->CalcTextSizeA(
		ImGui::GetFontSize(), FLT_MAX, 0.0f, timeStr.c_str()).x;

	// 计算布局
	static constexpr float spacingBeforeText = 3;
	static constexpr float spacingAfterText = 8;
	const float descWrapPos = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - timeWidth - spacingAfterText;
	const float descHeight = ImGui::CalcTextSize(
		text, nullptr, false, descWrapPos - ImGui::GetCursorPosX() - (color ? ImGui::CalcTextSize(COLOR_INDICATOR).x + spacingBeforeText : 0)).y;

	const float fontHeight = ImGui::GetFont()->FontSize;

	ImGui::PushID(itemId++);

	bool isHovered = false;
	if (color) {
		ImGui::Selectable("", false, 0, ImVec2(0, descHeight));
		if (ImGui::IsItemHovered()) {
			isHovered = true;
		}
		ImGui::SameLine(0, 0);

		ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)*color);

		if (descHeight >= fontHeight * 2) {
			// 不知为何 SetCursorPos 不起作用
			// 所以这里使用占位竖直居中颜色框
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
			ImGui::BeginGroup();
			ImGui::Dummy(ImVec2(0, (descHeight - fontHeight) / 2));
			ImGui::TextUnformatted(COLOR_INDICATOR);
			ImGui::EndGroup();
			ImGui::PopStyleVar();
		} else {
			ImGui::TextUnformatted(COLOR_INDICATOR);
		}
		ImGui::PopStyleColor();

		ImGui::SameLine(0, spacingBeforeText);
	}

	ImGui::PushTextWrapPos(descWrapPos);
	ImGui::TextUnformatted(text);
	ImGui::PopTextWrapPos();
	ImGui::SameLine(0, 0);

	// 描述过长导致换行时竖直居中时间
	if (color && descHeight >= fontHeight * 2) {
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (descHeight - fontHeight) / 2);
	}

	if (isExpanded) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.5f));
	}

	ImGui::PushFont(_fontMonoNumbers);
	ImGui::SetCursorPosX(descWrapPos + spacingAfterText);
	ImGui::TextUnformatted(timeStr.c_str());
	ImGui::PopFont();

	if (isExpanded) {
		ImGui::PopStyleColor();
	}

	ImGui::PopID();

	return isHovered;
}

// 返回鼠标悬停的项的序号，未悬停于任何项返回 -1
int OverlayDrawer::_DrawEffectTimings(
	int& itemId,
	const _EffectDrawInfo& drawInfo,
	bool showPasses,
	std::span<const ImColor> colors,
	bool singleEffect
) const noexcept {
	int result = -1;

	showPasses &= drawInfo.passTimings.size() > 1;

	std::string effectName;
	if (ScalingWindow::Get().Options().IsDeveloperMode()
		&& drawInfo.desc->passes[0].flags & EffectPassFlags::UseFP16) {
		// 开发者选项开启时显示效果是否使用 FP16
		effectName = StrHelper::Concat(GetEffectDisplayName(*drawInfo.desc), " (FP16)");
	} else {
		effectName = std::string(GetEffectDisplayName(*drawInfo.desc));
	}
	
	if (_DrawTimingItem(
		itemId,
		effectName.c_str(),
		(!singleEffect && !showPasses) ? &colors[0] : nullptr,
		drawInfo.totalTime,
		showPasses
	)) {
		result = 0;
	}

	if (showPasses) {
		for (size_t j = 0; j < drawInfo.passTimings.size(); ++j) {
			ImGui::Indent(16);

			if (_DrawTimingItem(
				itemId,
				drawInfo.desc->passes[j].desc.c_str(),
				&colors[j],
				drawInfo.passTimings[j]
			)) {
				result = (int)j;
			}

			ImGui::Unindent(16);
		}
	}

	return result;
}

void OverlayDrawer::_DrawTimelineItem(
	int& itemId,
	ImU32 color,
	float dpiScale,
	std::string_view name,
	float time,
	float effectsTotalTime,
	bool selected
) {
	ImGui::PushID(itemId++);

	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, color);
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, color);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, color);
	ImGui::PushStyleColor(ImGuiCol_Header, color);
	ImGui::Selectable("", selected);
	ImGui::PopStyleColor(3);

	if (ImGui::IsItemHovered() || ImGui::IsItemClicked()) {
		std::string content = fmt::format("{}\n{:.3f} ms\n{}%", name, time, std::lroundf(time / effectsTotalTime * 100));
		ImGui::PushFont(_fontMonoNumbers);
		_imguiImpl.Tooltip(content.c_str(), _dpiScale, nullptr, 500 * dpiScale);
		ImGui::PopFont();
	}

	// 空间足够时显示文字
	std::string text;
	if (selected) {
		text = fmt::format("{}%", std::lroundf(time / effectsTotalTime * 100));
	} else {
		text.assign(name);
	}

	float textWidth = ImGui::CalcTextSize(text.c_str()).x;
	float itemWidth = ImGui::GetItemRectSize().x;
	float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
	if (itemWidth - (selected ? 0 : itemSpacing) > textWidth + 4 * _dpiScale) {
		ImGui::SameLine(0, 0);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (itemWidth - textWidth - itemSpacing) / 2);
		// 竖直方向居中
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 0.5f * _dpiScale);
		ImGui::TextUnformatted(text.c_str());
	}

	ImGui::PopID();
}

static std::string IconLabel(ImWchar iconChar) noexcept {
	const wchar_t text[] = { iconChar, L'\0' };
	return StrHelper::UTF16ToUTF8(text);
}

bool OverlayDrawer::_DrawToolbar(uint32_t fps) noexcept {
	bool needRedraw = false;

	const float windowWidth = 360 * _dpiScale;
	ImGui::SetNextWindowSize({ windowWidth, (CORNER_ROUNDING + 31) * _dpiScale });
	ImGui::SetNextWindowPos(
		ImVec2((ImGui::GetIO().DisplaySize.x - windowWidth) / 2, -CORNER_ROUNDING * _dpiScale));

	_lastToolbarAlpha = _CalcToolbarAlpha();
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, _lastToolbarAlpha);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImU32)ImColor(15, 15, 15, 180));

	_isToolbarItemActive = false;

	if (ImGui::Begin(StrHelper::Concat("##", TOOLBAR_WINDOW_ID).c_str(), nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse))
	{
		{
			// 鼠标被 ImGui 捕获时禁止拖拽缩放窗口
			_isCursorOnCaptionArea = !ImGui::IsAnyMouseDown();
			if (_isCursorOnCaptionArea) {
				// 检查鼠标是否被其他窗口遮挡
				const char* hoveredWindowId = _imguiImpl.GetHoveredWindowId();
				_isCursorOnCaptionArea = hoveredWindowId &&
					hoveredWindowId == std::string_view(TOOLBAR_WINDOW_ID);
			}
		}

		ImGui::SetCursorPosY((CORNER_ROUNDING + 3) * _dpiScale);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4 * _dpiScale,4 * _dpiScale });
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4 * _dpiScale);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4 * _dpiScale, 0.0f });
		ImGui::PushStyleColor(ImGuiCol_Button, { 0,0,0,0 });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.118f, 0.533f, 0.894f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.118f, 0.533f, 0.894f, 0.8f });

		auto drawToggleButton = [&](bool& value, ImWchar icon, const char* tooltip) {
			bool stylePushed = value;
			if (stylePushed) {
				ImGui::PushStyleColor(ImGuiCol_Button, { 0.118f, 0.533f, 0.894f, 0.8f });
			}

			ImGui::PushFont(_fontIcons);
			if (ImGui::Button(IconLabel(icon).c_str())) {
				value = !value;
				needRedraw = true;
			}
			if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
				_isCursorOnCaptionArea = false;
				_isToolbarItemActive = true;
			}
			ImGui::PopFont();
			if (ImGui::IsItemHovered() || ImGui::IsItemClicked()) {
				_imguiImpl.Tooltip(tooltip, _dpiScale);
			}

			if (stylePushed) {
				ImGui::PopStyleColor();
			}
		};

		auto drawButton = [&](ImWchar icon, const char* tooltip, const char* description = nullptr) {
			ImGui::PushFont(_fontIcons);
			const bool clicked = ImGui::Button(IconLabel(icon).c_str());
			if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
				_isCursorOnCaptionArea = false;
				_isToolbarItemActive = true;
			}
			ImGui::PopFont();
			if (ImGui::IsItemHovered() || ImGui::IsItemClicked()) {
				_imguiImpl.Tooltip(tooltip, _dpiScale, description);
			}
			return clicked;
		};

		const std::string pinStr = _GetResourceString(L"Overlay_Toolbar_Pin");
		drawToggleButton(_isToolbarPinned, OverlayHelper::SegoeIcons::Pinned, pinStr.c_str());
		ImGui::SameLine();
		const std::string profilerStr = _GetResourceString(L"Overlay_Toolbar_Profiler");
		drawToggleButton(_isProfilerVisible, OverlayHelper::SegoeIcons::Diagnostic, profilerStr.c_str());
#ifdef _DEBUG
		ImGui::SameLine();
		const std::string demoStr = _GetResourceString(L"Overlay_Toolbar_Demo");
		drawToggleButton(_isDemoWindowVisible, OverlayHelper::SegoeIcons::Design, demoStr.c_str());
#endif
		ImGui::SameLine();
		const std::string screenshotStr = _GetResourceString(L"Overlay_Toolbar_TakeScreenshot");
		if (drawButton(OverlayHelper::SegoeIcons::Camera, screenshotStr.c_str(), "右键可以保存中间结果")) {
			const std::vector<const EffectDesc*>& effectDescs =
				ScalingWindow::Get().Renderer().ActiveEffectDescs();
			ScalingWindow::Get().Renderer().TakeScreenshot((uint32_t)effectDescs.size() - 1);
		}
		// 截图按钮右键菜单
		if (ImGui::BeginPopupContextItem()) {
			_isCursorOnCaptionArea = false;
			_isToolbarItemActive = true;

			ImGui::SeparatorText("选择一个效果");
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f * _dpiScale);
			ImGui::PushStyleVarY(ImGuiStyleVar_SelectableTextAlign, 0.5f);

			const std::vector<const EffectDesc*>& effectDescs =
				ScalingWindow::Get().Renderer().ActiveEffectDescs();
			const uint32_t effectCount = (uint32_t)effectDescs.size();
			for (uint32_t i = 0; i < effectCount; ++i) {
				std::string_view effectName = GetEffectDisplayName(*effectDescs[i]);
				if (ImGui::Selectable(effectName.data(), false, 0, ImVec2(0, 22.0f * _dpiScale))) {
					ScalingWindow::Get().Renderer().TakeScreenshot(i);
				}
			}

			ImGui::PopStyleVar();
			ImGui::EndPopup();
		}

		// 居中绘制 FPS
		ImGui::SameLine();
		const std::string fpsText = fmt::format("{} FPS", fps);
		ImGui::SetCursorPosX((ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(fpsText.c_str()).x) / 2);
		ImGui::SetCursorPosY((CORNER_ROUNDING + 1) * _dpiScale);
		ImGui::PushFont(_fontMonoNumbers);
		ImGui::TextUnformatted(fpsText.c_str());
		ImGui::PopFont();

		ImGui::SameLine();
		ImGui::SetCursorPosY((CORNER_ROUNDING + 3) * _dpiScale);
		ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 50 * _dpiScale);

		// 和主窗口保持一致 (#C42B1C)
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.769f, 0.169f, 0.11f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.769f, 0.169f, 0.11f, 0.8f });

		const std::string stopScalingStr = _GetResourceString(L"Overlay_Toolbar_StopScaling");
		if (drawButton(OverlayHelper::SegoeIcons::BackToWindow, stopScalingStr.c_str())) {
			ScalingWindow::Get().Dispatcher().TryEnqueue([]() {
				ScalingWindow::Get().Destroy();
			});
		}

		ImGui::SameLine();

		const std::string closeStr = _GetResourceString(L"Overlay_Toolbar_Close");
		if (drawButton(OverlayHelper::SegoeIcons::Cancel, closeStr.c_str())) {
			ScalingWindow::Get().Dispatcher().TryEnqueue([this]() {
				if (ScalingWindow::Get()) {
					ToolbarState(ToolbarState::Off);
					ScalingWindow::Get().Renderer().Render(true);
				}
			});
		}

		ImGui::PopStyleColor(5);
		ImGui::PopStyleVar(4);
	} else {
		_isCursorOnCaptionArea = false;
	}
	ImGui::End();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	
	return needRedraw;
}

#ifdef MP_DEBUG_OVERLAY
static std::string RectToStr(const RECT& rect) noexcept {
	return fmt::format("{},{},{},{} ({}x{})",
		rect.left, rect.top, rect.right, rect.bottom,
		rect.right - rect.left, rect.bottom - rect.top);
}
#endif

// 返回 true 表示应再渲染一次
bool OverlayDrawer::_DrawProfiler(const SmallVector<float>& effectTimings, uint32_t fps) noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();
	const Renderer& renderer = ScalingWindow::Get().Renderer();

	const uint32_t passCount = (uint32_t)_effectTimingsStatistics.size();

	bool needRedraw = false;
	
	// effectTimings 为空表示后端没有渲染新的帧
	if (!effectTimings.empty()) {
		steady_clock::time_point now = steady_clock::now();
		if (_lastUpdateTime == steady_clock::time_point{}) {
			// 后端渲染的第一帧
			_lastUpdateTime = now;

			for (uint32_t i = 0; i < passCount; ++i) {
				_lastestAvgEffectTimings[i] = effectTimings[i];
			}
		} else {
			if (now - _lastUpdateTime > 500ms) {
				// 更新间隔不少于 500ms，而不是 500ms 更新一次
				_lastUpdateTime = now;

				for (uint32_t i = 0; i < passCount; ++i) {
					auto& [total, count] = _effectTimingsStatistics[i];
					if (count > 0) {
						_lastestAvgEffectTimings[i] = total / count;
					}

					count = 0;
					total = 0;
				}
			}

			for (uint32_t i = 0; i < passCount; ++i) {
				auto& [total, count] = _effectTimingsStatistics[i];
				// 有时会跳过某些效果的渲染，即渲染时间为 0，这时不应计入
				if (effectTimings[i] > 1e-3) {
					++count;
					total += effectTimings[i];
				}
			}
		}
	}

	{
		const float windowWidth = 310 * _dpiScale;
		ImGui::SetNextWindowSizeConstraints(ImVec2(windowWidth, 0.0f), ImVec2(windowWidth, 500 * _dpiScale));
	}

	std::string profilerStr =
		StrHelper::Concat(_GetResourceString(L"Overlay_Profiler"), "##", PROFILER_WINDOW_ID);
	if (!ImGui::Begin(profilerStr.c_str(), &_isProfilerVisible, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::End();
		return needRedraw;
	}

	ImGui::PushTextWrapPos();
	ImGui::TextUnformatted(StrHelper::Concat("GPU: ", _hardwareInfo.gpuName).c_str());
	const std::string& captureMethodStr = _GetResourceString(L"Overlay_Profiler_CaptureMethod");
	ImGui::TextUnformatted(StrHelper::Concat(captureMethodStr.c_str(), ": ", renderer.FrameSource().Name()).c_str());
	if (options.IsStatisticsForDynamicDetectionEnabled() &&
		options.duplicateFrameDetectionMode == DuplicateFrameDetectionMode::Dynamic) {
		const std::pair<uint32_t, uint32_t> statistics =
			renderer.FrameSource().GetStatisticsForDynamicDetection();
		ImGui::TextUnformatted(StrHelper::Concat(_GetResourceString(L"Overlay_Profiler_DynamicDetection"), ": ").c_str());
		ImGui::SameLine(0, 0);
		ImGui::PushFont(_fontMonoNumbers);
		ImGui::TextUnformatted(fmt::format("{}/{} ({:.1f}%)", statistics.first, statistics.second,
			statistics.second == 0 ? 0.0f : statistics.first * 100.0f / statistics.second).c_str());
		ImGui::PopFont();
	}
	const std::string& frameRateStr = _GetResourceString(L"Overlay_Profiler_FrameRate");
	ImGui::TextUnformatted(fmt::format("{}: {} FPS", frameRateStr, fps).c_str());
	ImGui::PopTextWrapPos();

	const std::vector<const EffectDesc*>& effectDescs = renderer.ActiveEffectDescs();
	const uint32_t nEffect = (uint32_t)effectDescs.size();

	SmallVector<_EffectDrawInfo, 4> effectDrawInfos(effectDescs.size());

	{
		uint32_t idx = 0;
		for (uint32_t i = 0; i < nEffect; ++i) {
			_EffectDrawInfo& drawInfo = effectDrawInfos[i];
			drawInfo.desc = effectDescs[i];

			uint32_t nPass = (uint32_t)drawInfo.desc->passes.size();
			drawInfo.passTimings = { _lastestAvgEffectTimings.begin() + idx, nPass };
			idx += nPass;

			for (float t : drawInfo.passTimings) {
				drawInfo.totalTime += t;
			}
		}
	}

	static bool showPasses = false;

	// 开发者选项
	if (options.IsDeveloperMode()) {
		ImGui::Spacing();
		const std::string& developerOptionsStr = _GetResourceString(L"Home_Advanced_DeveloperOptions/Header");
		if (ImGui::CollapsingHeader(developerOptionsStr.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
			bool showSwitchButton = false;
			for (const _EffectDrawInfo& drawInfo : effectDrawInfos) {
				// 某个效果有多个通道，显示切换按钮
				if (drawInfo.passTimings.size() > 1) {
					showSwitchButton = true;
					break;
				}
			}
			
			if (showSwitchButton) {
				const std::string& buttonStr = _GetResourceString(showPasses
					? L"Overlay_Profiler_Timings_SwitchToEffects"
					: L"Overlay_Profiler_Timings_SwitchToPasses");
				if (ImGui::Button(buttonStr.c_str())) {
					showPasses = !showPasses;
					// 需要再次渲染以处理滚动条导致的布局变化
					needRedraw = true;
				}
			} else {
				showPasses = false;
			}
		}
	} else {
		showPasses = false;
	}

#ifdef MP_DEBUG_OVERLAY
	ImGui::Spacing();
	if (ImGui::CollapsingHeader("调试信息", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::TextUnformatted(StrHelper::Concat("源矩形: ",
			RectToStr(renderer.SrcRect())).c_str());
		ImGui::TextUnformatted(StrHelper::Concat("目标矩形: ",
			RectToStr(renderer.DestRect())).c_str());
		ImGui::TextUnformatted(StrHelper::Concat("渲染矩形: ",
			RectToStr(ScalingWindow::Get().RendererRect())).c_str());
		RECT scalingWndRect;
		GetWindowRect(ScalingWindow::Get().Handle(), &scalingWndRect);
		ImGui::TextUnformatted(StrHelper::Concat("缩放窗口矩形: ",
			RectToStr(scalingWndRect)).c_str());

		bool isTopMost = GetWindowExStyle(ScalingWindow::Get().Handle()) & WS_EX_TOPMOST;
		ImGui::TextUnformatted(
			StrHelper::Concat("缩放窗口置顶: ", isTopMost ? "是" : "否").c_str());

		ImGui::TextUnformatted(StrHelper::Concat("已捕获光标: ",
			ScalingWindow::Get().CursorManager().IsCursorCaptured() ? "是" : "否").c_str());

		RECT cursorClip;
		GetClipCursor(&cursorClip);
		ImGui::TextUnformatted(StrHelper::Concat("光标限制区域: ",
			RectToStr(cursorClip)).c_str());
	}
#endif

	ImGui::Spacing();
	// 效果渲染用时
	const std::string& timingsStr = _GetResourceString(L"Overlay_Profiler_Timings");
	if (ImGui::CollapsingHeader(timingsStr.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
		float effectsTotalTime = 0.0f;
		for (const _EffectDrawInfo& drawInfo : effectDrawInfos) {
			effectsTotalTime += drawInfo.totalTime;
		}

		SmallVector<ImColor, 4> colors;
		colors.reserve(_timelineColors.size());
		if (nEffect == 1) {
			colors.resize(_timelineColors.size());
			for (size_t i = 0; i < _timelineColors.size(); ++i) {
				colors[i] = OverlayHelper::TIMELINE_COLORS[_timelineColors[i]];
			}
		} else if (showPasses) {
			uint32_t i = 0;
			for (const _EffectDrawInfo& drawInfo : effectDrawInfos) {
				if (drawInfo.passTimings.size() == 1) {
					colors.push_back(OverlayHelper::TIMELINE_COLORS[_timelineColors[i]]);
					++i;
					continue;
				}

				++i;
				for (uint32_t j = 0; j < drawInfo.passTimings.size(); ++j) {
					colors.push_back(OverlayHelper::TIMELINE_COLORS[_timelineColors[i]]);
					++i;
				}
			}
		} else {
			size_t i = 0;
			for (const _EffectDrawInfo& drawInfo : effectDrawInfos) {
				colors.push_back(OverlayHelper::TIMELINE_COLORS[_timelineColors[i]]);

				++i;
				if (drawInfo.passTimings.size() > 1) {
					i += drawInfo.passTimings.size();
				}
			}
		}

		static int selectedIdx = -1;
		// 防止 ID 冲突
		int itemId = 0;

		if (nEffect > 1 || showPasses) {
			ImGui::Spacing();
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

			if (effectsTotalTime > 0) {
				if (showPasses) {
					if (ImGui::BeginTable("timeline", (int)passCount)) {
						for (uint32_t i = 0; i < passCount; ++i) {
							if (_lastestAvgEffectTimings[i] < 1e-3f) {
								continue;
							}

							ImGui::TableSetupColumn(
								std::to_string(i).c_str(),
								ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder,
								_lastestAvgEffectTimings[i] / effectsTotalTime
							);
						}

						ImGui::TableNextRow();

						uint32_t i = 0;
						for (const _EffectDrawInfo& drawInfo : effectDrawInfos) {
							for (uint32_t j = 0, end = (uint32_t)drawInfo.passTimings.size(); j < end; ++j) {
								if (drawInfo.passTimings[j] < 1e-5f) {
									continue;
								}

								ImGui::TableNextColumn();

								std::string name;
								if (drawInfo.passTimings.size() == 1) {
									name = std::string(GetEffectDisplayName(*drawInfo.desc));
								} else if (nEffect == 1) {
									name = drawInfo.desc->passes[j].desc;
								} else {
									name = StrHelper::Concat(
										GetEffectDisplayName(*drawInfo.desc), "/",
										drawInfo.desc->passes[j].desc
									);
								}

								_DrawTimelineItem(itemId, colors[i], _dpiScale, name, drawInfo.passTimings[j],
									effectsTotalTime, selectedIdx == (int)i);

								++i;
							}
						}

						ImGui::EndTable();
					}
				} else {
					if (ImGui::BeginTable("timeline", nEffect)) {
						for (uint32_t i = 0; i < nEffect; ++i) {
							if (effectDrawInfos[i].totalTime < 1e-5f) {
								continue;
							}

							ImGui::TableSetupColumn(
								std::to_string(i).c_str(),
								ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder,
								effectDrawInfos[i].totalTime / effectsTotalTime
							);
						}

						ImGui::TableNextRow();

						for (uint32_t i = 0; i < nEffect; ++i) {
							auto& drawInfo = effectDrawInfos[i];
							if (drawInfo.totalTime < 1e-5f) {
								continue;
							}

							ImGui::TableNextColumn();
							_DrawTimelineItem(
								itemId,
								colors[i],
								_dpiScale,
								GetEffectDisplayName(*drawInfo.desc),
								drawInfo.totalTime,
								effectsTotalTime,
								selectedIdx == (int)i
							);
						}

						ImGui::EndTable();
					}
				}
			} else {
				// 还未统计出时间时渲染占位
				if (ImGui::BeginTable("timeline", 1)) {
					ImGui::TableSetupColumn("0", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImU32 color = ImColor();
					ImGui::PushStyleColor(ImGuiCol_HeaderActive, color);
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, color);
					ImGui::Selectable("");
					ImGui::PopStyleColor(2);

					ImGui::EndTable();
				}
			}

			ImGui::PopStyleVar(4);

			ImGui::Spacing();
		}
		
		selectedIdx = -1;

		if (ImGui::BeginTable("timings", 1, ImGuiTableFlags_PadOuterX)) {
			ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder);

			if (nEffect == 1) {
				int hovered = _DrawEffectTimings(itemId, effectDrawInfos[0], showPasses, colors, true);
				if (hovered >= 0) {
					selectedIdx = hovered;
				}
			} else {
				int idx = 0;
				for (const _EffectDrawInfo& effectDesc : effectDrawInfos) {
					int idxBegin = idx;

					std::span<const ImColor> colorSpan;
					if (!showPasses || effectDesc.passTimings.size() == 1) {
						colorSpan = std::span(colors.begin() + idx, colors.begin() + idx + 1);
						++idx;
					} else {
						colorSpan = std::span(colors.begin() + idx, colors.begin() + idx + effectDesc.passTimings.size());
						idx += (int)effectDesc.passTimings.size();
					}

					int hovered = _DrawEffectTimings(itemId, effectDesc, showPasses, colorSpan, false);
					if (hovered >= 0) {
						selectedIdx = idxBegin + hovered;
					}
				}
			}

			ImGui::EndTable();
		}

		if (nEffect > 1) {
			ImGui::Separator();

			if (ImGui::BeginTable("total", 1, ImGuiTableFlags_PadOuterX)) {
				ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder);

				_DrawTimingItem(itemId, _GetResourceString(L"Overlay_Profiler_Timings_Total").c_str(), nullptr, effectsTotalTime);

				ImGui::EndTable();
			}
		}
	}
	
	ImGui::End();
	return needRedraw;
}

const std::string& OverlayDrawer::_GetResourceString(const std::wstring_view& key) noexcept {
	static phmap::flat_hash_map<std::wstring_view, std::string> cache;

	if (auto it = cache.find(key); it != cache.end()) {
		return it->second;
	}

	return cache[key] = StrHelper::UTF16ToUTF8(ScalingWindow::Get().GetLocalizedString(key));
}

float OverlayDrawer::_CalcToolbarAlpha() const noexcept {
	if (ScalingWindow::Get().IsResizingOrMoving()) {
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
	const POINT cursorPos = ScalingWindow::Get().CursorManager().CursorPos();
	const RECT& destRect = ScalingWindow::Get().Renderer().DestRect();
	const float cursorX = float(cursorPos.x - destRect.left);
	const float cursorY = float(cursorPos.y - destRect.top);

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

void OverlayDrawer::_ClearStatesIfNoVisibleWindow() noexcept {
	if (AnyVisibleWindow()) {
		return;
	}

	_imguiImpl.ClearStates();
	_isCursorOnCaptionArea = false;
	_isToolbarItemActive = false;
}

}
