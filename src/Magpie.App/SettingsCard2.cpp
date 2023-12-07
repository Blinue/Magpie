// 移植自 https://github.com/CommunityToolkit/Windows/blob/bef863ca70bb1edf8c940198dd5cc74afa5d2aab/components/SettingsControls/src/SettingsCard/SettingsCard.cs

#include "pch.h"
#include "SettingsCard2.h"
#if __has_include("SettingsCard2.g.cpp")
#include "SettingsCard2.g.cpp"
#endif
#include <winrt/Windows.UI.Input.h>

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;

namespace winrt::Magpie::App::implementation {

constexpr const wchar_t* CommonStates = L"CommonStates";
constexpr const wchar_t* NormalState = L"Normal";
constexpr const wchar_t* PointerOverState = L"PointerOver";
constexpr const wchar_t* PressedState = L"Pressed";
constexpr const wchar_t* DisabledState = L"Disabled";

constexpr const wchar_t* ContentAlignmentStates = L"ContentAlignmentStates";
constexpr const wchar_t* RightState = L"Right";
constexpr const wchar_t* RightWrappedState = L"RightWrapped";
constexpr const wchar_t* RightWrappedNoIconState = L"RightWrappedNoIcon";
constexpr const wchar_t* LeftState = L"Left";
constexpr const wchar_t* VerticalState = L"Vertical";

constexpr const wchar_t* ContentSpacingStates = L"ContentSpacingStates";
constexpr const wchar_t* NoContentSpacingState = L"NoContentSpacing";
constexpr const wchar_t* ContentSpacingState = L"ContentSpacing";

constexpr const wchar_t* RootGrid = L"PART_RootGrid";
constexpr const wchar_t* ActionIconPresenterHolder = L"PART_ActionIconPresenterHolder";
constexpr const wchar_t* HeaderPresenter = L"PART_HeaderPresenter";
constexpr const wchar_t* DescriptionPresenter = L"PART_DescriptionPresenter";
constexpr const wchar_t* HeaderIconPresenterHolder = L"PART_HeaderIconPresenterHolder";

const DependencyProperty SettingsCard2::_headerProperty = DependencyProperty::Register(
	L"Header",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	PropertyMetadata(nullptr, &SettingsCard2::_OnHeaderChanged)
);

const DependencyProperty SettingsCard2::_descriptionProperty = DependencyProperty::Register(
	L"Description",
	xaml_typename<IInspectable>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	PropertyMetadata(nullptr, &SettingsCard2::_OnDescriptionChanged)
);

const DependencyProperty SettingsCard2::_headerIconProperty = DependencyProperty::Register(
	L"HeaderIcon",
	xaml_typename<IconElement>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	PropertyMetadata(nullptr, &SettingsCard2::_OnHeaderIconChanged)
);

const DependencyProperty SettingsCard2::_actionIconProperty = DependencyProperty::Register(
	L"ActionIcon",
	xaml_typename<IconElement>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	PropertyMetadata(box_value(L"\ue974"))
);

const DependencyProperty SettingsCard2::_actionIconToolTipProperty = DependencyProperty::Register(
	L"ActionIconToolTip",
	xaml_typename<hstring>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	nullptr
);

const DependencyProperty SettingsCard2::_isClickEnabledProperty = DependencyProperty::Register(
	L"IsClickEnabled",
	xaml_typename<bool>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	PropertyMetadata(box_value(false), &SettingsCard2::_OnIsClickEnabledChanged)
);

const DependencyProperty SettingsCard2::_contentAlignmentProperty = DependencyProperty::Register(
	L"ContentAlignment",
	xaml_typename<Magpie::App::ContentAlignment>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	PropertyMetadata(box_value(ContentAlignment::Right))
);

const DependencyProperty SettingsCard2::_isActionIconVisibleProperty = DependencyProperty::Register(
	L"IsActionIconVisible",
	xaml_typename<bool>(),
	xaml_typename<Magpie::App::SettingsCard2>(),
	PropertyMetadata(box_value(true), &SettingsCard2::_OnIsActionIconVisibleChanged)
);

SettingsCard2::SettingsCard2() {
	DefaultStyleKey(box_value(GetRuntimeClassName()));
}

void SettingsCard2::OnApplyTemplate() {
	SettingsCard2_base::OnApplyTemplate();

	// https://github.com/microsoft/microsoft-ui-xaml/issues/7792
	// 对于 Content，模板中的样式不起作用
	auto resources = Resources();
	for (const auto& [key, value] : GetTemplateChild(RootGrid).as<Grid>().Resources()) {
		resources.Insert(key, value);
	}

	_OnActionIconChanged();
	_OnHeaderChanged();
	_OnHeaderIconChanged();
	_OnDescriptionChanged();
	_OnIsClickEnabledChanged();

	VisualStateGroup contentAlignmentStatesGroup = GetTemplateChild(ContentAlignmentStates).as<VisualStateGroup>();
	contentAlignmentStatesGroup.CurrentStateChanged([this](IInspectable const&, VisualStateChangedEventArgs const& e) {
		_CheckVerticalSpacingState(e.NewState());
	});

	// 修复启动时的动画错误
	SizeChanged([this, contentAlignmentStatesGroup(std::move(contentAlignmentStatesGroup))](IInspectable const&, SizeChangedEventArgs const&) {
		_CheckVerticalSpacingState(contentAlignmentStatesGroup.CurrentState());
	});

	VisualStateManager::GoToState(*this, IsEnabled() ? NormalState : DisabledState, true);
	IsEnabledChanged([this](IInspectable const&, DependencyPropertyChangedEventArgs const&) {
		VisualStateManager::GoToState(*this, IsEnabled() ? NormalState : DisabledState, true);
	});
}

void SettingsCard2::OnPointerPressed(PointerRoutedEventArgs const& e) {
	// 忽略鼠标右键
	if (IsClickEnabled() && !(e.Pointer().PointerDeviceType() == Windows::Devices::Input::PointerDeviceType::Mouse && e.GetCurrentPoint(*this).Properties().PointerUpdateKind() == Windows::UI::Input::PointerUpdateKind::RightButtonPressed)) {
		SettingsCard2_base::OnPointerPressed(e);
		VisualStateManager::GoToState(*this, PressedState, true);

		_isCursorCaptured = true;
	}
}

void SettingsCard2::OnPointerReleased(PointerRoutedEventArgs e) {
	if (_isCursorCaptured && IsClickEnabled()) {
		SettingsCard2_base::OnPointerReleased(e);
		VisualStateManager::GoToState(*this, _isCursorOnControl ? PointerOverState : NormalState, true);
	}

	_isCursorCaptured = false;
}

void SettingsCard2::_OnHeaderChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard2>(sender.as<Magpie::App::SettingsCard2>())->_OnHeaderChanged();
}

