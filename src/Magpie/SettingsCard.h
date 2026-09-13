#pragma once
#include "SettingsCard.g.h"

namespace winrt::Magpie::implementation {

struct SettingsCard : SettingsCardT<SettingsCard> {
	DEFINE_DEPENDENCY_PROPERTY(IInspectable, Header, _headerProperty)
	DEFINE_DEPENDENCY_PROPERTY(IInspectable, Description, _descriptionProperty)
	DEFINE_DEPENDENCY_PROPERTY(IconElement, HeaderIcon, _headerIconProperty)
	DEFINE_DEPENDENCY_PROPERTY(IconElement, ActionIcon, _actionIconProperty)
	DEFINE_DEPENDENCY_PROPERTY(bool, IsClickEnabled, _isClickEnabledProperty)
	DEFINE_DEPENDENCY_PROPERTY(winrt::Magpie::ContentAlignment, ContentAlignment, _contentAlignmentProperty)
	DEFINE_DEPENDENCY_PROPERTY(bool, IsActionIconVisible, _isActionIconVisibleProperty)
	DEFINE_DEPENDENCY_PROPERTY(bool, IsWrapEnabled, _isWrapEnabledProperty)

public:
	SettingsCard();
	~SettingsCard();

	void OnApplyTemplate();

	void OnPointerPressed(Input::PointerRoutedEventArgs const& args);

	void OnPointerReleased(Input::PointerRoutedEventArgs const& args);

	void OnKeyDown(Input::KeyRoutedEventArgs const& args);

private:
	static void _RegisterDependencyProperties();

	void _OnHeaderChanged() const;
	void _OnDescriptionChanged() const;
	void _OnHeaderIconChanged() const;
	void _OnIsClickEnabledChanged();
	void _OnActionIconChanged() const;
	void _OnIsWrapEnabledChanged() const;

	void _CheckVerticalSpacingState(VisualState const& s);

	void _EnableButtonInteraction();

	void _DisableButtonInteraction();

	IsEnabledChanged_revoker _isEnabledChangedRevoker;
	VisualStateGroup::CurrentStateChanged_revoker _contentAlignmentStatesChangedRevoker;
	SizeChanged_revoker _sizeChangedRevoker;

	UIElement::PointerEntered_revoker _pointerEnteredRevoker;
	UIElement::PointerExited_revoker _pointerExitedRevoker;
	UIElement::PointerCaptureLost_revoker _pointerCaptureLostRevoker;
	UIElement::PointerCanceled_revoker _pointerCanceledRevoker;
	UIElement::PreviewKeyDown_revoker _previewKeyDownRevoker;
	UIElement::PreviewKeyUp_revoker _previewKeyUpRevoker;

	bool _isCursorCaptured = false;
	bool _isCursorOnControl = false;
};

}

BASIC_FACTORY(SettingsCard)
