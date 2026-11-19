#include "pch.h"
#include "ExtensionsViewModel.h"
#if __has_include("ExtensionItem.g.cpp")
#include "ExtensionItem.g.cpp"
#endif
#if __has_include("ExtensionsViewModel.g.cpp")
#include "ExtensionsViewModel.g.cpp"
#endif
#include "ExtensionService.h"
#include "FileDialogHelper.h"
#include "Logger.h"
#include "AppFolderManager.h"
#include "StrHelper.h"
#include "Win32Helper.h"
#include <zip/zip.h>

using namespace Magpie;

namespace winrt::Magpie::implementation {

hstring ExtensionItem::Name() const noexcept {
    return hstring();
}

hstring ExtensionItem::Description() const noexcept {
	return hstring();
}

bool ExtensionItem::IsEnabled() const noexcept {
	return false;
}

void ExtensionItem::IsEnabled(bool /*value*/) noexcept {
}

ExtensionsViewModel::ExtensionsViewModel() {

}

winrt::fire_and_forget ExtensionsViewModel::ImportDll() noexcept {
	std::string extId = ExtensionService::Get().GenerateImportedExtId();

	co_await winrt::resume_background();

	com_ptr<IFileOpenDialog> pickExtDialog =
		try_create_instance<IFileOpenDialog>(CLSID_FileOpenDialog);
	if (!pickExtDialog) {
		Logger::Get().Error("创建 FileSaveDialog 失败");
		co_return;
	}

	pickExtDialog->SetTitle(L"选择插件");

	{
		const COMDLG_FILTERSPEC fileType = { L"zip 文件", L"*.zip"};
		pickExtDialog->SetFileTypes(1, &fileType);
	}
	pickExtDialog->SetDefaultExtension(L"zip");

	auto extZipPath = FileDialogHelper::OpenFileDialog(pickExtDialog.get());
	if (!extZipPath) {
		co_return;
	}

	std::wstring extIdUtf16 = StrHelper::UTF8ToUTF16(extId);
	std::wstring extDir = StrHelper::Concat(
		AppFolderManager::Get().GetWorkingDir().native(),
		L"\\", AppFolderManager::Get().GetExtDir(),
		L"\\", extIdUtf16
	);
	if (!Win32Helper::CreateDir(extDir, true)) {
		co_return;
	}

	int ec = zip_extract(
		StrHelper::UTF16ToUTF8(extZipPath->native()).c_str(),
		StrHelper::UTF16ToUTF8(extDir).c_str(),
		nullptr,
		nullptr
	);
	if (ec < 0) {
		Logger::Get().Error(fmt::format("解压失败，错误代码: {}", ec));
		co_return;
	}

	std::wstring indexPath = StrHelper::Concat(extDir, L"\\magpie-ext.json");
	if (!Win32Helper::FileExists(indexPath.c_str())) {
		Logger::Get().Error("未找到 magpie-ext.json");
		co_return;
	}
	
}

bool ExtensionsViewModel::HasExtension() const noexcept {
	return false;
}

}
