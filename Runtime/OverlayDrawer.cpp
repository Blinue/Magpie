#include "pch.h"
#include "OverlayDrawer.h"
#include "App.h"
#include "DeviceResources.h"
#include <imgui.h>
#include "ImGuiImpl.h"
#include "Renderer.h"
#include "GPUTimer.h"
#include "Logger.h"
#include "Config.h"
#include "StrUtils.h"
#include "FrameSourceBase.h"
#include <bit>	// std::bit_ceil
#include <Wbemidl.h>
#include <comdef.h>
#include <random>

#pragma comment(lib, "wbemuuid.lib")


OverlayDrawer::OverlayDrawer() {}

OverlayDrawer::~OverlayDrawer() {
	if (App::Get().GetConfig().Is3DMode() && IsUIVisiable()) {
		HWND hwndSrc = App::Get().GetHwndSrc();
		EnableWindow(hwndSrc, TRUE);
		SetForegroundWindow(hwndSrc);
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
	Renderer& renderer = App::Get().GetRenderer();
	UINT nEffect = renderer.GetEffectCount();

	UINT result = 0;
	for (UINT i = 0; i < nEffect; ++i) {
		result ^= (UINT)std::hash<std::string>()(renderer.GetEffectDesc(i).name);
	}
	return result;
}

static std::vector<UINT> GenerateTimelineColors() {
	Renderer& renderer = App::Get().GetRenderer();

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
	std::vector<UINT> result;

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
				std::vector<UINT> effectColors(nColors, 0);
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

bool OverlayDrawer::Initialize() {
	_imguiImpl.reset(new ImGuiImpl());
	if (!_imguiImpl->Initialize()) {
		Logger::Get().Error("初始化 ImGuiImpl 失败");
		return false;
	}

	ImGuiIO& io = ImGui::GetIO();
	
	_dpiScale = GetDpiForWindow(App::Get().GetHwndHost()) / 96.0f;

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 6;
	style.FrameBorderSize = 1;
	style.FrameRounding = 2;
	style.WindowMinSize = ImVec2(10, 10);
	style.ScaleAllSizes(_dpiScale);

	static std::vector<BYTE> fontData;
	if (fontData.empty()) {
		if (!Utils::ReadFile(L".\\assets\\NotoSansSC-Regular.otf", fontData)) {
			Logger::Get().Error("读取字体文件失败");
			return false;
		}
	}

	ImFontConfig config;
	config.FontDataOwnedByAtlas = false;

	ImVector<ImWchar> uiRanges;
	ImFontGlyphRangesBuilder builder;
	builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
	builder.AddText("■");
	builder.BuildRanges(&uiRanges);

	_fontUI = io.Fonts->AddFontFromMemoryTTF(fontData.data(), (int)fontData.size(), std::floor(18 * _dpiScale), &config, uiRanges.Data);

	ImVector<ImWchar> fpsRanges;
	builder.Clear();
	builder.AddText("0123456789 FPS");
	builder.BuildRanges(&fpsRanges);
	// FPS 的字体尺寸不跟随系统缩放
	_fontFPS = io.Fonts->AddFontFromMemoryTTF(fontData.data(), (int)fontData.size(), 32, &config, fpsRanges.Data);

	io.Fonts->Build();

	_RetrieveHardwareInfo();
	_timelineColors = GenerateTimelineColors();

	return true;
}

void OverlayDrawer::Draw() {
	bool isShowFPS = App::Get().GetConfig().IsShowFPS();

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

void OverlayDrawer::SetUIVisibility(bool value) {
	if (_isUIVisiable == value) {
		return;
	}
	_isUIVisiable = value;
	
	if (value) {
		if (App::Get().GetConfig().Is3DMode()) {
			// 使全屏窗口不透明且可以接收焦点
			HWND hwndHost = App::Get().GetHwndHost();
			INT_PTR style = GetWindowLongPtr(hwndHost, GWL_EXSTYLE);
			SetWindowLongPtr(hwndHost, GWL_EXSTYLE, style & ~(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE));
			Utils::SetForegroundWindow(hwndHost);
			
			// 使源窗口无法接收用户输入
			EnableWindow(App::Get().GetHwndSrc(), FALSE);

			ImGui::GetIO().MouseDrawCursor = true;
		}

		Logger::Get().Info("已开启覆盖层");
	} else {
		_validFrames = 0;
		std::fill(_frameTimes.begin(), _frameTimes.end(), 0.0f);

		if (!App::Get().GetConfig().IsShowFPS()) {
			_imguiImpl->ClearStates();
		}

		if (App::Get().GetConfig().Is3DMode()) {
			// 还原全屏窗口样式
			HWND hwndHost = App::Get().GetHwndHost();
			INT_PTR style = GetWindowLongPtr(hwndHost, GWL_EXSTYLE);
			SetWindowLongPtr(hwndHost, GWL_EXSTYLE, style | (WS_EX_TRANSPARENT | WS_EX_NOACTIVATE));
			
			// 重新激活源窗口
			HWND hwndSrc = App::Get().GetHwndSrc();
			EnableWindow(hwndSrc, TRUE);
			Utils::SetForegroundWindow(hwndSrc);

			ImGui::GetIO().MouseDrawCursor = false;
		}

		Logger::Get().Info("已关闭覆盖层");
	}
}

void OverlayDrawer::_DrawFPS() {
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

	std::string fps = fmt::format("{} FPS", App::Get().GetRenderer().GetGPUTimer().GetFramesPerSecond());
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
		ImGui::PushItemWidth(200);
		ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f);
		ImGui::Separator();
		if (ImGui::MenuItem(isLocked ? "Unlock" : "Lock", nullptr, nullptr)) {
			isLocked = !isLocked;
		}
		ImGui::PopItemWidth();
		ImGui::EndPopup();
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

// 只在 x86 和 x64 可用
static std::string GetCPUNameViaCPUID() {
	int nIDs = 0;
	int nExIDs = 0;

	char strCPUName[0x40] = { };

	std::array<int, 4> cpuInfo{};
	std::vector<std::array<int, 4>> extData;

	__cpuid(cpuInfo.data(), 0);

	// Calling __cpuid with 0x80000000 as the function_id argument
	// gets the number of the highest valid extended ID.
	__cpuid(cpuInfo.data(), 0x80000000);

	nExIDs = cpuInfo[0];
	for (int i = 0x80000000; i <= nExIDs; ++i) {
		__cpuidex(cpuInfo.data(), i, 0);
		extData.push_back(cpuInfo);
	}

	// Interpret CPU strCPUName string if reported
	if (nExIDs >= 0x80000004) {
		memcpy(strCPUName, extData[2].data(), sizeof(cpuInfo));
		memcpy(strCPUName + 16, extData[3].data(), sizeof(cpuInfo));
		memcpy(strCPUName + 32, extData[4].data(), sizeof(cpuInfo));
	}

	return StrUtils::Trim(strCPUName);
}

// 非常慢，需要大约 18 ms
static std::string GetCPUNameViaWMI() {
	winrt::com_ptr<IWbemLocator> wbemLocator;
	winrt::com_ptr<IWbemServices> wbemServices;
	winrt::com_ptr<IEnumWbemClassObject> enumWbemClassObject;
	winrt::com_ptr<IWbemClassObject> wbemClassObject;

	HRESULT hr = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&wbemLocator)
	);
	if (FAILED(hr)) {
		return "";
	}

	hr = wbemLocator->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"),
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
		_bstr_t(L"WQL"),
		_bstr_t(L"SELECT NAME FROM Win32_Processor"),
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
	hr = wbemClassObject->Get(_bstr_t(L"Name"), 0, &value, 0, 0);
	if (hr != WBEM_S_NO_ERROR || value.vt != VT_BSTR) {
		return "";
	}

	return StrUtils::Trim(_com_util::ConvertBSTRToString(value.bstrVal));
}

static std::string GetCPUName() {
	std::string result;
	
#ifdef _M_X64
	result = GetCPUNameViaCPUID();
	if (!result.empty()) {
		return result;
	}
#endif // _M_X64

	return GetCPUNameViaWMI();
}

struct EffectTimings {
	const EffectDesc* desc = nullptr;
	std::span<const float> passTimings;
	float totalTime = 0.0f;
};

// 返回鼠标悬停的项的序号，未悬停于任何项返回 -1
static int DrawEffectTimings(const EffectTimings& et, float totalTime, bool showPasses, float maxWindowWidth, std::span<const ImColor> colors, bool singleEffect) {
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
	
	ImGui::TextUnformatted(et.desc->name.c_str());

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
		ImGui::TextUnformatted(fmt::format("{:.3f} ms", et.totalTime).c_str());

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
				ImGui::TextUnformatted(time.c_str());
			}
		}
	} else {
		if (et.totalTime < 10) {
			ImGui::Dummy(ImVec2(rightAlignSpace, 0));
			ImGui::SameLine(0, 0);
		}
		ImGui::TextUnformatted(fmt::format("{:.3f} ms", et.totalTime).c_str());
	}

	return result;
}

