#pragma once
#include <ShlObj.h>

namespace Magpie {

struct FileDialogHelper {
	static std::optional<std::wstring> OpenFileDialog(
		IFileDialog* fileDialog,
		FILEOPENDIALOGOPTIONS options = 0
	) noexcept;
};

}
