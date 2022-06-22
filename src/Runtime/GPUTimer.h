#pragma once
#include "pch.h"


// 用于记录帧率和 GPU 时间
class GPUTimer {
public:
	// 上一帧的渲染时间
	std::chrono::nanoseconds GetElapsedTime() const noexcept { return _elapsedTime; }

	// 经过的总时间
	std::chrono::nanoseconds GetTotalTime() const noexcept { return _totalTime; }

	// 经过的总帧数
	UINT GetFrameCount() const noexcept { return _frameCount; }

	// 上一秒的帧数
	UINT GetFramesPerSecond() const noexcept { return _framesPerSecond; }

	// 在每帧开始时调用，用于记录帧率和检索渲染用时
	void OnBeginFrame();

	struct GPUTimings {
		std::vector<float> passes;
		// float overlay = 0.0f;
	};

	// 所有元素的处理时间，单位为 ms
	const GPUTimings& GetGPUTimings() const noexcept {
		return _gpuTimings;
	}

	// updateInterval 为更新渲染用时的间隔
	// 可为 0，即每帧都更新
	void StartProfiling(std::chrono::microseconds updateInterval, UINT passCount);

	void StopProfiling();

	void OnBeginEffects();

	// 每个通道结束后调用
	void OnEndPass(UINT idx);

	void OnEndEffects();

private:
	void _UpdateGPUTimings();

	std::chrono::time_point<std::chrono::steady_clock> _lastTimePoint;

	std::chrono::nanoseconds _elapsedTime{};
	std::chrono::nanoseconds _totalTime{};

	UINT _frameCount = 0;
	UINT _framesPerSecond = 0;
	UINT _framesThisSecond = 0;
	std::chrono::nanoseconds _fpsCounter{};

	GPUTimings _gpuTimings;
	// 记录的第一帧首先更新一次，而不是等待更新间隔
	bool _firstProfilingFrame = true;
	// 更新渲染用时的间隔
	std::chrono::nanoseconds _updateProfilingTime{};
	std::chrono::nanoseconds _profilingCounter{};

	struct _QueryInfo {
		winrt::com_ptr<ID3D11Query> disjoint;
		winrt::com_ptr<ID3D11Query> start;
		std::vector<winrt::com_ptr<ID3D11Query>> passes;
	};
	// [(disjoint, [timestamp])]
	// 允许额外的延迟时需保存两帧的数据
	std::array<_QueryInfo, 2> _queries;
	// -1：无需统计渲染时间
	// 否则为当前帧在 _queries 中的位置
	int _curQueryIdx = -1;

	// 用于保存渲染时间
	// (总计用时, 已统计帧数)
	std::vector<std::pair<float, UINT>> _passesTimings;
};
