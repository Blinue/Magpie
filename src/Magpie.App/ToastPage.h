#pragma once
#include "ToastPage.g.h"

namespace winrt::Magpie::App::implementation {

struct ToastPage : ToastPageT<ToastPage>,
                   wil::notify_property_changed_base<ToastPage> {
	ToastPage(uint64_t hwndToast);

	Imaging::SoftwareBitmapSource Logo() const noexcept {
		return _logo;
	}

	bool IsLogoShown() const noexcept {
		return _isLogoShown;
	}

	fire_and_forget ShowMessageOnWindow(hstring message, uint64_t hwndTarget, bool inApp = false);

	void ShowMessageInApp(hstring message);

private:
	void _IsLogoShown(bool value);

	Imaging::SoftwareBitmapSource _logo{ nullptr };
	HWND _hwndToast;
	MUXC::TeachingTip _oldTeachingTip{ nullptr };
	bool _isLogoShown = true;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ToastPage : ToastPageT<ToastPage, implementation::ToastPage> {
};

}
