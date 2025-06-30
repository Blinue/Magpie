#include "pch.h"
#include "CursorManager.h"
#include "Logger.h"
#include "Win32Helper.h"
#include "ScalingOptions.h"
#include "ScalingWindow.h"
#include "Renderer.h"
#include <magnification.h>
#include <dwmapi.h>

namespace Magpie {

// 将源窗口的光标位置映射到缩放后的光标位置。当光标位于源窗口之外，与源窗口的距离不会缩放。
// 对于光标，第一个像素映射到第一个像素，最后一个像素映射到最后一个像素，因此光标区域的缩放
// 倍率和窗口缩放倍率不同！
static POINT SrcToScaling(POINT pt, bool skipBorder) noexcept {
	const Renderer& renderer = ScalingWindow::Get().Renderer();
	const RECT& srcRect = renderer.SrcRect();
	const RECT& destRect = renderer.DestRect();
	const RECT& rendererRect = ScalingWindow::Get().RendererRect();

	POINT result{};

	if (pt.x >= srcRect.right) {
		result.x = (skipBorder ? rendererRect.right : destRect.right) + pt.x - srcRect.right;
	} else if (pt.x < srcRect.left) {
		result.x = (skipBorder ? rendererRect.left : destRect.left) + pt.x - srcRect.left;
	} else {
		double pos = double(pt.x - srcRect.left) / (srcRect.right - srcRect.left - 1);
		result.x = std::lround(pos * (destRect.right - destRect.left - 1)) + destRect.left;
	}

	if (pt.y >= srcRect.bottom) {
		result.y = (skipBorder ? rendererRect.bottom : destRect.bottom) + pt.y - srcRect.bottom;
	} else if (pt.y < srcRect.top) {
		result.y = (skipBorder ? rendererRect.top : destRect.top) + pt.y - srcRect.top;
	} else {
		double pos = double(pt.y - srcRect.top) / (srcRect.bottom - srcRect.top - 1);
		result.y = std::lround(pos * (destRect.bottom - destRect.top - 1)) + destRect.top;
	}

	return result;
}

enum class RoundMethod {
	Round,
	Floor,
	Ceil
};

static POINT ScalingToSrc(POINT pt, RoundMethod roundType = RoundMethod::Round) noexcept {
	const Renderer& renderer = ScalingWindow::Get().Renderer();
	const RECT& srcRect = renderer.SrcRect();
	const RECT& destRect = renderer.DestRect();

	const SIZE srcSize = Win32Helper::GetSizeOfRect(srcRect);
	const SIZE destSize = Win32Helper::GetSizeOfRect(destRect);

	POINT result = { srcRect.left, srcRect.top };

	if (pt.x >= destRect.right) {
		result.x += srcSize.cx + pt.x - destRect.right;
	} else if (pt.x < destRect.left) {
		result.x += pt.x - destRect.left;
	} else {
		double pos = double(pt.x - destRect.left) / (destSize.cx - 1);
		double delta = pos * (srcSize.cx - 1);

		if (roundType == RoundMethod::Round) {
			result.x += std::lround(delta);
		} else if (roundType == RoundMethod::Floor) {
			result.x += (LONG)std::floor(delta);
		} else {
			result.x += (LONG)std::ceil(delta);
		}
	}

	if (pt.y >= destRect.bottom) {
		result.y += srcSize.cy + pt.y - destRect.bottom;
	} else if (pt.y < destRect.top) {
		result.y += pt.y - destRect.top;
	} else {
		double pos = double(pt.y - destRect.top) / (destSize.cy - 1);
		double delta = pos * (srcSize.cy - 1);

		if (roundType == RoundMethod::Round) {
			result.y += std::lround(delta);
		} else if (roundType == RoundMethod::Floor) {
			result.y += (LONG)std::floor(delta);
		} else {
			result.y += (LONG)std::ceil(delta);
		}
	}

	return result;
}

CursorManager::~CursorManager() noexcept {
	_ShowSystemCursor(true, true);
	_RestoreClipCursor();

	if (_isUnderCapture) {
		POINT cursorPos;
		if (!GetCursorPos(&cursorPos)) {
			Logger::Get().Win32Error("GetCursorPos 失败");
		}

		_StopCapture(cursorPos, true);
		_ReliableSetCursorPos(cursorPos);
	}
}

void CursorManager::Update() noexcept {
	_UpdateCursorState();
	_UpdateCursorPos();
}

void CursorManager::OnScalingPosChanged() noexcept {
	if (_isUnderCapture && !ScalingWindow::Get().SrcTracker().IsMoving()) {
		// 确保光标的缩放后位置不变
		_ReliableSetCursorPos(ScalingToSrc(_cursorPos));
	}

	_lastCompletedHitTestId = _nextHitTestId++;
	_lastCompletedHitTestPos.x = std::numeric_limits<LONG>::max();
	_lastCompletedHitTestResult = HTNOWHERE;
}

void CursorManager::OnSrcStartMove() noexcept {
	if (!_isUnderCapture) {
		return;
	}

	// 以防 _UpdateCursorState 错过时机没有设置 _localCursorPosOnMoving。
	// 源窗口自己实现拖拽逻辑或标题栏上右键然后立刻左键可能遇到这种情况。
	if (_localCursorPosOnMoving.x == std::numeric_limits<LONG>::max()) {
		const RECT& rendererRect = ScalingWindow::Get().RendererRect();
		_localCursorPosOnMoving.x = _cursorPos.x - rendererRect.left;
		_localCursorPosOnMoving.y = _cursorPos.y - rendererRect.top;
	}

	// 源窗口移动时临时还原光标移动速度
	_RestoreCursorSpeed();
}

void CursorManager::OnSrcEndMove() noexcept {
	if (!_isUnderCapture) {
		return;
	}

	_localCursorPosOnMoving.x = std::numeric_limits<LONG>::max();

	_AdjustCursorSpeed();
}

void CursorManager::OnStartMove() noexcept {
	if (_isUnderCapture) {
		return;
	}

	const RECT& rendererRect = ScalingWindow::Get().RendererRect();
	_localCursorPosOnMoving.x = _cursorPos.x - rendererRect.left;
	_localCursorPosOnMoving.y = _cursorPos.y - rendererRect.top;
}

void CursorManager::OnEndResizeMove() noexcept {
	_localCursorPosOnMoving.x = std::numeric_limits<LONG>::max();
}

void CursorManager::OnSrcRectChanged() noexcept {
	_ClearHitTestResult();
}

void CursorManager::IsCursorOnOverlay(bool value) noexcept {
	if (_isOnOverlay == value) {
		return;
	}
	_isOnOverlay = value;
	
	_UpdateCursorState();
	_UpdateCursorPos();
}

void CursorManager::IsCursorCapturedOnOverlay(bool value) noexcept {
	if (_isCapturedOnOverlay == value) {
		return;
	}
	_isCapturedOnOverlay = value;

	_UpdateCursorState();
	_UpdateCursorPos();
}

void CursorManager::_ShowSystemCursor(bool show, bool onDestory) {
	if (ScalingWindow::Get().Options().IsDebugMode()) {
		return;
	}

	if (_isSystemCursorShown == show) {
		return;
	}

	static void (WINAPI* const showSystemCursor)(BOOL bShow) = []()->void(WINAPI*)(BOOL) {
		HMODULE hUser32 = GetModuleHandle(L"user32.dll");
		assert(hUser32);
		return (void(WINAPI*)(BOOL))GetProcAddress(hUser32, "ShowSystemCursor");
	}();

	if (showSystemCursor) {
		showSystemCursor((BOOL)show);
		_isSystemCursorShown = show;
	} else {
		// 获取 ShowSystemCursor 失败则回落到 Magnification API
		static bool initialized = []() {
			if (!MagInitialize()) {
				Logger::Get().Win32Error("MagInitialize 失败");
				return false;
			}

			return true;
		}();

		if (initialized) {
			MagShowSystemCursor(show);
			_isSystemCursorShown = show;
		}
	}

	ScalingWindow::Get().Renderer().OnCursorVisibilityChanged(show, onDestory);
}

void CursorManager::_AdjustCursorSpeed() noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();
	if (!options.IsAdjustCursorSpeed() || options.IsDebugMode()) {
		return;
	}

