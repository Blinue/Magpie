#pragma once
#include "WindowBase.h"
#include "ScalingOptions.h"
#include "ScalingError.h"
#include "SrcTracker.h"

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

	// 用于检查当前缩放是否结束
	static uint32_t RunId() noexcept {
		return _runId.load(std::memory_order_relaxed);
	}

	static void Dispatcher(const winrt::DispatcherQueue& value) noexcept {
		_dispatcher = value;
	}

	static const winrt::DispatcherQueue& Dispatcher() noexcept {
		return _dispatcher;
	}

	ScalingError Create(HWND hwndSrc, ScalingOptions options) noexcept;

	void Render() noexcept;

	void ToggleToolbarState() noexcept;

	const RECT& RendererRect() const noexcept {
		return _rendererRect;
	}

	const ScalingOptions& Options() const noexcept {
		return _options;
	}

	SrcTracker& SrcTracker() noexcept {
		return _srcTracker;
	}

	class Renderer& Renderer() noexcept {
		return *_renderer;
	}

	CursorManager& CursorManager() noexcept {
		return *_cursorManager;
	}

	bool IsSrcRepositioning() const noexcept {
		return _isSrcRepositioning;
	}

	void RecreateAfterSrcRepositioned() noexcept;

	void CleanAfterSrcRepositioned() noexcept;

	bool IsResizingOrMoving() const noexcept {
		return _isResizingOrMoving;
	}

	winrt::hstring GetLocalizedString(std::wstring_view resName) const;

	// 缩放过程中出现的错误
	ScalingError RuntimeError() const noexcept {
		return _runtimeError;
	}

	void RuntimeError(ScalingError value) noexcept {
		_runtimeError = value;
	}

	void ShowToast(std::wstring_view msg) const noexcept {
		_options.showToast(Handle(), msg);
	}

protected:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	ScalingWindow() noexcept;
	~ScalingWindow() noexcept;

	// 确保渲染窗口长宽比不变，且限制最小和最大尺寸。必须提供 width 和 height 之一，另一个
	// 应为 0。如果 isRendererSize 为真，传入的 width 和 height 为渲染矩形尺寸，否则为缩
	// 放窗口尺寸。返回时 width 和 height 是新的缩放窗口尺寸。
	bool _CalcWindowedScalingWindowSize(int& width, int& height, bool isRendererSize, uint32_t dpi = 0) const noexcept;

	RECT _CalcWindowedRendererRect() const noexcept;

	ScalingError _CalcFullscreenRendererRect(uint32_t& monitorCount) noexcept;

	ScalingError _InitialMoveSrcWindowInFullscreen() noexcept;

	void _Show() noexcept;

	bool _UpdateSrcState() noexcept;

	bool _CheckForegroundFor3DGameMode(HWND hwndFore) const noexcept;

	void _SetWindowProps() const noexcept;

	void _UpdateWindowProps() const noexcept;

	void _UpdateTouchProps(const RECT& srcRect) const noexcept;

	void _RemoveWindowProps() const noexcept;

	static LRESULT CALLBACK _RendererWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void _ResizeRenderer() noexcept;

	void _MoveRenderer() noexcept;

	static LRESULT CALLBACK _BorderHelperWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void _CreateBorderHelperWindows() noexcept;

	void _RepostionBorderHelperWindows() noexcept;

	RECT _CalcSrcTouchRect() const noexcept;

	void _UpdateTouchHoleWindows(bool onInit) noexcept;

	void _UpdateFrameMargins() const noexcept;

	winrt::fire_and_forget _UpdateFocusStateAsync(bool onShow = false) const noexcept;

	bool _IsBorderless() const noexcept;

	void _UpdateRendererRect() noexcept;

	bool _EnsureCaptionVisibleOnScreen() noexcept;

	void _UpdateWindowRectFromWindowPos(const WINDOWPOS& windowPos) noexcept;

	void _DelayedDestroy(bool onSrcHung = false) const noexcept;

	static inline std::atomic<uint32_t> _runId = 0;
	static inline winrt::DispatcherQueue _dispatcher{ nullptr };

	RECT _windowRect{};
	RECT _rendererRect{};
	HWND _hwndRenderer = NULL;

	uint32_t _currentDpi = USER_DEFAULT_SCREEN_DPI;
	uint32_t _topBorderThicknessInClient = 0;
	// Win11 中“无边框”窗口的边框在客户区内
	uint32_t _nonTopBorderThicknessInClient = 0;

	ScalingOptions _options;
	std::unique_ptr<class Renderer> _renderer;
	std::unique_ptr<class CursorManager> _cursorManager;

	class SrcTracker _srcTracker;

	winrt::ResourceLoader _resourceLoader{ nullptr };

	wil::unique_mutex_nothrow _exclModeMutex;

	std::array<wil::unique_hwnd, 4> _hwndResizeHelpers{};
	std::array<wil::unique_hwnd, 4> _hwndTouchHoles{};

	ScalingError _runtimeError = ScalingError::NoError;

	// 第一帧渲染完成后再显示
	bool _isFirstFrame = false;
	bool _isResizingOrMoving = false;
	// 用于区分调整大小和移动
	bool _isPreparingForResizing = false;
	bool _isMovingDueToSrcMoved = false;
	bool _shouldWaitForRender = false;
	bool _areResizeHelperWindowsVisible = false;
	bool _isSrcRepositioning = false;
};

}
