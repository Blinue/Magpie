#include "pch.h"
#include "CursorRenderer.h"
#include "App.h"
#include "Utils.h"

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

	// 如果输出可以容纳输入，将其居中；如果不能容纳，等比缩放到能容纳的最大大小
	RECT destRect{};
	if (inputDesc.Width <= outputDesc.Width && inputDesc.Height <= outputDesc.Height) {
		destRect.left = (outputDesc.Width - inputDesc.Width) / 2;
		destRect.right = destRect.left + inputDesc.Width;
		destRect.top = (outputDesc.Height - inputDesc.Height) / 2;
		destRect.bottom = destRect.top + inputDesc.Height;
	} else {
		float scaleX = float(outputDesc.Width) / inputDesc.Width;
		float scaleY = float(outputDesc.Height) / inputDesc.Height;

		if ( scaleX >= scaleY ) {
			long width = lroundf(inputDesc.Width * scaleY);
			destRect.left = (outputDesc.Width - width) / 2;
			destRect.right = destRect.left + width;
			destRect.top = 0;
			destRect.bottom = outputDesc.Height;
		} else {
			long height = lroundf(inputDesc.Height * scaleX);
			destRect.left = 0;
			destRect.right = outputDesc.Width;
			destRect.top = (outputDesc.Height - height) / 2;
			destRect.bottom = destRect.top + height;
		}
	}

	/*if (App::GetInstance().IsAdjustCursorSpeed()) {
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
	}*/

	HRESULT hr = renderer.GetRenderTargetView(output.Get(), &_outputRtv);
	if (FAILED(hr)) {
		return false;
	}

	hr = renderer.GetShaderResourceView(input.Get(), &_inputSrv);
	if (FAILED(hr)) {
		return false;
	}

	_vp.TopLeftX = destRect.left;
	_vp.TopLeftY = destRect.top;
	_vp.Width = float(destRect.right - destRect.left);
	_vp.Height = float(destRect.bottom - destRect.top);
	_vp.MinDepth = 0.0f;
	_vp.MaxDepth = 1.0f;

	_sampler = renderer.GetSampler(Renderer::FilterType::POINT).Get();

	ComPtr<ID3DBlob> blob = nullptr;
	if (!Utils::CompilePixelShader(pixelShader, sizeof(pixelShader), &blob)) {
		return false;
	}

	hr = renderer.GetD3DDevice()->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_psShader);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("创建像素着色器失败\n\tHRESULT：0x%X", hr));
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
	_d3dDC->PSSetShader(_psShader.Get(), nullptr, 0);
	_d3dDC->PSSetSamplers(0, 1, &_sampler);
	_d3dDC->PSSetShaderResources(0, 1, &_inputSrv);

	_d3dDC->Draw(4, 0);
}
