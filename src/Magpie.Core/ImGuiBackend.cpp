// 原始文件: https://github.com/ocornut/imgui/blob/e489e40a853426767de9ce0637bc0c9ceb431c1e/backends/imgui_impl_dx11.cpp

#include "pch.h"
#include "ImGuiBackend.h"
#include <d3dcompiler.h>
#include <imgui.h>
#include "MagApp.h"
#include "DeviceResources.h"

namespace Magpie::Core {

// DirectX11 data
struct ImGui_ImplDX11_Data {
	ID3D11Buffer* pVB = nullptr;
	ID3D11Buffer* pIB = nullptr;
	ID3D11VertexShader* pVertexShader = nullptr;
	ID3D11InputLayout* pInputLayout = nullptr;
	ID3D11Buffer* pVertexConstantBuffer = nullptr;
	ID3D11PixelShader* pPixelShader = nullptr;
	ID3D11SamplerState* pFontSampler = nullptr;
	ID3D11ShaderResourceView* pFontTextureView = nullptr;
	ID3D11RasterizerState* pRasterizerState = nullptr;
	ID3D11BlendState* pBlendState = nullptr;
	ID3D11DepthStencilState* pDepthStencilState = nullptr;
	int VertexBufferSize = 5000;
	int IndexBufferSize = 10000;
};

struct VERTEX_CONSTANT_BUFFER_DX11 {
	float mvp[4][4];
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplDX11_Data* GetBackendData() noexcept {
	return ImGui::GetCurrentContext() ? (ImGui_ImplDX11_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

// Functions
static void SetupRenderState(ImDrawData* draw_data, ID3D11DeviceContext* ctx) noexcept {
	ImGui_ImplDX11_Data* bd = GetBackendData();

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
	ctx->IASetInputLayout(bd->pInputLayout);
	ctx->IASetVertexBuffers(0, 1, &bd->pVB, &stride, &offset);
	ctx->IASetIndexBuffer(bd->pIB, sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ctx->VSSetShader(bd->pVertexShader, nullptr, 0);
	ctx->VSSetConstantBuffers(0, 1, &bd->pVertexConstantBuffer);
	ctx->PSSetShader(bd->pPixelShader, nullptr, 0);
	ctx->PSSetSamplers(0, 1, &bd->pFontSampler);

	// Setup blend state
	const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
	ctx->OMSetBlendState(bd->pBlendState, blend_factor, 0xffffffff);
	ctx->OMSetDepthStencilState(bd->pDepthStencilState, 0);
	ctx->RSSetState(bd->pRasterizerState);
}

// Render function
void ImGuiBackend::RenderDrawData(ImDrawData* draw_data) noexcept {
	// Avoid rendering when minimized
	if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
		return;

	ImGui_ImplDX11_Data* bd = GetBackendData();
	DeviceResources& dr = MagApp::Get().GetDeviceResources();
	ID3D11DeviceContext* ctx = dr.GetD3DDC();
	ID3D11Device5* d3dDevice = dr.GetD3DDevice();

	// Create and grow vertex/index buffers if needed
	if (!bd->pVB || bd->VertexBufferSize < draw_data->TotalVtxCount) {
		if (bd->pVB) { bd->pVB->Release(); bd->pVB = nullptr; }
		bd->VertexBufferSize = draw_data->TotalVtxCount + 5000;
		D3D11_BUFFER_DESC desc;
		memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = bd->VertexBufferSize * sizeof(ImDrawVert);
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		if (d3dDevice->CreateBuffer(&desc, nullptr, &bd->pVB) < 0)
			return;
	}
	if (!bd->pIB || bd->IndexBufferSize < draw_data->TotalIdxCount) {
		if (bd->pIB) { bd->pIB->Release(); bd->pIB = nullptr; }
		bd->IndexBufferSize = draw_data->TotalIdxCount + 10000;
		D3D11_BUFFER_DESC desc;
		memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = bd->IndexBufferSize * sizeof(ImDrawIdx);
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (d3dDevice->CreateBuffer(&desc, nullptr, &bd->pIB) < 0)
			return;
	}

	// Upload vertex/index data into a single contiguous GPU buffer
	D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;
	if (ctx->Map(bd->pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource) != S_OK)
		return;
	if (ctx->Map(bd->pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource) != S_OK)
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
	ctx->Unmap(bd->pVB, 0);
	ctx->Unmap(bd->pIB, 0);

	// Setup orthographic projection matrix into our constant buffer
	// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
	{
		D3D11_MAPPED_SUBRESOURCE mapped_resource;
		if (ctx->Map(bd->pVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
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
		ctx->Unmap(bd->pVertexConstantBuffer, 0);
	}

	// Setup desired DX state
	SetupRenderState(draw_data, ctx);

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
					SetupRenderState(draw_data, ctx);
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

static void CreateFontsTexture() noexcept {
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplDX11_Data* bd = GetBackendData();
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

		ID3D11Texture2D* pTexture = nullptr;
		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = pixels;
		subResource.SysMemPitch = desc.Width;
		subResource.SysMemSlicePitch = 0;
		d3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
		IM_ASSERT(pTexture != nullptr);

		// Create texture view
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		d3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &bd->pFontTextureView);
		pTexture->Release();
	}

	// Store our identifier
	io.Fonts->SetTexID((ImTextureID)bd->pFontTextureView);

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
		d3dDevice->CreateSamplerState(&desc, &bd->pFontSampler);
	}
}

bool ImGuiBackend::CreateDeviceObjects() noexcept {
	ImGui_ImplDX11_Data* bd = GetBackendData();
	if (bd->pFontSampler)
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

		ID3DBlob* vertexShaderBlob;
		if (FAILED(D3DCompile(vertexShader, strlen(vertexShader), nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vertexShaderBlob, nullptr)))
			return false; // NB: Pass ID3DBlob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
		if (d3dDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &bd->pVertexShader) != S_OK) {
			vertexShaderBlob->Release();
			return false;
		}

		// Create the input layout
		D3D11_INPUT_ELEMENT_DESC local_layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		if (d3dDevice->CreateInputLayout(local_layout, 3, vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &bd->pInputLayout) != S_OK) {
			vertexShaderBlob->Release();
			return false;
		}
		vertexShaderBlob->Release();

		// Create the constant buffer
		{
			D3D11_BUFFER_DESC desc;
			desc.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER_DX11);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			d3dDevice->CreateBuffer(&desc, nullptr, &bd->pVertexConstantBuffer);
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

		ID3DBlob* pixelShaderBlob;
		if (FAILED(D3DCompile(pixelShader, strlen(pixelShader), nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &pixelShaderBlob, nullptr)))
			return false; // NB: Pass ID3DBlob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
		if (d3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &bd->pPixelShader) != S_OK) {
			pixelShaderBlob->Release();
			return false;
		}
		pixelShaderBlob->Release();
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
		d3dDevice->CreateBlendState(&desc, &bd->pBlendState);
	}

	// Create the rasterizer state
	{
		D3D11_RASTERIZER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.ScissorEnable = true;
		desc.DepthClipEnable = true;
		d3dDevice->CreateRasterizerState(&desc, &bd->pRasterizerState);
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
		d3dDevice->CreateDepthStencilState(&desc, &bd->pDepthStencilState);
	}

	CreateFontsTexture();

	return true;
}

void ImGuiBackend::InvalidateDeviceObjects() noexcept {
	ImGui_ImplDX11_Data* bd = GetBackendData();

	if (bd->pFontSampler) { bd->pFontSampler->Release(); bd->pFontSampler = nullptr; }
	if (bd->pFontTextureView) { bd->pFontTextureView->Release(); bd->pFontTextureView = nullptr; ImGui::GetIO().Fonts->SetTexID(0); } // We copied data->pFontTextureView to io.Fonts->TexID so let's clear that as well.
	if (bd->pIB) { bd->pIB->Release(); bd->pIB = nullptr; }
	if (bd->pVB) { bd->pVB->Release(); bd->pVB = nullptr; }
	if (bd->pBlendState) { bd->pBlendState->Release(); bd->pBlendState = nullptr; }
	if (bd->pDepthStencilState) { bd->pDepthStencilState->Release(); bd->pDepthStencilState = nullptr; }
	if (bd->pRasterizerState) { bd->pRasterizerState->Release(); bd->pRasterizerState = nullptr; }
	if (bd->pPixelShader) { bd->pPixelShader->Release(); bd->pPixelShader = nullptr; }
	if (bd->pVertexConstantBuffer) { bd->pVertexConstantBuffer->Release(); bd->pVertexConstantBuffer = nullptr; }
	if (bd->pInputLayout) { bd->pInputLayout->Release(); bd->pInputLayout = nullptr; }
	if (bd->pVertexShader) { bd->pVertexShader->Release(); bd->pVertexShader = nullptr; }
}

void ImGuiBackend::NewFrame() noexcept {
	ImGui_ImplDX11_Data* bd = GetBackendData();
	IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplDX11_Init()?");

	if (!bd->pFontSampler)
		CreateDeviceObjects();
}

ImGuiBackend::~ImGuiBackend() noexcept {
	ImGui_ImplDX11_Data* bd = GetBackendData();
	IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
	ImGuiIO& io = ImGui::GetIO();

	InvalidateDeviceObjects();
	io.BackendRendererName = nullptr;
	io.BackendRendererUserData = nullptr;
	IM_DELETE(bd);
}

bool ImGuiBackend::Initialize() noexcept {
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

	// Setup backend capabilities flags
	ImGui_ImplDX11_Data* bd = IM_NEW(ImGui_ImplDX11_Data)();
	io.BackendRendererUserData = (void*)bd;
	io.BackendRendererName = "Magpie";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

	return true;
}

}
