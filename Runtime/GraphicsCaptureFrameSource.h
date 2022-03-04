#pragma once
#include "pch.h"
#include "FrameSourceBase.h"
#include <winrt/Windows.Graphics.Capture.h>
#include <Windows.Graphics.Capture.Interop.h>
#include "Utils.h"


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

	UpdateState Update() override;

	bool IsScreenCapture() override {
		return _isScreenCapture;
	}

protected:
	bool _HasRoundCornerInWin11() override {
		return true;
	}

private:
	bool _CaptureFromWindow(IGraphicsCaptureItemInterop* interop);

	bool _CaptureFromStyledWindow(IGraphicsCaptureItemInterop* interop);

	bool _CaptureFromMonitor(IGraphicsCaptureItemInterop* interop);

	void _OnFrameArrived(winrt::Direct3D11CaptureFramePool const&, winrt::IInspectable const&);

	LONG_PTR _srcWndStyle = 0;
	D3D11_BOX _frameBox{};

	bool _isScreenCapture = false;

	winrt::GraphicsCaptureItem _captureItem{ nullptr };
	winrt::Direct3D11CaptureFramePool _captureFramePool{ nullptr };
	winrt::GraphicsCaptureSession _captureSession{ nullptr };
	winrt::IDirect3DDevice _wrappedD3DDevice{ nullptr };
	winrt::Direct3D11CaptureFramePool::FrameArrived_revoker _frameArrived;

	// 用于线程同步
	CONDITION_VARIABLE _cv{};
	Utils::CSMutex _cs;
	bool _newFrameArrived = false;
};
