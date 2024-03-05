#include "pch.h"
#include "OnnxEffectDrawer.h"
#include "Logger.h"
#include "TensorRTInferenceBackend.h"
#include "DirectMLInferenceBackend.h"

#pragma comment(lib, "onnxruntime.lib")

namespace Magpie::Core {

OnnxEffectDrawer::OnnxEffectDrawer() {}

OnnxEffectDrawer::~OnnxEffectDrawer() {}

/*static winrt::VideoFrame TextureToVideoFrame(ID3D11Texture2D* texture) noexcept {
	winrt::com_ptr<IDXGISurface> dxgiSurface;
	texture->QueryInterface<IDXGISurface>(dxgiSurface.put());

	winrt::IDirect3DSurface wrappedSurface;
	CreateDirect3D11SurfaceFromDXGISurface(
		dxgiSurface.get(),
		reinterpret_cast<IInspectable**>(winrt::put_abi(wrappedSurface))
	);
	return winrt::VideoFrame::CreateWithDirect3D11Surface(wrappedSurface);
}*/

bool OnnxEffectDrawer::Initialize(
	const wchar_t* modelPath,
	DeviceResources& deviceResources,
	BackendDescriptorStore& descriptorStore,
	ID3D11Texture2D** inOutTexture
) noexcept {
	_inferenceEngine = std::make_unique<DirectMLInferenceBackend>();

	if (!_inferenceEngine->Initialize(modelPath, deviceResources, descriptorStore, *inOutTexture, inOutTexture)) {
		return false;
	}

	// 不保存 LearningModel 以降低内存占用
	// https://learn.microsoft.com/en-us/windows/ai/windows-ml/performance-memory#memory-utilization
	/*winrt::LearningModel learningModel{ nullptr };
	try {
		learningModel = winrt::LearningModel::LoadFromFilePath(modelPath);
	} catch (const winrt::hresult_error& e) {
		Logger::Get().ComError("创建 LearningModel 失败", e.code());
		return false;
	}

	_inputName = learningModel.InputFeatures().GetAt(0).Name();
	_outputName = learningModel.OutputFeatures().GetAt(0).Name();

	try {
		winrt::com_ptr<IDXGIDevice> dxgiDevice;
		deviceResources.GetD3DDevice()->QueryInterface<IDXGIDevice>(dxgiDevice.put());

		winrt::IDirect3DDevice wrappedD3DDevice{ nullptr };
		HRESULT hr = CreateDirect3D11DeviceFromDXGIDevice(
			dxgiDevice.get(),
			reinterpret_cast<::IInspectable**>(winrt::put_abi(wrappedD3DDevice))
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 IDirect3DDevice 失败", hr);
			return false;
		}

		winrt::LearningModelSessionOptions options;
		// 默认情况下每个 LearningModelSession 都会拷贝 LearningModel 中的内部模型表示，这对我们
		// 是多余的，因为之后不会再创建新的 LearningModelSession。CloseModelOnSessionCreation
		// 使 LearningModelSession 获得该内部模型表示的所有权，从而防止拷贝。
		options.CloseModelOnSessionCreation(true);

		_session = winrt::LearningModelSession{
			learningModel,
			winrt::LearningModelDevice::CreateFromDirect3D11Device(wrappedD3DDevice),
			options
		};
	} catch (const winrt::hresult_error& e) {
		Logger::Get().ComError("创建 LearningModelSession 失败", e.code());
		return false;
	}

#ifdef _DEBUG
	// 启用日志输出
	_session.EvaluationProperties().Insert(L"EnableDebugOutput", nullptr);
#endif
	
	SIZE inputSize{};
	{
		D3D11_TEXTURE2D_DESC inputDesc;
		(*inOutTexture)->GetDesc(&inputDesc);
		inputSize = { (LONG)inputDesc.Width, (LONG)inputDesc.Height };
	}

	// 创建输出纹理
	winrt::com_ptr<ID3D11Texture2D> output = DirectXHelper::CreateTexture2D(
		deviceResources.GetD3DDevice(),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		inputSize.cx * 2,
		inputSize.cy * 2,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
	);

	// 创建张量
	_inputTensor = winrt::ImageFeatureValue::CreateFromVideoFrame(TextureToVideoFrame(*inOutTexture));
	_outputTensor = winrt::ImageFeatureValue::CreateFromVideoFrame(TextureToVideoFrame(output.get()));

	*inOutTexture = output.get();*/

	return true;
}

void OnnxEffectDrawer::Draw(EffectsProfiler& /*profiler*/) const noexcept {
	_inferenceEngine->Evaluate();
	/*winrt::LearningModelBinding binding(_session);

	try {
		binding.Bind(_inputName, _inputTensor);

		winrt::PropertySet props;
		// 防止结果被拷贝到 CPU
		props.Insert(L"DisableTensorCpuSync", winrt::box_value(true));
		binding.Bind(_outputName, _outputTensor, props);
	} catch (winrt::hresult_error& e) {
		Logger::Get().ComError("绑定失败", e.code());
	}

	try {
		_session.Evaluate(binding, {});
	} catch (winrt::hresult_error& e) {
		Logger::Get().ComError("Evaluate 失败", e.code());
	}*/
}

}
