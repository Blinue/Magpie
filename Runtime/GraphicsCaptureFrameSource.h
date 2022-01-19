#pragma once
#include "pch.h"
#include "FrameSourceBase.h"
#include <winrt/Windows.Graphics.Capture.h>
#include <Windows.Graphics.Capture.Interop.h>


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

	ComPtr<ID3D11Texture2D> GetOutput() override {
		return _output;
	}

	UpdateState Update() override;

	bool HasRoundCornerInWin11() override {
		return true;
	}

	bool IsScreenCapture() override {
		return _isScreenCapture;
	}

private:
	bool _CaptureFromWindow(winrt::impl::com_ref<IGraphicsCaptureItemInterop> interop);

	bool _CaptureFromStyledWindow(winrt::impl::com_ref<IGraphicsCaptureItemInterop> interop);

	bool _CaptureFromMonitor(winrt::impl::com_ref<IGraphicsCaptureItemInterop> interop);

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
	CRITICAL_SECTION _cs{};
	bool _newFrameArrived = false;

	ComPtr<ID3D11Texture2D> _output;
};