	if (!SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_originCursorSpeed, 0)) {
		Logger::Get().Win32Error("获取光标移速失败");
		return;
	}

	// 鼠标加速默认打开
	bool isMouseAccelerationOn = true;
	{
		std::array<INT, 3> values{};
		if (SystemParametersInfo(SPI_GETMOUSE, 0, values.data(), 0)) {
			isMouseAccelerationOn = values[2];
		} else {
			Logger::Get().Win32Error("检索鼠标加速失败");
		}
	}

	const Renderer& renderer = ScalingWindow::Get().Renderer();
	const SIZE srcSize = Win32Helper::GetSizeOfRect(renderer.SrcRect());
	const SIZE destSize = Win32Helper::GetSizeOfRect(renderer.DestRect());
	const double scale = ((double)destSize.cx / srcSize.cx + (double)destSize.cy / srcSize.cy) / 2;

	INT newSpeed = 0;

	// “提高指针精确度”（鼠标加速）打开时光标移速的调整为线性，否则为非线性
	// 参见 https://liquipedia.net/counterstrike/Mouse_Settings#Windows_Sensitivity
	if (isMouseAccelerationOn) {
		newSpeed = std::clamp((INT)lround(_originCursorSpeed / scale), 1, 20);
	} else {
		static constexpr std::array<double, 20> SENSITIVITIES = {
			0.03125, 0.0625, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875,
			1.0, 1.25, 1.5, 1.75, 2, 2.25, 2.5, 2.75, 3, 3.25, 3.5
		};

		_originCursorSpeed = std::clamp(_originCursorSpeed, 1, 20);
		double newSensitivity = SENSITIVITIES[static_cast<size_t>(_originCursorSpeed) - 1] / scale;

		auto it = std::lower_bound(SENSITIVITIES.begin(), SENSITIVITIES.end(),
			newSensitivity - FLOAT_EPSILON<double>);
		newSpeed = INT(it - SENSITIVITIES.begin()) + 1;

		if (it != SENSITIVITIES.begin() && it != SENSITIVITIES.end()) {
			// 找到两侧最接近的数值
			if (std::abs(*it - newSensitivity) > std::abs(*(it - 1) - newSensitivity)) {
				--newSpeed;
			}
		}
	}

	if (!SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)newSpeed, 0)) {
		Logger::Get().Win32Error("设置光标移速失败");
	}
}

void CursorManager::_RestoreCursorSpeed() noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();
	if (!options.IsAdjustCursorSpeed() || options.IsDebugMode()) {
		return;
	}

	if (_originCursorSpeed != 0) {
		SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)_originCursorSpeed, 0);
		_originCursorSpeed = 0;
	}
}

// SetCursorPos 无法可靠移动光标，虽然调用之后立刻查询光标位置没有问题，但经过一段时
// 间后再次查询会发现光标位置又回到了设置之前。这可能是因为 OS 异步处理硬件输入队列，
// SetCursorPos 时队列中仍有旧事件尚未处理。
// 
// 这个函数使用 ClipCursor 将光标限制在目标位置一段时间，等待系统将输入队列处理完毕。
void CursorManager::_ReliableSetCursorPos(POINT pos) const noexcept {
	RECT originClipRect;
	GetClipCursor(&originClipRect);

	RECT newClipRect{ pos.x,pos.y,pos.x + 1,pos.y + 1 };
	ClipCursor(&newClipRect);

	// 等待一段时间，不能太短
	Sleep(8);

	// 还原原始光标限制区域
	ClipCursor(&originClipRect);

	// 有的窗口（比如 Magpie 主窗口）捕获光标后光标形状有时不会主动更新，发送 WM_SETCURSOR
	// 强制更新。
	if (_isUnderCapture) {
		const HWND hwndSrc = ScalingWindow::Get().SrcTracker().Handle();

		HWND hwndChild;
		int16_t ht = Win32Helper::AdvancedWindowHitTest(hwndSrc, pos, 10, &hwndChild);
		// wParam 传顶层窗口还是子窗口文档没说明，但测试表明必须传入顶层窗口句柄才能起作用
		PostMessage(hwndChild, WM_SETCURSOR,
			(WPARAM)hwndSrc, MAKELPARAM(ht, WM_MOUSEMOVE));
	}
}

