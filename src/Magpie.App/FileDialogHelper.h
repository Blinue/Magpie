#pragma once

namespace winrt::Magpie::App {

struct FileDialogHelper {
	static std::optional<std::wstring> OpenFileDialog(
		IFileDialog* fileDialog,
		FILEOPENDIALOGOPTIONS options = 0
	) noexcept;
};

}
