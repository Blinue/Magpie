#pragma once
#include "pch.h"
#include "FrameSourceBase.h"
#include <winrt/Windows.Graphics.Capture.h>
#include <Windows.Graphics.Capture.Interop.h>
#include "Win32Utils.h"


namespace Magpie::Core {

// 使用 Window Runtime 的 Windows.Graphics.Capture API 抓取窗口
// 见 https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/screen-capture
class GraphicsCaptureFrameSource : public FrameSourceBase {
public:
	GraphicsCaptureFrameSource() {};
	virtual ~GraphicsCaptureFrameSource();

	bool Initialize() override;

	UpdateState Update() override;

	bool IsScreenCapture() override {
		return _isScreenCapture;
	}

	const char* GetName() const noexcept override {
		return "Graphics Capture";
	}

	bool StartCapture();

	void StopCapture();

protected:
	bool _HasRoundCornerInWin11() override {
		return true;
	}

	bool _CanCaptureTitleBar() override {
		return true;
	}

private:
	bool _CaptureFromWindow(IGraphicsCaptureItemInterop* interop);

	bool _CaptureFromStyledWindow(IGraphicsCaptureItemInterop* interop);

	bool _CaptureFromMonitor(IGraphicsCaptureItemInterop* interop);

	LONG_PTR _srcWndStyle = 0;
	D3D11_BOX _frameBox{};

	bool _isScreenCapture = false;

	winrt::Windows::Graphics::Capture::GraphicsCaptureItem _captureItem{ nullptr };
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool _captureFramePool{ nullptr };
	winrt::Windows::Graphics::Capture::GraphicsCaptureSession _captureSession{ nullptr };
	winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice _wrappedD3DDevice{ nullptr };
};

}
