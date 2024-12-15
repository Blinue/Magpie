#pragma once

namespace Magpie {

class LocalizationService {
public:
	static LocalizationService& Get() noexcept {
		static LocalizationService instance;
		return instance;
	}

	LocalizationService(const LocalizationService&) = delete;
	LocalizationService(LocalizationService&&) = delete;

	// 在初始化 AppSettings 前调用以使用系统默认语言，然后就可以从 AppSettings 里读取语言设置
	void EarlyInitialize();

	void Initialize();

	// 支持的所有语言的标签，均为小写
	static std::span<const wchar_t* > SupportedLanguages() noexcept;

	const wchar_t* Language() const noexcept {
		return _language;
	}

private:
	LocalizationService() = default;

	void _Language(const wchar_t* tag);

	const wchar_t* _language = nullptr;
};

}
