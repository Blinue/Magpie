#include "pch.h"
#include "CatmullRomDrawer.h"
#include "CommandContext.h"
#include "D3D12Context.h"
#include "DirectXHelper.h"
#include "Logger.h"
#include "shaders/CatmullRomCS.h"
#include "shaders/CatmullRomCS_SM5.h"
#include "shaders/CatmullRomCS_sRGB.h"
#include "shaders/CatmullRomCS_sRGB_SM5.h"
#include "shaders/CopyCS.h"
#include "shaders/CopyCS_SM5.h"
#include "shaders/CopyCS_sRGB.h"
#include "shaders/CopyCS_sRGB_SM5.h"

namespace Magpie {

void CatmullRomDrawer::Initialize(D3D12Context& d3d12Context) noexcept {
	_d3d12Context = &d3d12Context;
}

HRESULT CatmullRomDrawer::Draw(
	ComputeContext& computeContext,
	SizeU inputSize,
	SizeU outputSize,
	uint32_t inputSrvOffset,
	uint32_t outputUavOffset,
	bool outputSrgb
) noexcept {
	// 作为性能优化，输入和输出尺寸相同时原样复制
	if (inputSize == outputSize) {
		if (!_copyRootSignature) {
			HRESULT hr = _InitializeCopyRootSignature();
			if (FAILED(hr)) {
				Logger::Get().ComError("_InitializeCopyRootSignature 失败", hr);
				return hr;
			}
		}
		computeContext.SetRootSignature(_copyRootSignature.get());

		if (outputSrgb) {
			if (!_copySrgbPSO) {
				D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {
					.pRootSignature = _copyRootSignature.get(),
					.CS = DirectXHelper::SelectShader(
						_d3d12Context->GetShaderModel() >= D3D_SHADER_MODEL_6_0,
						CopyCS_sRGB,
						CopyCS_sRGB_SM5
					)
				};
				HRESULT hr = _d3d12Context->GetDevice()->CreateComputePipelineState(
					&psoDesc, IID_PPV_ARGS(&_copySrgbPSO));
				if (FAILED(hr)) {
					Logger::Get().ComError("CreateComputePipelineState 失败", hr);
					return hr;
				}
			}
			
			computeContext.SetPipelineState(_copySrgbPSO.get());
		} else {
			if (!_copyPSO) {
				D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {
					.pRootSignature = _copyRootSignature.get(),
					.CS = DirectXHelper::SelectShader(
						_d3d12Context->GetShaderModel() >= D3D_SHADER_MODEL_6_0,
						CopyCS,
						CopyCS_SM5
					)
				};
				HRESULT hr = _d3d12Context->GetDevice()->CreateComputePipelineState(
					&psoDesc, IID_PPV_ARGS(&_copyPSO));
				if (FAILED(hr)) {
					Logger::Get().ComError("CreateComputePipelineState 失败", hr);
					return hr;
				}
			}

			computeContext.SetPipelineState(_copyPSO.get());
		}

		computeContext.SetRootDescriptorTable(0, inputSrvOffset);
		computeContext.SetRootDescriptorTable(1, outputUavOffset);
	} else {
		if (!_catmullRomRootSignature) {
			HRESULT hr = _InitializeCatmullRomRootSignature();
			if (FAILED(hr)) {
				Logger::Get().ComError("_InitializeCatmullRomRootSignature 失败", hr);
				return hr;
			}
		}
		computeContext.SetRootSignature(_catmullRomRootSignature.get());

		if (outputSrgb) {
			if (!_catmullRomSrgbPSO) {
				D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {
					.pRootSignature = _catmullRomRootSignature.get(),
					.CS = DirectXHelper::SelectShader(
						_d3d12Context->GetShaderModel() >= D3D_SHADER_MODEL_6_0,
						CatmullRomCS_sRGB,
						CatmullRomCS_sRGB_SM5
					)
				};
				HRESULT hr = _d3d12Context->GetDevice()->CreateComputePipelineState(
					&psoDesc, IID_PPV_ARGS(&_catmullRomSrgbPSO));
				if (FAILED(hr)) {
					Logger::Get().ComError("CreateComputePipelineState 失败", hr);
					return hr;
				}
			}

			computeContext.SetPipelineState(_catmullRomSrgbPSO.get());
		} else {
			if (!_catmullRomPSO) {
				D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {
					.pRootSignature = _catmullRomRootSignature.get(),
					.CS = DirectXHelper::SelectShader(
						_d3d12Context->GetShaderModel() >= D3D_SHADER_MODEL_6_0,
						CatmullRomCS,
						CatmullRomCS_SM5
					)
				};
				HRESULT hr = _d3d12Context->GetDevice()->CreateComputePipelineState(
					&psoDesc, IID_PPV_ARGS(&_catmullRomPSO));
				if (FAILED(hr)) {
					Logger::Get().ComError("CreateComputePipelineState 失败", hr);
					return hr;
				}
			}

			computeContext.SetPipelineState(_catmullRomPSO.get());
		}

		DirectXHelper::Constant32 constants[] = {
			{.uintVal = inputSize.width},
			{.uintVal = inputSize.height},
			{.floatVal = 1.0f / inputSize.width},
			{.floatVal = 1.0f / inputSize.height},
			{.floatVal = 1.0f / outputSize.width},
			{.floatVal = 1.0f / outputSize.height}
		};
		computeContext.SetRoot32BitConstants(0, (UINT)std::size(constants), constants);

		computeContext.SetRootDescriptorTable(1, inputSrvOffset);
		computeContext.SetRootDescriptorTable(2, outputUavOffset);
	}

	constexpr uint32_t BLOCK_SIZE = 16;
	computeContext.Dispatch(
		(outputSize.width + BLOCK_SIZE - 1) / BLOCK_SIZE,
		(outputSize.height + BLOCK_SIZE - 1) / BLOCK_SIZE
	);

	return S_OK;
}

HRESULT CatmullRomDrawer::_InitializeCatmullRomRootSignature() noexcept {
	winrt::com_ptr<ID3DBlob> signature;

	CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
	CD3DX12_DESCRIPTOR_RANGE1 uavRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

	D3D12_ROOT_PARAMETER1 rootParams[] = {
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
			.Constants = {
				.Num32BitValues = 6
			}
		},
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			.DescriptorTable = {
				.NumDescriptorRanges = 1,
				.pDescriptorRanges = &srvRange
			}
		},
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			.DescriptorTable = {
				.NumDescriptorRanges = 1,
				.pDescriptorRanges = &uavRange
			}
		}
	};

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {
		.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
		.ShaderRegister = 0
	};

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
		(UINT)std::size(rootParams), rootParams, 1, &samplerDesc);

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

	hr = _d3d12Context->GetDevice()->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&_catmullRomRootSignature)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateRootSignature 失败", hr);
		return hr;
	}

	return S_OK;
}

HRESULT CatmullRomDrawer::_InitializeCopyRootSignature() noexcept {
	winrt::com_ptr<ID3DBlob> signature;

	CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
	CD3DX12_DESCRIPTOR_RANGE1 uavRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

	D3D12_ROOT_PARAMETER1 rootParams[] = {
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			.DescriptorTable = {
				.NumDescriptorRanges = 1,
				.pDescriptorRanges = &srvRange
			}
		},
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			.DescriptorTable = {
				.NumDescriptorRanges = 1,
				.pDescriptorRanges = &uavRange
			}
		}
	};

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
		(UINT)std::size(rootParams), rootParams, 0, nullptr);

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

	hr = _d3d12Context->GetDevice()->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&_copyRootSignature)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateRootSignature 失败", hr);
		return hr;
	}

	return S_OK;
}

}
