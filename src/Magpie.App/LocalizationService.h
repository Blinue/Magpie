#pragma once

class LocalizationService {
public:
	static LocalizationService& Get() noexcept {
		static LocalizationService instance;
		return instance;
	}

	LocalizationService(const LocalizationService&) = delete;
	LocalizationService(LocalizationService&&) = delete;

	void Initialize();

	std::span<const wchar_t*> GetSupportedLanguages() const noexcept;

private:
	LocalizationService() = default;
};
