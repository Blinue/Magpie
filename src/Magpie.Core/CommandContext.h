#pragma once
#include "D3D12Context.h"
#include "SmallVector.h"
#include "Logger.h"

namespace Magpie {

class DescriptorHeap;

template <typename T>
class CommandContext {
public:
	CommandContext() noexcept = default;
	CommandContext(const CommandContext&) = delete;
	CommandContext(CommandContext&&) = delete;

	void Initialize(D3D12Context& d3d12Context) noexcept {
		_d3d12Context = &d3d12Context;
		_commandList = d3d12Context.GetCommandList();
	}

	ID3D12GraphicsCommandList* GetCommandList() const noexcept {
		return _commandList;
	}

	HRESULT Execute(ID3D12CommandQueue* commandQueue) noexcept {
		_FlushBarriers();

		HRESULT hr = _commandList->Close();
		if (FAILED(hr)) {
			Logger::Get().ComError("ID3D12GraphicsCommandList::Close 失败", hr);
			return hr;
		}

		commandQueue->ExecuteCommandLists(1, CommandListCast(&_commandList));

		((T*)this)->ClearStateCache();

		return S_OK;
	}

	void SetPipelineState(ID3D12PipelineState* pipelineState) noexcept {
		_commandList->SetPipelineState(pipelineState);
	}

	void SetDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap) noexcept {
		if (descriptorHeap != _curDescriptorHeap) {
			_curDescriptorHeap = descriptorHeap;
			_commandList->SetDescriptorHeaps(1, &descriptorHeap);
		}
	}

	ID3D12DescriptorHeap* GetCurDescriptorHeap() const noexcept {
		return _curDescriptorHeap;
	}

	void InsertTransitionBarrier(
		ID3D12Resource* resource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter
	) noexcept {
#ifdef _DEBUG
		// 检查是否存在冗余的状态转换
		auto it = std::find_if(
			_pendingBarriers.begin(),
			_pendingBarriers.end(),
			[&](const D3D12_RESOURCE_BARRIER& barrier) {
				return barrier.Transition.pResource == resource;
			}
		);
		assert(it == _pendingBarriers.end());
#endif
		_pendingBarriers.push_back(
			CD3DX12_RESOURCE_BARRIER::Transition(resource, stateBefore, stateAfter, 0));
	}

	void CopyBufferRegion(
		ID3D12Resource* destBuffer,
		uint32_t destOffset,
		ID3D12Resource* srcBuffer,
		uint32_t srcOffset,
		uint32_t numBytes,
		bool shouldFlushBarriers
	) noexcept {
		if (shouldFlushBarriers) {
			_FlushBarriers();
		}
		
		_commandList->CopyBufferRegion(destBuffer, destOffset, srcBuffer, srcOffset, numBytes);
	}

	void CopyTextureRegion(
		ID3D12Resource* destResource,
		uint32_t dstX,
		uint32_t dstY,
		ID3D12Resource* srcResource,
		const D3D12_BOX* pSrcBox = nullptr
	) noexcept {
		CopyTextureRegion(
			CD3DX12_TEXTURE_COPY_LOCATION(destResource),
			dstX,
			dstY,
			CD3DX12_TEXTURE_COPY_LOCATION(srcResource),
			pSrcBox
		);
	}

	void CopyTextureRegion(
		const CD3DX12_TEXTURE_COPY_LOCATION& dest,
		uint32_t dstX,
		uint32_t dstY,
		const CD3DX12_TEXTURE_COPY_LOCATION& src,
		const D3D12_BOX* pSrcBox = nullptr
	) noexcept {
		_FlushBarriers();
		_commandList->CopyTextureRegion(&dest, dstX, dstY, 0, &src, pSrcBox);
	}

	void DiscardResource(ID3D12Resource* pResource) noexcept {
		_commandList->DiscardResource(pResource, nullptr);
	}

protected:
	void _ClearStateCache() noexcept {
		_FlushBarriers();

		_curDescriptorHeap = nullptr;
	}

	void _FlushBarriers() noexcept {
		if (!_pendingBarriers.empty()) {
			_commandList->ResourceBarrier(
				(UINT)_pendingBarriers.size(), _pendingBarriers.data());
			_pendingBarriers.clear();
		}
	}

	D3D12Context* _d3d12Context = nullptr;
	ID3D12GraphicsCommandList* _commandList = nullptr;

	ID3D12DescriptorHeap* _curDescriptorHeap = nullptr;

	SmallVector<D3D12_RESOURCE_BARRIER, 0> _pendingBarriers;
};

class ComputeContext : public CommandContext<ComputeContext> {
public:
	void SetRootSignature(ID3D12RootSignature* rootSignature) noexcept;

	void SetRoot32BitConstants(
		uint32_t rootParameterIndex,
		uint32_t constantCount,
		const void* pData
	) noexcept;

	void SetComputeRootConstantBufferView(
		uint32_t rootParameterIndex,
		D3D12_GPU_VIRTUAL_ADDRESS bufferLocation
	) noexcept;

	void SetRootDescriptorTable(
		uint32_t rootParameterIndex,
		uint32_t baseDescriptorOffset
	) noexcept;

	void Dispatch(
		uint32_t threadGroupCountX,
		uint32_t threadGroupCountY = 1,
		uint32_t threadGroupCountZ = 1
	) noexcept;

	void ClearStateCache() noexcept;

private:

};

class GraphicsContext : public CommandContext<GraphicsContext> {
public:
	void SetRootSignature(ID3D12RootSignature* rootSignature) noexcept;

	void SetRoot32BitConstants(
		uint32_t rootParameterIndex,
		uint32_t constantCount,
		const void* pData
	) noexcept;

	void SetRootDescriptorTable(
		uint32_t rootParameterIndex,
		uint32_t baseDescriptorOffset
	) noexcept;

	void IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY trimitiveTopology) noexcept;

	void RSSetViewportAndScissorRect(const D3D12_RECT& rect) noexcept;

	void OMSetRenderTarget(uint32_t rtvDescriptorOffset) noexcept;

	uint32_t OMGetRenderTarget() const noexcept {
		return _curRtvDescriptorOffset;
	}

	void Draw(uint32_t vertexCount) noexcept;

	void ClearStateCache() noexcept;

private:
	D3D12_PRIMITIVE_TOPOLOGY _curTrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	uint32_t _curRtvDescriptorOffset = std::numeric_limits<uint32_t>::max();
};

}
