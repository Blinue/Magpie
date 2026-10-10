#include "pch.h"
#include "CommandContext.h"
#include "D3D12Context.h"
#include "DescriptorHeap.h"
#include "DirectXHelper.h"
#include "ImGuiBackend.h"
#include "Logger.h"
#include "shaders/ImGuiVS.h"
#include "shaders/ImGuiVS_SM5.h"
#include "shaders/ImGuiPS.h"
#include "shaders/ImGuiPS_SM5.h"
#include "shaders/ImGuiPS_HDR.h"
#include "shaders/ImGuiPS_HDR_SM5.h"

namespace Magpie {

// 我们使用 SRV 偏移量作为 TexID，但 ImTextureID_Invalid 的默认值 0 是合法的
// 偏移量，因此我们将 ImTextureID_Invalid 重定义为 -1。这是官方支持的做法，见
// ImTextureID_Invalid 的注释。
static_assert(ImTextureID_Invalid == (ImTextureID)-1);

ImGuiBackend::~ImGuiBackend() noexcept {
#ifdef _DEBUG
	if (_d3d12Context) {
		auto& descriptorHeap = _d3d12Context->GetDescriptorHeap();
		for (const auto& pair : _textureDatas) {
			descriptorHeap.Free(pair.first, 1);
		}
	}
#endif
}

void ImGuiBackend::Initialize(D3D12Context& d3d12Context, const ColorInfo& colorInfo) noexcept {
	_d3d12Context = &d3d12Context;
	_colorInfo = colorInfo;

	ImGuiIO& io = ImGui::GetIO();
	io.BackendRendererName = "Magpie";
	// 支持 ImDrawCmd::VtxOffset 和动态更新纹理
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;
}

HRESULT ImGuiBackend::RenderDrawData(
	const ImDrawData& drawData,
	POINT viewportOffset,
	GraphicsContext& graphicsContext,
	uint64_t frameFenceValue,
	uint64_t completedFenceValue
) noexcept {
	// Catch up with texture updates. Most of the times, the list will have 1 element with an OK status, aka nothing to do.
	// (This almost always points to ImGui::GetPlatformIO().Textures[] but is part of ImDrawData to allow overriding or disabling texture updates).
	if (drawData.Textures) {
		for (ImTextureData* texData : *drawData.Textures) {
			if (texData->Status != ImTextureStatus_OK) {
				HRESULT hr = _UpdateTexture(*texData, graphicsContext, frameFenceValue, completedFenceValue);
				if (FAILED(hr)) {
					Logger::Get().ComError("_UpdateTexture 失败", hr);
					return hr;
				}
			}
		}
	}

	ID3D12Device5* device = _d3d12Context->GetDevice();
	_FrameResource* curFrameResource = nullptr;

	// UI 渲染允许很多帧并行以降低延迟，因此按需创建 _FrameResource
	for (_FrameResource& frameResource : _frameResources) {
		if (frameResource.fenceValue <= completedFenceValue) {
			curFrameResource = &frameResource;
		}
	}

	if (!curFrameResource) {
		curFrameResource = &_frameResources.emplace_back();
	}

	curFrameResource->fenceValue = frameFenceValue;

	// 按需创建和增长顶点和索引缓冲区
	if (!curFrameResource->vertexBuffer || curFrameResource->vertexBufferSize < drawData.TotalVtxCount) {
		curFrameResource->vertexBufferSize = drawData.TotalVtxCount + 5000;

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

		D3D12_HEAP_FLAGS heapFlags = _d3d12Context->IsHeapFlagCreateNotZeroedSupported() ?
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

		CD3DX12_RESOURCE_DESC desc =
			CD3DX12_RESOURCE_DESC::Buffer(curFrameResource->vertexBufferSize * sizeof(ImDrawVert));

		HRESULT hr = device->CreateCommittedResource(
			&heapProps,
			heapFlags,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&curFrameResource->vertexBuffer)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommittedResource 失败", hr);
			return hr;
		}

		D3D12_RANGE readRange{};
		hr = curFrameResource->vertexBuffer->Map(0, &readRange, &curFrameResource->vertexBufferData);
		if (FAILED(hr)) {
			Logger::Get().ComError("ID3D12Resource::Map 失败", hr);
			return hr;
		}
	}

