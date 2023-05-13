#include "pch.h"
#include "OverlayDrawer.h"
#include "MagApp.h"
#include "DeviceResources.h"
#include "ImGuiImpl.h"
#include "Renderer.h"
#include "GPUTimer.h"
#include "Logger.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include "FrameSourceBase.h"
#include "CommonSharedConstants.h"
#include "EffectDesc.h"
#include <bit>	// std::bit_ceil
#include <Wbemidl.h>
#include <random>

#pragma comment(lib, "wbemuuid.lib")

namespace Magpie::Core {

OverlayDrawer::OverlayDrawer() noexcept {
	HWND hwndSrc = MagApp::Get().GetHwndSrc();
	_isSrcMainWnd = Win32Utils::GetWndClassName(hwndSrc) == CommonSharedConstants::MAIN_WINDOW_CLASS_NAME;
}

OverlayDrawer::~OverlayDrawer() {
	if (MagApp::Get().GetOptions().Is3DGameMode() && IsUIVisiable()) {
		_EnableSrcWnd(true);
	}
}

static const ImColor TIMELINE_COLORS[] = {
	{229,57,53,255},
	{156,39,176,255},
	{63,81,181,255},
	{30,136,229,255},
	{0,137,123,255},
	{121,85,72,255},
	{117,117,117,255}
};

static UINT GetSeed() {
	Renderer& renderer = MagApp::Get().GetRenderer();
	UINT nEffect = renderer.GetEffectCount();

	UINT result = 0;
	for (UINT i = 0; i < nEffect; ++i) {
		result ^= (UINT)std::hash<std::string>()(renderer.GetEffectDesc(i).name);
	}
	return result;
}

static SmallVector<UINT> GenerateTimelineColors() {
	Renderer& renderer = MagApp::Get().GetRenderer();

	const UINT nEffect = renderer.GetEffectCount();
	UINT totalColors = nEffect > 1 ? nEffect : 0;
	for (UINT i = 0; i < nEffect; ++i) {
		UINT nPass = (UINT)renderer.GetEffectDesc(i).passes.size();
		if (nPass > 1) {
			totalColors += nPass;
		}
	}

	if (totalColors == 0) {
		return {};
	}

	constexpr UINT nColors = (UINT)std::size(TIMELINE_COLORS);

	std::default_random_engine randomEngine(GetSeed());
	SmallVector<UINT> result;

	if (totalColors <= nColors) {
		result.resize(nColors);
		for (UINT i = 0; i < nColors; ++i) {
			result[i] = i;
		}
		std::shuffle(result.begin(), result.end(), randomEngine);

		result.resize(totalColors);
	} else {
		// 相邻通道颜色不同，相邻效果颜色不同
		result.resize(totalColors);
		std::uniform_int_distribution<UINT> uniformDst(0, nColors - 1);

		if (nEffect <= nColors) {
			if (nEffect > 1) {
				// 确保效果的颜色不重复
				std::array<UINT, nColors> effectColors{};
				for (UINT i = 0; i < nColors; ++i) {
					effectColors[i] = i;
				}
				std::shuffle(effectColors.begin(), effectColors.end(), randomEngine);

				UINT i = 0;
				for (UINT j = 0; j < nEffect; ++j) {
					result[i] = effectColors[j];
					++i;

					UINT nPass = (UINT)renderer.GetEffectDesc(j).passes.size();
					if (nPass > 1) {
						i += nPass;
					}
				}
			}
		} else {
			// 仅确保与前一个效果颜色不同
			UINT prevColor = UINT_MAX;
			UINT i = 0;
			for (UINT j = 0; j < nEffect; ++j) {
				UINT c = uniformDst(randomEngine);
				while (c == prevColor) {
					c = uniformDst(randomEngine);
				}

				result[i] = c;
				prevColor = c;
				++i;

				UINT nPass = (UINT)renderer.GetEffectDesc(j).passes.size();
				if (nPass > 1) {
					i += nPass;
				}
			}
		}

		// 生成通道的颜色
		size_t idx = 0;
		for (UINT i = 0; i < nEffect; ++i) {
			UINT nPass = (UINT)renderer.GetEffectDesc(i).passes.size();

			if (nEffect > 1) {
				++idx;

				if (nPass == 1) {
					continue;
				}
			}

			for (UINT j = 0; j < nPass; ++j) {
				UINT c = uniformDst(randomEngine);

				if (i > 0 || j > 0) {
					UINT prevColor = (i > 0 && j == 0) ? result[idx - 2] : result[idx - 1];

					if (j + 1 == nPass && i + 1 != nEffect &&
						renderer.GetEffectDesc(i + 1).passes.size() == 1) {
					// 当前效果的最后一个通道且下一个效果只有一个通道
						UINT nextColor = result[idx + 1];
						while (c == prevColor || c == nextColor) {
							c = uniformDst(randomEngine);
						}
					} else {
						while (c == prevColor) {
							c = uniformDst(randomEngine);
						}
					}
				}

				result[idx] = c;
				++idx;
			}
		}
	}

	return result;
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

bool OverlayDrawer::Initialize() noexcept {
	_imguiImpl.reset(new ImGuiImpl());
	if (!_imguiImpl->Initialize()) {
		Logger::Get().Error("初始化 ImGuiImpl 失败");
		return false;
	}

	_dpiScale = GetDpiForWindow(MagApp::Get().GetHwndHost()) / 96.0f;

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 6;
	style.FrameBorderSize = 1;
	style.FrameRounding = 2;
	style.WindowMinSize = ImVec2(10, 10);
	style.ScaleAllSizes(_dpiScale);

	if (!_BuildFonts()) {
		Logger::Get().Error("_BuildFonts 失败");
		return false;
	}

	_RetrieveHardwareInfo();
	_timelineColors = GenerateTimelineColors();

	return true;
}

void OverlayDrawer::Draw() noexcept {
	bool isShowFPS = MagApp::Get().GetOptions().IsShowFPS();

	if (!_isUIVisiable && !isShowFPS) {
		return;
	}

	_imguiImpl->NewFrame();
	ImGui::PushFont(_fontUI);

	if (isShowFPS) {
		_DrawFPS();
	}

	if (_isUIVisiable) {
		_DrawUI();
	}

	ImGui::PopFont();
	ImGui::Render();
	_imguiImpl->EndFrame();
}

void OverlayDrawer::SetUIVisibility(bool value) noexcept {
	if (_isUIVisiable == value) {
		return;
	}
	_isUIVisiable = value;

	if (value) {
		if (MagApp::Get().GetOptions().Is3DGameMode()) {
			// 使全屏窗口不透明且可以接收焦点
			HWND hwndHost = MagApp::Get().GetHwndHost();
			INT_PTR style = GetWindowLongPtr(hwndHost, GWL_EXSTYLE);
			SetWindowLongPtr(hwndHost, GWL_EXSTYLE, style & ~(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE));
			Win32Utils::SetForegroundWindow(hwndHost);

			// 使源窗口无法接收用户输入
			_EnableSrcWnd(false);
			// 由 ImGui 绘制光标
			ImGui::GetIO().MouseDrawCursor = true;
		}

		Logger::Get().Info("已开启覆盖层");
	} else {
		_validFrames = 0;
		std::fill(_frameTimes.begin(), _frameTimes.end(), 0.0f);

		if (!MagApp::Get().GetOptions().IsShowFPS()) {
			_imguiImpl->ClearStates();
		}

		if (MagApp::Get().GetOptions().Is3DGameMode()) {
			// 还原全屏窗口样式
			HWND hwndHost = MagApp::Get().GetHwndHost();
			INT_PTR style = GetWindowLongPtr(hwndHost, GWL_EXSTYLE);
			SetWindowLongPtr(hwndHost, GWL_EXSTYLE, style | (WS_EX_TRANSPARENT | WS_EX_NOACTIVATE));

			// 重新激活源窗口
			_EnableSrcWnd(true);

			ImGui::GetIO().MouseDrawCursor = false;
		}

		Logger::Get().Info("已关闭覆盖层");
	}
}

bool OverlayDrawer::_BuildFonts() noexcept {
	static std::vector<BYTE> fontData;
	if (fontData.empty()) {
		std::wstring fontPath = GetSystemFontsFolder();
		if (Win32Utils::GetOSVersion().IsWin11()) {
			fontPath += L"\\SegUIVar.ttf";
		} else {
			fontPath += L"\\segoeui.ttf";
		}

		if (!Win32Utils::ReadFile(fontPath.c_str(), fontData)) {
			Logger::Get().Error("读取字体文件失败");
			return false;
		}
	}

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
	if (!MagApp::Get().GetOptions().Is3DGameMode()) {
		// 非 3D 游戏模式无需 ImGui 绘制光标
		io.Fonts->Flags |= ImFontAtlasFlags_NoMouseCursors;
	}
	

	const float fontSize = 18 * _dpiScale;
	const float fpsSize = 24 * _dpiScale;

	static const ImWchar NUMBER_RANGES[] = { L'0', L'9', 0 };
	static const ImWchar NOT_NUMBER_RANGES[] = { 0x20, L'0' - 1, L'9' + 1, 0x7E, 0 };

	static ImVector<ImWchar> uiRanges;
	// 额外字体体积巨大（Microsoft YaHei UI 超过 20M），我们只缓存路径
	static std::string extraFontPath;
	static ImVector<ImWchar> extraRanges;
	if (uiRanges.empty()) {
		winrt::ResourceContext resourceContext = winrt::ResourceContext::GetForViewIndependentUse();
		std::wstring language(resourceContext.QualifierValues().Lookup(L"Language"));
		StrUtils::ToLowerCase(language);

		ImFontGlyphRangesBuilder builder;

		if (language == L"en-us") {
			// Basic Latin
			static const ImWchar ENGLISH_RANGES[] = { 0x20, 0x7E, 0 };
			builder.AddRanges(ENGLISH_RANGES);
		} else if (language == L"es" || language == L"pt") {
			// Basic Latin + Latin-1 Supplement
			// 参见 https://en.wikipedia.org/wiki/Latin-1_Supplement
			builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
		} else if (language == L"ru" || language == L"uk") {
			builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
		} else if (language == L"tr") {
			// Basic Latin + Latin-1 Supplement + Latin Extended-A
			// 参见 https://en.wikipedia.org/wiki/Latin_Extended-A
			static const ImWchar TURKISH_RANGES[] = { 0x20, 0x17F, 0 };
			builder.AddRanges(TURKISH_RANGES);
		} else {
			// 一些语言需要加载额外的字体：
			// 简体中文 -> Microsoft YaHei UI
			// 繁体中文 -> Microsoft JhengHei UI
			// 日语 -> Yu Gothic UI
			// 参见 https://learn.microsoft.com/en-us/windows/apps/design/style/typography#fonts-for-non-latin-languages

			uint8_t type;
			if (language == L"zh-hans") {
				type = 0;
			} else if (language == L"zh-hant") {
				type = 1;
			} else if (language == L"ja") {
				type = 2;
			} else {
				type = 3;
			}
			
			if (type == 3) {
				// 未知语言
				builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
			} else {
				extraFontPath = StrUtils::UTF16ToUTF8(GetSystemFontsFolder());
				if (type == 0) {
					// Microsoft JhengHei UI 是第二个字体
					extraFontPath += "\\msyh.ttc";
				} else if (type == 1) {
					// Microsoft JhengHei UI 是第二个字体
					extraFontPath += "\\msjh.ttc";
				} else {
					// Yu Gothic UI 是第二个字体
					extraFontPath += "\\YuGothM.ttc";
				}

				if (type == 0) {
					builder.AddRanges(io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
				} else if (type == 1) {
					// TODO: 有没有常用繁体中文字符集？
					builder.AddRanges(io.Fonts->GetGlyphRangesChineseFull());
				} else {
					builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
				}
				
				builder.BuildRanges(&extraRanges);

				builder.Clear();
				builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
			}
		}
		builder.SetBit(L'■');

		builder.BuildRanges(&uiRanges);
	}

	ImFontConfig config;
	config.FontDataOwnedByAtlas = false;

	//////////////////////////////////////////////////////////
	// 
	// uiRanges (+ extraRanges) -> _fontUI
	// 
	//////////////////////////////////////////////////////////

#ifdef _DEBUG
	std::char_traits<char>::copy(config.Name, "_fontUI", std::size(config.Name));
#endif

	_fontUI = io.Fonts->AddFontFromMemoryTTF(fontData.data(), (int)fontData.size(), fontSize, &config, uiRanges.Data);

	if (!extraRanges.empty()) {
		assert(Win32Utils::FileExists(StrUtils::UTF8ToUTF16(extraFontPath).c_str()));

		// 在 MergeMode 下已有字符会跳过而不是覆盖
		config.MergeMode = true;
		// 三个字体文件里我们需要的都是其中的第二个字体
		// msyh.ttc: 0 是微软雅黑，1 是 Microsoft YaHei UI
		// msjh.ttc: 0 是 Microsoft JhengHei，1 是 Microsoft JhengHei UI
		// YuGothM.ttc: 0 是 Yu Gothic Medium，1 是 Yu Gothic UI
		config.FontNo = 1;
		// 额外字体数据由 ImGui 管理，退出缩放时释放
		config.FontDataOwnedByAtlas = true;
		io.Fonts->AddFontFromFileTTF(extraFontPath.c_str(), fontSize, &config, extraRanges.Data);
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
	_fontMonoNumbers = io.Fonts->AddFontFromMemoryTTF(fontData.data(), (int)fontData.size(), fontSize, &config, NUMBER_RANGES);

	// 其他不等宽的字符
	config.MergeMode = true;
	config.GlyphMinAdvanceX = 0;
	config.GlyphMaxAdvanceX = std::numeric_limits<float>::max();
	io.Fonts->AddFontFromMemoryTTF(fontData.data(), (int)fontData.size(), fontSize, &config, NOT_NUMBER_RANGES);

	if (MagApp::Get().GetOptions().IsShowFPS()) {
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
		_fontFPS = io.Fonts->AddFontFromMemoryTTF(fontData.data(), (int)fontData.size(), fpsSize, &config, NUMBER_RANGES);

		// 其他不等宽的字符
		config.MergeMode = true;
		config.GlyphMinAdvanceX = 0;
		config.GlyphMaxAdvanceX = std::numeric_limits<float>::max();
		io.Fonts->AddFontFromMemoryTTF(fontData.data(), (int)fontData.size(), fpsSize, &config, (const ImWchar*)L"  FFPPSS");
	}

	return io.Fonts->Build();
}

static std::string_view GetEffectDisplayName(const EffectDesc* desc) noexcept {
	auto delimPos = desc->name.find_last_of('\\');
	if (delimPos == std::string::npos) {
		return desc->name;
	} else {
		return std::string_view(desc->name.begin() + delimPos + 1, desc->name.end());
	}
}

static void DrawTextWithFont(const char* text, ImFont* font) noexcept {
	ImGui::PushFont(font);
	ImGui::TextUnformatted(text);
	ImGui::PopFont();
}

// 返回鼠标悬停的项的序号，未悬停于任何项返回 -1
int OverlayDrawer::_DrawEffectTimings(
	const _EffectTimings& et,
	bool showPasses,
	float maxWindowWidth,
	std::span<const ImColor> colors,
	bool singleEffect
) noexcept {
	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	int result = -1;

	if (!singleEffect && (et.passTimings.size() == 1 || !showPasses)) {
		ImGui::Selectable("", false, ImGuiSelectableFlags_SpanAllColumns);
		if (ImGui::IsItemHovered()) {
			result = 0;
		}
		ImGui::SameLine(0, 0);

		ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)colors[0]);
		ImGui::TextUnformatted("■");
		ImGui::PopStyleColor();
		ImGui::SameLine(0, 3);
	}

	ImGui::TextUnformatted(std::string(GetEffectDisplayName(et.desc)).c_str());

	ImGui::TableNextColumn();

	const float rightAlignSpace = ImGui::CalcTextSize("0").x;

	if (et.passTimings.size() > 1) {
		if (showPasses) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 0.5f));
		}

