#include "pch.h"
#include "Effect.h"
#include "App.h"


extern std::shared_ptr<spdlog::logger> logger;

bool Effect::Initialize(ComPtr<ID3D11Texture2D> input) {
	Renderer& renderer = App::GetInstance().GetRenderer();
	_d3dDevice = renderer.GetD3DDevice();
	_d3dDC = renderer.GetD3DDC();

	D3D11_TEXTURE2D_DESC desc;
	input->GetDesc(&desc);
	_inputSize.cx = desc.Width;
	_inputSize.cy = desc.Height;

	HRESULT hr = renderer.GetShaderResourceView(input.Get(), &_inputSrv);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("获取 ShaderResourceView 失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	_sampler = renderer.GetSampler(Renderer::FilterType::LINEAR).Get();

	// 编译顶点着色器
	ComPtr<ID3DBlob> blob = nullptr;
	hr = _CompileShaderFromFile(L"shaders\\Lanczos6.hlsl", "VS", "vs_5_0", &blob);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("编译顶点着色器失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	hr = _d3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_vsShader);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("创建顶点着色器失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	// 创建输入布局
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(XMFLOAT3), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);
	hr = _d3dDevice->CreateInputLayout(layout, numElements, blob->GetBufferPointer(),
			blob->GetBufferSize(), &_vtxLayout);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("创建输入布局失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	// 编译像素着色器
	hr = _CompileShaderFromFile(L"shaders\\Lanczos6.hlsl", "PS", "ps_5_0", &blob);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("编译像素着色器失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	hr = _d3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_psShader);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("创建像素着色器失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	return true;
}

void Effect::Draw() {
	_d3dDC->OMSetRenderTargets(1, &_outputRtv, nullptr);
	_d3dDC->RSSetViewports(1, &_vp);

	_d3dDC->IASetInputLayout(_vtxLayout.Get());

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	auto t = _vtxBuffer.Get();
	_d3dDC->IASetVertexBuffers(0, 1, &t, &stride, &offset);

	_d3dDC->VSSetShader(_vsShader.Get(), nullptr, 0);

	_d3dDC->PSSetShader(_psShader.Get(), nullptr, 0);
	_d3dDC->PSSetSamplers(0, 1, &_sampler);
	_d3dDC->PSSetShaderResources(0, 1, &_inputSrv);

	_d3dDC->Draw(4, 0);
}



bool Effect::SetOutput(ComPtr<ID3D11Texture2D> output) {
	HRESULT hr = App::GetInstance().GetRenderer().GetRenderTargetView(output.Get(), &_outputRtv);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("获取 RenderTargetView 失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	D3D11_TEXTURE2D_DESC desc;
	output->GetDesc(&desc);
	D2D1_SIZE_U outputTextureSize = { desc.Width, desc.Height };

	_vp.Width = (float)outputTextureSize.width;
	_vp.Height = (float)outputTextureSize.height;
	_vp.MinDepth = 0.0f;
	_vp.MaxDepth = 1.0f;

	// 创建顶点缓冲区
	D2D1_SIZE_U outputSize = {
		(UINT32)lroundf(_inputSize.cx * 1.5f),
		(UINT32)lroundf(_inputSize.cy * 1.5f)
	};

	float outputLeft, outputTop, outputRight, outputBottom;
	if (outputTextureSize.width == outputSize.width && outputTextureSize.height == outputSize.height) {
		outputLeft = outputBottom = -1;
		outputRight = outputTop = 1;
	} else {
		outputLeft = std::floorf(((float)outputTextureSize.width - outputSize.width) / 2) * 2 / outputTextureSize.width - 1;
		outputTop = 1 - std::ceilf(((float)outputTextureSize.height - outputSize.height) / 2) * 2 / outputTextureSize.height;
		outputRight = outputLeft + 2 * outputSize.width / (float)outputTextureSize.width;
		outputBottom = outputTop - 2 * outputSize.height / (float)outputTextureSize.height;
	}

	float pixelWidth = 1.0f / _inputSize.cx;
	float pixelHeight = 1.0f / _inputSize.cy;
	SimpleVertex vertices[] = {
		{ XMFLOAT3(outputLeft, outputTop, 0.5f), XMFLOAT4(0.0f, 0.0f, pixelWidth, pixelHeight) },
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
	hr = _d3dDevice->CreateBuffer(&bd, &InitData, &_vtxBuffer);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("创建顶点缓冲区失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	return true;
}

HRESULT Effect::_CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut) {
	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &errorBlob);
	if (FAILED(hr)) {
		if (errorBlob) {
			SPDLOG_LOGGER_ERROR(logger, fmt::sprintf(
				"编译着色器失败：%s\n\tHRESULT：0x%X", (const char*)errorBlob->GetBufferPointer(), hr));
		}
		return hr;
	}

	return S_OK;
}
