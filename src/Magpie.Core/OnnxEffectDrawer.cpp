#include "pch.h"
#include "OnnxEffectDrawer.h"

#include <winrt/Windows.Media.h>
#include "DirectXHelper.h"
#include "DeviceResources.h"
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include "Logger.h"

namespace winrt {
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media;
using namespace Microsoft::AI::MachineLearning;
}

using namespace Windows::Graphics::DirectX::Direct3D11;

namespace Magpie::Core {

bool OnnxEffectDrawer::Initialize(
	const wchar_t* modelPath,
	DeviceResources& deviceResources,
	ID3D11Texture2D** inOutTexture
) noexcept {
	_model = winrt::LearningModel::LoadFromFilePath(modelPath);

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
	winrt::LearningModelDevice device = winrt::LearningModelDevice::CreateFromDirect3D11Device(wrappedD3DDevice);
	_session = winrt::LearningModelSession{ _model, device };
	
	SIZE inputSize{};
	{
		D3D11_TEXTURE2D_DESC inputDesc;
		(*inOutTexture)->GetDesc(&inputDesc);
		inputSize = { (LONG)inputDesc.Width, (LONG)inputDesc.Height };
	}

	{
		winrt::com_ptr<IDXGISurface> dxgiSurface;
		(*inOutTexture)->QueryInterface<IDXGISurface>(dxgiSurface.put());

		winrt::IDirect3DSurface wrappedSource;
		CreateDirect3D11SurfaceFromDXGISurface(
			dxgiSurface.get(),
			reinterpret_cast<IInspectable**>(winrt::put_abi(wrappedSource))
		);
		_inputFrame = winrt::VideoFrame::CreateWithDirect3D11Surface(wrappedSource);
	}

	// 创建输出纹理
	winrt::com_ptr<ID3D11Texture2D> output = DirectXHelper::CreateTexture2D(
		deviceResources.GetD3DDevice(),
		DXGI_FORMAT_R8G8B8A8_UNORM,
		inputSize.cx * 2,
		inputSize.cy * 2,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
	);
	*inOutTexture = output.get();

	{
		winrt::com_ptr<IDXGISurface> dxgiSurface = output.as<IDXGISurface>();

		winrt::IDirect3DSurface wrappedOutput;
		CreateDirect3D11SurfaceFromDXGISurface(
			dxgiSurface.get(),
			reinterpret_cast<IInspectable**>(winrt::put_abi(wrappedOutput))
		);
		_outputFrame = winrt::VideoFrame::CreateWithDirect3D11Surface(wrappedOutput);
	}

	_modelInputFrame = winrt::VideoFrame::CreateAsDirect3D11SurfaceBacked(
		winrt::DirectXPixelFormat::R8G8B8A8UIntNormalized, inputSize.cx, inputSize.cy, wrappedD3DDevice);
	_modelOutputFrame = winrt::VideoFrame::CreateAsDirect3D11SurfaceBacked(
		winrt::DirectXPixelFormat::R8G8B8A8UIntNormalized, inputSize.cx * 2, inputSize.cy * 2, wrappedD3DDevice);

	// create the input/output tensors
	_inputTensor = winrt::ImageFeatureValue::CreateFromVideoFrame(_modelInputFrame);
	_outputTensor = winrt::ImageFeatureValue::CreateFromVideoFrame(_modelOutputFrame);

	return true;
}

template <typename Async>
auto WaitAsyncAction(Async const& async) {
	auto status = async.Status();
	if (status == winrt::AsyncStatus::Started) {
		status = winrt::impl::wait_for_completed(async, 0xFFFFFFFF); // INFINITE
	}
	winrt::impl::check_status_canceled(status);

	return async.GetResults();
}

void OnnxEffectDrawer::Draw(EffectsProfiler& /*profiler*/) const noexcept {
	/*Ort::Env env;
	Ort::SessionOptions sessionOptions;
	Ort::Session session = Ort::Session(env, L"D:\\Upscale_L.onnx", sessionOptions);

	winrt::VideoFrame inputFrame(winrt::BitmapPixelFormat::Rgba16, 1366, 768, winrt::BitmapAlphaMode::Ignore);

	const char* inputName = "input";
	const char* outputName = "output";
	Ort::IoBinding binding(session);
	//binding.BindInput("input", inputFrame);
	//session.Run(Ort::RunOptions(nullptr), binding);*/

	// copy the source frame to the input frame
	WaitAsyncAction(_inputFrame.CopyToAsync(_modelInputFrame));

	winrt::LearningModelBinding binding(_session);
	binding.Bind(_model.InputFeatures().GetAt(0).Name(), _inputTensor);
	binding.Bind(_model.OutputFeatures().GetAt(0).Name(), _outputTensor);
	_session.Evaluate(binding, L"test");
	
	WaitAsyncAction(_modelOutputFrame.CopyToAsync(_outputFrame));
}

}
