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

DesktopDuplicationFrameSource::~DesktopDuplicationFrameSource() {
	_exiting = true;
	WaitForSingleObject(_hDDPThread, 1000);
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
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	HRESULT hr = App::GetInstance().GetRenderer().GetD3DDevice()->CreateTexture2D(&desc, nullptr, &_output);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
		return false;
	}

	// 创建共享纹理
	desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
	hr = App::GetInstance().GetRenderer().GetD3DDevice()->CreateTexture2D(&desc, nullptr, &_sharedTex);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
		return false;
	}

	hr = _sharedTex.As<IDXGIKeyedMutex>(&_sharedTexMutex);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("检索 IDXGIKeyedMutex 失败", hr));
		return false;
	}

	ComPtr<IDXGIResource> sharedDxgiRes;
	hr = _sharedTex.As<IDXGIResource>(&sharedDxgiRes);
	if (FAILED(hr)) {
		return false;
	}

	HANDLE hSharedTex = NULL;
	hr = sharedDxgiRes->GetSharedHandle(&hSharedTex);
	if (FAILED(hr)) {
		return false;
	}

	if (!_InitializeDdpD3D(hSharedTex)) {
		SPDLOG_LOGGER_ERROR(logger, "初始化 D3D 失败");
		return false;
	}

	ComPtr<IDXGIOutput1> output = GetDXGIOutput(hMonitor);
	if (!output) {
		SPDLOG_LOGGER_ERROR(logger, "无法找到 IDXGIOutput");
		return false;
	}

	hr = output->DuplicateOutput(
		_ddpD3dDevice.Get(),
		_outputDup.ReleaseAndGetAddressOf()
	);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("DuplicateOutput 失败", hr));
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

	// 使全屏窗口无法被捕获到
	if (!SetWindowDisplayAffinity(App::GetInstance().GetHwndHost(), WDA_EXCLUDEFROMCAPTURE)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("SetWindowDisplayAffinity 失败"));
		return false;
	}
	

	_hDDPThread = CreateThread(nullptr, 0, _DDPThreadProc, this, 0, nullptr);
	if (!_hDDPThread) {
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, "DesktopDuplicationFrameSource 初始化完成");
	return true;
}


FrameSourceBase::UpdateState DesktopDuplicationFrameSource::Update() {
	UINT newFrameState = _newFrameState.load();
	if (newFrameState == 2) {
		// 第一帧之前不渲染
		return UpdateState::Waiting;
	} else if (newFrameState == 0) {
		return UpdateState::NoUpdate;
	}

	// 不必等待，当 newFrameState 变化时 DDP 线程已将锁释放
	HRESULT hr = _sharedTexMutex->AcquireSync(1, 0);
	if (hr == static_cast<HRESULT>(WAIT_TIMEOUT)) {
		return UpdateState::Waiting;
	}
	
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("AcquireSync 失败", hr));
		return UpdateState::Error;
	}

	_newFrameState.store(0);

	const auto& dc = App::GetInstance().GetRenderer().GetD3DDC();
	dc->CopyResource(_output.Get(), _sharedTex.Get());

	_sharedTexMutex->ReleaseSync(0);

	return UpdateState::NewFrame;
}

bool DesktopDuplicationFrameSource::_InitializeDdpD3D(HANDLE hSharedTex) {
	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	if (Renderer::IsDebugLayersAvailable()) {
		// 在 DEBUG 配置启用调试层
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		// 不支持功能级别 9.x，但这里加上没坏处
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};
	UINT nFeatureLevels = ARRAYSIZE(featureLevels);

	// 使用和 Renderer 相同的图像适配器以避免 GPU 间的纹理拷贝
	HRESULT hr = D3D11CreateDevice(
		App::GetInstance().GetRenderer().GetGraphicsAdapter().Get(),
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		createDeviceFlags,
		featureLevels,
		nFeatureLevels,
		D3D11_SDK_VERSION,
		&_ddpD3dDevice,
		nullptr,
		&_ddpD3dDC
	);

	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("D3D11CreateDevice 失败", hr));
		return false;
	}

	// 获取共享纹理
	hr = _ddpD3dDevice->OpenSharedResource(hSharedTex, IID_PPV_ARGS(&_ddpSharedTex));
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("OpenSharedResource 失败", hr));
		return false;
	}

	hr = _ddpSharedTex.As<IDXGIKeyedMutex>(&_ddpSharedTexMutex);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("检索 IDXGIKeyedMutex 失败", hr));
		return false;
	}
	
	return true;
}