winrt::fire_and_forget CursorManager::_SrcHitTestAsync(POINT screenPos) noexcept {
	const uint32_t runId = ScalingWindow::RunId();
	const uint32_t id = _nextHitTestId++;
	const HWND hwndSrc = ScalingWindow::Get().SrcTracker().Handle();

	co_await winrt::resume_background();

	const int16_t area = Win32Helper::AdvancedWindowHitTest(hwndSrc, screenPos, 100);

	co_await ScalingWindow::Get().Dispatcher();

	if (runId != ScalingWindow::RunId() || id <= _lastCompletedHitTestId) {
		co_return;
	}

	_lastCompletedHitTestId = id;
	_lastCompletedHitTestPos = screenPos;
	if (_lastCompletedHitTestResult != area) {
		_lastCompletedHitTestResult = area;
		// 命中测试变化则立刻重新计算捕获
		_UpdateCursorState();
	}
}

void CursorManager::_ClearHitTestResult() noexcept {
	_lastCompletedHitTestId = _nextHitTestId++;
	_lastCompletedHitTestPos.x = std::numeric_limits<LONG>::max();
	_lastCompletedHitTestResult = HTNOWHERE;
}

static bool PtInWindow(HWND hWnd, POINT pt) noexcept {
	// 检查窗口是否可见
	if (!IsWindowVisible(hWnd)) {
		return false;
	}

	// 检查是否在窗口内
	RECT windowRect;
	if (!GetWindowRect(hWnd, &windowRect) || !PtInRect(&windowRect, pt)) {
		return false;
	}

	// 检查窗口是否对鼠标透明
	if (GetWindowExStyle(hWnd) & WS_EX_TRANSPARENT) {
		return false;
	}

	// 检查窗口是否被冻结。这个调用比较耗时，因此稍晚检查
	{
		UINT isCloaked = 0;
		HRESULT hr = DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &isCloaked, sizeof(isCloaked));
		if (SUCCEEDED(hr) && isCloaked) {
			return false;
		}
	}

	// 进一步检查窗口是否对鼠标透明，这比较耗时，因此稍晚检查。除了 WS_EX_TRANSPARENT，还存在两种透明机制:
	// 
	// 1. 分层窗口
	// 2. 使用 SetWindowRgn 自定义形状的窗口
	// 
	// 注意无需考虑 HTTRANSPARENT，它只能作用于子窗口。
	// 
	// 由于前者只能使客户区域透明，ChildWindowFromPointEx 可以完美处理，该接口
	// 也会考虑自定义形状的窗口。反之如果位于非客户区，我们需手动处理后者。
	//
	// 可以参考 ChildWindowFromPointEx 的实现:
	// https://github.com/tongzx/nt5src/blob/daad8a087a4e75422ec96b7911f1df4669989611/Source/XPSP1/NT/windows/core/ntuser/kernel/winwhere.c#L47

	RECT clientRect;
	if (!Win32Helper::GetClientScreenRect(hWnd, clientRect)) {
		// 出错返回 true，因为已经确定光标在窗口内
		return true;
	}

	if (PtInRect(&clientRect, pt)) {
		// 使用 ChildWindowFromPointEx 检查客户区是否透明。
		// 不关心子窗口，因此跳过尽可能多的子窗口以提高性能。
		SetLastError(0);
		if (ChildWindowFromPointEx(
			hWnd,
			{ pt.x - clientRect.left, pt.y - clientRect.top },
			CWP_SKIPINVISIBLE | CWP_SKIPDISABLED | CWP_SKIPTRANSPARENT
		)) {
			return true;
		}

		// 如果因权限不足等原因失败则视为不透明
		if (GetLastError() == 0) {
			// 命中了透明像素
			return false;
		}
	}

	// 不在客户区或 ChildWindowFromPointEx 失败则检查窗口区域
	static HRGN hRgn = CreateRectRgn(0, 0, 0, 0);
	const int regionType = GetWindowRgn(hWnd, hRgn);
	if (regionType == SIMPLEREGION || regionType == COMPLEXREGION) {
		if (!PtInRegion(hRgn, pt.x - windowRect.left, pt.y - windowRect.top)) {
			return false;
		}
	}

	return true;
}

// 检测光标位于哪个窗口上，是否检测缩放窗口由 clickThroughHost 指定
static HWND WindowFromPoint(HWND hwndScaling, const RECT& rendererRect, POINT pt, bool clickThroughHost) noexcept {
	struct EnumData {
		HWND result;
		HWND hwndScaling;
		RECT rendererRect;
		POINT pt;
		bool clickThroughHost;
	} data{ NULL, hwndScaling, rendererRect, pt, clickThroughHost };

	EnumWindows([](HWND hWnd, LPARAM lParam) {
		EnumData& data = *(EnumData*)lParam;
		if (hWnd == data.hwndScaling) {
			if (PtInRect(&data.rendererRect, data.pt) && !data.clickThroughHost) {
				data.result = hWnd;
				return FALSE;
			} else {
				return TRUE;
			}
		}

		if (PtInWindow(hWnd, data.pt)) {
			data.result = hWnd;
			return FALSE;
		} else {
			return TRUE;
		}
	}, (LPARAM)&data);

	return data.result;
}

