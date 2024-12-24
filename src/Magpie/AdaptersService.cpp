#include "pch.h"
#include <d3d11_4.h>
#include "AdaptersService.h"
#include "Logger.h"
#include "Win32Helper.h"
#include "App.h"
#include "DirectXHelper.h"

using namespace winrt::Magpie::implementation;
using namespace winrt;

namespace Magpie {

bool AdaptersService::Initialize() noexcept {
	com_ptr<IDXGIFactory7> dxgiFactory;

	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateDXGIFactory1 失败", hr);
		return false;
	}

	com_ptr<IDXGIAdapter1> curAdapter;
	for (UINT adapterIdx = 0;
		SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIdx, curAdapter.put()));
		++adapterIdx
	) {
		DXGI_ADAPTER_DESC1 desc;
		hr = curAdapter->GetDesc1(&desc);
		if (FAILED(hr) || DirectXHelper::IsWARP(desc)) {
			continue;
		}

		// 初始化时不检查是否支持 FL11，有些设备上 D3D11CreateDevice 相当慢
		_adapterInfos.push_back({
			.idx = adapterIdx,
			.vendorId = desc.VendorId,
			.deviceId = desc.DeviceId,
			.description = desc.Description
		});
	}

	_UpdateProfiles();

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

bool AdaptersService::_GatherAdapterInfos(
	com_ptr<IDXGIFactory7>& dxgiFactory,
	wil::unique_event_nothrow& adaptersChangedEvent,
	DWORD& adaptersChangedCookie
) noexcept {
	// 显卡变化后需要重新创建 DXGI 工厂
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateDXGIFactory1 失败", hr);
		return false;
	}

	hr = dxgiFactory->RegisterAdaptersChangedEvent(
		adaptersChangedEvent.get(), &adaptersChangedCookie);
	if (FAILED(hr)) {
		Logger::Get().ComError("RegisterAdaptersChangedEvent 失败", hr);
		return false;
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
		if (FAILED(hr) || DirectXHelper::IsWARP(desc)) {
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

	// 删除不支持功能级别 11 的显卡
	std::vector<AdapterInfo> compatibleAdapterInfos;
	wil::srwlock writeLock;
	Win32Helper::RunParallel([&](uint32_t i) {
		D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
		if (SUCCEEDED(D3D11CreateDevice(adapters[i].get(), D3D_DRIVER_TYPE_UNKNOWN,
			NULL, 0, &fl, 1, D3D11_SDK_VERSION, nullptr, nullptr, nullptr))) {
			auto lock = writeLock.lock_exclusive();
			compatibleAdapterInfos.push_back(std::move(adapterInfos[i]));
		}
	}, (uint32_t)adapters.size());

	App::Get().Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [this, adapterInfos(std::move(compatibleAdapterInfos))]() {
		_adapterInfos = std::move(adapterInfos);
		_UpdateProfiles();
		AdaptersChanged.Invoke();
	});

	return true;
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
	if (!_GatherAdapterInfos(dxgiFactory, adaptersChangedEvent, adaptersChangedCookie)) {
		return;
	}

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
			if (!_GatherAdapterInfos(dxgiFactory, adaptersChangedEvent, adaptersChangedCookie)) {
				break;
			}
		}
	}

	dxgiFactory->UnregisterAdaptersChangedEvent(adaptersChangedCookie);
}

// 有更改返回 true
bool AdaptersService::_UpdateProfileGraphicsCardId(Profile& profile) noexcept {
	GraphicsCardId& gcid = profile.graphicsCardId;
	
	if (gcid.vendorId == 0 && gcid.deviceId == 0) {
		if (gcid.idx < 0) {
			// 使用默认显卡
			return false;
		}

		// 来自旧版本的配置文件不存在 vendorId 和 deviceId，更新为新版本
		auto it = std::find_if(_adapterInfos.begin(), _adapterInfos.end(),
			[&](const AdapterInfo& ai) { return (int)ai.idx == gcid.idx; });
		if (it == _adapterInfos.end()) {
			// 未找到序号则改为使用默认显卡，无论如何原始配置已经丢失
			gcid.idx = -1;
		} else {
			gcid.vendorId = it->vendorId;
			gcid.deviceId = it->deviceId;
		}

		return true;
	}

	auto it = std::find_if(_adapterInfos.begin(), _adapterInfos.end(), [&](const AdapterInfo& ai) {
		return (int)ai.idx == gcid.idx && ai.vendorId == gcid.vendorId && ai.deviceId == gcid.deviceId;
	});
	if (it != _adapterInfos.end()) {
		// 全部匹配
		return false;
	}

	// 序号指定的显卡不匹配则查找新序号
	it = std::find_if(_adapterInfos.begin(), _adapterInfos.end(), [&](const AdapterInfo& ai) {
		return ai.vendorId == gcid.vendorId && ai.deviceId == gcid.deviceId;
	});
	if (it == _adapterInfos.end()) {
		// 找不到则将 idx 置为 -1 表示使用默认显卡，不改变 vendorId 和 deviceId，
		// 这样当指定的显卡再次可用时将自动使用。
		if (gcid.idx == -1) {
			return false;
		} else {
			gcid.idx = -1;
			return true;
		}
	} else {
		gcid.idx = it->idx;
		return true;
	}
}

void AdaptersService::_UpdateProfiles() noexcept {
	bool needSave = false;

	// 更新所有配置文件的显卡配置
	if (_UpdateProfileGraphicsCardId(AppSettings::Get().DefaultProfile())) {
		needSave = true;
	}

	for (Profile& profile : AppSettings::Get().Profiles()) {
		if (_UpdateProfileGraphicsCardId(profile)) {
			needSave = true;
		}
	}

	if (needSave) {
		AppSettings::Get().SaveAsync();
	}
}

}
