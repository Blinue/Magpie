#pragma once

namespace winrt::Magpie::App {

class LocalizationService {
public:
	static LocalizationService& Get() noexcept {
		static LocalizationService instance;
		return instance;
	}

	LocalizationService(const LocalizationService&) = delete;
	LocalizationService(LocalizationService&&) = delete;

	void Initialize();

	static std::span<const wchar_t*> SupportedLanguages() noexcept;

private:
	LocalizationService() = default;
};

}
