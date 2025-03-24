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

OverlayDrawer::OverlayDrawer() :
	_resourceLoader(winrt::ResourceLoader::GetForViewIndependentUse(CommonSharedConstants::APP_RESOURCE_MAP_ID))
{}

bool OverlayDrawer::Initialize(DeviceResources* deviceResources) noexcept {
	if (!_imguiImpl.Initialize(deviceResources)) {
		Logger::Get().Error("初始化 ImGuiImpl 失败");
		return false;
	}

	_dpiScale = GetDpiForWindow(ScalingWindow::Get().Handle()) / float(USER_DEFAULT_SCREEN_DPI);

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.PopupRounding = style.WindowRounding = CORNER_ROUNDING * _dpiScale;
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
	HRESULT hr = deviceResources->GetGraphicsAdapter()->GetDesc(&desc);
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
	if (!_isVisible) {
		return;
	}

	_lastFPS = fps;

	if (_isFirstFrame) {
		// 刚显示时需连续渲染两帧才能显示
		_isFirstFrame = false;
		++count;
	}

	// 很多时候需要多次渲染避免呈现中间状态，但最多只渲染 10 次
	for (int i = 0; i < 10; ++i) {
		_imguiImpl.NewFrame();

		if (_isVisible) {
			bool needRedraw = _DrawToolbar(fps);

			if (_isProfilerVisible && _DrawProfiler(effectTimings, fps)) {
				needRedraw = true;
			}
			
			if (needRedraw) {
				++count;
			}

#ifdef _DEBUG
			if (_isDemoWindowVisible) {
				ImGui::ShowDemoWindow();
			}
#endif
		}

		// 中间状态不应执行渲染，因此调用 EndFrame 而不是 Render
		ImGui::EndFrame();
		
		if (--count == 0) {
			break;
		}
	}
	
	_imguiImpl.Draw(drawOffset);
}

// 3D 游戏模式下关闭叠加层将激活源窗口，但有时不希望这么做，比如用户切换
// 窗口导致停止缩放。通过 noSetForeground 禁止激活源窗口
void OverlayDrawer::IsVisible(bool value) noexcept {
	if (_isVisible == value) {
		return;
	}
	_isVisible = value;

	if (value) {
		Logger::Get().Info("已开启叠加层");
	} else {
		_imguiImpl.ClearStates();
		Logger::Get().Info("已关闭叠加层");
	}
}

void OverlayDrawer::MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	if (_isVisible) {
		_imguiImpl.MessageHandler(msg, wParam, lParam);
	}
}

bool OverlayDrawer::NeedRedraw(uint32_t fps) const noexcept {
	if (!_isVisible) {
		return false;
	}

	if (_lastFPS != fps) {
		return true;
	}

	// ImGui::GetIO().MousePos 尚未更新
	POINT cursorPos = ScalingWindow::Get().CursorManager().CursorPos();
	const RECT& destRect = ScalingWindow::Get().Renderer().DestRect();
	ImVec2 imguiCursorPos = {
		float(cursorPos.x - destRect.left),
		float(cursorPos.y - destRect.top)
	};
	return _CalcToolbarAlpha(imguiCursorPos) != _lastToolbarAlpha;
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
		ImGuiImpl::Tooltip(content.c_str(), 500 * dpiScale);
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
	const Renderer& renderer = ScalingWindow::Get().Renderer();

	bool needRedraw = false;

	float windowWidth = 400 * _dpiScale;
	ImGui::SetNextWindowSize({ windowWidth, 37 * _dpiScale });
	LONG rendererWidth = renderer.DestRect().right - renderer.DestRect().left;
	ImGui::SetNextWindowPos({ (rendererWidth - windowWidth) / 2, -CORNER_ROUNDING * _dpiScale });

	_lastToolbarAlpha = _CalcToolbarAlpha(ImGui::GetIO().MousePos);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, _lastToolbarAlpha);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, (ImU32)ImColor(15, 15, 15, 180));
	if (ImGui::Begin("toolbar", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
		ImGui::SetCursorPosY(9 * _dpiScale);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4 * _dpiScale,4 * _dpiScale });
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4 * _dpiScale);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4 * _dpiScale, 0.0f });
		ImGui::PushStyleColor(ImGuiCol_Button, { 0,0,0,0 });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.118f, 0.533f, 0.894f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.118f, 0.533f, 0.894f, 0.8f });

		bool originVal = _isToolbarPinned;
		if (originVal) {
			ImGui::PushStyleColor(ImGuiCol_Button, { 0.118f, 0.533f, 0.894f, 0.8f });
		}

		ImGui::PushFont(_fontIcons);
		if (ImGui::Button(IconLabel(OverlayHelper::SegoeIcons::Pinned).c_str())) {
			_isToolbarPinned = !_isToolbarPinned;
			needRedraw = true;
		}
		ImGui::PopFont();
		ImGui::SetItemTooltip("固定工具栏");

		if (originVal) {
			ImGui::PopStyleColor();
		}

		originVal = _isProfilerVisible;
		if (originVal) {
			ImGui::PushStyleColor(ImGuiCol_Button, { 0.118f, 0.533f, 0.894f, 0.8f });
		}

		ImGui::SameLine();
		ImGui::PushFont(_fontIcons);
		if (ImGui::Button(IconLabel(OverlayHelper::SegoeIcons::Diagnostic).c_str())) {
			_isProfilerVisible = !_isProfilerVisible;
			needRedraw = true;
		}
		ImGui::PopFont();
		ImGui::SetItemTooltip("性能分析器");

		if (originVal) {
			ImGui::PopStyleColor();
		}

