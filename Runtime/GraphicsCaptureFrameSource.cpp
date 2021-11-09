#include "pch.h"
#include "GraphicsCaptureFrameSource.h"
#include "App.h"
#include "StrUtils.h"
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include <Windows.Graphics.Capture.Interop.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include "Utils.h"


namespace winrt {
using namespace Windows::Foundation::Metadata;
}


extern std::shared_ptr<spdlog::logger> logger;

bool GraphicsCaptureFrameSource::Initialize() {
	App::GetInstance().SetErrorMsg(ErrorMessages::GRAPHICS_CAPTURE);

	// 只在 Win10 1903 及更新版本中可用
	const RTL_OSVERSIONINFOW& version = Utils::GetOSVersion();
	if (Utils::CompareVersion(version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber, 10, 0, 18362) < 0) {
		SPDLOG_LOGGER_ERROR(logger, "当前操作系统无法使用 GraphicsCapture");
		return false;
	}

	_d3dDC = App::GetInstance().GetRenderer().GetD3DDC();
	HWND hwndSrc = App::GetInstance().GetHwndSrc();

	// 包含边框的窗口尺寸
	RECT srcRect{};
	HRESULT hr = DwmGetWindowAttribute(hwndSrc, DWMWA_EXTENDED_FRAME_BOUNDS, &srcRect, sizeof(srcRect));
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("DwmGetWindowAttribute 失败", hr));
		return false;
	}

	const RECT& srcClient = App::GetInstance().GetSrcClientRect();
	
	// 在源窗口存在 DPI 缩放时有时会有一像素的偏移（取决于窗口在屏幕上的位置）
	// 可能是 DwmGetWindowAttribute 的 bug
	_frameInWnd = {
		UINT(srcClient.left - srcRect.left),
		UINT(srcClient.top - srcRect.top),
		0,
		UINT(srcClient.right - srcRect.left),
		UINT(srcClient.bottom - srcRect.top),
		1
	};

	SIZE frameSize = { LONG(_frameInWnd.right - _frameInWnd.left), LONG(_frameInWnd.bottom - _frameInWnd.top) };

	try {
		if (!winrt::ApiInformation::IsTypePresent(L"Windows.Graphics.Capture.GraphicsCaptureSession")) {
			SPDLOG_LOGGER_ERROR(logger, "不存在 GraphicsCaptureSession API");
			return false;
		}
		if (!winrt::GraphicsCaptureSession::IsSupported()) {
			SPDLOG_LOGGER_ERROR(logger, "当前不支持 WinRT 捕获");
			return false;
		}

		hr = CreateDirect3D11DeviceFromDXGIDevice(
			App::GetInstance().GetRenderer().GetDXGIDevice().Get(),
			reinterpret_cast<::IInspectable**>(winrt::put_abi(_wrappedD3DDevice))
		);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 IDirect3DDevice 失败", hr));
			return false;
		}

		// 从窗口句柄获取 GraphicsCaptureItem
		auto interop = winrt::get_activation_factory<winrt::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
		if (!interop) {
			SPDLOG_LOGGER_ERROR(logger, "获取 IGraphicsCaptureItemInterop 失败");
			return false;
		}

		winrt::GraphicsCaptureItem captureItem{ nullptr };
		hr = interop->CreateForWindow(
			hwndSrc,
			winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
			winrt::put_abi(captureItem)
		);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 GraphicsCaptureItem 失败", hr));
			return false;
		}
		
		// 创建帧缓冲池
		// 帧的尺寸为不含阴影的窗口尺寸，和 _captureItem.Size() 不同
		_captureFramePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(
			_wrappedD3DDevice,
			winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized,
			1,	// 帧的缓存数量
			{ srcRect.right - srcRect.left, srcRect.bottom - srcRect.top } // 帧的尺寸
		);

		// 开始捕获
		_captureSession = _captureFramePool.CreateCaptureSession(captureItem);

		// 不捕获光标
		if (winrt::ApiInformation::IsPropertyPresent(
			L"Windows.Graphics.Capture.GraphicsCaptureSession",
			L"IsCursorCaptureEnabled"
		)) {
			// 从 v2004 开始提供
			_captureSession.IsCursorCaptureEnabled(false);
		} else {
			SPDLOG_LOGGER_INFO(logger, "当前系统无 IsCursorCaptureEnabled API");
		}

		// 不显示黄色边框
		if (winrt::ApiInformation::IsPropertyPresent(
			L"Windows.Graphics.Capture.GraphicsCaptureSession",
			L"IsBorderRequired"
		)) {
			// 从 Win11 开始提供
			// 先请求权限
			auto status = winrt::GraphicsCaptureAccess::RequestAccessAsync(winrt::GraphicsCaptureAccessKind::Borderless).get();
			if (status == decltype(status)::Allowed) {
				_captureSession.IsBorderRequired(false);
			} else {
				SPDLOG_LOGGER_INFO(logger, "IsCursorCaptureEnabled 失败");
			}
		} else {
			SPDLOG_LOGGER_INFO(logger, "当前系统无 IsBorderRequired API");
		}

		_captureSession.StartCapture();
	} catch (const winrt::hresult_error& e) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("初始化 WinRT 失败：{}", StrUtils::UTF16ToUTF8(e.message())));
		return false;
	}


	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width = frameSize.cx;
	desc.Height = frameSize.cy;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	hr = App::GetInstance().GetRenderer().GetD3DDevice()->CreateTexture2D(&desc, nullptr, &_output);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
		return false;
	}

	App::GetInstance().SetErrorMsg(ErrorMessages::GENERIC);
	SPDLOG_LOGGER_INFO(logger, "GraphicsCaptureFrameSource 初始化完成");
	return true;
}

ComPtr<ID3D11Texture2D> GraphicsCaptureFrameSource::GetOutput() {
	return _output;
}

bool GraphicsCaptureFrameSource::Update() {
	winrt::Direct3D11CaptureFrame frame = _captureFramePool.TryGetNextFrame();
	if (!frame) {
		// 缓冲池没有帧
		return false;
	}

	// 从帧获取 IDXGISurface
	winrt::IDirect3DSurface d3dSurface = frame.Surface();

	winrt::com_ptr<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess> dxgiInterfaceAccess(
		d3dSurface.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>()
	);

	ComPtr<ID3D11Texture2D> withFrame;
	HRESULT hr = dxgiInterfaceAccess->GetInterface(IID_PPV_ARGS(&withFrame));
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从获取 IDirect3DSurface 获取 ID3D11Texture2D 失败", hr));
		return false;
	}

	_d3dDC->CopySubresourceRegion(_output.Get(), 0, 0, 0, 0, withFrame.Get(), 0, &_frameInWnd);

	return true;
}

GraphicsCaptureFrameSource::~GraphicsCaptureFrameSource() {
	if (_captureSession) {
		_captureSession.Close();
	}
	if (_captureFramePool) {
		_captureFramePool.Close();
	}

	winrt::uninit_apartment();
}
