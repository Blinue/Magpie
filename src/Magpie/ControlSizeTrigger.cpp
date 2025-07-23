// 移植自 https://github.com/CommunityToolkit/Windows/blob/bef863ca70bb1edf8c940198dd5cc74afa5d2aab/components/Triggers/src/ControlSizeTrigger.cs

#include "pch.h"
#include "ControlSizeTrigger.h"
#if __has_include("ControlSizeTrigger.g.cpp")
#include "ControlSizeTrigger.g.cpp"
#endif

namespace winrt::Magpie::implementation {

DependencyProperty ControlSizeTrigger::_canTriggerProperty{ nullptr };
DependencyProperty ControlSizeTrigger::_maxWidthProperty{ nullptr };
DependencyProperty ControlSizeTrigger::_minWidthProperty{ nullptr };
DependencyProperty ControlSizeTrigger::_maxHeightProperty{ nullptr };
DependencyProperty ControlSizeTrigger::_minHeightProperty{ nullptr };
DependencyProperty ControlSizeTrigger::_targetElementProperty{ nullptr };

void ControlSizeTrigger::RegisterDependencyProperties() {
	_canTriggerProperty = DependencyProperty::Register(
		L"CanTrigger",
		xaml_typename<bool>(),
		xaml_typename<Magpie::ControlSizeTrigger>(),
		PropertyMetadata(box_value(true), &ControlSizeTrigger::_OnPropertyChanged)
	);

	_maxWidthProperty = DependencyProperty::Register(
		L"MaxWidth",
		xaml_typename<double>(),
		xaml_typename<Magpie::ControlSizeTrigger>(),
		PropertyMetadata(box_value(std::numeric_limits<double>::infinity()), &ControlSizeTrigger::_OnPropertyChanged)
	);

	_minWidthProperty = DependencyProperty::Register(
		L"MinWidth",
		xaml_typename<double>(),
		xaml_typename<Magpie::ControlSizeTrigger>(),
		PropertyMetadata(box_value(0.0), &ControlSizeTrigger::_OnPropertyChanged)
	);

	_maxHeightProperty = DependencyProperty::Register(
		L"MaxHeight",
		xaml_typename<double>(),
		xaml_typename<Magpie::ControlSizeTrigger>(),
		PropertyMetadata(box_value(std::numeric_limits<double>::infinity()), &ControlSizeTrigger::_OnPropertyChanged)
	);

	_minHeightProperty = DependencyProperty::Register(
		L"MinHeight",
		xaml_typename<double>(),
		xaml_typename<Magpie::ControlSizeTrigger>(),
		PropertyMetadata(box_value(0.0), &ControlSizeTrigger::_OnPropertyChanged)
	);

	_targetElementProperty = DependencyProperty::Register(
		L"TargetElement",
		xaml_typename<FrameworkElement>(),
		xaml_typename<Magpie::ControlSizeTrigger>(),
		PropertyMetadata(nullptr, &ControlSizeTrigger::_OnTargetElementChanged)
	);
}

void ControlSizeTrigger::_OnPropertyChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<ControlSizeTrigger>(sender.try_as<Magpie::ControlSizeTrigger>())->_UpdateTrigger();
}

void ControlSizeTrigger::_OnTargetElementChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const& e) {
	ControlSizeTrigger* that = get_self<ControlSizeTrigger>(sender.try_as<Magpie::ControlSizeTrigger>());

	that->_targetElementSizeChangedRevoker.revoke();

	if (IInspectable newValue = e.NewValue()) {
		that->_targetElementSizeChangedRevoker = newValue.try_as<FrameworkElement>().SizeChanged(auto_revoke,
			[that](IInspectable const&, SizeChangedEventArgs const&) {
				that->_UpdateTrigger();
			}
		);
	}

	that->_UpdateTrigger();
}

void ControlSizeTrigger::_UpdateTrigger() {
	const FrameworkElement targetElement = TargetElement();

	if (!targetElement || !CanTrigger()) {
		SetActive(false);
		return;
	}

	const double actualWidth = targetElement.ActualWidth();
	const double actualHeight = targetElement.ActualHeight();
	SetActive(
		actualWidth >= MinWidth() &&
		actualWidth < MaxWidth() &&
		actualHeight >= MinHeight() &&
		actualHeight < MaxHeight()
	);
}

}
