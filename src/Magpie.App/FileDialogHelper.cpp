#include "pch.h"
#include "FileDialogHelper.h"
#include "Logger.h"
#include "App.h"

namespace winrt::Magpie::App {

// 出错返回空，取消返回空字符串
std::optional<std::wstring> FileDialogHelper::OpenFileDialog(IFileDialog* fileDialog, FILEOPENDIALOGOPTIONS options) noexcept {
	FILEOPENDIALOGOPTIONS options1{};
	fileDialog->GetOptions(&options1);
	fileDialog->SetOptions(options1 | options | FOS_FORCEFILESYSTEM);

	if (fileDialog->Show((HWND)Application::Current().as<App>().HwndMain()) != S_OK) {
		// 被用户取消
		return std::wstring();
	}

	com_ptr<IShellItem> file;
	HRESULT hr = fileDialog->GetResult(file.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IFileSaveDialog::GetResult 失败", hr);
		return std::nullopt;
	}

	wchar_t* fileName = nullptr;
	hr = file->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &fileName);
	if (FAILED(hr)) {
		Logger::Get().ComError("IShellItem::GetDisplayName 失败", hr);
		return std::nullopt;
	}

	std::wstring result(fileName);
	CoTaskMemFree(fileName);
	return std::move(result);
}

}
