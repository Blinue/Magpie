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

PageFrame::PageFrame() {
	DefaultStyleKey(box_value(name_of<Magpie::PageFrame>()));
}

void PageFrame::OnApplyTemplate() {
	GetTemplateChild(_PartTitlePresenter).as(_titlePresenter);
	_Update();

	__super::OnApplyTemplate();
}

void PageFrame::Title(const hstring& value) {
	SetValue(TitleProperty, box_value(value));
}

hstring PageFrame::Title() const {
	return GetValue(TitleProperty).as<hstring>();
}

void PageFrame::_OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	winrt::get_self<PageFrame>(sender.as<default_interface<PageFrame>>())->_Update();
}

void PageFrame::_Update() {
	if (_titlePresenter) {
		if (Title().empty()) {
			_titlePresenter.Visibility(Visibility::Collapsed);
		} else {
			_titlePresenter.Visibility(Visibility::Visible);
		}
	}
}

}
