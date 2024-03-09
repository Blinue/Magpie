#include "pch.h"
#include "DirectMLInferenceBackend.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include "shaders/TensorToTextureCS.h"
#include "shaders/TextureToTensorCS.h"
#include "Logger.h"
#include <onnxruntime/core/session/onnxruntime_session_options_config_keys.h>
#include <onnxruntime/core/providers/dml/dml_provider_factory.h>
#include "Win32Utils.h"

namespace Magpie::Core {

static winrt::com_ptr<ID3D12Device> CreateD3D12Device(IDXGIAdapter4* adapter) noexcept {
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
		adapter,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&d3d12Device)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("D3D12CreateDevice 失败", hr);
		return {};
	}

	return d3d12Device;
}

static winrt::com_ptr<IDMLDevice> CreateDMLDevice(ID3D12Device* d3d12Device) noexcept {
	winrt::com_ptr<IDMLDevice> dmlDevice;
	HRESULT hr = DMLCreateDevice1(
		d3d12Device,
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
		Logger::Get().ComError("DMLCreateDevice1 失败", hr);
		return {};
	}

	return dmlDevice;
}

static winrt::com_ptr<ID3D12Resource> ShareTextureWithD3D12(ID3D11Texture2D* texture, ID3D12Device* d3d12Device, DWORD access) noexcept {
	winrt::com_ptr<IDXGIResource1> dxgiResource;
	HRESULT hr = texture->QueryInterface<IDXGIResource1>(dxgiResource.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("获取 IDXGIResource1 失败", hr);
		return {};
	}

	HANDLE sharedHandle;
	hr = dxgiResource->CreateSharedHandle(nullptr, access, nullptr, &sharedHandle);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateSharedHandle 失败", hr);
		return {};
	}

	Win32Utils::ScopedHandle scopedSharedHandle(sharedHandle);

	winrt::com_ptr<ID3D12Resource> result;
	hr = d3d12Device->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&result));
	if (FAILED(hr)) {
		Logger::Get().ComError("OpenSharedHandle 失败", hr);
		return {};
	}

	return result;
}

