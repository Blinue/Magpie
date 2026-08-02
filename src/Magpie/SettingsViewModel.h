#pragma once
#include "SettingsViewModel.g.h"
#include "OnnxRuntimeService.h"

namespace winrt::Magpie::implementation {

struct SettingsViewModel : SettingsViewModelT<SettingsViewModel>,
                           wil::notify_property_changed_base<SettingsViewModel> {
	SettingsViewModel();

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

	bool IsOnnxRuntimeSupported() const noexcept;
	bool IsOnnxRuntimeInstalled() const noexcept;
	bool IsOnnxRuntimeBusy() const noexcept;
	bool IsOnnxRuntimeError() const noexcept;
	bool IsOnnxRuntimeRestartRequired() const noexcept;
	double OnnxRuntimeDownloadProgress() const noexcept;
	bool IsOnnxRuntimeProgressIndeterminate() const noexcept;
	void DownloadOnnxRuntime();
	void CancelOnnxRuntimeDownload();
	fire_and_forget OpenModelsLocation() const noexcept;

private:
	void _OnnxRuntimeService_StatusChanged(::Magpie::OnnxRuntimeStatus status);
	void _OnnxRuntimeService_DownloadProgressChanged(double progress);

	::Magpie::Event<::Magpie::OnnxRuntimeStatus>::EventRevoker _onnxStatusChangedRevoker;
	::Magpie::Event<double>::EventRevoker _onnxDownloadProgressChangedRevoker;

	// 安装完成后必须重启：运行时是在 main 里固定的
	// Set once an install completes - the runtime is pinned in main(), so it
	// cannot be picked up until the next launch.
	bool _onnxRestartRequired = false;
};

}
