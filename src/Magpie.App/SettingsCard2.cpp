#include "pch.h"
#include "SettingsCard2.h"
#if __has_include("SettingsCard2.g.cpp")
#include "SettingsCard2.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::Magpie::App::implementation {

const wchar_t* CommonStates = L"CommonStates";
const wchar_t* NormalState = L"Normal";
const wchar_t* PointerOverState = L"PointerOver";
const wchar_t* PressedState = L"Pressed";
const wchar_t* DisabledState = L"Disabled";

const wchar_t* ContentAlignmentStates = L"ContentAlignmentStates";
const wchar_t* RightState = L"Right";
const wchar_t* RightWrappedState = L"RightWrapped";
const wchar_t* RightWrappedNoIconState = L"RightWrappedNoIcon";
const wchar_t* LeftState = L"Left";
const wchar_t* VerticalState = L"Vertical";

const wchar_t* ContentSpacingStates = L"ContentSpacingStates";
const wchar_t* NoContentSpacingState = L"NoContentSpacing";
const wchar_t* ContentSpacingState = L"ContentSpacing";

const wchar_t* ActionIconPresenterHolder = L"PART_ActionIconPresenterHolder";
const wchar_t* HeaderPresenter = L"PART_HeaderPresenter";
const wchar_t* DescriptionPresenter = L"PART_DescriptionPresenter";
const wchar_t* HeaderIconPresenterHolder = L"PART_HeaderIconPresenterHolder";

const DependencyProperty SettingsCard2::HeaderProperty = DependencyProperty::Register(
	L"Header",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	PropertyMetadata(nullptr, &SettingsCard2::_OnHeaderChanged)
);

const DependencyProperty SettingsCard2::DescriptionProperty = DependencyProperty::Register(
	L"Description",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	PropertyMetadata(nullptr, &SettingsCard2::_OnDescriptionChanged)
);

const DependencyProperty SettingsCard2::HeaderIconProperty = DependencyProperty::Register(
	L"HeaderIcon",
	xaml_typename<Controls::IconElement>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	PropertyMetadata(nullptr, &SettingsCard2::_OnHeaderIconChanged)
);

const DependencyProperty SettingsCard2::ActionIconProperty = DependencyProperty::Register(
	L"ActionIcon",
	xaml_typename<Controls::IconElement>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	PropertyMetadata(box_value(L"\ue974"))
);

const DependencyProperty SettingsCard2::ActionIconToolTipProperty = DependencyProperty::Register(
	L"ActionIconToolTip",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	nullptr
);

const DependencyProperty SettingsCard2::IsClickEnabledProperty = DependencyProperty::Register(
	L"IsClickEnabled",
	xaml_typename<bool>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	PropertyMetadata(box_value(false), &SettingsCard2::_OnIsClickEnabledChanged)
);

const DependencyProperty SettingsCard2::ContentAlignmentProperty = DependencyProperty::Register(
	L"ContentAlignment",
	xaml_typename<Magpie::App::ContentAlignment>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	PropertyMetadata(box_value(ContentAlignment::Right))
);

const DependencyProperty SettingsCard2::IsActionIconVisibleProperty = DependencyProperty::Register(
	L"IsActionIconVisible",
	xaml_typename<bool>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	PropertyMetadata(box_value(true), &SettingsCard2::_OnIsActionIconVisibleChanged)
);

SettingsCard2::SettingsCard2() {
	DefaultStyleKey(box_value(GetRuntimeClassName()));
}

void SettingsCard2::OnApplyTemplate() {
	SettingsCard2_base<SettingsCard2>::OnApplyTemplate();

	//IsEnabledChanged() -= OnIsEnabledChanged;
	_OnActionIconChanged();
	_OnHeaderChanged();
	_OnHeaderIconChanged();
	_OnDescriptionChanged();
	_OnIsClickEnabledChanged();
	_CheckInitialVisualState();

	//RegisterPropertyChangedCallback(ContentProperty, OnContentChanged);
	//IsEnabledChanged += OnIsEnabledChanged;
}

void SettingsCard2::_OnHeaderChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard2>(sender.as<default_interface<SettingsCard2>>())->_OnHeaderChanged();
}

void SettingsCard2::_OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard2>(sender.as<default_interface<SettingsCard2>>())->_OnDescriptionChanged();
}

void SettingsCard2::_OnHeaderIconChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard2>(sender.as<default_interface<SettingsCard2>>())->_OnHeaderIconChanged();
}

void SettingsCard2::_OnIsClickEnabledChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard2>(sender.as<default_interface<SettingsCard2>>())->_OnIsClickEnabledChanged();
}

void SettingsCard2::_OnIsActionIconVisibleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard2>(sender.as<default_interface<SettingsCard2>>())->_OnActionIconChanged();
}

void SettingsCard2::_OnHeaderChanged() {
}

void SettingsCard2::_OnDescriptionChanged() {
}

void SettingsCard2::_OnHeaderIconChanged() {
}

void SettingsCard2::_OnIsClickEnabledChanged() {
}

void SettingsCard2::_OnActionIconChanged() {
}

void SettingsCard2::_CheckInitialVisualState() {
	VisualStateManager::GoToState(*this, IsEnabled() ? NormalState : DisabledState, true);

	/*if (GetTemplateChild(L"ContentAlignmentStates") is VisualStateGroup contentAlignmentStatesGroup) {
		contentAlignmentStatesGroup.CurrentStateChanged -= this.ContentAlignmentStates_Changed;
		CheckVerticalSpacingState(contentAlignmentStatesGroup.CurrentState);
		contentAlignmentStatesGroup.CurrentStateChanged += this.ContentAlignmentStates_Changed;
	}*/
}

}
