// Magpie.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "XamlApp.h"


int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow
) {
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	auto& app = XamlApp::Get();
	app.Initialize(hInstance, L"Magpie_XamlHost", L"Magpie");

	winrt::Magpie::App::App hostApp = winrt::Magpie::App::App{};
	app.Show(winrt::Magpie::App::MainPage());

	return app.Run();
}
