#pragma once
#include "pch.h"
#include "Env.h"
#include "Effect.h"

using namespace DirectX;


class D3DContext {
public:
	D3DContext() {
		_InitD3D();

		const RECT& srcClient = Env::$instance->GetSrcClient();
		_effect.reset(new Effect(
			{ UINT(srcClient.right - srcClient.left), UINT(srcClient.bottom - srcClient.top) },
			_d3dRenderTargetView,
			_linearSampler,
			{1.2f,1.2f}
		));
	}

	// 不可复制，不可移动
	D3DContext(const D3DContext&) = delete;
	D3DContext(D3DContext&&) = delete;

	~D3DContext() {
		if (_d3dRenderTargetView) {
			_d3dRenderTargetView->Release();
		}
		if (_linearSampler) {
			_linearSampler->Release();
		}
	}

	void Render(ComPtr<ID3D11Texture2D> input) {
		if (!input) {
			return;
		}
		
		{
			ID3D11ShaderResourceView* rv = nullptr;
			D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
			desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			desc.Texture2D.MostDetailedMip = 0;
			desc.Texture2D.MipLevels = 1;
			Debug::ThrowIfComFailed(
				_d3dDevice->CreateShaderResourceView(input.Get(), &desc, &rv),
				L""
			);

			_effect->Apply(rv);

			rv->Release();
		}

		_dxgiSwapChain->Present(0, 0);
	}

private:
	void _InitD3D() {
		const RECT& hostClient = Env::$instance->GetHostClient();
		const int hostWidth = hostClient.right - hostClient.left;
		const int hostHeight = hostClient.bottom - hostClient.top;

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
			Debug::ThrowIfComFailed(
				D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
					featureLevels, 1, D3D11_SDK_VERSION, &d3dDevice, nullptr, &d3dDC),
				L""
			);
			Debug::ThrowIfComFailed(
					d3dDevice.As<ID3D11Device3>(&_d3dDevice),
					L""
			);
			Debug::ThrowIfComFailed(
				d3dDC.As<ID3D11DeviceContext4>(&_d3dDC),
				L""
			);
		}
		
		{
			ComPtr<IDXGIFactory2> dxgiFactory;

			ComPtr<IDXGIDevice> dxgiDevice = nullptr;
			Debug::ThrowIfComFailed(
				_d3dDevice.As<IDXGIDevice>(&dxgiDevice),
				L""
			);
			ComPtr<IDXGIAdapter> dxgiAdapter;
			Debug::ThrowIfComFailed(
				dxgiDevice->GetAdapter(&dxgiAdapter),
				L""
			);
			Debug::ThrowIfComFailed(
				dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)),
				L""
			);

			DXGI_SWAP_CHAIN_DESC1 sd = {};
			sd.Width = hostWidth;
			sd.Height = hostHeight;
			sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.BufferCount = 2;
			sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

			Debug::ThrowIfComFailed(
				dxgiFactory->CreateSwapChainForHwnd(_d3dDevice.Get(),
					Env::$instance->GetHwndHost(), &sd, nullptr, nullptr, &_dxgiSwapChain),
				L""
			);
			Debug::ThrowIfComFailed(
				dxgiFactory->MakeWindowAssociation(Env::$instance->GetHwndHost(), DXGI_MWA_NO_ALT_ENTER),
				L""
			);
		}

		Env::$instance->SetD3DContext(_d3dDevice, _d3dDC);

		{
			ComPtr<ID3D11Texture2D> pBackBuffer;
			Debug::ThrowIfComFailed(
				_dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)),
				L""
			);

			Debug::ThrowIfComFailed(
				_d3dDevice->CreateRenderTargetView(pBackBuffer.Get(), NULL, &_d3dRenderTargetView),
				L""
			);
		}

		

		// Compile the vertex shader
		D3D11_SAMPLER_DESC sampDesc = {};
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = 0;
		Debug::ThrowIfComFailed(
			_d3dDevice->CreateSamplerState(&sampDesc, &_linearSampler),
			L""
		);

		// Set primitive topology
		_d3dDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	}

	

private:
	ComPtr<ID3D11Device3> _d3dDevice = nullptr;
	ComPtr<IDXGISwapChain1> _dxgiSwapChain = nullptr;
	ComPtr<ID3D11DeviceContext4> _d3dDC = nullptr;
	ID3D11RenderTargetView* _d3dRenderTargetView = nullptr;
	ID3D11SamplerState* _linearSampler = nullptr;
	
	std::unique_ptr<Effect> _effect = nullptr;
};
