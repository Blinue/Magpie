#include "pch.h"
#include "OverlayDrawer.h"
#include "DeviceResources.h"
#include "Renderer.h"
#include "StepTimer.h"
#include "Logger.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include "FrameSourceBase.h"
#include "CommonSharedConstants.h"
#include "EffectDesc.h"
#include <bit>	// std::bit_ceil
#include <random>
#include "ImGuiHelper.h"
#include "ImGuiFontsCacheManager.h"
#include "ScalingWindow.h"

namespace Magpie::Core {

bool OverlayDrawer::Initialize(DeviceResources* deviceResources) noexcept {
	HWND hwndSrc = ScalingWindow::Get().HwndSrc();
	_isSrcMainWnd = Win32Utils::GetWndClassName(hwndSrc) == CommonSharedConstants::MAIN_WINDOW_CLASS_NAME;

	if (!_imguiImpl.Initialize(deviceResources)) {
		Logger::Get().Error("初始化 ImGuiImpl 失败");
		return false;
	}

	_dpiScale = GetDpiForWindow(ScalingWindow::Get().Handle()) / 96.0f;

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.PopupRounding = style.WindowRounding = 6;
	style.FrameBorderSize = 1;
	style.FrameRounding = 2;
	style.WindowMinSize = ImVec2(10, 10);
	style.ScaleAllSizes(_dpiScale);

	if (!_BuildFonts()) {
		Logger::Get().Error("_BuildFonts 失败");
		return false;
	}

	// 获取硬件信息
	DXGI_ADAPTER_DESC desc{};
	HRESULT hr = deviceResources->GetGraphicsAdapter()->GetDesc(&desc);
	_hardwareInfo.gpuName = SUCCEEDED(hr) ? StrUtils::UTF16ToUTF8(desc.Description) : "UNAVAILABLE";

	// 将 _fontUI 设为默认字体
	ImGui::GetIO().FontDefault = _fontUI;

	return true;
}

void OverlayDrawer::Draw(uint32_t count) noexcept {
	bool isShowFPS = ScalingWindow::Get().Options().IsShowFPS();

	if (!_isUIVisiable && !isShowFPS) {
		return;
	}

	if (_isFirstFrame) {
		// 刚显示时需连续渲染三帧：第一帧不会显示，第二帧不会将窗口限制在视口内
		_isFirstFrame = false;
		count = 3;
	}

	for (uint32_t i = 0; i < count; ++i) {
		_imguiImpl.NewFrame();

		if (isShowFPS) {
			_DrawFPS();
		}

		if (_isUIVisiable) {
			_DrawUI();
		}

		ImGui::Render();
	}
	
	_imguiImpl.Draw();
}

void OverlayDrawer::SetUIVisibility(bool value) noexcept {
	if (_isUIVisiable == value) {
		return;
	}
	_isUIVisiable = value;

	if (value) {
		Logger::Get().Info("已开启叠加层");
	} else {
		if (!ScalingWindow::Get().Options().IsShowFPS()) {
			_imguiImpl.ClearStates();
		}

		Logger::Get().Info("已关闭叠加层");
	}
}

void OverlayDrawer::MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	if (_isUIVisiable || ScalingWindow::Get().Options().IsShowFPS()) {
		_imguiImpl.MessageHandler(msg, wParam, lParam);
	}
}

static const std::wstring& GetAppLanguage() noexcept {
	static std::wstring language;
	if (language.empty()) {
		winrt::ResourceContext resourceContext = winrt::ResourceContext::GetForViewIndependentUse();
		language = resourceContext.QualifierValues().Lookup(L"Language");
		StrUtils::ToLowerCase(language);
	}
	return language;
}

