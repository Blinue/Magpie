#include "pch.h"
#include "FileDialogHelper.h"
#include "Logger.h"
#include "App.h"
#include "MainWindow.h"

using namespace ::Magpie;
using namespace winrt::Magpie::implementation;
using namespace winrt;

namespace Magpie {

// 出错返回 nullopt，取消返回空字符串
std::optional<std::filesystem::path> FileDialogHelper::OpenFileDialog(
	IFileDialog* fileDialog,
	FILEOPENDIALOGOPTIONS options
) noexcept {
	FILEOPENDIALOGOPTIONS oldOptions{};
	fileDialog->GetOptions(&oldOptions);
	fileDialog->SetOptions(oldOptions | options | FOS_FORCEFILESYSTEM | FOS_OKBUTTONNEEDSINTERACTION);

	if (fileDialog->Show(App::Get().MainWindow().Handle()) != S_OK) {
		// 被用户取消
		return std::filesystem::path{};
	}

	com_ptr<IShellItem> file;
	HRESULT hr = fileDialog->GetResult(file.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IFileSaveDialog::GetResult 失败", hr);
		return std::nullopt;
	}

	wil::unique_cotaskmem_string fileName;
	hr = file->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, fileName.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IShellItem::GetDisplayName 失败", hr);
		return std::nullopt;
	}

	return std::filesystem::path(fileName.get());
}

}
