#pragma once
#include <imgui.h>
#include <parallel_hashmap/phmap.h>

namespace Magpie {

class D3D12Context;
class GraphicsContext;

class ImGuiBackend {
public:
	ImGuiBackend() = default;
	ImGuiBackend(const ImGuiBackend&) = delete;
	ImGuiBackend(ImGuiBackend&&) = delete;

	~ImGuiBackend() noexcept;

	bool Initialize(D3D12Context& d3d12Context) noexcept;

	HRESULT RenderDrawData(
		const ImDrawData& drawData,
		POINT viewportOffset,
		GraphicsContext& graphicsContext,
		uint64_t frameFenceValue,
		uint64_t completedFenceValue
	) noexcept;

private:
	HRESULT _UpdateTexture(
		ImTextureData& texData,
		GraphicsContext& graphicsContext,
		uint64_t frameFenceValue,
		uint64_t completedFenceValue
	) noexcept;

	D3D12Context* _d3d12Context = nullptr;

	struct _TextureData {
		winrt::com_ptr<ID3D12Resource> texture;
		uint64_t fenceValue = 0;
	};
	// 使用 SRV 偏移量作为键
	phmap::flat_hash_map<uint32_t, _TextureData> _textureDatas;

	struct _UploadBuffer {
		winrt::com_ptr<ID3D12Resource> buffer;
		void* bufferData = nullptr;
		uint32_t size = 0;
		uint64_t fenceValue = 0;
	};
	std::vector<_UploadBuffer> _uploadBuffers;
};

}
