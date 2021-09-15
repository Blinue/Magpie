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
		bool isMonochrome = false;
	};

	bool _ResolveCursor(HCURSOR hCursor, _CursorInfo& result) const;

	bool _DrawWithCursor();

private:
	INT _cursorSpeed = 0;
	RECT _destRect{};
	SIZE _inputSize{};
	float _scaleX = 0;
	float _scaleY = 0;
	std::unordered_map<HCURSOR, _CursorInfo> _cursorMap;

	ComPtr<ID3D11DeviceContext3> _d3dDC;
	ComPtr<ID3D11Device3> _d3dDevice;

	ID3D11RenderTargetView* _outputRtv = nullptr;
	ID3D11ShaderResourceView* _inputSrv = nullptr;
	D3D11_VIEWPORT _vp{};

	ComPtr<ID3D11PixelShader> _noCursorPS;
	ComPtr<ID3D11PixelShader> _withCursorPS;
	ComPtr<ID3D11Buffer> _withCursorCB;
	ID3D11SamplerState* _linearSam = nullptr;
	ID3D11SamplerState* _pointSam = nullptr;
};
