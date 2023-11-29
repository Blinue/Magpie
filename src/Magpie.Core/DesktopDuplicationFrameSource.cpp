#include "pch.h"
#include "DesktopDuplicationFrameSource.h"
#include "MagApp.h"
#include "DeviceResources.h"
#include "Logger.h"
#include "Win32Utils.h"
#include "SmallVector.h"


namespace Magpie::Core {

static winrt::com_ptr<IDXGIOutput1> FindMonitor(IDXGIAdapter1* adapter, HMONITOR hMonitor) {
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

// 根据显示器句柄查找 IDXGIOutput1
static winrt::com_ptr<IDXGIOutput1> GetDXGIOutput(HMONITOR hMonitor) {
	auto& dr = MagApp::Get().GetDeviceResources();
	IDXGIAdapter1* curAdapter = dr.GetGraphicsAdapter();

	// 首先在当前使用的图形适配器上查询显示器
	winrt::com_ptr<IDXGIOutput1> output = FindMonitor(curAdapter, hMonitor);
	if (output) {
		return output;
	}

	// 未找到则在所有图形适配器上查找
	winrt::com_ptr<IDXGIAdapter1> adapter;
	IDXGIFactory5* dxgiFactory = dr.GetDXGIFactory();
	for (UINT adapterIndex = 0;
		SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, adapter.put()));
		++adapterIndex
		) {
		if (adapter.get() == curAdapter) {
			continue;
		}

		output = FindMonitor(adapter.get(), hMonitor);
		if (output) {
			return output;
		}
	}

	return nullptr;
}

DesktopDuplicationFrameSource::~DesktopDuplicationFrameSource() {
	_exiting.store(true, std::memory_order_release);
	WaitForSingleObject(_hDDPThread, 1000);
}

