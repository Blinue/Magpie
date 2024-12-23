#pragma once
#include "ToastPage.g.h"
#include "Event.h"

namespace winrt::Magpie::implementation {

struct ToastPage : ToastPageT<ToastPage>, wil::notify_property_changed_base<ToastPage> {
	ToastPage(uint64_t hwndToast);

	void InitializeComponent();

	Imaging::SoftwareBitmapSource Logo() const noexcept {
		return _logo;
	}

	bool IsLogoShown() const noexcept {
		return _isLogoShown;
	}

	fire_and_forget ShowMessageOnWindow(std::wstring title, std::wstring message, HWND hwndTarget, bool showLogo);

	void Close() {
		_oldTeachingTip = nullptr;
		_isClosed = true;
	}

private:
	void _UpdateTheme();

	void _IsLogoShown(bool value);

	::Magpie::MultithreadEvent<bool>::EventRevoker _appThemeChangedRevoker;

	Imaging::SoftwareBitmapSource _logo{ nullptr };
	HWND _hwndToast;
	MUXC::TeachingTip _oldTeachingTip{ nullptr };
	bool _isLogoShown = true;
	// 防止退出时崩溃
	bool _isClosed = false;
};

}
