#pragma once
#include "pch.h"
#include <AppxPackaging.h>


namespace winrt::Magpie::App {

struct AppXHelper {
	static std::wstring GetPackageFullName(HWND hWnd);

	class AppXReader {
	public:
		bool Initialize(HWND hWnd) noexcept;

		std::wstring GetDisplayName() const noexcept;

	private:
		bool _ResolveApplication(const std::wstring& praid) noexcept;

		std::wstring _packageFullName;
		com_ptr<IAppxManifestApplication> _appxApp;
	};
};

}