	if (!curFrameResource->indexBuffer || curFrameResource->indexBufferSize < drawData.TotalIdxCount) {
		curFrameResource->indexBufferSize = drawData.TotalIdxCount + 10000;

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

		D3D12_HEAP_FLAGS heapFlags = _d3d12Context->IsHeapFlagCreateNotZeroedSupported() ?
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

		CD3DX12_RESOURCE_DESC desc =
			CD3DX12_RESOURCE_DESC::Buffer(curFrameResource->indexBufferSize * sizeof(ImDrawIdx));

		HRESULT hr = device->CreateCommittedResource(
			&heapProps,
			heapFlags,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&curFrameResource->indexBuffer)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommittedResource 失败", hr);
			return hr;
		}

		D3D12_RANGE readRange{};
		hr = curFrameResource->indexBuffer->Map(0, &readRange, &curFrameResource->indexBufferData);
		if (FAILED(hr)) {
			Logger::Get().ComError("ID3D12Resource::Map 失败", hr);
			return hr;
		}
	}

	// 上传顶点和索引数据
	ImDrawVert* vtxDst = (ImDrawVert*)curFrameResource->vertexBufferData;
	ImDrawIdx* idxDst = (ImDrawIdx*)curFrameResource->indexBufferData;
	for (const ImDrawList* drawList : drawData.CmdLists) {
		std::memcpy(vtxDst, drawList->VtxBuffer.Data, drawList->VtxBuffer.Size * sizeof(ImDrawVert));
		std::memcpy(idxDst, drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += drawList->VtxBuffer.Size;
		idxDst += drawList->IdxBuffer.Size;
	}

	uint32_t descriptorTableIdx;
	HRESULT hr = _SetupRenderState(
		drawData, graphicsContext, *curFrameResource, viewportOffset, descriptorTableIdx);
	if (FAILED(hr)) {
		Logger::Get().ComError("_SetupRenderState 失败", hr);
		return hr;
	}

	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	int globalVtxOffset = 0;
	int globalIdxOffset = 0;
	for (const ImDrawList* drawList : drawData.CmdLists) {
		for (const ImDrawCmd& drawCmd : drawList->CmdBuffer) {
			// 不支持 UserCallback

			ImVec2 clipMin(drawCmd.ClipRect.x, drawCmd.ClipRect.y);
			ImVec2 clipMax(drawCmd.ClipRect.z, drawCmd.ClipRect.w);
			if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y) {
				continue;
			}

			graphicsContext.RSSetScissorRect(D3D12_RECT{
				(LONG)clipMin.x + viewportOffset.x,
				(LONG)clipMin.y + viewportOffset.y,
				(LONG)clipMax.x + viewportOffset.x,
				(LONG)clipMax.y + viewportOffset.y
			});

			graphicsContext.SetRootDescriptorTable(descriptorTableIdx, drawCmd.GetTexID());
			graphicsContext.DrawIndexed(drawCmd.ElemCount,
				drawCmd.IdxOffset + globalIdxOffset, drawCmd.VtxOffset + globalVtxOffset);
		}
		
		globalIdxOffset += drawList->IdxBuffer.Size;
		globalVtxOffset += drawList->VtxBuffer.Size;
	}

	return S_OK;
}

void ImGuiBackend::OnColorInfoChanged(const ColorInfo& colorInfo) noexcept {
	const winrt::AdvancedColorKind oldKind = _colorInfo.kind;
	_colorInfo = colorInfo;

	if (colorInfo.kind != oldKind) {
		bool wasHDR = oldKind == winrt::AdvancedColorKind::HighDynamicRange;
		bool isHDR = colorInfo.kind == winrt::AdvancedColorKind::HighDynamicRange;
		if (wasHDR != isHDR) {
			_imguiRootSignature = nullptr;
		}

		_imguiPSO = nullptr;
	}
}

