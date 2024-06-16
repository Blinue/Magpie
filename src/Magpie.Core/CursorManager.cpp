#include "pch.h"
#include "CursorManager.h"
#include "Logger.h"
#include <magnification.h>
#include "Win32Utils.h"
#include "ScalingOptions.h"
#include "ScalingWindow.h"
#include "Renderer.h"

#pragma comment(lib, "Magnification.lib")

namespace Magpie::Core {

// 将源窗口的光标位置映射到缩放后的光标位置。当光标位于源窗口之外，与源窗口的距离不会缩放。
// 对于光标，第一个像素映射到第一个像素，最后一个像素映射到最后一个像素，因此光标区域的缩放
// 倍率和窗口缩放倍率不同！
static POINT SrcToScaling(POINT pt) noexcept {
	const Renderer& renderer = ScalingWindow::Get().Renderer();
	const RECT& srcRect = renderer.SrcRect();
	const RECT& destRect = renderer.DestRect();
	const RECT& scalingRect = ScalingWindow::Get().WndRect();

	POINT result;

	if (pt.x >= srcRect.right) {
		result.x = scalingRect.right + pt.x - srcRect.right;
	} else if (pt.x < srcRect.left) {
		result.x = scalingRect.left + pt.x - srcRect.left;
	} else {
		double pos = double(pt.x - srcRect.left) / (srcRect.right - srcRect.left - 1);
		result.x = std::lround(pos * (destRect.right - destRect.left - 1)) + destRect.left;
	}

	if (pt.y >= srcRect.bottom) {
		result.y = scalingRect.bottom + pt.y - srcRect.bottom;
	} else if (pt.y < srcRect.top) {
		result.y = scalingRect.top + pt.y - srcRect.top;
	} else {
		double pos = double(pt.y - srcRect.top) / (srcRect.bottom - srcRect.top - 1);
		result.y = std::lround(pos * (destRect.bottom - destRect.top - 1)) + destRect.top;
	}

	return result;
}

static POINT ScalingToSrc(POINT pt) noexcept {
	const Renderer& renderer = ScalingWindow::Get().Renderer();
	const RECT& srcRect = renderer.SrcRect();
	const RECT& destRect = renderer.DestRect();

	const SIZE srcSize = Win32Utils::GetSizeOfRect(srcRect);
	const SIZE destSize = Win32Utils::GetSizeOfRect(destRect);

	POINT result = { srcRect.left, srcRect.top };

	if (pt.x >= destRect.right) {
		result.x += srcSize.cx + pt.x - destRect.right;
	} else if (pt.x < destRect.left) {
		result.x += pt.x - destRect.left;
	} else {
		double pos = double(pt.x - destRect.left) / (destSize.cx - 1);
		result.x += std::lround(pos * (srcSize.cx - 1));
	}

	if (pt.y >= destRect.bottom) {
		result.y += srcSize.cy + pt.y - destRect.bottom;
	} else if (pt.y < destRect.top) {
		result.y += pt.y - destRect.top;
	} else {
		double pos = double(pt.y - destRect.top) / (destSize.cy - 1);
		result.y += std::lround(pos * (srcSize.cy - 1));
	}

	return result;
}

// SetCursorPos 无法可靠移动光标，虽然调用之后立刻查询光标位置没有问题，但经过一
// 段时间后再次查询会发现光标位置又回到了设置之前。这可能是因为 OS 异步处理硬件输
// 入队列，SetCursorPos 时队列中仍有旧事件尚未处理。
// 这个函数使用 SendInput 将移动光标事件插入输入队列，然后等待系统处理到该事件，
// 避免了并发问题。如果设置不成功则多次尝试。这里旨在尽最大努力，我怀疑是否有完美
// 的解决方案。
static void ReliableSetCursorPos(POINT pos) noexcept {
	// 检查光标的限制区域，如果要设置的位置不在限制区域内则回落到 SetCursorPos
	RECT clipRect;
	GetClipCursor(&clipRect);

	if (PtInRect(&clipRect, pos)) {
		const int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		const int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

		INPUT input{
			.type = INPUT_MOUSE,
			.mi{
				.dx = (pos.x * 65535) / (screenWidth - 1),
				.dy = (pos.y * 65535) / (screenHeight - 1),
				.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK
			}
		};

		// 如果设置不成功则多次尝试
		for (int i = 0; i < 10; ++i) {
			if (!SendInput(1, &input, sizeof(input))) {
				Logger::Get().Win32Error("SendInput 失败");
				break;
			}

			// 等待系统处理
			Sleep(0);

			POINT curCursorPos;
			if (!GetCursorPos(&curCursorPos)) {
				Logger::Get().Win32Error("GetCursorPos 失败");
				break;
			}

			if (curCursorPos == pos) {
				// 已成功，但保险起见再设置一次
				SendInput(1, &input, sizeof(input));
				return;
			}
		}
		// 多次不成功回落到 SetCursorPos
	}

	SetCursorPos(pos.x, pos.y);
}

CursorManager::~CursorManager() noexcept {
	if (ScalingWindow::Get().Options().IsDebugMode()) {
		return;
	}

	_ShowSystemCursor(true, true);

	if (_lastClip.left != std::numeric_limits<LONG>::max()) {
		_RestoreClipCursor();
	}

	if (_isUnderCapture) {
		POINT cursorPos;
		if (!GetCursorPos(&cursorPos)) {
			Logger::Get().Win32Error("GetCursorPos 失败");
		}

		_StopCapture(cursorPos, true);
		ReliableSetCursorPos(cursorPos);
	}
}

bool CursorManager::Initialize() noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();
	if (options.IsDebugMode()) {
		_shouldDrawCursor = true;
		_isUnderCapture = true;
	} else if (options.Is3DGameMode()) {
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		_StartCapture(cursorPos);
		ReliableSetCursorPos(cursorPos);

		_shouldDrawCursor = true;
		_ShowSystemCursor(false);
	}

