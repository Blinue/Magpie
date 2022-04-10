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


OverlayDrawer::~OverlayDrawer() {
	if (_handlerID != 0) {
		App::Get().UnregisterWndProcHandler(_handlerID);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplMagpie_Shutdown();
	ImGui::DestroyContext();
}

float GetDpiScale() {
	return GetDpiForWindow(App::Get().GetHwndHost()) / 96.0f;
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
	
	float dpiScale = GetDpiScale();

	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowRounding = 6;
	ImGui::GetStyle().FrameBorderSize = 1;
	ImGui::GetStyle().ScaleAllSizes(dpiScale);

	io.Fonts->AddFontFromFileTTF(".\\assets\\NotoSansSC-Regular.otf", std::floor(20.0f * dpiScale), NULL);

	ImGui_ImplMagpie_Init();
	ImGui_ImplDX11_Init(dr.GetD3DDevice(), dr.GetD3DDC());

	dr.GetRenderTargetView(renderTarget, &_rtv);

	_handlerID = App::Get().RegisterWndProcHandler(WndProcHandler);

	return true;
}

void DrawUI() {
	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNav)) {
		// Early out if the window is collapsed, as an optimization.
		ImGui::End();
		return;
	}
	
	ImGui::Text(fmt::format("FPS: {}", App::Get().GetRenderer().GetGPUTimer().GetFramesPerSecond()).c_str());

	ImGui::End();
}

void OverlayDrawer::Draw() {
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

	if (cm.IsCursorCapturedOnOverlay()) {
		// 将 UI 窗口限制在主窗口内
		ImGuiWindow* window = ImGui::GetCurrentContext()->HoveredWindow;
		SIZE outputSize = Utils::GetSizeOfRect(App::Get().GetRenderer().GetOutputRect());
		window->Pos.x = std::clamp(window->Pos.x, 0.0f, outputSize.cx - window->Size.x);
		window->Pos.y = std::clamp(window->Pos.y, 0.0f, outputSize.cy - window->Size.y);
	}
	
	DrawUI();

	ImGui::Render();

	const RECT& outputRect = App::Get().GetRenderer().GetOutputRect();
	ImGui::GetDrawData()->DisplayPos = ImVec2(float(-outputRect.left), float(-outputRect.top));
	
	auto d3dDC = App::Get().GetDeviceResources().GetD3DDC();
	d3dDC->OMSetRenderTargets(1, &_rtv, NULL);
	
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
