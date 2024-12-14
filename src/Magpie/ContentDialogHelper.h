#pragma once

namespace Magpie {

struct ContentDialogHelper {
	static winrt::IAsyncOperation<winrt::Controls::ContentDialogResult> ShowAsync(winrt::Controls::ContentDialog dialog);
	static bool IsAnyDialogOpen() noexcept;
	static void CloseActiveDialog();
};

}
