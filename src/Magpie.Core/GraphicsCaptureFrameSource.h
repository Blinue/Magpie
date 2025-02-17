#pragma once
#include "FrameSourceBase.h"
#include <winrt/Windows.Graphics.Capture.h>
#include <Windows.Graphics.Capture.Interop.h>
#include <ShlObj.h>

namespace Magpie {

// 使用 Window Runtime 的 Windows.Graphics.Capture API 抓取窗口
// 见 https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/screen-capture
class GraphicsCaptureFrameSource final : public FrameSourceBase {
public:
	virtual ~GraphicsCaptureFrameSource();

	bool IsScreenCapture() const noexcept override {
		return _isScreenCapture;
	}

	FrameSourceWaitType WaitType() const noexcept override {
		return FrameSourceWaitType::WaitForMessage;
	}

	const char* Name() const noexcept override {
		return "Graphics Capture";
	}

	void OnCursorVisibilityChanged(bool isVisible, bool onDestory) noexcept override;

protected:
	bool _HasRoundCornerInWin11() noexcept override {
		return true;
	}

	bool _Initialize() noexcept override;

	FrameSourceState _Update() noexcept override;

private:
	bool _StartCapture() noexcept;

	void _StopCapture() noexcept;

	bool _CaptureWindow(IGraphicsCaptureItemInterop* interop) noexcept;

	bool _CaptureMonitor(IGraphicsCaptureItemInterop* interop) noexcept;

	bool _TryCreateGraphicsCaptureItem(IGraphicsCaptureItemInterop* interop) noexcept;

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
