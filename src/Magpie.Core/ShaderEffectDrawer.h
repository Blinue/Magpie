#pragma once
#include "EffectDrawerBase.h"
#include "SmallVector.h"

namespace Magpie {

class EffectsMemPoolBuilder;
struct ShaderEffectDrawInfo;

class ShaderEffectDrawer : public EffectDrawerBase {
public:
	virtual ~ShaderEffectDrawer() noexcept;

	HRESULT Initialize(
		D3D12Context& d3d12Context,
		const EffectOption& effectOption,
		EffectDrawerInfo& effectDrawerInfo
	) noexcept override;

	void Bind(SizeU inputSize, SizeU outputSize, const ColorInfo& colorInfo) noexcept override;

	HRESULT Update(EffectDrawerState& state, EffectDebugInfo& debugInfo) noexcept override;

	void BuildMemPool(EffectsMemPoolBuilder& memPoolBuilder) noexcept override;

	HRESULT CreateResourcesInMemPool(
		D3D12MA::Allocation* textureAllocation,
		D3D12MA::Allocation* /*bufferAllocation*/,
		std::span<const uint64_t> resourceOffsets
	) noexcept override;

	HRESULT Draw(
		ComputeContext& computeContext,
		uint32_t inputSrvOffset,
		uint32_t outputUavOffset,
		ID3D12Resource* aliasedOutput,
		EffectDrawerExportData& exportData
	) noexcept override;

private:
	HRESULT _InitializeCommonResources() noexcept;

	bool _ComputeNonPersistentTextureSizes() noexcept;

	HRESULT _CreatePersistentResources() noexcept;

	void _UpdateConstants() noexcept;

	HRESULT _CreatePSOs() noexcept;

	HRESULT _CreateConstantBuffer() noexcept;

	void _ComputeDispatchCounts() noexcept;

	HRESULT _CreateNonPersistentResources(
		D3D12MA::Allocation* textureAllocation,
		std::span<const uint64_t> resourceOffsets
	) noexcept;

	void _ComputeDescriptorLayout(
		SmallVectorImpl<std::pair<uint32_t, DXGI_FORMAT>>& descriptorLayout,
		SmallVectorImpl<uint32_t>& passDescriptorOffsets
	) const noexcept;

	HRESULT _CreateDescritors() noexcept;

	DXGI_FORMAT _GetDxgiFormat(EffectTextureFormat texFormat) const noexcept;

	HRESULT _Export(
		EffectDrawerExportData& exportData,
		ComputeContext& computeContext,
		uint32_t passIdx
	) noexcept;

	enum _Error {
		NoError,
		CompilationFailed,
		InvalidTextureSize,
		InvalidTextureSource
	} _error = _Error::NoError;

	D3D12Context* _d3d12Context = nullptr;
	const EffectOption* _effectOption = nullptr;
	const EffectInfo* _effectInfo = nullptr;

	SizeU _inputSize{};
	SizeU _outputSize{};

	winrt::AdvancedColorKind _acKind = winrt::AdvancedColorKind::StandardDynamicRange;
	float _maxLuminance = 1.0f;
	float _sdrWhiteLevel = 1.0f;

	std::string _compilationTaskId;
	const ShaderEffectDrawInfo* _drawInfo = nullptr;

	winrt::com_ptr<D3D12MA::Allocation> _constantBufferAllocation;
	winrt::com_ptr<D3D12MA::Allocation> _constantUploadBufferAllocation;
	void* _constantUploadBufferData = nullptr;
	uint32_t _constantsDataSize = 0;

	// 和 DrawInfo 中的 textures 一一对应
	SmallVector<SizeU> _textureSizes;

	struct _TextureSourceData {
		winrt::com_ptr<D3D12MA::Allocation> uploadBufferAllocation;
		SizeU textureSize{};
		// _textureResourceDatas 序号
		uint32_t targetTexResIdx = 0;
	};
	SmallVector<_TextureSourceData> _textureSourceDatas;

	// 存储需要数据持久保存的资源，不能放入显存池
	winrt::com_ptr<D3D12MA::Allocation> _persistentResourcesAllocation;
	uint32_t _persistentResourceCount = 0;
	
	struct _TextureResourceData {
		winrt::com_ptr<ID3D12Resource> resource;
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
	};
	// 由于可能会为 DrawInfo 中的每个纹理定义创建多个资源，因此 _textureResourceDatas 通常比
	// DrawInfo 中的 textures 成员更多。
	// 布局: 持久保存的资源 | Pass1 的输出 | Pass2 的输出 | ...
	// 纹理名和偏移量相同时不会重复创建资源。
	SmallVector<_TextureResourceData> _textureResourceDatas;

	// 描述符布局: [SRV] | [UAV] | [SRV] | ...
	// 多个通道的 SRV 和 UAV 可能合并。
	uint32_t _descriptorBaseOffset = UINT_MAX;
	uint32_t _descriptorCount = 0;

	struct _PassData {
		winrt::com_ptr<ID3D12RootSignature> rootSignature;
		winrt::com_ptr<ID3D12PipelineState> pso;
		// 输入和输出纹理在 _textureResourceDatas 中的索引。INPUT 和 OUTPUT 是 UINT_MAX
		SmallVector<uint32_t> inputTexResIdxes;
		SmallVector<uint32_t> outputTexResIdxes;
		// 这里是 _descriptorBaseOffset 中成员的引用，无需释放
		uint32_t descriptorBaseOffset = UINT_MAX;
		std::pair<uint32_t, uint32_t> dispatchCount;
	};
	SmallVector<_PassData> _passDatas;
	
	bool _hasNonPersistentResource = false;
	bool _shouldUpdateTextures = false;
	bool _shouldUpdateConstantBuffer = false;
};

}
