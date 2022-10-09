#pragma once
#include "CommonPCH.h"


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

	static int CompareVersion(int major1, int minor1, int build1, int major2, int minor2, int build2) noexcept {
		if (major1 != major2) {
			return major1 - major2;
		}

		if (minor1 != minor2) {
			return minor1 - minor2;
		} else {
			return build1 - build2;
		}
	}

	template<typename T>
	class ScopeExit {
	public:
		ScopeExit(const ScopeExit&) = delete;
		ScopeExit(ScopeExit&&) = delete;

		explicit ScopeExit(T&& exitScope) : _exitScope(std::forward<T>(exitScope)) {}
		~ScopeExit() { _exitScope(); }

	private:
		T _exitScope;
	};

	static bool ZstdCompress(std::span<const BYTE> src, std::vector<BYTE>& dest, int compressionLevel);
	static bool ZstdDecompress(std::span<const BYTE> src, std::vector<BYTE>& dest);

	static uint64_t HashData(std::span<const BYTE> data) noexcept;
};
