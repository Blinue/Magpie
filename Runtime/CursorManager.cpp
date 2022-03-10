#include "pch.h"
#include "CursorManager.h"
#include "App.h"
#include "FrameSourceBase.h"
#include "Renderer.h"
#include "Logger.h"
#include "Utils.h"
#include "DeviceResources.h"


CursorManager::~CursorManager() {
	if (App::Get().IsMultiMonitorMode()) {
		if (_isUnderCapture) {
			POINT pt{};
			if (!::GetCursorPos(&pt)) {
				Logger::Get().Win32Error("GetCursorPos 失败");
			}
			_StopCapture(pt);
		}
	} else if (!App::Get().IsBreakpointMode()) {
		// CursorDrawer 析构时计时器已销毁
		ClipCursor(nullptr);
		if (App::Get().IsAdjustCursorSpeed()) {
			SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)_cursorSpeed, 0);
		}

		MagShowSystemCursor(TRUE);
	}

	Logger::Get().Info("CursorDrawer 已析构");
}

bool CursorManager::Initialize() {
	if (!App::Get().IsMultiMonitorMode() && !App::Get().IsBreakpointMode()) {
		// 非多屏幕模式下，限制光标在窗口内
		const RECT& srcFrameRect = App::Get().GetFrameSource().GetSrcFrameRect();
		if (!ClipCursor(&srcFrameRect)) {
			Logger::Get().Win32Error("ClipCursor 失败");
		}

		if (App::Get().IsAdjustCursorSpeed()) {
			// 设置鼠标移动速度
			if (SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_cursorSpeed, 0)) {
				SIZE srcFrameSize = Utils::GetSizeOfRect(App::Get().GetFrameSource().GetSrcFrameRect());
				SIZE outputSize = Utils::GetSizeOfRect(App::Get().GetRenderer().GetOutputRect());

				double speedScale = ((double)outputSize.cx / srcFrameSize.cx + (double)outputSize.cy / srcFrameSize.cy) / 2;
				long newSpeed = std::clamp(lround(_cursorSpeed / speedScale), 1L, 20L);

				if (!SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)newSpeed, 0)) {
					Logger::Get().Win32Error("设置光标移速失败");
				}
			} else {
				Logger::Get().Win32Error("获取光标移速失败");
			}

			Logger::Get().Info("已调整光标移速");
		}

		if (!MagShowSystemCursor(FALSE)) {
			Logger::Get().Win32Error("MagShowSystemCursor 失败");
		}
	}

	Logger::Get().Info("CursorManager 初始化完成");
	return true;
}

