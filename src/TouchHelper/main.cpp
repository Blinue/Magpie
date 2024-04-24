#include "pch.h"
#include <magnification.h>

int APIENTRY wWinMain(
	_In_ HINSTANCE /*hInstance*/,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ LPWSTR /*lpCmdLine*/,
	_In_ int /*nCmdShow*/
) {
	if (!MagInitialize()) {
		MessageBox(NULL, L"失败", L"消息", MB_OK);
		return 1;
	}

	RECT srcRect{ 0,0,100,100 };
	RECT destRect{ 10,10,210,210 };
	if (!MagSetInputTransform(TRUE, &srcRect, &destRect)) {
		MessageBox(NULL, L"失败", L"消息", MB_OK);
		return 1;
	}
	
	return 0;
}
