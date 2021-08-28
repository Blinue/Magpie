#pragma once
#include "pch.h"
#include "Env.h"
#include <directxcolors.h>


using namespace DirectX;

struct SimpleVertex {
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

struct ConstantBuffer {
	XMMATRIX mWorld;
	XMMATRIX mView;
	XMMATRIX mProjection;
};

ID3D11Buffer* g_pVertexBuffer;

class D3DContext {
public:
	D3DContext() {
		_InitD3D();
	}

	// 不可复制，不可移动
	D3DContext(const D3DContext&) = delete;
	D3DContext(D3DContext&&) = delete;

	~D3DContext() {
		if (_d3dRenderTargetView) {
			_d3dRenderTargetView->Release();
		}
		if (_constantBuffer) {
			_constantBuffer->Release();
		}
	}

	void Render() {
		_d3dDC->OMSetRenderTargets(1, &_d3dRenderTargetView, nullptr);

		static ULONGLONG timeStart = 0;
		ULONGLONG timeCur = GetTickCount64();
		if (timeStart == 0)
			timeStart = timeCur;
		float t = (timeCur - timeStart) / 1000.0f;

		_worldTransform = XMMatrixRotationY(t);

		ConstantBuffer cb{};
		cb.mWorld = XMMatrixTranspose(_worldTransform);
		cb.mView = XMMatrixTranspose(_viewTransform);
		cb.mProjection = XMMatrixTranspose(_projectionTransform);
		_d3dDC->UpdateSubresource(_constantBuffer, 0, nullptr, &cb, 0, 0);

		_d3dDC->ClearRenderTargetView(_d3dRenderTargetView, Colors::MidnightBlue);

		_d3dDC->VSSetShader(_vsShader.Get(), nullptr, 0);
		_d3dDC->VSSetConstantBuffers(0, 1, &_constantBuffer);
		_d3dDC->PSSetShader(_psShader.Get(), nullptr, 0);
		_d3dDC->DrawIndexed(36, 0, 0);

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
			sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

		D3D11_VIEWPORT vp{};
		vp.Width = (FLOAT)hostWidth;
		vp.Height = (FLOAT)hostHeight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		_d3dDC->RSSetViewports(1, &vp);

		// Compile the vertex shader
		ComPtr<ID3DBlob> blob = nullptr;
		Debug::ThrowIfComFailed(
			_CompileShaderFromFile(L"shaders\\Test.fxh", "VS", "vs_4_0", &blob),
			L""
		);

		// Create the vertex shader
		Debug::ThrowIfComFailed(
			_d3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_vsShader),
			L""
		);

		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};
		UINT numElements = ARRAYSIZE(layout);

		// Create the input layout
		Debug::ThrowIfComFailed(
			_d3dDevice->CreateInputLayout(layout, numElements, blob->GetBufferPointer(),
				blob->GetBufferSize(), &_vertexLayout),
			L""
		);

		// Set the input layout
		_d3dDC->IASetInputLayout(_vertexLayout.Get());

		// Compile the pixel shader
		Debug::ThrowIfComFailed(
			_CompileShaderFromFile(L"shaders\\Test.fxh", "PS", "ps_4_0", &blob),
			L""
		);

		// Create the pixel shader
		Debug::ThrowIfComFailed(
			_d3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_psShader),
			L""
		);
		
		// Create vertex buffer
		SimpleVertex vertices[] =
		{
			{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
			{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) },
		};
		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(SimpleVertex) * 8;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		ID3D11Buffer* buffer;
		D3D11_SUBRESOURCE_DATA InitData = {};
		InitData.pSysMem = vertices;
		Debug::ThrowIfComFailed(
			_d3dDevice->CreateBuffer(&bd, &InitData, &buffer),
			L""
		);

		// Set vertex buffer
		UINT stride = sizeof(SimpleVertex);
		UINT offset = 0;
		_d3dDC->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
		buffer->Release();

		WORD indices[] =
		{
			3,1,0,
			2,1,3,

			0,5,4,
			1,5,0,

			3,4,7,
			0,4,3,

			1,6,5,
			2,6,1,

			2,7,6,
			3,7,2,

			6,4,5,
			7,4,6,
		};

		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(WORD) * 36;        // 36 vertices needed for 12 triangles in a triangle list
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		InitData.pSysMem = indices;
		Debug::ThrowIfComFailed(
			_d3dDevice->CreateBuffer(&bd, &InitData, &buffer),
			L""
		);
		_d3dDC->IASetIndexBuffer(buffer, DXGI_FORMAT_R16_UINT, 0);
		buffer->Release();

		// Set primitive topology
		_d3dDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(ConstantBuffer);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		Debug::ThrowIfComFailed(
			_d3dDevice->CreateBuffer(&bd, nullptr, &_constantBuffer),
			L""
		);

		_worldTransform = XMMatrixIdentity();

		XMVECTOR Eye = XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f);
		XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		_viewTransform = XMMatrixLookAtLH(Eye, At, Up);

		_projectionTransform = XMMatrixPerspectiveFovLH(XM_PIDIV2, hostWidth / (FLOAT)hostHeight, 0.01f, 100.0f);
	}

	HRESULT _CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut) {
		DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
		// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
		// Setting this flag improves the shader debugging experience, but still allows 
		// the shaders to be optimized and to run exactly the way they will run in 
		// the release configuration of this program.
		dwShaderFlags |= D3DCOMPILE_DEBUG;

		// Disable optimizations to further improve shader debugging
		dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		ComPtr<ID3DBlob> pErrorBlob = nullptr;
		HRESULT hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
			dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
		if (FAILED(hr)) {
			if (pErrorBlob) {
				OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			}
			return hr;
		}

		return S_OK;
	}

private:
	ComPtr<ID3D11Device3> _d3dDevice = nullptr;
	ComPtr<IDXGISwapChain1> _dxgiSwapChain = nullptr;
	ComPtr<ID3D11DeviceContext4> _d3dDC = nullptr;
	ID3D11RenderTargetView* _d3dRenderTargetView = nullptr;

	ComPtr<ID3D11VertexShader> _vsShader = nullptr;
	ComPtr<ID3D11PixelShader> _psShader = nullptr;
	ComPtr<ID3D11InputLayout> _vertexLayout = nullptr;
	ID3D11Buffer* _constantBuffer = nullptr;

	XMMATRIX _worldTransform;
	XMMATRIX _viewTransform;
	XMMATRIX _projectionTransform;
};
