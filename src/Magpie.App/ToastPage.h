#pragma once
#include "ToastPage.g.h"

namespace winrt::Magpie::App::implementation {

struct ToastPage : ToastPageT<ToastPage> {
    IAsyncAction ShowMessage(const hstring& message);
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ToastPage : ToastPageT<ToastPage, implementation::ToastPage> {
};

}
