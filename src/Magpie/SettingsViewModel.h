#pragma once
#include "SettingsViewModel.g.h"

namespace winrt::Magpie::implementation {

struct SettingsViewModel : SettingsViewModelT<SettingsViewModel>,
                           wil::notify_property_changed_base<SettingsViewModel> {
	IVector<IInspectable> Languages() const;

	int Language() const noexcept;
	void Language(int value);

	bool RequireRestart() const noexcept;
	void Restart() const;

	int Theme() const noexcept;
	void Theme(int value);

	bool IsRunAtStartup() const noexcept;
	void IsRunAtStartup(bool value);

	bool IsPortableMode() const noexcept;
	void IsPortableMode(bool value);

	fire_and_forget OpenConfigLocation() const noexcept;

	bool IsShowNotifyIcon() const noexcept;
	void IsShowNotifyIcon(bool value);

	bool IsProcessElevated() const noexcept;

	bool IsAlwaysRunAsAdmin() const noexcept;
	void IsAlwaysRunAsAdmin(bool value);
};

}
