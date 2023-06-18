#include "pch.h"
#include "CursorDrawer.h"
#include "DeviceResources.h"
#include "Logger.h"
#include "Utils.h"
#include "DirectXHelper.h"
#include "ScalingOptions.h"
#include "shaders/SimpleVertexShader.h"
#include "shaders/SimplePixelShader.h"
#include <DirectXMath.h>

using namespace DirectX;

namespace Magpie::Core {

struct VertexPositionTexture {
	VertexPositionTexture() = default;

	VertexPositionTexture(const VertexPositionTexture&) = default;
	VertexPositionTexture& operator=(const VertexPositionTexture&) = default;

	VertexPositionTexture(VertexPositionTexture&&) = default;
	VertexPositionTexture& operator=(VertexPositionTexture&&) = default;

	VertexPositionTexture(XMFLOAT3 const& iposition, XMFLOAT2 const& itextureCoordinate) noexcept
		: position(iposition), textureCoordinate(itextureCoordinate) {
	}

	VertexPositionTexture(FXMVECTOR iposition, FXMVECTOR itextureCoordinate) noexcept {
		XMStoreFloat3(&this->position, iposition);
		XMStoreFloat2(&this->textureCoordinate, itextureCoordinate);
	}

	XMFLOAT3 position;
	XMFLOAT2 textureCoordinate;

