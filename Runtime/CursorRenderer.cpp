#include "pch.h"
#include "CursorRenderer.h"
#include "App.h"
#include "Utils.h"


extern std::shared_ptr<spdlog::logger> logger;


const char noCursorPS[] = R"(

Texture2D tex : register(t0);
SamplerState sam : register(s0);

struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float4 TexCoord : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_Target{
	return tex.Sample(sam, input.TexCoord);
}

)";

const char withCursorPS[] = R"(

Texture2D inputTex : register(t0);
Texture2D<uint4> cursorTex : register(t1);

SamplerState linearSam : register(s0);
SamplerState pointSam : register(s1);

cbuffer constants : register(b0) {
	uint2 inputTexSize;
};

cbuffer constants : register(b1) {
	int4 cursorRect;
};

struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float4 TexCoord : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_TARGET {
	float2 coord = input.TexCoord.xy;
	int2 pos = floor(coord * inputTexSize);
	
	float4 cur = inputTex.Sample(linearSam, coord);
	if (pos.x < cursorRect.x || pos.x >= cursorRect.z || pos.y < cursorRect.y || pos.y >= cursorRect.w) {
		return cur;
	} else {
		uint2 cursorSize = cursorRect.zw - cursorRect.xy;
		int2 cursorCoord = pos - cursorRect.xy;
		uint andMask = cursorTex.Load(int3(cursorCoord, 0)).x;
		uint3 xorMask = cursorTex.Load(int3(cursorCoord.x, cursorCoord.y + cursorSize.y, 0)).bgr;
		
		return float4(((uint3(cur.rgb * 255) & andMask) ^ xorMask) / 255.0f, 1);
	}
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
	if (!ClipCursor(&App::GetInstance().GetSrcClientRect())) {
		SPDLOG_LOGGER_ERROR(logger, "ClipCursor 失败");
	}

	D3D11_TEXTURE2D_DESC inputDesc, outputDesc;
	input->GetDesc(&inputDesc);
	output->GetDesc(&outputDesc);

	// 如果输出可以容纳输入，将其居中；如果不能容纳，等比缩放到能容纳的最大大小
	if (inputDesc.Width <= outputDesc.Width && inputDesc.Height <= outputDesc.Height) {
		_destRect.left = (outputDesc.Width - inputDesc.Width) / 2;
		_destRect.right = _destRect.left + inputDesc.Width;
		_destRect.top = (outputDesc.Height - inputDesc.Height) / 2;
		_destRect.bottom = _destRect.top + inputDesc.Height;
	} else {
		float scaleX = float(outputDesc.Width) / inputDesc.Width;
		float scaleY = float(outputDesc.Height) / inputDesc.Height;

		if ( scaleX >= scaleY ) {
			long width = lroundf(inputDesc.Width * scaleY);
			_destRect.left = (outputDesc.Width - width) / 2;
			_destRect.right = _destRect.left + width;
			_destRect.top = 0;
			_destRect.bottom = outputDesc.Height;
		} else {
			long height = lroundf(inputDesc.Height * scaleX);
			_destRect.left = 0;
			_destRect.right = outputDesc.Width;
			_destRect.top = (outputDesc.Height - height) / 2;
			_destRect.bottom = _destRect.top + height;
		}
	}

	const RECT& srcClient = app.GetSrcClientRect();
	_scaleX = float(_destRect.right - _destRect.left) / (srcClient.right - srcClient.left);
	_scaleY = float(_destRect.bottom - _destRect.top) / (srcClient.bottom - srcClient.top);

	if (App::GetInstance().IsAdjustCursorSpeed()) {
		// 设置鼠标移动速度
		if (SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_cursorSpeed, 0)) {
			long newSpeed = std::clamp(lroundf(_cursorSpeed / (_scaleX + _scaleY) * 2), 1L, 20L);

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

	_vp.TopLeftX = (FLOAT)_destRect.left;
	_vp.TopLeftY = (FLOAT)_destRect.top;
	_vp.Width = float(_destRect.right - _destRect.left);
	_vp.Height = float(_destRect.bottom - _destRect.top);
	_vp.MinDepth = 0.0f;
	_vp.MaxDepth = 1.0f;

	_sampler = renderer.GetSampler(Renderer::FilterType::POINT).Get();

	ComPtr<ID3DBlob> blob = nullptr;
	if (!Utils::CompilePixelShader(noCursorPS, sizeof(noCursorPS), &blob)) {
		return false;
	}
	hr = renderer.GetD3DDevice()->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_noCursorPS);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("创建像素着色器失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	if (!Utils::CompilePixelShader(withCursorPS, sizeof(withCursorPS), &blob)) {
		return false;
	}
	hr = renderer.GetD3DDevice()->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_withCursorPS);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("创建像素着色器失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	// 创建存储输入尺寸的缓冲区
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = 16;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	D3D11_SUBRESOURCE_DATA initData{};
	UINT inputTexSize[4] = { inputDesc.Width, inputDesc.Height, 0, 0 };
	initData.pSysMem = inputTexSize;

	hr = _d3dDevice->CreateBuffer(&bd, &initData, &_constantBuffer1);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("CreateBuffer 失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	// 创建存储光标位置的缓冲区
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = 16;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = _d3dDevice->CreateBuffer(&bd, nullptr, &_constantBuffer2);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("CreateBuffer 失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	_linearSam = renderer.GetSampler(Renderer::FilterType::LINEAR);
	_pointSam = renderer.GetSampler(Renderer::FilterType::POINT);
	
	if (!MagShowSystemCursor(FALSE)) {
		SPDLOG_LOGGER_ERROR(logger, "MagShowSystemCursor 失败");
	}

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

bool GetHBmpBits32(HBITMAP hBmp, int& width, int& height, std::vector<BYTE>& pixels) {
	BITMAP bmp{};
	GetObject(hBmp, sizeof(bmp), &bmp);
	width = bmp.bmWidth;
	height = bmp.bmHeight;

	BITMAPINFO bi{};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = width;
	bi.bmiHeader.biHeight = -height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage = width * height * 4;

	pixels.resize(bi.bmiHeader.biSizeImage);
	HDC hdc = GetDC(NULL);
	GetDIBits(hdc, hBmp, 0, height, &pixels[0], &bi, DIB_RGB_COLORS);
	ReleaseDC(NULL, hdc);

	return true;
}

bool CursorRenderer::_ResolveCursor(HCURSOR hCursor, _CursorInfo& result) const {
	assert(hCursor != NULL);

	ICONINFO ii{};
	GetIconInfo(hCursor, &ii);

	result.xHotSpot = ii.xHotspot;
	result.yHotSpot = ii.yHotspot;

	std::vector<BYTE> pixels;

	if (ii.hbmColor == NULL) {
		// 单色光标
		GetHBmpBits32(ii.hbmMask, result.width, result.height, pixels);
	} else {
		GetHBmpBits32(ii.hbmMask, result.width, result.height, pixels);

		std::vector<BYTE> colorMsk;
		GetHBmpBits32(ii.hbmColor, result.width, result.height, colorMsk);
		if (pixels.size() != colorMsk.size()) {
			return false;
		}

		pixels.resize(pixels.size() * 2);
		std::memcpy(pixels.data() + colorMsk.size(), colorMsk.data(), colorMsk.size());
		result.height *= 2;
	}

	if (ii.hbmColor) {
		DeleteBitmap(ii.hbmColor);
	}
	DeleteBitmap(ii.hbmMask);

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = result.width;
	desc.Height = result.height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = &pixels[0];
	initData.SysMemPitch = result.width * 4;

	ComPtr<ID3D11Texture2D> texture;
	HRESULT hr = _d3dDevice->CreateTexture2D(&desc, &initData, &texture);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("创建 Texture2D 失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	hr = _d3dDevice->CreateShaderResourceView(texture.Get(), nullptr, &result.masks);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("创建 ShaderResourceView 失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	result.height /= 2;

	return true;
}

void CursorRenderer::Draw() {
	_d3dDC->OMSetRenderTargets(1, &_outputRtv, nullptr);
	_d3dDC->RSSetViewports(1, &_vp);
	

	CURSORINFO ci{};
	ci.cbSize = sizeof(ci);
	GetCursorInfo(&ci);

	_CursorInfo* info = nullptr;

	if (ci.hCursor && ci.flags == CURSOR_SHOWING) {
		auto it = _cursorMap.find(ci.hCursor);
		if (it != _cursorMap.end()) {
			info = &it->second;
		} else {
			// 未在映射中找到，创建新映射
			_CursorInfo t;
			if (_ResolveCursor(ci.hCursor, t)) {
				info = &_cursorMap[ci.hCursor];
				*info = t;
			}
		}
	}

	if (info) {
		// 映射坐标
		// 鼠标坐标为整数，否则会出现模糊
		const RECT& srcClient = App::GetInstance().GetSrcClientRect();
		POINT targetScreenPos = {
			lroundf((ci.ptScreenPos.x - srcClient.left) * _scaleX) - info->xHotSpot,
			lroundf((ci.ptScreenPos.y - srcClient.top) * _scaleY) - info->yHotSpot
		};

		// 向着色器传递光标位置
		_d3dDC->PSSetConstantBuffers(0, 0, nullptr);
		D3D11_MAPPED_SUBRESOURCE ms{};
		HRESULT hr = _d3dDC->Map(_constantBuffer2.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		if (FAILED(hr)) {
			_d3dDC->PSSetShaderResources(0, 1, &_inputSrv);
			_d3dDC->PSSetShader(_noCursorPS.Get(), nullptr, 0);
			_d3dDC->PSSetSamplers(0, 1, &_sampler);
			_d3dDC->Draw(3, 0);
			return;
		}
		INT* cursorRect = (INT*)ms.pData;
		cursorRect[0] = targetScreenPos.x;
		cursorRect[1] = targetScreenPos.y;
		cursorRect[2] = targetScreenPos.x + info->width;
		cursorRect[3] = targetScreenPos.y + info->height;
		_d3dDC->Unmap(_constantBuffer2.Get(), 0);

		_d3dDC->PSSetShader(_withCursorPS.Get(), nullptr, 0);
		ID3D11ShaderResourceView* srvs[2] = { _inputSrv, info->masks.Get() };
		_d3dDC->PSSetShaderResources(0, 2, srvs);
		ID3D11Buffer* buffer[2] = { _constantBuffer1.Get(), _constantBuffer2.Get() };
		_d3dDC->PSSetConstantBuffers(0, 2, buffer);
		ID3D11SamplerState* samplers[2] = { _linearSam.Get(), _pointSam.Get() };
		_d3dDC->PSSetSamplers(0, 2, samplers);
	} else {
		// 不显示鼠标或创建映射失败
		_d3dDC->PSSetShaderResources(0, 1, &_inputSrv);
		_d3dDC->PSSetShader(_noCursorPS.Get(), nullptr, 0);
		_d3dDC->PSSetSamplers(0, 1, &_sampler);
	}
	
	_d3dDC->Draw(3, 0);
}
