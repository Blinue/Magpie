#pragma once
#include <rapidjson/document.h>

namespace winrt::Magpie::App {

struct JsonHelper {
	static bool ReadBool(
		const rapidjson::GenericObject<true, rapidjson::Value>& obj,
		const char* name,
		bool& result,
		bool required = false
	) noexcept;

	static bool ReadBoolFlag(
		const rapidjson::GenericObject<true, rapidjson::Value>& obj,
		const char* nodeName,
		uint32_t flagBit,
		uint32_t& flags,
		bool required = false
	) noexcept;

	static bool ReadUInt(
		const rapidjson::GenericObject<true, rapidjson::Value>& obj,
		const char* name,
		uint32_t& result,
		bool required = false
	) noexcept;

	static bool ReadInt(
		const rapidjson::GenericObject<true, rapidjson::Value>& obj,
		const char* name,
		int& result,
		bool required = false
	) noexcept;

	static bool ReadInt64(
		const rapidjson::GenericObject<true, rapidjson::Value>& obj,
		const char* name,
		int64_t& result,
		bool required = false
	) noexcept;

	static bool ReadFloat(
		const rapidjson::GenericObject<true, rapidjson::Value>& obj,
		const char* name,
		float& result,
		bool required = false
	) noexcept;

	static bool ReadString(
		const rapidjson::GenericObject<true, rapidjson::Value>& obj,
		const char* name,
		std::wstring& result,
		bool required = false
	) noexcept;
};

}
