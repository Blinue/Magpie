#pragma once

#include "PageFrame.g.h"


namespace winrt::Magpie::implementation {

struct PageFrame : PageFrame_base<PageFrame> {
    PageFrame();

    void OnApplyTemplate();

    void Title(const hstring& value);

    hstring Title() const;

    static Windows::UI::Xaml::DependencyProperty TitleProperty;

private:
    static void _OnPropertyChanged(Windows::UI::Xaml::DependencyObject const& sender, Windows::UI::Xaml::DependencyPropertyChangedEventArgs const&);

    void _Update();

    Windows::UI::Xaml::Controls::TextBlock _titlePresenter{ nullptr };

    static constexpr const wchar_t* _PartTitlePresenter = L"TitlePresenter";
};

}

namespace winrt::Magpie::factory_implementation {

struct PageFrame : PageFrameT<PageFrame, implementation::PageFrame> {
};

}
