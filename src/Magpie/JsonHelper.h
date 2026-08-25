#pragma once
#include <rapidjson/document.h>

namespace Magpie {

struct JsonHelper {
	static bool ReadBool(
		const rapidjson::GenericObject<true, rapidjson::Value>& obj,
		const char* name,
		bool& result,
		bool required = false
	) noexcept;

	template <typename Enum>
	static bool ReadBoolFlag(
		const rapidjson::GenericObject<true, rapidjson::Value>& obj,
		const char* nodeName,
		Enum flagBit,
		Enum& flags,
		bool required = false
	) noexcept {
		auto node = obj.FindMember(nodeName);
		if (node == obj.MemberEnd()) {
			return !required;
		}

		if (!node->value.IsBool()) {
			return false;
		}

		if (node->value.GetBool()) {
			flags |= flagBit;
		} else {
			flags &= ~flagBit;
		}

		return true;
	}

	template <typename Enum>
	static bool ReadEnum(
		const rapidjson::GenericObject<true, rapidjson::Value>& obj,
		const char* name,
		Enum& result,
		bool required = false
	) noexcept {
		auto node = obj.FindMember(name);
		if (node == obj.MemberEnd()) {
			return !required;
		}

		if (!node->value.IsUint()) {
			return false;
		}

		uint32_t value = node->value.GetUint();
		if (value < (uint32_t)Enum::COUNT) {
			result = (Enum)value;
		}

		return true;
	}

	static bool ReadUInt(
		const rapidjson::GenericObject<true, rapidjson::Value>& obj,
		const char* name,
		uint32_t& result,
		bool required = false
	) noexcept;

	static bool ReadUInt16(
		const rapidjson::GenericObject<true, rapidjson::Value>& obj,
		const char* name,
		uint16_t& result,
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
