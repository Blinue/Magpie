#include "pch.h"
#include "DirectMLInferenceBackend.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include "shaders/TensorToTextureCS.h"
#include "shaders/TextureToTensorCS.h"
#include "Logger.h"
#include <onnxruntime/core/session/onnxruntime_session_options_config_keys.h>
#include <onnxruntime/core/providers/dml/dml_provider_factory.h>

namespace Magpie::Core {

bool DirectMLInferenceBackend::Initialize(
	const wchar_t* modelPath,
	DeviceResources& deviceResources,
	BackendDescriptorStore& /*descriptorStore*/,
	ID3D11Texture2D* input,
	ID3D11Texture2D** output
) noexcept {
	ID3D11Device5* d3d11Device = deviceResources.GetD3DDevice();
	_d3d11DC = deviceResources.GetD3DDC();

	SIZE inputSize;
	{
		D3D11_TEXTURE2D_DESC inputDesc;
		input->GetDesc(&inputDesc);
		inputSize.cx = (LONG)inputDesc.Width;
		inputSize.cy = (LONG)inputDesc.Height;
	}

	// 创建输出纹理
	_outputTex = DirectXHelper::CreateTexture2D(
		d3d11Device,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		inputSize.cx * 2,
		inputSize.cy * 2,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,
		D3D11_USAGE_DEFAULT,
		D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE
	);
	*output = _outputTex.get();

	uint32_t pixelCount = uint32_t(inputSize.cx * inputSize.cy);

#ifdef _DEBUG
	// 启用 D3D12 调试层
	{
		winrt::com_ptr<ID3D12Debug> debugController;
		HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
		if (SUCCEEDED(hr)) {
			debugController->EnableDebugLayer();
		}
	}
#endif

	winrt::com_ptr<ID3D12Device> d3d12Device;
	HRESULT hr = D3D12CreateDevice(
		deviceResources.GetGraphicsAdapter(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&d3d12Device)
	);
	if (FAILED(hr)) {
		return false;
	}

	{
		D3D12_HEAP_PROPERTIES heapDesc{
			.Type = D3D12_HEAP_TYPE_DEFAULT
		};
		D3D12_RESOURCE_DESC resDesc{
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Width = pixelCount * 3 * 2,
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.SampleDesc{
				.Count = 1
			},
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		};
		hr = d3d12Device->CreateCommittedResource(
			&heapDesc,
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
			&resDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&_inputBuffer)
		);
		if (FAILED(hr)) {
			return false;
		}

		resDesc.Width *= 4;
		hr = d3d12Device->CreateCommittedResource(
			&heapDesc,
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
			&resDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&_outputBuffer)
		);
		if (FAILED(hr)) {
			return false;
		}
	}

	{
		D3D12_COMMAND_QUEUE_DESC desc{
			.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
		};

		hr = d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&_commandQueue));
		if (FAILED(hr)) {
			return false;
		}
	}

	try {
		const OrtApi& ortApi = Ort::GetApi();

		_env = Ort::Env(ORT_LOGGING_LEVEL_INFO, "", _OrtLog, nullptr);

		const OrtDmlApi* ortDmlApi = nullptr;
		ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, (const void**)&ortDmlApi);

		Ort::SessionOptions sessionOptions;
		sessionOptions.SetIntraOpNumThreads(1);
		sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
		sessionOptions.DisableMemPattern();
		sessionOptions.AddConfigEntry(kOrtSessionOptionsDisableCPUEPFallback, "1");

		Ort::ThrowOnError(ortApi.AddFreeDimensionOverride(sessionOptions, "DATA_BATCH", 1));

		winrt::com_ptr<IDMLDevice> dmlDevice;
		hr = DMLCreateDevice1(
			d3d12Device.get(),
#ifdef _DEBUG
			DML_CREATE_DEVICE_FLAG_DEBUG,
#else
			DML_CREATE_DEVICE_FLAG_NONE,
#endif
			// https://github.com/microsoft/onnxruntime/blob/cd56ea4a74ee41c040899d702667d2c86bee4ef0/onnxruntime/core/providers/dml/dml_provider_factory.cc#L470
			DML_FEATURE_LEVEL_5_0,
			IID_PPV_ARGS(&dmlDevice)
		);
		if (FAILED(hr)) {
			return false;
		}

		Ort::ThrowOnError(ortDmlApi->SessionOptionsAppendExecutionProvider_DML1(
			sessionOptions, dmlDevice.get(), _commandQueue.get()));

		_session = Ort::Session(_env, modelPath, sessionOptions);

		void* dmlResource;
		Ort::ThrowOnError(ortDmlApi->CreateGPUAllocationFromD3DResource(_inputBuffer.get(), &dmlResource));
		_allocatedInput.copy_from((IUnknown*)dmlResource);
		Ort::ThrowOnError(ortDmlApi->FreeGPUAllocation(dmlResource));

		Ort::ThrowOnError(ortDmlApi->CreateGPUAllocationFromD3DResource(_outputBuffer.get(), &dmlResource));
		_allocatedOutput.copy_from((IUnknown*)dmlResource);
		Ort::ThrowOnError(ortDmlApi->FreeGPUAllocation(dmlResource));

		_ioBinding = Ort::IoBinding(_session);

		Ort::MemoryInfo memoryInfo(
			"DML",
			OrtAllocatorType::OrtDeviceAllocator,
			(int)deviceResources.GetAdapterIndex(),
			OrtMemType::OrtMemTypeDefault
		);

		const int64_t inputShape[]{ 1,3,inputSize.cy,inputSize.cx };
		_ioBinding.BindInput("input", Ort::Value::CreateTensor(
			memoryInfo,
			_allocatedInput.get(),
			pixelCount * 3 * 2,
			inputShape,
			std::size(inputShape),
			ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16
		));

		const int64_t outputShape[]{ 1,3,inputSize.cy * 2,inputSize.cx * 2 };
		_ioBinding.BindOutput("output", Ort::Value::CreateTensor(
			memoryInfo,
			_allocatedOutput.get(),
			pixelCount * 4 * 3 * 2,
			outputShape,
			std::size(outputShape),
			ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16
		));
	} catch (const Ort::Exception& e) {
		OutputDebugStringA(e.what());
		return false;
	}

	hr = d3d11Device->CreateFence(
		_fenceValue, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(&_d3d11Fence));
	if (FAILED(hr)) {
		return false;
	}

	{
		HANDLE sharedHandle;
		hr = _d3d11Fence->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, &sharedHandle);
		if (FAILED(hr)) {
			return false;
		}

		hr = d3d12Device->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&_d3d12Fence));
		if (FAILED(hr)) {
			return false;
		}

		CloseHandle(sharedHandle);
	}

	{
		HANDLE sharedHandle;

		winrt::com_ptr<IDXGIResource1> dxgiResource;
		hr = input->QueryInterface<IDXGIResource1>(dxgiResource.put());
		if (FAILED(hr)) {
			return false;
		}

		hr = dxgiResource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ, nullptr, &sharedHandle);
		if (FAILED(hr)) {
			return false;
		}

		hr = d3d12Device->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&_d3d12InputTex));
		if (FAILED(hr)) {
			return false;
		}

		CloseHandle(sharedHandle);

		if (!_outputTex.try_as(dxgiResource)) {
			return false;
		}

		hr = dxgiResource->CreateSharedHandle(nullptr,
			DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr, &sharedHandle);
		if (FAILED(hr)) {
			return false;
		}

		hr = d3d12Device->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&_d3d12OutputTex));
		if (FAILED(hr)) {
			return false;
		}

		CloseHandle(sharedHandle);
	}

	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 4,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		};

		hr = d3d12Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_cbvHeap));
		if (FAILED(hr)) {
			return false;
		}
	}

	UINT descriptorSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = _cbvHeap->GetCPUDescriptorHandleForHeapStart();

	d3d12Device->CreateShaderResourceView(_d3d12InputTex.get(), nullptr, cbvHandle);
	cbvHandle.ptr += descriptorSize;

	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC desc{
			.Format = DXGI_FORMAT_R16_FLOAT,
			.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
			.Buffer{
				.NumElements = pixelCount * 3
			}
		};
		d3d12Device->CreateUnorderedAccessView(_inputBuffer.get(), nullptr, &desc, cbvHandle);
	}
	cbvHandle.ptr += descriptorSize;

	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc{
			.Format = DXGI_FORMAT_R16_FLOAT,
			.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer{
				.NumElements = pixelCount * 4 * 3
			}
		};
		d3d12Device->CreateShaderResourceView(_outputBuffer.get(), &desc, cbvHandle);
	}
	cbvHandle.ptr += descriptorSize;
	
	d3d12Device->CreateUnorderedAccessView(_d3d12OutputTex.get(), nullptr, nullptr, cbvHandle);

	{
		D3D12_DESCRIPTOR_RANGE ranges[]{
			D3D12_DESCRIPTOR_RANGE{
				.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				.NumDescriptors = 1,
				.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
			},
			D3D12_DESCRIPTOR_RANGE{
				.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
				.NumDescriptors = 1,
				.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
			},
		};

		D3D12_ROOT_PARAMETER rootParam{
			D3D12_ROOT_PARAMETER{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				.DescriptorTable{
					.NumDescriptorRanges = (UINT)std::size(ranges),
					.pDescriptorRanges = ranges
				}
			}
		};

		D3D12_STATIC_SAMPLER_DESC samDesc{
			.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
			.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER
		};

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc{
			.Version = D3D_ROOT_SIGNATURE_VERSION_1_0,
			.Desc_1_0{
				.NumParameters = 1,
				.pParameters = &rootParam,
				.NumStaticSamplers = 1,
				.pStaticSamplers = &samDesc
			}
		};

		winrt::com_ptr<ID3DBlob> blob;
		hr = D3D12SerializeVersionedRootSignature(&desc, blob.put(), nullptr);
		if (FAILED(hr)) {
			return false;
		}

		hr = d3d12Device->CreateRootSignature(
			0,
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			IID_PPV_ARGS(&_rootSignature)
		);
		if (FAILED(hr)) {
			return false;
		}
	}
	
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc{
			.pRootSignature = _rootSignature.get(),
			.CS{
				.pShaderBytecode = TextureToTensorCS,
				.BytecodeLength = std::size(TextureToTensorCS)
			}
		};
		hr = d3d12Device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&_tex2TensorPipelineState));
		if (FAILED(hr)) {
			return false;
		}

		desc.CS.pShaderBytecode = TensorToTextureCS;
		desc.CS.BytecodeLength = std::size(TensorToTextureCS);
		hr = d3d12Device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&_tensor2TexPipelineState));
		if (FAILED(hr)) {
			return false;
		}
	}

	winrt::com_ptr<ID3D12CommandAllocator> d3d12CommandAllocator;
	hr = d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&d3d12CommandAllocator));
	if (FAILED(hr)) {
		return false;
	}

	hr = d3d12Device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		d3d12CommandAllocator.get(),
		_tex2TensorPipelineState.get(),
		IID_PPV_ARGS(&_tex2TensorCommandList)
	);
	if (FAILED(hr)) {
		return false;
	}

	_tex2TensorCommandList->SetComputeRootSignature(_rootSignature.get());
	{
		ID3D12DescriptorHeap* t = _cbvHeap.get();
		_tex2TensorCommandList->SetDescriptorHeaps(1, &t);
	}
	_tex2TensorCommandList->SetComputeRootDescriptorTable(0, _cbvHeap->GetGPUDescriptorHandleForHeapStart());

	static constexpr std::pair<uint32_t, uint32_t> TEX_TO_TENSOR_BLOCK_SIZE{ 16, 16 };
	_tex2TensorCommandList->Dispatch(
		(inputSize.cx + TEX_TO_TENSOR_BLOCK_SIZE.first - 1) / TEX_TO_TENSOR_BLOCK_SIZE.first,
		(inputSize.cy + TEX_TO_TENSOR_BLOCK_SIZE.second - 1) / TEX_TO_TENSOR_BLOCK_SIZE.second,
		1
	);
	hr = _tex2TensorCommandList->Close();
	if (FAILED(hr)) {
		return false;
	}

	hr = d3d12Device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		d3d12CommandAllocator.get(),
		_tensor2TexPipelineState.get(),
		IID_PPV_ARGS(&_tensor2TexCommandList)
	);
	if (FAILED(hr)) {
		return false;
	}

	_tensor2TexCommandList->SetComputeRootSignature(_rootSignature.get());
	{
		ID3D12DescriptorHeap* t = _cbvHeap.get();
		_tensor2TexCommandList->SetDescriptorHeaps(1, &t);
	}
	{
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = _cbvHeap->GetGPUDescriptorHandleForHeapStart();
		gpuHandle.ptr += 2 * static_cast<UINT64>(descriptorSize);
		_tensor2TexCommandList->SetComputeRootDescriptorTable(0, gpuHandle);
	}
	
	static constexpr std::pair<uint32_t, uint32_t> TENSOR_TO_TEX_BLOCK_SIZE{ 8, 8 };
	_tensor2TexCommandList->Dispatch(
		(inputSize.cx * 2 + TENSOR_TO_TEX_BLOCK_SIZE.first - 1) / TENSOR_TO_TEX_BLOCK_SIZE.first,
		(inputSize.cy * 2 + TENSOR_TO_TEX_BLOCK_SIZE.second - 1) / TENSOR_TO_TEX_BLOCK_SIZE.second,
		1
	);
	hr = _tensor2TexCommandList->Close();
	if (FAILED(hr)) {
		return false;
	}

	return true;
}

void DirectMLInferenceBackend::Evaluate() noexcept {
	_d3d11DC->Signal(_d3d11Fence.get(), ++_fenceValue);
	_d3d11DC->Flush();

	_commandQueue->Wait(_d3d12Fence.get(), _fenceValue);
	{
		ID3D12CommandList* t = _tex2TensorCommandList.get();
		_commandQueue->ExecuteCommandLists(1, &t);
	}

	try {
		Ort::RunOptions runOptions;
		_session.Run(runOptions, _ioBinding);
	} catch (const Ort::Exception& e) {
		OutputDebugStringA(e.what());
		return;
	}
	
	{
		ID3D12CommandList* t = _tensor2TexCommandList.get();
		_commandQueue->ExecuteCommandLists(1, &t);
	}

	_commandQueue->Signal(_d3d12Fence.get(), ++_fenceValue);
	_d3d11DC->Wait(_d3d11Fence.get(), _fenceValue);
}

}
