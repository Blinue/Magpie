#pragma once
#include "ExtensionsViewModel.g.h"

namespace winrt::Magpie::implementation {
    struct ExtensionsViewModel : ExtensionsViewModelT<ExtensionsViewModel>,
								wil::notify_property_changed_base<ExtensionsViewModel> {
		IVector<IInspectable> Extensions() const noexcept {
			return _extensions;
		}

	private:
		IVector<IInspectable> _extensions;
    };
}
