#pragma once
#include "ToastPage.g.h"
#include "WinRTUtils.h"
#include "AppSettings.h"

namespace winrt::Magpie::App::implementation {

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

	fire_and_forget ShowMessageOnWindow(hstring message, uint64_t hwndTarget, bool inApp = false);

	void ShowMessageInApp(hstring message);

private:
	void _AppSettings_ThemeChanged(Magpie::App::Theme theme);

	void _UISettings_ColorValuesChanged(Windows::UI::ViewManagement::UISettings const&, IInspectable const&);

	void _UpdateColorValuesChangedRevoker();

	void _UpdateTheme();

	void _IsLogoShown(bool value);

	WinRTUtils::EventRevoker _themeChangedRevoker;

	Windows::UI::ViewManagement::UISettings _uiSettings;
	Windows::UI::ViewManagement::UISettings::ColorValuesChanged_revoker _colorValuesChangedRevoker;

	Imaging::SoftwareBitmapSource _logo{ nullptr };
	HWND _hwndToast;
	MUXC::TeachingTip _oldTeachingTip{ nullptr };
	bool _isLogoShown = true;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ToastPage : ToastPageT<ToastPage, implementation::ToastPage> {
};

}
