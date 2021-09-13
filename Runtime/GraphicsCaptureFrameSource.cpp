#include "pch.h"
#include "GraphicsCaptureFrameSource.h"
#include "App.h"
#include "Utils.h"


extern std::shared_ptr<spdlog::logger> logger;

bool GraphicsCaptureFrameSource::Initialize() {
	_d3dDC = App::GetInstance().GetRenderer().GetD3DDC();
    HWND hwndSrc = App::GetInstance().GetHwndSrc();

    // 包含边框的窗口尺寸
    RECT srcRect{};
    HRESULT hr = DwmGetWindowAttribute(hwndSrc, DWMWA_EXTENDED_FRAME_BOUNDS, &srcRect, sizeof(srcRect));
    if (FAILED(hr)) {
        SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("DwmGetWindowAttribute 失败", hr));
        return false;
    }

    const RECT& srcClient = App::GetInstance().GetSrcClientRect();
    _clientInFrame = {
        UINT(srcClient.left - srcRect.left),
		UINT(srcClient.top - srcRect.top),
		0,
		UINT(srcClient.right - srcRect.left),
		UINT(srcClient.bottom - srcRect.top),
		1
    };

    try {
        // Windows.Graphics.Capture API 似乎只能运行于 MTA，造成诸多麻烦
        winrt::init_apartment(winrt::apartment_type::multi_threaded);

        if (!winrt::ApiInformation::IsTypePresent(L"Windows.Graphics.Capture.GraphicsCaptureSession")) {
            SPDLOG_LOGGER_ERROR(logger, "不存在 GraphicsCaptureSession API");
            App::SetErrorMsg(ErrorMessages::WINRT);
            return false;
        }
        if (!winrt::GraphicsCaptureSession::IsSupported()) {
            SPDLOG_LOGGER_ERROR(logger, "当前不支持 WinRT 捕获");
            App::SetErrorMsg(ErrorMessages::WINRT);
            return false;
        }

        hr = CreateDirect3D11DeviceFromDXGIDevice(
            App::GetInstance().GetRenderer().GetDXGIDevice().Get(),
            reinterpret_cast<::IInspectable**>(winrt::put_abi(_wrappedD3DDevice))
        );
        if (FAILED(hr)) {
            SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 IDirect3DDevice 失败", hr));
            App::SetErrorMsg(ErrorMessages::WINRT);
            return false;
        }

        // 从窗口句柄获取 GraphicsCaptureItem
        auto interop = winrt::get_activation_factory<winrt::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        if (!interop) {
            SPDLOG_LOGGER_ERROR(logger, "获取 IGraphicsCaptureItemInterop 失败");
            App::SetErrorMsg(ErrorMessages::WINRT);
            return false;
        }

        hr = interop->CreateForWindow(
            hwndSrc,
            winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
            winrt::put_abi(_captureItem)
        );
        if (FAILED(hr)) {
            SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 GraphicsCaptureItem 失败", hr));
            App::SetErrorMsg(ErrorMessages::WINRT);
            return false;
        }

        // 创建帧缓冲池
        _captureFramePool = winrt::Direct3D11CaptureFramePool::Create(
            _wrappedD3DDevice,
            winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            1,					// 帧的缓存数量
            _captureItem.Size() // 帧的尺寸
        );

        // 开始捕获
        _captureSession = _captureFramePool.CreateCaptureSession(_captureItem);

        if (winrt::ApiInformation::IsPropertyPresent(
            L"Windows.Graphics.Capture.GraphicsCaptureSession",
            L"IsCursorCaptureEnabled"
        )) {
            // 从 v2004 开始提供
            _captureSession.IsCursorCaptureEnabled(false);
		} else {
			SPDLOG_LOGGER_INFO(logger, "当前系统无 IsCursorCaptureEnabled API");
		}

        _captureSession.StartCapture();
    } catch (const winrt::hresult_error& e) {
        SPDLOG_LOGGER_ERROR(logger, fmt::format("初始化 WinRT 失败：{}", Utils::UTF16ToUTF8(e.message())));
        App::SetErrorMsg(ErrorMessages::WINRT);
        return false;
    }


    D3D11_TEXTURE2D_DESC desc{};
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Width = _clientInFrame.right - _clientInFrame.left;
    desc.Height = _clientInFrame.bottom - _clientInFrame.top;
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

	_d3dDC->CopySubresourceRegion(_output.Get(), 0, 0, 0, 0, withFrame.Get(), 0, &_clientInFrame);

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
