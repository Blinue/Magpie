#include "pch.h"
#include "PageFrame.h"
#if __has_include("PageFrame.g.cpp")
#include "PageFrame.g.cpp"
#endif
#include "XamlHelper.h"
#include "App.h"

using namespace ::Magpie;
using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Text;

namespace winrt::Magpie::implementation {

const DependencyProperty PageFrame::TitleProperty = DependencyProperty::Register(
	L"Title",
	xaml_typename<hstring>(),
	xaml_typename<class_type>(),
	PropertyMetadata(box_value(L""), &PageFrame::_OnTitleChanged)
);

const DependencyProperty PageFrame::IconProperty = DependencyProperty::Register(
	L"Icon",
	xaml_typename<IconElement>(),
	xaml_typename<class_type>(),
	PropertyMetadata(nullptr, &PageFrame::_OnIconChanged)
);

const DependencyProperty PageFrame::HeaderActionProperty = DependencyProperty::Register(
	L"HeaderAction",
	xaml_typename<FrameworkElement>(),
	xaml_typename<class_type>(),
	PropertyMetadata(nullptr, &PageFrame::_OnHeaderActionChanged)
);

const DependencyProperty PageFrame::MainContentProperty = DependencyProperty::Register(
	L"MainContent",
	xaml_typename<IInspectable>(),
	xaml_typename<class_type>(),
	PropertyMetadata(nullptr, &PageFrame::_OnMainContentChanged)
);

void PageFrame::Loading(FrameworkElement const&, IInspectable const&) {
	_Update();
}

void PageFrame::Loaded(IInspectable const&, RoutedEventArgs const&) {
	// Win10 中更新 ToolTip 的主题
	XamlHelper::UpdateThemeOfTooltips(*this, App::Get().RootPage().ActualTheme());
}

void PageFrame::SizeChanged(IInspectable const&, SizeChangedEventArgs const& e) {
	// 根据尺寸调整边距
	const double marginWidth = e.NewSize().Width > 590 ? 40 : 25;

	{
		auto headerGrid = HeaderGrid();
		Thickness margin = headerGrid.Margin();
		margin.Left = marginWidth;
		margin.Right = marginWidth;
		headerGrid.Margin(margin);
	}
	{
		auto scrollViewer = this->ScrollViewer();
		Thickness padding = scrollViewer.Padding();
		padding.Left = marginWidth;
		padding.Right = marginWidth;
		scrollViewer.Padding(padding);
	}
}

void PageFrame::ScrollViewer_PointerPressed(IInspectable const&, PointerRoutedEventArgs const&) {
	XamlHelper::CloseComboBoxPopup(XamlRoot());
}

void PageFrame::ScrollViewer_ViewChanging(IInspectable const&, ScrollViewerViewChangingEventArgs const&) {
	XamlHelper::CloseComboBoxPopup(XamlRoot());
}

void PageFrame::ScrollViewer_KeyDown(IInspectable const& sender, KeyRoutedEventArgs const& args) {
	auto scrollViewer = sender.as<Controls::ScrollViewer>();
	switch (args.Key()) {
	case VirtualKey::Up:
		scrollViewer.ChangeView(scrollViewer.HorizontalOffset(), scrollViewer.VerticalOffset() - 100, 1);
		break;
	case VirtualKey::Down:
		scrollViewer.ChangeView(scrollViewer.HorizontalOffset(), scrollViewer.VerticalOffset() + 100, 1);
		break;
	}
}

void PageFrame::_Update() {
	HeaderActionPresenter().Visibility(HeaderAction() ? Visibility::Visible : Visibility::Collapsed);

	IconElement icon = Icon();
	if (icon) {
		icon.Width(28);
		icon.Height(28);
	}

	IconContainer().Visibility(icon ? Visibility::Visible : Visibility::Collapsed);
}

void PageFrame::_OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	PageFrame* that = get_self<PageFrame>(sender.as<class_type>());
	that->_Update();
	that->RaisePropertyChanged(L"Title");
}

void PageFrame::_OnIconChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	PageFrame* that = get_self<PageFrame>(sender.as<class_type>());
	that->_Update();
	that->RaisePropertyChanged(L"Icon");
}

void PageFrame::_OnHeaderActionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	PageFrame* that = get_self<PageFrame>(sender.as<class_type>());
	that->_Update();
	that->RaisePropertyChanged(L"HeaderAction");
}

void PageFrame::_OnMainContentChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	PageFrame* that = get_self<PageFrame>(sender.as<class_type>());
	that->RaisePropertyChanged(L"MainContent");
}

}
