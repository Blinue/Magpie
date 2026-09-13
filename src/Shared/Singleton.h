#pragma once

namespace Magpie {

template <typename T>
class Singleton {
public:
	Singleton(const Singleton&) = delete;
	Singleton(Singleton&&) = delete;

	static T& Get() noexcept {
		// 这要求 T 可以直接在头文件中构造，且构造函数对 Singleton 可见
		static T instance;
		return instance;
	}

protected:
	Singleton() = default;
};

}
