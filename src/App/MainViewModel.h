#pragma once
#include "MainViewModel.g.h"

namespace winrt::Magpie::App::implementation {

struct MainViewModel : MainViewModelT<MainViewModel> {
    MainViewModel() = default;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct MainViewModel : MainViewModelT<MainViewModel, implementation::MainViewModel> {
};

}
