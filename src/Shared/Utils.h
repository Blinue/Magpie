#pragma once

struct Utils {
	// 单位为微秒
	template<typename Fn>
	static int Measure(const Fn& func) {
		using namespace std::chrono;

		auto t = steady_clock::now();
		func();
		auto dura = duration_cast<microseconds>(steady_clock::now() - t);

		return int(dura.count());
	}

	static uint64_t HashData(std::span<const BYTE> data) noexcept;

	struct Ignore {
		constexpr Ignore() noexcept = default;

		template <typename T>
		constexpr Ignore(const T&) noexcept {}

		template <typename T>
		constexpr const Ignore& operator=(const T&) const noexcept {
			return *this;
		}
	};
};
