#pragma once
#include "pch.h"
#include "EffectDesc.h"

class EffectDrawer;
class GPUTimer;
class OverlayDrawer;
class CursorManager;


class Renderer {
public:
	Renderer();
	Renderer(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;

	~Renderer();

	bool Initialize(const std::string& effectsJson);

	void Render();

	GPUTimer& GetGPUTimer() {
		return *_gpuTimer;
	}

	// 可能为空
	OverlayDrawer* GetOverlayDrawer() {
		return _overlayDrawer.get();
	}

	bool IsUIVisiable() const noexcept;

	void SetUIVisibility(bool value);

	const RECT& GetOutputRect() const noexcept {
		return _outputRect;
	}

	const RECT& GetVirtualOutputRect() const noexcept {
		return _virtualOutputRect;
	}

	UINT GetEffectCount() const noexcept {
		return (UINT)_effects.size();
	}

	const EffectDesc& GetEffectDesc(UINT idx) const noexcept;

private:
	bool _CheckSrcState();

	bool _ResolveEffectsJson(const std::string& effectsJson);

	bool _UpdateDynamicConstants();

	RECT _srcWndRect{};
	RECT _outputRect{};
	// 尺寸可能大于主窗口
	RECT _virtualOutputRect{};

	bool _waitingForNextFrame = false;

	std::vector<std::unique_ptr<EffectDrawer>> _effects;
	std::array<EffectConstant32, 12> _dynamicConstants;
	winrt::com_ptr<ID3D11Buffer> _dynamicCB;

	std::unique_ptr<OverlayDrawer> _overlayDrawer;
	UINT _handlerID = 0;

	std::unique_ptr<GPUTimer> _gpuTimer;
};
