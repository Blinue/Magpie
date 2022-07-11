#pragma once
#include "pch.h"
#include <AppxPackaging.h>


namespace winrt::Magpie::App {

struct AppXHelper {
	class AppXReader {
	public:
		bool Initialize(HWND hWnd) noexcept;

		std::wstring GetDisplayName() const noexcept;

		std::wstring GetIconPath(SIZE preferredSize) const noexcept;

	private:
		bool _ResolveApplication(const std::wstring& praid) noexcept;

		std::wstring _packageFullName;
		std::wstring _packagePath;
		com_ptr<IAppxManifestApplication> _appxApp;
	};
};

}
