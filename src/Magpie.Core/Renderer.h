#pragma once

namespace Magpie::Core {

struct ScalingOptions;
class DeviceResources;

class Renderer {
public:
	Renderer() noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) noexcept = default;

	bool Initialize(HWND hwndScaling, const ScalingOptions& options) noexcept;

	void Render() noexcept;

private:
	std::unique_ptr<DeviceResources> _deviceResources;
};

}