static void DrawTimelineItem(ImU32 color, float dpiScale, const std::string& name,
	float time, float effectsTotalTime, bool selected = false) {
	ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, color);
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, color);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, color);
	ImGui::PushStyleColor(ImGuiCol_Header, color);
	ImGui::Selectable("", selected);
	ImGui::PopStyleColor(3);

	if (ImGui::IsItemHovered() || ImGui::IsItemClicked()) {
		std::string content = fmt::format("{}\n{:.3f} ms\n{}%", name, time, std::lroundf(time / effectsTotalTime * 100));
		ImGuiImpl::Tooltip(content.c_str(), 500 * dpiScale);
	}

	// 空间足够时显示文字
	std::string text = selected ? fmt::format("{}%", std::lroundf(time / effectsTotalTime * 100)) : name;
	float textWidth = ImGui::CalcTextSize(text.c_str()).x;
	float itemWidth = ImGui::GetItemRectSize().x;
	float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
	if (itemWidth - (selected ? 0 : itemSpacing - 2) > textWidth) {
		ImGui::SameLine(0, 0);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (itemWidth - textWidth - itemSpacing) / 2);
		ImGui::TextUnformatted(text.c_str());
	}
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

	ImGuiImpl::Tooltip(fmt::format("{:.3f}", v0).c_str());
}

