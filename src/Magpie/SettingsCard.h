#pragma once
#include "SettingsCard.g.h"
#include "SettingsCardStyle.g.h"

namespace winrt::Magpie::implementation {

struct SettingsCard : SettingsCardT<SettingsCard> {
	SettingsCard();
	~SettingsCard();

	static void RegisterDependencyProperties();
	static DependencyProperty HeaderProperty() { return _headerProperty; }
	static DependencyProperty DescriptionProperty() { return _descriptionProperty; }
	static DependencyProperty HeaderIconProperty() { return _headerIconProperty; }
	static DependencyProperty ActionIconProperty() { return _actionIconProperty; }
	static DependencyProperty IsClickEnabledProperty() { return _isClickEnabledProperty; }
	static DependencyProperty ContentAlignmentProperty() { return _contentAlignmentProperty; }
	static DependencyProperty IsActionIconVisibleProperty() { return _isActionIconVisibleProperty; }
	static DependencyProperty IsWrapEnabledProperty() { return _isWrapEnabledProperty; }

	IInspectable Header() const { return GetValue(_headerProperty); }
	void Header(IInspectable const& value) const { SetValue(_headerProperty, value); }

	IInspectable Description() const { return GetValue(_descriptionProperty); }
	void Description(IInspectable const& value) const { SetValue(_descriptionProperty, value); }

	IconElement HeaderIcon() const { return GetValue(_headerIconProperty).try_as<IconElement>(); }
	void HeaderIcon(IconElement const& value) const { SetValue(_headerIconProperty, value); }

	IconElement ActionIcon() const { return GetValue(_actionIconProperty).try_as<IconElement>(); }
	void ActionIcon(IconElement const& value) const { SetValue(_actionIconProperty, value); }

	bool IsClickEnabled() const { return GetValue(_isClickEnabledProperty).try_as<bool>().value(); }
	void IsClickEnabled(bool value) const { SetValue(_isClickEnabledProperty, box_value(value)); }

	ContentAlignment ContentAlignment() const { return GetValue(_contentAlignmentProperty).try_as<Magpie::ContentAlignment>().value(); }
	void ContentAlignment(Magpie::ContentAlignment value) const { SetValue(_contentAlignmentProperty, box_value(value)); }

	bool IsActionIconVisible() const { return GetValue(_isActionIconVisibleProperty).try_as<bool>().value(); }
	void IsActionIconVisible(bool value) const { SetValue(_isActionIconVisibleProperty, box_value(value)); }

	bool IsWrapEnabled() const { return GetValue(_isWrapEnabledProperty).try_as<bool>().value(); }
	void IsWrapEnabled(bool value) const { SetValue(_isWrapEnabledProperty, box_value(value)); }

	void OnApplyTemplate();

	void OnPointerPressed(Input::PointerRoutedEventArgs const& args);

	void OnPointerReleased(Input::PointerRoutedEventArgs const& args);

private:
	static DependencyProperty _headerProperty;
	static DependencyProperty _descriptionProperty;
	static DependencyProperty _headerIconProperty;
	static DependencyProperty _actionIconProperty;
	static DependencyProperty _isClickEnabledProperty;
	static DependencyProperty _contentAlignmentProperty;
	static DependencyProperty _isActionIconVisibleProperty;
	static DependencyProperty _isWrapEnabledProperty;

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

struct SettingsCardStyle : SettingsCardStyleT<SettingsCardStyle> {};

}

BASIC_FACTORY(SettingsCard)
BASIC_FACTORY(SettingsCardStyle)
