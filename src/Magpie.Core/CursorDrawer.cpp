#include "pch.h"
#include "CursorDrawer.h"
#include "DeviceResources.h"
#include "Logger.h"
#include "Utils.h"
#include "DirectXHelper.h"
#include "ScalingOptions.h"
#include "shaders/SimpleVS.h"
#include "shaders/SimplePS.h"
#include "shaders/MaskedCursorPS.h"
#include "shaders/MonochromeCursorPS.h"
#include <DirectXMath.h>
#include "Win32Utils.h"
#include "ScalingWindow.h"
#include "Renderer.h"
#include "CursorManager.h"

using namespace DirectX;

namespace Magpie::Core {

struct VertexPositionTexture {
	VertexPositionTexture() = default;

	VertexPositionTexture(const VertexPositionTexture&) = default;
	VertexPositionTexture& operator=(const VertexPositionTexture&) = default;

	VertexPositionTexture(VertexPositionTexture&&) = default;
	VertexPositionTexture& operator=(VertexPositionTexture&&) = default;

	VertexPositionTexture(XMFLOAT2 const& iposition, XMFLOAT2 const& itextureCoordinate) noexcept
		: position(iposition), textureCoordinate(itextureCoordinate) {
	}

	VertexPositionTexture(FXMVECTOR iposition, FXMVECTOR itextureCoordinate) noexcept {
		XMStoreFloat2(&this->position, iposition);
		XMStoreFloat2(&this->textureCoordinate, itextureCoordinate);
	}

	XMFLOAT2 position;
	XMFLOAT2 textureCoordinate;

