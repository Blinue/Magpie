#include "pch.h"
#include "CursorWindow.h"

// https://learn.microsoft.com/en-us/windows/win32/menurc/about-cursors
static const std::pair<LPCWSTR, const wchar_t*> STANDARD_CURSORS[] = {
	{IDC_ARROW, L"Arrow"},
	{IDC_IBEAM, L"IBeam"},
	{IDC_WAIT, L"Wait"},
	{IDC_CROSS, L"Crosshair"},
	{IDC_UPARROW, L"UpArrow"},
	{IDC_SIZENWSE, L"SizeNWSE"},
	{IDC_SIZENESW, L"SizeNESW"},
	{IDC_SIZEWE, L"SizeWE"},
	{IDC_SIZENS, L"SizeNS"},
	{IDC_SIZEALL, L"SizeAll"},
	{IDC_NO, L"No"},
	{IDC_HAND, L"Hand"},
	{IDC_APPSTARTING, L"AppStarting"},
	{IDC_HELP, L"Help"},
	{IDC_PIN, L"Pin"},
	{IDC_PERSON, L"Person"}
};

static wil::unique_hcursor CreateCursorFromBitmaps(HBITMAP hColorBitmap, HBITMAP hMaskBitmap) noexcept {
	ICONINFO iconInfo = {
		.fIcon = FALSE,
		.xHotspot = 0,
		.yHotspot = 0,
		.hbmMask = hMaskBitmap,
		.hbmColor = hColorBitmap
	};
	return wil::unique_hcursor(CreateIconIndirect(&iconInfo));
}

static wil::unique_hcursor CreateColorCursor() noexcept {
	const uint32_t width = (uint32_t)GetSystemMetrics(SM_CXCURSOR);
	const uint32_t height = (uint32_t)GetSystemMetrics(SM_CYCURSOR);

	// 颜色位图初始化为半透明红色
	std::vector<uint32_t> colorBits(width * height, 0x80FF0000);

	wil::unique_hbitmap hColorBitmap(CreateBitmap(width, height, 1, 32, colorBits.data()));
	// 仍需要遮罩位图，不过既然颜色位图有透明通道，遮罩位图不会被使用，因此无需初始化
	wil::unique_hbitmap hMaskBitmap(CreateBitmap(width, height, 1, 1, nullptr));
	return CreateCursorFromBitmaps(hColorBitmap.get(), hMaskBitmap.get());
}

static wil::unique_hcursor CreateMonochromeCursor() noexcept {
	const uint32_t width = (uint32_t)GetSystemMetrics(SM_CXCURSOR);
	const uint32_t height = (uint32_t)GetSystemMetrics(SM_CYCURSOR);

	// width 必定是 16 的倍数，因此遮罩位图每行都已按 WORD 对齐
	assert(width % 16 == 0);
	const uint32_t widthByteCount = width / 8;
	// 单色光标无颜色位图，遮罩位图高度是光标高度的两倍
	const uint32_t bitmapHeight = height * 2;

	// 分为四个部分：
	// 左上：白色 (AND=0, XOR=1)
	// 右上：黑色 (AND=0, XOR=0)
	// 左下：反色 (AND=1, XOR=1)
	// 右下：透明 (AND=1, XOR=0)
	assert(width % 16 == 0);
	std::vector<uint8_t> maskBits(widthByteCount * bitmapHeight);

	// AND 遮罩下半置 1
	for (uint32_t y = height / 2; y < height; ++y) {
		for (uint32_t x = 0; x < width; ++x) {
			uint32_t byteIdx = y * widthByteCount + x / 8;
			uint32_t bitIdx = 7 - (x % 8);
			maskBits[byteIdx] |= 1 << bitIdx;
		}
	}

	// XOR 遮罩左半置 1
	for (uint32_t y = height; y < bitmapHeight; ++y) {
		for (uint32_t x = 0; x < width / 2; ++x) {
			uint32_t byteIdx = y * widthByteCount + x / 8;
			uint32_t bitIdx = 7 - (x % 8);
			maskBits[byteIdx] |= 1 << bitIdx;
		}
	}

	wil::unique_hbitmap hMaskBitmap(CreateBitmap(width, bitmapHeight, 1, 1, maskBits.data()));
	return CreateCursorFromBitmaps(NULL, hMaskBitmap.get());
}

