#pragma once
#include "ExtensionsPage.g.h"
#include "ExtensionsViewModel.h"

namespace winrt::Magpie::implementation {
    struct ExtensionsPage : ExtensionsPageT<ExtensionsPage> {
		winrt::Magpie::ExtensionsViewModel ViewModel() const noexcept {
			return *_viewModel;
		}

	private:
		com_ptr<ExtensionsViewModel> _viewModel = make_self<ExtensionsViewModel>();
    };
}

BASIC_FACTORY(ExtensionsPage)