void SettingsCard2::_OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard2>(sender.as<Magpie::App::SettingsCard2>())->_OnDescriptionChanged();
}

void SettingsCard2::_OnHeaderIconChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard2>(sender.as<Magpie::App::SettingsCard2>())->_OnHeaderIconChanged();
}

void SettingsCard2::_OnIsClickEnabledChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard2>(sender.as<Magpie::App::SettingsCard2>())->_OnIsClickEnabledChanged();
}

void SettingsCard2::_OnIsActionIconVisibleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard2>(sender.as<Magpie::App::SettingsCard2>())->_OnActionIconChanged();
}

void SettingsCard2::_OnHeaderChanged() {
	if (FrameworkElement headerPresenter = GetTemplateChild(HeaderPresenter).try_as<FrameworkElement>()) {
		headerPresenter.Visibility(Header() ? Visibility::Visible : Visibility::Collapsed);
	}
}

void SettingsCard2::_OnDescriptionChanged() {
	if (FrameworkElement descriptionPresenter = GetTemplateChild(DescriptionPresenter).try_as<FrameworkElement>()) {
		descriptionPresenter.Visibility(Description() ? Visibility::Visible : Visibility::Collapsed);
	}
}

void SettingsCard2::_OnHeaderIconChanged() {
	if (FrameworkElement headerIconPresenter = GetTemplateChild(HeaderIconPresenterHolder).try_as<FrameworkElement>()) {
		headerIconPresenter.Visibility(HeaderIcon() ? Visibility::Visible : Visibility::Collapsed);
	}
}

