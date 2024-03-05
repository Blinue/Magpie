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
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
	);
	*output = outputTex.get();

	uint32_t pixelCount = uint32_t(_inputSize.cx * _inputSize.cy);
	pixelCount = (pixelCount + 1) / 2 * 2;

	winrt::com_ptr<ID3D11Buffer> inputBuffer;
	winrt::com_ptr<ID3D11Buffer> outputBuffer;
	{
		D3D11_BUFFER_DESC desc{
			.ByteWidth = pixelCount * 3 * 2,
			.BindFlags = D3D11_BIND_UNORDERED_ACCESS
		};
		HRESULT hr = d3dDevice->CreateBuffer(&desc, nullptr, inputBuffer.put());
		if (FAILED(hr)) {
			return false;
		}

		desc.ByteWidth *= 4;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		hr = d3dDevice->CreateBuffer(&desc, nullptr, outputBuffer.put());
		if (FAILED(hr)) {
			return false;
		}
	}

	_inputTexSrv = descriptorStore.GetShaderResourceView(input);
	_pointSampler = deviceResources.GetSampler(
		D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);

	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc{
			.Format = DXGI_FORMAT_R16_FLOAT,
			.ViewDimension = D3D11_UAV_DIMENSION_BUFFER,
			.Buffer{
				.NumElements = pixelCount * 3
			}
		};

		HRESULT hr = d3dDevice->CreateUnorderedAccessView(
			inputBuffer.get(), &desc, _inputBufferUav.put());
		if (FAILED(hr)) {
			return false;
		}
	}

	{
		D3D11_SHADER_RESOURCE_VIEW_DESC desc{
			.Format = DXGI_FORMAT_R16_FLOAT,
			.ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
			.Buffer{
				.NumElements = pixelCount * 4 * 3
			}
		};

		HRESULT hr = d3dDevice->CreateShaderResourceView(
			outputBuffer.get(), &desc, _outputBufferSrv.put());
		if (FAILED(hr)) {
			return false;
		}
	}

	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc{
			.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D
		};
		HRESULT hr = d3dDevice->CreateUnorderedAccessView(
			outputTex.get(), &desc, _outputTexUav.put());
		if (FAILED(hr)) {
			return false;
		}
	}

	HRESULT hr = d3dDevice->CreateComputeShader(
		TextureToTensorCS, sizeof(TextureToTensorCS), nullptr, _texToTensorShader.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateComputeShader 失败", hr);
		return false;
	}

	hr = d3dDevice->CreateComputeShader(
		TensorToTextureCS, sizeof(TensorToTextureCS), nullptr, _tensorToTexShader.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateComputeShader 失败", hr);
		return false;
	}

	static constexpr std::pair<uint32_t, uint32_t> TEX_TO_TENSOR_BLOCK_SIZE{ 16, 16 };
	static constexpr std::pair<uint32_t, uint32_t> TENSOR_TO_TEX_BLOCK_SIZE{ 8, 8 };
	_texToTensorDispatchCount = {
		(_inputSize.cx + TEX_TO_TENSOR_BLOCK_SIZE.first - 1) / TEX_TO_TENSOR_BLOCK_SIZE.first,
		(_inputSize.cy + TEX_TO_TENSOR_BLOCK_SIZE.second - 1) / TEX_TO_TENSOR_BLOCK_SIZE.second
	};
	_tensorToTexDispatchCount = {
		(_inputSize.cx * 2 + TENSOR_TO_TEX_BLOCK_SIZE.first - 1) / TENSOR_TO_TEX_BLOCK_SIZE.first,
		(_inputSize.cy * 2 + TENSOR_TO_TEX_BLOCK_SIZE.second - 1) / TENSOR_TO_TEX_BLOCK_SIZE.second
	};

#ifdef _DEBUG
	// 启用 D3D12 调试层
	{
		winrt::com_ptr<ID3D12Debug> debugController;
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
		if (SUCCEEDED(hr)) {
			debugController->EnableDebugLayer();
		}
	}
#endif

	winrt::com_ptr<ID3D12Device> d3d12Device;
	hr = D3D12CreateDevice(
		deviceResources.GetGraphicsAdapter(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&d3d12Device)
	);
	if (FAILED(hr)) {
		return false;
	}

	winrt::com_ptr<ID3D12CommandQueue> d3d12CommandQueue;
	{
		D3D12_COMMAND_QUEUE_DESC desc{
			.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
		};
		
		hr = d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue));
		if (FAILED(hr)) {
			return false;
		}
	}

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