bool DesktopDuplicationFrameSource::Initialize() {
	if (!FrameSourceBase::Initialize()) {
		Logger::Get().Error("初始化 FrameSourceBase 失败");
		return false;
	}

	// WDA_EXCLUDEFROMCAPTURE 只在 Win10 20H1 及更新版本中可用
	if (!Win32Utils::GetOSVersion().Is20H1OrNewer()) {
		Logger::Get().Error("当前操作系统无法使用 Desktop Duplication");
		return false;
	}

	HWND hwndSrc = MagApp::Get().GetHwndSrc();

	HMONITOR hMonitor = MonitorFromWindow(hwndSrc, MONITOR_DEFAULTTONEAREST);
	if (!hMonitor) {
		Logger::Get().Win32Error("MonitorFromWindow 失败");
		return false;
	}

	MONITORINFO mi{};
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfo(hMonitor, &mi)) {
		Logger::Get().Win32Error("GetMonitorInfo 失败");
		return false;
	}

	if (!_CenterWindowIfNecessary(hwndSrc, mi.rcWork)) {
		Logger::Get().Error("居中源窗口失败");
		return false;
	}

	if (!_UpdateSrcFrameRect()) {
		Logger::Get().Error("_UpdateSrcFrameRect 失败");
		return false;
	}

	auto& dr = MagApp::Get().GetDeviceResources();

	_output = dr.CreateTexture2D(
		DXGI_FORMAT_B8G8R8A8_UNORM,
		_srcFrameRect.right - _srcFrameRect.left,
		_srcFrameRect.bottom - _srcFrameRect.top,
		D3D11_BIND_SHADER_RESOURCE
	);
	if (!_output) {
		Logger::Get().Error("创建 Texture2D 失败");
		return false;
	}

	// 创建共享纹理
	_sharedTex = dr.CreateTexture2D(
		DXGI_FORMAT_B8G8R8A8_UNORM,
		_srcFrameRect.right - _srcFrameRect.left,
		_srcFrameRect.bottom - _srcFrameRect.top,
		D3D11_BIND_SHADER_RESOURCE,
		D3D11_USAGE_DEFAULT,
		D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX
	);
	if (!_sharedTex) {
		Logger::Get().Error("创建 Texture2D 失败");
		return false;
	}

	_sharedTexMutex = _sharedTex.try_as<IDXGIKeyedMutex>();
	if (!_sharedTexMutex) {
		Logger::Get().Error("检索 IDXGIKeyedMutex 失败");
		return false;
	}

	winrt::com_ptr<IDXGIResource> sharedDxgiRes = _sharedTex.try_as<IDXGIResource>();
	if (!sharedDxgiRes) {
		Logger::Get().Error("检索 IDXGIResource 失败");
		return false;
	}

	HANDLE hSharedTex = NULL;
	HRESULT hr = sharedDxgiRes->GetSharedHandle(&hSharedTex);
	if (FAILED(hr)) {
		Logger::Get().Error("GetSharedHandle 失败");
		return false;
	}

	if (!_InitializeDdpD3D(hSharedTex)) {
		Logger::Get().Error("初始化 D3D 失败");
		return false;
	}

	winrt::com_ptr<IDXGIOutput1> output = GetDXGIOutput(hMonitor);
	if (!output) {
		Logger::Get().Error("无法找到 IDXGIOutput");
		return false;
	}

	hr = output->DuplicateOutput(_ddpD3dDevice.get(), _outputDup.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("DuplicateOutput 失败", hr);
		return false;
	}

	// 计算源窗口客户区在该屏幕上的位置，用于计算新帧是否有更新
	_srcClientInMonitor = {
		_srcFrameRect.left - mi.rcMonitor.left,
		_srcFrameRect.top - mi.rcMonitor.top,
		_srcFrameRect.right - mi.rcMonitor.left,
		_srcFrameRect.bottom - mi.rcMonitor.top
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
	if (!SetWindowDisplayAffinity(MagApp::Get().GetHwndHost(), WDA_EXCLUDEFROMCAPTURE)) {
		Logger::Get().Win32Error("SetWindowDisplayAffinity 失败");
		return false;
	}


	_hDDPThread = CreateThread(nullptr, 0, _DDPThreadProc, this, 0, nullptr);
	if (!_hDDPThread) {
		return false;
	}

	Logger::Get().Info("DesktopDuplicationFrameSource 初始化完成");
	return true;
}


FrameSourceBase::UpdateState DesktopDuplicationFrameSource::Update() {
	const UINT newFrameState = _newFrameState.load(std::memory_order_acquire);
	if (newFrameState == 2) {
		// 第一帧之前不渲染
		return UpdateState::Waiting;
	} else if (newFrameState == 0) {
		return UpdateState::NoUpdate;
	}

	// 不必等待，当 newFrameState 变化时捕获线程已将锁释放
	HRESULT hr = _sharedTexMutex->AcquireSync(1, 0);
	if (hr == static_cast<HRESULT>(WAIT_TIMEOUT)) {
		return UpdateState::Waiting;
	}

	if (FAILED(hr)) {
		Logger::Get().ComError("AcquireSync 失败", hr);
		return UpdateState::Error;
	}

	// 不需要对捕获线程可见
	_newFrameState.store(0, std::memory_order_relaxed);

	MagApp::Get().GetDeviceResources().GetD3DDC()->CopyResource(_output.get(), _sharedTex.get());

	_sharedTexMutex->ReleaseSync(0);

	return UpdateState::NewFrame;
}

