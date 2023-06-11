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

	bool Initialize(HWND hwndSrc, HWND hwndScaling, const ScalingOptions& options) noexcept;

	void Render() noexcept;

private:
	void _BackendThreadProc(HWND hwndSrc, HWND hwndScaling, const ScalingOptions& options) noexcept;

	std::unique_ptr<DeviceResources> _frontendResources;
	
	std::thread _backendThread;
};

}