		if (et.totalTime < 10) {
			// 右对齐
			ImGui::Dummy(ImVec2(rightAlignSpace, 0));
			ImGui::SameLine(0, 0);
		}
		DrawTextWithFont(fmt::format("{:.3f} ms", et.totalTime).c_str(), _fontMonoNumbers);

		if (showPasses) {
			ImGui::PopStyleColor();
		}

		if (showPasses) {
			for (size_t j = 0; j < et.passTimings.size(); ++j) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				ImGui::Indent(20);

				float fontHeight = ImGui::GetFont()->FontSize;
				std::string time = fmt::format("{:.3f} ms", et.passTimings[j]);
				// 手动计算布局
				// 运行到此处时还无法确定是否需要滚动条，这里始终减去滚动条的宽度，否则展开时可能会有一帧的跳跃
				float descWrap = maxWindowWidth - ImGui::CalcTextSize(time.c_str()).x - ImGui::GetStyle().WindowPadding.x - ImGui::GetStyle().ScrollbarSize - ImGui::GetStyle().CellPadding.x * 2;
				float descHeight = ImGui::CalcTextSize(et.desc->passes[j].desc.c_str(), nullptr, false, descWrap - ImGui::GetCursorPos().x - ImGui::CalcTextSize("■").x - 3).y;

				ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)colors[j]);
				if (descHeight >= fontHeight * 2) {
					// 不知为何 SetCursorPos 不起作用
					// 所以这里使用占位竖直居中颜色框
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2());
					ImGui::BeginGroup();
					ImGui::Dummy(ImVec2(0, (descHeight - fontHeight) / 2));
					ImGui::TextUnformatted("■");
					ImGui::EndGroup();
					ImGui::PopStyleVar();
				} else {
					ImGui::TextUnformatted("■");
				}

				ImGui::PopStyleColor();
				ImGui::SameLine(0, 3);

				ImGui::PushTextWrapPos(descWrap);
				ImGui::TextUnformatted(et.desc->passes[j].desc.c_str());
				ImGui::PopTextWrapPos();
				ImGui::Unindent(20);

				ImGui::SameLine(0, 0);
				ImGui::Selectable("", false, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, descHeight));
				if (ImGui::IsItemHovered()) {
					result = (int)j;
				}

				ImGui::TableNextColumn();
				// 描述过长导致换行时竖直居中时间
				if (descHeight >= fontHeight * 2) {
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (descHeight - fontHeight) / 2);
				}

				if (et.passTimings[j] < 10) {
					ImGui::Dummy(ImVec2(rightAlignSpace, 0));
					ImGui::SameLine(0, 0);
				}
				DrawTextWithFont(time.c_str(), _fontMonoNumbers);
			}
		}
	} else {
		if (et.totalTime < 10) {
			ImGui::Dummy(ImVec2(rightAlignSpace, 0));
			ImGui::SameLine(0, 0);
		}
		DrawTextWithFont(fmt::format("{:.3f} ms", et.totalTime).c_str(), _fontMonoNumbers);
	}

	return result;
}

