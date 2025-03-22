#include "pch.h"
#include "EffectsProfiler.h"
#include "DeviceResources.h"

namespace Magpie {

void EffectsProfiler::Start(ID3D11Device* d3dDevice, uint32_t passCount) {
	assert(_passQueries.empty());
	_passQueries.resize(passCount);

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

void EffectsProfiler::OnBeginEffects(ID3D11DeviceContext* d3dDC) {
	if (_passQueries.empty()) {
		return;
	}

	d3dDC->Begin(_disjointQuery.get());
	d3dDC->End(_startQuery.get());

	_curPass = 0;
}

void EffectsProfiler::OnEndPass(ID3D11DeviceContext* d3dDC) {
	if (_passQueries.empty()) {
		return;
	}

	d3dDC->End(_passQueries[_curPass++].get());
}

void EffectsProfiler::OnEndEffects(ID3D11DeviceContext* d3dDC) {
	if (_passQueries.empty()) {
		return;
	}

	d3dDC->End(_disjointQuery.get());
}

template <typename T>
static T GetQueryData(ID3D11DeviceContext* d3dDC, ID3D11Query* query) noexcept {
	T data{};
	while (d3dDC->GetData(query, &data, sizeof(data), 0) != S_OK) {
		Sleep(0);
	}
	return data;
}

void EffectsProfiler::QueryTimings(ID3D11DeviceContext* d3dDC) noexcept {
	if (_passQueries.empty()) {
		return;
	}

	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData =
		GetQueryData<D3D11_QUERY_DATA_TIMESTAMP_DISJOINT>(d3dDC, _disjointQuery.get());

	if (disjointData.Disjoint) {
		return;
	}

	const float toMS = 1000.0f / disjointData.Frequency;

	uint64_t prevTimestamp = GetQueryData<uint64_t>(d3dDC, _startQuery.get());

	auto lock = _timingsLock.lock_exclusive();
	_timings.resize(_passQueries.size());
	for (size_t i = 0; i < _passQueries.size(); ++i) {
		uint64_t timestamp = GetQueryData<uint64_t>(d3dDC, _passQueries[i].get());
		_timings[i] = (timestamp - prevTimestamp) * toMS;

		prevTimestamp = timestamp;
	}
}

SmallVector<float> EffectsProfiler::GetTimings() noexcept {
	auto lock = _timingsLock.lock_exclusive();

	// 没有渲染新帧时 _timings 为空
	SmallVector<float> result = std::move(_timings);
	_timings.clear();
	return result;
}

}
