#pragma once
#include "pch.h"


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

	CursorManager& GetCursorManager() noexcept {
		return *_cursorManager;
	}

	const RECT& GetOutputRect() const noexcept {
		return _outputRect;
	}

private:
	bool _CheckSrcState();

	bool _ResolveEffectsJson(const std::string& effectsJson);

	RECT _srcWndRect{};
	RECT _outputRect{};

	bool _waitingForNextFrame = false;

	std::vector<std::unique_ptr<EffectDrawer>> _effects;

	std::unique_ptr<UIDrawer> _UIDrawer;

	std::unique_ptr<GPUTimer> _gpuTimer;
	std::unique_ptr<CursorManager> _cursorManager;
};
