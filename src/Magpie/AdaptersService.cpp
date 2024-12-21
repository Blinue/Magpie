#include "pch.h"
#include <d3d11_4.h>
#include "AdaptersService.h"
#include "Logger.h"
#include "Win32Helper.h"
#include "App.h"

using namespace winrt::Magpie::implementation;
using namespace winrt;

namespace Magpie {

bool AdaptersService::Initialize() noexcept {
	_monitorThread = std::thread(std::bind_front(&AdaptersService::_MonitorThreadProc, this));
    return true;
}

void AdaptersService::Uninitialize() noexcept {
	if (!_monitorThread.joinable()) {
		return;
	}

	const HANDLE hToastThread = _monitorThread.native_handle();
	if (!wil::handle_wait(hToastThread, 0)) {
		const DWORD threadId = GetThreadId(hToastThread);

		// 持续尝试直到 _monitorThread 创建了消息队列
		while (!PostThreadMessage(threadId, WM_QUIT, 0, 0)) {
			if (wil::handle_wait(hToastThread, 1)) {
				break;
			}
		}
	}

	_monitorThread.join();
}

void AdaptersService::_GatherAdapterInfos(
	com_ptr<IDXGIFactory7>& dxgiFactory,
	wil::unique_event_nothrow& adaptersChangedEvent,
	DWORD& adaptersChangedCookie
) noexcept {
	// 显卡变化后需要重新创建 DXGI 工厂
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateDXGIFactory1 失败", hr);
		return;
	}

	hr = dxgiFactory->RegisterAdaptersChangedEvent(
		adaptersChangedEvent.get(), &adaptersChangedCookie);
	if (FAILED(hr)) {
		Logger::Get().ComError("RegisterAdaptersChangedEvent 失败", hr);
		return;
	}

	std::vector<AdapterInfo> adapterInfos;
	SmallVector<com_ptr<IDXGIAdapter1>> adapters;

	com_ptr<IDXGIAdapter1> curAdapter;
	for (UINT adapterIdx = 0;
		SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIdx, curAdapter.put()));
		++adapterIdx
	) {
		DXGI_ADAPTER_DESC1 desc;
		hr = curAdapter->GetDesc1(&desc);
		if (FAILED(hr)) {
			continue;
		}

		// 不包含 WARP
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}

		adapterInfos.push_back({
			.idx = adapterIdx,
			.vendorId = desc.VendorId,
			.deviceId = desc.DeviceId,
			.description = desc.Description
		});

		adapters.push_back(std::move(curAdapter));
	}

	App::Get().Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [this, adapterInfos(std::move(adapterInfos))]() {
		_adapterInfos = std::move(adapterInfos);
		AdaptersChanged.Invoke();
	});

	SmallVector<uint32_t> noFL11Adapters;
	{
		wil::srwlock writeLock;
		Win32Helper::RunParallel([&](uint32_t i) {
			D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
			if (FAILED(D3D11CreateDevice(adapters[i].get(), D3D_DRIVER_TYPE_UNKNOWN,
				NULL, 0, &fl, 1, D3D11_SDK_VERSION, nullptr, nullptr, nullptr))) {
				auto lock = writeLock.lock_exclusive();
				noFL11Adapters.push_back(i);
			}
		}, (uint32_t)adapters.size());
	}

	App::Get().Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [this, noFL11Adapters(std::move(noFL11Adapters))]() {
		for (uint32_t i : noFL11Adapters) {
			_adapterInfos[i].isFL11Supported = false;
		}

		if (!noFL11Adapters.empty()) {
			AdaptersChanged.Invoke();
		}
	});
}

void AdaptersService::_MonitorThreadProc() noexcept {
#ifdef _DEBUG
	SetThreadDescription(GetCurrentThread(), L"AdaptersService 线程");
#endif

	winrt::init_apartment(winrt::apartment_type::single_threaded);

	wil::unique_event_nothrow adaptersChangedEvent;
	HRESULT hr = adaptersChangedEvent.create();
	if (FAILED(hr)) {
		Logger::Get().ComError("创建 event 失败", hr);
		return;
	}

	com_ptr<IDXGIFactory7> dxgiFactory;
	DWORD adaptersChangedCookie = 0;
	_GatherAdapterInfos(dxgiFactory, adaptersChangedEvent, adaptersChangedCookie);

	while (true) {
		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT) {
			break;
		}

		HANDLE hAdaptersChangedEvent = adaptersChangedEvent.get();
		if (MsgWaitForMultipleObjectsEx(1, &hAdaptersChangedEvent,
			INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE) == WAIT_OBJECT_0) {
			// WAIT_OBJECT_0 表示显卡变化
			// WAIT_OBJECT_0 + 1 表示有新消息
			_GatherAdapterInfos(dxgiFactory, adaptersChangedEvent, adaptersChangedCookie);
		}
	}

	dxgiFactory->UnregisterAdaptersChangedEvent(adaptersChangedCookie);
}

}
