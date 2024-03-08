#include "pch.h"
#include "DirectMLInferenceBackend.h"
#include "DeviceResources.h"
#include "StrUtils.h"
#include "DirectXHelper.h"
#include "shaders/TensorToTextureCS.h"
#include "shaders/TextureToTensorCS.h"
#include "BackendDescriptorStore.h"
#include "Logger.h"
#include <onnxruntime/core/session/onnxruntime_session_options_config_keys.h>
#include <onnxruntime/core/providers/dml/dml_provider_factory.h>

namespace Magpie::Core {

static void ORT_API_CALL OrtLog(
	void* /*param*/,
	OrtLoggingLevel severity,
	const char* /*category*/,
	const char* /*logid*/,
	const char* /*code_location*/,
	const char* message
) {
	const char* SEVERITIES[] = {
		"verbose",
		"info",
		"warning",
		"error",
		"fatal"
	};
	//Logger::Get().Info(StrUtils::Concat("[", SEVERITIES[severity], "] ", message));
	OutputDebugStringA(StrUtils::Concat("[", SEVERITIES[severity], "] ", message, "\n").c_str());
}

bool DirectMLInferenceBackend::Initialize(
	const wchar_t* modelPath,
	DeviceResources& deviceResources,
	BackendDescriptorStore& descriptorStore,
	ID3D11Texture2D* input,
	ID3D11Texture2D** output
) noexcept {
	ID3D11Device5* d3dDevice = deviceResources.GetD3DDevice();
	_d3dDC = deviceResources.GetD3DDC();

	{
		D3D11_TEXTURE2D_DESC inputDesc;
		input->GetDesc(&inputDesc);
		_inputSize = { (LONG)inputDesc.Width, (LONG)inputDesc.Height };
	}

	// 创建输出纹理
	winrt::com_ptr<ID3D11Texture2D> outputTex = DirectXHelper::CreateTexture2D(
		d3dDevice,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		_inputSize.cx * 2,
		_inputSize.cy * 2,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,
		D3D11_USAGE_DEFAULT,
		D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE
	);
	*output = outputTex.get();

	uint32_t pixelCount = uint32_t(_inputSize.cx * _inputSize.cy);

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

	HANDLE inputSharedHandle;
	HANDLE outputSharedHandle;
	{
		winrt::com_ptr<IDXGIResource1> dxgiResource;
		hr = input->QueryInterface<IDXGIResource1>(dxgiResource.put());
		if (FAILED(hr)) {
			return false;
		}

		hr = dxgiResource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ, nullptr, &inputSharedHandle);
		if (FAILED(hr)) {
			return false;
		}

		if (!outputTex.try_as(dxgiResource)) {
			return false;
		}

		hr = dxgiResource->CreateSharedHandle(nullptr,
			DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr, &outputSharedHandle);
		if (FAILED(hr)) {
			return false;
		}
	}

	winrt::com_ptr<ID3D12Resource> inputBuffer;
	winrt::com_ptr<ID3D12Resource> outputBuffer;
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
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&inputBuffer)
		);
		if (FAILED(hr)) {
			return false;
		}

		resDesc.Width *= 4;
		hr = d3d12Device->CreateCommittedResource(
			&heapDesc,
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
			&resDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(&outputBuffer)
		);
		if (FAILED(hr)) {
			return false;
		}
	}

	winrt::com_ptr<ID3D12DescriptorHeap> d3d12CBVHeap;
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 4,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		};

		hr = d3d12Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&d3d12CBVHeap));
		if (FAILED(hr)) {
			return false;
		}
	}

	UINT descriptorSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = d3d12CBVHeap->GetCPUDescriptorHandleForHeapStart();



	winrt::com_ptr<ID3D12RootSignature> d3d12RootSignature;
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
					.NumDescriptorRanges = std::size(ranges),
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
			IID_PPV_ARGS(&d3d12RootSignature)
		);
		if (FAILED(hr)) {
			return false;
		}
	}

	winrt::com_ptr<ID3D12PipelineState> d3d12PipelineState;
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc{
			.pRootSignature = d3d12RootSignature.get(),
			.CS{
				.pShaderBytecode = TextureToTensorCS,
				.BytecodeLength = std::size(TextureToTensorCS)
			}
		};
		hr = d3d12Device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&d3d12PipelineState));
		if (FAILED(hr)) {
			return false;
		}
	}

	winrt::com_ptr<ID3D12CommandQueue> d3d12CommandQueue;
	{
		D3D12_COMMAND_QUEUE_DESC desc{
			.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
		};
		
		hr = d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue));
		if (FAILED(hr)) {
			return false;
		}
	}

	winrt::com_ptr<ID3D12CommandAllocator> d3d12CommandAllocator;
	hr = d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&d3d12CommandAllocator));
	if (FAILED(hr)) {
		return false;
	}

	//d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, d3d12CommandAllocator.get(), )

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

	try {
		const OrtApi& ortApi = Ort::GetApi();

		_env = Ort::Env(ORT_LOGGING_LEVEL_INFO, "", OrtLog, nullptr);

		const OrtDmlApi* ortDmlApi = nullptr;
		ortApi.GetExecutionProviderApi("DML", ORT_API_VERSION, (const void**)&ortDmlApi);

		Ort::SessionOptions sessionOptions;
		sessionOptions.SetIntraOpNumThreads(1);
		sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
		sessionOptions.DisableMemPattern();
		sessionOptions.AddConfigEntry(kOrtSessionOptionsDisableCPUEPFallback, "1");

		ortDmlApi->SessionOptionsAppendExecutionProvider_DML1(sessionOptions, dmlDevice.get(), d3d12CommandQueue.get());

		_session = Ort::Session(_env, modelPath, sessionOptions);
	} catch (const Ort::Exception& e) {
		OutputDebugStringA(e.what());
		return false;
	}

	return false;
}

void DirectMLInferenceBackend::Evaluate() noexcept {
}

}
