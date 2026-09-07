#pragma once
#include "Singleton.h"
#include <shellapi.h>

namespace Magpie {

class NotifyIconService : public Singleton<NotifyIconService> {
	friend Singleton<NotifyIconService>;

public:
	void Initialize() noexcept;
	void Uninitialize() noexcept;

	void IsShow(bool value) noexcept;
	bool IsShow() const noexcept {
		// 返回 _shouldShow 而不是 _isShow，对外接口假设总是创建成功
		return _shouldShow;
	}

private:
	NotifyIconService() = default;

	static LRESULT _NotifyIconWndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		return Get()._NotifyIconWndProc(hWnd, msg, wParam, lParam);
	}
	LRESULT _NotifyIconWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	NOTIFYICONDATA _nid{};
	bool _isShow = false;
	bool _shouldShow = false;
};

}