void SettingsCard2::_OnIsClickEnabledChanged() {
	_OnActionIconChanged();

	if (IsClickEnabled()) {
		_EnableButtonInteraction();
	} else {
		_DisableButtonInteraction();
	}
}

void SettingsCard2::_OnActionIconChanged() {
	if (FrameworkElement actionIconPresenter = GetTemplateChild(ActionIconPresenterHolder).try_as<FrameworkElement>()) {
		if (IsClickEnabled() && IsActionIconVisible()) {
			actionIconPresenter.Visibility(Visibility::Visible);
		} else {
			actionIconPresenter.Visibility(Visibility::Collapsed);
		}
	}
}

void SettingsCard2::_CheckVerticalSpacingState(VisualState const& s) {
	// On state change, checking if the Content should be wrapped (e.g. when the card is made smaller or the ContentAlignment is set to Vertical). If the Content and the Header or Description are not null, we add spacing between the Content and the Header/Description.

	const hstring stateName = s ? s.Name() : hstring();
	if (!stateName.empty() && (stateName == RightWrappedState || stateName == RightWrappedNoIconState ||
		stateName == VerticalState) && Content() && (Header() || Description())) {
		VisualStateManager::GoToState(*this, ContentSpacingState, true);
	} else {
		VisualStateManager::GoToState(*this, NoContentSpacingState, true);
	}
}

void SettingsCard2::_EnableButtonInteraction() {
	_DisableButtonInteraction();

	IsTabStop(true);

	_pointerEnteredRevoker = PointerEntered(auto_revoke, [this](IInspectable const&, PointerRoutedEventArgs const&) {
		VisualStateManager::GoToState(*this, _isCursorCaptured ? PressedState : PointerOverState, true);
		_isCursorOnControl = true;
	});

	_pointerExitedRevoker = PointerExited(auto_revoke, [this](IInspectable const&, PointerRoutedEventArgs const&) {
		VisualStateManager::GoToState(*this, NormalState, true);
		_isCursorOnControl = false;
	});

	auto goToNormalState = [this](IInspectable const&, PointerRoutedEventArgs const&) {
		VisualStateManager::GoToState(*this, NormalState, true);
	};

	_pointerCaptureLostRevoker = PointerCaptureLost(auto_revoke, goToNormalState);
	_pointerCanceledRevoker = PointerCanceled(auto_revoke, goToNormalState);

	_previewKeyDownRevoker = PreviewKeyDown(auto_revoke, [this](IInspectable const&, KeyRoutedEventArgs const& e) {
		const VirtualKey key = e.Key();
		if (key == VirtualKey::Enter || key == VirtualKey::Space || key == VirtualKey::GamepadA) {
			// Check if the active focus is on the card itself - only then we show the pressed state.
			if (FocusManager::GetFocusedElement(XamlRoot()) == *this) {
				VisualStateManager::GoToState(*this, PressedState, true);
			}
		}
	});

	_previewKeyUpRevoker = PreviewKeyUp(auto_revoke, [this](IInspectable const&, KeyRoutedEventArgs const& e) {
		const VirtualKey key = e.Key();
		if (key == VirtualKey::Enter || key == VirtualKey::Space || key == VirtualKey::GamepadA) {
			VisualStateManager::GoToState(*this, NormalState, true);
		}
	});
}

void SettingsCard2::_DisableButtonInteraction() {
	IsTabStop(false);
	_pointerEnteredRevoker.revoke();
	_pointerExitedRevoker.revoke();
	_pointerCaptureLostRevoker.revoke();
	_pointerCanceledRevoker.revoke();
	_previewKeyDownRevoker.revoke();
	_previewKeyUpRevoker.revoke();
}

}
