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

	void Initialize(uint32_t passCount, DeviceResources* deviceResource) noexcept;

	void Start();

	void Stop();

	void OnBeginEffects();

	void OnEndPass();

	void OnEndEffects();

	void QueryTimings() noexcept;

	// 从前端线程调用
	SmallVector<float> GetTimings() noexcept;

private:
	DeviceResources* _deviceResource = nullptr;

	SmallVector<float> _timings;
	Win32Utils::SRWMutex _timingsMutex;

	winrt::com_ptr<ID3D11Query> _disjointQuery;
	winrt::com_ptr<ID3D11Query> _startQuery;
	std::vector<winrt::com_ptr<ID3D11Query>> _passQueries;

	uint32_t _curPass = 0;
};

}
