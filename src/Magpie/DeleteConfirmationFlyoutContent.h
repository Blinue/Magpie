#pragma once
#include "DeleteConfirmationFlyoutContent.g.h"

namespace winrt::Magpie::implementation {
    struct DeleteConfirmationFlyoutContent : DeleteConfirmationFlyoutContentT<DeleteConfirmationFlyoutContent>,
		wil::notify_property_changed_base<DeleteConfirmationFlyoutContent>
	{
		hstring Text() const { return _text; }

		void Text(hstring value);

		winrt::event_token ConfirmButtonClick(const RoutedEventHandler& handler) {
			return ConfirmButton().Click(handler);
		}

		auto ConfirmButtonClick(const winrt::event_token& token) noexcept {
			return ConfirmButton().Click(token);
		}

	private:
		hstring _text;
    };
}

BASIC_FACTORY(DeleteConfirmationFlyoutContent)
