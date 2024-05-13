#include "pch.h"
#include "ScalingModesViewModel.h"
#if __has_include("ScalingModesViewModel.g.cpp")
#include "ScalingModesViewModel.g.cpp"
#endif
#include "EffectsService.h"
#include "AppSettings.h"
#include "EffectHelper.h"
#include "Logger.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include "ScalingMode.h"
#include "FileDialogHelper.h"
#include "CommonSharedConstants.h"

using namespace ::Magpie::Core;

namespace winrt::Magpie::App::implementation {

ScalingModesViewModel::ScalingModesViewModel() {
	_AddScalingModes();

	_scalingModeAddedRevoker = ScalingModesService::Get().ScalingModeAdded(
		auto_revoke, { this, &ScalingModesViewModel::_ScalingModesService_Added });
	_scalingModeMovedRevoker = ScalingModesService::Get().ScalingModeMoved(
		auto_revoke, { this, &ScalingModesViewModel::_ScalingModesService_Moved });
	_scalingModeRemovedRevoker = ScalingModesService::Get().ScalingModeRemoved(
		auto_revoke, { this, &ScalingModesViewModel::_ScalingModesService_Removed });
}

static std::optional<std::wstring> OpenFileDialogForJson(IFileDialog* fileDialog) noexcept {
	static std::wstring jsonFileStr(
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID)
		.GetString(L"FileDialog_JsonFile"));

	const COMDLG_FILTERSPEC fileType{ jsonFileStr.c_str(), L"*.json"};
	fileDialog->SetFileTypes(1, &fileType);
	fileDialog->SetDefaultExtension(L"json");

	return FileDialogHelper::OpenFileDialog(fileDialog, FOS_STRICTFILETYPES);
}

void ScalingModesViewModel::Export() const noexcept {
	com_ptr<IFileSaveDialog> fileDialog = try_create_instance<IFileSaveDialog>(CLSID_FileSaveDialog);
	if (!fileDialog) {
		Logger::Get().Error("创建 FileSaveDialog 失败");
		return;
	}

	fileDialog->SetFileName(L"ScalingModes");
	static std::wstring title(
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID)
		.GetString(L"ExportDialog_Title"));
	fileDialog->SetTitle(title.c_str());

	std::optional<std::wstring> fileName = OpenFileDialogForJson(fileDialog.get());
	if (!fileName.has_value() || fileName->empty()) {
		return;
	}

	rapidjson::StringBuffer json;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(json);
	writer.StartObject();
	ScalingModesService::Get().Export(writer);
	writer.EndObject();

	Win32Utils::WriteTextFile(fileName->c_str(), {json.GetString(), json.GetLength()});
}

static bool ImportImpl(bool legacy) noexcept {
	com_ptr<IFileOpenDialog> fileDialog = try_create_instance<IFileOpenDialog>(CLSID_FileOpenDialog);
	if (!fileDialog) {
		Logger::Get().Error("创建 FileOpenDialog 失败");
		return false;
	}

	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	hstring title = resourceLoader.GetString(legacy ? L"ImportLegacyDialog_Title" : L"ImportDialog_Title");
	fileDialog->SetTitle(title.c_str());

	std::optional<std::wstring> fileName = OpenFileDialogForJson(fileDialog.get());
	if (!fileName.has_value()) {
		return false;
	}
	if (fileName->empty()) {
		return true;
	}

	std::string json;
	if (!Win32Utils::ReadTextFile(fileName->c_str(), json)) {
		return false;
	}

	rapidjson::Document doc;
	// 导入时放宽 json 格式限制
	doc.ParseInsitu<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag>(json.data());
	if (doc.HasParseError()) {
		Logger::Get().Error(fmt::format("解析缩放模式失败\n\t错误码: {}", (int)doc.GetParseError()));
		return false;
	}

	if (legacy) {
		return ScalingModesService::Get().ImportLegacy(doc);
	}

	if (!doc.IsObject()) {
		return false;
	}

	return ScalingModesService::Get().Import(((const rapidjson::Document&)doc).GetObj(), false);
}

void ScalingModesViewModel::_Import(bool legacy) {
	ShowErrorMessage(false);
	if (!ImportImpl(legacy)) {
		ShowErrorMessage(true);
	}
}

void ScalingModesViewModel::PrepareForAdd() {
	std::vector<IInspectable> copyFromList;

	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	copyFromList.push_back(box_value(resourceLoader.GetString(
		L"ScalingModes_NewScalingModeFlyout_CopyFrom_None")));
	
	for (const auto& scalingMode : AppSettings::Get().ScalingModes()) {
		copyFromList.push_back(box_value(scalingMode.name));
	}
	_newScalingModeCopyFromList = single_threaded_vector(std::move(copyFromList));
	RaisePropertyChanged(L"NewScalingModeCopyFromList");

	_newScalingModeName.clear();
	RaisePropertyChanged(L"NewScalingModeName");

	_newScalingModeCopyFrom = 0;
	RaisePropertyChanged(L"NewScalingModeCopyFrom");
}

void ScalingModesViewModel::NewScalingModeName(const hstring& value) noexcept {
	_newScalingModeName = value;
	RaisePropertyChanged(L"NewScalingModeName");
	RaisePropertyChanged(L"IsAddButtonEnabled");
}

void ScalingModesViewModel::AddScalingMode() {
	ScalingModesService::Get().AddScalingMode(_newScalingModeName, _newScalingModeCopyFrom - 1);
}

fire_and_forget ScalingModesViewModel::_AddScalingModes(bool isInitialExpanded) {
	if (_addingScalingModes) {
		co_return;
	}
	_addingScalingModes = true;

	ScalingModesService& scalingModesService = ScalingModesService::Get();
	uint32_t total = scalingModesService.GetScalingModeCount();
	uint32_t curSize = _scalingModes.Size();

	CoreDispatcher dispatcher(nullptr);

	if (total - curSize <= 5) {
		for (; curSize < total; ++curSize) {
			_scalingModes.Append(ScalingModeItem(curSize, isInitialExpanded));
		}
	} else {
		assert(!isInitialExpanded);

		// 延迟加载
		for (int j = 0; j < 5; ++j) {
			_scalingModes.Append(ScalingModeItem(curSize++, false));
		}

		dispatcher = CoreWindow::GetForCurrentThread().Dispatcher();
		auto weakThis = get_weak();

		while (true) {
			co_await 10ms;
			co_await dispatcher;

			if (!weakThis.get()) {
				co_return;
			}

			total = scalingModesService.GetScalingModeCount();
			curSize = _scalingModes.Size();

			if (curSize < total) {
				_scalingModes.Append(ScalingModeItem(curSize++, false));
			}
			
			if (curSize >= total) {
				break;
			}
		}
	}

	_addingScalingModes = false;
}

void ScalingModesViewModel::_ScalingModesService_Added(EffectAddedWay way) {
	_AddScalingModes(way == EffectAddedWay::Add);
}

void ScalingModesViewModel::_ScalingModesService_Moved(uint32_t index, bool isMoveUp) {
	const uint32_t targetIndex = isMoveUp ? index - 1 : index + 1;

	ScalingModeItem targetItem = _scalingModes.GetAt(targetIndex).as<ScalingModeItem>();
	_scalingModes.RemoveAt(targetIndex);
	_scalingModes.InsertAt(index, targetItem);
}

void ScalingModesViewModel::_ScalingModesService_Removed(uint32_t index) {
	_scalingModes.RemoveAt(index);
}

}
