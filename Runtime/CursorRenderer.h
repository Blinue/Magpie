#pragma once
#include "pch.h"


// 处理光标的渲染
class CursorRenderer {
public:
	bool Initialize(ComPtr<ID3D11Texture2D> input, ComPtr<ID3D11Texture2D> output);

	~CursorRenderer();

	void Draw();

private:
	struct CursorInfo {
		HCURSOR handle = NULL;
		ComPtr<ID3D11Texture2D> texture = nullptr;
		int xHotSpot = 0;
		int yHotSpot = 0;
		int width = 0;
		int height = 0;
		bool isMonochrome = false;
	};
/*
private:
	

	CursorInfo* _cursorInfo = nullptr;
	D2D1_POINT_2L _targetScreenPos{};

public:
	void Render() {
		if (!_cursorInfo || _cursorInfo->isMonochrome) {
			return;
		}

		D2D1_RECT_F cursorRect = {
			FLOAT(_targetScreenPos.x),
			FLOAT(_targetScreenPos.y),
			FLOAT(_targetScreenPos.x + _cursorInfo->width),
			FLOAT(_targetScreenPos.y + _cursorInfo->height)
		};

		//Env::$instance->GetD2DDC()->DrawBitmap(_cursorInfo->bmp.Get(), &cursorRect);
	}

private:
	void _CalcCursorPos() {
		CURSORINFO ci{};
		ci.cbSize = sizeof(ci);
		//Debug::ThrowIfWin32Failed(
		GetCursorInfo(&ci);
		//	L"GetCursorInfo 失败"
		//);

		if (ci.hCursor == NULL || ci.flags != CURSOR_SHOWING) {
			_cursorInfo = nullptr;
			return;
		}

		auto it = _cursorMap.find(ci.hCursor);
		if (it != _cursorMap.end()) {
			_cursorInfo = &it->second;
		} else {
			try {
				// 未在映射中找到，创建新映射
				_ResolveCursor(ci.hCursor, ci.hCursor);

				_cursorInfo = &_cursorMap[ci.hCursor];
			} catch (...) {
				// 如果出错，不绘制光标
				_cursorInfo = nullptr;
				return;
			}
		}

		// 映射坐标
		// 鼠标坐标为整数，否则会出现模糊
		const RECT& srcClient = Env::$instance->GetSrcClient();
		const D2D_RECT_F& destRect = Env::$instance->GetDestRect();
		float scaleX = (destRect.right - destRect.left) / (srcClient.right - srcClient.left);
		float scaleY = (destRect.bottom - destRect.top) / (srcClient.bottom - srcClient.top);

		_targetScreenPos = {
			lroundf((ci.ptScreenPos.x - srcClient.left) * scaleX + destRect.left) - _cursorInfo->xHotSpot,
			lroundf((ci.ptScreenPos.y - srcClient.top) * scaleY + destRect.top) - _cursorInfo->yHotSpot
		};
	}

	ComPtr<ID2D1Bitmap> _CursorToD2DBitmap(HCURSOR hCursor) {
		assert(hCursor != NULL);

		
		return d2dBmpCursor;
	}

	

	void _ResolveCursor(HCURSOR hTptCursor, HCURSOR hCursor) {
		assert(hCursor != NULL);

		ICONINFO ii{};
		GetIconInfo(hCursor, &ii);

		CursorInfo cursorInfo;
		cursorInfo.handle = hCursor;
		cursorInfo.xHotSpot = ii.xHotspot;
		cursorInfo.yHotSpot = ii.yHotspot;
		cursorInfo.isMonochrome = (ii.hbmColor == NULL);

		if (!cursorInfo.isMonochrome) {
			SIZE size = _GetSizeOfHBmp(ii.hbmColor);
			cursorInfo.width = size.cx;
			cursorInfo.height = size.cy;

			cursorInfo.bmp = _CursorToD2DBitmap(hCursor);
		} else {
			SIZE size = _GetSizeOfHBmp(ii.hbmMask);
			cursorInfo.width = size.cx;
			cursorInfo.height = size.cy / 2;

			cursorInfo.bmp = _MonochromeToD2DBitmap(ii.hbmMask);
		}

		if (ii.hbmColor) {
			DeleteBitmap(ii.hbmColor);
		}
		DeleteBitmap(ii.hbmMask);

		_cursorMap[hTptCursor] = cursorInfo;
	}

	SIZE _GetSizeOfHBmp(HBITMAP hBmp) {
		BITMAP bmp{};
		GetObject(hBmp, sizeof(bmp), &bmp);
		return { bmp.bmWidth, bmp.bmHeight };
	}


	std::map<HCURSOR, CursorInfo> _cursorMap;
	*/
private:
	INT _cursorSpeed = 0;

	ComPtr<ID3D11DeviceContext4> _d3dDC = nullptr;
	ComPtr<ID3D11Device5> _d3dDevice = nullptr;
	ComPtr<ID3D11Texture2D> _input = nullptr;
	ComPtr<ID3D11Texture2D> _output = nullptr;

	ID3D11RenderTargetView* _outputRtv = nullptr;
	ID3D11ShaderResourceView* _inputSrv = nullptr;
	D3D11_VIEWPORT _vp{};

	ID3D11SamplerState* _sampler = nullptr;
	ComPtr<ID3D11PixelShader> _psShader = nullptr;
};