#ifdef _DEBUG
		originVal = _isDemoWindowVisible;
		if (originVal) {
			ImGui::PushStyleColor(ImGuiCol_Button, { 0.118f, 0.533f, 0.894f, 0.8f });
		}

		ImGui::SameLine();
		ImGui::PushFont(_fontIcons);
		if (ImGui::Button(IconLabel(OverlayHelper::SegoeIcons::Design).c_str())) {
			_isDemoWindowVisible = !_isDemoWindowVisible;
			needRedraw = true;
		}
		ImGui::PopFont();
		ImGui::SetItemTooltip("ImGui 演示窗口");

		if (originVal) {
			ImGui::PopStyleColor();
		}
#endif

		ImGui::SameLine();
		ImGui::PushFont(_fontIcons);
		if (ImGui::Button(IconLabel(OverlayHelper::SegoeIcons::Camera).c_str())) {
			ScalingWindow::Get().ShowToast(L"截图已保存到 C:\\Users\\XX\\Pictures\\Screenshots");
		}
		ImGui::PopFont();
		ImGui::SetItemTooltip("保存截图");

		ImGui::SameLine();
		const std::string fpsText = fmt::format("{} FPS", fps);
		ImGui::SetCursorPosX((ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(fpsText.c_str()).x) / 2);
		ImGui::SetCursorPosY(7 * _dpiScale);
		ImGui::TextUnformatted(fpsText.c_str());

		ImGui::SameLine();
		ImGui::SetCursorPosY(9 * _dpiScale);
		ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 50 * _dpiScale);

		// 和主窗口保持一致 (#C42B1C)
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.769f, 0.169f, 0.11f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.769f, 0.169f, 0.11f, 0.8f });

		ImGui::PushFont(_fontIcons);
		if (ImGui::Button(IconLabel(OverlayHelper::SegoeIcons::BackToWindow).c_str())) {
			ScalingWindow::Get().Dispatcher().TryEnqueue([]() {
				ScalingWindow::Get().Destroy();
			});
		}
		ImGui::PopFont();
		ImGui::SetItemTooltip("停止缩放");

		ImGui::SameLine();
		ImGui::PushFont(_fontIcons);
		if (ImGui::Button(IconLabel(OverlayHelper::SegoeIcons::Cancel).c_str())) {
			ScalingWindow::Get().Dispatcher().TryEnqueue([]() {
				ScalingWindow::Get().ToggleOverlay();
			});
		}
		ImGui::PopFont();
		ImGui::SetItemTooltip("关闭叠加层");

		ImGui::PopStyleColor(2);

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(4);
	}
	ImGui::End();

	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	
	return needRedraw;
}

static std::string RectToStr(const RECT& rect) noexcept {
	return fmt::format("{},{},{},{} ({}x{})",
		rect.left, rect.top, rect.right, rect.bottom,
		rect.right - rect.left, rect.bottom - rect.top);
}

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

		static float initPosX = Win32Helper::GetSizeOfRect(renderer.DestRect()).cx - windowWidth;
		ImGui::SetNextWindowPos(ImVec2(initPosX, 20), ImGuiCond_FirstUseEver);
	}

	std::string profilerStr = _GetResourceString(L"Overlay_Profiler");
	if (!ImGui::Begin(profilerStr.c_str(), nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize)) {
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
	if (options.IsDeveloperMode() && GetAppLanguage() == L"zh-hans") {
		ImGui::Spacing();
		if (ImGui::CollapsingHeader("开发者选项", ImGuiTreeNodeFlags_DefaultOpen)) {
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
	} else {
		showPasses = false;
	}

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

	return cache[key] = StrHelper::UTF16ToUTF8(_resourceLoader.GetString(key));
}

float OverlayDrawer::_CalcToolbarAlpha(ImVec2 cursorPos) const noexcept {
	if (_isToolbarPinned) {
		return 1.0f;
	}

	std::optional<ImVec4> windowRect = _imguiImpl.GetWindowRect("toolbar");
	if (!windowRect) {
		return 0.0f;
	}

	// 为了裁掉圆角，顶部有一部分在屏幕外
	windowRect->y = 0.0f;

	// 计算离边或角最短的距离
	float dist = 0;
	if (cursorPos.x < windowRect->x) {
		if (cursorPos.y < windowRect->y) {
			dist = std::hypot(windowRect->x - cursorPos.x, windowRect->y - cursorPos.y);
		} else if (cursorPos.y > windowRect->w) {
			dist = std::hypot(windowRect->x - cursorPos.x, cursorPos.y - windowRect->w);
		} else {
			dist = windowRect->x - cursorPos.x;
		}
	} else if (cursorPos.x > windowRect->z) {
		if (cursorPos.y < windowRect->y) {
			dist = std::hypot(cursorPos.x - windowRect->z, windowRect->y - cursorPos.y);
		} else if (cursorPos.y > windowRect->w) {
			dist = std::hypot(cursorPos.x - windowRect->z, cursorPos.y - windowRect->w);
		} else {
			dist = cursorPos.x - windowRect->z;
		}
	} else {
		if (cursorPos.y < windowRect->y) {
			dist = windowRect->y - cursorPos.y;
		} else if (cursorPos.y > windowRect->w) {
			dist = cursorPos.y - windowRect->w;
		} else {
			dist = 0;
		}
	}
	dist /= _dpiScale;

	return (40.0f - std::clamp(dist - 10.0f, 0.0f, 40.0f)) / 40.0f;
}

}
