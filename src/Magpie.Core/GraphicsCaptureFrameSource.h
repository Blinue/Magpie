#pragma once
#include "FrameSourceBase.h"
#include <winrt/Windows.Graphics.Capture.h>
#include <Windows.Graphics.Capture.Interop.h>

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
		return NAME;
	}

	bool StartCapture();

	void StopCapture();

	static constexpr const char* NAME = "Graphics Capture";

protected:
	bool _HasRoundCornerInWin11() override {
		return true;
	}

	bool _CanCaptureTitleBar() override {
		return true;
	}

private:
	bool _CaptureWindow(IGraphicsCaptureItemInterop* interop);

	bool _CaptureMonitor(IGraphicsCaptureItemInterop* interop);

	bool _TryCreateGraphicsCaptureItem(IGraphicsCaptureItemInterop* interop, HWND hwndSrc) noexcept;

	void _RemoveOwnerFromAltTabList(HWND hwndSrc) noexcept;

	LONG_PTR _originalSrcExStyle = 0;
	LONG_PTR _originalOwnerExStyle = 0;
	winrt::com_ptr<ITaskbarList> _taskbarList;

	D3D11_BOX _frameBox{};

	bool _isScreenCapture = false;

	winrt::Windows::Graphics::Capture::GraphicsCaptureItem _captureItem{ nullptr };
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool _captureFramePool{ nullptr };
	winrt::Windows::Graphics::Capture::GraphicsCaptureSession _captureSession{ nullptr };
	winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice _wrappedD3DDevice{ nullptr };
};

}
