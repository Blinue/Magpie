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

void PageFrame::InitializeComponent() {
	PageFrameT::InitializeComponent();

	_UpdateIconContainer();
	_UpdateHeaderActionPresenter();
}

void PageFrame::Title(const hstring& value) {
	if (_title == value) {
		return;
	}

	_title = value;
	RaisePropertyChanged(L"Title");
}

void PageFrame::Icon(Controls::IconElement const& value) {
	if (_icon == value) {
		return;
	}

	if (value) {
		value.Width(28);
		value.Height(28);
	}

	_icon = value;
	RaisePropertyChanged(L"Icon");

	_UpdateIconContainer();
}

void PageFrame::HeaderAction(FrameworkElement const& value) {
	if (_headerAction == value) {
		return;
	}

	_headerAction = value;
	RaisePropertyChanged(L"HeaderAction");

	_UpdateHeaderActionPresenter();
}

void PageFrame::MainContent(IInspectable const& value) {
	if (_mainContent == value) {
		return;
	}

	_mainContent = value;
	RaisePropertyChanged(L"HeaderAction");
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

void PageFrame::_UpdateIconContainer() {
	IconContainer().Visibility(_icon ? Visibility::Visible : Visibility::Collapsed);
}

void PageFrame::_UpdateHeaderActionPresenter() {
	HeaderActionPresenter().Visibility(_headerAction ? Visibility::Visible : Visibility::Collapsed);
}

}
