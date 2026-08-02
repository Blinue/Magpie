#include "pch.h"
#include "SettingsViewModel.h"
#if __has_include("SettingsViewModel.g.cpp")
#include "SettingsViewModel.g.cpp"
#endif
#include "App.h"
#include "AppSettings.h"
#include "AutoStartHelper.h"
#include "CommonSharedConstants.h"
#include "LocalizationService.h"
#include "OnnxRuntimeService.h"
#include "Win32Helper.h"
// WIN32_LEAN_AND_MEAN 排除了 shellapi.h / excluded by WIN32_LEAN_AND_MEAN
#include <shellapi.h>

using namespace Magpie;

namespace winrt::Magpie::implementation {

SettingsViewModel::SettingsViewModel() {
	OnnxRuntimeService& service = OnnxRuntimeService::Get();

	_onnxStatusChangedRevoker = service.StatusChanged(
		auto_revoke,
		std::bind_front(&SettingsViewModel::_OnnxRuntimeService_StatusChanged, this)
	);
	_onnxDownloadProgressChangedRevoker = service.DownloadProgressChanged(
		auto_revoke,
		std::bind_front(&SettingsViewModel::_OnnxRuntimeService_DownloadProgressChanged, this)
	);
}

void SettingsViewModel::_OnnxRuntimeService_StatusChanged(OnnxRuntimeStatus status) {
	if (status == OnnxRuntimeStatus::Installed) {
		_onnxRestartRequired = true;
		RaisePropertyChanged(L"IsOnnxRuntimeRestartRequired");
	}

	RaisePropertyChanged(L"IsOnnxRuntimeInstalled");
	RaisePropertyChanged(L"IsOnnxRuntimeBusy");
	RaisePropertyChanged(L"IsOnnxRuntimeError");
	RaisePropertyChanged(L"IsOnnxRuntimeProgressIndeterminate");
}

void SettingsViewModel::_OnnxRuntimeService_DownloadProgressChanged(double /*progress*/) {
	RaisePropertyChanged(L"OnnxRuntimeDownloadProgress");
}

bool SettingsViewModel::IsOnnxRuntimeSupported() const noexcept {
#ifdef MAGPIE_ONNX_ENABLED
	return true;
#else
	return false;
#endif
}

bool SettingsViewModel::IsOnnxRuntimeInstalled() const noexcept {
	return OnnxRuntimeService::Get().IsInstalled();
}

bool SettingsViewModel::IsOnnxRuntimeBusy() const noexcept {
	const OnnxRuntimeStatus status = OnnxRuntimeService::Get().Status();
	return status == OnnxRuntimeStatus::Downloading ||
		status == OnnxRuntimeStatus::Extracting;
}

bool SettingsViewModel::IsOnnxRuntimeError() const noexcept {
	return OnnxRuntimeService::Get().Status() == OnnxRuntimeStatus::Error;
}

bool SettingsViewModel::IsOnnxRuntimeRestartRequired() const noexcept {
	return _onnxRestartRequired;
}

double SettingsViewModel::OnnxRuntimeDownloadProgress() const noexcept {
	return OnnxRuntimeService::Get().DownloadProgress() * 100;
}

bool SettingsViewModel::IsOnnxRuntimeProgressIndeterminate() const noexcept {
	// 解压阶段没有进度可报
	// Extraction reports no progress, so the bar spins instead of lying.
	return OnnxRuntimeService::Get().Status() == OnnxRuntimeStatus::Extracting;
}

void SettingsViewModel::DownloadOnnxRuntime() {
	OnnxRuntimeService::Get().DownloadAndInstall();
	RaisePropertyChanged(L"IsOnnxRuntimeBusy");
}

void SettingsViewModel::CancelOnnxRuntimeDownload() {
	OnnxRuntimeService::Get().Cancel();
}

fire_and_forget SettingsViewModel::OpenModelsLocation() const noexcept {
	// 目录可能还不存在，先建出来，否则资源管理器会报错
	// The folder may not exist yet; create it first or Explorer just errors.
	const std::wstring modelsDir =
		(Win32Helper::GetExePath().parent_path() / L"models").wstring();
	Win32Helper::CreateDir(modelsDir, true);

	co_await resume_background();
	ShellExecute(nullptr, L"open", modelsDir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

IVector<IInspectable> SettingsViewModel::Languages() const {
	std::span<const wchar_t*> tags = LocalizationService::Get().SupportedLanguages();

	std::vector<IInspectable> languages;
	languages.reserve(tags.size() + 1);

	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	languages.push_back(box_value(resourceLoader.GetString(L"Settings_General_Language_System")));
	for (const wchar_t* tag : tags) {
		Windows::Globalization::Language language(tag);
		languages.push_back(box_value(language.NativeName()));
	}
	return single_threaded_vector(std::move(languages));;
}

int SettingsViewModel::Language() const noexcept {
	return AppSettings::Get().Language() + 1;
}

void SettingsViewModel::Language(int value) {
	if (value < 0) {
		return;
	}

	AppSettings::Get().Language(value - 1);
	RaisePropertyChanged(L"Language");
	RaisePropertyChanged(L"RequireRestart");
}

bool SettingsViewModel::RequireRestart() const noexcept {
	static int initLanguage = AppSettings::Get().Language();
	return AppSettings::Get().Language() != initLanguage;
}

void SettingsViewModel::Restart() const {
	App::Get().Restart();
}

int SettingsViewModel::Theme() const noexcept {
	switch (AppSettings::Get().Theme()) {
	case AppTheme::System:
		return 0;
	case AppTheme::Light:
		return 1;
	case AppTheme::Dark:
		return 2;
	default:
		return 0;
	}
}

void SettingsViewModel::Theme(int value) {
	if (value < 0) {
		return;
	}

	AppTheme theme;
	switch (value) {
	case 1:
		theme = AppTheme::Light;
		break;
	case 2:
		theme = AppTheme::Dark;
		break;
	default:
		theme = AppTheme::System;
		break;
	}

	AppSettings::Get().Theme(theme);
	RaisePropertyChanged(L"Theme");
}

bool SettingsViewModel::IsRunAtStartup() const noexcept {
	return AutoStartHelper::IsAutoStartEnabled();
}

void SettingsViewModel::IsRunAtStartup(bool value) {
	if (value) {
		AutoStartHelper::EnableAutoStart(AppSettings::Get().IsAlwaysRunAsAdmin());
	} else {
		AutoStartHelper::DisableAutoStart();
	}

	RaisePropertyChanged(L"IsRunAtStartup");
}

bool SettingsViewModel::IsPortableMode() const noexcept {
	return AppSettings::Get().IsPortableMode();
}

void SettingsViewModel::IsPortableMode(bool value) {
	AppSettings& settings = AppSettings::Get();

	if (settings.IsPortableMode() == value) {
		return;
	}

	settings.IsPortableMode(value);
	RaisePropertyChanged(L"IsPortableMode");
}

fire_and_forget SettingsViewModel::OpenConfigLocation() const noexcept {
	std::filesystem::path configPath =
		AppSettings::Get().ConfigDir() / CommonSharedConstants::CONFIG_FILENAME;
	co_await resume_background();
	Win32Helper::OpenFolderAndSelectFile(configPath.c_str());
}

bool SettingsViewModel::IsShowNotifyIcon() const noexcept {
	return AppSettings::Get().IsShowNotifyIcon();
}

void SettingsViewModel::IsShowNotifyIcon(bool value) {
	AppSettings::Get().IsShowNotifyIcon(value);
	RaisePropertyChanged(L"IsShowNotifyIcon");
}

bool SettingsViewModel::IsProcessElevated() const noexcept {
	return Win32Helper::IsProcessElevated();
}

bool SettingsViewModel::IsAlwaysRunAsAdmin() const noexcept {
	return AppSettings::Get().IsAlwaysRunAsAdmin();
}

void SettingsViewModel::IsAlwaysRunAsAdmin(bool value) {
	AppSettings::Get().IsAlwaysRunAsAdmin(value);
}

}
