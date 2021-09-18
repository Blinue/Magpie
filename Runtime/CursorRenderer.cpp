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
Texture2D cursorTex : register(t1);

SamplerState linearSam : register(s0);
SamplerState pointSam : register(s1);

cbuffer constants : register(b0) {
	float4 cursorRect;
};

struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float4 TexCoord : TEXCOORD0;
};

float4 main(VS_OUTPUT input) : SV_TARGET {
	float2 coord = input.TexCoord.xy;
	
	float3 cur = inputTex.Sample(linearSam, coord).rgb;
	if (coord.x < cursorRect.x || coord.x > cursorRect.z || coord.y < cursorRect.y || coord.y > cursorRect.w) {
		return float4(cur, 1);
	} else {
		float2 cursorCoord = (coord - cursorRect.xy) / (cursorRect.zw - cursorRect.xy);
		cursorCoord.y /= 2;

		uint3 andMask = cursorTex.Sample(pointSam, cursorCoord).rgb * 255;
		float4 xorMask = cursorTex.Sample(pointSam, float2(cursorCoord.x, cursorCoord.y + 0.5f));
		
		float3 r = ((uint3(cur * 255) & andMask) ^ uint3(xorMask.rgb * 255)) / 255.0f;
		// 模拟透明度
		return float4(lerp(r, cur, 1.0f - xorMask.a), 1);
	}
}

)";

bool CursorRenderer::Initialize(ComPtr<ID3D11Texture2D> input, ComPtr<ID3D11Texture2D> output) {
	App& app = App::GetInstance();
	Renderer& renderer = app.GetRenderer();
	_d3dDC = renderer.GetD3DDC();
	_d3dDevice = renderer.GetD3DDevice();

	// 限制鼠标在窗口内
	if (!ClipCursor(&App::GetInstance().GetSrcClientRect())) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("ClipCursor 失败"));
	}

	D3D11_TEXTURE2D_DESC inputDesc, outputDesc;
	input->GetDesc(&inputDesc);
	output->GetDesc(&outputDesc);

	_inputSize.cx = inputDesc.Width;
	_inputSize.cy = inputDesc.Height;

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

	SPDLOG_LOGGER_INFO(logger, fmt::format("scaleX：{}，scaleY：{}", _scaleX, _scaleY));

	if (App::GetInstance().IsAdjustCursorSpeed()) {
		// 设置鼠标移动速度
		if (SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_cursorSpeed, 0)) {
			long newSpeed = std::clamp(lroundf(_cursorSpeed / (_scaleX + _scaleY) * 2), 1L, 20L);

			if (!SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)newSpeed, 0)) {
				SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("设置光标移速失败"));
			}
		} else {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("获取光标移速失败"));
		}

		SPDLOG_LOGGER_INFO(logger, "已调整光标移速");
	}

	HRESULT hr = renderer.GetRenderTargetView(output.Get(), &_outputRtv);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("GetRenderTargetView 失败", hr));
		return false;
	}

	hr = renderer.GetShaderResourceView(input.Get(), &_inputSrv);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("GetShaderResourceView 失败", hr));
		return false;
	}

	_vp.TopLeftX = (FLOAT)_destRect.left;
	_vp.TopLeftY = (FLOAT)_destRect.top;
	_vp.Width = float(_destRect.right - _destRect.left);
	_vp.Height = float(_destRect.bottom - _destRect.top);
	_vp.MinDepth = 0.0f;
	_vp.MaxDepth = 1.0f;

	ComPtr<ID3DBlob> blob = nullptr;
	if (!Utils::CompilePixelShader(noCursorPS, sizeof(noCursorPS), &blob)) {
		SPDLOG_LOGGER_ERROR(logger, "编译无光标着色器失败");
		return false;
	}
	hr = renderer.GetD3DDevice()->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_noCursorPS);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建像素着色器失败", hr));
		return false;
	}

	if (!Utils::CompilePixelShader(withCursorPS, sizeof(withCursorPS), &blob)) {
		SPDLOG_LOGGER_ERROR(logger, "编译有光标着色器失败");
		return false;
	}
	hr = renderer.GetD3DDevice()->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_withCursorPS);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建像素着色器失败", hr));
		return false;
	}

	// 创建提供光标信息的缓冲区
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = 16;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	hr = _d3dDevice->CreateBuffer(&bd, nullptr, &_withCursorCB);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateBuffer 失败", hr));
		return false;
	}

	if (!renderer.GetSampler(Renderer::FilterType::LINEAR, &_linearSam) 
		|| !renderer.GetSampler(Renderer::FilterType::POINT, &_pointSam)
	) {
		SPDLOG_LOGGER_ERROR(logger, "GetSampler 失败");
		return false;
	}
	
	if (!MagShowSystemCursor(FALSE)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("MagShowSystemCursor 失败"));
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
	if (!GetObject(hBmp, sizeof(bmp), &bmp)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetObject 失败"));
		return false;
	}
	width = bmp.bmWidth;
	height = bmp.bmHeight;

	BITMAPINFO bi{};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = width;
	bi.bmiHeader.biHeight = -height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biSizeImage = width * height * 4;

	pixels.resize(bi.bmiHeader.biSizeImage);
	HDC hdc = GetDC(NULL);
	if (GetDIBits(hdc, hBmp, 0, height, &pixels[0], &bi, DIB_RGB_COLORS) != height) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDIBits 失败"));
		ReleaseDC(NULL, hdc);
		return false;
	}
	ReleaseDC(NULL, hdc);

	return true;
}

