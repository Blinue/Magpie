#pragma once

namespace Magpie {

class D3D12Context;
class ComputeContext;

class CatmullRomDrawer {
public:
	void Initialize(D3D12Context& d3d12Context) noexcept;

	HRESULT Draw(
		ComputeContext& computeContext,
		SizeU inputSize,
		SizeU outputSize,
		uint32_t inputSrvOffset,
		uint32_t outputUavOffset,
		bool outputSrgb
	) noexcept;

private:
	D3D12Context* _d3d12Context = nullptr;

	HRESULT _InitializeCatmullRomRootSignature() noexcept;
	HRESULT _InitializeCopyRootSignature() noexcept;

	winrt::com_ptr<ID3D12RootSignature> _catmullRomRootSignature;
	winrt::com_ptr<ID3D12PipelineState> _catmullRomPSO;
	winrt::com_ptr<ID3D12PipelineState> _catmullRomSrgbPSO;

	winrt::com_ptr<ID3D12RootSignature> _copyRootSignature;
	winrt::com_ptr<ID3D12PipelineState> _copyPSO;
	winrt::com_ptr<ID3D12PipelineState> _copySrgbPSO;
};

}
