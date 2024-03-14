#pragma once
#include "SmallVector.h"
#include "Win32Utils.h"

namespace Magpie::Core {

class DeviceResources;

class EffectsProfiler {
public:
	EffectsProfiler() = default;

	EffectsProfiler(const EffectsProfiler&) = delete;
	EffectsProfiler(EffectsProfiler&&) = delete;

	void Start(ID3D11Device* d3dDevice, uint32_t passCount);

	void Stop();

	void OnBeginEffects(ID3D11DeviceContext* d3dDC);

	void OnEndPass(ID3D11DeviceContext* d3dDC);

	void OnEndEffects(ID3D11DeviceContext* d3dDC);

	void QueryTimings(ID3D11DeviceContext* d3dDC) noexcept;

	// 从前端线程调用
	SmallVector<float> GetTimings() noexcept;

private:
	SmallVector<float> _timings;
	Win32Utils::SRWMutex _timingsMutex;

	winrt::com_ptr<ID3D11Query> _disjointQuery;
	winrt::com_ptr<ID3D11Query> _startQuery;
	std::vector<winrt::com_ptr<ID3D11Query>> _passQueries;

	uint32_t _curPass = 0;
};

}