	Logger::Get().Info("CursorManager 初始化完成");
	return true;
}

void CursorManager::Update() noexcept {
	_UpdateCursorClip();

	_hCursor = NULL;
	_cursorPos = { std::numeric_limits<LONG>::max(),std::numeric_limits<LONG>::max() };

	const ScalingOptions& options = ScalingWindow::Get().Options();
	if (!options.IsDrawCursor() || !_shouldDrawCursor) {
		return;
	}

	CURSORINFO ci{ .cbSize = sizeof(CURSORINFO) };
	if (!GetCursorInfo(&ci)) {
		Logger::Get().Win32Error("GetCursorPos 失败");
		return;
	}

	if (!ci.hCursor || ci.flags != CURSOR_SHOWING) {
		return;
	}

	_hCursor = ci.hCursor;
	// 不处于捕获状态则位于叠加层上
	_cursorPos = _isUnderCapture ? SrcToScaling(ci.ptScreenPos) : ci.ptScreenPos;
	const RECT& scalingRect = ScalingWindow::Get().WndRect();
	_cursorPos.x -= scalingRect.left;
	_cursorPos.y -= scalingRect.top;
}

void CursorManager::IsCursorOnOverlay(bool value) noexcept {
	if (_isOnOverlay == value) {
		return;
	}
	_isOnOverlay = value;
	
	_UpdateCursorClip();
}

void CursorManager::IsCursorCapturedOnOverlay(bool value) noexcept {
	if (_isCapturedOnOverlay == value) {
		return;
	}
	_isCapturedOnOverlay = value;

	_UpdateCursorClip();
}

