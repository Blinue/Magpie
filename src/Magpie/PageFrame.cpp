#include "pch.h"
#include "PageFrame.h"
#if __has_include("PageFrame.g.cpp")
#include "PageFrame.g.cpp"
#endif
#include "App.h"
#include "XamlHelper.h"

using namespace ::Magpie;
using namespace winrt;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Text;

namespace winrt::Magpie::implementation {

DependencyProperty PageFrame::_titleProperty{ nullptr };
DependencyProperty PageFrame::_iconProperty{ nullptr };
DependencyProperty PageFrame::_headerActionProperty{ nullptr };
DependencyProperty PageFrame::_mainContentProperty{ nullptr };

void PageFrame::InitializeComponent() {
	_RegisterDependencyProperties();

	PageFrameT::InitializeComponent();

	_OnIconContainerChanged();
	_OnHeaderActionChanged();
}

void PageFrame::Loaded(IInspectable const&, RoutedEventArgs const&) {
	// Win10 中更新 ToolTip 的主题
	XamlHelper::UpdateThemeOfTooltips(*this, App::Get().IsDarkTheme() ? ElementTheme::Dark : ElementTheme::Light);
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
	auto scrollViewer = sender.try_as<struct ScrollViewer>();
	switch (args.Key()) {
	case VirtualKey::Up:
		scrollViewer.ChangeView(scrollViewer.HorizontalOffset(), scrollViewer.VerticalOffset() - 100, 1);
		break;
	case VirtualKey::Down:
		scrollViewer.ChangeView(scrollViewer.HorizontalOffset(), scrollViewer.VerticalOffset() + 100, 1);
		break;
	default:
		break;
	}
}

void PageFrame::_RegisterDependencyProperties() {
	if (_titleProperty) {
		return;
	}

	_titleProperty = DependencyProperty::Register(
		L"Title",
		xaml_typename<hstring>(),
		xaml_typename<class_type>(),
		nullptr
	);

	_iconProperty = DependencyProperty::Register(
		L"Icon",
		xaml_typename<IconElement>(),
		xaml_typename<class_type>(),
		PropertyMetadata(nullptr, [](DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
			get_self<PageFrame>(sender.try_as<class_type>())->_OnIconContainerChanged();
		})
	);

	_headerActionProperty = DependencyProperty::Register(
		L"HeaderAction",
		xaml_typename<FrameworkElement>(),
		xaml_typename<class_type>(),
		PropertyMetadata(nullptr, [](DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
			get_self<PageFrame>(sender.try_as<class_type>())->_OnHeaderActionChanged();
		})
	);

	_mainContentProperty = DependencyProperty::Register(
		L"MainContent",
		xaml_typename<IInspectable>(),
		xaml_typename<class_type>(),
		PropertyMetadata(nullptr, [](DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
			get_self<PageFrame>(sender.try_as<class_type>())->_OnMainContentChanged();
		})
	);
}

void PageFrame::_OnIconContainerChanged() {
	IconContainer().Visibility(Icon() ? Visibility::Visible : Visibility::Collapsed);
}

void PageFrame::_OnHeaderActionChanged() {
	HeaderActionPresenter().Visibility(HeaderAction() ? Visibility::Visible : Visibility::Collapsed);
}

void PageFrame::_OnMainContentChanged() {
	static double pageMaxWidth =
		Resources().TryLookup(box_value(L"PageMaxWidth")).try_as<double>().value();

	// 如果在 ContentControl 上设置 MaxWidth，如果页面根元素是 Grid 可能会有布局错误
	if (FrameworkElement content = MainContent().try_as<FrameworkElement>()) {
		content.MaxWidth(pageMaxWidth);
	}
}

}
