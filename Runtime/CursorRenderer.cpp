#include "pch.h"
#include "CursorRenderer.h"
#include "App.h"
#include "Utils.h"
#include "shaders/MonochromeCursorPS.h"
#include <VertexTypes.h>

extern std::shared_ptr<spdlog::logger> logger;



bool CursorRenderer::Initialize(ComPtr<ID3D11Texture2D> renderTarget, SIZE outputSize) {
	App& app = App::GetInstance();
	Renderer& renderer = app.GetRenderer();
	_d3dDC = renderer.GetD3DDC();
	_d3dDevice = renderer.GetD3DDevice();

	// 限制鼠标在窗口内
	if (!ClipCursor(&App::GetInstance().GetSrcClientRect())) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("ClipCursor 失败"));
	}

	D3D11_TEXTURE2D_DESC rtDesc;
	renderTarget->GetDesc(&rtDesc);

	_destRect.left = (rtDesc.Width - outputSize.cx) / 2;
	_destRect.right = _destRect.left + outputSize.cx;
	_destRect.top = (rtDesc.Height - outputSize.cy) / 2;
	_destRect.bottom = _destRect.top + outputSize.cy;

	_vp.TopLeftX = (FLOAT)_destRect.left;
	_vp.TopLeftY = (FLOAT)_destRect.top;
	_vp.Width = FLOAT(_destRect.right - _destRect.left);
	_vp.Height = FLOAT(_destRect.bottom - _destRect.top);
	_vp.MinDepth = 0.0f;
	_vp.MaxDepth = 1.0f;

	const RECT& srcClient = app.GetSrcClientRect();
	SIZE srcSize = { srcClient.right - srcClient.left, srcClient.bottom - srcClient.top };

	_scaleX = float(_destRect.right - _destRect.left) / srcSize.cx;
	_scaleY = float(_destRect.bottom - _destRect.top) / srcSize.cy;

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

	HRESULT hr = renderer.GetRenderTargetView(renderTarget.Get(), &_rtv);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("GetRenderTargetView 失败", hr));
		return false;
	}

	hr = renderer.GetD3DDevice()->CreatePixelShader(
		MonochromeCursorPSShaderByteCode, sizeof(MonochromeCursorPSShaderByteCode), nullptr, &_monoCursorPS);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 MonochromeCursorPS 失败", hr));
		return false;
	}
	
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VertexPositionTexture) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	hr = renderer.GetD3DDevice()->CreateBuffer(&bd, nullptr, &_vtxBuffer);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("创建顶点缓冲区失败\n\tHRESULT：0x%X", hr));
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

	ComPtr<ID3D11Texture2D> texture;

	if(ii.hbmColor == NULL) {
		// 单色光标
		BITMAP bmp{};
		if (!GetObject(ii.hbmMask, sizeof(bmp), &bmp)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetObject 失败"));
			clear();
			return false;
		}
		result.width = bmp.bmWidth;
		result.height = bmp.bmHeight;

		BITMAPINFO bi{};
		bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bi.bmiHeader.biWidth = result.width;
		bi.bmiHeader.biHeight = -(LONG)result.height;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biCompression = BI_RGB;
		bi.bmiHeader.biBitCount = 32;
		bi.bmiHeader.biSizeImage = result.width * result.height * 4;

		std::vector<BYTE> pixels(bi.bmiHeader.biSizeImage);
		HDC hdc = GetDC(NULL);
		if (GetDIBits(hdc, ii.hbmMask, 0, result.height, &pixels[0], &bi, DIB_RGB_COLORS) != result.height) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDIBits 失败"));
			ReleaseDC(NULL, hdc);
			clear();
			return false;
		}
		ReleaseDC(NULL, hdc);

		// 存在反色部分的光标的纹理由两部分组成：上半部分为单色光标的掩码，红色通道是 AND 掩码，绿色通道是 XOR 掩码
		// 下半部分为光标覆盖部分的纹理，在运行时从原始纹理中复制过来
		const int halfSize = bi.bmiHeader.biSizeImage / 8;
		BYTE* upPtr = &pixels[1];
		BYTE* downPtr = &pixels[static_cast<size_t>(halfSize) * 4];
		for (int i = 0; i < halfSize; ++i) {
			*upPtr = *downPtr;
			// 判断光标是否存在反色部分
			if (!result.hasInv && *(upPtr - 1) == 255 && *upPtr == 255) {
				result.hasInv = true;
			}

			upPtr += 4;
			downPtr += 4;
		}

		if (result.hasInv) {
			D3D11_TEXTURE2D_DESC desc{};
			desc.Width = result.width;
			desc.Height = result.height;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.Usage = D3D11_USAGE_DEFAULT;

			D3D11_SUBRESOURCE_DATA initData{};
			initData.pSysMem = &pixels[0];
			initData.SysMemPitch = result.width * 4;

			HRESULT hr = _d3dDevice->CreateTexture2D(&desc, &initData, &texture);
			if (FAILED(hr)) {
				SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
				return false;
			}

			result.height /= 2;
		}
	}

	clear();

	if(!result.hasInv) {
		// 光标无反色部分，使用 WIC 将光标转换为带 Alpha 通道的图像
		ComPtr<IWICImagingFactory2> wicFactory = App::GetInstance().GetWICImageFactory();
		if (!wicFactory) {
			SPDLOG_LOGGER_ERROR(logger, "获取 WICImageFactory 失败");
			return false;
		}

		ComPtr<IWICBitmap> wicBitmap;
		HRESULT hr = wicFactory->CreateBitmapFromHICON(hCursor, &wicBitmap);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateBitmapFromHICON 失败", hr));
			return false;
		}

		hr = wicBitmap->GetSize(&result.width, &result.height);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("GetSize 失败", hr));
			return false;
		}
		
		std::vector<BYTE> pixels(result.width * result.height * 4);
		hr = wicBitmap->CopyPixels(nullptr, result.width * 4, (UINT)pixels.size(), pixels.data());
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CopyPixels 失败", hr));
			return false;
		}

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

		hr = _d3dDevice->CreateTexture2D(&desc, &initData, &texture);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
			return false;
		}
	}

	HRESULT hr = _d3dDevice->CreateShaderResourceView(texture.Get(), nullptr, &result.texture);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 ShaderResourceView 失败", hr));
		return false;
	}

	return true;
}