HRESULT ImGuiBackend::_UpdateTexture(
	ImTextureData& texData,
	GraphicsContext& graphicsContext,
	uint64_t frameFenceValue,
	uint64_t completedFenceValue
) noexcept {
	// 只支持 RGBA32
	assert(texData.Format == ImTextureFormat_RGBA32);
	
	ID3D12Device5* device = _d3d12Context->GetDevice();

	if (texData.Status == ImTextureStatus_WantCreate) {
		auto& descriptorHeap = _d3d12Context->GetDescriptorHeap();

		uint32_t srvOffset;
		HRESULT hr = descriptorHeap.Alloc(1, srvOffset);
		if (FAILED(hr)) {
			Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
			return hr;
		}

		texData.SetTexID(srvOffset);

		_TextureData& backendData = _textureDatas.emplace(texData.GetTexID(), _TextureData{}).first->second;

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

		D3D12_HEAP_FLAGS heapFlags = _d3d12Context->IsHeapFlagCreateNotZeroedSupported() ?
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

		CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			texData.Width,
			texData.Height,
			1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_NONE
		);

		hr = device->CreateCommittedResource(
			&heapProps,
			heapFlags,
			&texDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			nullptr,
			IID_PPV_ARGS(&backendData.texture)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommittedResource 失败", hr);
			return hr;
		}

		CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc =
			CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(texDesc.Format, 1);
		device->CreateShaderResourceView(backendData.texture.get(), &srvDesc,
			descriptorHeap.GetCpuHandle(srvOffset));
		// We don't set tex->Status to ImTextureStatus_OK to let the code fallthrough below.
	}

	if (texData.Status == ImTextureStatus_WantCreate || texData.Status == ImTextureStatus_WantUpdates) {
		assert(_textureDatas.contains(texData.GetTexID()));
		_TextureData& backendData = _textureDatas.find(texData.GetTexID())->second;

		// We could use the smaller rect on _WantCreate but using the full rect allows us to clear the texture.
		// FIXME-OPT: Uploading single box even when using ImTextureStatus_WantUpdates. Could use tex->Updates[]
		// - Copy all blocks contiguously in upload buffer.
		// - Barrier before copy, submit all CopyTextureRegion(), barrier after copy.
		PointU uploadPt;
		SizeU uploadSize;
		if (texData.Status == ImTextureStatus_WantCreate) {
			uploadPt = { 0, 0 };
			uploadSize = { (uint32_t)texData.Width, (uint32_t)texData.Height };
		} else {
			uploadPt = { texData.UpdateRect.x, texData.UpdateRect.y };
			uploadSize = { texData.UpdateRect.w, texData.UpdateRect.h };
		}

		uint32_t uploadRowSize = uploadSize.width * (uint32_t)texData.BytesPerPixel;
		uint32_t uploadRowPitch = DirectXHelper::Align(uploadRowSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
		uint32_t requiredBufferSize = uploadRowPitch * uploadSize.height;

		// 寻找最大的空闲缓冲区
		SmallVector<uint32_t> freeBuffers;
		uint32_t maxSizeBufferIdx = std::numeric_limits<uint32_t>::max();
		uint32_t maxBufferSize = 0;
		for (uint32_t i = 0, end = (uint32_t)_uploadBuffers.size(); i < end; ++i) {
			if (_uploadBuffers[i].fenceValue > completedFenceValue) {
				continue;
			}

			freeBuffers.push_back(i);

			uint32_t curBufferSize = _uploadBuffers[i].size;
			if (curBufferSize > maxBufferSize && curBufferSize >= requiredBufferSize) {
				maxBufferSize = curBufferSize;
				maxSizeBufferIdx = i;
			}
		}

		_UploadBuffer* curBuffer;

		if (maxSizeBufferIdx == std::numeric_limits<uint32_t>::max()) {
			_UploadBuffer newBuffer = {
				.size = requiredBufferSize
			};

			CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

			D3D12_HEAP_FLAGS heapFlags = _d3d12Context->IsHeapFlagCreateNotZeroedSupported() ?
				D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

			CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(newBuffer.size);

			HRESULT hr = device->CreateCommittedResource(
				&heapProps,
				heapFlags,
				&bufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&newBuffer.buffer)
			);
			if (FAILED(hr)) {
				Logger::Get().ComError("CreateCommittedResource 失败", hr);
				return hr;
			}

			D3D12_RANGE readRange{};
			hr = newBuffer.buffer->Map(0, &readRange, &newBuffer.bufferData);
			if (FAILED(hr)) {
				Logger::Get().ComError("ID3D12Resource::Map 失败", hr);
				return hr;
			}

			curBuffer = &_uploadBuffers.emplace_back(std::move(newBuffer));
		} else {
			curBuffer = &_uploadBuffers[maxSizeBufferIdx];
		}

		curBuffer->fenceValue = frameFenceValue;

		// 如果需要复制整行且对齐相同则可以简化成一个 memcpy
		if (uploadSize.width == (uint32_t)texData.Width && uploadRowSize == uploadRowPitch) {
			assert(uploadPt.x == 0);
			std::memcpy(curBuffer->bufferData, texData.GetPixelsAt(0, uploadPt.y), requiredBufferSize);
		} else {
			for (uint32_t i = 0; i < uploadSize.height; ++i) {
				std::memcpy(
					(uint8_t*)curBuffer->bufferData + uploadRowPitch * i,
					texData.GetPixelsAt(uploadPt.x, uploadPt.y + i),
					uploadRowSize
				);
			}
		}

		graphicsContext.InsertTransitionBarrier(
			backendData.texture.get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_COPY_DEST
		);

		graphicsContext.CopyTextureRegion(
			CD3DX12_TEXTURE_COPY_LOCATION(backendData.texture.get()),
			uploadPt.x,
			uploadPt.y,
			CD3DX12_TEXTURE_COPY_LOCATION(
				curBuffer->buffer.get(),
				D3D12_PLACED_SUBRESOURCE_FOOTPRINT{
					.Footprint = {
						.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
						.Width = uploadSize.width,
						.Height = uploadSize.height,
						.Depth = 1,
						.RowPitch = uploadRowPitch
					}
				}
			)
		);

		graphicsContext.InsertTransitionBarrier(
			backendData.texture.get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);

		backendData.fenceValue = frameFenceValue;

		// 最后释放空闲缓冲区，防止 curBuffer 失效
		for (auto it = freeBuffers.rbegin(); it != freeBuffers.rend(); ++it) {
			if (*it != maxSizeBufferIdx) {
				_uploadBuffers.erase(_uploadBuffers.begin() + *it);
			}
		}

		texData.SetStatus(ImTextureStatus_OK);
	} else if (texData.Status == ImTextureStatus_WantDestroy) {
		auto it = _textureDatas.find(texData.GetTexID());
		assert(it != _textureDatas.end());

		if (it->second.fenceValue <= completedFenceValue) {
			// 可以安全销毁
			_d3d12Context->GetDescriptorHeap().Free(it->first, 1);
			_textureDatas.erase(it);

			texData.SetTexID(ImTextureID_Invalid);
			texData.SetStatus(ImTextureStatus_Destroyed);
		}
	}

	return S_OK;
}