DWORD WINAPI DesktopDuplicationFrameSource::_DDPThreadProc(LPVOID lpThreadParameter) {
	DesktopDuplicationFrameSource& that = *(DesktopDuplicationFrameSource*)lpThreadParameter;

	DXGI_OUTDUPL_FRAME_INFO info{};
	ComPtr<IDXGIResource> dxgiRes;
	std::vector<BYTE> dupMetaData;

	while (!that._exiting.load()) {
		if (dxgiRes) {
			that._outputDup->ReleaseFrame();
		}
		HRESULT hr = that._outputDup->AcquireNextFrame(500, &info, &dxgiRes);
		if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
			continue;
		}

		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("AcquireNextFrame 失败", hr));
			continue;
		}

		bool noUpdate = true;

		// 检索 move rects 和 dirty rects
		// 这些区域如果和窗口客户区有重叠则表明画面有变化
		if (info.TotalMetadataBufferSize) {
			if (info.TotalMetadataBufferSize > dupMetaData.size()) {
				dupMetaData.resize(info.TotalMetadataBufferSize);
			}

			UINT bufSize = info.TotalMetadataBufferSize;

			// move rects
			hr = that._outputDup->GetFrameMoveRects(bufSize, (DXGI_OUTDUPL_MOVE_RECT*)dupMetaData.data(), &bufSize);
			if (FAILED(hr)) {
				SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("GetFrameMoveRects 失败", hr));
				continue;
			}

			UINT nRect = bufSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);
			for (UINT i = 0; i < nRect; ++i) {
				const DXGI_OUTDUPL_MOVE_RECT& rect = ((DXGI_OUTDUPL_MOVE_RECT*)dupMetaData.data())[i];
				if (Utils::CheckOverlap(that._srcClientInMonitor, rect.DestinationRect)) {
					noUpdate = false;
					break;
				}
			}

			if (noUpdate) {
				bufSize = info.TotalMetadataBufferSize;

				// dirty rects
				hr = that._outputDup->GetFrameDirtyRects(bufSize, (RECT*)dupMetaData.data(), &bufSize);
				if (FAILED(hr)) {
					SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("GetFrameDirtyRects 失败", hr));
					continue;
				}

				nRect = bufSize / sizeof(RECT);
				for (UINT i = 0; i < nRect; ++i) {
					const RECT& rect = ((RECT*)dupMetaData.data())[i];
					if (Utils::CheckOverlap(that._srcClientInMonitor, rect)) {
						noUpdate = false;
						break;
					}
				}
			}
		}

		if (noUpdate) {
			continue;
		}

		ComPtr<ID3D11Resource> d3dRes;
		hr = dxgiRes.As<ID3D11Resource>(&d3dRes);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从 IDXGIResource 检索 ID3D11Resource 失败", hr));
			continue;
		}

		hr = that._ddpSharedTexMutex->AcquireSync(0, 100);
		while (hr == static_cast<HRESULT>(WAIT_TIMEOUT)) {
			if (that._exiting.load()) {
				return 0;
			}

			hr = that._ddpSharedTexMutex->AcquireSync(0, 100);
		}

		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("AcquireSync 失败", hr));
			continue;
		}


		that._ddpD3dDC->CopySubresourceRegion(that._ddpSharedTex.Get(), 0, 0, 0, 0, d3dRes.Get(), 0, &that._frameInMonitor);
		that._ddpSharedTexMutex->ReleaseSync(1);
		that._newFrameState.store(1);
	}

	return 0;
}
