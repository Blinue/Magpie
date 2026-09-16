#include "pch.h"
#include "ContentDialogHelper.h"

namespace winrt {
using namespace Windows::UI::Xaml::Controls;
}

namespace Magpie {

static winrt::weak_ref<winrt::ContentDialog> activeDialog{ nullptr };

winrt::IAsyncOperation<winrt::ContentDialogResult>
ContentDialogHelper::ShowAsync(winrt::ContentDialog dialog) {
	assert(activeDialog == nullptr);

	activeDialog = dialog;
	winrt::ContentDialogResult result = co_await dialog.ShowAsync();
	activeDialog = nullptr;
	co_return result;
}

bool ContentDialogHelper::IsAnyDialogOpen() noexcept {
	return activeDialog != nullptr;
}

void ContentDialogHelper::CloseActiveDialog() {
	if (activeDialog == nullptr) {
		return;
	}

	if (auto dialog = activeDialog.get()) {
		dialog.Hide();
	}
}

}
