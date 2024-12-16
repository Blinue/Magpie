#include "pch.h"
#include "ContentDialogHelper.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;

namespace Magpie {

//static weak_ref<ContentDialog> activeDialog{ nullptr };

IAsyncOperation<ContentDialogResult> ContentDialogHelper::ShowAsync(ContentDialog dialog) {
	//assert(activeDialog == nullptr);

	//activeDialog = dialog;
	ContentDialogResult result = co_await dialog.ShowAsync();
	//activeDialog = nullptr;
	co_return result;
}

bool ContentDialogHelper::IsAnyDialogOpen() noexcept {
	return false;//activeDialog != nullptr;
}

void ContentDialogHelper::CloseActiveDialog() {
	/*if (activeDialog == nullptr) {
		return;
	}

	if (auto dialog = activeDialog.get()) {
		dialog.Hide();
	}*/
}

}
