#pragma once

namespace Magpie {

class TrayIconService {
public:
	static TrayIconService& Get() noexcept {
		static TrayIconService instance;
		return instance;
	}

	void Initialize() noexcept;
	void Uninitialize() noexcept;

	void IsShow(bool value) noexcept;
	bool IsShow() noexcept {
		// 返回 _shouldShow 而不是 _isShow，对外接口假设总是创建成功
		return _shouldShow;
	}

private:
	static LRESULT _TrayIconWndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return Get()._TrayIconWndProc(hWnd, msg, wParam, lParam);
	}
	LRESULT _TrayIconWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	NOTIFYICONDATA _nid{};
	bool _isShow = false;
	bool _shouldShow = false;
	static const UINT _WM_TASKBARCREATED;
};

}
