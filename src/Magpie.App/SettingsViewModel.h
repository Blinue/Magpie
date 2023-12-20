#pragma once
#include "SettingsViewModel.g.h"

namespace winrt::Magpie::App::implementation {

struct SettingsViewModel : SettingsViewModelT<SettingsViewModel> {
	SettingsViewModel();

	IVector<IInspectable> Languages() const;

	int Language() const noexcept;
	void Language(int value);

	bool RequireRestart() const noexcept;
	void Restart() const;

	int Theme() const noexcept;
	void Theme(int value);

	bool IsRunAtStartup() const noexcept {
		return _isRunAtStartup;
	}

	void IsRunAtStartup(bool value) noexcept;

	bool IsMinimizeAtStartup() const noexcept {
		return _isMinimizeAtStartup;
	}

	void IsMinimizeAtStartup(bool value);

	bool IsMinimizeAtStartupEnabled() const noexcept;

	bool IsPortableMode() const noexcept;
	void IsPortableMode(bool value);

	fire_and_forget OpenConfigLocation() const noexcept;

	bool IsShowTrayIcon() const noexcept;
	void IsShowTrayIcon(bool value);

	bool IsProcessElevated() const noexcept;

	bool IsAlwaysRunAsAdmin() const noexcept;
	void IsAlwaysRunAsAdmin(bool value);

	bool IsAllowScalingMaximized() const noexcept;
	void IsAllowScalingMaximized(bool value);

	bool IsSimulateExclusiveFullscreen() const noexcept;
	void IsSimulateExclusiveFullscreen(bool value);

	bool IsInlineParams() const noexcept;
	void IsInlineParams(bool value);

	bool IsDeveloperMode() const noexcept;
	void IsDeveloperMode(bool value);

	bool IsDebugMode() const noexcept;
	void IsDebugMode(bool value);

	bool IsEffectCacheDisabled() const noexcept;
	void IsEffectCacheDisabled(bool value);

	bool IsFontCacheDisabled() const noexcept;
	void IsFontCacheDisabled(bool value);

	bool IsSaveEffectSources() const noexcept;
	void IsSaveEffectSources(bool value);

	bool IsWarningsAreErrors() const noexcept;
	void IsWarningsAreErrors(bool value);

	int DuplicateFrameDetectionMode() const noexcept;
	void DuplicateFrameDetectionMode(int value);

	bool IsDynamicDection() const noexcept;

	bool IsStatisticsForDynamicDetectionEnabled() const noexcept;
	void IsStatisticsForDynamicDetectionEnabled(bool value);

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) {
		_propertyChangedEvent.remove(token);
	}

private:
	void _UpdateStartupOptions();

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	bool _isRunAtStartup = false;
	bool _isMinimizeAtStartup = false;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct SettingsViewModel : SettingsViewModelT<SettingsViewModel, implementation::SettingsViewModel> {
};

}