void CursorManager::BeginFrame() {
	// 多屏幕模式下光标可以在屏幕间移动
	if (App::Get().IsMultiMonitorMode()) {
		// _DynamicClip 根据当前光标位置的四个方向有无屏幕来确定应该在哪些方向限制光标，但这无法
		// 处理屏幕之间存在间隙的情况。解决办法是 _StopCapture 只在目标位置存在屏幕时才取消捕获，
		// 当光标试图移动到间隙中时将被挡住。如果光标的速度足以跨越间隙，则它依然可以在屏幕间移动。

		POINT cursorPt;
		if (!::GetCursorPos(&cursorPt)) {
			Logger::Get().Win32Error("GetCursorPos 失败");
			return;
		}

		if (_isUnderCapture) {
			const RECT& srcFrameRect = App::Get().GetFrameSource().GetSrcFrameRect();

			if (PtInRect(&srcFrameRect, cursorPt)) {
				_DynamicClip(cursorPt);
			} else {
				_StopCapture(cursorPt);
			}
		} else {
			const RECT& hostRect = App::Get().GetHostWndRect();

			if (PtInRect(&hostRect, cursorPt)) {
				_StartCapture(cursorPt);
				_DynamicClip(cursorPt);
			}
		}
	}

	if (App::Get().IsNoCursor()) {
		// 不绘制光标
		_curCursor = NULL;
		return;
	}

	if (App::Get().IsMultiMonitorMode()) {
		// 多屏幕模式下不处于捕获状态则不绘制光标
		if (!_isUnderCapture) {
			_curCursor = NULL;
			return;
		}
	} else if (!App::Get().IsBreakpointMode() && App::Get().IsConfineCursorIn3DGames()) {
		// 开启“在 3D 游戏中限制光标”则每帧都限制一次光标
		ClipCursor(&App::Get().GetFrameSource().GetSrcFrameRect());
	}

	CURSORINFO ci{};
	ci.cbSize = sizeof(ci);
	if (!::GetCursorInfo(&ci)) {
		Logger::Get().Win32Error("GetCursorInfo 失败");
		return;
	}

	if (!ci.hCursor || ci.flags != CURSOR_SHOWING) {
		_curCursor = NULL;
		return;
	}

	if (!_ResolveCursor(ci.hCursor, false)) {
		Logger::Get().Error("解析光标失败");
		_curCursor = NULL;
		return;
	}

	const RECT& srcFrameRect = App::Get().GetFrameSource().GetSrcFrameRect();
	const RECT& outputRect = App::Get().GetRenderer().GetOutputRect();
	const RECT& virtualOutputRect = App::Get().GetRenderer().GetVirtualOutputRect();

	double pos = double(ci.ptScreenPos.x - srcFrameRect.left) / (srcFrameRect.right - srcFrameRect.left);
	_curCursorPos.x = std::lround(pos * (virtualOutputRect.right - virtualOutputRect.left)) + virtualOutputRect.left;

	pos = double(ci.ptScreenPos.y - srcFrameRect.top) / (srcFrameRect.bottom - srcFrameRect.top);
	_curCursorPos.y = std::lround(pos * (virtualOutputRect.bottom - virtualOutputRect.top)) + virtualOutputRect.top;

	POINT cursorLeftTop = {
		_curCursorPos.x - _curCursorInfo->hotSpot.x,
		_curCursorPos.y - _curCursorInfo->hotSpot.y
	};

	if (cursorLeftTop.x > outputRect.right
		|| cursorLeftTop.y > outputRect.bottom
		|| cursorLeftTop.x + _curCursorInfo->size.cx < outputRect.left
		|| cursorLeftTop.y + _curCursorInfo->size.cy < outputRect.top
	) {
		// 光标的渲染位置不在屏幕内
		_curCursor = NULL;
		return;
	}

	_curCursor = ci.hCursor;
}

bool CursorManager::GetCursorTexture(ID3D11Texture2D** texture, CursorManager::CursorType& cursorType) {
	if (_curCursorInfo->texture) {
		*texture = _curCursorInfo->texture.get();
		cursorType = _curCursorInfo->type;
		return true;
	}

	if (!_ResolveCursor(_curCursor, true)) {
		return false;
	} else {
		const char* cursorTypes[] = { "Color", "Masked Color", "Monochrome" };
		Logger::Get().Info(fmt::format("已解析光标：{}\n\t类型：{}",
			(void*)_curCursor, cursorTypes[(int)_curCursorInfo->type]));
	}

	*texture = _curCursorInfo->texture.get();
	cursorType = _curCursorInfo->type;
	return true;
}

