#pragma once
#include "SettingsCard2.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsCard2 : SettingsCard2_base<SettingsCard2> {
	SettingsCard2();

	~SettingsCard2();

	IInspectable Header() const {
		return GetValue(_headerProperty);
	}

	void Header(IInspectable const& value) {
		SetValue(_headerProperty, value);
	}

	IInspectable Description() const {
		return GetValue(_descriptionProperty);
	}

	void Description(IInspectable const& value) {
		SetValue(_descriptionProperty, value);
	}

	Controls::IconElement HeaderIcon() const {
		return GetValue(_headerIconProperty).as<Controls::IconElement>();
	}

	void HeaderIcon(Controls::IconElement const& value) {
		SetValue(_headerIconProperty, value);
	}

	Controls::IconElement ActionIcon() const {
		return GetValue(_actionIconProperty).as<Controls::IconElement>();
	}

	void ActionIcon(Controls::IconElement const& value) {
		SetValue(_actionIconProperty, value);
	}

	hstring ActionIconToolTip() const {
		return GetValue(_actionIconToolTipProperty).as<hstring>();
	}

	void ActionIconToolTip(const hstring& value) {
		SetValue(_actionIconToolTipProperty, box_value(value));
	}

	bool IsClickEnabled() const {
		return GetValue(_isClickEnabledProperty).as<bool>();
	}

	void IsClickEnabled(bool value) {
		SetValue(_isClickEnabledProperty, box_value(value));
	}

	ContentAlignment ContentAlignment() const {
		return GetValue(_contentAlignmentProperty).as<Magpie::App::ContentAlignment>();
	}

	void ContentAlignment(Magpie::App::ContentAlignment value) {
		SetValue(_contentAlignmentProperty, box_value(value));
	}

	bool IsActionIconVisible() const {
		return GetValue(_isActionIconVisibleProperty).as<bool>();
	}

	void IsActionIconVisible(bool value) {
		SetValue(_isActionIconVisibleProperty, box_value(value));
	}

	void OnApplyTemplate();

	void OnPointerPressed(Input::PointerRoutedEventArgs const& e);

	void OnPointerReleased(Input::PointerRoutedEventArgs e);

	static DependencyProperty HeaderProperty() {
		return _headerProperty;
	}

	static DependencyProperty DescriptionProperty() {
		return _descriptionProperty;
	}

	static DependencyProperty HeaderIconProperty() {
		return _headerIconProperty;
	}

	static DependencyProperty ActionIconProperty() {
		return _actionIconProperty;
	}

	static DependencyProperty ActionIconToolTipProperty() {
		return _actionIconToolTipProperty;
	}

	static DependencyProperty IsClickEnabledProperty() {
		return _isClickEnabledProperty;
	}

	static DependencyProperty ContentAlignmentProperty() {
		return _contentAlignmentProperty;
	}

	static DependencyProperty IsActionIconVisibleProperty() {
		return _isActionIconVisibleProperty;
	}

private:
	static const DependencyProperty _headerProperty;
	static const DependencyProperty _descriptionProperty;
	static const DependencyProperty _headerIconProperty;
	static const DependencyProperty _actionIconProperty;
	static const DependencyProperty _actionIconToolTipProperty;
	static const DependencyProperty _isClickEnabledProperty;
	static const DependencyProperty _contentAlignmentProperty;
	static const DependencyProperty _isActionIconVisibleProperty;

	static void _OnHeaderChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnDescriptionChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnHeaderIconChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnIsClickEnabledChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);
	static void _OnIsActionIconVisibleChanged(DependencyObject const& sender, DependencyPropertyChangedEventArgs const&);

	void _OnHeaderChanged();
	void _OnDescriptionChanged();
	void _OnHeaderIconChanged();
	void _OnIsClickEnabledChanged();
	void _OnActionIconChanged();

	void _CheckVerticalSpacingState(VisualState const& s);

	void _EnableButtonInteraction();

	void _DisableButtonInteraction();

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

struct SettingsCard2 : SettingsCard2T<SettingsCard2, implementation::SettingsCard2> {
};

}
