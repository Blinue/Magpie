#pragma once
#include "SmallVector.h"
#include <dxgi1_6.h>
#include "Event.h"

namespace Magpie {

struct Profile;

struct AdapterInfo {
	uint32_t idx = 0;
	uint32_t vendorId = 0;
	uint32_t deviceId = 0;
	std::wstring description;
	// 延迟设置，仅用于用户界面提示
	bool isFL11Supported = true;
};

class AdaptersService {
public:
	static AdaptersService& Get() noexcept {
		static AdaptersService instance;
		return instance;
	}

	AdaptersService(const AdaptersService&) = delete;
	AdaptersService(AdaptersService&&) = delete;

	bool Initialize() noexcept;

	void Uninitialize() noexcept;

	const std::vector<AdapterInfo>& AdapterInfos() const noexcept {
		return _adapterInfos;
	}

	Event<> AdaptersChanged;

private:
	AdaptersService() = default;

	bool _GatherAdapterInfos(
		winrt::com_ptr<IDXGIFactory7>& dxgiFactory,
		wil::unique_event_nothrow& adaptersChangedEvent,
		DWORD& adaptersChangedCookie
	) noexcept;

	void _MonitorThreadProc() noexcept;

	bool _UpdateProfileGraphicsCardId(Profile& profile, int adapterCount) noexcept;

	void _UpdateProfiles() noexcept;

	std::thread _monitorThread;
	std::vector<AdapterInfo> _adapterInfos;
};

}