void OverlayDrawer::_DrawUI() {
	auto& config = App::Get().GetConfig();
	auto& renderer = App::Get().GetRenderer();
	auto& gpuTimer = renderer.GetGPUTimer();

#ifdef _DEBUG
	ImGui::ShowDemoWindow();
#endif

	const float maxWindowWidth = 400 * _dpiScale;
	ImGui::SetNextWindowSizeConstraints(ImVec2(), ImVec2(maxWindowWidth, 500 * _dpiScale));

	static float initPosX = Utils::GetSizeOfRect(App::Get().GetRenderer().GetOutputRect()).cx - maxWindowWidth;
	ImGui::SetNextWindowPos(ImVec2(initPosX, 20), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Profiler", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::End();
		return;
	}
	
	// 始终为滚动条预留空间
	ImGui::PushTextWrapPos(maxWindowWidth - ImGui::GetStyle().WindowPadding.x - ImGui::GetStyle().ScrollbarSize);
	ImGui::TextUnformatted(StrUtils::Concat("GPU: ", _hardwareInfo.gpuName).c_str());
	ImGui::TextUnformatted(StrUtils::Concat("CPU: ", _hardwareInfo.cpuName).c_str());
	ImGui::TextUnformatted(StrUtils::Concat("VSync: ", config.IsDisableVSync() ? "OFF" : "ON").c_str());
	ImGui::TextUnformatted(StrUtils::Concat("Capture Method: ", App::Get().GetFrameSource().GetName()).c_str());
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
			}, &_frameTimes, (int)_frameTimes.size(), 0, fmt::format("avg: {:.3f} FPS", _validFrames * 1000 / totalTime).c_str(), 0, maxFPS, ImVec2(250 * _dpiScale, 80 * _dpiScale));
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
				fmt::format("avg: {:.3f} ms", totalTime / _validFrames).c_str(),
				0, maxTime2 * 1.7f, ImVec2(250 * _dpiScale, 80 * _dpiScale));
		}
		/*
		ImGui::Spacing();

		if (ImGui::Button(showFPS ? "Switch to timings" : "Switch to FPS")) {
			showFPS = !showFPS;
		}*/
	}
	
	ImGui::Spacing();
	if (ImGui::CollapsingHeader("Timings", ImGuiTreeNodeFlags_DefaultOpen)) {
		const auto& gpuTimings = gpuTimer.GetGPUTimings();
		const UINT nEffect = renderer.GetEffectCount();

		std::vector<EffectTimings> effectTimings(nEffect);

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

		std::vector<ImColor> colors;
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
							ImGui::TableSetupColumn(
								std::to_string(i).c_str(),
								ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder,
								std::max(1e-5f, gpuTimings.passes[i] / effectsTotalTime)
							);
						}

						ImGui::TableNextRow();

						UINT i = 0;
						for (const EffectTimings& et : effectTimings) {
							for (UINT j = 0, end = (UINT)et.passTimings.size(); j < end; ++j) {
								ImGui::TableNextColumn();

								std::string name;
								if (et.passTimings.size() == 1) {
									name = et.desc->name;
								} else if (nEffect == 1) {
									name = et.desc->passes[j].desc;
								} else {
									name = StrUtils::Concat(et.desc->name, "/", et.desc->passes[j].desc);
								}

								DrawTimelineItem(colors[i], _dpiScale, name, et.passTimings[j], effectsTotalTime, selectedIdx == i);

								++i;
							}
						}

						ImGui::EndTable();
					}
				} else {
					if (ImGui::BeginTable("timeline", nEffect)) {
						for (UINT i = 0; i < nEffect; ++i) {
							ImGui::TableSetupColumn(
								std::to_string(i).c_str(),
								ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder,
								std::max(1e-5f, effectTimings[i].totalTime / effectsTotalTime)
							);
						}

						ImGui::TableNextRow();

						for (UINT i = 0; i < nEffect; ++i) {
							ImGui::TableNextColumn();
							auto& et = effectTimings[i];

							DrawTimelineItem(colors[i], _dpiScale, et.desc->name, et.totalTime, effectsTotalTime, selectedIdx == i);
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
				int hovered = DrawEffectTimings(et, effectsTotalTime, true, maxWindowWidth, colors, true);
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
					
					int hovered = DrawEffectTimings(et, effectsTotalTime, showPasses, maxWindowWidth, colorSpan, false);
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
				ImGui::TextUnformatted(fmt::format("{:.3f} ms", effectsTotalTime).c_str());
				
				ImGui::EndTable();
			}
		}
		ImGui::PopStyleVar();
	}

	ImGui::End();
}

void OverlayDrawer::_RetrieveHardwareInfo() {
	DXGI_ADAPTER_DESC desc{};
	HRESULT hr = App::Get().GetDeviceResources().GetGraphicsAdapter()->GetDesc(&desc);
	_hardwareInfo.gpuName = SUCCEEDED(hr) ? StrUtils::UTF16ToUTF8(desc.Description) : "UNAVAILABLE";

	std::string cpuName = GetCPUName();
	_hardwareInfo.cpuName = !cpuName.empty() ? std::move(cpuName) : "UNAVAILABLE";
}
