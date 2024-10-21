#pragma once
#include "ToastPage.g.h"

namespace winrt::Magpie::App::implementation {

struct ToastPage : ToastPageT<ToastPage> {
    MUXC::TeachingTip ShowMessage(const hstring& message);
    void HideMessage();

private:
    MUXC::TeachingTip _prevTeachingTip{ nullptr };
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ToastPage : ToastPageT<ToastPage, implementation::ToastPage> {
};

}
