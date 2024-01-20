#include "pch.h"
#include "EffectsProfiler.h"
#include "DeviceResources.h"
#include <mutex>

namespace Magpie::Core {

void EffectsProfiler::Initialize(uint32_t passCount, DeviceResources* deviceResource) noexcept {
	_deviceResource = deviceResource;

	// 初始化时不会有竞态条件
	_timings.resize(passCount);
}

void EffectsProfiler::Start() {
	assert(_passQueries.empty());
	_passQueries.resize(_timings.size());

	ID3D11Device* d3dDevice = _deviceResource->GetD3DDevice();

	D3D11_QUERY_DESC desc{ .Query = D3D11_QUERY_TIMESTAMP_DISJOINT };
	d3dDevice->CreateQuery(&desc, _disjointQuery.put());

	desc.Query = D3D11_QUERY_TIMESTAMP;
	d3dDevice->CreateQuery(&desc, _startQuery.put());
	for (winrt::com_ptr<ID3D11Query>& query : _passQueries) {
		d3dDevice->CreateQuery(&desc, query.put());
	}
}

void EffectsProfiler::Stop() {
	_disjointQuery = nullptr;
	_startQuery = nullptr;
	_passQueries.clear();
}

void EffectsProfiler::OnBeginEffects() {
	if (_passQueries.empty()) {
		return;
	}

	ID3D11DeviceContext* d3dDC = _deviceResource->GetD3DDC();
	d3dDC->Begin(_disjointQuery.get());
	d3dDC->End(_startQuery.get());

	_curPass = 0;
}

void EffectsProfiler::OnEndPass() {
	if (_passQueries.empty()) {
		return;
	}

	ID3D11DeviceContext* d3dDC = _deviceResource->GetD3DDC();
	d3dDC->End(_passQueries[_curPass++].get());
}

void EffectsProfiler::OnEndEffects() {
	if (_passQueries.empty()) {
		return;
	}

	ID3D11DeviceContext* d3dDC = _deviceResource->GetD3DDC();
	d3dDC->End(_disjointQuery.get());
}

template<typename T>
static T GetQueryData(ID3D11DeviceContext* d3dDC, ID3D11Query* query) noexcept {
	T data{};
	while (d3dDC->GetData(query, &data, sizeof(data), 0) != S_OK) {
		Sleep(0);
	}
	return data;
}

void EffectsProfiler::QueryTimings() noexcept {
	if (_passQueries.empty()) {
		return;
	}

	ID3D11DeviceContext* d3dDC = _deviceResource->GetD3DDC();

	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData =
		GetQueryData<D3D11_QUERY_DATA_TIMESTAMP_DISJOINT>(d3dDC, _disjointQuery.get());

	if (disjointData.Disjoint) {
		return;
	}

	const float toMS = 1000.0f / disjointData.Frequency;

	uint64_t prevTimestamp = GetQueryData<uint64_t>(d3dDC, _startQuery.get());

	for (size_t i = 0; i < _passQueries.size(); ++i) {
		uint64_t timestamp = GetQueryData<uint64_t>(d3dDC, _passQueries[i].get());
		_timings[i] = (timestamp - prevTimestamp) * toMS;

		prevTimestamp = timestamp;
	}
}

SmallVector<float> EffectsProfiler::GetTimings() noexcept {
	std::scoped_lock lk(_timingsMutex);
	return _timings;
}

}