static const std::wstring& GetSystemFontsFolder() noexcept {
	static std::wstring result;

	if (result.empty()) {
		wchar_t* fontsFolder = nullptr;
		HRESULT hr = SHGetKnownFolderPath(FOLDERID_Fonts, 0, NULL, &fontsFolder);
		if (FAILED(hr)) {
			CoTaskMemFree(fontsFolder);
			Logger::Get().ComError("SHGetKnownFolderPath 失败", hr);
			return result;
		}

		result = fontsFolder;
		CoTaskMemFree(fontsFolder);
	}

	return result;
}

bool OverlayDrawer::_BuildFonts() noexcept {
	const std::wstring& language = GetAppLanguage();
	ImFontAtlas& fontAtlas = *ImGui::GetIO().Fonts;

	const bool fontCacheDisabled = ScalingWindow::Get().Options().IsFontCacheDisabled();
	if (!fontCacheDisabled && ImGuiFontsCacheManager::Get().Load(language, fontAtlas)) {
		_fontUI = fontAtlas.Fonts[0];
		_fontMonoNumbers = fontAtlas.Fonts[1];
		_fontFPS = fontAtlas.Fonts[2];
	} else {
		fontAtlas.Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight | ImFontAtlasFlags_NoMouseCursors;

		std::wstring fontPath = GetSystemFontsFolder();
		if (Win32Utils::GetOSVersion().IsWin11()) {
			fontPath += L"\\SegUIVar.ttf";
		} else {
			fontPath += L"\\segoeui.ttf";
		}

		std::vector<uint8_t> fontData;
		if (!Win32Utils::ReadFile(fontPath.c_str(), fontData)) {
			Logger::Get().Error("读取字体文件失败");
			return false;
		}

		{
			// 构建 ImFontAtlas 前 uiRanges 不能析构，因为 ImGui 只保存了指针
			ImVector<ImWchar> uiRanges;
			_BuildFontUI(language, fontData, uiRanges);
			_BuildFontFPS(fontData);

			if (!fontAtlas.Build()) {
				Logger::Get().Error("构建 ImFontAtlas 失败");
				return false;
			}
		}

		if (!fontCacheDisabled) {
			ImGuiFontsCacheManager::Get().Save(language, fontAtlas);
		}
	}

	if (!_imguiImpl.BuildFonts()) {
		Logger::Get().Error("构建字体失败");
		return false;
	}

	return true;
}

