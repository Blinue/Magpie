#pragma once
#include "pch.h"
#include <AppxPackaging.h>


namespace winrt::Magpie::App {

class AppXReader {
public:
	bool Initialize(HWND hWnd) noexcept;

	std::wstring GetDisplayName() noexcept;

	std::wstring GetIconPath(uint32_t preferredSize, bool preferLightTheme, bool* hasBackground = nullptr) noexcept;

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
