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

static winrt::VideoFrame TextureToVideoFrame(ID3D11Texture2D* texture) noexcept {
	winrt::com_ptr<IDXGISurface> dxgiSurface;
	texture->QueryInterface<IDXGISurface>(dxgiSurface.put());

	winrt::IDirect3DSurface wrappedSurface;
	CreateDirect3D11SurfaceFromDXGISurface(
		dxgiSurface.get(),
		reinterpret_cast<IInspectable**>(winrt::put_abi(wrappedSurface))
	);
	return winrt::VideoFrame::CreateWithDirect3D11Surface(wrappedSurface);
}

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

	*inOutTexture = output.get();

	return true;
}

void OnnxEffectDrawer::Draw(EffectsProfiler& /*profiler*/) const noexcept {
	winrt::LearningModelBinding binding(_session);
	binding.Bind(_model.InputFeatures().GetAt(0).Name(), _inputTensor);

	winrt::PropertySet props;
	props.Insert(L"DisableTensorCpuSync", winrt::box_value(true));
	binding.Bind(_model.OutputFeatures().GetAt(0).Name(), _outputTensor, props);

	_session.Evaluate(binding, L"test");
}

}
