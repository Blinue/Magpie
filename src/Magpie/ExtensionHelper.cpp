#include "pch.h"
#include "ExtensionHelper.h"

bool ExtensionHelper::IsValidId(std::string_view id) noexcept {
	if (id.empty()) {
		return false;
	}

	// 只允许字母、数字、"-" 和 "_"
	const char* VALID_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";
	if (id.find_first_not_of(VALID_CHARS) != std::string_view::npos) {
		return false;
	}

	// 以下划线开头的 ID 保留内部使用
	return !id.starts_with("_");
}