HRESULT ImGuiBackend::_SetupRenderState(
	const ImDrawData& drawData,
	GraphicsContext& graphicsContext,
	const _FrameResource& curFrameResource,
	POINT viewportOffset,
	uint32_t& descriptorTableIdx
) noexcept {
	if (!_imguiPSO) {
		HRESULT hr = _CreateImGuiPSO();
		if (FAILED(hr)) {
			Logger::Get().ComError("_CreateImGuiPSO 失败", hr);
			return hr;
		}
	}

	graphicsContext.SetPipelineState(_imguiPSO.get());
	graphicsContext.SetRootSignature(_imguiRootSignature.get());

	graphicsContext.RSSetViewportRect(CD3DX12_VIEWPORT(
		(float)viewportOffset.x,
		(float)viewportOffset.y,
		drawData.DisplaySize.x,
		drawData.DisplaySize.y
	));

	graphicsContext.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	graphicsContext.IASetVertexBuffer(D3D12_VERTEX_BUFFER_VIEW{
		.BufferLocation = curFrameResource.vertexBuffer->GetGPUVirtualAddress(),
		.SizeInBytes = curFrameResource.vertexBufferSize * (UINT)sizeof(ImDrawVert),
		.StrideInBytes = sizeof(ImDrawVert)
	});

	graphicsContext.IASetIndexBuffer(D3D12_INDEX_BUFFER_VIEW{
		.BufferLocation = curFrameResource.indexBuffer->GetGPUVirtualAddress(),
		.SizeInBytes = curFrameResource.indexBufferSize * (UINT)sizeof(ImDrawIdx),
		.Format = DXGI_FORMAT_R16_UINT
	});

	{
		// 用于把坐标从屏幕空间转换到裁剪空间
		float scale[2] = { 2.0f / drawData.DisplaySize.x, 2.0f / -drawData.DisplaySize.y };
		graphicsContext.SetRoot32BitConstants(0, 2, scale);
	}

	if (_colorInfo.kind == winrt::AdvancedColorKind::HighDynamicRange) {
		graphicsContext.SetRoot32BitConstants(1, 1, &_colorInfo.sdrWhiteLevel);
		descriptorTableIdx = 2;
	} else {
		descriptorTableIdx = 1;
	}
	
	return S_OK;
}