static winrt::com_ptr<IUnknown> AllocateD3D12Resource(const OrtDmlApi* ortDmlApi, ID3D12Resource* buffer) {
	void* dmlResource;
	Ort::ThrowOnError(ortDmlApi->CreateGPUAllocationFromD3DResource(buffer, &dmlResource));

	winrt::com_ptr<IUnknown> allocatedBuffer;
	allocatedBuffer.copy_from((IUnknown*)dmlResource);

	Ort::ThrowOnError(ortDmlApi->FreeGPUAllocation(dmlResource));

	return allocatedBuffer;
}

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
		inputSize = { (LONG)inputDesc.Width, (LONG)inputDesc.Height };
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

	const uint32_t elemCount = uint32_t(inputSize.cx * inputSize.cy * 3);

	winrt::com_ptr<ID3D12Device> d3d12Device = CreateD3D12Device(deviceResources.GetGraphicsAdapter());
	if (!d3d12Device) {
		Logger::Get().Error("CreateD3D12Device 失败");
		return false;
	}

	{
		D3D12_COMMAND_QUEUE_DESC desc{
			.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
		};

		HRESULT hr = d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&_commandQueue));
		if (FAILED(hr)) {
			return false;
		}
	}

	bool isFP16Data = false;

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

		winrt::com_ptr<IDMLDevice> dmlDevice = CreateDMLDevice(d3d12Device.get());
		if (!dmlDevice) {
			Logger::Get().Error("CreateDMLDevice 失败");
			return false;
		}

		Ort::ThrowOnError(ortDmlApi->SessionOptionsAppendExecutionProvider_DML1(
			sessionOptions, dmlDevice.get(), _commandQueue.get()));

		_session = Ort::Session(_env, modelPath, sessionOptions);

		if (!_IsValidModel(_session, isFP16Data)) {
			Logger::Get().Error("不支持此模型");
			return false;
		}

		// 创建张量缓冲区
		{
			D3D12_HEAP_PROPERTIES heapDesc{
				.Type = D3D12_HEAP_TYPE_DEFAULT
			};
			D3D12_RESOURCE_DESC resDesc{
				.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
				.Width = elemCount * (isFP16Data ? 2 : 4),
				.Height = 1,
				.DepthOrArraySize = 1,
				.MipLevels = 1,
				.SampleDesc{
					.Count = 1
				},
				.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
				.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
			};
			HRESULT hr = d3d12Device->CreateCommittedResource(
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

		// 创建 IOBinding
		_ioBinding = Ort::IoBinding(_session);

		Ort::MemoryInfo memoryInfo(
			"DML",
			OrtAllocatorType::OrtDeviceAllocator,
			(int)deviceResources.GetAdapterIndex(),
			OrtMemType::OrtMemTypeDefault
		);

		const ONNXTensorElementDataType dataType =
			isFP16Data ? ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16 : ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT;

		const int64_t inputShape[]{ 1,3,inputSize.cy,inputSize.cx };
		_allocatedInput = AllocateD3D12Resource(ortDmlApi, _inputBuffer.get());
		_ioBinding.BindInput("input", Ort::Value::CreateTensor(
			memoryInfo,
			_allocatedInput.get(),
			size_t(elemCount * (isFP16Data ? 2 : 4)),
			inputShape,
			std::size(inputShape),
			dataType
		));

		const int64_t outputShape[]{ 1,3,inputSize.cy * 2,inputSize.cx * 2 };
		_allocatedOutput = AllocateD3D12Resource(ortDmlApi, _outputBuffer.get());
		_ioBinding.BindOutput("output", Ort::Value::CreateTensor(
			memoryInfo,
			_allocatedOutput.get(),
			size_t(elemCount * 4 * (isFP16Data ? 2 : 4)),
			outputShape,
			std::size(outputShape),
			dataType
		));
	} catch (const Ort::Exception& e) {
		Logger::Get().Error(e.what());
		return false;
	}

	if (!_CreateFence(d3d11Device, d3d12Device.get())) {
		Logger::Get().Error("_CreateFence 失败");
		return false;
	}

	_d3d12InputTex = ShareTextureWithD3D12(input, d3d12Device.get(), DXGI_SHARED_RESOURCE_READ);
	_d3d12OutputTex = ShareTextureWithD3D12(_outputTex.get(), d3d12Device.get(),
		DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE);
	if (!_d3d12InputTex || !_d3d12OutputTex) {
		Logger::Get().Error("ShareTextureWithD3D12 失败");
		return false;
	}

	UINT descriptorSize;
	if (!_CreateCBVHeap(d3d12Device.get(), elemCount, isFP16Data, descriptorSize)) {
		Logger::Get().Error("_CreateCBVHeap 失败");
		return false;
	}

	if (!_CreatePipelineStates(d3d12Device.get())) {
		Logger::Get().Error("_CreatePipelineStates 失败");
		return false;
	}

	if (!_CalcCommandLists(d3d12Device.get(), inputSize, descriptorSize)) {
		Logger::Get().Error("_CalcCommandLists 失败");
		return false;
	}

	return true;
}

void DirectMLInferenceBackend::Evaluate() noexcept {
	HRESULT hr = _d3d11DC->Signal(_d3d11Fence.get(), ++_fenceValue);
	if (FAILED(hr)) {
		Logger::Get().ComError("Signal 失败", hr);
		return;
	}
	_d3d11DC->Flush();

	hr = _commandQueue->Wait(_d3d12Fence.get(), _fenceValue);
	if (FAILED(hr)) {
		Logger::Get().ComError("Wait 失败", hr);
		return;
	}

	// 输入纹理 -> 输入张量
	{
		ID3D12CommandList* t = _tex2TensorCommandList.get();
		_commandQueue->ExecuteCommandLists(1, &t);
	}

	try {
		_session.Run(Ort::RunOptions{ nullptr }, _ioBinding);
	} catch (const Ort::Exception& e) {
		Logger::Get().Error(e.what());
		return;
	}
	
	// 输出张量 -> 输出纹理
	{
		ID3D12CommandList* t = _tensor2TexCommandList.get();
		_commandQueue->ExecuteCommandLists(1, &t);
	}

	hr = _commandQueue->Signal(_d3d12Fence.get(), ++_fenceValue);
	if (FAILED(hr)) {
		Logger::Get().ComError("Signal 失败", hr);
		return;
	}

	hr = _d3d11DC->Wait(_d3d11Fence.get(), _fenceValue);
	if (FAILED(hr)) {
		Logger::Get().ComError("Wait 失败", hr);
		return;
	}
}