	static constexpr unsigned int InputElementCount = 2;
	static constexpr D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount] =
	{
		{ "SV_Position", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
};

bool CursorDrawer::Initialize(
	DeviceResources& deviceResources,
	ID3D11Texture2D* backBuffer,
	const RECT& viewportRect,
	const ScalingOptions& options
) noexcept {
	_deviceResources = &deviceResources;
	_backBuffer = backBuffer;
	_viewportRect = viewportRect;
	_cursorScaling = options.cursorScaling;
	_interpolationMode = options.cursorInterpolationMode;

	ID3D11Device* d3dDevice = deviceResources.GetD3DDevice();

	HRESULT hr = d3dDevice->CreateVertexShader(
		SimpleVertexShader, std::size(SimpleVertexShader), nullptr, _simpleVS.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("创建顶点着色器失败", hr);
		return false;
	}

	hr = d3dDevice->CreateInputLayout(
		VertexPositionTexture::InputElements,
		VertexPositionTexture::InputElementCount,
		SimpleVertexShader,
		std::size(SimpleVertexShader),
		_simpleIL.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("创建输入布局失败", hr);
		return false;
	}

	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VertexPositionTexture) * 4;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	hr = d3dDevice->CreateBuffer(&bd, nullptr, _vtxBuffer.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("创建顶点缓冲区失败", hr);
		return false;
	}

	return true;
}

void CursorDrawer::Draw(HCURSOR hCursor, POINT cursorPos) noexcept {
	if (!hCursor) {
		return;
	}

	const _CursorInfo* ci = _ResolveCursor(hCursor);
	if (!ci) {
		return;
	}

	const SIZE cursorSize = { lroundf(ci->size.cx * _cursorScaling), lroundf(ci->size.cy * _cursorScaling) };
	const POINT cursorTopLeft = {
		lroundf(cursorPos.x - ci->hotSpot.x * _cursorScaling),
		lroundf(cursorPos.y - ci->hotSpot.y * _cursorScaling)
	};

	if (cursorTopLeft.x + cursorSize.cx <= _viewportRect.left ||
		cursorTopLeft.y + cursorSize.cy <= _viewportRect.top ||
		cursorTopLeft.x >= _viewportRect.right ||
		cursorTopLeft.y >= _viewportRect.bottom
	) {
		// 光标在窗口外，不应发生这种情况
		return;
	}

	float left = (cursorTopLeft.x - _viewportRect.left) / float(_viewportRect.right - _viewportRect.left) * 2 - 1.0f;
	float top = 1.0f - (cursorTopLeft.y - _viewportRect.top) / float(_viewportRect.bottom - _viewportRect.top) * 2;
	float right = left + cursorSize.cx / float(_viewportRect.right - _viewportRect.left) * 2;
	float bottom = top - cursorSize.cy / float(_viewportRect.bottom - _viewportRect.top) * 2;

	ID3D11DeviceContext* d3dDC = _deviceResources->GetD3DDC();
	d3dDC->IASetInputLayout(_simpleIL.get());
	d3dDC->VSSetShader(_simpleVS.get(), nullptr, 0);

	if (ci->type == _CursorType::Color) {
		// 配置顶点缓冲区
		{
			D3D11_MAPPED_SUBRESOURCE ms;
			HRESULT hr = d3dDC->Map(_vtxBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
			if (FAILED(hr)) {
				Logger::Get().ComError("Map 失败", hr);
				return;
			}

			VertexPositionTexture* data = (VertexPositionTexture*)ms.pData;
			data[0] = { XMFLOAT3(left, top, 0.5f), XMFLOAT2(0.0f, 0.0f) };
			data[1] = { XMFLOAT3(right, top, 0.5f), XMFLOAT2(1.0f, 0.0f) };
			data[2] = { XMFLOAT3(left, bottom, 0.5f), XMFLOAT2(0.0f, 1.0f) };
			data[3] = { XMFLOAT3(right, bottom, 0.5f), XMFLOAT2(1.0f, 1.0f) };

			d3dDC->Unmap(_vtxBuffer.get(), 0);

			ID3D11Buffer* vtxBuffer = _vtxBuffer.get();
			UINT stride = sizeof(VertexPositionTexture);
			UINT offset = 0;
			d3dDC->IASetVertexBuffers(0, 1, &vtxBuffer, &stride, &offset);
		}

		// 配置像素着色器
		if (!_SetSimplePS(ci->texture.get())) {
			return;
		}

		{
			d3dDC->PSSetShader(_simplePS.get(), nullptr, 0);
			d3dDC->PSSetConstantBuffers(0, 0, nullptr);
			ID3D11ShaderResourceView* cursorSRV = _deviceResources->GetShaderResourceView(ci->texture.get());
			d3dDC->PSSetShaderResources(0, 1, &cursorSRV);
			ID3D11SamplerState* cursorSampler = _deviceResources->GetSampler(
				_interpolationMode == CursorInterpolationMode::NearestNeighbor
				? D3D11_FILTER_MIN_MAG_MIP_POINT
				: D3D11_FILTER_MIN_MAG_MIP_LINEAR,
				D3D11_TEXTURE_ADDRESS_CLAMP);
			d3dDC->PSSetSamplers(0, 1, &cursorSampler);
		}
		
		// 配置渲染目标和视口
		{
			ID3D11RenderTargetView* rtv = _deviceResources->GetRenderTargetView(_backBuffer);

			d3dDC->OMSetRenderTargets(1, &rtv, nullptr);
			D3D11_VIEWPORT vp{
				(float)_viewportRect.left,
				(float)_viewportRect.top,
				float(_viewportRect.right - _viewportRect.left),
				float(_viewportRect.bottom - _viewportRect.top),
				0.0f,
				1.0f
			};
			d3dDC->RSSetViewports(1, &vp);
		}

		// 预乘 alpha
		_SetPremultipliedAlphaBlend(true);

		d3dDC->Draw(4, 0);
	}
}

const CursorDrawer::_CursorInfo* CursorDrawer::_ResolveCursor(HCURSOR hCursor) noexcept {
	auto it = _cursorInfos.find(hCursor);
	if (it != _cursorInfos.end()) {
		return &it->second;
	}

	ICONINFO ii{};
	if (!GetIconInfo(hCursor, &ii)) {
		Logger::Get().Win32Error("GetIconInfo 失败");
		return nullptr;
	}

	Utils::ScopeExit se([&ii]() {
		if (ii.hbmColor) {
			DeleteBitmap(ii.hbmColor);
		}
		DeleteBitmap(ii.hbmMask);
	});

	BITMAP bmp{};
	if (!GetObject(ii.hbmMask, sizeof(bmp), &bmp)) {
		Logger::Get().Win32Error("GetObject 失败");
		return nullptr;
	}

	_CursorInfo& ci = _cursorInfos[hCursor];

	ci.hotSpot = { (LONG)ii.xHotspot, (LONG)ii.yHotspot };
	// 单色光标的 hbmMask 高度为实际高度的两倍
	ci.size = { bmp.bmWidth, ii.hbmColor ? bmp.bmHeight : bmp.bmHeight / 2 };

	BITMAPINFO bi{};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = bmp.bmWidth;
	bi.bmiHeader.biHeight = -bmp.bmHeight;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biSizeImage = bmp.bmWidth * bmp.bmHeight * 4;

	if (ii.hbmColor == NULL) {
		// 单色光标
		ci.type = _CursorType::Monochrome;

		std::unique_ptr<BYTE[]> pixels(new BYTE[bi.bmiHeader.biSizeImage]);
		HDC hdc = GetDC(NULL);
		if (GetDIBits(hdc, ii.hbmMask, 0, bmp.bmHeight, pixels.get(), &bi, DIB_RGB_COLORS) != bmp.bmHeight) {
			Logger::Get().Win32Error("GetDIBits 失败");
			ReleaseDC(NULL, hdc);
			return nullptr;
		}
		ReleaseDC(NULL, hdc);

		// 红色通道是 AND 掩码，绿色通道是 XOR 掩码
		// 这里将下半部分的 XOR 掩码复制到上半部分的绿色通道中
		const int halfSize = bi.bmiHeader.biSizeImage / 8;
		BYTE* upPtr = &pixels[1];
		BYTE* downPtr = &pixels[static_cast<size_t>(halfSize) * 4];
		for (int i = 0; i < halfSize; ++i) {
			*upPtr = *downPtr;

			upPtr += 4;
			downPtr += 4;
		}

		D3D11_SUBRESOURCE_DATA initData{};
		initData.pSysMem = pixels.get();
		initData.SysMemPitch = bmp.bmWidth * 4;

		ci.texture = DirectXHelper::CreateTexture2D(
			_deviceResources->GetD3DDevice(),
			DXGI_FORMAT_R8G8B8A8_UNORM,
			bmp.bmWidth,
			bmp.bmHeight / 2,
			D3D11_BIND_SHADER_RESOURCE,
			D3D11_USAGE_IMMUTABLE,
			0,
			&initData
		);
		if (!ci.texture) {
			Logger::Get().Error("创建纹理失败");
			return nullptr;
		}

		return &ci;
	}

	std::unique_ptr<BYTE[]> pixels(new BYTE[bi.bmiHeader.biSizeImage]);
	HDC hdc = GetDC(NULL);
	if (GetDIBits(hdc, ii.hbmColor, 0, bmp.bmHeight, pixels.get(), &bi, DIB_RGB_COLORS) != bmp.bmHeight) {
		Logger::Get().Win32Error("GetDIBits 失败");
		ReleaseDC(NULL, hdc);
		return nullptr;
	}
	ReleaseDC(NULL, hdc);

	// 若颜色掩码有 A 通道，则是彩色光标，否则是彩色掩码光标
	bool hasAlpha = false;
	for (uint32_t i = 3; i < bi.bmiHeader.biSizeImage; i += 4) {
		if (pixels[i] != 0) {
			hasAlpha = true;
			break;
		}
	}

	if (hasAlpha) {
		// 彩色光标
		ci.type = _CursorType::Color;

		for (size_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
			// 预乘 Alpha 通道
			double alpha = pixels[i + 3] / 255.0f;

			BYTE b = (BYTE)std::lround(pixels[i] * alpha);
			pixels[i] = (BYTE)std::lround(pixels[i + 2] * alpha);
			pixels[i + 1] = (BYTE)std::lround(pixels[i + 1] * alpha);
			pixels[i + 2] = b;
			pixels[i + 3] = 255 - pixels[i + 3];
		}
	} else {
		// 彩色掩码光标
		ci.type = _CursorType::MaskedColor;

		std::unique_ptr<BYTE[]> maskPixels(new BYTE[bi.bmiHeader.biSizeImage]);
		hdc = GetDC(NULL);
		if (GetDIBits(hdc, ii.hbmMask, 0, bmp.bmHeight, maskPixels.get(), &bi, DIB_RGB_COLORS) != bmp.bmHeight) {
			Logger::Get().Win32Error("GetDIBits 失败");
			ReleaseDC(NULL, hdc);
			return nullptr;
		}
		ReleaseDC(NULL, hdc);

		// 将 XOR 掩码复制到透明通道中
		for (size_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
			std::swap(pixels[i], pixels[i + 2]);
			pixels[i + 3] = maskPixels[i];
		}
	}

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = &pixels[0];
	initData.SysMemPitch = bmp.bmWidth * 4;

	ci.texture = DirectXHelper::CreateTexture2D(
		_deviceResources->GetD3DDevice(),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		bmp.bmWidth,
		bmp.bmHeight,
		D3D11_BIND_SHADER_RESOURCE,
		D3D11_USAGE_IMMUTABLE,
		0,
		&initData
	);
	if (!ci.texture) {
		Logger::Get().Error("创建纹理失败");
		return nullptr;
	}

	return &ci;
}

bool CursorDrawer::_SetSimplePS(ID3D11Texture2D* cursorTexture) noexcept {
	if (!_simplePS) {
		HRESULT hr = _deviceResources->GetD3DDevice()->CreatePixelShader(
			SimplePixelShader, sizeof(SimplePixelShader), nullptr, _simplePS.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建像素着色器失败", hr);
			return false;
		}
	}

	ID3D11DeviceContext* d3dDC = _deviceResources->GetD3DDC();
	d3dDC->PSSetShader(_simplePS.get(), nullptr, 0);
	d3dDC->PSSetConstantBuffers(0, 0, nullptr);
	ID3D11ShaderResourceView* cursorSRV = _deviceResources->GetShaderResourceView(cursorTexture);
	d3dDC->PSSetShaderResources(0, 1, &cursorSRV);
	ID3D11SamplerState* cursorSampler = _deviceResources->GetSampler(
		_interpolationMode == CursorInterpolationMode::NearestNeighbor
		? D3D11_FILTER_MIN_MAG_MIP_POINT
		: D3D11_FILTER_MIN_MAG_MIP_LINEAR,
		D3D11_TEXTURE_ADDRESS_CLAMP);
	d3dDC->PSSetSamplers(0, 1, &cursorSampler);

	return true;
}

bool CursorDrawer::_SetPremultipliedAlphaBlend(bool enable) noexcept {
	ID3D11DeviceContext* d3dDC = _deviceResources->GetD3DDC();

	if (!enable) {
		d3dDC->OMSetBlendState(nullptr, nullptr, 0xffffffff);
		return true;
	}

	if (!premultipliedAlphaBlendBlendState) {
		// FinalColor = ScreenColor * CursorColor.a + CursorColor
		D3D11_BLEND_DESC desc{};
		desc.RenderTarget[0].BlendEnable = TRUE;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		desc.RenderTarget[0].BlendOp = desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		HRESULT hr = _deviceResources->GetD3DDevice()->CreateBlendState(
			&desc, premultipliedAlphaBlendBlendState.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建混合状态失败", hr);
			return false;
		}
	}

	d3dDC->OMSetBlendState(premultipliedAlphaBlendBlendState.get(), nullptr, 0xffffffff);
	return true;
}

}
