#include "pch.h"
#include "DesktopDuplicationFrameSource.h"
#include "Logger.h"
#include "Win32Helper.h"
#include "ScalingWindow.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include "SmallVector.h"

namespace Magpie {

static winrt::com_ptr<IDXGIOutput1> FindMonitor(IDXGIAdapter1* adapter, HMONITOR hMonitor) noexcept {
	winrt::com_ptr<IDXGIOutput> output;

	for (UINT adapterIndex = 0;
		SUCCEEDED(adapter->EnumOutputs(adapterIndex, output.put()));
		++adapterIndex
	) {
		DXGI_OUTPUT_DESC desc;
		HRESULT hr = output->GetDesc(&desc);
		if (FAILED(hr)) {
			Logger::Get().ComError("GetDesc 失败", hr);
			continue;
		}

		if (desc.Monitor == hMonitor) {
			winrt::com_ptr<IDXGIOutput1> output1 = output.try_as<IDXGIOutput1>();
			if (!output1) {
				Logger::Get().Error("从 IDXGIOutput 获取 IDXGIOutput1 失败");
				return nullptr;
			}

			return output1;
		}
	}

	return nullptr;
}

bool DesktopDuplicationFrameSource::_Initialize() noexcept {
	// WDA_EXCLUDEFROMCAPTURE 只在 Win10 20H1 及更新版本中可用
	if (!Win32Helper::GetOSVersion().Is20H1OrNewer()) {
		Logger::Get().Error("当前操作系统无法使用 Desktop Duplication");
		return false;
	}

	const HWND hwndSrc = ScalingWindow::Get().SrcInfo().Handle();
	const RECT& srcRect = ScalingWindow::Get().SrcInfo().FrameRect();

	HMONITOR hMonitor = MonitorFromWindow(hwndSrc, MONITOR_DEFAULTTONEAREST);
	if (!hMonitor) {
		Logger::Get().Win32Error("MonitorFromWindow 失败");
		return false;
	}

	{
		MONITORINFO mi{ .cbSize = sizeof(mi) };
		if (!GetMonitorInfo(hMonitor, &mi)) {
			Logger::Get().Win32Error("GetMonitorInfo 失败");
			return false;
		}

		// 最大化的窗口无需调整位置
		if (Win32Helper::GetWindowShowCmd(hwndSrc) != SW_SHOWMAXIMIZED) {
			if (!_CenterWindowIfNecessary(hwndSrc, mi.rcWork)) {
				Logger::Get().Error("居中源窗口失败");
				return false;
			}
		}

		// 计算源窗口客户区在该屏幕上的位置，用于计算新帧是否有更新
		_srcClientInMonitor = {
			srcRect.left - mi.rcMonitor.left,
			srcRect.top - mi.rcMonitor.top,
			srcRect.right - mi.rcMonitor.left,
			srcRect.bottom - mi.rcMonitor.top
		};
	}

	_frameInMonitor = {
		(UINT)_srcClientInMonitor.left,
		(UINT)_srcClientInMonitor.top,
		0,
		(UINT)_srcClientInMonitor.right,
		(UINT)_srcClientInMonitor.bottom,
		1
	};
	
	_output = DirectXHelper::CreateTexture2D(
		_deviceResources->GetD3DDevice(),
		DXGI_FORMAT_B8G8R8A8_UNORM,
		srcRect.right - srcRect.left,
		srcRect.bottom - srcRect.top,
		D3D11_BIND_SHADER_RESOURCE
	);
	if (!_output) {
		Logger::Get().Error("CreateTexture2D 失败");
		return false;
	}

	winrt::com_ptr<IDXGIOutput1> output = FindMonitor(
		_deviceResources->GetGraphicsAdapter(), hMonitor);
	if (!output) {
		Logger::Get().Error("无法找到 IDXGIOutput");
		return false;
	}

	HRESULT hr = output->DuplicateOutput(_deviceResources->GetD3DDevice(), _outputDup.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("DuplicateOutput 失败", hr);
		return false;
	}

	// 使全屏窗口无法被捕获到
	if (!SetWindowDisplayAffinity(ScalingWindow::Get().Handle(), WDA_EXCLUDEFROMCAPTURE)) {
		Logger::Get().Win32Error("SetWindowDisplayAffinity 失败");
		return false;
	}

	Logger::Get().Info("DesktopDuplicationFrameSource 初始化完成");
	return true;
}

FrameSourceState DesktopDuplicationFrameSource::_Update() noexcept {
	ID3D11DeviceContext4* d3dDC = _deviceResources->GetD3DDC();

	if (_isFrameAcquired) {
		// 根据文档，释放后立刻获取下一帧可以提高性能
		_outputDup->ReleaseFrame();
		_isFrameAcquired = false;
	}

	DXGI_OUTDUPL_FRAME_INFO info;
	winrt::com_ptr<IDXGIResource> dxgiRes;
	// 等待 1ms
	HRESULT hr = _outputDup->AcquireNextFrame(1, &info, dxgiRes.put());
	if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
		return FrameSourceState::Waiting;
	}

