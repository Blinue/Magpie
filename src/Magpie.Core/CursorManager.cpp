#include "pch.h"
#include "CursorManager.h"
#include "Logger.h"
#include <magnification.h>
#include "Win32Utils.h"
#include "ScalingOptions.h"

#pragma comment(lib, "Magnification.lib")

namespace Magpie::Core {

CursorManager::~CursorManager() noexcept {
	if (_curClips != RECT{}) {
		ClipCursor(nullptr);
	}

	if (_isUnderCapture) {
		POINT pt{};
		if (!::GetCursorPos(&pt)) {
			Logger::Get().Win32Error("GetCursorPos 失败");
		}
		_curClips = {};
		_StopCapture(pt, true);
	}
}

bool CursorManager::Initialize(
	HWND hwndSrc,
	HWND hwndScaling,
	const RECT& srcRect,
	const RECT& scalingWndRect,
	const RECT& destRect,
	const ScalingOptions& options
) noexcept {
	_hwndSrc = hwndSrc;
	_hwndScaling = hwndScaling;
	_srcRect = srcRect;
	_scalingWndRect = scalingWndRect;
	_destRect = destRect;
	_isAdjustCursorSpeed = options.IsAdjustCursorSpeed();
	_isDebugMode = options.IsDebugMode();
	_is3DGameMode = options.Is3DGameMode();
	_isDrawCursor = options.IsDrawCursor();

	return true;
}

std::pair<HCURSOR, POINT> CursorManager::Update() noexcept {
	_UpdateCursorClip();

	std::pair<HCURSOR, POINT> result{NULL,
		{ std::numeric_limits<LONG>::max(), std::numeric_limits<LONG>::max() }};

	if (!_isDrawCursor || !_isUnderCapture) {
		// 不绘制光标
		return result;
	}

	if (_is3DGameMode) {
		HWND hwndFore = GetForegroundWindow();
		if (hwndFore != _hwndScaling && hwndFore != _hwndSrc) {
			return result;
		}
	}

	CURSORINFO ci{ sizeof(CURSORINFO) };
	if (!GetCursorInfo(&ci)) {
		Logger::Get().Win32Error("GetCursorPos 失败");
		return result;
	}

	if (ci.hCursor && ci.flags != CURSOR_SHOWING) {
		return result;
	}

	result.first = ci.hCursor;
	result.second = _SrcToScaling(ci.ptScreenPos, false);

	return result;
}

void CursorManager::_ShowSystemCursor(bool show) {
	static void (WINAPI* const showSystemCursor)(BOOL bShow) = []()->void(WINAPI*)(BOOL) {
		HMODULE lib = LoadLibrary(L"user32.dll");
		if (!lib) {
			return nullptr;
		}

		return (void(WINAPI*)(BOOL))GetProcAddress(lib, "ShowSystemCursor");
	}();

	if (showSystemCursor) {
		showSystemCursor((BOOL)show);
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
		}
	}

	_cursorVisibilityChangedEvent(show);
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

	const SIZE srcSize = Win32Utils::GetSizeOfRect(_srcRect);
	const SIZE destSize = Win32Utils::GetSizeOfRect(_destRect);
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

		// 跳过不可见的窗口
		if (!(GetWindowLongPtr(hWnd, GWL_STYLE) & WS_VISIBLE)) {
			return TRUE;
		}

		// 跳过透明窗口
		if (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT) {
			return TRUE;
		}

		// 跳过被冻结的窗口
		UINT isCloaked{};
		DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &isCloaked, sizeof(isCloaked));
		if (isCloaked != 0) {
			return TRUE;
		}

		// 对于分层窗口（Layered Window），没有公开的 API 可以检测某个像素是否透明。
		// ChildWindowFromPointEx 是一个替代方案，当命中透明像素时它将返回 NULL。
		// Windows 内部有 LayerHitTest (https://github.com/tongzx/nt5src/blob/daad8a087a4e75422ec96b7911f1df4669989611/Source/XPSP1/NT/windows/core/ntuser/kernel/winwhere.c#L21) 方法用于对分层窗口执行命中测试，虽然它没有被公开，但 ChildWindowFromPointEx 使用了它
		// 在比 Magpie 权限更高的窗口上使用会失败，失败则假设不是分层窗口
		POINT clientPt = data.pt;
		ScreenToClient(hWnd, &clientPt);
		SetLastError(0);
		if (!ChildWindowFromPointEx(hWnd, clientPt, CWP_SKIPDISABLED | CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT)) {
			if (GetLastError() == 0) {
				// 命中了透明像素
				return TRUE;
			}

			// 源窗口的权限比 Magpie 更高，回落到 GetWindowRect
			RECT windowRect{};
			if (!GetWindowRect(hWnd, &windowRect) || !PtInRect(&windowRect, data.pt)) {
				return TRUE;
			}
		}

		data.result = hWnd;
		return FALSE;
	}, (LPARAM)&data);

	return data.result;
}

