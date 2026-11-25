#pragma once
#include "ExtensionItem.g.h"
#include "ExtensionsViewModel.g.h"

namespace Magpie {
struct Extension;
}

namespace winrt::Magpie::implementation {

struct ExtensionItem : ExtensionItemT<ExtensionItem>,
					   wil::notify_property_changed_base<ExtensionItem> {
	ExtensionItem(std::string_view extId, const ::Magpie::Extension* data)
		: _extId(extId), _data(data) {}

	hstring Name() const noexcept;

	hstring Description() const noexcept;

	bool IsEnabled() const noexcept;
	void IsEnabled(bool value) noexcept;

private:
	const std::string _extId;
	// 和缩放模式不同，Extension 的地址稳定，可以保存指针
	const ::Magpie::Extension* _data = 0;
};

struct ExtensionsViewModel : ExtensionsViewModelT<ExtensionsViewModel>,
							 wil::notify_property_changed_base<ExtensionsViewModel> {
	ExtensionsViewModel();

	winrt::fire_and_forget ImportExtension() noexcept;

	bool HasExtension() const noexcept;

	IVector<IInspectable> Extensions() const noexcept {
		return _extensions;
	}

private:
	IObservableVector<IInspectable> _extensions{ nullptr };
};

}