static wil::unique_hcursor CreateMaskedColorCursor() noexcept {
	const uint32_t width = (uint32_t)GetSystemMetrics(SM_CXCURSOR);
	const uint32_t height = (uint32_t)GetSystemMetrics(SM_CYCURSOR);

	// width 必定是 16 的倍数，因此遮罩位图每行都已按 WORD 对齐
	assert(width % 16 == 0);
	const uint32_t widthByteCount = width / 8;
	
	// 分为两个部分：
	// 左半：红色 (COLOR=0x00FF0000, MASK=0)
	// 右半：和红色 XOR (COLOR=0x00FF0000, MASK=1)
	
	// 颜色位图初始化为红色。注意透明通道为 0，否则会被识别为彩色光标
	std::vector<uint32_t> colorBits(width * height, 0x00FF0000);
	std::vector<uint8_t> maskBits(widthByteCount * height, 0);
	
	// XOR 遮罩右半置 1
	for (uint32_t y = 0; y < height; ++y) {
		for (uint32_t x = width / 2; x < width; ++x) {
			uint32_t byteIdx = y * widthByteCount + x / 8;
			uint32_t bitIdx = 7 - (x % 8);
			maskBits[byteIdx] |= 1 << bitIdx;
		}
	}

	wil::unique_hbitmap hColorBitmap(CreateBitmap(width, height, 1, 32, colorBits.data()));
	wil::unique_hbitmap hMaskBitmap(CreateBitmap(width, height, 1, 1, maskBits.data()));
	return CreateCursorFromBitmaps(hColorBitmap.get(), hMaskBitmap.get());
}

bool CursorWindow::Create() noexcept {
	static const wchar_t* WINDOW_NAME = L"CursorWindow";

	WNDCLASSEXW wcex = {
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = _WndProc,
		.hInstance = wil::GetModuleInstanceHandle(),
		.lpszClassName = WINDOW_NAME
	};
	if (!RegisterClassEx(&wcex)) {
		return false;
	}

	CreateWindow(
		WINDOW_NAME,
		WINDOW_NAME,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		wil::GetModuleInstanceHandle(),
		this
	);
	if (!Handle()) {
		return false;
	}

	const double dpiScale = _DpiScale();
	SetWindowPos(Handle(), NULL, 0, 0,
		std::lround(500 * dpiScale), std::lround(400 * dpiScale),
		SWP_NOMOVE | SWP_SHOWWINDOW);
	return true;
}

