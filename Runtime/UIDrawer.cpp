#include "pch.h"
#include "UIDrawer.h"
#include "App.h"
#include "DeviceResources.h"
#include <imgui.h>
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx11.h"


UIDrawer::~UIDrawer() {
	if (_handlerID != 0) {
		App::GetInstance().UnregisterWndProcHandler(_handlerID);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

bool UIDrawer::Initialize(ID3D11Texture2D* renderTarget) {
	auto& dr = App::GetInstance().GetDeviceResources();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(App::GetInstance().GetHwndHost());
	ImGui_ImplDX11_Init(dr.GetD3DDevice(), dr.GetD3DDC());

	dr.GetRenderTargetView(renderTarget, &_rtv);

	_handlerID = App::GetInstance().RegisterWndProcHandler(_WndProcHandler);

	return true;
}

void UIDrawer::Draw() {
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX11_NewFrame();
	ImGui::NewFrame();

	bool show = true;
	ImGui::ShowDemoWindow(&show);

	ImGui::Render();

	auto& dr = App::GetInstance().GetDeviceResources();

	App::GetInstance().GetDeviceResources().GetD3DDC()->OMSetRenderTargets(1, &_rtv, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool UIDrawer::_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
	return false;
}