bool DirectMLInferenceBackend::_CreateFence(ID3D11Device5* d3d11Device, ID3D12Device* d3d12Device) noexcept {
	HRESULT hr = d3d11Device->CreateFence(
		_fenceValue, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(&_d3d11Fence));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateFence 失败", hr);
		return false;
	}

	HANDLE sharedHandle;
	hr = _d3d11Fence->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, &sharedHandle);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateSharedHandle 失败", hr);
		return false;
	}

	Win32Utils::ScopedHandle scopedSharedHandle(sharedHandle);

	hr = d3d12Device->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&_d3d12Fence));
	if (FAILED(hr)) {
		Logger::Get().ComError("OpenSharedHandle 失败", hr);
		return false;
	}

	return true;
}

bool DirectMLInferenceBackend::_CreateCBVHeap(
	ID3D12Device* d3d12Device,
	uint32_t elemCount,
	bool isFP16Data, 
	UINT& descriptorSize
) noexcept {
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 4,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		};

		HRESULT hr = d3d12Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_cbvHeap));
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateDescriptorHeap 失败", hr);
			return false;
		}
	}

	descriptorSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = _cbvHeap->GetCPUDescriptorHandleForHeapStart();

	d3d12Device->CreateShaderResourceView(_d3d12InputTex.get(), nullptr, cbvHandle);
	cbvHandle.ptr += descriptorSize;

	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC desc{
			.Format = isFP16Data ? DXGI_FORMAT_R16_FLOAT : DXGI_FORMAT_R32_FLOAT,
			.ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
			.Buffer{
				.NumElements = elemCount
			}
		};
		d3d12Device->CreateUnorderedAccessView(_inputBuffer.get(), nullptr, &desc, cbvHandle);
	}
	cbvHandle.ptr += descriptorSize;

	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc{
			.Format = isFP16Data ? DXGI_FORMAT_R16_FLOAT : DXGI_FORMAT_R32_FLOAT,
			.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer{
				.NumElements = elemCount * 4
			}
		};
		d3d12Device->CreateShaderResourceView(_outputBuffer.get(), &desc, cbvHandle);
	}
	cbvHandle.ptr += descriptorSize;

	d3d12Device->CreateUnorderedAccessView(_d3d12OutputTex.get(), nullptr, nullptr, cbvHandle);
	return true;
}

bool DirectMLInferenceBackend::_CreatePipelineStates(ID3D12Device* d3d12Device) noexcept {
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
		HRESULT hr = D3D12SerializeVersionedRootSignature(&desc, blob.put(), nullptr);
		if (FAILED(hr)) {
			Logger::Get().ComError("D3D12SerializeVersionedRootSignature 失败", hr);
			return false;
		}

		hr = d3d12Device->CreateRootSignature(
			0,
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			IID_PPV_ARGS(&_rootSignature)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateRootSignature 失败", hr);
			return false;
		}
	}

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc{
		.pRootSignature = _rootSignature.get(),
		.CS{
			.pShaderBytecode = TextureToTensorCS,
			.BytecodeLength = std::size(TextureToTensorCS)
		}
	};
	HRESULT hr = d3d12Device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&_tex2TensorPipelineState));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateComputePipelineState 失败", hr);
		return false;
	}

	desc.CS.pShaderBytecode = TensorToTextureCS;
	desc.CS.BytecodeLength = std::size(TensorToTextureCS);
	hr = d3d12Device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&_tensor2TexPipelineState));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateComputePipelineState 失败", hr);
		return false;
	}

	return true;
}

bool DirectMLInferenceBackend::_CalcCommandLists(
	ID3D12Device* d3d12Device,
	SIZE inputSize,
	UINT descriptorSize
) noexcept {
	winrt::com_ptr<ID3D12CommandAllocator> d3d12CommandAllocator;
	HRESULT hr = d3d12Device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&d3d12CommandAllocator));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateCommandAllocator 失败", hr);
		return false;
	}

	// 输入纹理 -> 输入张量
	hr = d3d12Device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		d3d12CommandAllocator.get(),
		_tex2TensorPipelineState.get(),
		IID_PPV_ARGS(&_tex2TensorCommandList)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateCommandList 失败", hr);
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
		Logger::Get().ComError("Close 失败", hr);
		return false;
	}

	// 输出张量 -> 输出纹理
	hr = d3d12Device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		d3d12CommandAllocator.get(),
		_tensor2TexPipelineState.get(),
		IID_PPV_ARGS(&_tensor2TexCommandList)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateCommandList 失败", hr);
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
		Logger::Get().ComError("Close 失败", hr);
		return false;
	}

	return true;
}

}
