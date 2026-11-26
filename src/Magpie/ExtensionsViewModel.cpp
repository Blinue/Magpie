#include "pch.h"
#include "ExtensionsViewModel.h"
#if __has_include("ExtensionItem.g.cpp")
#include "ExtensionItem.g.cpp"
#endif
#if __has_include("ExtensionsViewModel.g.cpp")
#include "ExtensionsViewModel.g.cpp"
#endif
#include "App.h"
#include "AppSettings.h"
#include "AppFolderManager.h"
#include "ExtensionHelper.h"
#include "ExtensionService.h"
#include "FileDialogHelper.h"
#include "JsonHelper.h"
#include "Logger.h"
#include "StrHelper.h"
#include "ImageEncoderFactory.h"
#include "Win32Helper.h"
#include "InfoGrid.h"
#include <rapidjson/document.h>
#include <zip/zip.h>

using namespace ::Magpie;

namespace winrt::Magpie::implementation {

hstring ExtensionItem::Name() const noexcept {
    return winrt::to_hstring(_data->name);
}

hstring ExtensionItem::Description() const noexcept {
	return winrt::to_hstring(_data->description);
}

bool ExtensionItem::IsEnabled() const noexcept {
	return AppSettings::Get().ExtensionSettings()[_data->idx].isEnabled;
}

void ExtensionItem::IsEnabled(bool value) noexcept {
	ExtensionService::Get().SetExtensionEnabled(_extId, value);
	RaisePropertyChanged(L"IsEnabled");
	// 插件启用后才显示它提供的图像编码器和效果，部分功能在加载了 dll 之后才知道
	RaisePropertyChanged(L"InfoData");
}

Magpie::InfoGridData ExtensionItem::InfoData() const noexcept {
	assert(!_extId.starts_with("_"));

	SmallVector<InfoGridData::RowData, 0> rows;
	rows.reserve(6);

	rows.emplace_back(hstring(L"ID"), winrt::to_hstring(_extId));

	if (_data->version != Version{}) {
		rows.emplace_back(hstring(L"版本"), hstring(_data->version.ToString<wchar_t>()));
	}

	if (!_data->author.empty()) {
		rows.emplace_back(hstring(L"作者"), to_hstring(_data->author));
	}

	if (_data->homePage) {
		HyperlinkButton hyperlinkButton;
		hyperlinkButton.Padding({});
		hyperlinkButton.Content(box_value(L"访问主页"));
		hyperlinkButton.NavigateUri(_data->homePage);
		rows.emplace_back(hstring(L"主页"), std::move(hyperlinkButton));
	}

	if (!_data->imageEncoderFullIds.empty()) {
		std::string content;
		for (const std::string& encoderId : _data->imageEncoderFullIds) {
			const ImageEncoderMetadata* metadata =
				ImageEncoderFactory::Get().GetEncoderMetadata(encoderId);
			if (metadata) {
				if (!content.empty()) {
					content.push_back('\n');
				}
				content.append(metadata->name);
			} else {
				assert(false);
				Logger::Get().Error(StrHelper::Concat("未找到 ID 为 ", encoderId, " 的编码器"));
			}
		}

		rows.emplace_back(hstring(L"图像编码器"), to_hstring(content));
	}

	if (!_data->effectFullIds.empty()) {
		std::string content;
		for (const std::string& effectId : _data->effectFullIds) {
			// TODO
			if (!content.empty()) {
				content.push_back('\n');
			}
			content.append(effectId);
		}

		rows.emplace_back(hstring(L"效果"), to_hstring(content));
	}

	return make<InfoGridData>(std::move(rows));
}

ExtensionsViewModel::ExtensionsViewModel() {
	const SmallVectorImpl<ExtensionSetting>& extSettings = AppSettings::Get().ExtensionSettings();
	uint32_t extCount = (uint32_t)extSettings.size();

	std::vector<IInspectable> effects;
	effects.reserve(extCount);
	for (uint32_t i = 0; i < extCount; ++i) {
		const std::string& extId = extSettings[i].extId;
		if (const Extension* extension = ExtensionService::Get().GetExtension(extId)) {
			effects.push_back(*make_self<ExtensionItem>(extId, extension));
		} else {
			assert(false);
		}
	}
	_extensions = single_threaded_observable_vector(std::move(effects));
}

winrt::fire_and_forget ExtensionsViewModel::ImportExtension() noexcept {
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

	std::filesystem::path tempDir = AppFolderManager::Get().GetUniqueTempDir();
	if (tempDir.empty()) {
		Logger::Get().Error("AppFolderManager::GetUniqueTempDir 失败");
		co_return;
	}

	int ec = zip_extract(
		StrHelper::UTF16ToUTF8(extZipPath->native()).c_str(),
		StrHelper::UTF16ToUTF8(tempDir.native()).c_str(),
		nullptr,
		nullptr
	);
	if (ec < 0) {
		Logger::Get().Error(fmt::format("解压失败，错误代码: {}", ec));
		co_return;
	}

	std::wstring indexPath = StrHelper::Concat(tempDir.native(), L"\\magpie-ext.json");
	std::string indexJson;
	if (!Win32Helper::ReadTextFile(indexPath.c_str(), indexJson) || indexJson.empty()) {
		Logger::Get().Error("读取 magpie-ext.json 失败");
		co_return;
	}
	
	rapidjson::Document doc;
	doc.ParseInsitu(indexJson.data());

	if (doc.HasParseError()) {
		Logger::Get().Error(fmt::format("解析 json 失败\n\t错误码: {}", (int)doc.GetParseError()));
		co_return;
	}

	if (!doc.IsObject()) {
		Logger::Get().Error("根元素不是对象");
		co_return;
	}

	auto rootObj = const_cast<const rapidjson::Document&>(doc).GetObj();
	wil::zstring_view extId;
	if (!JsonHelper::ReadString(rootObj, "id", extId, true)) {
		Logger::Get().Error("解析 id 失败");
		co_return;
	}

	if (!ExtensionHelper::IsValidId(extId)) {
		Logger::Get().Error("id 不合法");
		co_return;
	}

	co_await App::Get().Dispatcher();

	if (ExtensionService::Get().GetExtension(extId)) {
		Logger::Get().Error(StrHelper::Concat("已存在 id 为 ", extId, " 的插件"));
		co_return;
	}

	std::wstring extDir = StrHelper::Concat(
		AppFolderManager::Get().GetExtDir(), L"\\", StrHelper::UTF8ToUTF16(extId));

	if (Win32Helper::DirExists(extDir.c_str())) {
		HRESULT hr = wil::RemoveDirectoryRecursiveNoThrow(
			extDir.c_str(), wil::RemoveDirectoryOptions::KeepRootDirectory);
		if (FAILED(hr)) {
			Logger::Get().ComError(StrHelper::Concat(
				"清空文件夹 ", StrHelper::UTF16ToUTF8(extDir), " 失败"), hr);
			co_return;
		}
	} else {
		if (!Win32Helper::CreateDir(extDir.c_str(), true)) {
			Logger::Get().Error(StrHelper::Concat(
				"创建文件夹 ", StrHelper::UTF16ToUTF8(extDir), " 失败"));
			co_return;
		}
	}

	if (!Win32Helper::MoveDir(tempDir, extDir)) {
		Logger::Get().Error(StrHelper::Concat(
			"移动文件夹 ", StrHelper::UTF16ToUTF8(tempDir.native()), " 失败"));
		co_return;
	}

	ExtensionService::Get().AddExtension(extId, rootObj);

	RaisePropertyChanged(L"HasExtension");
}

bool ExtensionsViewModel::HasExtension() const noexcept {
	return !AppSettings::Get().ExtensionSettings().empty();
}

}
