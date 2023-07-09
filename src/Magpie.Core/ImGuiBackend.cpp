// 原始文件: https://github.com/ocornut/imgui/blob/e489e40a853426767de9ce0637bc0c9ceb431c1e/backends/imgui_impl_dx11.cpp

#include "pch.h"
#include "ImGuiBackend.h"
#include <d3dcompiler.h>
#include <imgui.h>
#include "DeviceResources.h"
#include "StrUtils.h"
#include "Logger.h"

namespace Magpie::Core {

static constexpr const char* VERTEX_SHADER = R"(
cbuffer vertexBuffer : register(b0) {
	float4x4 ProjectionMatrix;
};

struct VS_INPUT {
	float2 pos : POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input) {
	PS_INPUT output;
	output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
	output.col = input.col;
	output.uv  = input.uv;
	return output;
})";

static constexpr const char* PIXEL_SHADER = R"(
struct PS_INPUT {
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

sampler sampler0;
Texture2D texture0;

float4 main(PS_INPUT input) : SV_Target {
	return input.col * float4(1, 1, 1, texture0.Sample(sampler0, input.uv).r);
})";

struct VERTEX_CONSTANT_BUFFER_DX11 {
	float mvp[4][4];
};

bool ImGuiBackend::Initialize(DeviceResources* deviceResources) noexcept {
	_deviceResources = deviceResources;

	ImGuiIO& io = ImGui::GetIO();
	io.BackendRendererName = "Magpie";
	// 支持 ImDrawCmd::VtxOffset
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	return true;
}

void ImGuiBackend::_SetupRenderState(ImDrawData* drawData) noexcept {
	ID3D11DeviceContext4* d3dDC = _deviceResources->GetD3DDC();

	D3D11_VIEWPORT vp{};
	vp.Width = drawData->DisplaySize.x;
	vp.Height = drawData->DisplaySize.y;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	d3dDC->RSSetViewports(1, &vp);

	d3dDC->IASetInputLayout(_inputLayout.get());
	{
		unsigned int stride = sizeof(ImDrawVert);
		unsigned int offset = 0;

		ID3D11Buffer* t = _vertexBuffer.get();
		d3dDC->IASetVertexBuffers(0, 1, &t, &stride, &offset);
	}

	d3dDC->IASetIndexBuffer(_indexBuffer.get(), sizeof(ImDrawIdx) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0);
	d3dDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	d3dDC->VSSetShader(_vertexShader.get(), nullptr, 0);
	{
		ID3D11Buffer* t = _vertexConstantBuffer.get();
		d3dDC->VSSetConstantBuffers(0, 1, &t);
	}
	d3dDC->PSSetShader(_pixelShader.get(), nullptr, 0);
	{
		ID3D11SamplerState* t = _fontSampler.get();
		d3dDC->PSSetSamplers(0, 1, &t);
	}

	const float blend_factor[4]{};
	d3dDC->OMSetBlendState(_blendState.get(), blend_factor, 0xffffffff);
	d3dDC->RSSetState(_rasterizerState.get());
}