bool DesktopDuplicationFrameSource::_InitializeDdpD3D(HANDLE hSharedTex) {
	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	if (DeviceResources::IsDebugLayersAvailable()) {
		// 在 DEBUG 配置启用调试层
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};
	UINT nFeatureLevels = ARRAYSIZE(featureLevels);

	// 使用和 Renderer 相同的图像适配器以避免 GPU 间的纹理拷贝
	HRESULT hr = D3D11CreateDevice(
		MagApp::Get().GetDeviceResources().GetGraphicsAdapter(),
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		createDeviceFlags,
		featureLevels,
		nFeatureLevels,
		D3D11_SDK_VERSION,
		_ddpD3dDevice.put(),
		nullptr,
		_ddpD3dDC.put()
	);

	if (FAILED(hr)) {
		Logger::Get().ComError("D3D11CreateDevice 失败", hr);
		return false;
	}

	// 获取共享纹理
	hr = _ddpD3dDevice->OpenSharedResource(hSharedTex, IID_PPV_ARGS(_ddpSharedTex.put()));
	if (FAILED(hr)) {
		Logger::Get().ComError("OpenSharedResource 失败", hr);
		return false;
	}

	_ddpSharedTexMutex = _ddpSharedTex.try_as<IDXGIKeyedMutex>();
	if (!_ddpSharedTexMutex) {
		Logger::Get().Error("检索 IDXGIKeyedMutex 失败");
		return false;
	}

	return true;
}

DWORD WINAPI DesktopDuplicationFrameSource::_DDPThreadProc(LPVOID lpThreadParameter) {
	DesktopDuplicationFrameSource& that = *(DesktopDuplicationFrameSource*)lpThreadParameter;

	DXGI_OUTDUPL_FRAME_INFO info{};
	winrt::com_ptr<IDXGIResource> dxgiRes;
	SmallVector<uint8_t, 0> dupMetaData;

	while (!that._exiting.load(std::memory_order_acquire)) {
		if (dxgiRes) {
			that._outputDup->ReleaseFrame();
		}
		HRESULT hr = that._outputDup->AcquireNextFrame(500, &info, dxgiRes.put());
		if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
			continue;
		}

		if (FAILED(hr)) {
			Logger::Get().ComError("AcquireNextFrame 失败", hr);
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
				Logger::Get().ComError("GetFrameMoveRects 失败", hr);
				continue;
			}

			UINT nRect = bufSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);
			for (UINT i = 0; i < nRect; ++i) {
				const DXGI_OUTDUPL_MOVE_RECT& rect = ((DXGI_OUTDUPL_MOVE_RECT*)dupMetaData.data())[i];
				if (Win32Utils::CheckOverlap(that._srcClientInMonitor, rect.DestinationRect)) {
					noUpdate = false;
					break;
				}
			}

			if (noUpdate) {
				bufSize = info.TotalMetadataBufferSize;

				// dirty rects
				hr = that._outputDup->GetFrameDirtyRects(bufSize, (RECT*)dupMetaData.data(), &bufSize);
				if (FAILED(hr)) {
					Logger::Get().ComError("GetFrameDirtyRects 失败", hr);
					continue;
				}

				nRect = bufSize / sizeof(RECT);
				for (UINT i = 0; i < nRect; ++i) {
					const RECT& rect = ((RECT*)dupMetaData.data())[i];
					if (Win32Utils::CheckOverlap(that._srcClientInMonitor, rect)) {
						noUpdate = false;
						break;
					}
				}
			}
		}

		if (noUpdate) {
			continue;
		}

		winrt::com_ptr<ID3D11Resource> d3dRes = dxgiRes.try_as<ID3D11Resource>();
		if (!d3dRes) {
			Logger::Get().Error("从 IDXGIResource 检索 ID3D11Resource 失败");
			continue;
		}

		hr = that._ddpSharedTexMutex->AcquireSync(0, 100);
		while (hr == static_cast<HRESULT>(WAIT_TIMEOUT)) {
			if (that._exiting.load(std::memory_order_acquire)) {
				return 0;
			}

			hr = that._ddpSharedTexMutex->AcquireSync(0, 100);
		}

		if (FAILED(hr)) {
			Logger::Get().ComError("AcquireSync 失败", hr);
			continue;
		}


		that._ddpD3dDC->CopySubresourceRegion(that._ddpSharedTex.get(), 0, 0, 0, 0, d3dRes.get(), 0, &that._frameInMonitor);
		that._ddpSharedTexMutex->ReleaseSync(1);
		that._newFrameState.store(1, std::memory_order_release);
	}

	return 0;
}

}
