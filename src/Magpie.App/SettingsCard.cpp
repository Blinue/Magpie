// 移植自 https://github.com/CommunityToolkit/Windows/tree/bef863ca70bb1edf8c940198dd5cc74afa5d2aab/components/SettingsControls/src/SettingsCard

#include "pch.h"
#include "SettingsCard.h"
#if __has_include("SettingsCard.g.cpp")
#include "SettingsCard.g.cpp"
#endif
#include <winrt/Windows.UI.Input.h>

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;

namespace winrt::Magpie::App::implementation {

static constexpr const wchar_t* CommonStates = L"CommonStates";
static constexpr const wchar_t* NormalState = L"Normal";
static constexpr const wchar_t* PointerOverState = L"PointerOver";
static constexpr const wchar_t* PressedState = L"Pressed";
static constexpr const wchar_t* DisabledState = L"Disabled";

static constexpr const wchar_t* ContentAlignmentStates = L"ContentAlignmentStates";
static constexpr const wchar_t* RightState = L"Right";
static constexpr const wchar_t* RightWrappedState = L"RightWrapped";
static constexpr const wchar_t* RightWrappedNoIconState = L"RightWrappedNoIcon";
static constexpr const wchar_t* LeftState = L"Left";
static constexpr const wchar_t* VerticalState = L"Vertical";

static constexpr const wchar_t* ContentSpacingStates = L"ContentSpacingStates";
static constexpr const wchar_t* NoContentSpacingState = L"NoContentSpacing";
static constexpr const wchar_t* ContentSpacingState = L"ContentSpacing";

static constexpr const wchar_t* RootGrid = L"PART_RootGrid";
static constexpr const wchar_t* ActionIconPresenterHolder = L"PART_ActionIconPresenterHolder";
static constexpr const wchar_t* HeaderPresenter = L"PART_HeaderPresenter";
static constexpr const wchar_t* DescriptionPresenter = L"PART_DescriptionPresenter";
static constexpr const wchar_t* HeaderIconPresenterHolder = L"PART_HeaderIconPresenterHolder";

static constexpr const wchar_t* RightWrappedTrigger = L"RightWrappedTrigger";
static constexpr const wchar_t* RightWrappedNoIconTrigger = L"RightWrappedNoIconTrigger";

const DependencyProperty SettingsCard::_headerProperty = DependencyProperty::Register(
	L"Header",
	xaml_typename<IInspectable>(),
	xaml_typename<class_type>(),
	PropertyMetadata(nullptr, &SettingsCard::_OnHeaderChanged)
);

const DependencyProperty SettingsCard::_descriptionProperty = DependencyProperty::Register(
	L"Description",
	xaml_typename<IInspectable>(),
	xaml_typename<class_type>(),
	PropertyMetadata(nullptr, &SettingsCard::_OnDescriptionChanged)
);

const DependencyProperty SettingsCard::_headerIconProperty = DependencyProperty::Register(
	L"HeaderIcon",
	xaml_typename<IconElement>(),
	xaml_typename<class_type>(),
	PropertyMetadata(nullptr, &SettingsCard::_OnHeaderIconChanged)
);

const DependencyProperty SettingsCard::_actionIconProperty = DependencyProperty::Register(
	L"ActionIcon",
	xaml_typename<IconElement>(),
	xaml_typename<class_type>(),
	PropertyMetadata(box_value(L"\ue974"))
);

const DependencyProperty SettingsCard::_actionIconToolTipProperty = DependencyProperty::Register(
	L"ActionIconToolTip",
	xaml_typename<hstring>(),
	xaml_typename<class_type>(),
	nullptr
);

const DependencyProperty SettingsCard::_isClickEnabledProperty = DependencyProperty::Register(
	L"IsClickEnabled",
	xaml_typename<bool>(),
	xaml_typename<class_type>(),
	PropertyMetadata(box_value(false), &SettingsCard::_OnIsClickEnabledChanged)
);

const DependencyProperty SettingsCard::_contentAlignmentProperty = DependencyProperty::Register(
	L"ContentAlignment",
	xaml_typename<Magpie::App::ContentAlignment>(),
	xaml_typename<class_type>(),
	PropertyMetadata(box_value(ContentAlignment::Right))
);

const DependencyProperty SettingsCard::_isActionIconVisibleProperty = DependencyProperty::Register(
	L"IsActionIconVisible",
	xaml_typename<bool>(),
	xaml_typename<class_type>(),
	PropertyMetadata(box_value(true), &SettingsCard::_OnIsActionIconVisibleChanged)
);

const DependencyProperty SettingsCard::_isWrapEnabledProperty = DependencyProperty::Register(
	L"IsWrapEnabled",
	xaml_typename<bool>(),
	xaml_typename<class_type>(),
	PropertyMetadata(box_value(false), &SettingsCard::_OnIsWrapEnabledChanged)
);

SettingsCard::SettingsCard() {
	DefaultStyleKey(box_value(GetRuntimeClassName()));
}

SettingsCard::~SettingsCard() {
	// 不知为何必须手动释放 StateTriggers，否则会内存泄露
	if (auto stateGroup = GetTemplateChild(ContentAlignmentStates)) {
		for (VisualState state : stateGroup.as<VisualStateGroup>().States()) {
			state.StateTriggers().Clear();
		}
	}
}

void SettingsCard::OnApplyTemplate() {
	// https://github.com/microsoft/microsoft-ui-xaml/issues/7792
	// 对于 Content，模板中的样式不起作用
	auto resources = Resources();
	for (const auto& [key, value] : GetTemplateChild(RootGrid).as<Grid>().Resources()) {
		resources.Insert(key, value);
	}

	_OnIsWrapEnabledChanged();

	_contentAlignmentStatesChangedRevoker.revoke();
	_sizeChangedRevoker.revoke();
	_isEnabledChangedRevoker.revoke();

	_OnActionIconChanged();
	_OnHeaderChanged();
	_OnHeaderIconChanged();
	_OnDescriptionChanged();
	_OnIsClickEnabledChanged();

	VisualStateGroup contentAlignmentStatesGroup = GetTemplateChild(ContentAlignmentStates).as<VisualStateGroup>();
	_contentAlignmentStatesChangedRevoker = contentAlignmentStatesGroup.CurrentStateChanged(auto_revoke, [this](IInspectable const&, VisualStateChangedEventArgs const& args) {
		_CheckVerticalSpacingState(args.NewState());
	});

	// 修复启动时的动画错误
	_sizeChangedRevoker = SizeChanged(auto_revoke, [this, contentAlignmentStatesGroup(std::move(contentAlignmentStatesGroup))](IInspectable const&, SizeChangedEventArgs const&) {
		_CheckVerticalSpacingState(contentAlignmentStatesGroup.CurrentState());
	});

	VisualStateManager::GoToState(*this, IsEnabled() ? NormalState : DisabledState, true);
	_isEnabledChangedRevoker = IsEnabledChanged(auto_revoke, [this](IInspectable const&, DependencyPropertyChangedEventArgs const&) {
		VisualStateManager::GoToState(*this, IsEnabled() ? NormalState : DisabledState, true);
	});

	base_type::OnApplyTemplate();
}

void SettingsCard::OnPointerPressed(PointerRoutedEventArgs const& args) {
	// 忽略鼠标右键
	if (IsClickEnabled() && !(args.Pointer().PointerDeviceType() == Windows::Devices::Input::PointerDeviceType::Mouse && args.GetCurrentPoint(*this).Properties().PointerUpdateKind() == Windows::UI::Input::PointerUpdateKind::RightButtonPressed)) {
		base_type::OnPointerPressed(args);
		VisualStateManager::GoToState(*this, PressedState, true);

		_isCursorCaptured = true;
	}
}

void SettingsCard::OnPointerReleased(PointerRoutedEventArgs const& args) {
	if (_isCursorCaptured && IsClickEnabled()) {
		base_type::OnPointerReleased(args);
		VisualStateManager::GoToState(*this, _isCursorOnControl ? PointerOverState : NormalState, true);
	}

	_isCursorCaptured = false;
}

void SettingsCard::_OnHeaderChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard>(sender.as<class_type>())->_OnHeaderChanged();
}

