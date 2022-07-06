#pragma once
#include "pch.h"
#include "NewProfileDialog.g.h"


namespace winrt::Magpie::App::implementation {

struct NewProfileDialog : NewProfileDialogT<NewProfileDialog> {
    NewProfileDialog();
};

}

namespace winrt::Magpie::App::factory_implementation {

struct NewProfileDialog : NewProfileDialogT<NewProfileDialog, implementation::NewProfileDialog> {
};

}
