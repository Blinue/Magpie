#pragma once
#include "pch.h"


// 处理光标的渲染
class CursorDrawer {
public:
	bool Initialize(ComPtr<ID3D11Texture2D> renderTarget, const RECT& destRect);

	~CursorDrawer();

	void Draw();

private:
	struct _CursorInfo {
		ComPtr<ID3D11ShaderResourceView> texture = nullptr;
		int xHotSpot = 0;
		int yHotSpot = 0;
		UINT width = 0;
		UINT height = 0;
		bool hasInv = false;
	};

	bool _ResolveCursor(HCURSOR hCursor, _CursorInfo& result) const;

private:
	SIZE _monoCursorSize{};
	INT _cursorSpeed = 0;
	RECT _destRect{};
	float _zoomFactorX = 1;
	float _zoomFactorY = 1;
	SIZE _renderTargetSize{};
	float _clientScaleX = 0;
	float _clientScaleY = 0;
	std::unordered_map<HCURSOR, _CursorInfo> _cursorMap;

	ComPtr<ID3D11DeviceContext> _d3dDC;
	ComPtr<ID3D11Device> _d3dDevice;

	ID3D11ShaderResourceView* _renderTargetSrv = nullptr;
	ID3D11RenderTargetView* _rtv = nullptr;
	ComPtr<ID3D11Buffer> _vtxBuffer;

	ComPtr<ID3D11Texture2D> _monoTmpTexture;
	ID3D11RenderTargetView* _monoTmpRtv = nullptr;
	ID3D11ShaderResourceView* _monoTmpSrv = nullptr;

	ComPtr<ID3D11PixelShader> _monoCursorPS;
	ComPtr<ID3D11Buffer> _withCursorCB;
	ID3D11SamplerState* _linearSam = nullptr;
	ID3D11SamplerState* _pointSam = nullptr;
};
