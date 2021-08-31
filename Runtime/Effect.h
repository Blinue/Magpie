#pragma once
#include "pch.h"
#include "Env.h"
#include <directxcolors.h>

using namespace DirectX;


struct SimpleVertex {
	XMFLOAT3 Pos;
	XMFLOAT4 TexCoord;
};


class Effect {
public:
	Effect(D2D_SIZE_U inputSize, ID3D11RenderTargetView* output, ID3D11SamplerState* linearSampler, D2D1_VECTOR_2F scale): _output(output), _linearSampler(linearSampler) {
		_d3dDevice = Env::$instance->GetD3DDevice();
		_d3dDC = Env::$instance->GetD3DDC();

		ComPtr<ID3D11Resource> outputResource;
		output->GetResource(&outputResource);
		ComPtr<ID3D11Texture2D> outputTexture;
		outputResource.As<ID3D11Texture2D>(&outputTexture);
		D3D11_TEXTURE2D_DESC desc;
		outputTexture->GetDesc(&desc);
		D2D1_SIZE_F outputTextureSize = { (float)desc.Width, (float)desc.Height };

		_vp.Width = outputTextureSize.width;
		_vp.Height = outputTextureSize.height;
		_vp.MinDepth = 0.0f;
		_vp.MaxDepth = 1.0f;

		// 编译顶点着色器
		ComPtr<ID3DBlob> blob = nullptr;
		Debug::ThrowIfComFailed(
			_CompileShaderFromFile(L"shaders\\Lanczos6.hlsl", "VS", "vs_5_0", &blob),
			L""
		);

		Debug::ThrowIfComFailed(
			_d3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_vsShader),
			L""
		);

		// 创建输入布局
		D3D11_INPUT_ELEMENT_DESC layout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(XMFLOAT3), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(layout);
		Debug::ThrowIfComFailed(
			_d3dDevice->CreateInputLayout(layout, numElements, blob->GetBufferPointer(),
				blob->GetBufferSize(), &_vtxLayout),
			L""
		);

		// 编译像素着色器
		Debug::ThrowIfComFailed(
			_CompileShaderFromFile(L"shaders\\Lanczos6.hlsl", "PS", "ps_5_0", &blob),
			L""
		);
		Debug::ThrowIfComFailed(
			_d3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_psShader),
			L""
		);
		
		// 创建顶点缓冲区
		float outputLeft = -((inputSize.width & 0xfffffffe) / (float)outputTextureSize.width);
		float outputTop = (inputSize.height & 0xfffffffe) / (float)outputTextureSize.height;
		float outputRight = outputLeft + 2 * inputSize.width / (float)outputTextureSize.width;
		float outputBottom = outputTop - 2 * inputSize.height  / (float)outputTextureSize.height;
		float pixelWidth = 1.0f / inputSize.width;
		float pixelHeight = 1.0f / inputSize.height;
		SimpleVertex vertices[] = {
			{ XMFLOAT3(outputLeft, outputTop, 0.5f), XMFLOAT4(0.0f, 0.0f, pixelWidth, pixelHeight)},
			{ XMFLOAT3(outputRight, outputTop, 0.5f), XMFLOAT4(1.0f, 0.0f, pixelWidth, pixelHeight) },
			{ XMFLOAT3(outputLeft, outputBottom, 0.5f), XMFLOAT4(0.0f, 1.0f, pixelWidth, pixelHeight) },
			{ XMFLOAT3(outputRight, outputBottom, 0.5f), XMFLOAT4(1.0f, 1.0f, pixelWidth, pixelHeight) }
		};
		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(vertices);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA InitData = {};
		InitData.pSysMem = vertices;
		Debug::ThrowIfComFailed(
			_d3dDevice->CreateBuffer(&bd, &InitData, &_vtxBuffer),
			L""
		);
	}

	void Apply(ID3D11ShaderResourceView* input) {
		_d3dDC->RSSetViewports(1, &_vp);

		_d3dDC->OMSetRenderTargets(1, &_output, nullptr);
		// _d3dDC->ClearRenderTargetView(_output, Colors::Black);

		_d3dDC->IASetInputLayout(_vtxLayout.Get());

		UINT stride = sizeof(SimpleVertex);
		UINT offset = 0;
		auto t = _vtxBuffer.Get();
		_d3dDC->IASetVertexBuffers(0, 1, &t, &stride, &offset);

		_d3dDC->VSSetShader(_vsShader.Get(), nullptr, 0);

		_d3dDC->PSSetShader(_psShader.Get(), nullptr, 0);
		_d3dDC->PSSetSamplers(0, 1, &_linearSampler);
		_d3dDC->PSSetShaderResources(0, 1, &input);

		_d3dDC->Draw(4, 0);
	}
private:
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


	ID3D11Device3* _d3dDevice = nullptr;
	ID3D11DeviceContext4* _d3dDC = nullptr;
	ID3D11SamplerState* _linearSampler = nullptr;
	ComPtr<ID3D11VertexShader> _vsShader = nullptr;
	ComPtr<ID3D11PixelShader> _psShader = nullptr;
	ComPtr<ID3D11InputLayout> _vtxLayout = nullptr;
	ComPtr<ID3D11Buffer> _vtxBuffer = nullptr;

	ID3D11RenderTargetView* _output;
	D3D11_VIEWPORT _vp{};
};