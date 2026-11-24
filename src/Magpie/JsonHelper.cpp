#include "pch.h"
#include "JsonHelper.h"
#include "StrHelper.h"

namespace Magpie {

bool JsonHelper::ReadBool(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	bool& result,
	bool required
) noexcept {
	auto it = obj.FindMember(name);
	if (it == obj.MemberEnd()) {
		return !required;
	}

	if (!it->value.IsBool()) {
		return false;
	}

	result = it->value.GetBool();
	return true;
}

bool JsonHelper::ReadUInt(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	uint32_t& result,
	bool required
) noexcept {
	auto it = obj.FindMember(name);
	if (it == obj.MemberEnd()) {
		return !required;
	}

	if (!it->value.IsUint()) {
		return false;
	}

	result = it->value.GetUint();
	return true;
}

bool JsonHelper::ReadUInt16(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	uint16_t& result,
	bool required
) noexcept {
	uint32_t value = 0;
	if (!ReadUInt(obj, name, value, required)) {
		return false;
	}

	assert(value >= std::numeric_limits<uint16_t>::min() &&
		value <= std::numeric_limits<uint16_t>::max());
	
	result = (uint16_t)value;
	return true;
}

bool JsonHelper::ReadInt(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	int& result,
	bool required
) noexcept {
	auto it = obj.FindMember(name);
	if (it == obj.MemberEnd()) {
		return !required;
	}

	if (!it->value.IsInt()) {
		return false;
	}

	result = it->value.GetInt();
	return true;
}

bool JsonHelper::ReadInt64(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	int64_t& result,
	bool required
) noexcept {
	auto it = obj.FindMember(name);
	if (it == obj.MemberEnd()) {
		return !required;
	}

	if (!it->value.IsInt64()) {
		return false;
	}

	result = it->value.GetInt64();
	return true;
}

bool JsonHelper::ReadFloat(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	float& result,
	bool required
) noexcept {
	auto it = obj.FindMember(name);
	if (it == obj.MemberEnd()) {
		return !required;
	}

	if (!it->value.IsNumber()) {
		return false;
	}

	result = it->value.GetFloat();
	return true;
}

bool JsonHelper::ReadString(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	wil::zstring_view& result,
	bool required
) noexcept {
	auto it = obj.FindMember(name);
	if (it == obj.MemberEnd()) {
		return !required;
	}

	if (!it->value.IsString()) {
		return false;
	}

	result = { it->value.GetString(), it->value.GetStringLength() };
	return true;
}

bool JsonHelper::ReadString(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	std::string& result,
	bool required
) noexcept {
	auto it = obj.FindMember(name);
	if (it == obj.MemberEnd()) {
		return !required;
	}

	if (!it->value.IsString()) {
		return false;
	}

	result.assign(it->value.GetString(), it->value.GetStringLength());
	return true;
}

bool JsonHelper::ReadString(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	std::wstring& result,
	bool required
) noexcept {
	auto it = obj.FindMember(name);
	if (it == obj.MemberEnd()) {
		return !required;
	}

	if (!it->value.IsString()) {
		return false;
	}

	result = StrHelper::UTF8ToUTF16(
		std::string_view(it->value.GetString(), it->value.GetStringLength()));
	return true;
}

}
