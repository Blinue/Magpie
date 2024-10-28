#pragma once
#include "WindowBase.h"
#include "ScalingOptions.h"
#include "Win32Utils.h"
#include "ScalingError.h"

namespace Magpie::Core {

class CursorManager;

class ScalingWindow : public WindowBase<ScalingWindow> {
	friend class base_type;

public:
	static ScalingWindow& Get() noexcept {
		static ScalingWindow instance;
		return instance;
	}

	ScalingError Create(
		const winrt::DispatcherQueue& dispatcher,
		HWND hwndSrc,
		ScalingOptions&& options
	) noexcept;

	void Render() noexcept;

	void ToggleOverlay() noexcept;

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

	CursorManager& CursorManager() noexcept {
		return *_cursorManager;
	}

	const winrt::DispatcherQueue& Dispatcher() const noexcept {
		return _dispatcher;
	}

	bool IsSrcRepositioning() const noexcept {
		return _isSrcRepositioning;
	}

	void RecreateAfterSrcRepositioned() noexcept;

	void CleanAfterSrcRepositioned() noexcept;

protected:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	ScalingWindow() noexcept;
	~ScalingWindow() noexcept;

	int _CheckSrcState() const noexcept;

	bool _CheckForeground(HWND hwndForeground) const noexcept;

	bool _DisableDirectFlip() noexcept;

	void _SetWindowProps() const noexcept;

	void _RemoveWindowProps() const noexcept;

	void _CreateTouchHoleWindows() noexcept;

	winrt::DispatcherQueue _dispatcher{ nullptr };

	RECT _wndRect{};

	ScalingOptions _options;
	std::unique_ptr<class Renderer> _renderer;
	std::unique_ptr<class CursorManager> _cursorManager;

	HWND _hwndSrc = NULL;
	RECT _srcWndRect{};

	wil::unique_hwnd _hwndDDF;
	wil::unique_mutex_nothrow _exclModeMutex;

	std::array<wil::unique_hwnd, 4> _hwndTouchHoles{};

	bool _isSrcRepositioning = false;
	bool _isDDFWindowShown = false;
};

}
