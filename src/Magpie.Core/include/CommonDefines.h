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

template <typename T>
static constexpr inline T FLOAT_EPSILON = std::numeric_limits<T>::epsilon() * 100;

// 不支持 nan 和无穷大
template <typename T>
static bool IsApprox(T l, T r) noexcept {
	static_assert(std::is_floating_point_v<T>, "T 必须是浮点数类型");
	return std::abs(l - r) < FLOAT_EPSILON<T>;
}

// 这些宏用于实验或调试

// 窗口模式缩放时把用于调整窗口尺寸的辅助窗口标示出来
// #define MP_DEBUG_BORDER

// 在性能分析器上显示调试信息
// #define MP_DEBUG_OVERLAY

// 使用 composition swapchain 呈现
// #define MP_USE_COMPSWAPCHAIN
