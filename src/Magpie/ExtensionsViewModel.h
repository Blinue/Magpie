#pragma once
#include "ExtensionItem.g.h"
#include "ExtensionsViewModel.g.h"

namespace winrt::Magpie::implementation {
	struct ExtensionItem : ExtensionItemT<ExtensionItem>,
						   wil::notify_property_changed_base<ExtensionItem> {
		hstring Name() const noexcept;

		hstring Description() const noexcept;

		bool IsEnabled() const noexcept;
		void IsEnabled(bool value) noexcept;
	};

    struct ExtensionsViewModel : ExtensionsViewModelT<ExtensionsViewModel>,
								 wil::notify_property_changed_base<ExtensionsViewModel> {
		ExtensionsViewModel();

		winrt::fire_and_forget ImportDll() noexcept;

		bool HasExtension() const noexcept;

		IVector<IInspectable> Extensions() const noexcept {
			return _extensions;
		}

	private:
		IVector<IInspectable> _extensions;
    };
}
