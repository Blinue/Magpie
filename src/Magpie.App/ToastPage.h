#pragma once
#include "ToastPage.g.h"

namespace winrt::Magpie::App::implementation {

struct ToastPage : ToastPageT<ToastPage>,
                   wil::notify_property_changed_base<ToastPage> {
	ToastPage(uint64_t hwndToast);

	Imaging::SoftwareBitmapSource Logo() const noexcept {
		return _logo;
	}

	fire_and_forget ShowMessageOnWindow(hstring message, uint64_t hwndTarget);

private:
	Imaging::SoftwareBitmapSource _logo{ nullptr };
	HWND _hwndToast;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ToastPage : ToastPageT<ToastPage, implementation::ToastPage> {
};

}
