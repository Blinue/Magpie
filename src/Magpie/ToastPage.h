#pragma once
#include "ToastPage.g.h"
#include "Event.h"
#include "AppSettings.h"

namespace winrt::Magpie::implementation {

struct ToastPage : ToastPageT<ToastPage>,
                   wil::notify_property_changed_base<ToastPage> {
	ToastPage(uint64_t hwndToast);

	void InitializeComponent();

	Imaging::SoftwareBitmapSource Logo() const noexcept {
		return _logo;
	}

	bool IsLogoShown() const noexcept {
		return _isLogoShown;
	}

	fire_and_forget ShowMessageOnWindow(hstring title, hstring message, uint64_t hwndTarget, bool inApp = false);

	void ShowMessageInApp(hstring title, hstring message);

private:
	void _UpdateTheme();

	void _IsLogoShown(bool value);

	::Magpie::EventRevoker _appThemeChangedRevoker;

	Imaging::SoftwareBitmapSource _logo{ nullptr };
	HWND _hwndToast;
	MUXC::TeachingTip _oldTeachingTip{ nullptr };
	bool _isLogoShown = true;
};

}

namespace winrt::Magpie::factory_implementation {

struct ToastPage : ToastPageT<ToastPage, implementation::ToastPage> {
};

}
