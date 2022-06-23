// main.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "XamlApp.h"


int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ LPWSTR /*lpCmdLine*/,
	_In_ int /*nCmdShow*/
) {
	auto& app = XamlApp::Get();
	if (!app.Initialize(hInstance)) {
		return -1;
	}

	return app.Run();
}