static bool IsEdgeArea(int16_t area) noexcept {
	return area >= HTSIZEFIRST && area <= HTSIZELAST;
}

void CursorManager::_UpdateCursorState() noexcept {
	if (ScalingWindow::Get().IsResizingOrMoving()) {
		_RestoreClipCursor();
		return;
	}

	if (ScalingWindow::Get().SrcTracker().IsMoving()) {
		if (_isUnderCapture) {
			// 防止缩放后光标超出屏幕
			_ClipCursorOnSrcMoving();
		} else {
			_RestoreClipCursor();
		}

		return;
	}

	const ScalingOptions& options = ScalingWindow::Get().Options();
	const Renderer& renderer = ScalingWindow::Get().Renderer();
	const RECT& srcRect = renderer.SrcRect();
	const RECT& destRect = renderer.DestRect();

	// 优先级: 
	// 1. 3D 游戏模式: 每帧都限制一次，不退出捕获，不支持多屏幕
	// 2. 常规: 根据多屏幕限制光标，捕获/取消捕获，支持 UI 和多屏幕

	if (options.Is3DGameMode()) {
		if (!_isUnderCapture) {
			POINT cursorPos;
			GetCursorPos(&cursorPos);
			_StartCapture(cursorPos);
			_ReliableSetCursorPos(cursorPos);

			_shouldDrawCursor = true;
			_ShowSystemCursor(false);

			// 缩放窗口始终透明
			HWND hwndScaling = ScalingWindow::Get().Handle();
			SetWindowLong(hwndScaling, GWL_EXSTYLE,
				GetWindowExStyle(hwndScaling) | WS_EX_TRANSPARENT);
		}

		// 开启“在 3D 游戏中限制光标”则每帧都限制一次光标
		_SetClipCursor(srcRect, true);
		return;
	}

	if (_isCapturedOnOverlay) {
		// 光标被叠加层捕获时将光标限制在输出区域内
		_SetClipCursor(destRect);
		return;
	}

	const HWND hwndSrc = ScalingWindow::Get().SrcTracker().Handle();

	{
		// 如果前台窗口捕获了光标，应避免在光标移入/移出缩放窗口或叠加层时跳跃。为了解决
		// 前一个问题，此时则将光标限制在前台窗口内，因此不会移出缩放窗口。为了解决后一个
		// 问题，叠加层将不会试图捕获光标。
		_isCapturedOnForeground = false;

		GUITHREADINFO info{ .cbSize = sizeof(info) };
		GetGUIThreadInfo(NULL, &info);

		if (info.hwndCapture &&
			!(info.flags & (GUI_INMENUMODE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE)))
		{
			_isCapturedOnForeground = true;

			// 拖拽源窗口时应确保光标位置稳定，为此我们需要确定初始光标位置。但为什么在这
			// 里而不是 OnSrcStartMove 中？虽然很难注意到，但光标在标题栏上轻微移动时不会
			// 触发拖拽，而是移动距离足够或者一段时间后才会触发，这意味着 OnSrcStartMove
			// 中光标可能已经不在初始位置。幸运的是，左键按下后前台窗口会立刻捕获光标，这是
			// 确定初始光标位置的最好时机。
			if (info.hwndCapture == hwndSrc &&
				_localCursorPosOnMoving.x == std::numeric_limits<LONG>::max())
			{
				const RECT& rendererRect = ScalingWindow::Get().RendererRect();
				_localCursorPosOnMoving.x = _cursorPos.x - rendererRect.left;
				_localCursorPosOnMoving.y = _cursorPos.y - rendererRect.top;
			}

			// 如果光标不在缩放窗口内或通过标题栏拖动窗口时不应限制光标
			if (_isUnderCapture && !(info.flags & GUI_INMOVESIZE)) {
				_SetClipCursor(srcRect);
			} else {
				_RestoreClipCursor();
			}

			// 当光标被前台窗口捕获时我们除了限制光标外什么也不做，即光标
			// 可以在缩放窗口上自由移动。
			return;
		}

		// 处理只是点击了标题栏而没有拖动的情况
		if (info.hwndCapture != hwndSrc) {
			_localCursorPosOnMoving.x = std::numeric_limits<LONG>::max();
		}
	}

	const HWND hwndScaling = ScalingWindow::Get().Handle();
	// 复制是特意的，用于检查缩放窗口的位置和尺寸更改
	const RECT rendererRect = ScalingWindow::Get().RendererRect();
	const bool isSrcFocused = ScalingWindow::Get().SrcTracker().IsFocused();
	
	const DWORD style = GetWindowExStyle(hwndScaling);

	POINT cursorPos;
	if (!GetCursorPos(&cursorPos)) {
		Logger::Get().Win32Error("GetCursorPos 失败");
		_RestoreClipCursor();
		return;
	}

	bool shouldClearHitTestResult = true;

	const POINT originCursorPos = cursorPos;

	if (_isUnderCapture) {
		///////////////////////////////////////////////////////////
		// 
		// 处于捕获状态
		// --------------------------------------------------------------
		// 					|   缩放位置被遮挡	|     缩放位置未被遮挡
		// --------------------------------------------------------------
		// 实际位置被遮挡	|      退出捕获		| 退出捕获，缩放窗口不透明
		// --------------------------------------------------------------
		// 实际位置未被遮挡	|      退出捕获		|         无操作
		// --------------------------------------------------------------
		// 
		///////////////////////////////////////////////////////////

		HWND hwndCur = WindowFromPoint(hwndScaling, rendererRect, SrcToScaling(cursorPos, isSrcFocused), false);
		_shouldDrawCursor = hwndCur == hwndScaling;

		if (_shouldDrawCursor) {
			// 缩放窗口未被遮挡
			bool stopCapture = _isOnOverlay;

			if (!stopCapture) {
				// 检查源窗口是否被遮挡
				hwndCur = WindowFromPoint(hwndScaling, rendererRect, cursorPos, true);

				stopCapture = hwndCur != hwndSrc &&
					(!IsChild(hwndSrc, hwndCur) || !(GetWindowStyle(hwndCur) & WS_CHILD));

				if (!stopCapture) {
					shouldClearHitTestResult = false;

					if (_lastCompletedHitTestPos != cursorPos) {
						_SrcHitTestAsync(cursorPos);
					}

					stopCapture = IsEdgeArea(_lastCompletedHitTestResult);
					// 窗口模式缩放时可调整大小的区域经常位于缩放窗口边缘，因此使用系统光标
					if (stopCapture) {
						_shouldDrawCursor = !options.IsWindowedMode();
					}
				}
			}

			if (stopCapture) {
				if (style & WS_EX_TRANSPARENT) {
					SetWindowLong(hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
				}

				// 源窗口被遮挡或者光标位于叠加层上，这时虽然停止捕获光标，但依然将光标隐藏
				_StopCapture(cursorPos);
			} else {
				if (_isOnOverlay) {
					if (style & WS_EX_TRANSPARENT) {
						SetWindowLong(hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
					}
				} else {
					if (!(style & WS_EX_TRANSPARENT)) {
						SetWindowLong(hwndScaling, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);
					}
				}
			}
		} else {
			// 缩放窗口被遮挡
			if (style & WS_EX_TRANSPARENT) {
				SetWindowLong(hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
			}

			if (!_StopCapture(cursorPos)) {
				_shouldDrawCursor = true;
			}
		}
	} else {
		/////////////////////////////////////////////////////////
		// 
		// 未处于捕获状态
		// -------------------------------------------------------------
		//					|   缩放位置被遮挡	|   缩放位置未被遮挡
		// -------------------------------------------------------------
		// 实际位置被遮挡	|      无操作		|    缩放窗口不透明
		// -------------------------------------------------------------
		// 实际位置未被遮挡	|      无操作		| 开始捕获，缩放窗口透明
		// -------------------------------------------------------------
		// 
		/////////////////////////////////////////////////////////

		HWND hwndCur = WindowFromPoint(hwndScaling, rendererRect, cursorPos, false);
		_shouldDrawCursor = hwndCur == hwndScaling;

		if (_shouldDrawCursor) {
			// 缩放窗口未被遮挡
			POINT newCursorPos = ScalingToSrc(cursorPos);

			if (PtInRect(&srcRect, newCursorPos)) {
				bool startCapture = !_isOnOverlay;

				if (startCapture) {
					// 检查源窗口是否被遮挡
					hwndCur = WindowFromPoint(hwndScaling, rendererRect, newCursorPos, true);

					startCapture = hwndCur == hwndSrc ||
						(IsChild(hwndSrc, hwndCur) && (GetWindowStyle(hwndCur) & WS_CHILD));

					if (startCapture) {
						shouldClearHitTestResult = false;

						if (_lastCompletedHitTestPos != newCursorPos) {
							_SrcHitTestAsync(newCursorPos);
						}

						startCapture = !IsEdgeArea(_lastCompletedHitTestResult);
						// 窗口模式缩放时可调整大小的区域经常位于缩放窗口边缘，因此使用系统光标
						if (!startCapture) {
							_shouldDrawCursor = !options.IsWindowedMode();
						}
					}
				}

				if (startCapture) {
					if (!(style & WS_EX_TRANSPARENT)) {
						SetWindowLong(hwndScaling, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);
					}
					
					_StartCapture(cursorPos);
				} else {
					if (style & WS_EX_TRANSPARENT) {
						SetWindowLong(hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
					}
				}
			} else if (isSrcFocused) {
				// 跳过黑边
				if (_isOnOverlay) {
					// 从内部移到外部，此时有 UI 贴边
					if (newCursorPos.x >= srcRect.right) {
						cursorPos.x += rendererRect.right - destRect.right;
					} else if (newCursorPos.x < srcRect.left) {
						cursorPos.x -= destRect.left - rendererRect.left;
					}

					if (newCursorPos.y >= srcRect.bottom) {
						cursorPos.y += rendererRect.bottom - destRect.bottom;
					} else if (newCursorPos.y < srcRect.top) {
						cursorPos.y -= destRect.top - rendererRect.top;
					}

					if (!MonitorFromPoint(cursorPos, MONITOR_DEFAULTTONULL)) {
						// 目标位置不存在屏幕，则将光标限制在输出区域内
						cursorPos.x = std::clamp(cursorPos.x, destRect.left, destRect.right - 1);
						cursorPos.y = std::clamp(cursorPos.y, destRect.top, destRect.bottom - 1);
					}
				} else {
					// 从外部移到内部
					const POINT clampedPos{
						std::clamp(cursorPos.x, destRect.left, destRect.right - 1),
						std::clamp(cursorPos.y, destRect.top, destRect.bottom - 1)
					};

					if (WindowFromPoint(hwndScaling, rendererRect, clampedPos, false) == hwndScaling) {
						if (!(style & WS_EX_TRANSPARENT)) {
							SetWindowLong(hwndScaling, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);
						}

						_StartCapture(cursorPos);
					} else {
						// 要跳跃的位置被遮挡
						if (style & WS_EX_TRANSPARENT) {
							SetWindowLong(hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
						}
					}
				}
			} else {
				// 源窗口不在前台则允许光标进入黑边
				if (!_isOnOverlay) {
					if (PtInRect(&destRect, cursorPos)) {
						if (!(style & WS_EX_TRANSPARENT)) {
							SetWindowLong(hwndScaling, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);
						}

						_StartCapture(cursorPos);
					} else {
						if (style & WS_EX_TRANSPARENT) {
							SetWindowLong(hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
						}
					}
				}
			}
		}
	}

	if (shouldClearHitTestResult) {
		_ClearHitTestResult();
	}

	// 只要光标缩放后的位置在缩放窗口上，且该位置未被其他窗口遮挡，便可以隐藏光标。
	// 即使当前并未捕获光标也是如此。
	_ShowSystemCursor(!_shouldDrawCursor);

	_ClipCursorForMonitors(cursorPos);

	// SetCursorPos 应在 ClipCursor 之后，否则会受到上一次 ClipCursor 的影响
	if (cursorPos != originCursorPos) {
		_ReliableSetCursorPos(cursorPos);
	}

	// 光标位置可能改变，需要更新
	_UpdateCursorPos();
}

static BOOL CALLBACK EnumMonitorProc(HMONITOR, HDC, LPRECT monitorRect, LPARAM data) {
	((SmallVectorImpl<RECT>*)data)->push_back(*monitorRect);
	return TRUE;
}

static SmallVector<RECT, 0> ObtainMonitorRects() noexcept {
	SmallVector<RECT, 0> monitorRects;
	if (!EnumDisplayMonitors(NULL, NULL, EnumMonitorProc, (LPARAM)&monitorRects)) {
		Logger::Get().Win32Error("EnumDisplayMonitors 失败");
	}
	return monitorRects;
}

static bool AnyIntersectedMonitor(const SmallVectorImpl<RECT>& monitorRects, const RECT& testRect) noexcept {
	for (const RECT& monitorRect : monitorRects) {
		if (Win32Helper::IsRectOverlap(monitorRect, testRect)) {
			return true;
		}
	}
	return false;
}

void CursorManager::_ClipCursorForMonitors(POINT cursorPos) noexcept {
	if (!_shouldDrawCursor) {
		_RestoreClipCursor();
	}

	const RECT& rendererRect = ScalingWindow::Get().RendererRect();
	const Renderer& renderer = ScalingWindow::Get().Renderer();
	const RECT& srcRect = renderer.SrcRect();
	const RECT& destRect = renderer.DestRect();

	const bool isSrcFocused = ScalingWindow::Get().SrcTracker().IsFocused();

	// 根据当前光标位置的四个方向有无屏幕来确定应该在哪些方向限制光标，但这无法
	// 处理屏幕之间存在间隙的情况。解决办法是 _StopCapture 只在目标位置存在屏幕时才取消捕获，
	// 当光标试图移动到间隙中时将被挡住。如果光标的速度足以跨越间隙，则它依然可以在屏幕间移动。
	const POINT scaledPos = _isUnderCapture ? SrcToScaling(cursorPos, true) : cursorPos;

	RECT clips{ LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX };

	const SmallVector<RECT, 0> monitorRects = ObtainMonitorRects();
	if (!monitorRects.empty()) {
		RECT rect{ LONG_MIN, scaledPos.y, rendererRect.left, scaledPos.y + 1 };

		// left
		if (!AnyIntersectedMonitor(monitorRects, rect)) {
			if (_isUnderCapture) {
				// 已确定缩放窗口左侧无屏幕，计算屏幕左边缘
				LONG minLeft = LONG_MAX;
				for (const RECT& monitorRect : monitorRects) {
					if (monitorRect.top <= scaledPos.y && monitorRect.bottom > scaledPos.y) {
						minLeft = std::min(minLeft, monitorRect.left);
					}
				}

				if (minLeft < destRect.right) {
					// 存在黑边且源窗口位于前台时，应阻止光标进入黑边
					if ((minLeft < destRect.left && isSrcFocused) || minLeft == destRect.left) {
						clips.left = srcRect.left;
					} else {
						// 将缩放后光标位置限制在屏幕内
						clips.left = ScalingToSrc({ minLeft,scaledPos.y }, RoundMethod::Ceil).x;
					}
				}
			} else if (isSrcFocused && destRect.left != rendererRect.left) {
				// 源窗口在前台时阻止光标进入黑边
				clips.left = destRect.left;
			}
		}

		// top
		rect = { scaledPos.x, LONG_MIN, scaledPos.x + 1, rendererRect.top };
		if (!AnyIntersectedMonitor(monitorRects, rect)) {
			if (_isUnderCapture) {
				LONG minTop = LONG_MAX;
				for (const RECT& monitorRect : monitorRects) {
					if (monitorRect.left <= scaledPos.x && monitorRect.right > scaledPos.x) {
						minTop = std::min(minTop, monitorRect.top);
					}
				}

				if (minTop < destRect.bottom) {
					if ((minTop < destRect.top && isSrcFocused) || minTop == destRect.top) {
						clips.top = srcRect.top;
					} else {
						clips.top = ScalingToSrc({ scaledPos.x,minTop }, RoundMethod::Ceil).y;
					}
				}
			} else if (isSrcFocused && destRect.top != rendererRect.top) {
				clips.top = destRect.top;
			}
		}

		// right
		rect = { rendererRect.right, scaledPos.y, LONG_MAX, scaledPos.y + 1 };
		if (!AnyIntersectedMonitor(monitorRects, rect)) {
			if (isSrcFocused) {
				clips.right = _isUnderCapture ? srcRect.right : destRect.right;
			} else if (_isUnderCapture && destRect.right == rendererRect.right) {
				clips.right = srcRect.right;
			}

			if (_isUnderCapture) {
				LONG maxRight = LONG_MIN;
				for (const RECT& monitorRect : monitorRects) {
					if (monitorRect.top <= scaledPos.y && monitorRect.bottom > scaledPos.y) {
						maxRight = std::max(maxRight, monitorRect.right);
					}
				}

				if (maxRight > destRect.left) {
					if ((maxRight > destRect.right && isSrcFocused) || maxRight == destRect.right) {
						clips.right = srcRect.right;
					} else {
						clips.right = ScalingToSrc({ maxRight,scaledPos.y }, RoundMethod::Floor).x;
					}
				}
			} else if (isSrcFocused && destRect.right != rendererRect.right) {
				clips.right = destRect.right;
			}
		}

		// bottom
		rect = { scaledPos.x, rendererRect.bottom, scaledPos.x + 1, LONG_MAX };
		if (!AnyIntersectedMonitor(monitorRects, rect)) {
			if (_isUnderCapture) {
				LONG maxBottom = LONG_MIN;
				for (const RECT& monitorRect : monitorRects) {
					if (monitorRect.left <= scaledPos.x && monitorRect.right > scaledPos.x) {
						maxBottom = std::max(maxBottom, monitorRect.bottom);
					}
				}

				if (maxBottom > destRect.top) {
					if ((maxBottom > destRect.bottom && isSrcFocused) || maxBottom == destRect.bottom) {
						clips.bottom = srcRect.bottom;
					} else {
						clips.bottom = ScalingToSrc({ scaledPos.x,maxBottom }, RoundMethod::Floor).y;
					}
				}
			} else if (isSrcFocused && destRect.bottom != rendererRect.bottom) {
				clips.bottom = destRect.bottom;
			}
		}
	}

	if (clips == RECT{ LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX }) {
		_RestoreClipCursor();
	} else {
		_SetClipCursor(clips);
	}
}

void CursorManager::_ClipCursorOnSrcMoving() noexcept {
	assert(ScalingWindow::Get().SrcTracker().IsMoving() && _isUnderCapture);
	assert(_localCursorPosOnMoving.x != std::numeric_limits<LONG>::max());

	const RECT& rendererRect = ScalingWindow::Get().RendererRect();
	const POINT scaledPos = {
		_localCursorPosOnMoving.x + rendererRect.left,
		_localCursorPosOnMoving.y + rendererRect.top
	};
	const POINT originPos = ScalingToSrc(scaledPos);

	RECT clips{ LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX };

	SmallVector<RECT, 0> monitorRects = ObtainMonitorRects();
	if (!monitorRects.empty()) {
		// 移动源窗口时，如果只有一个显示器，应将光标限制在工作矩形内。一旦超出工作矩形，
		// 源窗口将无法继续移动。还需检查窗口样式，以和 OS 保持一致，见
		// https://github.com/tongzx/nt5src/blob/daad8a087a4e75422ec96b7911f1df4669989611/Source/XPSP1/NT/windows/core/ntuser/kernel/movesize.c#L1142
		if (monitorRects.size() == 1) {
			const DWORD exStyle = GetWindowExStyle(ScalingWindow::Get().SrcTracker().Handle());
			if ((exStyle & (WS_EX_TOPMOST | WS_EX_TOOLWINDOW)) == 0) {
				// 获取主显示器句柄，来自 https://devblogs.microsoft.com/oldnewthing/20141106-00/?p=43683
				const HMONITOR hMon = MonitorFromWindow(NULL, MONITOR_DEFAULTTOPRIMARY);
				MONITORINFO mi{ .cbSize = sizeof(MONITORINFO) };
				if (GetMonitorInfo(hMon, &mi)) {
					monitorRects[0] = mi.rcWork;
					clips = mi.rcWork;
				}
			}
		}

		// left
		if (scaledPos.x < originPos.x) {
			LONG minLeft = LONG_MAX;
			for (const RECT& monitorRect : monitorRects) {
				if (monitorRect.top <= scaledPos.y && monitorRect.bottom > scaledPos.y) {
					minLeft = std::min(minLeft, monitorRect.left);
				}
			}
			if (minLeft != LONG_MAX) {
				// 将缩放后位置限制在屏幕内
				clips.left = minLeft + (originPos.x - scaledPos.x);
			}
		}

		// top
		if (scaledPos.y < originPos.y) {
			LONG minTop = LONG_MAX;
			for (const RECT& monitorRect : monitorRects) {
				if (monitorRect.left <= scaledPos.x && monitorRect.right > scaledPos.x) {
					minTop = std::min(minTop, monitorRect.top);
				}
			}
			if (minTop != LONG_MAX) {
				clips.top = minTop + (originPos.y - scaledPos.y);
			}
		}

		// right
		if (scaledPos.x > originPos.x) {
			LONG maxRight = LONG_MIN;
			for (const RECT& monitorRect : monitorRects) {
				if (monitorRect.top <= scaledPos.y && monitorRect.bottom > scaledPos.y) {
					maxRight = std::max(maxRight, monitorRect.right);
				}
			}
			if (maxRight != LONG_MIN) {
				clips.right = maxRight - (scaledPos.x - originPos.x);
			}
		}

		// bottom
		if (scaledPos.y > originPos.y) {
			LONG maxBottom = LONG_MIN;
			for (const RECT& monitorRect : monitorRects) {
				if (monitorRect.left <= scaledPos.x && monitorRect.right > scaledPos.x) {
					maxBottom = std::max(maxBottom, monitorRect.bottom);
				}
			}
			if (maxBottom != LONG_MIN) {
				clips.bottom = maxBottom - (scaledPos.y - originPos.y);
			}
		}
	}

	if (clips == RECT{ LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX }) {
		_RestoreClipCursor();
	} else {
		_SetClipCursor(clips);
	}
}

void CursorManager::_UpdateCursorPos() noexcept {
	if (_shouldDrawCursor) {
		CURSORINFO ci{ .cbSize = sizeof(CURSORINFO) };
		if (!GetCursorInfo(&ci)) {
			_hCursor = NULL;
			return;
		}

		if (ci.flags == CURSOR_SHOWING) {
			if (ScalingWindow::Get().Options().IsWindowedMode()) {
				_hCursor = ci.hCursor;
			} else {
				// 全屏模式缩放时我们阻止了源窗口四周可调整尺寸的区域，这使得鼠标从客户区移到边框
				// 的过程中会有一瞬间的闪烁。为了解决这个问题，这里特别处理调整尺寸时的光标形状。
				if (ci.hCursor == _hDiagonalSize1Cursor || ci.hCursor == _hDiagonalSize2Cursor ||
					ci.hCursor == _hHorizontalSizeCursor || ci.hCursor == _hVerticalSizeCursor) {
					if (_hCursor != ci.hCursor) {
						using std::chrono::steady_clock;

						if (_sizeCursorStartTime == steady_clock::time_point{}) {
							_sizeCursorStartTime = steady_clock::now();
						} else {
							// 延迟 50ms 更新以防止闪烁
							if (steady_clock::now() - _sizeCursorStartTime > 50ms) {
								_hCursor = ci.hCursor;
							}
						}
					}
				} else {
					_sizeCursorStartTime = {};
					_hCursor = ci.hCursor;
				}
			}
		} else {
			_hCursor = NULL;
		}

		_cursorPos = ci.ptScreenPos;
	} else {
		_hCursor = NULL;

		// 光标在缩放窗口外也检索光标位置，叠加层可能需要
		if (!GetCursorPos(&_cursorPos)) {
			return;
		}
	}

	// 拖拽源窗口时肯定处于捕获状态
	const bool isSrcMoving = _isUnderCapture && ScalingWindow::Get().SrcTracker().IsMoving();
	// 拖拽缩放窗口时肯定不处于捕获状态而且光标在工具栏上
	const bool isScalingMoving = !_isUnderCapture &&
		ScalingWindow::Get().IsResizingOrMoving() &&
		_localCursorPosOnMoving.x != std::numeric_limits<LONG>::max();
	
	if (isSrcMoving || isScalingMoving) {
		// 拖拽源窗口和缩放窗口时确保光标位置稳定
		const RECT& rendererRect = ScalingWindow::Get().RendererRect();
		_cursorPos.x = _localCursorPosOnMoving.x + rendererRect.left;
		_cursorPos.y = _localCursorPosOnMoving.y + rendererRect.top;
	} else if (_isUnderCapture) {
		_cursorPos = SrcToScaling(_cursorPos, false);
	}
}

void CursorManager::_StartCapture(POINT& cursorPos) noexcept {
	if (_isUnderCapture) {
		return;
	}

	const Renderer& renderer = ScalingWindow::Get().Renderer();
	const RECT& srcRect = renderer.SrcRect();
	const RECT& destRect = renderer.DestRect();

	// 在以下情况下进入捕获状态:
	// 1. 当前未捕获
	// 2. 光标进入全屏区域
	// 
	// 进入捕获状态时: 
	// 1. 调整光标速度，全局隐藏光标
	// 2. 将光标移到源窗口的对应位置
	//
	// 在有黑边的情况下自动将光标调整到画面内

	SIZE srcFrameSize = Win32Helper::GetSizeOfRect(srcRect);
	SIZE outputSize = Win32Helper::GetSizeOfRect(destRect);

	_AdjustCursorSpeed();

	// 移动光标位置，应跳过黑边
	cursorPos = ScalingToSrc(POINT{
		std::clamp(cursorPos.x, destRect.left, destRect.right - 1),
		std::clamp(cursorPos.y, destRect.top, destRect.bottom - 1)
	});

	_isUnderCapture = true;
}

bool CursorManager::_StopCapture(POINT& cursorPos, bool onDestroy) noexcept {
	if (!_isUnderCapture) {
		return true;
	}

	// 在以下情况下离开捕获状态:
	// 1. 当前处于捕获状态
	// 2. 光标离开源窗口客户区
	// 3. 目标位置存在屏幕
	//
	// 离开捕获状态时:
	// 1. 还原光标速度，全局显示光标
	// 2. 将光标移到全屏窗口外的对应位置
	//
	// 在有黑边的情况下自动将光标调整到全屏窗口外

	POINT newCursorPos = SrcToScaling(cursorPos, ScalingWindow::Get().SrcTracker().IsFocused());

	if (onDestroy || MonitorFromPoint(newCursorPos, MONITOR_DEFAULTTONULL)) {
		cursorPos = newCursorPos;
		_RestoreCursorSpeed();
		_isUnderCapture = false;
		return true;
	} else {
		// 目标位置不存在屏幕，则将光标限制在源窗口内
		const RECT& srcRect = ScalingWindow::Get().Renderer().SrcRect();

		cursorPos.x = std::clamp(cursorPos.x, srcRect.left, srcRect.right - 1);
		cursorPos.y = std::clamp(cursorPos.y, srcRect.top, srcRect.bottom - 1);

		return false;
	}
}

void CursorManager::_SetClipCursor(const RECT& clipRect, bool is3DGameMode) noexcept {
	if (ScalingWindow::Get().Options().IsDebugMode()) {
		return;
	}

	// 限制区域有变化才调用 ClipCursor，因为每次调用 ClipCursor 都会向前台窗口发送
	// WM_MOUSEMOVE 消息，一些程序无法正确处理，如 GH#920 和 GH#927。
	//
	// 曾经尝试过尊重其他程序的裁剪区域（GH#947），和我们的做交集，结果发现不切实际。
	// 一方面我们的场景很复杂，是否捕获光标、源窗口是否在前台、光标位置等都会影响限制
	// 区域，和其他程序协同几乎不可能；另一方面切换窗口后 OS 会自动清空限制区域，很难
	// 遇到需要协同的情况，不值得付出努力。
	if (!is3DGameMode) {
		RECT curClip;
		GetClipCursor(&curClip);

		if (curClip == _lastRealClip && clipRect == _lastClip) {
			return;
		}
	}

	if (ClipCursor(&clipRect)) {
		_lastClip = clipRect;
		GetClipCursor(&_lastRealClip);
	}
}

void CursorManager::_RestoreClipCursor() noexcept {
	if (_lastClip.left == std::numeric_limits<LONG>::max()) {
		return;
	}

	RECT curClip;
	GetClipCursor(&curClip);

	// 如果其他程序更改了光标限制区域，我们就放弃更改
	if (curClip != _lastRealClip) {
		return;
	}

	if (ClipCursor(nullptr)) {
		_lastClip = { std::numeric_limits<LONG>::max() };
	}
}

}
