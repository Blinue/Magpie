#pragma once
#include "pch.h"
#include "MicaBrush.g.h"


namespace winrt::Magpie::implementation {

struct MicaBrush : MicaBrushT<MicaBrush> {
	MicaBrush(FrameworkElement root);

	void OnConnected();
	void OnDisconnected();

private:
	void _UpdateBrush();

	IAsyncAction _AccessibilitySettings_HighContrastChanged(
		Windows::UI::ViewManagement::AccessibilitySettings const&,
		IInspectable const&
	);
	IAsyncAction _CompositionCapabilities_Changed(
		Windows::UI::Composition::CompositionCapabilities sender,
		IInspectable const&
	);
	IAsyncAction _PowerManager_EnergySaverStatusChanged(IInspectable const&, IInspectable const&);
	void _RootElement_ActualThemeChanged(FrameworkElement const&, IInspectable const&);
	void _App_HostWndFocusedChanged(IInspectable const&, bool isFocused);

	App _app{ nullptr };

	FrameworkElement _rootElement{ nullptr };
	bool _windowActivated = false;

	Windows::UI::ViewManagement::UISettings _settings{ nullptr };
	Windows::UI::ViewManagement::AccessibilitySettings _accessibilitySettings{ nullptr };
	std::optional<bool> _fastEffects;
	std::optional<bool> _energySaver;
	bool _hasMica = false;

	winrt::event_token _highContrastChangedToken{};
	winrt::event_token _energySaverStatusChangedToken{};
	winrt::event_token _compositionCapabilitiesChangedToken{};
	winrt::event_token _rootElementThemeChangedToken{};
	winrt::event_token _hostWndFocusedChangedToken{};
};

}

namespace winrt::Magpie::factory_implementation {

struct MicaBrush : MicaBrushT<MicaBrush, implementation::MicaBrush> {
};

}
