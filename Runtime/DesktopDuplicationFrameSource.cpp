#include "pch.h"
#include "DesktopDuplicationFrameSource.h"
#include "App.h"


static ComPtr<IDXGIOutput1> FindMonitor(ComPtr<IDXGIAdapter1> adapter, HMONITOR hMonitor) {
	ComPtr<IDXGIOutput> output;

	for (UINT adapterIndex = 0;
			SUCCEEDED(adapter->EnumOutputs(adapterIndex,
				output.ReleaseAndGetAddressOf()));
			adapterIndex++
	) {
		DXGI_OUTPUT_DESC desc;
		HRESULT hr = output->GetDesc(&desc);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("GetDesc 失败", hr));
			continue;
		}

		if (desc.Monitor == hMonitor) {
			ComPtr<IDXGIOutput1> output1;
			hr = output.As<IDXGIOutput1>(&output1);
			if (FAILED(hr)) {
				SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从 IDXGIOutput 获取 IDXGIOutput1 失败", hr));
				return nullptr;
			}

			return output1;
		}
	}

	return nullptr;
}

// 根据显示器句柄查找 IDXGIOutput1
static ComPtr<IDXGIOutput1> GetDXGIOutput(HMONITOR hMonitor) {
	const Renderer& renderer = App::GetInstance().GetRenderer();
	ComPtr<IDXGIAdapter1> curAdapter = App::GetInstance().GetRenderer().GetGraphicsAdapter();

	// 首先在当前使用的图形适配器上查询显示器
	ComPtr<IDXGIOutput1> output = FindMonitor(curAdapter, hMonitor);
	if (output) {
		return output;
	}

	// 未找到则在所有图形适配器上查找
	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<IDXGIFactory2> dxgiFactory = renderer.GetDXGIFactory();
	for (UINT adapterIndex = 0;
			SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex,
				adapter.ReleaseAndGetAddressOf()));
			adapterIndex++
	) {
		if (adapter == curAdapter) {
			continue;
		}

		output = FindMonitor(adapter, hMonitor);
		if (output) {
			return output;
		}
	}

	return nullptr;
}

bool DesktopDuplicationFrameSource::Initialize() {
	// WDA_EXCLUDEFROMCAPTURE 只在 Win10 v2004 及更新版本中可用
	const RTL_OSVERSIONINFOW& version = Utils::GetOSVersion();
	if (Utils::CompareVersion(version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber, 10, 0, 19041) < 0) {
		SPDLOG_LOGGER_ERROR(logger, "当前操作系统无法使用 Desktop Duplication");
		return false;
	}

	HWND hwndSrc = App::GetInstance().GetHwndSrc();
	HMONITOR hMonitor = MonitorFromWindow(hwndSrc, MONITOR_DEFAULTTONEAREST);
	if (!hMonitor) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("MonitorFromWindow 失败"));
		return false;
	}

	MONITORINFO mi{};
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfo(hMonitor, &mi)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetMonitorInfo 失败"));
		return false;
	}

	if (!_CenterWindowIfNecessary(hwndSrc, mi.rcWork)) {
		SPDLOG_LOGGER_ERROR(logger, "居中源窗口失败");
		return false;
	}

	ComPtr<IDXGIOutput1> output = GetDXGIOutput(hMonitor);
	if (!output) {
		SPDLOG_LOGGER_ERROR(logger, "无法找到 IDXGIOutput");
		return false;
	}

	HRESULT hr = output->DuplicateOutput(
		App::GetInstance().GetRenderer().GetD3DDevice().Get(),
		_outputDup.ReleaseAndGetAddressOf()
	);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("DuplicateOutput 失败", hr));
		return false;
	}

	RECT srcClientRect;
	if (!Utils::GetClientScreenRect(App::GetInstance().GetHwndSrcClient(), srcClientRect)) {
		SPDLOG_LOGGER_ERROR(logger, "GetClientScreenRect 失败");
		return false;
	}

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

	// 使全屏窗口无法被捕获到
	if (!SetWindowDisplayAffinity(App::GetInstance().GetHwndHost(), WDA_EXCLUDEFROMCAPTURE)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("SetWindowDisplayAffinity 失败"));
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

	SPDLOG_LOGGER_INFO(logger, "DesktopDuplicationFrameSource 初始化完成");
	return true;
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
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("AcquireNextFrame 失败", hr));
		return UpdateState::Error;
	}

	bool noUpdate = true;

	// 检索 move rects 和 dirty rects
	// 这些区域如果和窗口客户区有重叠则表明画面有变化
	if (info.TotalMetadataBufferSize) {
		if (info.TotalMetadataBufferSize > _dupMetaData.size()) {
			_dupMetaData.resize(info.TotalMetadataBufferSize);
		}

		UINT bufSize = info.TotalMetadataBufferSize;

		// move rects
		hr = _outputDup->GetFrameMoveRects(bufSize, (DXGI_OUTDUPL_MOVE_RECT*)_dupMetaData.data(), &bufSize);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("GetFrameMoveRects 失败", hr));
			return UpdateState::Error;
		}

		UINT nRect = bufSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);
		for (UINT i = 0; i < nRect; ++i) {
			const DXGI_OUTDUPL_MOVE_RECT& rect = ((DXGI_OUTDUPL_MOVE_RECT*)_dupMetaData.data())[i];
			if (Utils::CheckOverlap(_srcClientInMonitor, rect.DestinationRect)) {
				noUpdate = false;
				break;
			}
		}

		if (noUpdate) {
			bufSize = info.TotalMetadataBufferSize;

			// dirty rects
			hr = _outputDup->GetFrameDirtyRects(bufSize, (RECT*)_dupMetaData.data(), &bufSize);
			if (FAILED(hr)) {
				SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("GetFrameDirtyRects 失败", hr));
				return UpdateState::Error;
			}

			nRect = bufSize / sizeof(RECT);
			for (UINT i = 0; i < nRect; ++i) {
				const RECT& rect = ((RECT*)_dupMetaData.data())[i];
				if (Utils::CheckOverlap(_srcClientInMonitor, rect)) {
					noUpdate = false;
					break;
				}
			}
		}
	}

	if (noUpdate) {
		return _firstFrame ? UpdateState::Waiting : UpdateState::NoUpdate;
	}

	ComPtr<ID3D11Resource> d3dRes;
	hr = _dxgiRes.As<ID3D11Resource>(&d3dRes);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从 IDXGIResource 检索 ID3D11Resource 失败", hr));
		return UpdateState::NoUpdate;
	}

	App::GetInstance().GetRenderer().GetD3DDC()->CopySubresourceRegion(
		_output.Get(), 0, 0, 0, 0, d3dRes.Get(), 0, &_frameInMonitor);

	_firstFrame = false;

	return UpdateState::NewFrame;
}
