#pragma once
#include <shellapi.h>

namespace Magpie {

class NotifyIconService {
public:
	static NotifyIconService& Get() noexcept {
		static NotifyIconService instance;
		return instance;
	}

	void Initialize() noexcept;
	void Uninitialize() noexcept;

	// 可从任意线程调用 / safe to call from any thread
	void ShowBalloon(std::wstring title, std::wstring text) noexcept;

	void IsShow(bool value) noexcept;
	bool IsShow() const noexcept {
		// 返回 _shouldShow 而不是 _isShow，对外接口假设总是创建成功
		return _shouldShow;
	}

private:
	// 气球提示可能来自缩放线程，投递到拥有图标的线程处理
	// Balloons can originate on the scaling thread; posted to the icon's thread.
	static constexpr UINT _WM_SHOW_BALLOON = WM_USER + 10;

	static LRESULT _NotifyIconWndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return Get()._NotifyIconWndProc(hWnd, msg, wParam, lParam);
	}
	LRESULT _NotifyIconWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	NOTIFYICONDATA _nid{};
	bool _isShow = false;
	bool _shouldShow = false;
};

}