void OverlayDrawer::_BuildFontUI(
	std::wstring_view language,
	const std::vector<uint8_t>& fontData,
	ImVector<ImWchar>& uiRanges
) noexcept {
	ImFontAtlas& fontAtlas = *ImGui::GetIO().Fonts;

	std::string extraFontPath;
	const ImWchar* extraRanges = nullptr;
	int extraFontNo = 0;

	ImFontGlyphRangesBuilder builder;
	
	if (language == L"en-us") {
		builder.AddRanges(ImGuiHelper::ENGLISH_RANGES);
	} else if (language == L"ru" || language == L"uk") {
		builder.AddRanges(fontAtlas.GetGlyphRangesCyrillic());
	} else if (language == L"tr" || language == L"hu") {
		builder.AddRanges(ImGuiHelper::Latin_1_Extended_A_RANGES);
	} else if (language == L"vi") {
		builder.AddRanges(fontAtlas.GetGlyphRangesVietnamese());
	} else {
		// 默认 Basic Latin + Latin-1 Supplement
		// 参见 https://en.wikipedia.org/wiki/Latin-1_Supplement
		builder.AddRanges(fontAtlas.GetGlyphRangesDefault());

		// 一些语言需要加载额外的字体：
		// 简体中文 -> Microsoft YaHei UI
		// 繁体中文 -> Microsoft JhengHei UI
		// 日语 -> Yu Gothic UI
		// 韩语/朝鲜语 -> Malgun Gothic
		// 参见 https://learn.microsoft.com/en-us/windows/apps/design/style/typography#fonts-for-non-latin-languages
		if (language == L"zh-hans") {
			// msyh.ttc: 0 是微软雅黑，1 是 Microsoft YaHei UI
			extraFontPath = StrUtils::Concat(StrUtils::UTF16ToUTF8(GetSystemFontsFolder()), "\\msyh.ttc");
			extraFontNo = 1;
			extraRanges = ImGuiHelper::GetGlyphRangesChineseSimplifiedOfficial();
		} else if (language == L"zh-hant") {
			// msjh.ttc: 0 是 Microsoft JhengHei，1 是 Microsoft JhengHei UI
			extraFontPath = StrUtils::Concat(StrUtils::UTF16ToUTF8(GetSystemFontsFolder()), "\\msjh.ttc");
			extraFontNo = 1;
			extraRanges = ImGuiHelper::GetGlyphRangesChineseTraditionalOfficial();
		} else if (language == L"ja") {
			// YuGothM.ttc: 0 是 Yu Gothic Medium，1 是 Yu Gothic UI
			extraFontPath = StrUtils::Concat(StrUtils::UTF16ToUTF8(GetSystemFontsFolder()), "\\YuGothM.ttc");
			extraFontNo = 1;
			extraRanges = fontAtlas.GetGlyphRangesJapanese();
		} else if (language == L"ko") {
			extraFontPath = StrUtils::Concat(StrUtils::UTF16ToUTF8(GetSystemFontsFolder()), "\\malgun.ttf");
			extraRanges = fontAtlas.GetGlyphRangesKorean();
		}
	}
	builder.SetBit(L'■');
	builder.BuildRanges(&uiRanges);

	ImFontConfig config;
	config.FontDataOwnedByAtlas = false;

	const float fontSize = 18 * _dpiScale;

	//////////////////////////////////////////////////////////
	// 
	// uiRanges (+ extraRanges) -> _fontUI
	// 
	//////////////////////////////////////////////////////////


#ifdef _DEBUG
	std::char_traits<char>::copy(config.Name, "_fontUI", std::size(config.Name));
#endif

	_fontUI = fontAtlas.AddFontFromMemoryTTF(
		(void*)fontData.data(), (int)fontData.size(), fontSize, &config, uiRanges.Data);

	if (extraRanges) {
		assert(Win32Utils::FileExists(StrUtils::UTF8ToUTF16(extraFontPath).c_str()));

		// 在 MergeMode 下已有字符会跳过而不是覆盖
		config.MergeMode = true;
		config.FontNo = extraFontNo;
		// 额外字体数据由 ImGui 管理，退出缩放时释放
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
		(void*)fontData.data(), (int)fontData.size(), fontSize, &config, ImGuiHelper::NUMBER_RANGES);

	// 其他不等宽的字符
	config.MergeMode = true;
	config.GlyphMinAdvanceX = 0;
	config.GlyphMaxAdvanceX = std::numeric_limits<float>::max();
	fontAtlas.AddFontFromMemoryTTF(
		(void*)fontData.data(), (int)fontData.size(), fontSize, &config, ImGuiHelper::NOT_NUMBER_RANGES);
}

void OverlayDrawer::_BuildFontFPS(const std::vector<uint8_t>& fontData) noexcept {
	ImFontAtlas& fontAtlas = *ImGui::GetIO().Fonts;

	ImFontConfig config;
	config.FontDataOwnedByAtlas = false;

	const float fpsSize = 24 * _dpiScale;

	//////////////////////////////////////////////////////////
	//
	// NUMBER_RANGES + " FPS" -> _fontFPS
	// 
	//////////////////////////////////////////////////////////

#ifdef _DEBUG
	std::char_traits<char>::copy(config.Name, "_fontFPS", std::size(config.Name));
#endif

	// 等宽的数字字符
	config.MergeMode = false;
	config.GlyphMinAdvanceX = config.GlyphMaxAdvanceX = fpsSize * 0.42f;
	_fontFPS = fontAtlas.AddFontFromMemoryTTF(
		(void*)fontData.data(), (int)fontData.size(), fpsSize, &config, ImGuiHelper::NUMBER_RANGES);

	// 其他不等宽的字符
	config.MergeMode = true;
	config.GlyphMinAdvanceX = 0;
	config.GlyphMaxAdvanceX = std::numeric_limits<float>::max();
	fontAtlas.AddFontFromMemoryTTF(
		(void*)fontData.data(), (int)fontData.size(), fpsSize, &config, (const ImWchar*)L"  FFPPSS");
}