bool CursorRenderer::_ResolveCursor(HCURSOR hCursor, _CursorInfo& result) const {
	assert(hCursor != NULL);

	ICONINFO ii{};
	if (!GetIconInfo(hCursor, &ii)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetIconInfo 失败"));
		return false;
	}

	result.xHotSpot = ii.xHotspot;
	result.yHotSpot = ii.yHotspot;

	auto clear = [&ii]() {
		if (ii.hbmColor) {
			DeleteBitmap(ii.hbmColor);
		}
		DeleteBitmap(ii.hbmMask);
	};

	std::vector<BYTE> pixels;

	result.isMonochrome = ii.hbmColor == NULL;
	if (result.isMonochrome) {
		// 单色光标
		if (!GetHBmpBits32(ii.hbmMask, result.width, result.height, pixels)) {
			SPDLOG_LOGGER_ERROR(logger, "GetHBmpBits32 失败");
			clear();
			return false;
		}
	} else {
		if (!GetHBmpBits32(ii.hbmMask, result.width, result.height, pixels)) {
			SPDLOG_LOGGER_ERROR(logger, "GetHBmpBits32 失败");
			clear();
			return false;
		}

		std::vector<BYTE> colorMsk;
		if (!GetHBmpBits32(ii.hbmColor, result.width, result.height, colorMsk)) {
			SPDLOG_LOGGER_ERROR(logger, "GetHBmpBits32 失败");
			clear();
			return false;
		}
		if (pixels.size() != colorMsk.size()) {
			SPDLOG_LOGGER_ERROR(logger, "hbmMask 和 hbmColor 的尺寸不一致");
			clear();
			return false;
		}

		pixels.resize(pixels.size() * 2);
		std::memcpy(pixels.data() + colorMsk.size(), colorMsk.data(), colorMsk.size());
		result.height *= 2;
	}

	// 特别处理 Alpha 通道全为 0 的光标
	bool isTransparent = true;
	for (int i = 0, n = result.width * result.height / 2; i < n; ++i) {
		if (pixels[n * 4 + i * 4 + 3] > 0) {
			isTransparent = false;
			break;
		}
	}
	if (isTransparent) {
		for (int i = 0, n = result.width * result.height / 2; i < n; ++i) {
			pixels[n * 4 + i * 4 + 3] = 255;
		}
	}

	clear();

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = result.width;
	desc.Height = result.height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
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
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
		return false;
	}

	hr = _d3dDevice->CreateShaderResourceView(texture.Get(), nullptr, &result.masks);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 ShaderResourceView 失败", hr));
		return false;
	}

	result.height /= 2;

	return true;
}

bool CursorRenderer::_DrawWithCursor() {
	CURSORINFO ci{};
	ci.cbSize = sizeof(ci);
	if (!GetCursorInfo(&ci)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetCursorInfo 失败"));
		return false;
	}

	if (!ci.hCursor || ci.flags != CURSOR_SHOWING) {
		return false;
	}

	_CursorInfo* info = nullptr;
	
	auto it = _cursorMap.find(ci.hCursor);
	if (it != _cursorMap.end()) {
		info = &it->second;
	} else {
		// 未在映射中找到，创建新映射
		_CursorInfo t;
		if (_ResolveCursor(ci.hCursor, t)) {
			info = &_cursorMap[ci.hCursor];
			*info = t;

			SPDLOG_LOGGER_INFO(logger, fmt::format("已解析光标：{}", (void*)ci.hCursor));
		} else {
			SPDLOG_LOGGER_ERROR(logger, "解析光标失败");
			return false;
		}
	}

	// 映射坐标
	// 鼠标坐标为整数，否则会出现模糊
	const RECT& srcClient = App::GetInstance().GetSrcClientRect();
	POINT targetScreenPos = {
		lroundf((ci.ptScreenPos.x - srcClient.left) * _scaleX) - info->xHotSpot,
		lroundf((ci.ptScreenPos.y - srcClient.top) * _scaleY) - info->yHotSpot
	};

	// 向着色器传递光标信息
	_d3dDC->PSSetConstantBuffers(0, 0, nullptr);
	D3D11_MAPPED_SUBRESOURCE ms{};
	HRESULT hr = _d3dDC->Map(_withCursorCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("Map 失败", hr));
		return false;
	}
	FLOAT* cursorRect = (FLOAT*)ms.pData;
	cursorRect[0] = float(targetScreenPos.x) / _inputSize.cx;
	cursorRect[1] = float(targetScreenPos.y) / _inputSize.cy;
	cursorRect[2] = float(targetScreenPos.x + info->width) / _inputSize.cx;
	cursorRect[3] = float(targetScreenPos.y + info->height) / _inputSize.cy;
	_d3dDC->Unmap(_withCursorCB.Get(), 0);

	_d3dDC->PSSetShader(_withCursorPS.Get(), nullptr, 0);
	ID3D11ShaderResourceView* srvs[2] = { _inputSrv, info->masks.Get() };
	_d3dDC->PSSetShaderResources(0, 2, srvs);
	ID3D11Buffer* withCursorCB = _withCursorCB.Get();
	_d3dDC->PSSetConstantBuffers(0, 1, &withCursorCB);
	ID3D11SamplerState* samplers[2] = { _linearSam, _pointSam };
	_d3dDC->PSSetSamplers(0, 2, samplers);

	return true;
}

void CursorRenderer::Draw() {
	_d3dDC->OMSetRenderTargets(1, &_outputRtv, nullptr);
	_d3dDC->RSSetViewports(1, &_vp);
	
	if (!_DrawWithCursor()) {
		// 不显示鼠标或创建映射失败
		_d3dDC->PSSetShaderResources(0, 1, &_inputSrv);
		_d3dDC->PSSetShader(_noCursorPS.Get(), nullptr, 0);
		_d3dDC->PSSetSamplers(0, 1, &_pointSam);
	}
	
	_d3dDC->Draw(3, 0);
}
