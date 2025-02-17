#include "pch.h"
#include "CursorDrawer.h"
#include "DeviceResources.h"
#include "Logger.h"
#include "DirectXHelper.h"
#include "ScalingOptions.h"
#include "shaders/SimpleVS.h"
#include "shaders/SimplePS.h"
#include "shaders/MaskedCursorPS.h"
#include "shaders/MonochromeCursorPS.h"
#include <DirectXMath.h>
#include "Win32Helper.h"
#include "ScalingWindow.h"
#include "Renderer.h"
#include "CursorManager.h"
#include "StrHelper.h"

using namespace DirectX;

namespace Magpie {

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
	if (!_isCursorVisible) {
		// 截屏时暂时不渲染光标
		return;
	}

	const CursorManager& cursorManager = ScalingWindow::Get().CursorManager();
	const HCURSOR hCursor = cursorManager.Cursor();

	if (!hCursor) {
		return;
	}

	const _CursorInfo* ci = _ResolveCursor(hCursor);
	if (!ci) {
		return;
	}

	const bool isSrcFocused = ScalingWindow::Get().SrcInfo().IsFocused();
	const POINT cursorPos = cursorManager.CursorPos();

	const ScalingOptions& options = ScalingWindow::Get().Options();
	float cursorScaling = options.cursorScaling;
	if (cursorScaling < 1e-5) {
		// 光标缩放和源窗口相同
		const Renderer& renderer = ScalingWindow::Get().Renderer();
		const SIZE srcSize = Win32Helper::GetSizeOfRect(renderer.SrcRect());
		const SIZE destSize = Win32Helper::GetSizeOfRect(renderer.DestRect());
		cursorScaling = (((float)destSize.cx / srcSize.cx) + ((float)destSize.cy / srcSize.cy)) / 2;
	}

	const SIZE cursorSize{
		lroundf(ci->size.cx * cursorScaling),
		lroundf(ci->size.cy * cursorScaling)
	};
	RECT cursorRect{
		.left = lroundf(cursorPos.x - ci->hotSpot.x * cursorScaling),
		.top = lroundf(cursorPos.y - ci->hotSpot.y * cursorScaling),
		.right = cursorRect.left + cursorSize.cx,
		.bottom = cursorRect.top + cursorSize.cy
	};

	const RECT& swapChainRect = ScalingWindow::Get().SwapChainRect();
	const RECT& destRect = ScalingWindow::Get().Renderer().DestRect();

	const RECT viewportRect{
		isSrcFocused ? destRect.left - swapChainRect.left : 0,
		isSrcFocused ? destRect.top - swapChainRect.top : 0,
		(isSrcFocused ? destRect.right : swapChainRect.right) - swapChainRect.left,
		(isSrcFocused ? destRect.bottom : swapChainRect.bottom) - swapChainRect.top
	};

	if (cursorRect.left >= viewportRect.right ||
		cursorRect.top >= viewportRect.bottom ||
		cursorRect.right <= viewportRect.left ||
		cursorRect.bottom <= viewportRect.top
	) {
		return;
	}

