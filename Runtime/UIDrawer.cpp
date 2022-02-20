#include "pch.h"
#include "UIDrawer.h"
#include "App.h"
#include "DeviceResources.h"
#include <imgui.h>
#include "imgui_impl_magpie.h"
#include "imgui_impl_dx11.h"
#include "Renderer.h"
#include "GPUTimer.h"


UIDrawer::~UIDrawer() {
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

bool UIDrawer::Initialize(ID3D11Texture2D* renderTarget) {
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

	_handlerID = App::Get().RegisterWndProcHandler(_WndProcHandler);

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

void UIDrawer::Draw() {
	ImGui_ImplMagpie_NewFrame();
	ImGui_ImplDX11_NewFrame();
	ImGui::NewFrame();

	if (ImGui::GetIO().WantCaptureMouse) {
		if (!_cursorOnUI) {
			HWND hwndHost = App::Get().GetHwndHost();
			LONG_PTR style = GetWindowLongPtr(hwndHost, GWL_EXSTYLE);
			SetWindowLongPtr(hwndHost, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);

			_cursorOnUI = true;
		}
	} else {
		if (_cursorOnUI) {
			HWND hwndHost = App::Get().GetHwndHost();
			LONG_PTR style = GetWindowLongPtr(hwndHost, GWL_EXSTYLE);
			SetWindowLongPtr(hwndHost, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);

			_cursorOnUI = false;
		}
	}

	bool show = true;
	DrawUI();

	ImGui::Render();

	const RECT& outputRect = App::Get().GetRenderer().GetOutputRect();
	ImGui::GetDrawData()->DisplayPos = ImVec2(float(-outputRect.left), float(-outputRect.top));
	
	auto d3dDC = App::Get().GetDeviceResources().GetD3DDC();
	d3dDC->OMSetRenderTargets(1, &_rtv, NULL);
	
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

bool UIDrawer::_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	ImGui_ImplMagpie_WndProcHandler(hWnd, msg, wParam, lParam);
	return false;
}
