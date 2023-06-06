#pragma once

namespace Magpie::Core {

struct ScalingOptions;

class DeviceResources {
public:
	DeviceResources() noexcept = default;
	DeviceResources(const DeviceResources&) = delete;
	DeviceResources(DeviceResources&&) noexcept = default;

	bool Initialize(HWND hwndScaling, const ScalingOptions& options) noexcept;

	static bool IsDebugLayersAvailable() noexcept;
};

}
