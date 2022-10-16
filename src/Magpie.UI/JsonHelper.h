#pragma once
#include "pch.h"
#include <rapidjson/document.h>


namespace winrt::Magpie::UI {

struct JsonHelper {
	static bool ReadBool(
		const rapidjson::GenericObject<false, rapidjson::Value>& obj,
		const char* name,
		bool& result,
		bool required = false
	);

	static bool ReadBoolFlag(
		const rapidjson::GenericObject<false, rapidjson::Value>& obj,
		const char* nodeName,
		uint32_t flagBit,
		uint32_t& flags
	);

	static bool ReadUInt(
		const rapidjson::GenericObject<false, rapidjson::Value>& obj,
		const char* name,
		uint32_t& result,
		bool required = false
	);

	static bool ReadInt(
		const rapidjson::GenericObject<false, rapidjson::Value>& obj,
		const char* name,
		int& result,
		bool required = false
	);

	static bool ReadFloat(
		const rapidjson::GenericObject<false, rapidjson::Value>& obj,
		const char* name,
		float& result,
		bool required = false
	);

	static bool ReadString(
		const rapidjson::GenericObject<false, rapidjson::Value>& obj,
		const char* name,
		std::wstring& result,
		bool required = false
	);
};

}