void ImGuiBackend::RenderDrawData(ImDrawData* drawData) noexcept {
	// Avoid rendering when minimized
	if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f) {
		return;
	}

	ID3D11DeviceContext4* d3dDC = _deviceResources->GetD3DDC();
	ID3D11Device5* d3dDevice = _deviceResources->GetD3DDevice();

	HRESULT hr;

	// Create and grow vertex/index buffers if needed
	if (!_vertexBuffer || _vertexBufferSize < drawData->TotalVtxCount) {
		_vertexBufferSize = drawData->TotalVtxCount + 5000;
		D3D11_BUFFER_DESC desc{};
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = _vertexBufferSize * sizeof(ImDrawVert);
		desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		hr = d3dDevice->CreateBuffer(&desc, nullptr, _vertexBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBuffer 失败", hr);
			return;
		}
	}
	if (!_indexBuffer || _indexBufferSize < drawData->TotalIdxCount) {
		_indexBufferSize = drawData->TotalIdxCount + 10000;
		D3D11_BUFFER_DESC desc{};
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = _indexBufferSize * sizeof(ImDrawIdx);
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		hr = d3dDevice->CreateBuffer(&desc, nullptr, _indexBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBuffer 失败", hr);
			return;
		}
	}

	// Upload vertex/index data into a single contiguous GPU buffer
	D3D11_MAPPED_SUBRESOURCE vtxResource, idxResource;
	hr = d3dDC->Map(_vertexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &vtxResource);
	if (FAILED(hr)) {
		Logger::Get().ComError("Map 失败", hr);
		return;
	}

	hr = d3dDC->Map(_indexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &idxResource);
	if (FAILED(hr)) {
		Logger::Get().ComError("Map 失败", hr);
		return;
	}

	ImDrawVert* vtxDst = (ImDrawVert*)vtxResource.pData;
	ImDrawIdx* idxDst = (ImDrawIdx*)idxResource.pData;
	for (int n = 0; n < drawData->CmdListsCount; ++n) {
		const ImDrawList* cmdList = drawData->CmdLists[n];
		std::memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
		std::memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmdList->VtxBuffer.Size;
		idxDst += cmdList->IdxBuffer.Size;
	}
	d3dDC->Unmap(_vertexBuffer.get(), 0);
	d3dDC->Unmap(_indexBuffer.get(), 0);

	// Setup orthographic projection matrix into our constant buffer
	// Our visible imgui space lies from drawData->DisplayPos (top left) to drawData->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		hr = d3dDC->Map(_vertexConstantBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		if (FAILED(hr)) {
			Logger::Get().ComError("Map 失败", hr);
			return;
		}

		VERTEX_CONSTANT_BUFFER_DX11* constant_buffer = (VERTEX_CONSTANT_BUFFER_DX11*)mappedResource.pData;
		float left = drawData->DisplayPos.x;
		float right = drawData->DisplayPos.x + drawData->DisplaySize.x;
		float top = drawData->DisplayPos.y;
		float bottom = drawData->DisplayPos.y + drawData->DisplaySize.y;
		float mvp[4][4] = {
			{ 2.0f / (right - left), 0.0f, 0.0f, 0.0f },
			{ 0.0f, 2.0f / (top - bottom), 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.5f, 0.0f },
			{ (right + left) / (left - right), (top + bottom) / (bottom - top), 0.5f, 1.0f },
		};
		std::memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
		d3dDC->Unmap(_vertexConstantBuffer.get(), 0);
	}

	// Setup desired DX state
	_SetupRenderState(drawData);

	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	int globalIdxOffset = 0;
	int globalVtxOffset = 0;
	ImVec2 clip_off = drawData->DisplayPos;
	for (int n = 0; n < drawData->CmdListsCount; n++) {
		const ImDrawList* cmdList = drawData->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++) {
			const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd_i];
			if (pcmd->UserCallback != nullptr) {
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					_SetupRenderState(drawData);
				else
					pcmd->UserCallback(cmdList, pcmd);
			} else {
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clipMin(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
				ImVec2 clipMax(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
				if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
					continue;

				// Apply scissor/clipping rectangle
				const D3D11_RECT r = { (LONG)clipMin.x, (LONG)clipMin.y, (LONG)clipMax.x, (LONG)clipMax.y };
				d3dDC->RSSetScissorRects(1, &r);

				// Bind texture, Draw
				ID3D11ShaderResourceView* textureSrv = (ID3D11ShaderResourceView*)pcmd->GetTexID();
				d3dDC->PSSetShaderResources(0, 1, &textureSrv);
				d3dDC->DrawIndexed(pcmd->ElemCount, pcmd->IdxOffset + globalIdxOffset, pcmd->VtxOffset + globalVtxOffset);
			}
		}
		globalIdxOffset += cmdList->IdxBuffer.Size;
		globalVtxOffset += cmdList->VtxBuffer.Size;
	}
}

