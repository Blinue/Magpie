#pragma once
#include "pch.h"
#include "FrameSourceBase.h"
#include <winrt/Windows.Graphics.Capture.h>


namespace winrt {
using namespace Windows::Foundation;
using namespace Windows::Graphics;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;
}


// 使用 Window Runtime 的 Windows.Graphics.Capture API 抓取窗口
// 见 https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/screen-capture
class GraphicsCaptureFrameSource : public FrameSourceBase {
public:
	GraphicsCaptureFrameSource() {};
	virtual ~GraphicsCaptureFrameSource();

	bool Initialize() override;

	ComPtr<ID3D11Texture2D> GetOutput() override;

	bool Update() override;

	bool HasRoundCornerInWin11() override {
		return true;
	}

private:
	D3D11_BOX _frameInWnd{};

	winrt::Direct3D11CaptureFramePool _captureFramePool{ nullptr };
	winrt::GraphicsCaptureSession _captureSession{ nullptr };
	winrt::IDirect3DDevice _wrappedD3DDevice{ nullptr };

	ComPtr<ID3D11DeviceContext> _d3dDC;

	ComPtr<ID3D11Texture2D> _output;
};
