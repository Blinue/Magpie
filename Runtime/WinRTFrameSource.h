#pragma once
#include "pch.h"
#include "FrameSourceBase.h"
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
class WinRTFrameSource : public FrameSourceBase {
public:
	WinRTFrameSource() {};
	~WinRTFrameSource();

	bool Initialize() override;

	ComPtr<ID3D11Texture2D> GetOutput() override;

	bool Update() override;

private:
	D2D1_RECT_U _clientInFrame{};

	winrt::Direct3D11CaptureFramePool _captureFramePool{ nullptr };
	winrt::GraphicsCaptureSession _captureSession{ nullptr };
	winrt::GraphicsCaptureItem _captureItem{ nullptr };
	winrt::IDirect3DDevice _wrappedD3DDevice{ nullptr };

	ComPtr<ID3D11DeviceContext4> _d3dDC = nullptr;

	ComPtr<ID3D11Texture2D> _output;
};
