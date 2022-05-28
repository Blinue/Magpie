#pragma once

#include "SettingsGroup.g.h"
#include "SettingsGroupAutomationPeer.g.h"


namespace winrt::Magpie::implementation
{
	struct SettingsGroup : SettingsGroup_base<SettingsGroup>
	{
		SettingsGroup();

		void MyHeader(const hstring& value);

		hstring MyHeader() const;

		void Description(Windows::Foundation::IInspectable value);

		Windows::Foundation::IInspectable Description() const;

		void OnApplyTemplate();

		Windows::UI::Xaml::Automation::Peers::AutomationPeer OnCreateAutomationPeer();

		static const Windows::UI::Xaml::DependencyProperty MyHeaderProperty;
		static const Windows::UI::Xaml::DependencyProperty DescriptionProperty;

	private:
		static void _OnMyHeaderChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);
		static void _OnDescriptionChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);

		void _Setting_IsEnabledChanged(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);

		void _Update();

		void _SetEnabledState();

		Windows::UI::Xaml::Controls::TextBlock _myHeaderPresenter{ nullptr };
		Windows::UI::Xaml::Controls::ContentPresenter _descriptionPresenter{ nullptr };

		event_token _isEnabledChangedToken{};

		static constexpr const wchar_t* _PartMyHeaderPresenter = L"MyHeaderPresenter";
		static constexpr const wchar_t* _PartDescriptionPresenter = L"DescriptionPresenter";
	};

	struct SettingsGroupAutomationPeer : SettingsGroupAutomationPeerT<SettingsGroupAutomationPeer> {
		SettingsGroupAutomationPeer(Magpie::SettingsGroup owner);

		hstring GetNameCore();
	};
}

namespace winrt::Magpie::factory_implementation {

struct SettingsGroup : SettingsGroupT<SettingsGroup, implementation::SettingsGroup> {
};

struct SettingsGroupAutomationPeer : SettingsGroupAutomationPeerT<SettingsGroupAutomationPeer, implementation::SettingsGroupAutomationPeer> {
};

}
