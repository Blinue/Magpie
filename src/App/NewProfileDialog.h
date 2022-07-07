#pragma once
#include "pch.h"
#include "NewProfileDialog.g.h"


namespace winrt::Magpie::App::implementation {

struct NewProfileDialog : NewProfileDialogT<NewProfileDialog> {
    NewProfileDialog();

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	IVector<IInspectable> CandidateWindows() const noexcept {
		return _candidateWindows;
	}

	IVector<IInspectable> Profiles() const noexcept {
		return _profiles;
	}

	int32_t ProfileIndex() const noexcept {
		return _profileIndex;
	}

	void ProfileIndex(int32_t value) {
		_profileIndex = value;
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ProfileIndex"));
	}

private:
	event<PropertyChangedEventHandler> _propertyChangedEvent;

	IVector<IInspectable> _candidateWindows;
	IVector<IInspectable> _profiles;
	int32_t _profileIndex = 0;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct NewProfileDialog : NewProfileDialogT<NewProfileDialog, implementation::NewProfileDialog> {
};

}
