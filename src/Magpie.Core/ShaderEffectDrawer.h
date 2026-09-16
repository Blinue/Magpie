#pragma once
#include "EffectDrawerBase.h"
#include "SmallVector.h"

namespace Magpie {

struct ShaderEffectDrawInfo;

class ShaderEffectDrawer : public EffectDrawerBase {
public:
	virtual ~ShaderEffectDrawer() noexcept;

	const EffectInfo* Initialize(
		D3D12Context& d3d12Context,
		const EffectOption& effectOption
	) noexcept override;

	void Bind(SizeU inputSize, SizeU outputSize, const ColorInfo& colorInfo) noexcept override;

	HRESULT Update(EffectDrawerState& state, std::string& message) noexcept override;

	HRESULT Draw(
		ComputeContext& computeContext,
		uint32_t inputSrvOffset,
		uint32_t outputUavOffset
	) noexcept override;

private:
	HRESULT _CreateDeviceResources() noexcept;

	void _UpdateConstants() noexcept;

	HRESULT _UpdateSizeDependentResources() noexcept;

	D3D12Context* _d3d12Context = nullptr;
	const EffectOption* _effectOption = nullptr;
	const EffectInfo* _effectInfo = nullptr;

	SizeU _inputSize{};
	SizeU _outputSize{};
	ColorInfo _colorInfo;
	std::string _compilationTaskId;
	const ShaderEffectDrawInfo* _drawInfo = nullptr;
	std::string _errorMsg;

	struct _PassData {
		winrt::com_ptr<ID3D12RootSignature> rootSignature;
		winrt::com_ptr<ID3D12PipelineState> pso;
		// 这里是 _descriptorBaseOffset 中成员的引用，无需释放
		uint32_t descriptorBaseOffset = std::numeric_limits<uint32_t>::max();
		std::pair<uint32_t, uint32_t> dispatchCount;
	};
	SmallVector<_PassData> _passDatas;

	winrt::com_ptr<ID3D12Resource> _constantBuffer;
	winrt::com_ptr<ID3D12Resource> _constantUploadBuffer;
	void* _constantUploadBufferData = nullptr;
	uint32_t _constantsDataSize = 0;

	SmallVector<winrt::com_ptr<ID3D12Resource>> _textures;
	SmallVector<D3D12_RESOURCE_STATES> _textureStates;
	struct _TextureSourceData {
		winrt::com_ptr<ID3D12Resource> uploadBuffer;
		SizeU textureSize{};
		uint32_t targetTextureIdx = 0;
	};
	SmallVector<_TextureSourceData, 1> _textureSourceDatas;
	// 描述符布局: [SRV] | [UAV] | [SRV] | ...
	// 多个通道的 SRV 和 UAV 可能合并，_textureDescriptorMap 保存了布局。
	uint32_t _descriptorBaseOffset = std::numeric_limits<uint32_t>::max();
	// 和 ShaderEffectPassDesc 的 inputs/outputs 不同，这里存储 _textures 中元素索引，
	// 从 0 开始。虽然可以用额外字段区分 SRV 和 UAV，但这里想尽可能避免堆分配，因此 UAV
	// 的索引都加上 _textures.size() 作为区分。
	SmallVector<uint32_t> _textureDescriptorMap;

	bool _shouldUpdateSizeDependentResources = false;
	bool _shouldUpdateConstantBuffer = false;
};

}
