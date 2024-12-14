﻿#pragma once
#include "ToastPage.g.h"
#include "WinRTHelper.h"
#include "AppSettings.h"

namespace winrt::Magpie::implementation {

struct ToastPage : ToastPageT<ToastPage>,
                   wil::notify_property_changed_base<ToastPage> {
	ToastPage(uint64_t hwndToast);

	void InitializeComponent();

	Imaging::SoftwareBitmapSource Logo() const noexcept {
		return _logo;
	}

	bool IsLogoShown() const noexcept {
		return _isLogoShown;
	}

	fire_and_forget ShowMessageOnWindow(hstring title, hstring message, uint64_t hwndTarget, bool inApp = false);

	void ShowMessageInApp(hstring title, hstring message);

private:
	void _AppSettings_ThemeChanged(::Magpie::AppTheme theme);

	void _UISettings_ColorValuesChanged(Windows::UI::ViewManagement::UISettings const&, IInspectable const&);

	void _UpdateColorValuesChangedRevoker();

	void _UpdateTheme();

	void _IsLogoShown(bool value);

	WinRTHelper::EventRevoker _themeChangedRevoker;

	Windows::UI::ViewManagement::UISettings _uiSettings;
	Windows::UI::ViewManagement::UISettings::ColorValuesChanged_revoker _colorValuesChangedRevoker;

	Imaging::SoftwareBitmapSource _logo{ nullptr };
	HWND _hwndToast;
	MUXC::TeachingTip _oldTeachingTip{ nullptr };
	bool _isLogoShown = true;
};

}

namespace winrt::Magpie::factory_implementation {

struct ToastPage : ToastPageT<ToastPage, implementation::ToastPage> {
};

}