void CursorManager::_ShowSystemCursor(bool show, bool onDestory) {
	if (_isSystemCursorShown == show) {
		return;
	}

	static void (WINAPI* const showSystemCursor)(BOOL bShow) = []()->void(WINAPI*)(BOOL) {
		HMODULE lib = LoadLibrary(L"user32.dll");
		if (!lib) {
			return nullptr;
		}

		return (void(WINAPI*)(BOOL))GetProcAddress(lib, "ShowSystemCursor");
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
	const SIZE srcSize = Win32Utils::GetSizeOfRect(renderer.SrcRect());
	const SIZE destSize = Win32Utils::GetSizeOfRect(renderer.DestRect());
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

		auto it = std::lower_bound(SENSITIVITIES.begin(), SENSITIVITIES.end(), newSensitivity - 1e-6);
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
	const LONG_PTR exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
	if (exStyle & WS_EX_TRANSPARENT) {
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
	if (!Win32Utils::GetClientScreenRect(hWnd, clientRect)) {
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

		// ChildWindowFromPointEx 返回 NULL 可能是因为命中了透明像素或权限不足
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
static HWND WindowFromPoint(HWND hwndScaling, const RECT& scalingWndRect, POINT pt, bool clickThroughHost) noexcept {
	struct EnumData {
		HWND result;
		HWND hwndScaling;
		RECT scalingWndRect;
		POINT pt;
		bool clickThroughHost;
	} data{ NULL, hwndScaling, scalingWndRect, pt, clickThroughHost };

	EnumWindows([](HWND hWnd, LPARAM lParam) {
		EnumData& data = *(EnumData*)lParam;
		if (hWnd == data.hwndScaling) {
			if (PtInRect(&data.scalingWndRect, data.pt) && !data.clickThroughHost) {
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

void CursorManager::_UpdateCursorClip() noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();
	const Renderer& renderer = ScalingWindow::Get().Renderer();
	const RECT& srcRect = renderer.SrcRect();
	const RECT& destRect = renderer.DestRect();

	// 优先级: 
	// 1. 调试模式: 不限制，不捕获
	// 2. 3D 游戏模式: 每帧都限制一次，不退出捕获，不支持多屏幕
	// 3. 常规: 根据多屏幕限制光标，捕获/取消捕获，支持 UI 和多屏幕

	if (options.IsDebugMode()) {
		return;
	}

	if (options.Is3DGameMode()) {
		// 开启“在 3D 游戏中限制光标”则每帧都限制一次光标
		_SetClipCursor(srcRect, true);
		return;
	}

	if (_isCapturedOnOverlay) {
		// 光标被叠加层捕获时将光标限制在输出区域内
		_SetClipCursor(destRect);
		return;
	}

	// 如果前台窗口捕获了光标，应避免在光标移入/移出缩放窗口或叠加层时跳跃。为了解决
	// 前一个问题，此时则将光标限制在前台窗口内，因此不会移出缩放窗口。为了解决后一个
	// 问题，叠加层将不会试图捕获光标。
	GUITHREADINFO info{ .cbSize = sizeof(info) };
	if (GetGUIThreadInfo(NULL, &info)) {
		if (info.hwndCapture) {
			_isCapturedOnForeground = true;

			// 如果光标不在缩放窗口内不应限制光标
			if (_isUnderCapture) {
				_SetClipCursor(srcRect);
			}
			
			// 当光标被前台窗口捕获时我们除了限制光标外什么也不做，即光标
			// 可以在缩放窗口上自由移动
			return;
		} else {
			_isCapturedOnForeground = false;
		}
	} else {
		_isCapturedOnForeground = false;
	}

	const HWND hwndScaling = ScalingWindow::Get().Handle();
	const RECT scalingRect = ScalingWindow::Get().WndRect();
	const HWND hwndSrc = ScalingWindow::Get().HwndSrc();
	
	INT_PTR style = GetWindowLongPtr(hwndScaling, GWL_EXSTYLE);

	POINT cursorPos;
	if (!GetCursorPos(&cursorPos)) {
		Logger::Get().Win32Error("GetCursorPos 失败");
		return;
	}

	const POINT originCursorPos = cursorPos;

	if (_isUnderCapture) {
		///////////////////////////////////////////////////////////
		// 
		// 处于捕获状态
		// --------------------------------------------------------
		//                  |  缩放位置被遮挡  |    缩放位置未被遮挡
		// --------------------------------------------------------
		// 实际位置被遮挡    |    退出捕获     | 退出捕获，主窗口不透明
		// --------------------------------------------------------
		// 实际位置未被遮挡  |    退出捕获      |        无操作
		// --------------------------------------------------------
		// 
		///////////////////////////////////////////////////////////

		HWND hwndCur = WindowFromPoint(hwndScaling, scalingRect, SrcToScaling(cursorPos), false);
		_shouldDrawCursor = hwndCur == hwndScaling;

		if (_shouldDrawCursor) {
			// 缩放窗口未被遮挡
			bool stopCapture = _isOnOverlay;

			if (!stopCapture) {
				// 判断源窗口是否被遮挡
				hwndCur = WindowFromPoint(hwndScaling, scalingRect, cursorPos, true);
				stopCapture = hwndCur != hwndSrc && (!IsChild(hwndSrc, hwndCur) || !((GetWindowStyle(hwndCur) & WS_CHILD)));
			}

			if (stopCapture) {
				if (style | WS_EX_TRANSPARENT) {
					SetWindowLongPtr(hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
				}

				// 源窗口被遮挡或者光标位于叠加层上，这时虽然停止捕获光标，但依然将光标隐藏
				_StopCapture(cursorPos);
			} else {
				if (_isOnOverlay) {
					if (style | WS_EX_TRANSPARENT) {
						SetWindowLongPtr(hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
					}
				} else {
					if (!(style & WS_EX_TRANSPARENT)) {
						SetWindowLongPtr(hwndScaling, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);
					}
				}
			}
		} else {
			// 缩放窗口被遮挡
			if (style | WS_EX_TRANSPARENT) {
				SetWindowLongPtr(hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
			}

			if (!_StopCapture(cursorPos)) {
				_shouldDrawCursor = true;
			}
		}
	} else {
		/////////////////////////////////////////////////////////
		// 
		// 未处于捕获状态
		// -----------------------------------------------------
		//					|  缩放位置被遮挡	|  缩放位置未被遮挡
		// ------------------------------------------------------
		// 实际位置被遮挡		|    无操作		|   缩放窗口不透明
		// ------------------------------------------------------
		// 实际位置未被遮挡	|    无操作		| 开始捕获，主窗口透明
		// ------------------------------------------------------
		// 
		/////////////////////////////////////////////////////////

		HWND hwndCur = WindowFromPoint(hwndScaling, scalingRect, cursorPos, false);
		_shouldDrawCursor = hwndCur == hwndScaling;

		if (_shouldDrawCursor) {
			// 缩放窗口未被遮挡
			POINT newCursorPos = ScalingToSrc(cursorPos);

			if (PtInRect(&srcRect, newCursorPos)) {
				bool startCapture = !_isOnOverlay;

				if (startCapture) {
					// 判断源窗口是否被遮挡
					hwndCur = WindowFromPoint(hwndScaling, scalingRect, newCursorPos, true);
					startCapture = hwndCur == hwndSrc || ((IsChild(hwndSrc, hwndCur) && (GetWindowStyle(hwndCur) & WS_CHILD)));
				}

				if (startCapture) {
					if (!(style & WS_EX_TRANSPARENT)) {
						SetWindowLongPtr(hwndScaling, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);
					}
					
					_StartCapture(cursorPos);
				} else {
					if (style | WS_EX_TRANSPARENT) {
						SetWindowLongPtr(hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
					}
				}
			} else {
				// 跳过黑边
				if (_isOnOverlay) {
					// 从内部移到外部
					// 此时有 UI 贴边
					if (newCursorPos.x >= srcRect.right) {
						cursorPos.x += scalingRect.right - destRect.right;
					} else if (newCursorPos.x < srcRect.left) {
						cursorPos.x -= destRect.left - scalingRect.left;
					}

					if (newCursorPos.y >= srcRect.bottom) {
						cursorPos.y += scalingRect.bottom - destRect.bottom;
					} else if (newCursorPos.y < srcRect.top) {
						cursorPos.y -= destRect.top - scalingRect.top;
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

					if (WindowFromPoint(hwndScaling, scalingRect, clampedPos, false) == hwndScaling) {
						if (!(style & WS_EX_TRANSPARENT)) {
							SetWindowLongPtr(hwndScaling, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);
						}

						_StartCapture(cursorPos);
					} else {
						// 要跳跃的位置被遮挡
						if (style | WS_EX_TRANSPARENT) {
							SetWindowLongPtr(hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
						}
					}
				}
			}
		}
	}

	// 只要光标缩放后的位置在缩放窗口上，且该位置未被其他窗口遮挡，便可以隐藏光标。
	// 即使当前并未捕获光标也是如此。
	_ShowSystemCursor(!_shouldDrawCursor);

	if (_shouldDrawCursor) {
		// 根据当前光标位置的四个方向有无屏幕来确定应该在哪些方向限制光标，但这无法
		// 处理屏幕之间存在间隙的情况。解决办法是 _StopCapture 只在目标位置存在屏幕时才取消捕获，
		// 当光标试图移动到间隙中时将被挡住。如果光标的速度足以跨越间隙，则它依然可以在屏幕间移动。
		POINT hostPos = _isUnderCapture ? SrcToScaling(cursorPos) : cursorPos;

		RECT clips{ LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX };

		// left
		RECT rect{ LONG_MIN, hostPos.y, scalingRect.left, hostPos.y + 1 };
		if (!MonitorFromRect(&rect, MONITOR_DEFAULTTONULL)) {
			clips.left = _isUnderCapture ? srcRect.left : destRect.left;
		}

		// top
		rect = { hostPos.x, LONG_MIN, hostPos.x + 1, scalingRect.top };
		if (!MonitorFromRect(&rect, MONITOR_DEFAULTTONULL)) {
			clips.top = _isUnderCapture ? srcRect.top : destRect.top;
		}

		// right
		rect = { scalingRect.right, hostPos.y, LONG_MAX, hostPos.y + 1 };
		if (!MonitorFromRect(&rect, MONITOR_DEFAULTTONULL)) {
			clips.right = _isUnderCapture ? srcRect.right : destRect.right;
		}

		// bottom
		rect = { hostPos.x, scalingRect.bottom, hostPos.x + 1, LONG_MAX };
		if (!MonitorFromRect(&rect, MONITOR_DEFAULTTONULL)) {
			clips.bottom = _isUnderCapture ? srcRect.bottom : destRect.bottom;
		}

		_SetClipCursor(clips);
	} else if (_lastClip.left != std::numeric_limits<LONG>::max()) {
		_RestoreClipCursor();
		_lastClip = { std::numeric_limits<LONG>::max() };
	}

	// SetCursorPos 应在 ClipCursor 之后，否则会受到上一次 ClipCursor 的影响
	if (cursorPos != originCursorPos) {
		ReliableSetCursorPos(cursorPos);
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

	SIZE srcFrameSize = Win32Utils::GetSizeOfRect(srcRect);
	SIZE outputSize = Win32Utils::GetSizeOfRect(destRect);

	if (ScalingWindow::Get().Options().IsAdjustCursorSpeed()) {
		_AdjustCursorSpeed();
	}

	// 移动光标位置

	// 跳过黑边
	cursorPos.x = std::clamp(cursorPos.x, destRect.left, destRect.right - 1);
	cursorPos.y = std::clamp(cursorPos.y, destRect.top, destRect.bottom - 1);

	cursorPos = ScalingToSrc(cursorPos);

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

	POINT newCursorPos = SrcToScaling(cursorPos);

	if (onDestroy || MonitorFromPoint(newCursorPos, MONITOR_DEFAULTTONULL)) {
		cursorPos = newCursorPos;

		if (ScalingWindow::Get().Options().IsAdjustCursorSpeed()) {
			SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)_originCursorSpeed, 0);
		}
		
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
	RECT curClip;
	GetClipCursor(&curClip);

	// 如果当前光标裁剪区域与我们之前设置的不同，肯定是其他程序更改了，记录此原始裁剪区域
	// 用于后续计算和还原。
	// 如果我们没有裁剪光标区域，则 _lastClip.left == std::numeric_limits<LONG>::max()，
	// curClip != _lastClip 始终为真。
	// 但如果其他程序恰好设置了和我们相同的裁剪区域怎么办？
	if (curClip != _lastClip) {
		_originClip = curClip;
	}

	// 为了尊重原始裁剪区域，计算和我们的裁剪区域的交集。除非两个区域不相交或光标在叠加层上
	RECT targetClip;
	if (_isOnOverlay || !IntersectRect(&targetClip, &_originClip, &clipRect)) {
		// 不相交则不再尊重原始裁剪区域，这不太可能发生
		targetClip = clipRect;
	}

	// 裁剪区域变化了才调用 ClipCursor。每次调用 ClipCursor 都会向前台窗口发送 WM_MOUSEMOVE
	// 消息，一些程序无法正确处理。见 GH#920
	if (targetClip != _lastClip || is3DGameMode) {
		ClipCursor(&targetClip);
		_lastClip = targetClip;
	}
}

void CursorManager::_RestoreClipCursor() const noexcept {
	RECT curClip;
	GetClipCursor(&curClip);

	// 如果 curClip != _lastClip，则其他程序已经更改光标裁剪区域，我们放弃更改
	if (curClip == _lastClip && curClip != _originClip) {
		ClipCursor(&_originClip);
	}
}

}
