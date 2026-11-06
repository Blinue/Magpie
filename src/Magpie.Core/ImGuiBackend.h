#pragma once
#include <D3D12MemAlloc.h>
#include <imgui.h>
#include <parallel_hashmap/phmap.h>
#include "SmallVector.h"

namespace Magpie {

class D3D12Context;
class GraphicsContext;

class ImGuiBackend {
public:
	ImGuiBackend() = default;
	ImGuiBackend(const ImGuiBackend&) = delete;
	ImGuiBackend(ImGuiBackend&&) = delete;

	~ImGuiBackend() noexcept;

	void Initialize(
		D3D12Context& d3d12Context,
		winrt::AdvancedColorKind acKind,
		float sdrWhiteLevel
	) noexcept;

	HRESULT RenderDrawData(
		const ImDrawData& drawData,
		POINT viewportOffset,
		GraphicsContext& graphicsContext,
		uint64_t completedFenceValue,
		uint64_t frameFenceValue
	) noexcept;

	void OnColorInfoChanged(const ColorInfo& colorInfo) noexcept;

private:
	HRESULT _UpdateTexture(
		ImTextureData& texData,
		GraphicsContext& graphicsContext,
		uint64_t completedFenceValue,
		uint64_t frameFenceValue
	) noexcept;

	struct _FrameResource {
		uint64_t fenceValue = 0;
		int vertexCount = 0;
		int indexCount = 0;
		winrt::com_ptr<D3D12MA::Allocation> vertexBufferAllocation;
		winrt::com_ptr<D3D12MA::Allocation> indexBufferAllocation;
		void* vertexBufferData = nullptr;
		void* indexBufferData = nullptr;
	};

	HRESULT _SetupRenderState(
		const ImDrawData& drawData,
		GraphicsContext& graphicsContext,
		const _FrameResource& curFrameResource,
		POINT viewportOffset,
		uint32_t& descriptorTableIdx
	) noexcept;

	HRESULT _CreateImGuiPSO() noexcept;

	D3D12Context* _d3d12Context = nullptr;

	winrt::AdvancedColorKind _acKind = winrt::AdvancedColorKind::StandardDynamicRange;
	float _sdrWhiteLevel = 1.0f;

	struct _TextureData {
		winrt::com_ptr<D3D12MA::Allocation> textureAllocation;
		uint64_t fenceValue = 0;
	};
	// 使用 SRV 偏移量（同时也是 ImTextureID）作为键
	phmap::flat_hash_map<uint32_t, _TextureData> _textureDatas;

	SmallVector<_FrameResource, 0> _frameResources;

	winrt::com_ptr<ID3D12RootSignature> _imguiRootSignature;
	winrt::com_ptr<ID3D12PipelineState> _imguiPSO;
};

}
