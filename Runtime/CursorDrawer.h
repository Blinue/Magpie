#pragma once
#include "pch.h"


// 处理光标的渲染
class CursorDrawer {
public:
	bool Initialize(ID3D11Texture2D* renderTarget);

	~CursorDrawer();

	bool Update();

	void Draw();

private:
	struct _CursorInfo {
		winrt::com_ptr<ID3D11ShaderResourceView> texture = nullptr;
		int xHotSpot = 0;
		int yHotSpot = 0;
		UINT width = 0;
		UINT height = 0;
		bool hasInv = false;
	};

	bool _ResolveCursor(HCURSOR hCursor, _CursorInfo& result) const;

	void _StartCapture(POINT cursorPt);

	void _StopCapture(POINT cursorPt);

	void _DynamicClip(POINT cursorPt);

private:
	bool _isUnderCapture = false;
	std::array<bool, 4> _curClips{};

	SIZE _monoCursorSize{};
	INT _cursorSpeed = 0;
	float _zoomFactorX = 1;
	float _zoomFactorY = 1;
	SIZE _renderTargetSize{};
	float _clientScaleX = 0;
	float _clientScaleY = 0;
	std::unordered_map<HCURSOR, _CursorInfo> _cursorMap;

	ID3D11ShaderResourceView* _renderTargetSrv = nullptr;
	ID3D11RenderTargetView* _rtv = nullptr;
	winrt::com_ptr<ID3D11Buffer> _vtxBuffer;

	winrt::com_ptr<ID3D11Texture2D> _monoTmpTexture;
	ID3D11RenderTargetView* _monoTmpRtv = nullptr;
	ID3D11ShaderResourceView* _monoTmpSrv = nullptr;

	winrt::com_ptr<ID3D11PixelShader> _monoCursorPS;
	winrt::com_ptr<ID3D11Buffer> _withCursorCB;
	ID3D11SamplerState* _linearSam = nullptr;
	ID3D11SamplerState* _pointSam = nullptr;
};
