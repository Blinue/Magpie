#pragma once
#include "ToastPage.g.h"

namespace winrt::Magpie::App::implementation {

struct ToastPage : ToastPageT<ToastPage> {
	ToastPage(uint64_t hwndToast) : _hwndToast((HWND)hwndToast) {}

	fire_and_forget ShowMessageOnWindow(hstring message, uint64_t hwndTarget);

private:
	HWND _hwndToast;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ToastPage : ToastPageT<ToastPage, implementation::ToastPage> {
};

}
