#pragma once
#include "SettingsCard2.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsCard2 : SettingsCard2_base<SettingsCard2> {
	SettingsCard2();

	IInspectable Header() const {
		return GetValue(HeaderProperty);
	}

	void Header(IInspectable const& value) {
		SetValue(HeaderProperty, value);
	}

	IInspectable Description() const {
		return GetValue(DescriptionProperty);
	}

	void Description(IInspectable const& value) {
		SetValue(DescriptionProperty, value);
	}

	Controls::IconElement HeaderIcon() const {
		return GetValue(HeaderIconProperty).as<Controls::IconElement>();
	}

	void HeaderIcon(Controls::IconElement const& value) {
		SetValue(HeaderIconProperty, value);
	}

	Controls::IconElement ActionIcon() const {
		return GetValue(ActionIconProperty).as<Controls::IconElement>();
	}

	void ActionIcon(Controls::IconElement const& value) {
		SetValue(ActionIconProperty, value);
	}

	hstring ActionIconToolTip() const {
		return GetValue(ActionIconToolTipProperty).as<hstring>();
	}

	void ActionIconToolTip(const hstring& value) {
		SetValue(ActionIconToolTipProperty, box_value(value));
	}

	bool IsClickEnabled() const {
		return GetValue(IsClickEnabledProperty).as<bool>();
	}

	void IsClickEnabled(bool value) {
		SetValue(IsClickEnabledProperty, box_value(value));
	}

	ContentAlignment ContentAlignment() const {
		return GetValue(ContentAlignmentProperty).as<Magpie::App::ContentAlignment>();
	}

	void ContentAlignment(Magpie::App::ContentAlignment value) {
		SetValue(ContentAlignmentProperty, box_value(value));
	}

	bool IsActionIconVisible() const {
		return GetValue(IsActionIconVisibleProperty).as<bool>();
	}

	void IsActionIconVisible(bool value) {
		SetValue(IsActionIconVisibleProperty, box_value(value));
	}

	void OnApplyTemplate();

	static const DependencyProperty HeaderProperty;
	static const DependencyProperty DescriptionProperty;
	static const DependencyProperty HeaderIconProperty;
	static const DependencyProperty ActionIconProperty;
	static const DependencyProperty ActionIconToolTipProperty;
	static const DependencyProperty IsClickEnabledProperty;
	static const DependencyProperty ContentAlignmentProperty;
	static const DependencyProperty IsActionIconVisibleProperty;

private:
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

	void _CheckInitialVisualState();
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsCard2 : SettingsCard2T<SettingsCard2, implementation::SettingsCard2> {
};

}