void SettingsCard::_OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard>(sender.as<class_type>())->_OnDescriptionChanged();
}

void SettingsCard::_OnHeaderIconChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard>(sender.as<class_type>())->_OnHeaderIconChanged();
}

void SettingsCard::_OnIsClickEnabledChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard>(sender.as<class_type>())->_OnIsClickEnabledChanged();
}

void SettingsCard::_OnIsActionIconVisibleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard>(sender.as<class_type>())->_OnActionIconChanged();
}

void SettingsCard::_OnIsWrapEnabledChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&) {
	get_self<SettingsCard>(sender.as<class_type>())->_OnIsWrapEnabledChanged();
}

static bool IsNotEmpty(IInspectable const& value) noexcept {
	if (!value) {
		return false;
	}

	// 不知为何空字符串会导致崩溃，因此做额外的检查
	std::optional<hstring> str = value.try_as<hstring>();
	return !str || !str->empty();
}

void SettingsCard::_OnHeaderChanged() const {
	if (FrameworkElement headerPresenter = GetTemplateChild(HeaderPresenter).try_as<FrameworkElement>()) {
		headerPresenter.Visibility(IsNotEmpty(Header()) ? Visibility::Visible : Visibility::Collapsed);
	}
}

void SettingsCard::_OnDescriptionChanged() const {
	if (FrameworkElement descriptionPresenter = GetTemplateChild(DescriptionPresenter).try_as<FrameworkElement>()) {
		descriptionPresenter.Visibility(IsNotEmpty(Description()) ? Visibility::Visible : Visibility::Collapsed);
	}
}