int OverlayDrawer::_DrawEffectTimings(const _EffectTimings& /*et*/, bool /*showPasses*/, float /*maxWindowWidth*/, std::span<const ImColor> /*colors*/, bool /*singleEffect*/) noexcept {
	return 0;
}

void OverlayDrawer::_DrawTimelineItem(ImU32 /*color*/, float /*dpiScale*/, std::string_view /*name*/, float /*time*/, float /*effectsTotalTime*/, bool /*selected*/) {
}

void OverlayDrawer::_DrawFPS() noexcept {
}

void OverlayDrawer::_DrawUI() noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();
	const Renderer& renderer = ScalingWindow::Get().Renderer();
	//auto& gpuTimer = renderer();

#ifdef _DEBUG
	ImGui::ShowDemoWindow();
#endif

	const float maxWindowWidth = 400 * _dpiScale;
	ImGui::SetNextWindowSizeConstraints(ImVec2(), ImVec2(maxWindowWidth, 500 * _dpiScale));

	static float initPosX = Win32Utils::GetSizeOfRect(renderer.DestRect()).cx - maxWindowWidth;
	ImGui::SetNextWindowPos(ImVec2(initPosX, 20), ImGuiCond_FirstUseEver);

	std::string profilerStr = _GetResourceString(L"Overlay_Profiler");
	if (!ImGui::Begin(profilerStr.c_str(), nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::End();
		return;
	}

	// 始终为滚动条预留空间
	ImGui::PushTextWrapPos(maxWindowWidth - ImGui::GetStyle().WindowPadding.x - ImGui::GetStyle().ScrollbarSize);
	ImGui::TextUnformatted(StrUtils::Concat("GPU: ", _hardwareInfo.gpuName).c_str());
	const std::string& captureMethodStr = _GetResourceString(L"Overlay_Profiler_CaptureMethod");
	ImGui::TextUnformatted(StrUtils::Concat(captureMethodStr.c_str(), ": ", renderer.FrameSource().Name()).c_str());
	if (options.IsStatisticsForDynamicDetectionEnabled() &&
		options.duplicateFrameDetectionMode == DuplicateFrameDetectionMode::Dynamic) {
		const std::pair<uint32_t, uint32_t> statistics =
			renderer.FrameSource().GetStatisticsForDynamicDetection();
		ImGui::TextUnformatted(StrUtils::Concat(_GetResourceString(L"Overlay_Profiler_DynamicDetection"), ": ").c_str());
		ImGui::SameLine(0, 0);
		ImGui::PushFont(_fontMonoNumbers);
		ImGui::TextUnformatted(fmt::format("{}/{} ({:.1f}%)", statistics.first, statistics.second,
			statistics.second == 0 ? 0.0f : statistics.first * 100.0f / statistics.second).c_str());
		ImGui::PopFont();
	}
	ImGui::PopTextWrapPos();
	

	ImGui::End();
}

void OverlayDrawer::_EnableSrcWnd(bool /*enable*/) noexcept {
}

const std::string& OverlayDrawer::_GetResourceString(const std::wstring_view& key) noexcept {
	static phmap::flat_hash_map<std::wstring_view, std::string> cache;

	if (auto it = cache.find(key); it != cache.end()) {
		return it->second;
	}

	return cache[key] = StrUtils::UTF16ToUTF8(_resourceLoader.GetString(key));
}

}
