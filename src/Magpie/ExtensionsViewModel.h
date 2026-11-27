#pragma once
#include "ExtensionItem.g.h"
#include "ExtensionsViewModel.g.h"
#include "Event.h"

namespace Magpie {
struct Extension;
}

namespace winrt::Magpie::implementation {

struct ExtensionItem : ExtensionItemT<ExtensionItem>,
					   wil::notify_property_changed_base<ExtensionItem> {
	ExtensionItem(std::string_view extId, const ::Magpie::Extension* data)
		: _extId(extId), _data(data)
	{
		assert(!extId.empty() && data);
	}

	hstring Name() const noexcept;

	hstring Description() const noexcept;

	bool IsEnabled() const noexcept;
	void IsEnabled(bool value) noexcept;

	Magpie::InfoGridData InfoData() const noexcept;

	void Remove() noexcept;

private:
	bool _IsRemoved() const noexcept;

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
	void _ExtensionService_Added(uint32_t extIdx) noexcept;

	void _ExtensionService_Removed(uint32_t extIdx) noexcept;

	IObservableVector<IInspectable> _extensions{ nullptr };

	::Magpie::Event<uint32_t>::EventRevoker _extensionAddedRevoker;
	::Magpie::Event<uint32_t>::EventRevoker _extensionRemovedRevoker;
};

}
