#define DEFINE_FLAG_ACCESSOR(Name, FlagBit, FlagsVar) \
	bool Name() const noexcept { return WI_IsFlagSet(FlagsVar, FlagBit); } \
	void Name(bool value) noexcept { WI_UpdateFlag(FlagsVar, FlagBit, value); }

#define _WIDEN_HELPER(x) L ## x
#define WIDEN(x) _WIDEN_HELPER(x)
#define _STRING_HELPER(x) #x
#define STRING(x) _STRING_HELPER(x)

struct Ignore {
	constexpr Ignore() noexcept = default;

	template <typename T>
	constexpr Ignore(const T&) noexcept {}

	template <typename T>
	constexpr const Ignore& operator=(const T&) const noexcept {
		return *this;
	}
};

// 这些宏用于实验或调试

// 把用于调整窗口尺寸的辅助窗口标示出来
// #define MP_DEBUG_BORDER

// 在性能分析器上显示调试信息
// #define MP_DEBUG_OVERLAY

// 使用 composition swapchain 呈现
// #define MP_USE_COMPSWAPCHAIN
