#pragma once
#include "WindowBase.h"
#include "ScalingOptions.h"
#include "Win32Helper.h"
#include "ScalingError.h"
#include "SrcInfo.h"

namespace Magpie {

class CursorManager;

class ScalingWindow : public WindowBaseT<ScalingWindow> {
	using base_type = WindowBaseT<ScalingWindow>;
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

	const RECT& SwapChainRect() const noexcept {
		return _swapChainRect;
	}

	const ScalingOptions& Options() const noexcept {
		return _options;
	}

	SrcInfo& SrcInfo() noexcept {
		return _srcInfo;
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

	// 缩放过程中出现的错误
	ScalingError RuntimeError() const noexcept {
		return _runtimeError;
	}

	void RuntimeError(ScalingError value) noexcept {
		_runtimeError = value;
	}

protected:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	ScalingWindow() noexcept;
	~ScalingWindow() noexcept;

	bool _CheckSrcState() noexcept;

	bool _CheckForegroundFor3DGameMode(HWND hwndFore) const noexcept;

	bool _DisableDirectFlip() noexcept;

	void _SetWindowProps() const noexcept;

	void _RemoveWindowProps() const noexcept;

	static LRESULT CALLBACK _BorderHelperWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void _CreateBorderHelperWindows() noexcept;

	void _CreateTouchHoleWindows() noexcept;

	void _UpdateFrameMargins() const noexcept;

	void _UpdateFocusState() const noexcept;

	bool _IsBorderless() const noexcept;

	winrt::DispatcherQueue _dispatcher{ nullptr };

	RECT _windowRect{};
	RECT _swapChainRect{};

	uint32_t _currentDpi = USER_DEFAULT_SCREEN_DPI;
	uint32_t _topBorderThicknessInClient = 0;

	ScalingOptions _options;
	std::unique_ptr<class Renderer> _renderer;
	std::unique_ptr<class CursorManager> _cursorManager;

	class SrcInfo _srcInfo;

	wil::unique_hwnd _hwndDDF;
	wil::unique_mutex_nothrow _exclModeMutex;

	std::array<wil::unique_hwnd, 4> _hwndResizeHelpers{};
	std::array<wil::unique_hwnd, 4> _hwndTouchHoles{};

	ScalingError _runtimeError = ScalingError::NoError;

	bool _isSrcRepositioning = false;
	bool _isDDFWindowShown = false;
};

}
