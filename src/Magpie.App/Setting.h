#pragma once

#include "Controls.Setting.g.h"

namespace winrt::Magpie::App::Controls::implementation
{
    struct Setting : SettingT<Setting>
    {
        Setting();

        void MyHeader(const hstring& value) {

        }

        hstring MyHeader() const {
            return L"";
        }

        void Description(Windows::Foundation::IInspectable value) {

        }

        Windows::Foundation::IInspectable Description() const {
            return nullptr;
        }

        void Icon(Windows::Foundation::IInspectable value) {

        }

        Windows::Foundation::IInspectable Icon() const {
            return nullptr;
        }

        void ActionContent(Windows::Foundation::IInspectable value) {

        }

        Windows::Foundation::IInspectable ActionContent() const {
            return nullptr;
        }

        void OnApplyTemplate() {}
    };
}

namespace winrt::Magpie::App::Controls::factory_implementation
{
    struct Setting : SettingT<Setting, implementation::Setting>
    {
    };
}
