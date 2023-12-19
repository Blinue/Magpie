#pragma once
#include "SettingsCard.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsCard : SettingsCardT<SettingsCard> {
	SettingsCard();

	~SettingsCard();

	static DependencyProperty HeaderProperty() { return _headerProperty; }
	static DependencyProperty DescriptionProperty() { return _descriptionProperty; }
	static DependencyProperty HeaderIconProperty() { return _headerIconProperty; }
	static DependencyProperty ActionIconProperty() { return _actionIconProperty; }
	static DependencyProperty ActionIconToolTipProperty() { return _actionIconToolTipProperty; }
	static DependencyProperty IsClickEnabledProperty() { return _isClickEnabledProperty; }
	static DependencyProperty ContentAlignmentProperty() { return _contentAlignmentProperty; }
	static DependencyProperty IsActionIconVisibleProperty() { return _isActionIconVisibleProperty; }
	static DependencyProperty IsWrapEnabledProperty() { return _isWrapEnabledProperty; }

	IInspectable Header() const { return GetValue(_headerProperty); }
	void Header(IInspectable const& value) const { SetValue(_headerProperty, value); }

	IInspectable Description() const { return GetValue(_descriptionProperty); }
	void Description(IInspectable const& value) const { SetValue(_descriptionProperty, value); }

	Controls::IconElement HeaderIcon() const { return GetValue(_headerIconProperty).as<Controls::IconElement>(); }
	void HeaderIcon(Controls::IconElement const& value) const { SetValue(_headerIconProperty, value); }

	Controls::IconElement ActionIcon() const { return GetValue(_actionIconProperty).as<Controls::IconElement>(); }
	void ActionIcon(Controls::IconElement const& value) const { SetValue(_actionIconProperty, value); }

	hstring ActionIconToolTip() const { return GetValue(_actionIconToolTipProperty).as<hstring>(); }
	void ActionIconToolTip(const hstring& value) const { SetValue(_actionIconToolTipProperty, box_value(value)); }

	bool IsClickEnabled() const { return GetValue(_isClickEnabledProperty).as<bool>(); }
	void IsClickEnabled(bool value) const { SetValue(_isClickEnabledProperty, box_value(value)); }

	ContentAlignment ContentAlignment() const { return GetValue(_contentAlignmentProperty).as<Magpie::App::ContentAlignment>(); }
	void ContentAlignment(Magpie::App::ContentAlignment value) const { SetValue(_contentAlignmentProperty, box_value(value)); }

	bool IsActionIconVisible() const { return GetValue(_isActionIconVisibleProperty).as<bool>(); }
	void IsActionIconVisible(bool value) const { SetValue(_isActionIconVisibleProperty, box_value(value)); }

	bool IsWrapEnabled() const { return GetValue(_isWrapEnabledProperty).as<bool>(); }
	void IsWrapEnabled(bool value) const { SetValue(_isWrapEnabledProperty, box_value(value)); }

	void OnApplyTemplate();

	void OnPointerPressed(Input::PointerRoutedEventArgs const& args);

	void OnPointerReleased(Input::PointerRoutedEventArgs const& args);

private:
	static const DependencyProperty _headerProperty;
	static const DependencyProperty _descriptionProperty;
	static const DependencyProperty _headerIconProperty;
	static const DependencyProperty _actionIconProperty;
	static const DependencyProperty _actionIconToolTipProperty;
	static const DependencyProperty _isClickEnabledProperty;
	static const DependencyProperty _contentAlignmentProperty;
	static const DependencyProperty _isActionIconVisibleProperty;
	static const DependencyProperty _isWrapEnabledProperty;

	static void _OnHeaderChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnHeaderIconChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnIsClickEnabledChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnIsActionIconVisibleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnIsWrapEnabledChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

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

namespace winrt::Magpie::App::factory_implementation {

struct SettingsCard : SettingsCardT<SettingsCard, implementation::SettingsCard> {
};

}
