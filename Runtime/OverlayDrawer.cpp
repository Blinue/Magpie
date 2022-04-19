#include "pch.h"
#include "OverlayDrawer.h"
#include "App.h"
#include "DeviceResources.h"
#include <imgui.h>
#include <imgui_internal.h>
#include "imgui_impl_magpie.h"
#include "imgui_impl_dx11.h"
#include "Renderer.h"
#include "GPUTimer.h"
#include "CursorManager.h"
#include "Logger.h"
#include "Config.h"
#include "StrUtils.h"
#include "EffectDrawer.h"
#include "FrameSourceBase.h"
#include <bit>	// std::bit_ceil
#include <Wbemidl.h>
#include <comdef.h>

#pragma comment(lib, "wbemuuid.lib")


OverlayDrawer::~OverlayDrawer() {
	if (_handlerID != 0) {
		App::Get().UnregisterWndProcHandler(_handlerID);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplMagpie_Shutdown();
	ImGui::DestroyContext();
}

static std::optional<LRESULT> WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	ImGui_ImplMagpie_WndProcHandler(hWnd, msg, wParam, lParam);
	return std::nullopt;
}

bool OverlayDrawer::Initialize(ID3D11Texture2D* renderTarget) {
	auto& dr = App::Get().GetDeviceResources();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard | ImGuiConfigFlags_NoMouseCursorChange;
	
	_dpiScale = GetDpiForWindow(App::Get().GetHwndHost()) / 96.0f;

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 6;
	style.FrameBorderSize = 1;
	style.WindowMinSize = ImVec2(10, 10);
	style.ScaleAllSizes(_dpiScale);

	std::vector<BYTE> fontData;
	if (!Utils::ReadFile(L".\\assets\\NotoSansSC-Regular.otf", fontData)) {
		Logger::Get().Error("读取字体文件失败");
		return false;
	}

	ImFontConfig config;
	config.FontDataOwnedByAtlas = false;
	_fontSmall = io.Fonts->AddFontFromMemoryTTF(fontData.data(), (int)fontData.size(), std::floor(18 * _dpiScale), &config, io.Fonts->GetGlyphRangesDefault());

	ImVector<ImWchar> fpsRanges;
	ImFontGlyphRangesBuilder builder;
	builder.AddText("0123456789 FPS");
	builder.BuildRanges(&fpsRanges);
	_fontLarge = io.Fonts->AddFontFromMemoryTTF(fontData.data(), (int)fontData.size(), std::floor(36 * _dpiScale), &config, fpsRanges.Data);

	io.Fonts->Build();

	ImGui_ImplMagpie_Init();
	ImGui_ImplDX11_Init(dr.GetD3DDevice(), dr.GetD3DDC());

	dr.GetRenderTargetView(renderTarget, &_rtv);

	_handlerID = App::Get().RegisterWndProcHandler(WndProcHandler);

	_RetrieveHardwareInfo();

	return true;
}

void OverlayDrawer::Draw() {
	bool isShowFPS = App::Get().GetConfig().IsShowFPS();

	if (!_isUIVisiable && !isShowFPS) {
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	CursorManager& cm = App::Get().GetCursorManager();

	bool originWantCaptureMouse = io.WantCaptureMouse;

	ImGui_ImplMagpie_NewFrame();
	ImGui_ImplDX11_NewFrame();
	ImGui::NewFrame();

	if (io.WantCaptureMouse) {
		if (!originWantCaptureMouse) {
			cm.OnCursorHoverOverlay();
		}
	} else {
		if (originWantCaptureMouse) {
			cm.OnCursorLeaveOverlay();
		}
	}

	// 将所有 ImGUI 窗口限制在视口内
	SIZE outputSize = Utils::GetSizeOfRect(App::Get().GetRenderer().GetOutputRect());
	for (ImGuiWindow* window : ImGui::GetCurrentContext()->Windows) {
		if (outputSize.cx > window->Size.x) {
			window->Pos.x = std::clamp(window->Pos.x, 0.0f, outputSize.cx - window->Size.x);
		} else {
			window->Pos.x = 0;
		}

		if (outputSize.cy > window->Size.y) {
			window->Pos.y = std::clamp(window->Pos.y, 0.0f, outputSize.cy - window->Size.y);
		} else {
			window->Pos.y = 0;
		}
	}

	ImGui::PushFont(_fontSmall);

	if (isShowFPS) {
		_DrawFPS();
	}
	
	if (_isUIVisiable) {
		_DrawUI();
	}

	ImGui::PopFont();

	ImGui::Render();

	const RECT& outputRect = App::Get().GetRenderer().GetOutputRect();
	ImGui::GetDrawData()->DisplayPos = ImVec2(float(-outputRect.left), float(-outputRect.top));
	
	auto d3dDC = App::Get().GetDeviceResources().GetD3DDC();
	d3dDC->OMSetRenderTargets(1, &_rtv, NULL);
	
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
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
			ImGui_ImplMagpie_ClearStates();
		}
	}
}

