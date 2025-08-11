#pragma once

template <typename T>
class WindowBaseT {
public:
	WindowBaseT() noexcept = default;
	WindowBaseT(const WindowBaseT&) = delete;
	WindowBaseT(WindowBaseT&&) noexcept = default;

	HWND Handle() const noexcept {
		return _hWnd;
	}

	operator bool() const noexcept {
		return _hWnd;
	}

	void Destroy() const noexcept {
		if (_hWnd) {
			DestroyWindow(_hWnd);
		}
	}

protected:
	// 确保无法通过基类指针删除这个对象
	~WindowBaseT() noexcept {
		Destroy();
	}

	static LRESULT CALLBACK _WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		if (msg == WM_NCCREATE) {
			WindowBaseT* that = (WindowBaseT*)(((CREATESTRUCT*)lParam)->lpCreateParams);
			assert(that && !that->_hWnd);
			that->_hWnd = hWnd;
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)that);
		} else if (T* that = (T*)GetWindowLongPtr(hWnd, GWLP_USERDATA)) {
			return that->_MessageHandler(msg, wParam, lParam);
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
		switch (msg) {
		case WM_CREATE:
		{
			_dpiScale = GetDpiForWindow(Handle()) / double(USER_DEFAULT_SCREEN_DPI);
			return 0;
		}
		case WM_DPICHANGED:
		{
			_dpiScale = HIWORD(wParam) / double(USER_DEFAULT_SCREEN_DPI);

			RECT* newRect = (RECT*)lParam;
			SetWindowPos(
				Handle(),
				NULL,
				newRect->left,
				newRect->top,
				newRect->right - newRect->left,
				newRect->bottom - newRect->top,
				SWP_NOZORDER | SWP_NOACTIVATE
			);

			return 0;
		}
		case WM_DESTROY:
		{
			_hWnd = NULL;
			return 0;
		}
		}

		return DefWindowProc(_hWnd, msg, wParam, lParam);
	}

	double DpiScale() const noexcept {
		return _dpiScale;
	}

private:
	HWND _hWnd = NULL;
	double _dpiScale = 1.0;
};
