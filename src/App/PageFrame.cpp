#include "pch.h"
#include "PageFrame.h"
#if __has_include("PageFrame.g.cpp")
#include "PageFrame.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;


namespace winrt::Magpie::implementation {

DependencyProperty PageFrame::TitleProperty = DependencyProperty::Register(
	L"Title",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::PageFrame>(),
	PropertyMetadata(box_value(L""), &PageFrame::_OnPropertyChanged)
);

DependencyProperty PageFrame::MainContentProperty = DependencyProperty::Register(
	L"MainContent",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::PageFrame>(),
	PropertyMetadata(nullptr, &PageFrame::_OnPropertyChanged)
);


PageFrame::PageFrame() {
	InitializeComponent();
}

void PageFrame::Title(const hstring& value) {
	SetValue(TitleProperty, box_value(value));
}

hstring PageFrame::Title() const {
	return GetValue(TitleProperty).as<hstring>();
}

void PageFrame::MainContent(IInspectable value) {
	SetValue(MainContentProperty, box_value(value));
}

IInspectable PageFrame::MainContent() const {
	return GetValue(MainContentProperty).as<IInspectable>();
}

void PageFrame::_OnPropertyChanged(DependencyObject const&, DependencyPropertyChangedEventArgs const&) {

}

}
