#pragma once
#include "pch.h"


// 处理光标的渲染
class CursorRenderer {
public:
	bool Initialize(ComPtr<ID3D11Texture2D> input, ComPtr<ID3D11Texture2D> output);

	~CursorRenderer();

	void Draw();

private:
	struct _CursorInfo {
		ComPtr<ID3D11ShaderResourceView> masks = nullptr;
		int xHotSpot = 0;
		int yHotSpot = 0;
		int width = 0;
		int height = 0;
	};

	bool _ResolveCursor(HCURSOR hCursor, _CursorInfo& result) const;
/*
private:

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
	*/
private:
	INT _cursorSpeed = 0;
	RECT _destRect{};
	float _scaleX = 0;
	float _scaleY = 0;
	std::unordered_map<HCURSOR, _CursorInfo> _cursorMap;

	ComPtr<ID3D11DeviceContext4> _d3dDC = nullptr;
	ComPtr<ID3D11Device5> _d3dDevice = nullptr;
	ComPtr<ID3D11Texture2D> _input = nullptr;
	ComPtr<ID3D11Texture2D> _output = nullptr;

	ID3D11RenderTargetView* _outputRtv = nullptr;
	ID3D11ShaderResourceView* _inputSrv = nullptr;
	D3D11_VIEWPORT _vp{};

	ID3D11SamplerState* _sampler = nullptr;
	ComPtr<ID3D11PixelShader> _noCursorPS = nullptr;
	ComPtr<ID3D11PixelShader> _withCursorPS = nullptr;
	ComPtr<ID3D11Buffer> _constantBuffer1 = nullptr;
	ComPtr<ID3D11Buffer> _constantBuffer2 = nullptr;
	ComPtr<ID3D11SamplerState> _linearSam = nullptr;
	ComPtr<ID3D11SamplerState> _pointSam = nullptr;
};