	if (FAILED(hr)) {
		Logger::Get().ComError("AcquireNextFrame 失败", hr);
		return FrameSourceState::Error;
	}

	_isFrameAcquired = true;

	bool noUpdate = true;

	// 检索 move rects 和 dirty rects
	// 这些区域如果和窗口客户区有重叠则表明画面有变化
	if (info.TotalMetadataBufferSize) {
		if (info.TotalMetadataBufferSize > _dupMetaData.size()) {
			_dupMetaData.resize(info.TotalMetadataBufferSize);
		}

		uint32_t bufSize = info.TotalMetadataBufferSize;

		// Move rects
		hr = _outputDup->GetFrameMoveRects(
			bufSize, (DXGI_OUTDUPL_MOVE_RECT*)_dupMetaData.data(), &bufSize);
		if (FAILED(hr)) {
			Logger::Get().ComError("GetFrameMoveRects 失败", hr);
			return FrameSourceState::Error;
		}

		uint32_t nRect = bufSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);
		for (uint32_t i = 0; i < nRect; ++i) {
			const DXGI_OUTDUPL_MOVE_RECT& rect = 
				((DXGI_OUTDUPL_MOVE_RECT*)_dupMetaData.data())[i];
			if (Win32Helper::CheckOverlap(_srcClientInMonitor, rect.DestinationRect)) {
				noUpdate = false;
				break;
			}
		}

		if (noUpdate) {
			bufSize = info.TotalMetadataBufferSize;

			// Dirty rects
			hr = _outputDup->GetFrameDirtyRects(
				bufSize, (RECT*)_dupMetaData.data(), &bufSize);
			if (FAILED(hr)) {
				Logger::Get().ComError("GetFrameDirtyRects 失败", hr);
				return FrameSourceState::Error;
			}

			nRect = bufSize / sizeof(RECT);
			for (uint32_t i = 0; i < nRect; ++i) {
				const RECT& rect = ((RECT*)_dupMetaData.data())[i];
				if (Win32Helper::CheckOverlap(_srcClientInMonitor, rect)) {
					noUpdate = false;
					break;
				}
			}
		}
	}

	if (noUpdate) {
		return FrameSourceState::Waiting;
	}
	
	winrt::com_ptr<ID3D11Texture2D> frameTexture = dxgiRes.try_as<ID3D11Texture2D>();
	if (!frameTexture) {
		Logger::Get().Error("从 IDXGIResource 检索 ID3D11Resource 失败");
		return FrameSourceState::Error;
	}

	d3dDC->CopySubresourceRegion(
		_output.get(), 0, 0, 0, 0, frameTexture.get(), 0, &_frameInMonitor);

	return FrameSourceState::NewFrame;
}

}