void CursorManager::_StartCapture(POINT cursorPt) {
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
	if (!MagShowSystemCursor(FALSE)) {
		Logger::Get().Win32Error("MagShowSystemCursor 失败");
	}

	const RECT& srcFrameRect = App::Get().GetFrameSource().GetSrcFrameRect();
	const RECT& hostRect = App::Get().GetHostWndRect();
	const RECT& outputRect = App::Get().GetRenderer().GetOutputRect();

	SIZE srcFrameSize = Utils::GetSizeOfRect(srcFrameRect);
	SIZE outputSize = Utils::GetSizeOfRect(outputRect);

	if (App::Get().IsAdjustCursorSpeed()) {
		// 设置鼠标移动速度
		if (SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_cursorSpeed, 0)) {
			double speedScale = ((double)outputSize.cx / srcFrameSize.cx + (double)outputSize.cy / srcFrameSize.cy) / 2;
			long newSpeed = std::clamp(lround(_cursorSpeed / speedScale), 1L, 20L);

			if (!SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)newSpeed, 0)) {
				Logger::Get().Win32Error("设置光标移速失败");
			}
		} else {
			Logger::Get().Win32Error("获取光标移速失败");
		}
	}

	// 移动光标位置
	
	// 跳过黑边
	cursorPt.x = std::clamp(cursorPt.x, hostRect.left + outputRect.left, hostRect.left + outputRect.right - 1);
	cursorPt.y = std::clamp(cursorPt.y, hostRect.top + outputRect.top, hostRect.top + outputRect.bottom - 1);

	// 从全屏窗口映射到源窗口
	double posX = double(cursorPt.x - hostRect.left - outputRect.left) / (outputRect.right - outputRect.left);
	double posY = double(cursorPt.y - hostRect.top - outputRect.top) / (outputRect.bottom - outputRect.top);

	posX = posX * srcFrameSize.cx + srcFrameRect.left;
	posY = posY * srcFrameSize.cy + srcFrameRect.top;

	posX = std::clamp<double>(posX, srcFrameRect.left, srcFrameRect.right - 1);
	posY = std::clamp<double>(posY, srcFrameRect.top, srcFrameRect.bottom - 1);

	SetCursorPos(std::lround(posX), std::lround(posY));

	_isUnderCapture = true;
}

void CursorManager::_StopCapture(POINT cursorPt) {
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

	const RECT& srcFrameRect = App::Get().GetFrameSource().GetSrcFrameRect();
	const RECT& hostRect = App::Get().GetHostWndRect();
	const RECT& outputRect = App::Get().GetRenderer().GetOutputRect();

	POINT newCursorPt{};

	if (cursorPt.x >= srcFrameRect.right) {
		newCursorPt.x = hostRect.right + cursorPt.x - srcFrameRect.right + 1;
	} else if (cursorPt.x < srcFrameRect.left) {
		newCursorPt.x = hostRect.left + cursorPt.x - srcFrameRect.left;
	} else {
		double pos = double(cursorPt.x - srcFrameRect.left) / (srcFrameRect.right - srcFrameRect.left);
		newCursorPt.x = std::lround(pos * (outputRect.right - outputRect.left)) + outputRect.left + hostRect.left;
	}

	if (cursorPt.y >= srcFrameRect.bottom) {
		newCursorPt.y = hostRect.bottom + cursorPt.y - srcFrameRect.bottom + 1;
	} else if (cursorPt.y < srcFrameRect.top) {
		newCursorPt.y = hostRect.top + cursorPt.y - srcFrameRect.top;
	} else {
		double pos = double(cursorPt.y - srcFrameRect.top) / (srcFrameRect.bottom - srcFrameRect.top);
		newCursorPt.y = std::lround(pos * (outputRect.bottom - outputRect.top)) + outputRect.top + hostRect.top;
	}

	if (MonitorFromPoint(newCursorPt, MONITOR_DEFAULTTONULL)) {
		ClipCursor(nullptr);
		_curClips = {};

		SetCursorPos(newCursorPt.x, newCursorPt.y);

		if (App::Get().IsAdjustCursorSpeed()) {
			SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)_cursorSpeed, 0);
		}

		if (!MagShowSystemCursor(TRUE)) {
			Logger::Get().Error("MagShowSystemCursor 失败");
		}
		// WGC 捕获模式会随机使 MagShowSystemCursor(TRUE) 失效，重新加载光标可以解决这个问题
		SystemParametersInfo(SPI_SETCURSORS, 0, 0, 0);

		_isUnderCapture = false;
	} else {
		// 目标位置不存在屏幕，则将光标限制在源窗口内
		SetCursorPos(
			std::clamp(cursorPt.x, srcFrameRect.left, srcFrameRect.right - 1),
			std::clamp(cursorPt.y, srcFrameRect.top, srcFrameRect.bottom - 1)
		);
	}
}

