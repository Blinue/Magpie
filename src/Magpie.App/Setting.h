#pragma once

#include "Controls.Setting.g.h"


namespace winrt::Magpie::App::Controls::implementation
{
    struct Setting : SettingT<Setting>
    {
        Setting();

        //void MyHeader(const hstring& value);

        //hstring MyHeader() const;

        void Description(Windows::Foundation::IInspectable value);

        Windows::Foundation::IInspectable Description() const;

        //void Icon(Windows::Foundation::IInspectable value);

        //Windows::Foundation::IInspectable Icon() const;

        //void ActionContent(Windows::Foundation::IInspectable value);

        //Windows::Foundation::IInspectable ActionContent() const;

        void OnApplyTemplate();

        //static Windows::UI::Xaml::DependencyProperty MyHeaderProperty;
        static Windows::UI::Xaml::DependencyProperty DescriptionProperty;
        //static Windows::UI::Xaml::DependencyProperty IconProperty;
        //static Windows::UI::Xaml::DependencyProperty ActionContentProperty;

    private:
        //static void _OnMyHeaderChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);
        static void _OnDescriptionChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);
        //static void _OnIconChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);

        void _Setting_IsEnabledChanged(Windows::Foundation::IInspectable const&, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);

        void _Update();

        void _SetEnabledState();

        //Windows::UI::Xaml::Controls::ContentPresenter _iconPresenter{ nullptr };
        Windows::UI::Xaml::Controls::ContentPresenter _descriptionPresenter{ nullptr };

        Setting* _setting = nullptr;

        winrt::event_token _isEnabledChangedToken{};

        //static constexpr const wchar_t* _PartIconPresenter = L"IconPresenter";
        static constexpr const wchar_t* _PartDescriptionPresenter = L"DescriptionPresenter";
    };
}

namespace winrt::Magpie::App::Controls::factory_implementation
{
    struct Setting : SettingT<Setting, implementation::Setting>
    {
    };
}
