#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"
#include "D2DContext.h"

#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include <Windows.Graphics.Capture.Interop.h>



namespace winrt {

using namespace Windows::Foundation;
using namespace Windows::Graphics;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;

}


// 使用 Window Runtime 的 Windows.Graphics.Capture API 抓取窗口
class WinRTCapturer : public WindowCapturerBase {
public:
	WinRTCapturer(
		HWND hwndSrc,
		const RECT& srcRect,
		D2DContext& d2dContext
	) : _srcRect(srcRect), _hwndSrc(hwndSrc), _d2dContext(d2dContext),
		_captureFramePool(nullptr), _captureSession(nullptr), _captureItem(nullptr), _wrappedDevice(nullptr)
	{
		winrt::init_apartment(winrt::apartment_type::multi_threaded);
		// 以下代码参考自 http://tips.hecomi.com/entry/2021/03/23/230947

		ID3D11Device* d3dDevice = d2dContext.GetD3DDevice();
		ComPtr<IDXGIDevice> dxgiDevice;
		Debug::ThrowIfComFailed(
			d3dDevice->QueryInterface<IDXGIDevice>(&dxgiDevice),
			L"获取 DXGI Device 失败"
		);

		Debug::ThrowIfComFailed(
			CreateDirect3D11DeviceFromDXGIDevice(
				dxgiDevice.Get(),
				reinterpret_cast<::IInspectable**>(winrt::put_abi(_wrappedDevice))
			),
			L"获取 IDirect3DDevice 失败"
		);
		Debug::Assert(_wrappedDevice, L"创建 IDirect3DDevice 失败");
		
		auto interop = winrt::get_activation_factory<winrt::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
		Debug::ThrowIfComFailed(
			interop->CreateForWindow
			(
				hwndSrc,
				winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
				winrt::put_abi(_captureItem)
			),
			L"创建 GraphicsCaptureItem 失败"
		);
		Debug::Assert(_captureItem, L"创建 GraphicsCaptureItem 失败");

		_captureFramePool = winrt::Direct3D11CaptureFramePool::Create(
			_wrappedDevice, // D3D device
			winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized, // Pixel format
			1, // Number of frames
			_captureItem.Size() // Size of the buffers
		);
		Debug::Assert(_captureFramePool, L"创建 Direct3D11CaptureFramePool 失败");

		_captureSession = _captureFramePool.CreateCaptureSession(_captureItem);
		Debug::Assert(_captureSession, L"CreateCaptureSession 失败");
		_captureSession.IsCursorCaptureEnabled(false);

		_captureSession.StartCapture();
	}

	~WinRTCapturer() {
		if (_captureSession) {
			_captureSession.Close();
		}
		if (_captureFramePool) {
			_captureFramePool.Close();
		}

		winrt::uninit_apartment();
	}

	std::variant<ComPtr<IWICBitmapSource>, ComPtr<ID2D1Bitmap1>> GetFrame() override {
		winrt::Direct3D11CaptureFrame frame = _captureFramePool.TryGetNextFrame();
		if (!frame) {
			return ComPtr<ID2D1Bitmap1>();
		}
		winrt::IDirect3DSurface d3dSurface = frame.Surface();

		winrt::com_ptr<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess> dxgiInterfaceAccess(
			d3dSurface.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>()
		);
		winrt::com_ptr<::IDXGISurface> dxgiSurface;
		Debug::ThrowIfComFailed(
			dxgiInterfaceAccess->GetInterface(
				__uuidof(dxgiSurface),
				dxgiSurface.put_void()
			),
			L"从获取 IDirect3DSurface 获取 IDXGISurface 失败"
		);

		ComPtr<ID2D1Bitmap1> bmp;
		Debug::ThrowIfComFailed(
			_d2dContext.GetD2DDC()->CreateBitmapFromDxgiSurface(dxgiSurface.get(), BitmapProperties1(D2D1_BITMAP_OPTIONS_NONE, PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)), &bmp),
			L""
		);
		return bmp;
	}

private:
	const RECT& _srcRect;
	HWND _hwndSrc;
	D2DContext& _d2dContext;

	winrt::Direct3D11CaptureFramePool _captureFramePool;
	winrt::GraphicsCaptureSession _captureSession;
	winrt::GraphicsCaptureItem _captureItem;
	winrt::IDirect3DDevice _wrappedDevice;
};
