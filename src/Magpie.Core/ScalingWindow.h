#pragma once
#include "WindowBase.h"
#include "ScalingOptions.h"

namespace Magpie::Core {

class ScalingWindow : public WindowBase<ScalingWindow> {
	friend class base_type;

public:
	static ScalingWindow& Get() noexcept {
		static ScalingWindow instance;
		return instance;
	}

	bool Create(HINSTANCE hInstance, HWND hwndSrc, ScalingOptions&& options) noexcept;

	void Render() noexcept;

	const RECT& WndRect() const noexcept {
		return _wndRect;
	}

	const ScalingOptions& Options() const noexcept {
		return _options;
	}

	HWND HwndSrc() const noexcept {
		return _hwndSrc;
	}

	class Renderer& Renderer() noexcept {
		return *_renderer;
	}

	class CursorManager& CursorManager() noexcept {
		return *_cursorManager;
	}

protected:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	ScalingWindow() noexcept;
	~ScalingWindow() noexcept;

	int _CheckSrcState() const noexcept;

	bool _CheckForeground(HWND hwndForeground) const noexcept;

	RECT _wndRect;

	ScalingOptions _options;
	std::unique_ptr<class Renderer> _renderer;
	std::unique_ptr<class CursorManager> _cursorManager;

	HWND _hwndSrc = NULL;
	RECT _srcWndRect{};
};

}
