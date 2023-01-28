#pragma once

namespace winrt::Magpie::App {

struct ContentDialogHelper {
	static IAsyncOperation<Controls::ContentDialogResult> ShowAsync(Controls::ContentDialog dialog);
	static bool IsAnyDialogOpen() noexcept;
	static void CloseActiveDialog();
};

}
