#pragma once
#include "pch.h"


class Renderer {
public:
	bool Initialize();

	void Render();

private:
	bool _InitD3D();

	ComPtr<ID3D11Device5> _d3dDevice = nullptr;
	ComPtr<IDXGISwapChain1> _dxgiSwapChain = nullptr;
	ComPtr<ID3D11DeviceContext4> _d3dDC = nullptr;
	ComPtr<ID3D11RenderTargetView> _renderTargetView = nullptr;

	std::shared_ptr<spdlog::logger> _logger = nullptr;
};
