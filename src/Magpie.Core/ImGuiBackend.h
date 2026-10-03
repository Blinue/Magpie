#pragma once
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

	bool Initialize(D3D12Context& d3d12Context, const ColorInfo& colorInfo) noexcept;

	HRESULT RenderDrawData(
		const ImDrawData& drawData,
		POINT viewportOffset,
		GraphicsContext& graphicsContext,
		uint64_t frameFenceValue,
		uint64_t completedFenceValue
	) noexcept;

	void OnColorInfoChanged(const ColorInfo& colorInfo) noexcept;

private:
	HRESULT _UpdateTexture(
		ImTextureData& texData,
		GraphicsContext& graphicsContext,
		uint64_t frameFenceValue,
		uint64_t completedFenceValue
	) noexcept;

	struct _FrameResource {
		uint64_t fenceValue = 0;
		winrt::com_ptr<ID3D12Resource> vertexBuffer;
		winrt::com_ptr<ID3D12Resource> indexBuffer;
		int vertexBufferSize = 0;
		int indexBufferSize = 0;
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
	ColorInfo _colorInfo;

	struct _TextureData {
		winrt::com_ptr<ID3D12Resource> texture;
		uint64_t fenceValue = 0;
	};
	phmap::flat_hash_map<ImTextureID, _TextureData> _textureDatas;

	struct _UploadBuffer {
		winrt::com_ptr<ID3D12Resource> buffer;
		void* bufferData = nullptr;
		uint32_t size = 0;
		uint64_t fenceValue = 0;
	};
	SmallVector<_UploadBuffer> _uploadBuffers;

	SmallVector<_FrameResource, 0> _frameResources;

	winrt::com_ptr<ID3D12RootSignature> _imguiRootSignature;
	winrt::com_ptr<ID3D12PipelineState> _imguiPSO;
};

}
