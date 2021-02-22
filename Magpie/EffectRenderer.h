#pragma once
#include "pch.h"
#include "Utils.h"
#include "EffectManager.h"

using namespace D2D1;

class EffectRenderer {
public:
	EffectRenderer(HWND hwndHost, const std::wstring_view &effectsJson, const SIZE& srcSize) {
        RECT clientRect{};
        Utils::GetClientScreenRect(hwndHost, clientRect);
        _hostWndClientSize = Utils::GetSize(clientRect);

        _InitD2D(hwndHost);

        _effectManager.reset(new EffectManager(_d2dFactory, _d2dDC, effectsJson, srcSize, _hostWndClientSize));
	}
 
public:
    void Render(ComPtr<IWICBitmapSource> srcBmp) const {
        ComPtr<ID2D1Image> outputImg = _effectManager->Apply(srcBmp);

        // »ñÈ¡Êä³öÍ¼Ïñ³ß´ç
        D2D1_RECT_F outputRect{};
        Debug::ThrowIfFailed(
            _d2dDC->GetImageLocalBounds(outputImg.Get(), &outputRect),
            L"»ñÈ¡Êä³öÍ¼Ïñ³ß´çÊ§°Ü"
        );
        D2D1_SIZE_F outputSize = Utils::GetSize(outputRect);

        // ½«Êä³öÍ¼ÏñÏÔÊ¾ÔÚ´°¿ÚÖÐÑë
        _d2dDC->BeginDraw();
        _d2dDC->Clear(D2D1_COLOR_F{ 0,1,0,1 });
        _d2dDC->DrawImage(
            outputImg.Get(),
            D2D1_POINT_2F{
                ((float)_hostWndClientSize.cx - outputSize.width) / 2,
                ((float)_hostWndClientSize.cy - outputSize.height) / 2
            }
        );

        Debug::ThrowIfFailed(
            _d2dDC->EndDraw(),
            L"EndDraw Ê§°Ü"
        );

        Debug::ThrowIfFailed(
            _d2dSwapChain->Present(0, 0),
            L"Present Ê§°Ü"
        );
    }
private:
    void _InitD2D(HWND hwndHost) {
        // This array defines the set of DirectX hardware feature levels this app  supports.
        // The ordering is important and you should  preserve it.
        // Don't forget to declare your app's minimum required feature level in its
        // description.  All apps are assumed to support 9.1 unless otherwise stated.
        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
        };

        // Create the DX11 API device object, and get a corresponding context.
        ComPtr<ID3D11Device> d3dDevice = nullptr;
        ComPtr<ID3D11DeviceContext> d3dDC = nullptr;

        D3D_FEATURE_LEVEL fl;
        Debug::ThrowIfFailed(
            D3D11CreateDevice(
                nullptr,                    // specify null to use the default adapter
                D3D_DRIVER_TYPE_HARDWARE,
                NULL,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                featureLevels,              // list of feature levels this app can support
                ARRAYSIZE(featureLevels),   // number of possible feature levels
                D3D11_SDK_VERSION,
                &d3dDevice,                    // returns the Direct3D device created
                &fl,            // returns feature level of device created
                &d3dDC                    // returns the device immediate context
            ),
            L""
        );

        // Obtain the underlying DXGI device of the Direct3D11 device.
        ComPtr<IDXGIDevice1> dxgiDevice;
        Debug::ThrowIfFailed(
            d3dDevice.As(&dxgiDevice),
            L""
        );

        Debug::ThrowIfFailed(
            D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &_d2dFactory),
            L""
        );

        // Obtain the Direct2D device for 2-D rendering.
        
        Debug::ThrowIfFailed(
            _d2dFactory->CreateDevice(dxgiDevice.Get(), &_d2dDevice),
            L""
        );
        
        // Get Direct2D device's corresponding device context object.
        Debug::ThrowIfFailed(
            _d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &_d2dDC),
            L""
        );

        // Identify the physical adapter (GPU or card) this device is runs on.
        ComPtr<IDXGIAdapter> dxgiAdapter;
        Debug::ThrowIfFailed(
            dxgiDevice->GetAdapter(&dxgiAdapter),
            L""
        );

        // Get the factory object that created the DXGI device.
        ComPtr<IDXGIFactory2> dxgiFactory = nullptr;
        Debug::ThrowIfFailed(
            dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)),
            L""
        );
        
        // Allocate a descriptor.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.Width = _hostWndClientSize.cx,
        swapChainDesc.Height = _hostWndClientSize.cy,
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // this is the most common swapchain format
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;                // don't use multi-sampling
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;                     // use double buffering to enable flip
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        // Get the final swap chain for this window from the DXGI factory.

        Debug::ThrowIfFailed(
            dxgiFactory->CreateSwapChainForHwnd(
                d3dDevice.Get(),
                hwndHost,
                &swapChainDesc,
                nullptr,
                nullptr,
                &_d2dSwapChain
            ),
            L""
        );
        
        // Ensure that DXGI doesn't queue more than one frame at a time.
        Debug::ThrowIfFailed(
            dxgiDevice->SetMaximumFrameLatency(1),
            L""
        );

        // Direct2D needs the dxgi version of the backbuffer surface pointer.
        ComPtr<IDXGISurface> dxgiBackBuffer = nullptr;
        Debug::ThrowIfFailed(
            _d2dSwapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer)),
            L""
        );

        // Now we set up the Direct2D render target bitmap linked to the swapchain. 
        // Whenever we render to this bitmap, it is directly rendered to the 
        // swap chain associated with the window.
        D2D1_BITMAP_PROPERTIES1 bitmapProperties = BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)
        );

        // Get a D2D surface from the DXGI back buffer to use as the D2D render target.
        ComPtr<ID2D1Bitmap1> d2dTargetBitmap = nullptr;
        Debug::ThrowIfFailed(
            _d2dDC->CreateBitmapFromDxgiSurface(
                dxgiBackBuffer.Get(),
                &bitmapProperties,
                &d2dTargetBitmap
            ),
            L""
        );

        // Now we can set the Direct2D render target.
        _d2dDC->SetTarget(d2dTargetBitmap.Get());
        _d2dDC->SetUnitMode(D2D1_UNIT_MODE_PIXELS);

        D2D1_RENDERING_CONTROLS rc{};
        _d2dDC->GetRenderingControls(&rc);
        rc.bufferPrecision = D2D1_BUFFER_PRECISION_32BPC_FLOAT;
        rc.tileSize.width = _hostWndClientSize.cx * 2;
        rc.tileSize.height = _hostWndClientSize.cy * 2;
        _d2dDC->SetRenderingControls(rc);
    }


    SIZE _hostWndClientSize{};

    ComPtr<ID2D1Factory1> _d2dFactory = nullptr;
    ComPtr<ID2D1Device> _d2dDevice = nullptr;
    ComPtr<ID2D1DeviceContext> _d2dDC = nullptr;
    ComPtr<IDXGISwapChain1> _d2dSwapChain = nullptr;

    std::unique_ptr<EffectManager> _effectManager = nullptr;
};
