#pragma once

namespace Magpie {

struct ContentDialogHelper {
	static winrt::IAsyncOperation<winrt::ContentDialogResult> ShowAsync(winrt::ContentDialog dialog);
	static bool IsAnyDialogOpen() noexcept;
	static void CloseActiveDialog();
};

}
