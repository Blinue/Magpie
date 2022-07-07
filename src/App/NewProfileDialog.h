#pragma once
#include "pch.h"
#include "NewProfileDialog.g.h"


namespace winrt::Magpie::App::implementation {

struct NewProfileDialog : NewProfileDialogT<NewProfileDialog> {
    NewProfileDialog();

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	IVector<IInspectable> CandidateWindows() const noexcept {
		return _candidateWindows;
	}

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

private:
	event<PropertyChangedEventHandler> _propertyChangedEvent;

	IVector<IInspectable> _candidateWindows;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct NewProfileDialog : NewProfileDialogT<NewProfileDialog, implementation::NewProfileDialog> {
};

}
