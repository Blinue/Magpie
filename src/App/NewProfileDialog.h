#pragma once
#include "pch.h"
#include "NewProfileDialog.g.h"
#include "CandidateWindow.g.h"


namespace winrt::Magpie::App::implementation {

struct CandidateWindow : CandidateWindowT<CandidateWindow> {
	CandidateWindow(uint64_t hWnd, uint32_t dpi, bool isLightTheme, CoreDispatcher const& dispatcher);

	CandidateWindow(Magpie::App::CandidateWindow const& other, int);

	void UpdateTitle();

	void UpdateIcon();

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	uint64_t HWnd() const noexcept {
		return _hWnd;
	}

	hstring Title() const noexcept {
		return _title;
	}

	IInspectable Icon() const noexcept {
		return _icon;
	}

	hstring DefaultProfileName() const noexcept {
		return _defaultProfileName;
	}

	hstring AUMID() const noexcept {
		return _aumid;
	}

	void OnThemeChanged(bool isLightTheme);

	void OnAccentColorChanged();

	void OnDpiChanged(uint32_t newDpi);

private:
	void _SetDefaultIcon();

	IAsyncAction _SetSoftwareBitmapIconAsync(Windows::Graphics::Imaging::SoftwareBitmap const& iconBitmap);

	void _SetIconPath(std::wstring_view iconPath);

	fire_and_forget _ResolveWindow(bool resolveIcon, bool resolveName);

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	hstring _aumid;
	uint64_t _hWnd = 0;
	hstring _title;
	IInspectable _icon{ nullptr };
	hstring _defaultProfileName;

	bool _isLightTheme = false;
	uint32_t _dpi = 96;
	CoreDispatcher _dispatcher{ nullptr };
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

	void Loading(FrameworkElement const&, IInspectable const&);

	void ActualThemeChanged(FrameworkElement const&, IInspectable const&);

	void CandidateWindowsListView_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&);

	void ProfileNameTextBox_TextChanged(IInspectable const&, Controls::TextChangedEventArgs const&);

private:
	void _ContentDialog_PrimaryButtonClick(Controls::ContentDialog const&, Controls::ContentDialogButtonClickEventArgs const&);

	IAsyncAction _DisplayInformation_DpiChanged(Windows::Graphics::Display::DisplayInformation const&, IInspectable const&);

	IAsyncAction _UISettings_ColorValuesChanged(Windows::UI::ViewManagement::UISettings const&, IInspectable const&);

	fire_and_forget _UpdateCandidateWindows();

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	IVector<Magpie::App::CandidateWindow> _candidateWindows;

	IVector<IInspectable> _profiles;
	int32_t _profileIndex = 0;

	Windows::Graphics::Display::DisplayInformation _displayInfomation{ nullptr };
	Windows::Graphics::Display::DisplayInformation::DpiChanged_revoker _dpiChangedRevoker;
	Windows::UI::ViewManagement::UISettings _uiSettings;
	Windows::UI::ViewManagement::UISettings::ColorValuesChanged_revoker _colorValuesChangedRevoker;

	Controls::ContentDialog _parent{ nullptr };
	Controls::ContentDialog::PrimaryButtonClick_revoker _primaryButtonClickRevoker;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct CandidateWindow : CandidateWindowT<CandidateWindow, implementation::CandidateWindow> {
};

struct NewProfileDialog : NewProfileDialogT<NewProfileDialog, implementation::NewProfileDialog> {
};

}
