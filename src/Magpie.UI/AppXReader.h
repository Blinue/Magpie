#pragma once
#include <variant>

namespace winrt::Magpie::UI {

// 用于解析打包应用
// 通常较为耗时（50 ms 左右），应在后台执行
class AppXReader {
public:
	bool Initialize(HWND hWnd) noexcept;

	bool Initialize(std::wstring_view aumid) noexcept;

	const std::wstring& AUMID() const noexcept {
		return _aumid;
	}

	std::wstring GetDisplayName() noexcept;

	const std::wstring& GetPackagePath() noexcept;

	std::wstring GetExecutablePath() noexcept;
	
	std::variant<std::wstring, Windows::Graphics::Imaging::SoftwareBitmap> GetIcon(
		uint32_t preferredSize,
		bool isLightTheme,
		bool noPath = false
	) noexcept;

	static void ClearCache() noexcept;

private:
	bool _ResolvePackagePath();

	std::wstring _aumid;
	std::wstring _praid;
	std::wstring _packageFullName;
	std::wstring _packagePath;
	std::wstring _displayName;
	std::wstring _executable;
	std::wstring _square44x44Logo;
};

}
