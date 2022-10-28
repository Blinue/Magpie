#pragma once
#include "pch.h"
#include <AppxPackaging.h>
#include <variant>


namespace winrt::Magpie::UI {

// 用于解析打包应用
// 通常较为耗时（50 ms 左右），应在后台执行
class AppXReader {
public:
	bool Initialize(HWND hWnd) noexcept;

	void Initialize(std::wstring_view aumid) noexcept {
		_aumid = aumid;
	}

	bool Resolve() noexcept;

	const std::wstring& AUMID() const noexcept {
		return _aumid;
	}

	std::wstring GetDisplayName() noexcept;

	const std::wstring& GetPackagePath() noexcept;

	std::wstring GetExecutablePath() noexcept;
	
	std::variant<std::wstring, Windows::Graphics::Imaging::SoftwareBitmap> GetIcon(uint32_t preferredSize, bool isLightTheme, bool noPath = false) noexcept;

private:
	bool _ResolvePackagePath();

	std::wstring _aumid;
	std::wstring _praid;
	std::wstring _packageFullName;
	std::wstring _packagePath;
	com_ptr<IAppxManifestApplication> _appxApp;
};


}
