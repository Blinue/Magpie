#pragma once
#include "pch.h"
#include "NewProfileDialog.g.h"
#include "CandidateWindow.g.h"


namespace winrt::Magpie::App::implementation {

struct CandidateWindow : CandidateWindowT<CandidateWindow> {
	CandidateWindow() = default;

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	uint64_t HWnd() const noexcept {
		return _hWnd;
	}

	void HWnd(uint64_t value) noexcept {
		_hWnd = value;
	}

	hstring Title() const noexcept {
		return _title;
	}

	void Title(const hstring& value) noexcept {
		_title = value;
	}

	IInspectable Icon() const noexcept {
		return _icon;
	}

	void Icon(const IInspectable& value) noexcept {
		_icon = value;
		_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Icon"));
	}

private:
	event<PropertyChangedEventHandler> _propertyChangedEvent;

	uint64_t _hWnd = 0;
	hstring _title;
	IInspectable _icon{ nullptr };
};

struct NewProfileDialog : NewProfileDialogT<NewProfileDialog> {
    NewProfileDialog();

	void ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&);

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	IVector<Magpie::App::CandidateWindow> CandidateWindows() const noexcept {
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

	void RootScrollViewer_SizeChanged(IInspectable const&, IInspectable const&);

private:
	event<PropertyChangedEventHandler> _propertyChangedEvent;

	IVector<Magpie::App::CandidateWindow> _candidateWindows;
	IVector<IInspectable> _profiles;
	int32_t _profileIndex = 0;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct CandidateWindow : CandidateWindowT<CandidateWindow, implementation::CandidateWindow> {
};

struct NewProfileDialog : NewProfileDialogT<NewProfileDialog, implementation::NewProfileDialog> {
};

}
