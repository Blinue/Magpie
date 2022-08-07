// main.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "XamlApp.h"


int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ LPWSTR lpCmdLine,
	_In_ int /*nCmdShow*/
) {
	// 程序结束时也不应调用 uninit_apartment
	// 见 https://kennykerr.ca/2018/03/24/cppwinrt-hosting-the-windows-runtime/
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	auto& app = XamlApp::Get();
	if (!app.Initialize(hInstance, lpCmdLine)) {
		return -1;
	}

	return app.Run();
}
