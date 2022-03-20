#pragma once
#include "pch.h"
#include "EffectDesc.h"


class EffectDrawer;
class GPUTimer;
class UIDrawer;
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

	const RECT& GetOutputRect() const noexcept {
		return _outputRect;
	}

	const RECT& GetVirtualOutputRect() const noexcept {
		return _virtualOutputRect;
	}

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

	std::unique_ptr<UIDrawer> _UIDrawer;

	std::unique_ptr<GPUTimer> _gpuTimer;
};
