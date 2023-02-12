#pragma once
#include "SettingsViewModel.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsViewModel : SettingsViewModelT<SettingsViewModel> {
	SettingsViewModel();

	IVector<IInspectable> Languages() const;

	int Language() const noexcept;
	void Language(int value);

	int Theme() const noexcept;
	void Theme(int value);

	bool IsRunAtStartup() const noexcept {
		return _isRunAtStartup;
	}

	void IsRunAtStartup(bool value) noexcept;

	bool IsMinimizeAtStartup() const noexcept {
		return _isMinimizeAtStartup;
	}

	void IsMinimizeAtStartup(bool value) noexcept;

	bool IsMinimizeAtStartupEnabled() const noexcept;

	bool IsPortableMode() const noexcept;
	void IsPortableMode(bool value) noexcept;

	fire_and_forget OpenConfigLocation() const noexcept;

	bool IsShowTrayIcon() const noexcept;
	void IsShowTrayIcon(bool value) noexcept;

	bool IsProcessElevated() const noexcept;

	bool IsAlwaysRunAsElevated() const noexcept;
	void IsAlwaysRunAsElevated(bool value) noexcept;

	bool IsSimulateExclusiveFullscreen() const noexcept;
	void IsSimulateExclusiveFullscreen(bool value) noexcept;

	bool IsInlineParams() const noexcept;
	void IsInlineParams(bool value) noexcept;

	bool IsDebugMode() const noexcept;
	void IsDebugMode(bool value) noexcept;

	bool IsDisableEffectCache() const noexcept;
	void IsDisableEffectCache(bool value) noexcept;

	bool IsSaveEffectSources() const noexcept;
	void IsSaveEffectSources(bool value) noexcept;

	bool IsWarningsAreErrors() const noexcept;
	void IsWarningsAreErrors(bool value) noexcept;

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

private:
	void _UpdateStartupOptions() noexcept;

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	bool _isRunAtStartup = false;
	bool _isMinimizeAtStartup = false;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsViewModel : SettingsViewModelT<SettingsViewModel, implementation::SettingsViewModel> {
};

}
