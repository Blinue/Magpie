#pragma once
#include <compare>
#include <tuple>

struct Version {
	constexpr Version() {}
	constexpr Version(uint32_t major, uint32_t minor, uint32_t patch)
		: major(major), minor(minor), patch(patch) {}

	std::strong_ordering operator<=>(const Version& other) const noexcept {
		return std::make_tuple(major, minor, patch) <=> std::make_tuple(other.major, other.minor, other.patch);
	}

	bool Parse(std::string_view str);

	std::wstring ToString() const noexcept {
		return fmt::format(L"{}.{}.{}", major, minor, patch);
	}

	uint32_t major = 0;
	uint32_t minor = 0;
	uint32_t patch = 0;
};

constexpr inline Version MAGPIE_VERSION(0, 10, 3);
constexpr inline const char* MAGPIE_TAG = "v0.10.3";
constexpr inline const wchar_t* MAGPIE_TAG_W = L"v0.10.3";