void OverlayDrawer::_DrawTimelineItem(ImU32 color, float dpiScale, std::string_view name, float time, float effectsTotalTime, bool selected) {
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
		ImGui::TextUnformatted(text.c_str());
	}
}

void OverlayDrawer::_DrawFPS() noexcept {
	static float oldOpacity = 0.0f;
	static float opacity = 0.0f;
	static bool isLocked = false;
	// 背景透明时绘制阴影
	const bool drawShadow = opacity < 1e-5f;

	static constexpr float PADDING_X = 5;
	static constexpr float PADDING_Y = 1;

	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowBgAlpha(opacity);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, drawShadow ? ImVec2() : ImVec2(PADDING_X, PADDING_Y));
	if (!ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing | (isLocked ? ImGuiWindowFlags_NoMove : 0) | (drawShadow ? ImGuiWindowFlags_NoBackground : 0))) {
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	if (oldOpacity != opacity) {
		// 透明时无边距，确保文字位置不变
		if (oldOpacity < 1e-5f) {
			if (opacity >= 1e-5f) {
				ImVec2 windowPos = ImGui::GetWindowPos();
				ImGui::SetWindowPos(ImVec2(windowPos.x - PADDING_X, windowPos.y - PADDING_Y));
			}
		} else {
			if (opacity < 1e-5f) {
				ImVec2 windowPos = ImGui::GetWindowPos();
				ImGui::SetWindowPos(ImVec2(windowPos.x + PADDING_X, windowPos.y + PADDING_Y));
			}
		}
		oldOpacity = opacity;
	}

	ImGui::PushFont(_fontFPS);

	ImVec2 cursorPos = ImGui::GetCursorPos();
	// 不知为何文字无法竖直居中，因此这里调整位置
	cursorPos.y -= 3;
	ImGui::SetCursorPosY(cursorPos.y);

	std::string fps = fmt::format("{} FPS", MagApp::Get().GetRenderer().GetGPUTimer().GetFramesPerSecond());
	if (drawShadow) {
		ImGui::SetCursorPos(ImVec2(cursorPos.x + 1.0f, cursorPos.y + 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 0.8f));
		ImGui::TextUnformatted(fps.c_str());
		ImGui::PopStyleColor();

		ImGui::SetCursorPos(cursorPos);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 0.6f));
		ImGui::TextUnformatted(fps.c_str());
		ImGui::PopStyleColor();

		ImGui::SetCursorPos(cursorPos);
	}
	ImGui::TextUnformatted(fps.c_str());

	ImGui::PopFont();

	ImGui::PopStyleVar();

	if (ImGui::BeginPopupContextWindow()) {
		ImGui::PushFont(_fontMonoNumbers);
		ImGui::PushItemWidth(200);
		ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f);
		ImGui::Separator();
		if (ImGui::MenuItem(isLocked ? "Unlock" : "Lock", nullptr, nullptr)) {
			isLocked = !isLocked;
		}
		ImGui::PopItemWidth();
		ImGui::PopFont();
		ImGui::EndPopup();
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

