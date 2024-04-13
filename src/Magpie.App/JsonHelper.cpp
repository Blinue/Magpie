#include "pch.h"
#include "JsonHelper.h"
#include "StrUtils.h"


namespace winrt::Magpie::App {

bool JsonHelper::ReadBool(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	bool& result,
	bool required
) noexcept {
	auto node = obj.FindMember(name);
	if (node == obj.MemberEnd()) {
		return !required;
	}

	if (!node->value.IsBool()) {
		return false;
	}

	result = node->value.GetBool();
	return true;
}

bool JsonHelper::ReadBoolFlag(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* nodeName,
	uint32_t flagBit,
	uint32_t& flags,
	bool required
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

bool JsonHelper::ReadUInt(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	uint32_t& result,
	bool required
) noexcept {
	auto node = obj.FindMember(name);
	if (node == obj.MemberEnd()) {
		return !required;
	}

	if (!node->value.IsUint()) {
		return false;
	}

	result = node->value.GetUint();
	return true;
}

bool JsonHelper::ReadInt(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	int& result,
	bool required
) noexcept {
	auto node = obj.FindMember(name);
	if (node == obj.MemberEnd()) {
		return !required;
	}

	if (!node->value.IsInt()) {
		return false;
	}

	result = node->value.GetInt();
	return true;
}

bool JsonHelper::ReadInt64(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	int64_t& result,
	bool required
) noexcept {
	auto node = obj.FindMember(name);
	if (node == obj.MemberEnd()) {
		return !required;
	}

	if (!node->value.IsInt64()) {
		return false;
	}

	result = node->value.GetInt64();
	return true;
}

bool JsonHelper::ReadFloat(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	float& result,
	bool required
) noexcept {
	auto node = obj.FindMember(name);
	if (node == obj.MemberEnd()) {
		return !required;
	}

	if (!node->value.IsNumber()) {
		return false;
	}

	result = node->value.GetFloat();
	return true;
}

bool JsonHelper::ReadString(
	const rapidjson::GenericObject<true, rapidjson::Value>& obj,
	const char* name,
	std::wstring& result,
	bool required
) noexcept {
	auto node = obj.FindMember(name);
	if (node == obj.MemberEnd()) {
		return !required;
	}

	if (!node->value.IsString()) {
		return false;
	}

	result = StrUtils::UTF8ToUTF16(node->value.GetString());
	return true;
}

}
