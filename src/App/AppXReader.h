#pragma once
#include "pch.h"
#include <AppxPackaging.h>
#include <variant>


namespace winrt::Magpie::App {

class AppXReader {
public:
	bool Initialize(HWND hWnd) noexcept;

	void Initialize(std::wstring_view aumid) noexcept {
		_aumid = aumid;
	}

	std::wstring GetDisplayName() noexcept;

	std::variant<std::wstring, Windows::Graphics::Imaging::SoftwareBitmap> GetIcon(uint32_t preferredSize, bool isLightTheme, bool noPath = false) noexcept;

	const std::wstring& AUMID() const noexcept {
		return _aumid;
	}

private:
	bool _ResolveApplication(const std::wstring& praid) noexcept;

	bool _LoadManifest();

	std::wstring _aumid;
	std::wstring _packageFullName;
	std::wstring _packagePath;
	com_ptr<IAppxManifestApplication> _appxApp;
};


}
