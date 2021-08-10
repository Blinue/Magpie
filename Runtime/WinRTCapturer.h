#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"
#include "D2DContext.h"
#include "Env.h"

#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include <Windows.Graphics.Capture.Interop.h>
#include <winrt/Windows.Foundation.Metadata.h>

namespace winrt {
using namespace Windows::Foundation;
using namespace Windows::Foundation::Metadata;
using namespace Windows::Graphics;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;
}


// 使用 Window Runtime 的 Windows.Graphics.Capture API 抓取窗口
// 见 https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/screen-capture
class WinRTCapturer : public WindowCapturerBase {
public:
	WinRTCapturer() {
		HWND hwndSrc = Env::$instance->GetHwndSrc();
		
		// 包含边框的窗口尺寸
		RECT srcRect{};
		Debug::ThrowIfComFailed(
			DwmGetWindowAttribute(hwndSrc, DWMWA_EXTENDED_FRAME_BOUNDS, &srcRect, sizeof(srcRect)),
			L"GetWindowRect 失败"
		);

		const RECT& srcClient = Env::$instance->GetSrcClient();
		_clientInFrame = {
			UINT32(srcClient.left - srcRect.left),
			UINT32(srcClient.top - srcRect.top),
			UINT32(srcClient.right - srcRect.left),
			UINT32(srcClient.bottom - srcRect.top)
		};

		try {
			// Windows.Graphics.Capture API 似乎只能运行于 MTA，造成诸多麻烦
			winrt::init_apartment(winrt::apartment_type::multi_threaded);

			Debug::Assert(
				winrt::ApiInformation::IsTypePresent(L"Windows.Graphics.Capture.GraphicsCaptureSession"),
				L"无法使用WinRT捕获"
			);

			Debug::Assert(winrt::GraphicsCaptureSession::IsSupported(), L"当前系统不支持 WinRT Capture");

			// 以下代码参考自 http://tips.hecomi.com/entry/2021/03/23/230947

			ComPtr<IDXGIDevice> dxgiDevice;
			Debug::ThrowIfComFailed(
				Env::$instance->GetD3DDevice()->QueryInterface<IDXGIDevice>(&dxgiDevice),
				L"获取 DXGI Device 失败"
			);

			Debug::ThrowIfComFailed(
				CreateDirect3D11DeviceFromDXGIDevice(
					dxgiDevice.Get(),
					reinterpret_cast<::IInspectable**>(winrt::put_abi(_wrappedD3DDevice))
				),
				L"获取 IDirect3DDevice 失败"
			);
			Debug::Assert(_wrappedD3DDevice, L"创建 IDirect3DDevice 失败");

			// 从窗口句柄获取 GraphicsCaptureItem
			auto interop = winrt::get_activation_factory<winrt::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
			Debug::Assert(interop, L"获取IGraphicsCaptureItemInterop失败");

			Debug::ThrowIfComFailed(
				interop->CreateForWindow(
					hwndSrc,
					winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
					winrt::put_abi(_captureItem)
				),
				L"创建 GraphicsCaptureItem 失败"
			);
			Debug::Assert(_captureItem, L"创建 GraphicsCaptureItem 失败");

			// 创建帧缓冲池
			_captureFramePool = winrt::Direct3D11CaptureFramePool::Create(
				_wrappedD3DDevice,
				winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized,
				1,					// 帧的缓存数量
				_captureItem.Size() // 帧的尺寸
			);
			Debug::Assert(_captureFramePool, L"创建 Direct3D11CaptureFramePool 失败");

			// 开始捕获
			_captureSession = _captureFramePool.CreateCaptureSession(_captureItem);
			Debug::Assert(_captureSession, L"CreateCaptureSession 失败");
			
			if (winrt::ApiInformation::IsPropertyPresent(
				L"Windows.Graphics.Capture.GraphicsCaptureSession",
				L"IsCursorCaptureEnabled"
			)) {
				// 从 v2004 开始提供
				_captureSession.IsCursorCaptureEnabled(false);
			}
			_captureSession.StartCapture();
		} catch (winrt::hresult_error) {
			// 有些窗口无法用WinRT捕获
			Debug::Assert(false, L"WinRT 捕获出错");
		}
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

	CaptureredFrameType GetFrameType() override {
		return CaptureredFrameType::D2DImage;
	}

	ComPtr<IUnknown> GetFrame() override {
		ID2D1DeviceContext* d2dDC = Env::$instance->GetD2DDC();

		winrt::Direct3D11CaptureFrame frame = _captureFramePool.TryGetNextFrame();
		if (!frame) {
			// 缓冲池没有帧就返回 nullptr
			return nullptr;
		}

		// 从帧获取 IDXGISurface
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

		// 从 IDXGISurface 获取 ID2D1Bitmap
		// 这里使用共享以避免拷贝
		ComPtr<ID2D1Bitmap> withFrame;
		auto p = BitmapProperties(PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));
		d2dDC->CreateSharedBitmap(__uuidof(IDXGISurface), dxgiSurface.get(), &p, &withFrame);

		// 获取的帧包含窗口边框，将客户区内容拷贝到新的 ID2D1Bitmap
		ComPtr<ID2D1Bitmap> withoutFrame;
		d2dDC->CreateBitmap(
			{ _clientInFrame.right - _clientInFrame.left, _clientInFrame.bottom - _clientInFrame.top },
			BitmapProperties(PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
			&withoutFrame
		);

		D2D1_POINT_2U destPoint{ 0,0 };
		withoutFrame->CopyFromBitmap(&destPoint, withFrame.Get(), &_clientInFrame);

		return withoutFrame;
	}

private:
	D2D1_RECT_U _clientInFrame;

	winrt::Direct3D11CaptureFramePool _captureFramePool{ nullptr };
	winrt::GraphicsCaptureSession _captureSession{ nullptr };
	winrt::GraphicsCaptureItem _captureItem{ nullptr };
	winrt::IDirect3DDevice _wrappedD3DDevice{ nullptr };
};