#ifdef _M_X64
// 只在 x86 可用
static std::string GetCPUNameViaCPUID() {
	std::string cpuName(48, '\0');

	std::array<int, 4> cpuInfo{};

	__cpuid(cpuInfo.data(), 0);

	// Calling __cpuid with 0x80000000 as the function_id argument
	// gets the number of the highest valid extended ID.
	__cpuid(cpuInfo.data(), 0x80000000);

	if (cpuInfo[0] < 0x80000004) {
		return {};
	}

	__cpuidex(cpuInfo.data(), 0x80000002, 0);
	memcpy(cpuName.data(), cpuInfo.data(), sizeof(cpuInfo));
	__cpuidex(cpuInfo.data(), 0x80000003, 0);
	memcpy(cpuName.data() + 16, cpuInfo.data(), sizeof(cpuInfo));
	__cpuidex(cpuInfo.data(), 0x80000004, 0);
	memcpy(cpuName.data() + 32, cpuInfo.data(), sizeof(cpuInfo));

	cpuName.resize(StrUtils::StrLen(cpuName.c_str()));
	StrUtils::Trim(cpuName);
	return cpuName;
}
#endif

// 非常慢，需要大约 18 ms
static std::string GetCPUNameViaWMI() {
	winrt::com_ptr<IWbemLocator> wbemLocator = winrt::try_create_instance<IWbemLocator>(CLSID_WbemLocator);
	if (!wbemLocator) {
		Logger::Get().Error("创建 WbemLocator 失败");
		return "";
	}

	winrt::com_ptr<IWbemServices> wbemServices;
	winrt::com_ptr<IEnumWbemClassObject> enumWbemClassObject;
	winrt::com_ptr<IWbemClassObject> wbemClassObject;

	HRESULT hr = wbemLocator->ConnectServer(
		Win32Utils::BStr(L"ROOT\\CIMV2"),
		nullptr,
		nullptr,
		nullptr,
		0,
		nullptr,
		nullptr,
		wbemServices.put()
	);
	if (hr != WBEM_S_NO_ERROR) {
		return "";
	}

	hr = CoSetProxyBlanket(
		wbemServices.get(),
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		nullptr,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE
	);
	if (FAILED(hr)) {
		return "";
	}

	hr = wbemServices->ExecQuery(
		Win32Utils::BStr(L"WQL"),
		Win32Utils::BStr(L"SELECT NAME FROM Win32_Processor"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		nullptr,
		enumWbemClassObject.put()
	);
	if (hr != WBEM_S_NO_ERROR) {
		return "";
	}

	ULONG uReturn = 0;
	hr = enumWbemClassObject->Next(WBEM_INFINITE, 1, wbemClassObject.put(), &uReturn);
	if (hr != WBEM_S_NO_ERROR || uReturn <= 0) {
		return "";
	}

	VARIANT value;
	VariantInit(&value);
	hr = wbemClassObject->Get(Win32Utils::BStr(L"Name"), 0, &value, 0, 0);
	if (hr != WBEM_S_NO_ERROR || value.vt != VT_BSTR) {
		return "";
	}

	std::string result = Win32Utils::BStr(value.bstrVal).ToUTF8();
	StrUtils::Trim(result);
	return result;
}

static std::string GetCPUName() {
#ifdef _M_X64
	std::string result = GetCPUNameViaCPUID();
	if (!result.empty()) {
		return result;
	}
#endif // _M_X64

	return GetCPUNameViaWMI();
}

// 自定义提示
static void MyPlotLines(float(*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 graph_size) {
	// 通过改变光标位置避免绘制提示窗口
	const ImVec2 mousePos = ImGui::GetIO().MousePos;
	ImGui::GetIO().MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	ImGui::PlotLines("", values_getter, data, values_count, values_offset, overlay_text, scale_min, scale_max, graph_size);
	ImGui::GetIO().MousePos = mousePos;

	ImVec2 framePadding = ImGui::GetStyle().FramePadding;
	ImVec2 graphRectMin = ImGui::GetItemRectMin();
	ImVec2 graphRectMax = ImGui::GetItemRectMax();

	float innerRectLeft = graphRectMin.x + framePadding.x;
	float innerRectTop = graphRectMin.y + framePadding.y;
	float innerRectRight = graphRectMax.x - framePadding.x;
	float innerRectBottom = graphRectMax.y - framePadding.y;

	// 检查光标是否在图表上
	if (mousePos.x < innerRectLeft || mousePos.y < innerRectTop ||
		mousePos.x >= innerRectRight || mousePos.y >= innerRectBottom) {
		return;
	}

	// 获取光标位置对应的值
	float t = std::clamp((mousePos.x - innerRectLeft) / (innerRectRight - innerRectLeft), 0.0f, 0.9999f);
	int v_idx = (int)(t * values_count);
	float v0 = values_getter(data, (v_idx + values_offset) % values_count);

	ImGuiImpl::Tooltip(fmt::format("{:.1f}", v0).c_str());
}

void OverlayDrawer::_DrawUI() noexcept {
	auto& settings = MagApp::Get().GetOptions();
	auto& renderer = MagApp::Get().GetRenderer();
	auto& gpuTimer = renderer.GetGPUTimer();

#ifdef _DEBUG
	ImGui::ShowDemoWindow();
#endif

	const float maxWindowWidth = 400 * _dpiScale;
	ImGui::SetNextWindowSizeConstraints(ImVec2(), ImVec2(maxWindowWidth, 500 * _dpiScale));

	static float initPosX = Win32Utils::GetSizeOfRect(MagApp::Get().GetRenderer().GetOutputRect()).cx - maxWindowWidth;
	ImGui::SetNextWindowPos(ImVec2(initPosX, 20), ImGuiCond_FirstUseEver);

	std::string profilerStr = StrUtils::UTF16ToUTF8(_resourceLoader.GetString(L"Overlay_Profiler"));
	if (!ImGui::Begin(profilerStr.c_str(), nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::End();
		return;
	}

	// 始终为滚动条预留空间
	ImGui::PushTextWrapPos(maxWindowWidth - ImGui::GetStyle().WindowPadding.x - ImGui::GetStyle().ScrollbarSize);
	ImGui::TextUnformatted(StrUtils::Concat("GPU: ", _hardwareInfo.gpuName).c_str());
	ImGui::TextUnformatted(StrUtils::Concat("CPU: ", _hardwareInfo.cpuName).c_str());
	std::string vSyncStr = StrUtils::UTF16ToUTF8(_resourceLoader.GetString(L"Overlay_VSync"));
	std::string stateStr = StrUtils::UTF16ToUTF8(_resourceLoader.GetString(
		settings.IsVSync() ? L"ToggleSwitch/OnContent" : L"ToggleSwitch/OffContent"));
	ImGui::TextUnformatted(StrUtils::Concat(vSyncStr, ": ", stateStr).c_str());
	std::string captureMethodStr = StrUtils::UTF16ToUTF8(_resourceLoader.GetString(L"Overlay_CaptureMethod"));
	ImGui::TextUnformatted(StrUtils::Concat(captureMethodStr.c_str(), ": ", MagApp::Get().GetFrameSource().GetName()).c_str());
	ImGui::PopTextWrapPos();

	ImGui::Spacing();

	static constexpr UINT nSamples = 180;

	if (_frameTimes.size() >= nSamples) {
		_frameTimes.erase(_frameTimes.begin(), _frameTimes.begin() + (_frameTimes.size() - nSamples + 1));
	} else if (_frameTimes.size() < nSamples) {
		_frameTimes.insert(_frameTimes.begin(), nSamples - _frameTimes.size() - 1, 0);
	}
	_frameTimes.push_back(std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(gpuTimer.GetElapsedTime()).count());
	_validFrames = std::min(_validFrames + 1, nSamples);

	// 帧率统计，支持在渲染时间和 FPS 间切换
	if (ImGui::CollapsingHeader("Frame Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushFont(_fontMonoNumbers);

		static bool showFPS = true;

		if (showFPS) {
			float totalTime = 0;
			float minTime = FLT_MAX;
			float minTime2 = FLT_MAX;
			for (UINT i = nSamples - _validFrames; i < nSamples; ++i) {
				totalTime += _frameTimes[i];

				if (_frameTimes[i] <= minTime) {
					minTime2 = minTime;
					minTime = _frameTimes[i];
				} else if (_frameTimes[i] < minTime2) {
					minTime2 = _frameTimes[i];
				}
			}

			if (minTime2 == FLT_MAX) {
				minTime2 = minTime;
			}

			// 减少抖动
			// 1. 使用第二小的值以缓解尖峰导致的抖动
			// 2. 以 30 为最小变化单位
			const float maxFPS = std::bit_ceil((UINT)std::ceilf((1000 / minTime2 - 10) / 30)) * 30 * 1.7f;

			MyPlotLines([](void* data, int idx) {
				float time = (*(std::deque<float>*)data)[idx];
				return time < 1e-6 ? 0 : 1000 / time;
			}, &_frameTimes, (int)_frameTimes.size(), 0, fmt::format("avg: {:.1f} FPS", _validFrames * 1000 / totalTime).c_str(), 0, maxFPS, ImVec2(250 * _dpiScale, 80 * _dpiScale));
		} else {
			float totalTime = 0;
			float maxTime = 0;
			float maxTime2 = 0;
			for (UINT i = nSamples - _validFrames; i < nSamples; ++i) {
				totalTime += _frameTimes[i];

				if (_frameTimes[i] >= maxTime) {
					maxTime2 = maxTime;
					maxTime = _frameTimes[i];
				} else if (_frameTimes[i] > maxTime2) {
					maxTime2 = _frameTimes[i];
				}
			}

			if (maxTime2 == 0) {
				maxTime2 = maxTime;
			}

			// 使用第二大的值以缓解尖峰导致的抖动
			MyPlotLines([](void* data, int idx) {
				return (*(std::deque<float>*)data)[idx];
			}, &_frameTimes, (int)_frameTimes.size(), 0,
				fmt::format("avg: {:.1f} ms", totalTime / _validFrames).c_str(),
				0, maxTime2 * 1.7f, ImVec2(250 * _dpiScale, 80 * _dpiScale));
		}

		ImGui::PopFont();

		ImGui::Spacing();

		if (ImGui::Button(showFPS ? "Switch to timings" : "Switch to FPS")) {
			showFPS = !showFPS;
		}
	}

	ImGui::Spacing();
	if (ImGui::CollapsingHeader("Timings", ImGuiTreeNodeFlags_DefaultOpen)) {
		const auto& gpuTimings = gpuTimer.GetGPUTimings();
		const UINT nEffect = renderer.GetEffectCount();
		
		SmallVector<_EffectTimings, 4> effectTimings(nEffect);

		{
			UINT idx = 0;
			for (UINT i = 0; i < nEffect; ++i) {
				auto& effectTiming = effectTimings[i];
				effectTiming.desc = &renderer.GetEffectDesc(i);

				UINT nPass = (UINT)effectTiming.desc->passes.size();
				effectTiming.passTimings = { gpuTimings.passes.begin() + idx, nPass };
				idx += nPass;

				for (float t : effectTiming.passTimings) {
					effectTiming.totalTime += t;
				}
			}
		}

		float effectsTotalTime = 0.0f;
		for (const auto& et : effectTimings) {
			effectsTotalTime += et.totalTime;
		}

		static bool showPasses = false;
		if (nEffect == 1) {
			showPasses = effectTimings[0].passTimings.size() > 1;
		} else {
			for (const auto& et : effectTimings) {
				// 某个效果有多个通道，显示切换按钮
				if (et.passTimings.size() > 1) {
					if (ImGui::Button(showPasses ? "Switch to effects" : "Switch to passes")) {
						showPasses = !showPasses;
					}
					break;
				}
			}
		}

		SmallVector<ImColor, 4> colors;
		colors.reserve(_timelineColors.size());
		if (nEffect == 1) {
			colors.resize(_timelineColors.size());
			for (size_t i = 0; i < _timelineColors.size(); ++i) {
				colors[i] = TIMELINE_COLORS[_timelineColors[i]];
			}
		} else if (showPasses) {
			UINT i = 0;
			for (const auto& et : effectTimings) {
				if (et.passTimings.size() == 1) {
					colors.push_back(TIMELINE_COLORS[_timelineColors[i]]);
					++i;
					continue;
				}

				++i;
				for (UINT j = 0; j < et.passTimings.size(); ++j) {
					colors.push_back(TIMELINE_COLORS[_timelineColors[i]]);
					++i;
				}
			}
		} else {
			size_t i = 0;
			for (const auto& et : effectTimings) {
				colors.push_back(TIMELINE_COLORS[_timelineColors[i]]);

				++i;
				if (et.passTimings.size() > 1) {
					i += et.passTimings.size();
				}
			}
		}

		static int selectedIdx = -1;

		if (nEffect > 1 || showPasses) {
			ImGui::Spacing();
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
			ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

			if (effectsTotalTime > 0) {
				if (showPasses) {
					if (ImGui::BeginTable("timeline", (int)gpuTimings.passes.size())) {
						for (UINT i = 0; i < gpuTimings.passes.size(); ++i) {
							if (gpuTimings.passes[i] < 1e-5f) {
								continue;
							}

							ImGui::TableSetupColumn(
								std::to_string(i).c_str(),
								ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder,
								gpuTimings.passes[i] / effectsTotalTime
							);
						}

						ImGui::TableNextRow();

						UINT i = 0;
						for (const _EffectTimings& et : effectTimings) {
							for (UINT j = 0, end = (UINT)et.passTimings.size(); j < end; ++j) {
								if (et.passTimings[j] < 1e-5f) {
									continue;
								}

								ImGui::TableNextColumn();

								std::string name;
								if (et.passTimings.size() == 1) {
									name = std::string(GetEffectDisplayName(et.desc));
								} else if (nEffect == 1) {
									name = et.desc->passes[j].desc;
								} else {
									name = StrUtils::Concat(GetEffectDisplayName(et.desc), "/", et.desc->passes[j].desc);
								}

								_DrawTimelineItem(colors[i], _dpiScale, name, et.passTimings[j], effectsTotalTime, selectedIdx == (int)i);

								++i;
							}
						}

						ImGui::EndTable();
					}
				} else {
					if (ImGui::BeginTable("timeline", nEffect)) {
						for (UINT i = 0; i < nEffect; ++i) {
							if (effectTimings[i].totalTime < 1e-5f) {
								continue;
							}

							ImGui::TableSetupColumn(
								std::to_string(i).c_str(),
								ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder,
								effectTimings[i].totalTime / effectsTotalTime
							);
						}

						ImGui::TableNextRow();

						for (UINT i = 0; i < nEffect; ++i) {
							auto& et = effectTimings[i];
							if (et.totalTime < 1e-5f) {
								continue;
							}

							ImGui::TableNextColumn();
							_DrawTimelineItem(colors[i], _dpiScale, GetEffectDisplayName(et.desc), et.totalTime, effectsTotalTime, selectedIdx == (int)i);
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

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x, ImGui::GetStyle().CellPadding.y * 2));
		if (ImGui::BeginTable("timings", 2, ImGuiTableFlags_PadOuterX)) {
			ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder);
			ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder);

			if (nEffect == 1) {
				const auto& et = effectTimings[0];
				int hovered = _DrawEffectTimings(et, true, maxWindowWidth, colors, true);
				if (hovered >= 0) {
					selectedIdx = hovered;
				}
			} else {
				size_t idx = 0;
				for (const auto& et : effectTimings) {
					int idxBegin = (int)idx;

					std::span<const ImColor> colorSpan;
					if (!showPasses || et.passTimings.size() == 1) {
						colorSpan = std::span(colors.begin() + idx, colors.begin() + idx + 1);
						++idx;
					} else {
						colorSpan = std::span(colors.begin() + idx, colors.begin() + idx + et.passTimings.size());
						idx += et.passTimings.size();
					}

					int hovered = _DrawEffectTimings(et, showPasses, maxWindowWidth, colorSpan, false);
					if (hovered >= 0) {
						selectedIdx = idxBegin + hovered;
					}
				}
			}

			ImGui::EndTable();
		}

		if (nEffect > 1) {
			ImGui::Separator();

			if (ImGui::BeginTable("total", 2, ImGuiTableFlags_PadOuterX)) {
				ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder);
				ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted("Total");
				ImGui::TableNextColumn();
				DrawTextWithFont(fmt::format("{:.3f} ms", effectsTotalTime).c_str(), _fontMonoNumbers);

				ImGui::EndTable();
			}
		}
		ImGui::PopStyleVar();
	}

	ImGui::End();
}

void OverlayDrawer::_RetrieveHardwareInfo() noexcept {
	DXGI_ADAPTER_DESC desc{};
	HRESULT hr = MagApp::Get().GetDeviceResources().GetGraphicsAdapter()->GetDesc(&desc);
	_hardwareInfo.gpuName = SUCCEEDED(hr) ? StrUtils::UTF16ToUTF8(desc.Description) : "UNAVAILABLE";

	std::string cpuName = GetCPUName();
	_hardwareInfo.cpuName = !cpuName.empty() ? std::move(cpuName) : "UNAVAILABLE";
}

void OverlayDrawer::_EnableSrcWnd(bool enable) noexcept {
	HWND hwndSrc = MagApp::Get().GetHwndSrc();
	if (!_isSrcMainWnd) {
		// 如果源窗口是 Magpie 主窗口会卡死
		EnableWindow(hwndSrc, TRUE);
	}
	if (enable) {
		SetForegroundWindow(hwndSrc);
	}
}

}
