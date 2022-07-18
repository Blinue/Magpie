#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "NewProfileFlyout.g.h"


namespace winrt::Magpie::App::implementation {

struct NewProfileFlyout : NewProfileFlyoutT<NewProfileFlyout> {
	NewProfileFlyout();
};

}

namespace winrt::Magpie::App::factory_implementation {

struct NewProfileFlyout : NewProfileFlyoutT<NewProfileFlyout, implementation::NewProfileFlyout> {
};

}
