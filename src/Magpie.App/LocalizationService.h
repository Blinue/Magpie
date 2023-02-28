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

	// 应在初始化 XAML 框架后立即调用以正确设置弹窗的语言
	// 出于未知的原因，如果在初始化 XAML 框架前调用会导致无法在运行时切换语言
	void EarlyInitialize();

	void Initialize();

	static const std::vector<std::wstring>& SupportedLanguages() noexcept;

private:
	LocalizationService() = default;
};

}
