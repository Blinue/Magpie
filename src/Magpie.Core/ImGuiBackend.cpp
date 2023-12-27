// 原始文件: https://github.com/ocornut/imgui/blob/e489e40a853426767de9ce0637bc0c9ceb431c1e/backends/imgui_impl_dx11.cpp

#include "pch.h"
#include "ImGuiBackend.h"
#include <d3dcompiler.h>
#include <imgui.h>
#include "DeviceResources.h"
#include "StrUtils.h"
#include "Logger.h"
#include "DirectXHelper.h"
#include "shaders/ImGuiImplVS.h"
#include "shaders/ImGuiImplPS.h"

namespace Magpie::Core {

struct VERTEX_CONSTANT_BUFFER {
	float mvp[4][4];
};

bool ImGuiBackend::Initialize(DeviceResources* deviceResources) noexcept {
	_deviceResources = deviceResources;

	ImGuiIO& io = ImGui::GetIO();
	io.BackendRendererName = "Magpie";
	// 支持 ImDrawCmd::VtxOffset
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	if (!_CreateDeviceObjects()) {
		Logger::Get().Error("_CreateDeviceObjects 失败");
		return false;
	}

	return true;
}

void ImGuiBackend::_SetupRenderState(ImDrawData* drawData) noexcept {
	ID3D11DeviceContext4* d3dDC = _deviceResources->GetD3DDC();

	D3D11_VIEWPORT vp{
		.Width = drawData->DisplaySize.x,
		.Height = drawData->DisplaySize.y,
		.MinDepth = 0.0f,
		.MaxDepth = 1.0f
	};
	d3dDC->RSSetViewports(1, &vp);

	d3dDC->IASetInputLayout(_inputLayout.get());
	{
		UINT stride = sizeof(ImDrawVert);
		UINT offset = 0;

		ID3D11Buffer* t = _vertexBuffer.get();
		d3dDC->IASetVertexBuffers(0, 1, &t, &stride, &offset);
	}

	d3dDC->IASetIndexBuffer(_indexBuffer.get(),
		sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
	d3dDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	d3dDC->VSSetShader(_vertexShader.get(), nullptr, 0);
	{
		ID3D11Buffer* t = _vertexConstantBuffer.get();
		d3dDC->VSSetConstantBuffers(0, 1, &t);
	}
	d3dDC->PSSetShader(_pixelShader.get(), nullptr, 0);
	{
		// 默认需要线性采样。设置 "io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines" 或
		// "style.AntiAliasedLinesUseTex = false" 来允许最近邻采样
		ID3D11SamplerState* t = _deviceResources->GetSampler(
			D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP);
		d3dDC->PSSetSamplers(0, 1, &t);
	}

	static constexpr float blendFactor[4]{};
	d3dDC->OMSetBlendState(_blendState.get(), blendFactor, 0xffffffff);
	d3dDC->RSSetState(_rasterizerState.get());
}

void ImGuiBackend::RenderDrawData(ImDrawData* drawData) noexcept {
	ID3D11DeviceContext4* d3dDC = _deviceResources->GetD3DDC();
	ID3D11Device5* d3dDevice = _deviceResources->GetD3DDevice();

	// 按需创建和增长顶点和索引缓冲区
	if (!_vertexBuffer || _vertexBufferSize < drawData->TotalVtxCount) {
		_vertexBufferSize = drawData->TotalVtxCount + 5000;

		D3D11_BUFFER_DESC desc{
			.ByteWidth = _vertexBufferSize * sizeof(ImDrawVert),
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
		};
		HRESULT hr = d3dDevice->CreateBuffer(&desc, nullptr, _vertexBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBuffer 失败", hr);
			return;
		}
	}
	if (!_indexBuffer || _indexBufferSize < drawData->TotalIdxCount) {
		_indexBufferSize = drawData->TotalIdxCount + 10000;

		D3D11_BUFFER_DESC desc{
			.ByteWidth = _indexBufferSize * sizeof(ImDrawIdx),
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_INDEX_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
		};
		HRESULT hr = d3dDevice->CreateBuffer(&desc, nullptr, _indexBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBuffer 失败", hr);
			return;
		}
	}

	// 上传顶点数据
	{
		D3D11_MAPPED_SUBRESOURCE vtxResource;
		HRESULT hr = d3dDC->Map(_vertexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &vtxResource);
		if (FAILED(hr)) {
			Logger::Get().ComError("Map 失败", hr);
			return;
		}

		ImDrawVert* vtxDst = (ImDrawVert*)vtxResource.pData;
		for (const ImDrawList* cmdList : drawData->CmdLists) {
			std::memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
			vtxDst += cmdList->VtxBuffer.Size;
		}

		d3dDC->Unmap(_vertexBuffer.get(), 0);
	}
	// 上传索引数据
	{
		D3D11_MAPPED_SUBRESOURCE idxResource;
		HRESULT hr = d3dDC->Map(_indexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &idxResource);
		if (FAILED(hr)) {
			Logger::Get().ComError("Map 失败", hr);
			return;
		}

		ImDrawIdx* idxDst = (ImDrawIdx*)idxResource.pData;
		for (const ImDrawList* cmdList : drawData->CmdLists) {
			std::memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
			idxDst += cmdList->IdxBuffer.Size;
		}

		d3dDC->Unmap(_indexBuffer.get(), 0);
	}

	// Setup orthographic projection matrix into our constant buffer
	// Our visible imgui space lies from drawData->DisplayPos (top left) to drawData->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
	{
		const float left = drawData->DisplayPos.x;
		const float right = drawData->DisplayPos.x + drawData->DisplaySize.x;
		const float top = drawData->DisplayPos.y;
		const float bottom = drawData->DisplayPos.y + drawData->DisplaySize.y;
		const VERTEX_CONSTANT_BUFFER data{
			.mvp{
				{ 2.0f / (right - left), 0.0f, 0.0f, 0.0f },
				{ 0.0f, 2.0f / (top - bottom), 0.0f, 0.0f },
				{ 0.0f, 0.0f, 0.5f, 0.0f },
				{ (right + left) / (left - right), (top + bottom) / (bottom - top), 0.5f, 1.0f },
			}
		};

		D3D11_MAPPED_SUBRESOURCE ms;
		HRESULT hr = d3dDC->Map(_vertexConstantBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		if (FAILED(hr)) {
			Logger::Get().ComError("Map 失败", hr);
			return;
		}
		
		std::memcpy(ms.pData, &data, sizeof(data));
		d3dDC->Unmap(_vertexConstantBuffer.get(), 0);
	}

	_SetupRenderState(drawData);

	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	int globalIdxOffset = 0;
	int globalVtxOffset = 0;
	const ImVec2 clipOff = drawData->DisplayPos;
	for (const ImDrawList* cmdList : drawData->CmdLists) {
		for (const ImDrawCmd& drawCmd : cmdList->CmdBuffer) {
			if (drawCmd.UserCallback) {
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (drawCmd.UserCallback == ImDrawCallback_ResetRenderState) {
					_SetupRenderState(drawData);
				} else {
					drawCmd.UserCallback(cmdList, &drawCmd);
				}
			} else {
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clipMin(drawCmd.ClipRect.x - clipOff.x, drawCmd.ClipRect.y - clipOff.y);
				ImVec2 clipMax(drawCmd.ClipRect.z - clipOff.x, drawCmd.ClipRect.w - clipOff.y);
				if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
					continue;

				// Apply scissor/clipping rectangle
				const D3D11_RECT r = { (LONG)clipMin.x, (LONG)clipMin.y, (LONG)clipMax.x, (LONG)clipMax.y };
				d3dDC->RSSetScissorRects(1, &r);

				// Bind texture, Draw
				ID3D11ShaderResourceView* textureSrv = (ID3D11ShaderResourceView*)drawCmd.GetTexID();
				d3dDC->PSSetShaderResources(0, 1, &textureSrv);
				d3dDC->DrawIndexed(drawCmd.ElemCount, drawCmd.IdxOffset + globalIdxOffset, drawCmd.VtxOffset + globalVtxOffset);
			}
		}
		
		globalIdxOffset += cmdList->IdxBuffer.Size;
		globalVtxOffset += cmdList->VtxBuffer.Size;
	}
}

bool ImGuiBackend::_CreateDeviceObjects() noexcept {
	ID3D11Device5* d3dDevice = _deviceResources->GetD3DDevice();

	HRESULT hr = d3dDevice->CreateVertexShader(ImGuiImplVS, std::size(ImGuiImplVS), nullptr, _vertexShader.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateVertexShader 失败", hr);
		return false;
	}

	static constexpr D3D11_INPUT_ELEMENT_DESC LOCAL_LAYOUT[] = {
		{ "SV_POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = d3dDevice->CreateInputLayout(LOCAL_LAYOUT, 3, ImGuiImplVS, std::size(ImGuiImplVS), _inputLayout.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateInputLayout 失败", hr);
		return false;
	}

	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER),
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
		};
		d3dDevice->CreateBuffer(&desc, nullptr, _vertexConstantBuffer.put());
	}

