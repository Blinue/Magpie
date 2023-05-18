// 原始文件: https://github.com/ocornut/imgui/blob/e489e40a853426767de9ce0637bc0c9ceb431c1e/backends/imgui_impl_dx11.cpp

#include "pch.h"
#include "ImGuiBackend.h"
#include <d3dcompiler.h>
#include <imgui.h>
#include "MagApp.h"
#include "DeviceResources.h"

namespace Magpie::Core {

struct VERTEX_CONSTANT_BUFFER_DX11 {
	float mvp[4][4];
};

void ImGuiBackend::_SetupRenderState(ImDrawData* draw_data, ID3D11DeviceContext* ctx) noexcept {
	// Setup viewport
	D3D11_VIEWPORT vp{};
	vp.Width = draw_data->DisplaySize.x;
	vp.Height = draw_data->DisplaySize.y;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = vp.TopLeftY = 0;
	ctx->RSSetViewports(1, &vp);

	// Setup shader and vertex buffers
	unsigned int stride = sizeof(ImDrawVert);
	unsigned int offset = 0;
	ctx->IASetInputLayout(_inputLayout.get());
	{
		ID3D11Buffer* t = _vertexBuffer.get();
		ctx->IASetVertexBuffers(0, 1, &t, &stride, &offset);
	}
	
	ctx->IASetIndexBuffer(_indexBuffer.get(), sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ctx->VSSetShader(_vertexShader.get(), nullptr, 0);
	{
		ID3D11Buffer* t = _vertexConstantBuffer.get();
		ctx->VSSetConstantBuffers(0, 1, &t);
	}
	ctx->PSSetShader(_pixelShader.get(), nullptr, 0);
	{
		ID3D11SamplerState* t = _fontSampler.get();
		ctx->PSSetSamplers(0, 1, &t);
	}
	

	// Setup blend state
	const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
	ctx->OMSetBlendState(_blendState.get(), blend_factor, 0xffffffff);
	ctx->OMSetDepthStencilState(_depthStencilState.get(), 0);
	ctx->RSSetState(_rasterizerState.get());
}

void ImGuiBackend::RenderDrawData(ImDrawData* draw_data) noexcept {
	// Avoid rendering when minimized
	if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
		return;

	DeviceResources& dr = MagApp::Get().GetDeviceResources();
	ID3D11DeviceContext4* ctx = dr.GetD3DDC();
	ID3D11Device5* d3dDevice = dr.GetD3DDevice();

	// Create and grow vertex/index buffers if needed
	if (!_vertexBuffer || _vertexBufferSize < draw_data->TotalVtxCount) {
		_vertexBufferSize = draw_data->TotalVtxCount + 5000;
		D3D11_BUFFER_DESC desc{};
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = _vertexBufferSize * sizeof(ImDrawVert);
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		if (d3dDevice->CreateBuffer(&desc, nullptr, _vertexBuffer.put()) < 0)
			return;
	}
	if (!_indexBuffer || _indexBufferSize < draw_data->TotalIdxCount) {
		_indexBufferSize = draw_data->TotalIdxCount + 10000;
		D3D11_BUFFER_DESC desc{};
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = _indexBufferSize * sizeof(ImDrawIdx);
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (d3dDevice->CreateBuffer(&desc, nullptr, _indexBuffer.put()) < 0)
			return;
	}

	// Upload vertex/index data into a single contiguous GPU buffer
	D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;
	if (ctx->Map(_vertexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource) != S_OK)
		return;
	if (ctx->Map(_indexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource) != S_OK)
		return;
	ImDrawVert* vtx_dst = (ImDrawVert*)vtx_resource.pData;
	ImDrawIdx* idx_dst = (ImDrawIdx*)idx_resource.pData;
	for (int n = 0; n < draw_data->CmdListsCount; n++) {
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}
	ctx->Unmap(_vertexBuffer.get(), 0);
	ctx->Unmap(_indexBuffer.get(), 0);

	// Setup orthographic projection matrix into our constant buffer
	// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
	{
		D3D11_MAPPED_SUBRESOURCE mapped_resource;
		if (ctx->Map(_vertexConstantBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
			return;
		VERTEX_CONSTANT_BUFFER_DX11* constant_buffer = (VERTEX_CONSTANT_BUFFER_DX11*)mapped_resource.pData;
		float L = draw_data->DisplayPos.x;
		float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
		float T = draw_data->DisplayPos.y;
		float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
		float mvp[4][4] =
		{
			{ 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
			{ 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
			{ 0.0f,         0.0f,           0.5f,       0.0f },
			{ (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
		};
		memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
		ctx->Unmap(_vertexConstantBuffer.get(), 0);
	}

	// Setup desired DX state
	_SetupRenderState(draw_data, ctx);

	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	int global_idx_offset = 0;
	int global_vtx_offset = 0;
	ImVec2 clip_off = draw_data->DisplayPos;
	for (int n = 0; n < draw_data->CmdListsCount; n++) {
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback != nullptr) {
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					_SetupRenderState(draw_data, ctx);
				else
					pcmd->UserCallback(cmd_list, pcmd);
			} else {
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
				ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
				if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
					continue;

				// Apply scissor/clipping rectangle
				const D3D11_RECT r = { (LONG)clip_min.x, (LONG)clip_min.y, (LONG)clip_max.x, (LONG)clip_max.y };
				ctx->RSSetScissorRects(1, &r);

				// Bind texture, Draw
				ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)pcmd->GetTexID();
				ctx->PSSetShaderResources(0, 1, &texture_srv);
				ctx->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset);
			}
		}
		global_idx_offset += cmd_list->IdxBuffer.Size;
		global_vtx_offset += cmd_list->VtxBuffer.Size;
	}
}

void ImGuiBackend::_CreateFontsTexture() noexcept {
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	ID3D11Device5* d3dDevice = MagApp::Get().GetDeviceResources().GetD3DDevice();

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

	// Upload texture to graphics system
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;

		winrt::com_ptr<ID3D11Texture2D> texture = nullptr;
		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = pixels;
		subResource.SysMemPitch = desc.Width;
		subResource.SysMemSlicePitch = 0;
		d3dDevice->CreateTexture2D(&desc, &subResource, texture.put());

		// Create texture view
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		d3dDevice->CreateShaderResourceView(texture.get(), &srvDesc, pFontTextureView.put());
	}

	// Store our identifier
	io.Fonts->SetTexID((ImTextureID)pFontTextureView.get());

	// Create texture sampler
	// (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
	{
		D3D11_SAMPLER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.MipLODBias = 0.f;
		desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		desc.MinLOD = 0.f;
		desc.MaxLOD = 0.f;
		d3dDevice->CreateSamplerState(&desc, _fontSampler.put());
	}
}

bool ImGuiBackend::CreateDeviceObjects() noexcept {
	if (_fontSampler)
		InvalidateDeviceObjects();

	ID3D11Device5* d3dDevice = MagApp::Get().GetDeviceResources().GetD3DDevice();

	// By using D3DCompile() from <d3dcompiler.h> / d3dcompiler.lib, we introduce a dependency to a given version of d3dcompiler_XX.dll (see D3DCOMPILER_DLL_A)
	// If you would like to use this DX11 sample code but remove this dependency you can:
	//  1) compile once, save the compiled shader blobs into a file or source code and pass them to CreateVertexShader()/CreatePixelShader() [preferred solution]
	//  2) use code to detect any version of the DLL and grab a pointer to D3DCompile from the DLL.
	// See https://github.com/ocornut/imgui/pull/638 for sources and details.

	// Create the vertex shader
	{
		static const char* vertexShader =
			"cbuffer vertexBuffer : register(b0) \
			{\
			  float4x4 ProjectionMatrix; \
			};\
			struct VS_INPUT\
			{\
			  float2 pos : POSITION;\
			  float4 col : COLOR0;\
			  float2 uv  : TEXCOORD0;\
			};\
			\
			struct PS_INPUT\
			{\
			  float4 pos : SV_POSITION;\
			  float4 col : COLOR0;\
			  float2 uv  : TEXCOORD0;\
			};\
			\
			PS_INPUT main(VS_INPUT input)\
			{\
			  PS_INPUT output;\
			  output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
			  output.col = input.col;\
			  output.uv  = input.uv;\
			  return output;\
			}";

		winrt::com_ptr<ID3DBlob> vertexShaderBlob;
		if (FAILED(D3DCompile(vertexShader, strlen(vertexShader), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, vertexShaderBlob.put(), nullptr)))
			return false; // NB: Pass ID3DBlob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer().
		if (d3dDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, _vertexShader.put()) != S_OK) {
			return false;
		}

		// Create the input layout
		D3D11_INPUT_ELEMENT_DESC local_layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		if (d3dDevice->CreateInputLayout(local_layout, 3, vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), _inputLayout.put()) != S_OK) {
			return false;
		}

		// Create the constant buffer
		{
			D3D11_BUFFER_DESC desc;
			desc.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER_DX11);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			d3dDevice->CreateBuffer(&desc, nullptr, _vertexConstantBuffer.put());
		}
	}

	// Create the pixel shader
	{
		static const char* pixelShader =
			"struct PS_INPUT\
			{\
			float4 pos : SV_POSITION;\
			float4 col : COLOR0;\
			float2 uv  : TEXCOORD0;\
			};\
			sampler sampler0;\
			Texture2D texture0;\
			\
			float4 main(PS_INPUT input) : SV_Target\
			{\
			float4 out_col = input.col * float4(1,1,1,texture0.Sample(sampler0, input.uv).a); \
			return out_col; \
			}";

		winrt::com_ptr<ID3DBlob> pixelShaderBlob;
		if (FAILED(D3DCompile(pixelShader, strlen(pixelShader), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, pixelShaderBlob.put(), nullptr)))
			return false; // NB: Pass ID3DBlob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer().
		if (d3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, _pixelShader.put()) != S_OK) {
			return false;
		}
	}

	// Create the blending setup
	{
		D3D11_BLEND_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.AlphaToCoverageEnable = false;
		desc.RenderTarget[0].BlendEnable = true;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		d3dDevice->CreateBlendState(&desc, _blendState.put());
	}

	// Create the rasterizer state
	{
		D3D11_RASTERIZER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.ScissorEnable = true;
		desc.DepthClipEnable = true;
		d3dDevice->CreateRasterizerState(&desc, _rasterizerState.put());
	}

	// Create depth-stencil State
	{
		D3D11_DEPTH_STENCIL_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.DepthEnable = false;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		desc.StencilEnable = false;
		desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		desc.BackFace = desc.FrontFace;
		d3dDevice->CreateDepthStencilState(&desc, _depthStencilState.put());
	}

	_CreateFontsTexture();

	return true;
}

void ImGuiBackend::InvalidateDeviceObjects() noexcept {
	_fontSampler = nullptr;
	
	if (pFontTextureView) {
		pFontTextureView = nullptr;
		// We copied data->pFontTextureView to io.Fonts->TexID so let's clear that as well.
		ImGui::GetIO().Fonts->SetTexID(0);
	}
	
	_indexBuffer = nullptr;
	_vertexBuffer = nullptr;
	_blendState = nullptr;
	_depthStencilState = nullptr;
	_rasterizerState = nullptr;
	_pixelShader = nullptr;
	_vertexConstantBuffer = nullptr;
	_inputLayout = nullptr;
	_vertexShader = nullptr;
}

void ImGuiBackend::NewFrame() noexcept {
	if (!_fontSampler)
		CreateDeviceObjects();
}

bool ImGuiBackend::Initialize() noexcept {
	// Setup backend capabilities flags
	ImGuiIO& io = ImGui::GetIO();
	io.BackendRendererName = "Magpie";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

	return true;
}

}
