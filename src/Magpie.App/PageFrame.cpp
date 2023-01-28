#include "pch.h"
#include "PageFrame.h"
#if __has_include("PageFrame.g.cpp")
#include "PageFrame.g.cpp"
#endif
#include "XamlUtils.h"
#include "Win32Utils.h"

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


PageFrame::PageFrame() {
	InitializeComponent();
}

void PageFrame::Title(const hstring& value) {
	SetValue(TitleProperty, box_value(value));
}

hstring PageFrame::Title() const {
	return GetValue(TitleProperty).as<hstring>();
}

void PageFrame::Icon(IconElement const& value) {
	SetValue(IconProperty, value);
}

IconElement PageFrame::Icon() const {
	return GetValue(IconProperty).as<IconElement>();
}

void PageFrame::HeaderAction(FrameworkElement const& value) {
	SetValue(HeaderActionProperty, value);
}

FrameworkElement PageFrame::HeaderAction() const {
	return GetValue(HeaderActionProperty).as<FrameworkElement>();
}

void PageFrame::MainContent(IInspectable const& value) {
	SetValue(MainContentProperty, value);
}

IInspectable PageFrame::MainContent() const {
	return GetValue(MainContentProperty).as<IInspectable>();
}

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

event_token PageFrame::PropertyChanged(PropertyChangedEventHandler const& value) {
	return _propertyChangedEvent.add(value);
}

void PageFrame::PropertyChanged(event_token const& token) {
	_propertyChangedEvent.remove(token);
}

void PageFrame::_Update() {
	TitleTextBlock().Visibility(Title().empty() ? Visibility::Collapsed : Visibility::Visible);
	HeaderActionPresenter().Visibility(HeaderAction() ? Visibility::Visible : Visibility::Collapsed);

	if (_rootNavigationView) {
		_UpdateHeaderStyle();
	}
}

void PageFrame::_UpdateHeaderStyle() {
	TextBlock textBlock = TitleTextBlock();

	IconElement icon = Icon();
	if (icon) {
		icon.Width(28);
		icon.Height(28);
	}

	if (_rootNavigationView.DisplayMode() == MUXC::NavigationViewDisplayMode::Minimal) {
		HeaderGrid().Margin({ 28, 8, 0, 0 });
		IconContainer().Visibility(Visibility::Collapsed);
		
		textBlock.FontSize(20);
	} else {
		HeaderGrid().Margin({ 0, Win32Utils::GetOSVersion().Is22H2OrNewer() ? 22.0 : 41.5, 0, 0});
		IconContainer().Visibility(icon ? Visibility::Visible : Visibility::Collapsed);
		
		textBlock.FontSize(30);
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
