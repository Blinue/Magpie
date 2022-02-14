#include "pch.h"
#include "UIDrawer.h"
#include "App.h"
#include "DeviceResources.h"
#include <imgui.h>
#include "imgui_impl_magpie.h"
#include "imgui_impl_dx11.h"
#include "Renderer.h"
#include "CursorDrawer.h"



UIDrawer::~UIDrawer() {
	if (_handlerID != 0) {
		App::GetInstance().UnregisterWndProcHandler(_handlerID);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplMagpie_Shutdown();
	ImGui::DestroyContext();
}

bool UIDrawer::Initialize(ID3D11Texture2D* renderTarget) {
	auto& dr = App::GetInstance().GetDeviceResources();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard | ImGuiConfigFlags_NoMouseCursorChange;

	ImGui::StyleColorsDark();

	ImGui_ImplMagpie_Init(App::GetInstance().GetHwndHost());
	ImGui_ImplDX11_Init(dr.GetD3DDevice(), dr.GetD3DDC());

	dr.GetRenderTargetView(renderTarget, &_rtv);

	_handlerID = App::GetInstance().RegisterWndProcHandler(_WndProcHandler);

	return true;
}

void UIDrawer::Draw() {
	auto& io = ImGui::GetIO();

	ImGui_ImplMagpie_NewFrame();
	ImGui_ImplDX11_NewFrame();
	ImGui::NewFrame();

	if (io.WantCaptureMouse) {
		if (!_cursorOnUI) {
			HWND hwndHost = App::GetInstance().GetHwndHost();
			LONG_PTR style = GetWindowLongPtr(hwndHost, GWL_EXSTYLE);
			SetWindowLongPtr(hwndHost, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);

			_cursorOnUI = true;
		}
	} else {
		if (_cursorOnUI) {
			HWND hwndHost = App::GetInstance().GetHwndHost();
			LONG_PTR style = GetWindowLongPtr(hwndHost, GWL_EXSTYLE);
			SetWindowLongPtr(hwndHost, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);

			_cursorOnUI = false;
		}
	}

	bool show = true;
	ImGui::ShowDemoWindow(&show);

	ImGui::Render();

	auto& dr = App::GetInstance().GetDeviceResources();

	App::GetInstance().GetDeviceResources().GetD3DDC()->OMSetRenderTargets(1, &_rtv, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

bool UIDrawer::_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	ImGui_ImplMagpie_WndProcHandler(hWnd, msg, wParam, lParam);
	return false;
}
