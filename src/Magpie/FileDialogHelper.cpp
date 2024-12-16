#include "pch.h"
#include "FileDialogHelper.h"
#include "Logger.h"
#include "App.h"

using namespace ::Magpie;
using namespace winrt::Magpie;
using namespace winrt;

namespace Magpie {

// 出错返回空，取消返回空字符串
std::optional<std::wstring> FileDialogHelper::OpenFileDialog(IFileDialog* fileDialog, FILEOPENDIALOGOPTIONS options) noexcept {
	FILEOPENDIALOGOPTIONS options1{};
	fileDialog->GetOptions(&options1);
	fileDialog->SetOptions(options1 | options | FOS_FORCEFILESYSTEM);

	if (fileDialog->Show(implementation::App::Get().MainWindow().Handle()) != S_OK) {
		// 被用户取消
		return std::wstring();
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

	return std::wstring(fileName.get());
}

}
