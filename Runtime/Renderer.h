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

	size_t GetEffectCount() const noexcept {
		return _effects.size();
	}

	const EffectDesc& GetEffectDesc(size_t idx) const noexcept;

	// 所有通道的处理时间，单位为 ms
	// 不可用返回空
	// 对于 Desktop Duplication 捕获模式未执行的通道为 0
	const std::vector<float>& GetEffectTimings() const  noexcept {
		return _effectTimings;
	}

private:
	bool _InitializeOverlayDrawer();

	bool _CheckSrcState();

	bool _ResolveEffectsJson(const std::string& effectsJson);

	bool _UpdateDynamicConstants();

	void _UpdateEffectTimings();

	bool _InitQueries(UINT idx);

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

	// [(disjoint, [timestamp])]
	// 允许额外的延迟时需保存两帧的数据
	std::array<std::pair<winrt::com_ptr<ID3D11Query>, std::vector<winrt::com_ptr<ID3D11Query>>>, 2> _queries;
	UINT _curQueryIdx = 0;
	std::vector<float> _effectTimings;
};
