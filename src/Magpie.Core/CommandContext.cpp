#include "pch.h"
#include "CommandContext.h"
#include "DescriptorHeap.h"

namespace Magpie {

void ComputeContext::SetRootSignature(ID3D12RootSignature* rootSignature) noexcept {
	_commandList->SetComputeRootSignature(rootSignature);
}

void ComputeContext::SetRoot32BitConstants(
	uint32_t rootParameterIndex,
	uint32_t constantCount,
	const void* pData
) noexcept {
	_commandList->SetComputeRoot32BitConstants(rootParameterIndex, constantCount, pData, 0);
}

void ComputeContext::SetComputeRootConstantBufferView(
	uint32_t rootParameterIndex,
	D3D12_GPU_VIRTUAL_ADDRESS bufferLocation
) noexcept {
	// 存在 DATA_STATIC 标志时 SetComputeRootConstantBufferView 会检查资源状态
	if (_d3d12Context->GetRootSignatureVersion() >= D3D_ROOT_SIGNATURE_VERSION_1_1) {
		_FlushBarriers();
	}
	
	_commandList->SetComputeRootConstantBufferView(rootParameterIndex, bufferLocation);
}

void ComputeContext::SetRootDescriptorTable(
	uint32_t rootParameterIndex,
	uint32_t baseDescriptorOffset
) noexcept {
	assert(baseDescriptorOffset != std::numeric_limits<uint32_t>::max());

	// 存在 DATA_STATIC 标志时 SetComputeRootDescriptorTable 会检查资源状态
	if (_d3d12Context->GetRootSignatureVersion() >= D3D_ROOT_SIGNATURE_VERSION_1_1) {
		_FlushBarriers();
	}

	_commandList->SetComputeRootDescriptorTable(
		rootParameterIndex,
		_d3d12Context->GetDescriptorHeap().GetGpuHandle(baseDescriptorOffset)
	);
}

void ComputeContext::Dispatch(
	uint32_t threadGroupCountX,
	uint32_t threadGroupCountY,
	uint32_t threadGroupCountZ
) noexcept {
	_FlushBarriers();
	_commandList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void ComputeContext::ClearStateCache() noexcept {
	_ClearStateCache();
}

void GraphicsContext::SetRootSignature(ID3D12RootSignature* rootSignature) noexcept {
	_commandList->SetGraphicsRootSignature(rootSignature);
}

void GraphicsContext::SetRoot32BitConstants(
	uint32_t rootParameterIndex,
	uint32_t constantCount,
	const void* pData
) noexcept {
	_commandList->SetGraphicsRoot32BitConstants(rootParameterIndex, constantCount, pData, 0);
}

void GraphicsContext::SetRootDescriptorTable(
	uint32_t rootParameterIndex,
	uint32_t baseDescriptorOffset
) noexcept {
	assert(baseDescriptorOffset != std::numeric_limits<uint32_t>::max());

	// 存在 DATA_STATIC 标志时 SetGraphicsRootDescriptorTable 会检查资源状态
	if (_d3d12Context->GetRootSignatureVersion() >= D3D_ROOT_SIGNATURE_VERSION_1_1) {
		_FlushBarriers();
	}

	_commandList->SetGraphicsRootDescriptorTable(
		rootParameterIndex,
		_d3d12Context->GetDescriptorHeap().GetGpuHandle(baseDescriptorOffset)
	);
}

void GraphicsContext::IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY trimitiveTopology) noexcept {
	if (trimitiveTopology != _curTrimitiveTopology) {
		_curTrimitiveTopology = trimitiveTopology;
		_commandList->IASetPrimitiveTopology(trimitiveTopology);
	}
}

void GraphicsContext::RSSetViewportAndScissorRect(const D3D12_RECT& rect) noexcept {
	CD3DX12_VIEWPORT viewport((float)rect.left, (float)rect.top,
		float(rect.right - rect.left), float(rect.bottom - rect.top));
	_commandList->RSSetViewports(1, &viewport);

	_commandList->RSSetScissorRects(1, &rect);
}

void GraphicsContext::OMSetRenderTarget(uint32_t rtvDescriptorOffset) noexcept {
	assert(rtvDescriptorOffset != std::numeric_limits<uint32_t>::max());

	if (rtvDescriptorOffset != _curRtvDescriptorOffset) {
		_curRtvDescriptorOffset = rtvDescriptorOffset;

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
			_d3d12Context->GetDescriptorHeap(true).GetCpuHandle(rtvDescriptorOffset);
		_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	}
}

void GraphicsContext::Draw(uint32_t vertexCount) noexcept {
	_FlushBarriers();
	_commandList->DrawInstanced(vertexCount, 1, 0, 0);
}

void GraphicsContext::ClearStateCache() noexcept {
	_ClearStateCache();

	_curTrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	_curRtvDescriptorOffset = std::numeric_limits<uint32_t>::max();
}

}
