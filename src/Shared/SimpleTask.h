#pragma once

namespace Magpie {

template <typename T>
struct SimpleTask {
	~SimpleTask() {
		if (!_isWaited) {
			_data.wait(std::pair<T, bool>{}, std::memory_order_relaxed);
		}
	}

	// 创建线程使用
	T GetResult(std::memory_order memoryOrder = std::memory_order_relaxed) noexcept {
		if (!_isWaited) {
			_data.wait(std::pair<T, bool>{}, std::memory_order_relaxed);
			_isWaited = true;
		}
		
		return _data.load(memoryOrder).first;
	}

	// 异步线程使用
	void SetResult(T value, std::memory_order memoryOrder = std::memory_order_relaxed) noexcept {
		assert(!_data.load(std::memory_order_relaxed).second);
		_data.store(std::make_pair(value, true), memoryOrder);
		_data.notify_one();
	}

private:
	std::atomic<std::pair<T, bool>> _data;
	bool _isWaited = false;
};

}