	hr = d3dDevice->CreatePixelShader(ImGuiImplPS, std::size(ImGuiImplPS), nullptr, _pixelShader.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreatePixelShader 失败", hr);
		return false;
	}

	{
		D3D11_BLEND_DESC desc{
			.AlphaToCoverageEnable = false,
			.RenderTarget{
				D3D11_RENDER_TARGET_BLEND_DESC{
					.BlendEnable = true,
					.SrcBlend = D3D11_BLEND_SRC_ALPHA,
					.DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
					.BlendOp = D3D11_BLEND_OP_ADD,
					.SrcBlendAlpha = D3D11_BLEND_ONE,
					.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
					.BlendOpAlpha = D3D11_BLEND_OP_ADD,
					.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL
				}
			}
		};
		hr = d3dDevice->CreateBlendState(&desc, _blendState.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBlendState 失败", hr);
			return false;
		}
	}

	// 创建光栅化器状态对象
	D3D11_RASTERIZER_DESC desc{
		.FillMode = D3D11_FILL_SOLID,
		.CullMode = D3D11_CULL_NONE,
		.ScissorEnable = true
	};
	hr = d3dDevice->CreateRasterizerState(&desc, _rasterizerState.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateRasterizerState 失败", hr);
		return false;
	}

	return true;
}