void OverlayDrawer::_DrawFPS() {
	static float fontSize = 0.5f;
	static float opacity = 0.5f;
	static ImVec4 fpsColor(1, 1, 1, 1);

	ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowBgAlpha(opacity);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2 + 6 * fontSize, 2));
	if (!ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoFocusOnAppearing)) {
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}

	ImFont* font = nullptr;
	if (fontSize <= 1.0f / 3.0f) {
		_fontSmall->Scale = fontSize * 1.5f + 0.5f;
		font = _fontSmall;
	} else {
		_fontLarge->Scale = fontSize * 0.75f + 0.25f;
		font = _fontLarge;
	}

	ImGui::PushFont(font);
	ImGui::TextColored(fpsColor, fmt::format("{} FPS", App::Get().GetRenderer().GetGPUTimer().GetFramesPerSecond()).c_str());
	ImGui::PopFont();
	font->Scale = 1.0f;

	if (font == _fontSmall) {
		// 还原字体
		ImGui::PushFont(_fontSmall);
		ImGui::PopFont();
	}

	ImGui::PopStyleVar();

	if (ImGui::BeginPopupContextWindow()) {
		ImGui::PushItemWidth(200);
		ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f);
		ImGui::SliderFloat("Size", &fontSize, 0.0f, 1.0f);
		ImGui::ColorEdit4("Text Color", &fpsColor.x, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_DisplayRGB);
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

void OverlayDrawer::_DrawUI() {
	auto& config = App::Get().GetConfig();
	auto& renderer = App::Get().GetRenderer();
	auto& gpuTimer = renderer.GetGPUTimer();

#ifdef _DEBUG
	ImGui::ShowDemoWindow();
#endif

	static ImVec2 initSize(350 * _dpiScale, 500 * _dpiScale);
	static float initPosX = Utils::GetSizeOfRect(App::Get().GetRenderer().GetOutputRect()).cx - 350 * _dpiScale - 100.0f;

	ImGui::SetNextWindowSize(initSize, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(initPosX, 20), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Profiler", nullptr, ImGuiWindowFlags_NoNav)) {
		ImGui::End();
		return;
	}

	ImGui::Text(StrUtils::Concat("GPU: ", _hardwareInfo.gpuName).c_str());
	ImGui::Text(StrUtils::Concat("CPU: ", _hardwareInfo.cpuName).c_str());

	ImGui::Text(StrUtils::Concat("VSync: ", config.IsDisableVSync() ? "OFF" : "ON").c_str());
	ImGui::Text(StrUtils::Concat("Capture Method: ", App::Get().GetFrameSource().GetName()).c_str());
	ImGui::Spacing();

	static UINT nSamples = 120;

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
			for (UINT i = nSamples - _validFrames; i < nSamples; ++i) {
				totalTime += _frameTimes[i];
				minTime = std::min(_frameTimes[i], minTime);
			}

			// 减少抖动
			const float maxFPS = std::bit_ceil((UINT)std::ceilf((1000 / minTime - 10) / 30)) * 30 * 1.7f;
			
			ImGui::PlotLines("", [](void* data, int idx) {
				float time = (*(std::deque<float>*)data)[idx];
				return time < 1e-6 ? 0 : 1000 / time;
			}, &_frameTimes, (int)_frameTimes.size(), 0, fmt::format("avg: {:.3f} FPS", _validFrames * 1000 / totalTime).c_str(), 0, maxFPS, ImVec2(250 * _dpiScale, 80 * _dpiScale));
		} else {
			float totalTime = 0;
			float maxTime = 0;
			for (UINT i = nSamples - _validFrames; i < nSamples; ++i) {
				totalTime += _frameTimes[i];
				maxTime = std::max(_frameTimes[i], maxTime);
			}

			ImGui::PlotLines("", [](void* data, int idx) {
				return (*(std::deque<float>*)data)[idx];
			}, &_frameTimes, (int)_frameTimes.size(), 0,
				fmt::format("avg: {:.3f} ms", totalTime / _validFrames).c_str(),
				0, maxTime * 1.7f, ImVec2(250 * _dpiScale, 80 * _dpiScale));
		}

		ImGui::Spacing();

		if (ImGui::Button(showFPS ? "Switch to timings" : "Switch to FPS")) {
			showFPS = !showFPS;
		}

		int value = nSamples;
		ImGui::PushItemWidth(200);
		ImGui::SliderInt("Sample size", &value, 60, 180, "%d");
		ImGui::PopItemWidth();
		nSamples = value;
	}

	ImGui::Spacing();
	if (ImGui::CollapsingHeader("GPU Timings", ImGuiTreeNodeFlags_DefaultOpen)) {
		const auto& gpuTimings = gpuTimer.GetGPUTimings();

		UINT idx = 0;
		for (UINT i = 0; i < renderer.GetEffectCount(); ++i) {
			const EffectDesc& effectDesc = renderer.GetEffectDesc(i);
			if (effectDesc.passes.size() == 1) {
				ImGui::Text(fmt::format("{} : {:.3f} ms",
					renderer.GetEffectDesc(i).name, gpuTimings.passes[idx]).c_str());
				++idx;
			} else {
				for (UINT j = 0; j < effectDesc.passes.size(); ++j) {
					ImGui::Text(fmt::format("{}/Pass{} : {:.3f} ms", renderer.GetEffectDesc(i).name,
						j + 1, gpuTimings.passes[idx]).c_str());
					++idx;
				}
			}
		}

		ImGui::Text(fmt::format("Overlay : {:.3f} ms", gpuTimings.overlay).c_str());
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
