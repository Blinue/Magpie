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