void CursorRenderer::Draw() {
	CURSORINFO ci{};
	ci.cbSize = sizeof(ci);
	if (!GetCursorInfo(&ci)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetCursorInfo 失败"));
		return;
	}

	if (!ci.hCursor || ci.flags != CURSOR_SHOWING) {
		return;
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
			return;
		}
	}

	// 映射坐标
	// 鼠标坐标为整数，否则会出现模糊
	const RECT& srcClient = App::GetInstance().GetSrcClientRect();
	POINT targetScreenPos = {
		lroundf((ci.ptScreenPos.x - srcClient.left) * _scaleX) - info->xHotSpot,
		lroundf((ci.ptScreenPos.y - srcClient.top) * _scaleY) - info->yHotSpot
	};

	_d3dDC->OMSetRenderTargets(1, &_rtv, nullptr);
	_d3dDC->RSSetViewports(1, &_vp);
	
	D3D11_MAPPED_SUBRESOURCE ms;
	_d3dDC->Map(_vtxBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);

	float left = targetScreenPos.x / _vp.Width * 2 - 1.0f;
	float top = 1.0f - targetScreenPos.y / _vp.Height * 2;
	float right = left + info->width / _vp.Width * 2;
	float bottom = top - info->height / _vp.Height * 2;

	VertexPositionTexture* data = (VertexPositionTexture*)ms.pData;
	data[0] = { XMFLOAT3(left, top, 0.5f), XMFLOAT2(0.0f, 0.0f) };
	data[1] = { XMFLOAT3(right, top, 0.5f), XMFLOAT2(1.0f, 0.0f) };
	data[2] = { XMFLOAT3(left, bottom, 0.5f), XMFLOAT2(0.0f, 1.0f) };
	data[3] = { XMFLOAT3(right, bottom, 0.5f), XMFLOAT2(1.0f, 1.0f) };

	_d3dDC->Unmap(_vtxBuffer.Get(), 0);

	Renderer& renderer = App::GetInstance().GetRenderer();
	renderer.SetSimpleVS(_vtxBuffer.Get());

	if (!info->hasInv) {
		renderer.SetCopyPS(_pointSam, info->texture.Get());
		renderer.SetAlphaBlend(true);
		_d3dDC->Draw(4, 0);

		renderer.SetAlphaBlend(false);
	} else {
		ComPtr<ID3D11Resource> texture, renderTarget;

		info->texture->GetResource(&texture);
		_rtv->GetResource(&renderTarget);
		D3D11_BOX box {
			targetScreenPos.x + _destRect.left,
			targetScreenPos.y + _destRect.top,
			0,
			targetScreenPos.x + info->width + _destRect.left,
			targetScreenPos.y + info->height + _destRect.top,
			1
		};
		
		_d3dDC->CopySubresourceRegion(texture.Get(), 0, 0, info->height, 0, renderTarget.Get(), 0, &box);

		_d3dDC->PSSetShader(_monoCursorPS.Get(), nullptr, 0);
		_d3dDC->PSSetConstantBuffers(0, 0, nullptr);
		ID3D11ShaderResourceView* t = info->texture.Get();
		_d3dDC->PSSetShaderResources(0, 1, &t);
		_d3dDC->PSSetSamplers(0, 1, &_pointSam);

		_d3dDC->Draw(4, 0);
	}
}
