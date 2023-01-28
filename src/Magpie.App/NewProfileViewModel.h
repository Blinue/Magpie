#pragma once
#include "NewProfileViewModel.g.h"

namespace winrt::Magpie::App::implementation {

struct NewProfileViewModel : NewProfileViewModelT<NewProfileViewModel> {
	NewProfileViewModel() = default;

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	void PrepareForOpen(uint32_t dpi, bool isLightTheme, CoreDispatcher const& dispatcher);

	IVector<IInspectable> CandidateWindows() const noexcept {
		return _candidateWindows;
	}

	int CandidateWindowIndex() const noexcept {
		return _candidateWindowIndex;
	}

	void CandidateWindowIndex(int value);

	hstring Name() const noexcept {
		return _name;
	}

	void Name(const hstring& value) noexcept;

	IVector<IInspectable> Profiles() const noexcept {
		return _profiles;
	}

	int ProfileIndex() const noexcept {
		return _profileIndex;
	}

	void ProfileIndex(int value) {
		_profileIndex = value;
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"ProfileIndex"));
	}

	bool IsConfirmButtonEnabled() const noexcept {
		return _isConfirmButtonEnabled;
	}

	bool IsNoCandidateWindow() const noexcept {
		return !_candidateWindows || _candidateWindows.Size() == 0;
	}

	bool IsAnyCandidateWindow() const noexcept {
		return _candidateWindows && _candidateWindows.Size() != 0;
	}

	bool IsNotRunningAsAdmin() const noexcept;

	void Confirm() const noexcept;

private:
	void _IsConfirmButtonEnabled(bool value) noexcept {
		if (_isConfirmButtonEnabled == value) {
			return;
		}

		_isConfirmButtonEnabled = value;
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsConfirmButtonEnabled"));
	}

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	IVector<IInspectable> _candidateWindows{ nullptr };
	int _candidateWindowIndex = -1;
	hstring _name;
	IVector<IInspectable> _profiles{ nullptr };
	int _profileIndex = 0;
	bool _isConfirmButtonEnabled = false;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct NewProfileViewModel : NewProfileViewModelT<NewProfileViewModel, implementation::NewProfileViewModel> {
};

}
