#include "pch.h"
#include "Renderer.h"
#include "App.h"
#include <DirectXColors.h>


bool Renderer::Initialize() {
	_logger = App::GetInstance()->GetLogger();
	if (!_InitD3D()) {
		return false;
	}

	return true;
}

void Renderer::Render() {
	ID3D11RenderTargetView* rtv = _renderTargetView.Get();
	_d3dDC->OMSetRenderTargets(1, &rtv, nullptr);
	_d3dDC->ClearRenderTargetView(rtv, DirectX::Colors::MidnightBlue);

	_dxgiSwapChain->Present(0, 0);
}

bool Renderer::_InitD3D() {
	HRESULT hr;
	const SIZE hostSize = App::GetInstance()->GetHostWndSize();

	{
		UINT createDeviceFlags = 0;
#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_11_1
		};

		ComPtr<ID3D11Device> d3dDevice;
		ComPtr<ID3D11DeviceContext> d3dDC;
		D3D_FEATURE_LEVEL featureLevel;
		hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			createDeviceFlags,
			featureLevels,
			1,
			D3D11_SDK_VERSION,
			&d3dDevice,
			&featureLevel,
			&d3dDC
		);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(_logger, fmt::format("D3D11CreateDevice 失败\n\tHRESULT：{}", hr));
			return false;
		}
		SPDLOG_LOGGER_INFO(_logger, fmt::format("已创建 D3D Device\n\t功能级别：{}", featureLevel));

		hr = d3dDevice.As<ID3D11Device5>(&_d3dDevice);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(_logger, fmt::format("获取 ID3D11Device5 失败\n\tHRESULT：{}", hr));
			return false;
		}

		hr = d3dDC.As<ID3D11DeviceContext4>(&_d3dDC);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(_logger, fmt::format("获取 ID3D11DeviceContext4 失败\n\tHRESULT：{}", hr));
			return false;
		}
	}

	{
		ComPtr<IDXGIFactory2> dxgiFactory;

		ComPtr<IDXGIDevice> dxgiDevice = nullptr;
		hr = _d3dDevice.As<IDXGIDevice>(&dxgiDevice);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(_logger, fmt::format("获取 IDXGIDevice 失败\n\tHRESULT：{}", hr));
			return false;
		}
		
		ComPtr<IDXGIAdapter> dxgiAdapter;
		hr = dxgiDevice->GetAdapter(&dxgiAdapter);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(_logger, fmt::format("获取 IDXGIAdapter 失败\n\tHRESULT：{}", hr));
			return false;
		}
		
		hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(_logger, fmt::format("获取 IDXGIFactory2 失败\n\tHRESULT：{}", hr));
			return false;
		}

		DXGI_SWAP_CHAIN_DESC1 sd = {};
		sd.Width = hostSize.cx;
		sd.Height = hostSize.cy;
		sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 2;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

		hr = dxgiFactory->CreateSwapChainForHwnd(
			_d3dDevice.Get(),
			App::GetInstance()->GetHwndHost(),
			&sd,
			nullptr,
			nullptr,
			&_dxgiSwapChain
		);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(_logger, fmt::format("创建交换链失败\n\tHRESULT：{}", hr));
			return false;
		}
		
		hr = dxgiFactory->MakeWindowAssociation(App::GetInstance()->GetHwndHost(), DXGI_MWA_NO_ALT_ENTER);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(_logger, fmt::format("MakeWindowAssociation 失败\n\tHRESULT：{}", hr));
		}
	}

	{
		ComPtr<ID3D11Texture2D> pBackBuffer;
		hr = _dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(_logger, fmt::format("获取后缓冲区失败\n\tHRESULT：{}", hr));
			return false;
		}
		
		hr = _d3dDevice->CreateRenderTargetView(pBackBuffer.Get(), NULL, &_renderTargetView);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(_logger, fmt::format("创建后缓冲区的渲染目标视图失败\n\tHRESULT：{}", hr));
			return false;
		}
	}

	// 所有效果都使用三角形带拓扑
	_d3dDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	return true;
}
