#include "pch.h"
#include "AboutViewModel.h"
#if __has_include("AboutViewModel.g.cpp")
#include "AboutViewModel.g.cpp"
#endif
#include "Version.h"
#include "UpdateService.h"
#include "AppSettings.h"

namespace winrt::Magpie::UI::implementation {

hstring AboutViewModel::Version() const noexcept {
	return MAGPIE_TAG_W;
}

fire_and_forget AboutViewModel::CheckForUpdate() {
	co_await UpdateService::Get().CheckForUpdateAsync();
}

bool AboutViewModel::IsCheckForPreviewUpdates() const noexcept {
	return AppSettings::Get().IsCheckForPreviewUpdates();
}

void AboutViewModel::IsCheckForPreviewUpdates(bool value) noexcept {
	AppSettings::Get().IsCheckForPreviewUpdates(value);
}

bool AboutViewModel::IsAutoDownloadUpdates() const noexcept {
	return AppSettings::Get().IsAutoDownloadUpdates();
}

void AboutViewModel::IsAutoDownloadUpdates(bool value) noexcept {
	AppSettings::Get().IsAutoDownloadUpdates(value);
}

}
