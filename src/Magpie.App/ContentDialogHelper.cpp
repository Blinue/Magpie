#include "pch.h"
#include "ContentDialogHelper.h"
#include "App.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;

namespace winrt::Magpie::App {

static weak_ref<ContentDialog> activeDialog;

IAsyncOperation<ContentDialogResult> ContentDialogHelper::ShowAsync(ContentDialog dialog) {
	assert(activeDialog == nullptr);

	activeDialog = dialog;
	ContentDialogResult result = co_await dialog.ShowAsync();
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