bool ImGuiBackend::_CreateFontsTexture() noexcept {
	ImGuiIO& io = ImGui::GetIO();
	ID3D11Device5* d3dDevice = _deviceResources->GetD3DDevice();

	HRESULT hr;

	// 字体纹理使用 R8_UNORM 格式
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

	// Upload texture to graphics system
	{
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		winrt::com_ptr<ID3D11Texture2D> texture = nullptr;
		D3D11_SUBRESOURCE_DATA subResource{};
		subResource.pSysMem = pixels;
		subResource.SysMemPitch = width;
		hr = d3dDevice->CreateTexture2D(&desc, &subResource, texture.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateTexture2D 失败", hr);
			return false;
		}

		// Create texture view
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = desc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		hr = d3dDevice->CreateShaderResourceView(texture.get(), &srvDesc, _fontTextureView.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateShaderResourceView 失败", hr);
			return false;
		}
	}

	// Store our identifier
	io.Fonts->SetTexID((ImTextureID)_fontTextureView.get());

	// Create texture sampler
	// (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
	{
		D3D11_SAMPLER_DESC desc{};
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		hr = d3dDevice->CreateSamplerState(&desc, _fontSampler.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateSamplerState 失败", hr);
			return false;
		}
	}

	// 清理不再需要的数据降低内存占用
	io.Fonts->ClearTexData();

	return true;
}

bool ImGuiBackend::_CreateDeviceObjects() noexcept {
	ID3D11Device5* d3dDevice = _deviceResources->GetD3DDevice();

	HRESULT hr;

	static winrt::com_ptr<ID3DBlob> vertexShaderBlob;
	if (!vertexShaderBlob) {
		hr = D3DCompile(VERTEX_SHADER, StrUtils::StrLen(VERTEX_SHADER),
			nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, vertexShaderBlob.put(), nullptr);
		if (FAILED(hr)) {
			Logger::Get().ComError("编译顶点着色器失败", hr);
			return false;
		}
	}

	hr = d3dDevice->CreateVertexShader(
		vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(),
		nullptr,
		_vertexShader.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateVertexShader 失败", hr);
		return false;
	}

	static constexpr D3D11_INPUT_ELEMENT_DESC LOCAL_LAYOUT[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)IM_OFFSETOF(ImDrawVert, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)IM_OFFSETOF(ImDrawVert, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr = d3dDevice->CreateInputLayout(LOCAL_LAYOUT, 3,
		vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), _inputLayout.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateInputLayout 失败", hr);
		return false;
	}

	{
		D3D11_BUFFER_DESC desc{};
		desc.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER_DX11);
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		d3dDevice->CreateBuffer(&desc, nullptr, _vertexConstantBuffer.put());
	}

	static winrt::com_ptr<ID3DBlob> pixelShaderBlob;
	if (!pixelShaderBlob) {
		hr = D3DCompile(PIXEL_SHADER, StrUtils::StrLen(PIXEL_SHADER),
			nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, pixelShaderBlob.put(), nullptr);
		if (FAILED(hr)) {
			Logger::Get().ComError("编译像素着色器失败", hr);
			return false;
		}
	}

	hr = d3dDevice->CreatePixelShader(
		pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize(),
		nullptr,
		_pixelShader.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreatePixelShader 失败", hr);
		return false;
	}

	{
		D3D11_BLEND_DESC desc{};
		desc.AlphaToCoverageEnable = false;
		desc.RenderTarget[0].BlendEnable = true;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		hr = d3dDevice->CreateBlendState(&desc, _blendState.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBlendState 失败", hr);
			return false;
		}
	}

	// Create the rasterizer state
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.ScissorEnable = true;
		hr = d3dDevice->CreateRasterizerState(&desc, _rasterizerState.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateRasterizerState 失败", hr);
			return false;
		}
	}

	if (!_CreateFontsTexture()) {
		Logger::Get().Error("_CreateFontsTexture 失败");
		return false;
	}

	return true;
}

void ImGuiBackend::BeginFrame() noexcept {
	if (!_fontSampler) {
		_CreateDeviceObjects();
	}
}

}
