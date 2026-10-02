#include "pch.h"
#include "DeleteConfirmationFlyoutContent.h"
#if __has_include("DeleteConfirmationFlyoutContent.g.cpp")
#include "DeleteConfirmationFlyoutContent.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::Magpie::implementation {

void DeleteConfirmationFlyoutContent::Text(hstring value) {
	if (_text == value) {
		return;
	}

	_text = std::move(value);
	RaisePropertyChanged(L"Text");
}

}
