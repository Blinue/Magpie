#pragma once
#include "DeviceResources.h"
#include "EffectDrawer.h"
#include "Win32Utils.h"

namespace Magpie::Core {

struct ScalingOptions;
class DeviceResources;
class FrameSourceBase;

class Renderer {
public:
	Renderer() noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) noexcept = default;

	bool Initialize(HWND hwndSrc, HWND hwndScaling, const ScalingOptions& options) noexcept;

	void Render() noexcept;

private:
	void _BackendThreadProc(const ScalingOptions& options) noexcept;

	bool _InitFrameSource(const ScalingOptions& options) noexcept;

	bool _BuildEffects(const ScalingOptions& options) noexcept;

	void _BackendRender() noexcept;

	// 只能由前台线程访问
	DeviceResources _frontendResources;
	
	std::thread _backendThread;

	// 只能由后台线程访问
	DeviceResources _backendResources;
	std::unique_ptr<FrameSourceBase> _frameSource;
	std::vector<EffectDrawer> _effectDrawers;
	winrt::com_ptr<ID3D11Fence> _d3dFence;
	UINT64 _fenceValue = 0;
	Win32Utils::ScopedHandle _fenceEvent;

	// 可由所有线程访问
	HWND _hwndSrc = NULL;
	HWND _hwndScaling = NULL;
};

}
