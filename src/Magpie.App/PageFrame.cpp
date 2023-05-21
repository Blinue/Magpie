#include "pch.h"
#include "PageFrame.h"
#if __has_include("PageFrame.g.cpp")
#include "PageFrame.g.cpp"
#endif
#include "XamlUtils.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Text;

namespace winrt::Magpie::App::implementation {

const DependencyProperty PageFrame::TitleProperty = DependencyProperty::Register(
	L"Title",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::App::PageFrame>(),
	PropertyMetadata(box_value(L""), &PageFrame::_OnTitleChanged)
);

const DependencyProperty PageFrame::IconProperty = DependencyProperty::Register(
	L"Icon",
	xaml_typename<IconElement>(),
	xaml_typename<Magpie::App::PageFrame>(),
	PropertyMetadata(nullptr, &PageFrame::_OnIconChanged)
);

const DependencyProperty PageFrame::HeaderActionProperty = DependencyProperty::Register(
	L"HeaderAction",
	xaml_typename<FrameworkElement>(),
	xaml_typename<Magpie::App::PageFrame>(),
	PropertyMetadata(nullptr, &PageFrame::_OnHeaderActionChanged)
);

const DependencyProperty PageFrame::MainContentProperty = DependencyProperty::Register(
	L"MainContent",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::PageFrame>(),
	PropertyMetadata(nullptr, &PageFrame::_OnMainContentChanged)
);

void PageFrame::Loading(FrameworkElement const&, IInspectable const&) {
	_Update();

	MainPage mainPage = XamlRoot().Content().as<Magpie::App::MainPage>();
	_rootNavigationView = mainPage.RootNavigationView();
	_displayModeChangedRevoker = _rootNavigationView.DisplayModeChanged(
		auto_revoke,
		[&](auto const&, auto const&) { _UpdateHeaderStyle(); }
	);
	_UpdateHeaderStyle();
}

void PageFrame::Loaded(IInspectable const&, RoutedEventArgs const&) {
	// Win10 中更新 ToolTip 的主题
	XamlUtils::UpdateThemeOfTooltips(*this, Application::Current().as<App>().MainPage().ActualTheme());
}

void PageFrame::ScrollViewer_PointerPressed(IInspectable const&, PointerRoutedEventArgs const&) {
	XamlUtils::CloseXamlPopups(XamlRoot());
}

void PageFrame::ScrollViewer_ViewChanging(IInspectable const&, ScrollViewerViewChangingEventArgs const&) {
	XamlUtils::CloseXamlPopups(XamlRoot());
}

void PageFrame::_Update() {
	HeaderActionPresenter().Visibility(HeaderAction() ? Visibility::Visible : Visibility::Collapsed);

	if (_rootNavigationView) {
		_UpdateHeaderStyle();
	}
}

void PageFrame::_UpdateHeaderStyle() {
	IconElement icon = Icon();
	if (icon) {
		icon.Width(28);
		icon.Height(28);
	}

	if (_rootNavigationView.DisplayMode() == MUXC::NavigationViewDisplayMode::Minimal) {
		IconContainer().Visibility(Visibility::Collapsed);
	} else {
		IconContainer().Visibility(icon ? Visibility::Visible : Visibility::Collapsed);
	}
}

void PageFrame::_OnTitleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	PageFrame* that = get_self<PageFrame>(sender.as<default_interface<PageFrame>>());
	that->_Update();
	that->_propertyChangedEvent(*that, PropertyChangedEventArgs{ L"Title" });
}

void PageFrame::_OnIconChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	PageFrame* that = get_self<PageFrame>(sender.as<default_interface<PageFrame>>());
	that->_Update();
	that->_propertyChangedEvent(*that, PropertyChangedEventArgs{ L"Icon" });
}

void PageFrame::_OnHeaderActionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	PageFrame* that = get_self<PageFrame>(sender.as<default_interface<PageFrame>>());
	that->_Update();
	that->_propertyChangedEvent(*that, PropertyChangedEventArgs{ L"HeaderAction" });
}

void PageFrame::_OnMainContentChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	PageFrame* that = get_self<PageFrame>(sender.as<default_interface<PageFrame>>());
	that->_propertyChangedEvent(*that, PropertyChangedEventArgs{ L"MainContent" });
}

}
