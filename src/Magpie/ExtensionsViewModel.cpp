#include "pch.h"
#include "ExtensionsViewModel.h"
#if __has_include("ExtensionItem.g.cpp")
#include "ExtensionItem.g.cpp"
#endif
#if __has_include("ExtensionsViewModel.g.cpp")
#include "ExtensionsViewModel.g.cpp"
#endif
#include "ExtensionService.h"

using namespace Magpie;

namespace winrt::Magpie::implementation {

hstring ExtensionItem::Name() const noexcept {
    return hstring();
}

hstring ExtensionItem::Description() const noexcept {
	return hstring();
}

bool ExtensionItem::IsEnabled() const noexcept {
	return false;
}

void ExtensionItem::IsEnabled(bool value) noexcept {
}

ExtensionsViewModel::ExtensionsViewModel() {

}

bool ExtensionsViewModel::HasExtension() const noexcept {
	return false;
}

}