	static constexpr D3D11_INPUT_ELEMENT_DESC InputElements[] =
	{
		{ "SV_POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
};

bool CursorDrawer::Initialize(DeviceResources& deviceResources, ID3D11Texture2D* backBuffer) noexcept {
	_deviceResources = &deviceResources;
	_backBuffer = backBuffer;

	const RECT& scalingWndRect = ScalingWindow::Get().WndRect();
	const RECT& destRect = ScalingWindow::Get().Renderer().DestRect();

	_viewportRect = {
		destRect.left - scalingWndRect.left,
		destRect.top - scalingWndRect.top,
		destRect.right - scalingWndRect.left,
		destRect.bottom - scalingWndRect.top
	};

	ID3D11Device* d3dDevice = deviceResources.GetD3DDevice();

	HRESULT hr = d3dDevice->CreateVertexShader(
		SimpleVS, std::size(SimpleVS), nullptr, _simpleVS.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("创建顶点着色器失败", hr);
		return false;
	}

	hr = d3dDevice->CreateInputLayout(
		VertexPositionTexture::InputElements,
		(UINT)std::size(VertexPositionTexture::InputElements),
		SimpleVS,
		std::size(SimpleVS),
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

void CursorDrawer::Draw() noexcept {
	const CursorManager& cursorManager = ScalingWindow::Get().CursorManager();
	const HCURSOR hCursor = cursorManager.Cursor();

	if (!hCursor) {
		return;
	}

	const _CursorInfo* ci = _ResolveCursor(hCursor);
	if (!ci) {
		return;
	}

	const POINT cursorPos = cursorManager.CursorPos();

	const float cursorScaling = ScalingWindow::Get().Options().cursorScaling;
	const SIZE cursorSize{ lroundf(ci->size.cx * cursorScaling), lroundf(ci->size.cy * cursorScaling) };
	RECT cursorRect{
		.left = lroundf(cursorPos.x - ci->hotSpot.x * cursorScaling),
		.top = lroundf(cursorPos.y - ci->hotSpot.y * cursorScaling),
		.right = cursorRect.left + cursorSize.cx,
		.bottom = cursorRect.top + cursorSize.cy
	};

	if (cursorRect.left >= _viewportRect.right ||
		cursorRect.top >= _viewportRect.bottom ||
		cursorRect.right <= _viewportRect.left ||
		cursorRect.bottom <= _viewportRect.top
	) {
		// 光标在窗口外，不应发生这种情况
		return;
	}

	const SIZE viewportSize = Win32Utils::GetSizeOfRect(_viewportRect);
	float left = (cursorRect.left - _viewportRect.left) / (float)viewportSize.cx * 2 - 1.0f;
	float top = 1.0f - (cursorRect.top - _viewportRect.top) / (float)viewportSize.cy * 2;
	float right = left + cursorSize.cx / (float)viewportSize.cx * 2;
	float bottom = top - cursorSize.cy / (float)viewportSize.cy * 2;

	ID3D11DeviceContext* d3dDC = _deviceResources->GetD3DDC();
	d3dDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	d3dDC->IASetInputLayout(_simpleIL.get());
	d3dDC->VSSetShader(_simpleVS.get(), nullptr, 0);

	// 配置顶点缓冲区
	{
		const VertexPositionTexture data[] = {
			{ XMFLOAT2(left, top), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT2(right, top), XMFLOAT2(1.0f, 0.0f) },
			{ XMFLOAT2(left, bottom), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT2(right, bottom), XMFLOAT2(1.0f, 1.0f) }
		};

		D3D11_MAPPED_SUBRESOURCE ms;
		HRESULT hr = d3dDC->Map(_vtxBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		if (FAILED(hr)) {
			Logger::Get().ComError("Map 失败", hr);
			return;
		}

		std::memcpy(ms.pData, data, sizeof(data));
		d3dDC->Unmap(_vtxBuffer.get(), 0);

		ID3D11Buffer* vtxBuffer = _vtxBuffer.get();
		UINT stride = sizeof(VertexPositionTexture);
		UINT offset = 0;
		d3dDC->IASetVertexBuffers(0, 1, &vtxBuffer, &stride, &offset);
	}

	// 配置渲染视口
	{
		D3D11_VIEWPORT vp{
			(float)_viewportRect.left,
			(float)_viewportRect.top,
			(float)viewportSize.cx,
			(float)viewportSize.cy,
			0.0f,
			1.0f
		};
		d3dDC->RSSetViewports(1, &vp);
		d3dDC->RSSetState(nullptr);
	}

	CursorInterpolationMode interpolationMode = ScalingWindow::Get().Options().cursorInterpolationMode;

	if (ci->type == _CursorType::Color) {
		// 配置像素着色器
		if (!_simplePS) {
			HRESULT hr = _deviceResources->GetD3DDevice()->CreatePixelShader(
				SimplePS, sizeof(SimplePS), nullptr, _simplePS.put());
			if (FAILED(hr)) {
				Logger::Get().ComError("创建像素着色器失败", hr);
				return;
			}
		}

		d3dDC->PSSetShader(_simplePS.get(), nullptr, 0);
		d3dDC->PSSetConstantBuffers(0, 0, nullptr);
		ID3D11ShaderResourceView* cursorSrv = ci->textureSrv.get();
		d3dDC->PSSetShaderResources(0, 1, &cursorSrv);
		ID3D11SamplerState* cursorSampler = _deviceResources->GetSampler(
			interpolationMode == CursorInterpolationMode::NearestNeighbor
			? D3D11_FILTER_MIN_MAG_MIP_POINT
			: D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			D3D11_TEXTURE_ADDRESS_CLAMP);
		d3dDC->PSSetSamplers(0, 1, &cursorSampler);

		// 预乘 alpha
		_SetPremultipliedAlphaBlend();
	} else {
		if (_tempCursorTextureSize != cursorSize) {
			_tempCursorTexture = nullptr;
			_tempCursorTextureRtv = nullptr;

			ID3D11Device* d3dDevice = _deviceResources->GetD3DDevice();

			// 创建临时纹理，如果光标尺寸变了则重新创建
			_tempCursorTexture = DirectXHelper::CreateTexture2D(
				d3dDevice,
				DXGI_FORMAT_R8G8B8A8_UNORM,
				cursorSize.cx,
				cursorSize.cy,
				D3D11_BIND_SHADER_RESOURCE
			);
			if (!_tempCursorTexture) {
				Logger::Get().Error("创建光标纹理失败");
				return;
			}

			HRESULT hr = d3dDevice->CreateShaderResourceView(
				_tempCursorTexture.get(), nullptr, _tempCursorTextureRtv.put());
			if (FAILED(hr)) {
				Logger::Get().ComError("CreateShaderResourceView 失败", hr);
				_tempCursorTexture = nullptr;
				return;
			}

			_tempCursorTextureSize = cursorSize;
		}

		D3D11_BOX srcBox{
			(UINT)std::max(cursorRect.left, _viewportRect.left),
			(UINT)std::max(cursorRect.top, _viewportRect.top),
			0,
			(UINT)std::min(cursorRect.right, _viewportRect.right),
			(UINT)std::min(cursorRect.bottom, _viewportRect.bottom),
			1
		};
		d3dDC->CopySubresourceRegion(
			_tempCursorTexture.get(),
			0,
			srcBox.left - cursorRect.left,
			srcBox.top - cursorRect.top,
			0,
			_backBuffer,
			0,
			&srcBox
		);

		if (ci->type == _CursorType::MaskedColor) {
			if (!_maskedCursorPS) {
				HRESULT hr = _deviceResources->GetD3DDevice()->CreatePixelShader(
					MaskedCursorPS, sizeof(MaskedCursorPS), nullptr, _maskedCursorPS.put());
				if (FAILED(hr)) {
					Logger::Get().ComError("创建像素着色器失败", hr);
					return;
				}
			}
			d3dDC->PSSetShader(_maskedCursorPS.get(), nullptr, 0);
		} else {
			if (!_monochromeCursorPS) {
				HRESULT hr = _deviceResources->GetD3DDevice()->CreatePixelShader(
					MonochromeCursorPS, sizeof(MonochromeCursorPS), nullptr, _monochromeCursorPS.put());
				if (FAILED(hr)) {
					Logger::Get().ComError("创建像素着色器失败", hr);
					return;
				}
			}
			d3dDC->PSSetShader(_monochromeCursorPS.get(), nullptr, 0);
		}

		d3dDC->PSSetConstantBuffers(0, 0, nullptr);

		ID3D11ShaderResourceView* srvs[2]{ _tempCursorTextureRtv.get(), ci->textureSrv.get() };
		d3dDC->PSSetShaderResources(0, 2, srvs);

		ID3D11SamplerState* samplers[2];
		samplers[0] = _deviceResources->GetSampler(
			D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
		if (interpolationMode == CursorInterpolationMode::NearestNeighbor) {
			samplers[1] = samplers[0];
		} else {
			samplers[1] = _deviceResources->GetSampler(
				D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP);
		}
		d3dDC->PSSetSamplers(0, 2, samplers);
	}

	d3dDC->Draw(4, 0);
}

const CursorDrawer::_CursorInfo* CursorDrawer::_ResolveCursor(HCURSOR hCursor) noexcept {
	if (auto it = _cursorInfos.find(hCursor); it != _cursorInfos.end()) {
		return &it->second;
	}

	ICONINFO iconInfo{};
	if (!GetIconInfo(hCursor, &iconInfo)) {
		Logger::Get().Win32Error("GetIconInfo 失败");
		return nullptr;
	}

	Utils::ScopeExit se([&iconInfo]() {
		if (iconInfo.hbmColor) {
			DeleteBitmap(iconInfo.hbmColor);
		}
		DeleteBitmap(iconInfo.hbmMask);
	});

	BITMAP bmp{};
	if (!GetObject(iconInfo.hbmMask, sizeof(bmp), &bmp)) {
		Logger::Get().Win32Error("GetObject 失败");
		return nullptr;
	}

	// 获取位图数据
	BITMAPINFO bi{
		.bmiHeader{
			.biSize = sizeof(BITMAPINFOHEADER),
			.biWidth = bmp.bmWidth,
			.biHeight = -bmp.bmHeight,
			.biPlanes = 1,
			.biBitCount = 32,
			.biCompression = BI_RGB,
			.biSizeImage = DWORD(bmp.bmWidth * bmp.bmHeight * 4)
		}
	};

	std::unique_ptr<uint8_t[]> pixels(std::make_unique<uint8_t[]>(bi.bmiHeader.biSizeImage));
	HDC hdcScreen = GetDC(NULL);
	if (GetDIBits(hdcScreen, iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask,
		0, bmp.bmHeight, pixels.get(), &bi, DIB_RGB_COLORS) != bmp.bmHeight
		) {
		Logger::Get().Win32Error("GetDIBits 失败");
		ReleaseDC(NULL, hdcScreen);
		return nullptr;
	}

	_CursorInfo cursorInfo{
		.hotSpot = { (LONG)iconInfo.xHotspot, (LONG)iconInfo.yHotspot },
		// 单色光标的 hbmMask 高度为实际高度的两倍
		.size = { bmp.bmWidth, iconInfo.hbmColor ? bmp.bmHeight : bmp.bmHeight / 2 }
	};
	winrt::com_ptr<ID3D11Texture2D> cursorTexture;

	ID3D11Device* d3dDevice = _deviceResources->GetD3DDevice();

	if (iconInfo.hbmColor) {
		// 彩色光标或彩色掩码光标

		// 若颜色掩码有 A 通道，则是彩色光标，否则是彩色掩码光标
		bool hasAlpha = false;
		for (DWORD i = 3; i < bi.bmiHeader.biSizeImage; i += 4) {
			if (pixels[i] != 0) {
				hasAlpha = true;
				break;
			}
		}

		if (hasAlpha) {
			// 彩色光标
			cursorInfo.type = _CursorType::Color;

			for (size_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
				// 预乘 Alpha 通道
				double alpha = pixels[i + 3] / 255.0f;

				uint8_t b = (uint8_t)std::lround(pixels[i] * alpha);
				pixels[i] = (uint8_t)std::lround(pixels[i + 2] * alpha);
				pixels[i + 1] = (uint8_t)std::lround(pixels[i + 1] * alpha);
				pixels[i + 2] = b;
				pixels[i + 3] = 255 - pixels[i + 3];
			}
		} else {
			// 彩色掩码光标
			cursorInfo.type = _CursorType::MaskedColor;

			std::unique_ptr<uint8_t[]> maskPixels(std::make_unique<uint8_t[]>(bi.bmiHeader.biSizeImage));
			if (GetDIBits(hdcScreen, iconInfo.hbmMask, 0, bmp.bmHeight, maskPixels.get(), &bi, DIB_RGB_COLORS) != bmp.bmHeight) {
				Logger::Get().Win32Error("GetDIBits 失败");
				ReleaseDC(NULL, hdcScreen);
				return nullptr;
			}

			// 将 XOR 掩码复制到透明通道中
			for (size_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
				std::swap(pixels[i], pixels[i + 2]);
				pixels[i + 3] = maskPixels[i];
			}
		}

		const D3D11_SUBRESOURCE_DATA initData{
			.pSysMem = pixels.get(),
			.SysMemPitch = UINT(bmp.bmWidth * 4)
		};

		cursorTexture = DirectXHelper::CreateTexture2D(
			d3dDevice,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			bmp.bmWidth,
			bmp.bmHeight,
			D3D11_BIND_SHADER_RESOURCE,
			D3D11_USAGE_IMMUTABLE,
			0,
			&initData
		);
	} else {
		// 单色光标
		cursorInfo.type = _CursorType::Monochrome;

		// 红色通道是 AND 掩码，绿色通道是 XOR 掩码
		// 构造 DXGI_FORMAT_R8G8_UNORM 的初始数据
		const int halfSize = bi.bmiHeader.biSizeImage / 8;
		uint8_t* upPtr = &pixels[0];
		uint8_t* downPtr = &pixels[(size_t)halfSize * 4];
		uint8_t* targetPtr = &pixels[0];
		for (int i = 0; i < halfSize; ++i) {
			*targetPtr++ = *upPtr;
			*targetPtr++ = *downPtr;

			upPtr += 4;
			downPtr += 4;
		}

		const D3D11_SUBRESOURCE_DATA initData{
			.pSysMem = pixels.get(),
			.SysMemPitch = UINT(bmp.bmWidth * 2)
		};
		
		cursorTexture = DirectXHelper::CreateTexture2D(
			d3dDevice,
			DXGI_FORMAT_R8G8_UNORM,
			bmp.bmWidth,
			bmp.bmHeight / 2,
			D3D11_BIND_SHADER_RESOURCE,
			D3D11_USAGE_IMMUTABLE,
			0,
			&initData
		);
	}

	ReleaseDC(NULL, hdcScreen);

	if (!cursorTexture) {
		Logger::Get().Error("创建光标纹理失败");
		return nullptr;
	}

	HRESULT hr = d3dDevice->CreateShaderResourceView(cursorTexture.get(), nullptr, cursorInfo.textureSrv.put());
	if (FAILED(hr)) {
		return nullptr;
	}

	return &_cursorInfos.emplace(hCursor, std::move(cursorInfo)).first->second;
}

bool CursorDrawer::_SetPremultipliedAlphaBlend() noexcept {
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

	_deviceResources->GetD3DDC()->OMSetBlendState(premultipliedAlphaBlendBlendState.get(), nullptr, 0xffffffff);
	return true;
}

}