HRESULT ImGuiBackend::_CreateImGuiPSO() noexcept {
	ID3D12Device5* device = _d3d12Context->GetDevice();
	const bool isScRGB = _colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;
	const bool isHDR = _colorInfo.kind == winrt::AdvancedColorKind::HighDynamicRange;

	if (!_imguiRootSignature) {
		winrt::com_ptr<ID3DBlob> signature;
		
		std::array<D3D12_ROOT_PARAMETER1, 3> rootParams;
		uint32_t curRootParamIdx = 0;

		rootParams[curRootParamIdx++] = D3D12_ROOT_PARAMETER1{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
			.Constants = {
				.ShaderRegister = 0,
				.Num32BitValues = 2
			},
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX
		};

		// HDR 下需要传入 SDR 内容亮度
		if (isHDR) {
			rootParams[curRootParamIdx++] = D3D12_ROOT_PARAMETER1{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
				.Constants = {
					.ShaderRegister = 0,
					.Num32BitValues = 1
				},
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
			};
		}

		CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);

		rootParams[curRootParamIdx++] = D3D12_ROOT_PARAMETER1{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			.DescriptorTable = {
				.NumDescriptorRanges = 1,
				.pDescriptorRanges = &srvRange
			},
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
		};

		// 默认需要线性采样。设置 "io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines" 或
		// "style.AntiAliasedLinesUseTex = false" 来允许最近邻采样。
		D3D12_STATIC_SAMPLER_DESC samplerDesc = {
			.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
			.ShaderRegister = 0,
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
		};
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
			curRootParamIdx, rootParams.data(), 1, &samplerDesc,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		HRESULT hr = D3DX12SerializeVersionedRootSignature(
			&rootSignatureDesc,
			_d3d12Context->GetRootSignatureVersion(),
			signature.put(),
			nullptr
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("D3DX12SerializeVersionedRootSignature 失败", hr);
			return hr;
		}
		
		hr = device->CreateRootSignature(
			0,
			signature->GetBufferPointer(),
			signature->GetBufferSize(),
			IID_PPV_ARGS(&_imguiRootSignature)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateRootSignature 失败", hr);
			return hr;
		}
	}
	
	bool isSM6Supported = _d3d12Context->GetShaderModel() >= D3D_SHADER_MODEL_6_0;

	D3D12_SHADER_BYTECODE psByteCode;
	if (isHDR) {
		psByteCode = DirectXHelper::SelectShader(isSM6Supported, ImGuiPS_HDR, ImGuiPS_HDR_SM5);
	} else {
		psByteCode = DirectXHelper::SelectShader(isSM6Supported, ImGuiPS, ImGuiPS_SM5);
	}

	static D3D12_INPUT_ELEMENT_DESC localLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)offsetof(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (UINT)offsetof(ImDrawVert, uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (UINT)offsetof(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
		.pRootSignature = _imguiRootSignature.get(),
		.VS = DirectXHelper::SelectShader(isSM6Supported, ImGuiVS, ImGuiVS_SM5),
		.PS = psByteCode,
		.BlendState = {
			.RenderTarget = {{
				.BlendEnable = TRUE,
				.SrcBlend = D3D12_BLEND_SRC_ALPHA,
				.DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
				.BlendOp = D3D12_BLEND_OP_ADD,
				.SrcBlendAlpha = D3D12_BLEND_ONE,
				.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA,
				.BlendOpAlpha = D3D12_BLEND_OP_ADD,
				.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
			}}
		},
		.SampleMask = UINT_MAX,
		.RasterizerState = {
			.FillMode = D3D12_FILL_MODE_SOLID,
			.CullMode = D3D12_CULL_MODE_NONE
		},
		.InputLayout = {
			.pInputElementDescs = localLayout,
			.NumElements = (UINT)std::size(localLayout)
		},
		.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		.NumRenderTargets = 1,
		.RTVFormats = { isScRGB ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM_SRGB },
		.SampleDesc = { .Count = 1 }
	};
	HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_imguiPSO));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateGraphicsPipelineState 失败", hr);
		return hr;
	}

	return S_OK;
}

}
