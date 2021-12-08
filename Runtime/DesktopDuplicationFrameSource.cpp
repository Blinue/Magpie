#include "pch.h"
#include "DesktopDuplicationFrameSource.h"
#include "App.h"

ComPtr<IDXGIOutput1> GetDXGIOutput(HMONITOR hMonitor) {
	ComPtr<IDXGIAdapter1> curAdapter = App::GetInstance().GetRenderer().GetGraphicsAdapter();

	// 首先在当前使用的图形适配器上查询显示器
	ComPtr<IDXGIOutput> output;
	for (UINT adapterIndex = 0;
			SUCCEEDED(curAdapter->EnumOutputs(adapterIndex,
				output.ReleaseAndGetAddressOf()));
			adapterIndex++
	) {
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		if (desc.Monitor == hMonitor) {
			ComPtr<IDXGIOutput1> output1;
			output.As<IDXGIOutput1>(&output1);
			return output1;
		}
	}

	return nullptr;
}

bool DesktopDuplicationFrameSource::Initialize() {
	HMONITOR hMonitor = MonitorFromWindow(App::GetInstance().GetHwndSrc(), MONITOR_DEFAULTTONEAREST);

	ComPtr<IDXGIOutput1> output = GetDXGIOutput(hMonitor);
	if (!output) {
		return false;
	}

	HRESULT hr = output->DuplicateOutput(App::GetInstance().GetRenderer().GetD3DDevice().Get(), _outputDup.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		return false;
	}

	const RECT& srcClientRect = App::GetInstance().GetSrcClientRect();

	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width = srcClientRect.right - srcClientRect.left;
	desc.Height = srcClientRect.bottom - srcClientRect.top;
	desc.Usage = D3D11_USAGE_DEFAULT;
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

	SetWindowDisplayAffinity(App::GetInstance().GetHwndHost(), WDA_EXCLUDEFROMCAPTURE);

	MONITORINFO mi{};
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfo(hMonitor, &mi)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetMonitorInfo 出错"));
		return false;
	}

	// 计算源窗口客户区在该屏幕上的位置，用于计算新帧是否有更新
	_srcClientInMonitor = {
		srcClientRect.left - mi.rcMonitor.left,
		srcClientRect.top - mi.rcMonitor.top,
		srcClientRect.right - mi.rcMonitor.left,
		srcClientRect.bottom - mi.rcMonitor.top
	};

	_frameInMonitor = {
		(UINT)_srcClientInMonitor.left,
		(UINT)_srcClientInMonitor.top,
		0,
		(UINT)_srcClientInMonitor.right,
		(UINT)_srcClientInMonitor.bottom,
		1
	};

	return true;
}

inline bool IsRectsOverlapped(const RECT& r1, const RECT& r2) {
	return r1.right > r2.left && r1.bottom > r2.top && r1.left < r2.right && r1.top < r2.bottom;
}

FrameSourceBase::UpdateState DesktopDuplicationFrameSource::Update() {
	DXGI_OUTDUPL_FRAME_INFO info;
	
	if (_dxgiRes) {
		_outputDup->ReleaseFrame();
		_dxgiRes = nullptr;
	}
	HRESULT hr = _outputDup->AcquireNextFrame(0, &info, &_dxgiRes);

	if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
		return _firstFrame ? UpdateState::Waiting : UpdateState::NoUpdate;
	}

	if (FAILED(hr)) {
		return UpdateState::Error;
	}

	// 检索 move rects 和 dirty rects
	// 这些区域如果和窗口客户区有重叠则表明画面有变化
	if (!_firstFrame) {
		bool noUpdate = true;

		if (info.TotalMetadataBufferSize) {
			if (info.TotalMetadataBufferSize > _dupMetaData.size()) {
				_dupMetaData.resize(info.TotalMetadataBufferSize);
			}

			UINT bufSize = info.TotalMetadataBufferSize;

			// move rects
			hr = _outputDup->GetFrameMoveRects(bufSize, (DXGI_OUTDUPL_MOVE_RECT*)_dupMetaData.data(), &bufSize);
			if (FAILED(hr)) {
				return UpdateState::Error;
			}

			UINT nRect = bufSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);
			for (UINT i = 0; i < nRect; ++i) {
				const DXGI_OUTDUPL_MOVE_RECT& rect = ((DXGI_OUTDUPL_MOVE_RECT*)_dupMetaData.data())[i];
				if (IsRectsOverlapped(_srcClientInMonitor, rect.DestinationRect)) {
					noUpdate = false;
					break;
				}
			}

			if (noUpdate) {
				bufSize = info.TotalMetadataBufferSize;

				// dirty rects
				hr = _outputDup->GetFrameDirtyRects(bufSize, (RECT*)_dupMetaData.data(), &bufSize);
				if (FAILED(hr)) {
					return UpdateState::Error;
				}

				nRect = bufSize / sizeof(RECT);
				for (UINT i = 0; i < nRect; ++i) {
					const RECT& rect = ((RECT*)_dupMetaData.data())[i];
					if (IsRectsOverlapped(_srcClientInMonitor, rect)) {
						noUpdate = false;
						break;
					}
				}
			}
		}
		
		if (noUpdate) {
			return UpdateState::NoUpdate;
		}
	}

	ComPtr<ID3D11Resource> d3dRes;
	_dxgiRes.As<ID3D11Resource>(&d3dRes);

	App::GetInstance().GetRenderer().GetD3DDC()->CopySubresourceRegion(
		_output.Get(), 0, 0, 0, 0, d3dRes.Get(), 0, &_frameInMonitor);

	_outputDup->ReleaseFrame();

	_firstFrame = false;
	
	return UpdateState::NewFrame;
}