bool ImGuiBackend::BuildFonts() noexcept {
	assert(!_fontTextureView);

	// 在第一帧前构建字体纹理
	ID3D11Device5* d3dDevice = _deviceResources->GetD3DDevice();
	ImGuiIO& io = ImGui::GetIO();

	// 字体纹理使用 R8_UNORM 格式
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

	// 上传纹理数据
	const D3D11_SUBRESOURCE_DATA initData{
		.pSysMem = pixels,
		.SysMemPitch = (UINT)width
	};
	winrt::com_ptr<ID3D11Texture2D> texture = DirectXHelper::CreateTexture2D(
		d3dDevice,
		DXGI_FORMAT_R8_UNORM,
		width,
		height,
		D3D11_BIND_SHADER_RESOURCE,
		D3D11_USAGE_DEFAULT,
		0,
		&initData
	);
	if (!texture) {
		Logger::Get().Error("创建字体纹理失败");
		return false;
	}

	HRESULT hr = d3dDevice->CreateShaderResourceView(texture.get(), nullptr, _fontTextureView.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateShaderResourceView 失败", hr);
		return false;
	}

	// 设置纹理 ID
	io.Fonts->SetTexID((ImTextureID)_fontTextureView.get());

	// 清理不再需要的数据降低内存占用
	io.Fonts->ClearTexData();
	return true;
}

}