void CursorManager::_DynamicClip(POINT cursorPt) {
	const RECT& srcFrameRect = App::Get().GetFrameSource().GetSrcFrameRect();
	const RECT& hostRect = App::Get().GetHostWndRect();
	const RECT& outputRect = App::Get().GetRenderer().GetOutputRect();

	POINT hostPt{};

	double t = double(cursorPt.x - srcFrameRect.left) / (srcFrameRect.right - srcFrameRect.left);
	hostPt.x = std::lround(t * (outputRect.right - outputRect.left)) + outputRect.left + hostRect.left;

	t = double(cursorPt.y - srcFrameRect.top) / (srcFrameRect.bottom - srcFrameRect.top);
	hostPt.y = std::lround(t * (outputRect.bottom - outputRect.top)) + outputRect.top + hostRect.top;

	std::array<bool, 4> clips{};
	// left
	RECT rect{ LONG_MIN, hostPt.y, hostRect.left, hostPt.y + 1 };
	clips[0] = !MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);
	// top
	rect = { hostPt.x, LONG_MIN, hostPt.x + 1,hostRect.top };
	clips[1] = !MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);
	// right
	rect = { hostRect.right, hostPt.y, LONG_MAX, hostPt.y + 1 };
	clips[2] = !MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);
	// bottom
	rect = { hostPt.x, hostRect.bottom, hostPt.x + 1, LONG_MAX };
	clips[3] = !MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);

	// 开启“在 3D 游戏中限制光标”则每帧都限制一次光标
	if (App::Get().IsConfineCursorIn3DGames() || clips != _curClips) {
		_curClips = clips;

		RECT clipRect = {
			clips[0] ? srcFrameRect.left : LONG_MIN,
			clips[1] ? srcFrameRect.top : LONG_MIN,
			clips[2] ? srcFrameRect.right : LONG_MAX,
			clips[3] ? srcFrameRect.bottom : LONG_MAX
		};
		ClipCursor(&clipRect);
	}
}

