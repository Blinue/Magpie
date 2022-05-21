#pragma once

#include "Controls.SettingsGroup.g.h"
#include "Controls.SettingsGroupAutomationPeer.g.h"


namespace winrt::Magpie::App::Controls::implementation
{
    struct SettingsGroup : SettingsGroupT<SettingsGroup>
    {
        SettingsGroup();

        void MyHeader(const hstring& value);

        hstring MyHeader() const;

        void Description(Windows::Foundation::IInspectable value);

        Windows::Foundation::IInspectable Description() const;

        void OnApplyTemplate();

        Windows::UI::Xaml::Automation::Peers::AutomationPeer OnCreateAutomationPeer();

        static Windows::UI::Xaml::DependencyProperty MyHeaderProperty;
        static Windows::UI::Xaml::DependencyProperty DescriptionProperty;

    private:
        static void _OnMyHeaderChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);
        static void _OnDescriptionChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);

        void _Setting_IsEnabledChanged(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);

        void _Update();

        void _SetEnabledState();

        Windows::UI::Xaml::Controls::TextBlock _myHeaderPresenter{ nullptr };
        Windows::UI::Xaml::Controls::ContentPresenter _descriptionPresenter{ nullptr };

        winrt::event_token _isEnabledChangedToken{};

        static constexpr const wchar_t* _PartMyHeaderPresenter = L"MyHeaderPresenter";
        static constexpr const wchar_t* _PartDescriptionPresenter = L"DescriptionPresenter";
    };

    struct SettingsGroupAutomationPeer : SettingsGroupAutomationPeerT<SettingsGroupAutomationPeer> {
        SettingsGroupAutomationPeer(Magpie::App::Controls::SettingsGroup owner);

        hstring GetNameCore();
    };
}

namespace winrt::Magpie::App::Controls::factory_implementation {

struct SettingsGroup : SettingsGroupT<SettingsGroup, implementation::SettingsGroup> {
};

struct SettingsGroupAutomationPeer : SettingsGroupAutomationPeerT<SettingsGroupAutomationPeer, implementation::SettingsGroupAutomationPeer> {
};

}
