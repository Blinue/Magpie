#include "pch.h"
#include "CursorRenderer.h"
#include "App.h"

using namespace DirectX;

extern std::shared_ptr<spdlog::logger> logger;


const char pixelShader[] = R"(
Texture2D tex : register(t0);
SamplerState sam : register(s0);

struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float4 TexCoord : TEXCOORD0;
};

float4 PS(VS_OUTPUT input) : SV_Target{
	return tex.Sample(sam, input.TexCoord);
}
)";

bool CursorRenderer::Initialize(ComPtr<ID3D11Texture2D> input, ComPtr<ID3D11Texture2D> output) {
	_input = input;
	_output = output;
	App& app = App::GetInstance();
	Renderer& renderer = app.GetRenderer();
	_d3dDC = renderer.GetD3DDC();
	_d3dDevice = renderer.GetD3DDevice();

	// 限制鼠标在窗口内
	/*if (!ClipCursor(&App::GetInstance().GetSrcClientRect())) {
		SPDLOG_LOGGER_ERROR(logger, "ClipCursor 失败");
	}*/

	D3D11_TEXTURE2D_DESC inputDesc, outputDesc;
	input->GetDesc(&inputDesc);
	output->GetDesc(&outputDesc);

	RECT destRect{};
	if (inputDesc.Width >= outputDesc.Width) {
		destRect.left = 0;
		destRect.right = outputDesc.Width;
	} else {
		destRect.left = (outputDesc.Width - inputDesc.Width) / 2;
		destRect.right = destRect.left + inputDesc.Width;
	}
	if (inputDesc.Height >= outputDesc.Height) {
		destRect.top = 0;
		destRect.bottom = outputDesc.Height;
	} else {
		destRect.top = (outputDesc.Height - inputDesc.Height) / 2;
		destRect.bottom = destRect.top + inputDesc.Height;
	}

	if (App::GetInstance().IsAdjustCursorSpeed()) {
		// 设置鼠标移动速度
		if (SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_cursorSpeed, 0)) {
			const RECT& srcClient = app.GetSrcClientRect();
			float scaleX = float(destRect.right - destRect.left) / (srcClient.right - srcClient.left);
			float scaleY = float(destRect.bottom - destRect.top) / (srcClient.bottom - srcClient.top);

			long newSpeed = std::clamp(lroundf(_cursorSpeed / (scaleX + scaleY) * 2), 1L, 20L);

			if (!SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)newSpeed, 0)) {
				SPDLOG_LOGGER_ERROR(logger, "设置光标移速失败");
			}
		} else {
			SPDLOG_LOGGER_ERROR(logger, "获取光标移速失败");
		}

		SPDLOG_LOGGER_INFO(logger, "已调整光标移速");
	}

	HRESULT hr = renderer.GetRenderTargetView(output.Get(), &_outputRtv);
	if (FAILED(hr)) {
		return false;
	}

	hr = renderer.GetShaderResourceView(input.Get(), &_inputSrv);
	if (FAILED(hr)) {
		return false;
	}

	_vp.Width = float(destRect.right - destRect.left);
	_vp.Height = float(destRect.bottom - destRect.top);
	_vp.MinDepth = 0.0f;
	_vp.MaxDepth = 1.0f;

	_vsShader = renderer.GetVSShader();
	_vtxLayout = renderer.GetInputLayout();
	_sampler = renderer.GetSampler(Renderer::FilterType::POINT).Get();

	SimpleVertex vertices[] = {
		{ XMFLOAT3(-1.0f, 1.0f, 0.5f), XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0.5f), XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 0.5f), XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0.5f), XMFLOAT4(1.0f, 1.0f, 0.0f, 0.0f) }
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

	ComPtr<ID3DBlob> blob = nullptr;
	ComPtr<ID3DBlob> errorMsgs = nullptr;
	hr = D3DCompile(pixelShader, sizeof(pixelShader), nullptr, nullptr, nullptr,
		"PS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &blob, &errorMsgs);
	if (FAILED(hr)) {
		if (errorMsgs) {
			SPDLOG_LOGGER_ERROR(logger, fmt::sprintf(
				"编译像素着色器失败：%s\n\tHRESULT：0x%X", (const char*)errorMsgs->GetBufferPointer(), hr));
		}
		return false;
	}

	hr = renderer.GetD3DDevice()->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_psShader);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("创建像素着色器失败\n\tHRESULT：0x%X", hr));
		return false;
	}
	
	/*if (!MagShowSystemCursor(FALSE)) {
		SPDLOG_LOGGER_ERROR(logger, "MagShowSystemCursor 失败");
	}*/

	SPDLOG_LOGGER_INFO(logger, "CursorRenderer 初始化完成");
	return true;
}

CursorRenderer::~CursorRenderer() {
	ClipCursor(nullptr);

	if (App::GetInstance().IsAdjustCursorSpeed()) {
		SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)_cursorSpeed, 0);
	}

	MagShowSystemCursor(TRUE);

	SPDLOG_LOGGER_INFO(logger, "CursorRenderer 已析构");
}

void CursorRenderer::Draw() {
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