bool CursorManager::_ResolveCursor(HCURSOR hCursor, bool resolveTexture) {
	auto it = _cursorInfos.find(hCursor);
	if (it != _cursorInfos.end() && (!resolveTexture || (resolveTexture && _curCursorInfo->texture))) {
		_curCursorInfo = &it->second;
		return true;
	}

	ICONINFO ii{};
	if (!GetIconInfo(hCursor, &ii)) {
		Logger::Get().Win32Error("GetIconInfo 失败");
		return false;
	}

	Utils::ScopeExit se([&ii]() {
		if (ii.hbmColor) {
			DeleteBitmap(ii.hbmColor);
		}
		DeleteBitmap(ii.hbmMask);
	});

	BITMAP bmp{};
	if (!GetObject(ii.hbmMask, sizeof(bmp), &bmp)) {
		Logger::Get().Win32Error("GetObject 失败");
		return false;
	}

	_curCursorInfo = it == _cursorInfos.end() ? &_cursorInfos[hCursor] : &it->second;

	_curCursorInfo->hotSpot = { (LONG)ii.xHotspot, (LONG)ii.yHotspot };
	// 单色光标的 hbmMask 高度为实际高度的两倍
	_curCursorInfo->size = { bmp.bmWidth, ii.hbmColor ? bmp.bmHeight : bmp.bmHeight / 2 };

	if (!resolveTexture) {
		return true;
	}

	auto& dr = App::Get().GetDeviceResources();

	BITMAPINFO bi{};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = bmp.bmWidth;
	bi.bmiHeader.biHeight = -bmp.bmHeight;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biSizeImage = bmp.bmWidth * bmp.bmHeight * 4;

	if (ii.hbmColor == NULL) {
		// 单色光标
		_curCursorInfo->type = CursorType::Monochrome;

		std::vector<BYTE> pixels(bi.bmiHeader.biSizeImage);
		HDC hdc = GetDC(NULL);
		if (GetDIBits(hdc, ii.hbmMask, 0, bmp.bmHeight, &pixels[0], &bi, DIB_RGB_COLORS) != bmp.bmHeight) {
			Logger::Get().Win32Error("GetDIBits 失败");
			ReleaseDC(NULL, hdc);
			return false;
		}
		ReleaseDC(NULL, hdc);

		// 红色通道是 AND 掩码，绿色通道是 XOR 掩码
		// 这里将下半部分的 XOR 掩码复制到上半部分的绿色通道中
		const int halfSize = bi.bmiHeader.biSizeImage / 8;
		BYTE* upPtr = &pixels[1];
		BYTE* downPtr = &pixels[static_cast<size_t>(halfSize) * 4];
		for (int i = 0; i < halfSize; ++i) {
			*upPtr = *downPtr;

			upPtr += 4;
			downPtr += 4;
		}

		D3D11_SUBRESOURCE_DATA initData{};
		initData.pSysMem = &pixels[0];
		initData.SysMemPitch = bmp.bmWidth * 4;

		_curCursorInfo->texture = dr.CreateTexture2D(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			bmp.bmWidth,
			bmp.bmHeight / 2,
			D3D11_BIND_SHADER_RESOURCE,
			D3D11_USAGE_IMMUTABLE,
			0,
			&initData
		);
		if (!_curCursorInfo->texture) {
			Logger::Get().Error("创建纹理失败");
			return false;
		}

		return true;
	}

	std::vector<BYTE> pixels(bi.bmiHeader.biSizeImage);
	HDC hdc = GetDC(NULL);
	if (GetDIBits(hdc, ii.hbmColor, 0, bmp.bmHeight, &pixels[0], &bi, DIB_RGB_COLORS) != bmp.bmHeight) {
		Logger::Get().Win32Error("GetDIBits 失败");
		ReleaseDC(NULL, hdc);
		return false;
	}
	ReleaseDC(NULL, hdc);

	// 若颜色掩码有 A 通道，则是彩色光标，否则是彩色掩码光标
	bool hasAlpha = false;
	for (UINT i = 3; i < bi.bmiHeader.biSizeImage; i += 4) {
		if (pixels[i] != 0) {
			hasAlpha = true;
			break;
		}
	}

	if (hasAlpha) {
		// 彩色光标
		_curCursorInfo->type = CursorType::Color;

		for (size_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
			// 预乘 Alpha 通道
			double alpha = pixels[i + 3] / 255.0f;

			BYTE b = (BYTE)std::lround(pixels[i] * alpha);
			pixels[i] = (BYTE)std::lround(pixels[i + 2] * alpha);
			pixels[i + 1] = (BYTE)std::lround(pixels[i + 1] * alpha);
			pixels[i + 2] = b;
			
			pixels[i + 3] = 255 - pixels[i + 3];
		}
	} else {
		// 彩色掩码光标
		_curCursorInfo->type = CursorType::MaskedColor;

		std::vector<BYTE> maskPixels(bi.bmiHeader.biSizeImage);
		HDC hdc = GetDC(NULL);
		if (GetDIBits(hdc, ii.hbmMask, 0, bmp.bmHeight, &maskPixels[0], &bi, DIB_RGB_COLORS) != bmp.bmHeight) {
			Logger::Get().Win32Error("GetDIBits 失败");
			ReleaseDC(NULL, hdc);
			return false;
		}
		ReleaseDC(NULL, hdc);

		// 将 XOR 掩码复制到透明通道中
		for (size_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
			std::swap(pixels[i], pixels[i + 2]);
			pixels[i + 3] = maskPixels[i];
		}
	}

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = &pixels[0];
	initData.SysMemPitch = bmp.bmWidth * 4;

	_curCursorInfo->texture = dr.CreateTexture2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		bmp.bmWidth,
		bmp.bmHeight,
		D3D11_BIND_SHADER_RESOURCE,
		D3D11_USAGE_IMMUTABLE,
		0,
		&initData
	);
	if (!_curCursorInfo->texture) {
		Logger::Get().Error("创建纹理失败");
		return false;
	}

	return true;
}
