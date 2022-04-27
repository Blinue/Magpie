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

#pragma comment(lib, "wbemuuid.lib")


OverlayDrawer::OverlayDrawer() {}

OverlayDrawer::~OverlayDrawer() {}

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

	std::vector<BYTE> fontData;
	if (!Utils::ReadFile(L".\\assets\\NotoSansSC-Regular.otf", fontData)) {
		Logger::Get().Error("读取字体文件失败");
		return false;
	}

	ImFontConfig config;
	config.FontDataOwnedByAtlas = false;
	_fontUI = io.Fonts->AddFontFromMemoryTTF(fontData.data(), (int)fontData.size(), std::floor(18 * _dpiScale), &config, io.Fonts->GetGlyphRangesDefault());

	ImVector<ImWchar> fpsRanges;
	ImFontGlyphRangesBuilder builder;
	builder.AddText("0123456789 FPS");
	builder.BuildRanges(&fpsRanges);
	// FPS 的字体尺寸不跟随系统缩放
	_fontFPS = io.Fonts->AddFontFromMemoryTTF(fontData.data(), (int)fontData.size(), 32, &config, fpsRanges.Data);

	io.Fonts->Build();

	_RetrieveHardwareInfo();

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

	if (!value) {
		_validFrames = 0;
		std::fill(_frameTimes.begin(), _frameTimes.end(), 0.0f);

		if (!App::Get().GetConfig().IsShowFPS()) {
			_imguiImpl->ClearStates();
		}

		Logger::Get().Info("已关闭覆盖层");
	} else {
		Logger::Get().Info("已开启覆盖层");
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
		ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 0.8f), fps.c_str());

		ImGui::SetCursorPos(cursorPos);
		ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 0.6f), fps.c_str());

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

static void DrawTime(float time, bool gray = false) {
	std::string text = fmt::format("{:.3f} ms", time);

	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(text.c_str()).x - 3);
	if (gray) {
		ImGui::TextColored(ImVec4(1, 1, 1, 0.5f), text.c_str());
	} else {
		ImGui::TextUnformatted(text.c_str());
	}
}

static void DrawEffectTimings(const EffectTimings& et, float totalTime, bool showPasses) {
	if (et.passTimings.size() > 1) {

		ImGui::TextUnformatted(et.desc->name.c_str());

		if (totalTime != 0.0f) {
			DrawTime(et.totalTime, showPasses);
		}

		if (showPasses) {
			ImGui::Indent(20);

			for (UINT j = 0; j < et.passTimings.size(); ++j) {
				std::string desc = et.desc->passes[j].desc;
				if (desc.empty()) {
					desc = fmt::format("Pass {}", j + 1);
				}

				ImGui::TextUnformatted(desc.c_str());

				if (totalTime != 0.0f) {
					DrawTime(et.passTimings[j]);
				}
			}

			ImGui::Unindent(20);
		}
	} else {
		ImGui::TextUnformatted(et.desc->name.c_str());
		if (totalTime != 0.0f) {
			DrawTime(et.totalTime);
		}
	}
	
}

void OverlayDrawer::_DrawUI() {
	auto& config = App::Get().GetConfig();
	auto& renderer = App::Get().GetRenderer();
	auto& gpuTimer = renderer.GetGPUTimer();

#ifdef _DEBUG
	ImGui::ShowDemoWindow();
#endif

	const float maxWindowWidth = 350 * _dpiScale;
	ImGui::SetNextWindowSizeConstraints(ImVec2(), ImVec2(maxWindowWidth, 500 * _dpiScale));

	static float initPosX = Utils::GetSizeOfRect(App::Get().GetRenderer().GetOutputRect()).cx - maxWindowWidth;
	ImGui::SetNextWindowPos(ImVec2(initPosX, 20), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Profiler", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {
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
			
			ImGui::PlotLines("", [](void* data, int idx) {
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
			ImGui::PlotLines("", [](void* data, int idx) {
				return (*(std::deque<float>*)data)[idx];
			}, &_frameTimes, (int)_frameTimes.size(), 0,
				fmt::format("avg: {:.3f} ms", totalTime / _validFrames).c_str(),
				0, maxTime2 * 1.7f, ImVec2(250 * _dpiScale, 80 * _dpiScale));
		}

		ImGui::Spacing();

		if (ImGui::Button(showFPS ? "Switch to timings" : "Switch to FPS")) {
			showFPS = !showFPS;
		}
	}

	ImGui::Spacing();
	if (ImGui::CollapsingHeader("Timings", ImGuiTreeNodeFlags_DefaultOpen)) {
		const auto& gpuTimings = gpuTimer.GetGPUTimings();
		const UINT nEffect = renderer.GetEffectCount();

		static std::vector<EffectTimings> effectTimings;
		effectTimings.clear();
		effectTimings.resize(nEffect);

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

		if (nEffect == 1) {
			const auto& et = effectTimings[0];
			DrawEffectTimings(et, et.totalTime, et.passTimings.size() > 1);
		} else {
			static bool showPasses = false;
			for (const auto& et : effectTimings) {
				// 某个效果有多个通道，显示切换按钮
				if (et.passTimings.size() > 1) {
					if (ImGui::Button(showPasses ? "Switch to effects" : "Switch to passes")) {
						showPasses = !showPasses;
					}
					break;
				}
			}

			float effectsTotalTime = 0.0f;
			for (const auto& et : effectTimings) {
				effectsTotalTime += et.totalTime;
			}

			for (const auto& et : effectTimings) {
				DrawEffectTimings(et, effectsTotalTime, showPasses);
			}

			ImGui::Separator();

			ImGui::TextUnformatted("Total");
			DrawTime(effectsTotalTime);
		}
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