void SettingsCard::_OnHeaderIconChanged() const {
	if (FrameworkElement headerIconPresenter = GetTemplateChild(HeaderIconPresenterHolder).try_as<FrameworkElement>()) {
		headerIconPresenter.Visibility(HeaderIcon() ? Visibility::Visible : Visibility::Collapsed);
	}
}

void SettingsCard::_OnIsClickEnabledChanged() {
	_OnActionIconChanged();

	if (IsClickEnabled()) {
		_EnableButtonInteraction();
	} else {
		_DisableButtonInteraction();
	}
}

void SettingsCard::_OnActionIconChanged() const {
	if (FrameworkElement actionIconPresenter = GetTemplateChild(ActionIconPresenterHolder).try_as<FrameworkElement>()) {
		if (IsClickEnabled() && IsActionIconVisible()) {
			actionIconPresenter.Visibility(Visibility::Visible);
		} else {
			actionIconPresenter.Visibility(Visibility::Collapsed);
		}
	}
}

void SettingsCard::_OnIsWrapEnabledChanged() const {
	auto trigger1 = GetTemplateChild(RightWrappedTrigger);
	auto trigger2 = GetTemplateChild(RightWrappedNoIconTrigger);

	if (trigger1 && trigger2) {
		// CanTrigger 无法使用 TemplateBinding？
		const bool isWrapEnabled = IsWrapEnabled();
		trigger1.as<ControlSizeTrigger>().CanTrigger(isWrapEnabled);
		trigger2.as<ControlSizeTrigger>().CanTrigger(isWrapEnabled);
	}
}

void SettingsCard::_CheckVerticalSpacingState(VisualState const& s) {
	// On state change, checking if the Content should be wrapped (e.g. when the card is made smaller or the ContentAlignment is set to Vertical). If the Content and the Header or Description are not null, we add spacing between the Content and the Header/Description.

	const hstring stateName = s ? s.Name() : hstring();
	if (!stateName.empty() && (stateName == RightWrappedState || stateName == RightWrappedNoIconState ||
		stateName == VerticalState) && Content() && (Header() || Description())) {
		VisualStateManager::GoToState(*this, ContentSpacingState, true);
	} else {
		VisualStateManager::GoToState(*this, NoContentSpacingState, true);
	}
}

void SettingsCard::_EnableButtonInteraction() {
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

	_previewKeyDownRevoker = PreviewKeyDown(auto_revoke, [this](IInspectable const&, KeyRoutedEventArgs const& args) {
		const VirtualKey key = args.Key();
		if (key == VirtualKey::Enter || key == VirtualKey::Space || key == VirtualKey::GamepadA) {
			// Check if the active focus is on the card itself - only then we show the pressed state.
			if (FocusManager::GetFocusedElement(XamlRoot()) == *this) {
				VisualStateManager::GoToState(*this, PressedState, true);
			}
		}
	});

	_previewKeyUpRevoker = PreviewKeyUp(auto_revoke, [this](IInspectable const&, KeyRoutedEventArgs const& args) {
		const VirtualKey key = args.Key();
		if (key == VirtualKey::Enter || key == VirtualKey::Space || key == VirtualKey::GamepadA) {
			VisualStateManager::GoToState(*this, NormalState, true);
		}
	});
}

void SettingsCard::_DisableButtonInteraction() {
	IsTabStop(false);
	_pointerEnteredRevoker.revoke();
	_pointerExitedRevoker.revoke();
	_pointerCaptureLostRevoker.revoke();
	_pointerCanceledRevoker.revoke();
	_previewKeyDownRevoker.revoke();
	_previewKeyUpRevoker.revoke();
}

}
