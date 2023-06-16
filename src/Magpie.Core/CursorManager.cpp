#include "pch.h"
#include "CursorManager.h"
#include "Logger.h"
#include <magnification.h>
#include "Win32Utils.h"
#include "ScalingOptions.h"

#pragma comment(lib, "Magnification.lib")

namespace Magpie::Core {

CursorManager::~CursorManager() noexcept {
}

bool CursorManager::Initialize(
	const RECT& srcRect,
	const RECT& scalingWndRect,
	const RECT& destRect,
	const ScalingOptions& options
) noexcept {
	_srcRect = srcRect;
	_scalingWndRect = scalingWndRect;
	_destRect = destRect;
	_isAdjustCursorSpeed = options.IsAdjustCursorSpeed();

	return true;
}

void CursorManager::Update() noexcept {
	_UpdateCursorClip();
}

void CursorManager::_AdjustCursorSpeed() noexcept {
}

void CursorManager::_UpdateCursorClip() noexcept {
}

static void ShowSystemCursor(bool show) noexcept {
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

	if (show) {
		/*MagApp::Get().Dispatcher().TryEnqueue([]() {
			if (!MagApp::Get().GetHwndHost()) {
				return;
			}

			// 修复有时不会立即显示光标的问题
			FrameSourceBase& frameSource = MagApp::Get().GetFrameSource();
			if (frameSource.GetName() == GraphicsCaptureFrameSource::NAME) {
				GraphicsCaptureFrameSource& wgc = (GraphicsCaptureFrameSource&)frameSource;
				// WGC 需要重启捕获
				// 没有用户报告这个问题，只在我的电脑上出现，可能和驱动有关
				wgc.StopCapture();
				wgc.StartCapture();
			} else {
				SystemParametersInfo(SPI_SETCURSORS, 0, 0, 0);
			}
		});*/
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
	ShowSystemCursor(false);

	SIZE srcFrameSize = Win32Utils::GetSizeOfRect(_srcRect);
	SIZE outputSize = Win32Utils::GetSizeOfRect(_destRect);

	if (_isAdjustCursorSpeed) {
		_AdjustCursorSpeed();
	}

	// 移动光标位置

	// 跳过黑边
	cursorPos.x = std::clamp(cursorPos.x, _scalingWndRect.left + _destRect.left, _scalingWndRect.left + _destRect.right - 1);
	cursorPos.y = std::clamp(cursorPos.y, _scalingWndRect.top + _destRect.top, _scalingWndRect.top + _destRect.bottom - 1);

	POINT newCursorPos = _ScalingToSrc(cursorPos);
	SetCursorPos(newCursorPos.x, newCursorPos.y);

	_isUnderCapture = true;
}

void CursorManager::_StopCapture(POINT cursorPos, bool onDestroy) noexcept {
}

// 将源窗口的光标位置映射到缩放后的光标位置
// 当光标位于源窗口之外，与源窗口的距离不会缩放
POINT CursorManager::_SrcToScaling(POINT pt, bool screenCoord) noexcept {
	POINT result;
	if (screenCoord) {
		result = { _scalingWndRect.left, _scalingWndRect.top };
	} else {
		result = {};
	}

	if (pt.x >= _srcRect.right) {
		result.x += _scalingWndRect.right - _scalingWndRect.left + pt.x - _srcRect.right;
	} else if (pt.x < _srcRect.left) {
		result.x += pt.x - _srcRect.left;
	} else {
		double pos = double(pt.x - _srcRect.left) / (_srcRect.right - _srcRect.left - 1);
		result.x += std::lround(pos * (_destRect.right - _destRect.left - 1)) + _destRect.left;
	}

	if (pt.y >= _srcRect.bottom) {
		result.y += _scalingWndRect.bottom - _scalingWndRect.top + pt.y - _srcRect.bottom;
	} else if (pt.y < _srcRect.top) {
		result.y += pt.y - _srcRect.top;
	} else {
		double pos = double(pt.y - _srcRect.top) / (_srcRect.bottom - _srcRect.top - 1);
		result.y += std::lround(pos * (_destRect.bottom - _destRect.top - 1)) + _destRect.top;
	}

	return result;
}

POINT CursorManager::_ScalingToSrc(POINT pt) noexcept {
	const SIZE srcSize = Win32Utils::GetSizeOfRect(_srcRect);
	const SIZE destSize = Win32Utils::GetSizeOfRect(_destRect);

	pt.x -= _scalingWndRect.left;
	pt.y -= _scalingWndRect.top;

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
