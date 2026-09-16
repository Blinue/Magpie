#include "pch.h"
#include "CommandContext.h"
#include "D3D12Context.h"
#include "DescriptorHeap.h"
#include "DirectXHelper.h"
#include "ImGuiBackend.h"
#include "Logger.h"
#include "SmallVector.h"

namespace Magpie {

ImGuiBackend::~ImGuiBackend() noexcept {
#ifdef _DEBUG
	auto& descriptorHeap = _d3d12Context->GetDescriptorHeap();
	for (const auto& pair : _textureDatas) {
		descriptorHeap.Free(pair.first, 1);
	}
#endif
}

bool ImGuiBackend::Initialize(D3D12Context& d3d12Context) noexcept {
	_d3d12Context = &d3d12Context;

	ImGuiIO& io = ImGui::GetIO();
	io.BackendRendererName = "Magpie";
	// 支持 ImDrawCmd::VtxOffset
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;

	return true;
}

HRESULT ImGuiBackend::RenderDrawData(
	const ImDrawData& drawData,
	POINT /*viewportOffset*/,
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

	return S_OK;
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

		// 以 SRV 偏移量作为 ID
		texData.SetTexID(srvOffset);

		_TextureData& backendData = _textureDatas.emplace(srvOffset, _TextureData{}).first->second;

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
		assert(_textureDatas.contains((uint32_t)texData.GetTexID()));
		_TextureData& backendData = _textureDatas.find((uint32_t)texData.GetTexID())->second;

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

		_UploadBuffer* curBuffer = nullptr;

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

		// 释放空闲缓冲区
		for (auto it = freeBuffers.rbegin(); it != freeBuffers.rend(); ++it) {
			if (*it != maxSizeBufferIdx) {
				_uploadBuffers.erase(_uploadBuffers.begin() + *it);
			}
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

		texData.SetStatus(ImTextureStatus_OK);
	} else if (texData.Status == ImTextureStatus_WantDestroy) {
		auto it = _textureDatas.find((uint32_t)texData.GetTexID());
		assert(it != _textureDatas.end());

		if (it->second.fenceValue <= completedFenceValue) {
			// 可以安全销毁
			_d3d12Context->GetDescriptorHeap().Free(it->first, 1);
			_textureDatas.erase(it);
		}
	}

	return S_OK;
}

}