void CursorManager::_UpdateCursorClip() noexcept {
	// 优先级：
	// 1. 断点模式：不限制，捕获/取消捕获，支持 UI
	// 2. 在 3D 游戏中限制光标：每帧都限制一次，不退出捕获，因此无法使用 UI，不支持多屏幕
	// 3. 常规：根据多屏幕限制光标，捕获/取消捕获，支持 UI 和多屏幕

	if (!_isDebugMode && _is3DGameMode) {
		// 开启“在 3D 游戏中限制光标”则每帧都限制一次光标
		_curClips = _srcRect;
		ClipCursor(&_srcRect);
		return;
	}

	const SIZE srcSize = Win32Utils::GetSizeOfRect(_srcRect);
	const SIZE destSize = Win32Utils::GetSizeOfRect(_destRect);
	
	INT_PTR style = GetWindowLongPtr(_hwndScaling, GWL_EXSTYLE);

	POINT cursorPos;
	if (!GetCursorPos(&cursorPos)) {
		Logger::Get().Win32Error("GetCursorPos 失败");
		return;
	}

	if (_isUnderCapture) {
		///////////////////////////////////////////////////////////
		// 
		// 处于捕获状态
		// --------------------------------------------------------
		//                  |  虚拟位置被遮挡  |    虚拟位置未被遮挡
		// --------------------------------------------------------
		// 实际位置被遮挡    |    退出捕获     | 退出捕获，主窗口不透明
		// --------------------------------------------------------
		// 实际位置未被遮挡  |    退出捕获      |        无操作
		// --------------------------------------------------------
		// 
		///////////////////////////////////////////////////////////

		HWND hwndCur = WindowFromPoint(_hwndScaling, _scalingWndRect, _SrcToScaling(cursorPos, true), false);

		if (hwndCur != _hwndScaling) {
			// 主窗口被遮挡
			if (style | WS_EX_TRANSPARENT) {
				SetWindowLongPtr(_hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
			}

			_StopCapture(cursorPos);
		} else {
			// 主窗口未被遮挡
			bool stopCapture = false;

			if (!stopCapture) {
				// 判断源窗口是否被遮挡
				hwndCur = WindowFromPoint(_hwndScaling, _scalingWndRect, cursorPos, true);
				stopCapture = hwndCur != _hwndSrc && (!IsChild(_hwndSrc, hwndCur) || !((GetWindowStyle(hwndCur) & WS_CHILD)));
			}

			if (stopCapture) {
				if (style | WS_EX_TRANSPARENT) {
					SetWindowLongPtr(_hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
				}

				_StopCapture(cursorPos);
			} else {
				if (!(style & WS_EX_TRANSPARENT)) {
					SetWindowLongPtr(_hwndScaling, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);
				}
			}
		}
	} else {
		/////////////////////////////////////////////////////////
		// 
		// 未处于捕获状态
		// -----------------------------------------------------
		//					|  虚拟位置被遮挡	|  虚拟位置未被遮挡
		// ------------------------------------------------------
		// 实际位置被遮挡		|    无操作		|    主窗口不透明
		// ------------------------------------------------------
		// 实际位置未被遮挡	|    无操作		| 开始捕获，主窗口透明
		// ------------------------------------------------------
		// 
		/////////////////////////////////////////////////////////

		HWND hwndCur = WindowFromPoint(_hwndScaling, _scalingWndRect, cursorPos, false);

		if (hwndCur == _hwndScaling) {
			// 主窗口未被遮挡
			POINT newCursorPos = _ScalingToSrc(cursorPos);

			if (!PtInRect(&_srcRect, newCursorPos)) {
				// 跳过黑边
				if (false) {
					// 从内部移到外部
					// 此时有 UI 贴边
					/*if (newCursorPos.x >= _srcRect.right) {
						cursorPos.x += _scalingWndRect.right - _scalingWndRect.left - outputRect.right;
					} else if (newCursorPos.x < _srcRect.left) {
						cursorPos.x -= outputRect.left;
					}

					if (newCursorPos.y >= _srcRect.bottom) {
						cursorPos.y += _scalingWndRect.bottom - _scalingWndRect.top - outputRect.bottom;
					} else if (newCursorPos.y < _srcRect.top) {
						cursorPos.y -= outputRect.top;
					}

					if (MonitorFromPoint(cursorPos, MONITOR_DEFAULTTONULL)) {
						SetCursorPos(cursorPos.x, cursorPos.y);
					} else {
						// 目标位置不存在屏幕，则将光标限制在输出区域内
						SetCursorPos(
							std::clamp(cursorPos.x, _scalingWndRect.left + outputRect.left, _scalingWndRect.left + outputRect.right - 1),
							std::clamp(cursorPos.y, _scalingWndRect.top + outputRect.top, _scalingWndRect.top + outputRect.bottom - 1)
						);
					}*/
				} else {
					// 从外部移到内部

					POINT clampedPos = {
						std::clamp(cursorPos.x, _destRect.left, _destRect.right - 1),
						std::clamp(cursorPos.y, _destRect.top, _destRect.bottom - 1)
					};

					if (WindowFromPoint(_hwndScaling, _scalingWndRect, clampedPos, false) == _hwndScaling) {
						if (!(style & WS_EX_TRANSPARENT)) {
							SetWindowLongPtr(_hwndScaling, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);
						}

						_StartCapture(cursorPos);
					} else {
						// 要跳跃的位置被遮挡
						if (style | WS_EX_TRANSPARENT) {
							SetWindowLongPtr(_hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
						}
					}
				}
			} else {
				bool startCapture = true;

				if (startCapture) {
					// 判断源窗口是否被遮挡
					hwndCur = WindowFromPoint(_hwndScaling, _scalingWndRect, newCursorPos, true);
					startCapture = hwndCur == _hwndSrc || ((IsChild(_hwndSrc, hwndCur) && (GetWindowStyle(hwndCur) & WS_CHILD)));
				}

				if (startCapture) {
					if (!(style & WS_EX_TRANSPARENT)) {
						SetWindowLongPtr(_hwndScaling, GWL_EXSTYLE, style | WS_EX_TRANSPARENT);
					}

					_StartCapture(cursorPos);
				} else {
					if (style | WS_EX_TRANSPARENT) {
						SetWindowLongPtr(_hwndScaling, GWL_EXSTYLE, style & ~WS_EX_TRANSPARENT);
					}
				}
			}
		}
	}

	if (_isDebugMode) {
		return;
	}

	if (!false && !_isUnderCapture) {
		return;
	}

	// 根据当前光标位置的四个方向有无屏幕来确定应该在哪些方向限制光标，但这无法
	// 处理屏幕之间存在间隙的情况。解决办法是 _StopCapture 只在目标位置存在屏幕时才取消捕获，
	// 当光标试图移动到间隙中时将被挡住。如果光标的速度足以跨越间隙，则它依然可以在屏幕间移动。
	::GetCursorPos(&cursorPos);
	POINT hostPos = false ? cursorPos : _SrcToScaling(cursorPos, true);

	RECT clips{ LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX };

	// left
	RECT rect{ LONG_MIN, hostPos.y, _scalingWndRect.left, hostPos.y + 1 };
	if (!MonitorFromRect(&rect, MONITOR_DEFAULTTONULL)) {
		clips.left = false ? _destRect.left : _srcRect.left;
	}

	// top
	rect = { hostPos.x, LONG_MIN, hostPos.x + 1,_scalingWndRect.top };
	if (!MonitorFromRect(&rect, MONITOR_DEFAULTTONULL)) {
		clips.top = false ? _destRect.top : _srcRect.top;
	}

	// right
	rect = { _scalingWndRect.right, hostPos.y, LONG_MAX, hostPos.y + 1 };
	if (!MonitorFromRect(&rect, MONITOR_DEFAULTTONULL)) {
		clips.right = false ? _destRect.right : _srcRect.right;
	}

	// bottom
	rect = { hostPos.x, _scalingWndRect.bottom, hostPos.x + 1, LONG_MAX };
	if (!MonitorFromRect(&rect, MONITOR_DEFAULTTONULL)) {
		clips.bottom = false ? _destRect.bottom : _srcRect.bottom;
	}

	if (clips != _curClips) {
		_curClips = clips;
		ClipCursor(&clips);
	}
}

void CursorManager::_StartCapture(POINT cursorPos) noexcept {
	if (_isUnderCapture) {
		return;
	}

	// 在以下情况下进入捕获状态：
	// 1. 当前未捕获
	// 2. 光标进入全屏区域
	// 
	// 进入捕获状态时：
	// 1. 调整光标速度，全局隐藏光标
	// 2. 将光标移到源窗口的对应位置
	//
	// 在有黑边的情况下自动将光标调整到画面内

	// 全局隐藏光标
	_ShowSystemCursor(false);

	SIZE srcFrameSize = Win32Utils::GetSizeOfRect(_srcRect);
	SIZE outputSize = Win32Utils::GetSizeOfRect(_destRect);

	if (_isAdjustCursorSpeed) {
		_AdjustCursorSpeed();
	}

	// 移动光标位置

	// 跳过黑边
	cursorPos.x = std::clamp(cursorPos.x, _destRect.left,  _destRect.right - 1);
	cursorPos.y = std::clamp(cursorPos.y, _destRect.top, _destRect.bottom - 1);

	POINT newCursorPos = _ScalingToSrc(cursorPos);
	SetCursorPos(newCursorPos.x, newCursorPos.y);

	_isUnderCapture = true;
}

void CursorManager::_StopCapture(POINT cursorPos, bool onDestroy) noexcept {
	if (!_isUnderCapture) {
		return;
	}

	if (_curClips != RECT{}) {
		_curClips = {};
		ClipCursor(nullptr);
	}

	// 在以下情况下离开捕获状态：
	// 1. 当前处于捕获状态
	// 2. 光标离开源窗口客户区
	// 3. 目标位置存在屏幕
	//
	// 离开捕获状态时
	// 1. 还原光标速度，全局显示光标
	// 2. 将光标移到全屏窗口外的对应位置
	//
	// 在有黑边的情况下自动将光标调整到全屏窗口外

	POINT newCursorPos = _SrcToScaling(cursorPos, true);

	if (onDestroy || MonitorFromPoint(newCursorPos, MONITOR_DEFAULTTONULL)) {
		SetCursorPos(newCursorPos.x, newCursorPos.y);

		if (_isAdjustCursorSpeed) {
			SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)_originCursorSpeed, 0);
		}

		_ShowSystemCursor(true);
		_isUnderCapture = false;
	} else {
		// 目标位置不存在屏幕，则将光标限制在源窗口内
		SetCursorPos(
			std::clamp(cursorPos.x, _srcRect.left, _srcRect.right - 1),
			std::clamp(cursorPos.y, _srcRect.top, _srcRect.bottom - 1)
		);
	}
}

// 将源窗口的光标位置映射到缩放后的光标位置
// 当光标位于源窗口之外，与源窗口的距离不会缩放
POINT CursorManager::_SrcToScaling(POINT pt, bool screenCoord) noexcept {
	POINT result;

	if (pt.x >= _srcRect.right) {
		result.x = _scalingWndRect.right + pt.x - _srcRect.right;
	} else if (pt.x < _srcRect.left) {
		result.x = _scalingWndRect.left + pt.x - _srcRect.left;
	} else {
		double pos = double(pt.x - _srcRect.left) / (_srcRect.right - _srcRect.left - 1);
		result.x = std::lround(pos * (_destRect.right - _destRect.left - 1)) + _destRect.left;
	}

	if (pt.y >= _srcRect.bottom) {
		result.y = _scalingWndRect.bottom + pt.y - _srcRect.bottom;
	} else if (pt.y < _srcRect.top) {
		result.y = _scalingWndRect.top + pt.y - _srcRect.top;
	} else {
		double pos = double(pt.y - _srcRect.top) / (_srcRect.bottom - _srcRect.top - 1);
		result.y = std::lround(pos * (_destRect.bottom - _destRect.top - 1)) + _destRect.top;
	}

	if (!screenCoord) {
		result.x -= _scalingWndRect.left;
		result.y -= _scalingWndRect.top;
	}

	return result;
}

POINT CursorManager::_ScalingToSrc(POINT pt) noexcept {
	const SIZE srcSize = Win32Utils::GetSizeOfRect(_srcRect);
	const SIZE destSize = Win32Utils::GetSizeOfRect(_destRect);

	POINT result = { _srcRect.left, _srcRect.top };

	if (pt.x >= _destRect.right) {
		result.x += srcSize.cx + pt.x - _destRect.right;
	} else if (pt.x < _destRect.left) {
		result.x += pt.x - _destRect.left;
	} else {
		double pos = double(pt.x - _destRect.left) / (destSize.cx - 1);
		result.x += std::lround(pos * (srcSize.cx - 1));
	}

	if (pt.y >= _destRect.bottom) {
		result.y += srcSize.cx + pt.y - _destRect.bottom;
	} else if (pt.y < _destRect.top) {
		result.y += pt.y - _destRect.top;
	} else {
		double pos = double(pt.y - _destRect.top) / (destSize.cy - 1);
		result.y += std::lround(pos * (srcSize.cy - 1));
	}

	return result;
}

}