	const SIZE viewportSize = Win32Helper::GetSizeOfRect(viewportRect);
	float left = (cursorRect.left - viewportRect.left) / (float)viewportSize.cx * 2 - 1.0f;
	float top = 1.0f - (cursorRect.top - viewportRect.top) / (float)viewportSize.cy * 2;
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
			(float)viewportRect.left,
			(float)viewportRect.top,
			(float)viewportSize.cx,
			(float)viewportSize.cy,
			0.0f,
			1.0f
		};
		d3dDC->RSSetViewports(1, &vp);
		d3dDC->RSSetState(nullptr);
	}

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

		const bool useBilinear = options.cursorInterpolationMode == CursorInterpolationMode::Bilinear &&
			std::abs(options.cursorScaling - 1.0f) > 1e-3;
		ID3D11SamplerState* cursorSampler = _deviceResources->GetSampler(
			useBilinear ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT,
			D3D11_TEXTURE_ADDRESS_CLAMP
		);
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
			(UINT)std::max(cursorRect.left, viewportRect.left),
			(UINT)std::max(cursorRect.top, viewportRect.top),
			0,
			(UINT)std::min(cursorRect.right, viewportRect.right),
			(UINT)std::min(cursorRect.bottom, viewportRect.bottom),
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

		{
			ID3D11ShaderResourceView* srvs[2]{ _tempCursorTextureRtv.get(), ci->textureSrv.get() };
			d3dDC->PSSetShaderResources(0, 2, srvs);
		}
		
		{
			// 支持双线性插值的单色光标和彩色掩码光标会转换为彩色光标，这里只需要最近邻插值
			ID3D11SamplerState* t = _deviceResources->GetSampler(
				D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
			d3dDC->PSSetSamplers(0, 1, &t);
		}
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

	wil::unique_hbitmap hbmpColor(iconInfo.hbmColor);
	wil::unique_hbitmap hbmpMask(iconInfo.hbmMask);

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
	wil::unique_hdc_window hdcScreen(wil::window_dc(GetDC(NULL)));
	if (GetDIBits(hdcScreen.get(), iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask,
		0, bmp.bmHeight, pixels.get(), &bi, DIB_RGB_COLORS) != bmp.bmHeight
	) {
		Logger::Get().Win32Error("GetDIBits 失败");
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
		for (uint32_t i = 3; i < bi.bmiHeader.biSizeImage; i += 4) {
			if (pixels[i] != 0) {
				hasAlpha = true;
				break;
			}
		}

		if (hasAlpha) {
			// 彩色光标
			cursorInfo.type = _CursorType::Color;

			for (uint32_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
				// 预乘 Alpha 通道
				double alpha = pixels[size_t(i + 3)] / 255.0f;

				uint8_t b = (uint8_t)std::lround(pixels[i] * alpha);
				pixels[i] = (uint8_t)std::lround(pixels[size_t(i + 2)] * alpha);
				pixels[size_t(i + 1)] = (uint8_t)std::lround(pixels[size_t(i + 1)] * alpha);
				pixels[size_t(i + 2)] = b;
				pixels[size_t(i + 3)] = 255 - pixels[size_t(i + 3)];
			}
		} else {
			// 彩色掩码光标
			std::unique_ptr<uint8_t[]> maskPixels(std::make_unique<uint8_t[]>(bi.bmiHeader.biSizeImage));
			if (GetDIBits(hdcScreen.get(), iconInfo.hbmMask, 0, bmp.bmHeight,
				maskPixels.get(), &bi, DIB_RGB_COLORS) != bmp.bmHeight
			) {
				Logger::Get().Win32Error("GetDIBits 失败");
				return nullptr;
			}

			// 计算此彩色掩码光标是否可以转换为彩色光标
			bool canConvertToColor = true;
			for (uint32_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
				if (maskPixels[i] != 0 &&
					(pixels[i] != 0 || pixels[size_t(i + 1)] != 0 || pixels[size_t(i + 2)] != 0)
				) {
					// 掩码不为 0 则不能转换为彩色光标
					canConvertToColor = false;
					break;
				}
			}

			if (canConvertToColor) {
				// 转换为彩色光标以获得更好的插值效果和渲染性能
				cursorInfo.type = _CursorType::Color;

				for (uint32_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
					if (maskPixels[i] == 0) {
						// 保留光标颜色
						// Alpha 通道已经是 0，无需设置
						std::swap(pixels[i], pixels[size_t(i + 2)]);
					} else {
						// 透明像素
						std::memset(&pixels[i], 0, 3);
						pixels[size_t(i + 3)] = 255;
					}
				}
			} else {
				cursorInfo.type = _CursorType::MaskedColor;

				// 将 XOR 掩码复制到透明通道中
				for (uint32_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
					std::swap(pixels[i], pixels[size_t(i + 2)]);
					pixels[size_t(i + 3)] = maskPixels[i];
				}
			}
		}
	} else {
		// 单色光标
		const uint32_t halfSize = bi.bmiHeader.biSizeImage / 2;

		// 计算此单色光标是否可以转换为彩色光标
		bool canConvertToColor = true;
		for (uint32_t i = 0; i < halfSize; i += 4) {
			// 上半部分是 AND 掩码，下半部分是 XOR 掩码
			if (pixels[i] != 0 && pixels[size_t(i + halfSize)] != 0) {
				// 存在反色像素则不能转换为彩色光标
				canConvertToColor = false;
				break;
			}
		}

		if (canConvertToColor) {
			// 转换为彩色光标以获得更好的插值效果和渲染性能
			cursorInfo.type = _CursorType::Color;

			for (uint32_t i = 0; i < halfSize; i += 4) {
				// 上半部分是 AND 掩码，下半部分是 XOR 掩码
				// https://learn.microsoft.com/en-us/windows-hardware/drivers/display/drawing-monochrome-pointers
				if (pixels[i] == 0) {
					if (pixels[size_t(i + halfSize)] == 0) {
						// 黑色
						std::memset(&pixels[i], 0, 4);
					} else {
						// 白色
						std::memset(&pixels[i], 255, 3);
						pixels[size_t(i + 3)] = 0;
					}
				} else {
					// 透明
					std::memset(&pixels[i], 0, 3);
					pixels[size_t(i + 3)] = 255;
				}
			}
		} else {
			cursorInfo.type = _CursorType::Monochrome;

			// 红色通道是 AND 掩码，绿色通道是 XOR 掩码
			// 构造 DXGI_FORMAT_R8G8_UNORM 的初始数据
			uint8_t* upPtr = &pixels[0];
			uint8_t* downPtr = &pixels[halfSize];
			uint8_t* targetPtr = &pixels[0];
			for (uint32_t i = 0; i < halfSize; i += 4) {
				*targetPtr++ = *upPtr;
				*targetPtr++ = *downPtr;

				upPtr += 4;
				downPtr += 4;
			}
		}
	}

	{
		const bool isMonochrome = cursorInfo.type == _CursorType::Monochrome;
		const D3D11_SUBRESOURCE_DATA initData{
			.pSysMem = pixels.get(),
			.SysMemPitch = UINT(bmp.bmWidth * (isMonochrome ? 2 : 4))
		};
		cursorTexture = DirectXHelper::CreateTexture2D(
			d3dDevice,
			isMonochrome ? DXGI_FORMAT_R8G8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM,
			bmp.bmWidth,
			iconInfo.hbmColor ? bmp.bmHeight : bmp.bmHeight / 2,
			D3D11_BIND_SHADER_RESOURCE,
			D3D11_USAGE_IMMUTABLE,
			0,
			&initData
		);
		if (!cursorTexture) {
			Logger::Get().Error("创建光标纹理失败");
			return nullptr;
		}
	}

	HRESULT hr = d3dDevice->CreateShaderResourceView(cursorTexture.get(), nullptr, cursorInfo.textureSrv.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateShaderResourceView 失败", hr);
		return nullptr;
	}

	const char* CURSOR_TYPES[] = { "彩色","彩色掩码","单色" };
	Logger::Get().Info(StrHelper::Concat("已解析", CURSOR_TYPES[(int)cursorInfo.type], "光标"));

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
