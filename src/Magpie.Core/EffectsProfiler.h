#pragma once
#include "SmallVector.h"

namespace Magpie {

class DeviceResources;

class EffectsProfiler {
public:
	EffectsProfiler() = default;

	EffectsProfiler(const EffectsProfiler&) = delete;
	EffectsProfiler(EffectsProfiler&&) = delete;

	void Start(ID3D11Device* d3dDevice, uint32_t passCount) noexcept;

	void Stop() noexcept;

	bool IsProfiling() const noexcept;

	void SetPassCount(ID3D11Device* d3dDevice, uint32_t passCount) noexcept;

	void OnBeginEffects(ID3D11DeviceContext* d3dDC) noexcept;

	void OnEndPass(ID3D11DeviceContext* d3dDC) noexcept;

	void OnEndEffects(ID3D11DeviceContext* d3dDC) noexcept;

	void QueryTimings(ID3D11DeviceContext* d3dDC) noexcept;

	// 从前端线程调用
	SmallVector<float> GetTimings() noexcept;

private:
	SmallVector<float> _timings;
	wil::srwlock _timingsLock;

	winrt::com_ptr<ID3D11Query> _disjointQuery;
	winrt::com_ptr<ID3D11Query> _startQuery;
	std::vector<winrt::com_ptr<ID3D11Query>> _passQueries;

	uint32_t _curPass = 0;
};

}