LRESULT CursorWindow::_MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	switch (msg) {
	case WM_CREATE:
	{
		const LRESULT ret = base_type::_MessageHandler(msg, wParam, lParam);

		const HMODULE hInst = wil::GetModuleInstanceHandle();
		_hwndBtn1 = CreateWindow(L"BUTTON", nullptr,
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, Handle(), (HMENU)1, hInst, 0);
		_hwndBtn2 = CreateWindow(L"BUTTON", L"彩色光标",
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, Handle(), (HMENU)2, hInst, 0);
		_hwndBtn3 = CreateWindow(L"BUTTON", L"单色光标",
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, Handle(), (HMENU)3, hInst, 0);
		_hwndBtn4 = CreateWindow(L"BUTTON", L"彩色掩码光标",
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, Handle(), (HMENU)4, hInst, 0);
		_UpdateButtonPos();

		SendMessage(_hwndBtn1, WM_SETFONT, (WPARAM)_UIFont(), TRUE);
		SendMessage(_hwndBtn2, WM_SETFONT, (WPARAM)_UIFont(), TRUE);
		SendMessage(_hwndBtn3, WM_SETFONT, (WPARAM)_UIFont(), TRUE);
		SendMessage(_hwndBtn4, WM_SETFONT, (WPARAM)_UIFont(), TRUE);

		_UpdateStandardCursor(true);

		return ret;
	}
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == BN_CLICKED) {
			const WORD btnId = LOWORD(wParam);

			_UpdateStandardCursor(btnId == 1);

			if (btnId == 2) {
				_hCursor = CreateColorCursor();
			} else if (btnId == 3) {
				_hCursor = CreateMonochromeCursor();
			} else if (btnId == 4) {
				_hCursor = CreateMaskedColorCursor();
			}

			return 0;
		}
		break;
	}
	case WM_SIZE:
	{
		_UpdateButtonPos();
		break;
	}
	case WM_SETCURSOR:
	{
		if (LOWORD(lParam) == HTCLIENT) {
			SetCursor(_hCursor.get());
			return TRUE;
		}
		break;
	}
	case WM_ERASEBKGND:
	{
		// 绘制渐变背景
		wil::unique_hdc_paint hdc = wil::BeginPaint(Handle());

		RECT rc;
		GetClientRect(Handle(), &rc);

		TRIVERTEX vertices[] = {
			{
				.x = rc.left,
				.y = rc.top,
				.Red = 0xE000,
				.Green = 0x6000,
				.Blue = 0xE000
			},
			{
				.x = rc.right,
				.y = rc.bottom,
				.Red = 0x6000,
				.Green = 0xE000,
				.Blue = 0x6000
			}
		};
		GRADIENT_RECT rect = { 0, 1 };
		GradientFill(hdc.get(), vertices, 2, &rect, 1, GRADIENT_FILL_RECT_V);

		return TRUE;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return base_type::_MessageHandler(msg, wParam, lParam);
}

void CursorWindow::_UpdateButtonPos() noexcept {
	RECT clientRect;
	GetClientRect(Handle(), &clientRect);

	const double dpiScale = _DpiScale();
	const SIZE btnSize = { std::lround(180 * dpiScale),std::lround(40 * dpiScale) };
	const LONG halfSpacing = std::lround(4 * dpiScale);

	const LONG btnLeft = ((clientRect.right - clientRect.left) - btnSize.cx) / 2;
	const LONG windowCenterY = (clientRect.bottom - clientRect.top) / 2;

	SetWindowPos(_hwndBtn1, NULL, btnLeft, windowCenterY - 2 * btnSize.cy - halfSpacing * 3,
		btnSize.cx, btnSize.cy, SWP_NOACTIVATE);
	SetWindowPos(_hwndBtn2, NULL, btnLeft, windowCenterY - btnSize.cy - halfSpacing,
		btnSize.cx, btnSize.cy, SWP_NOACTIVATE);
	SetWindowPos(_hwndBtn3, NULL, btnLeft, windowCenterY + halfSpacing,
		btnSize.cx, btnSize.cy, SWP_NOACTIVATE);
	SetWindowPos(_hwndBtn4, NULL, btnLeft, windowCenterY + btnSize.cy + halfSpacing * 3,
		btnSize.cx, btnSize.cy, SWP_NOACTIVATE);
}

void CursorWindow::_UpdateStandardCursor(bool useStandardCursor) noexcept {
	if (useStandardCursor) {
		// 切换标准光标
		if (_curStandardCursorIdx == std::numeric_limits<uint32_t>::max()) {
			_curStandardCursorIdx = 0;
		} else {
			_curStandardCursorIdx = (_curStandardCursorIdx + 1) % (uint32_t)std::size(STANDARD_CURSORS);
		}

		_hCursor.reset(LoadCursor(NULL, STANDARD_CURSORS[_curStandardCursorIdx].first));

		SetWindowText(_hwndBtn1, (L"系统光标 - "s + STANDARD_CURSORS[_curStandardCursorIdx].second).c_str());
	} else if (_curStandardCursorIdx != std::numeric_limits<uint32_t>::max()) {
		_curStandardCursorIdx = std::numeric_limits<uint32_t>::max();
		SetWindowText(_hwndBtn1, L"系统光标");
	}
}
