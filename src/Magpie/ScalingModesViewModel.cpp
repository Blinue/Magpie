#include "pch.h"
#include "ScalingModesViewModel.h"
#if __has_include("ScalingModesViewModel.g.cpp")
#include "ScalingModesViewModel.g.cpp"
#endif
#include "App.h"
#include "AppSettings.h"
#include "CommonSharedConstants.h"
#include "FileDialogHelper.h"
#include "Logger.h"
#include "ScalingMode.h"
#include "ScalingModeItem.h"
#include "ToastService.h"
#include "Win32Helper.h"

using namespace Magpie;

namespace winrt::Magpie::implementation {

ScalingModesViewModel::ScalingModesViewModel() {
	_AddScalingModes();

	_scalingModeAddedRevoker = ScalingModesService::Get().ScalingModeAdded(
		auto_revoke, std::bind_front(&ScalingModesViewModel::_ScalingModesService_Added, this));
	_scalingModeMovedRevoker = ScalingModesService::Get().ScalingModeMoved(
		auto_revoke, std::bind_front(&ScalingModesViewModel::_ScalingModesService_Moved, this));
	_scalingModeRemovedRevoker = ScalingModesService::Get().ScalingModeRemoved(
		auto_revoke, std::bind_front(&ScalingModesViewModel::_ScalingModesService_Removed, this));
}

static std::optional<std::filesystem::path> OpenFileDialogForJson(
	IFileDialog* fileDialog,
	const wchar_t* title,
	const wchar_t* jsonFileStr
) noexcept {
	fileDialog->SetTitle(title);
	const COMDLG_FILTERSPEC fileType{ jsonFileStr, L"*.json" };
	fileDialog->SetFileTypes(1, &fileType);
	fileDialog->SetDefaultExtension(L"json");

	return FileDialogHelper::OpenFileDialog(fileDialog, FOS_STRICTFILETYPES);
}

fire_and_forget ScalingModesViewModel::Export() noexcept {
	ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	const hstring title = resourceLoader.GetString(L"Dialog_Export_Title");
	const hstring jsonFileStr = resourceLoader.GetString(L"Dialog_JsonFile");

	auto weakThis = get_weak();

	// 在主线程使用 IFileOpenDialog 有些问题，尤其在 Win10 中
	co_await resume_background();

	com_ptr<IFileSaveDialog> fileDialog = try_create_instance<IFileSaveDialog>(CLSID_FileSaveDialog);
	if (!fileDialog) {
		Logger::Get().Error("创建 FileSaveDialog 失败");
		co_return;
	}

	fileDialog->SetFileName(L"ScalingModes");

	std::optional<std::filesystem::path> fileName =
		OpenFileDialogForJson(fileDialog.get(), title.c_str(), jsonFileStr.c_str());
	if (!fileName.has_value() || fileName->empty()) {
		co_return;
	}

	co_await App::Get().Dispatcher();

	if (!weakThis.get()) {
		co_return;
	}

	rapidjson::StringBuffer json;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(json);
	writer.StartObject();
	ScalingModesService::Get().Export(writer);
	writer.EndObject();

	if (!Win32Helper::WriteTextFile(fileName->c_str(), { json.GetString(), json.GetLength() })) {
		const hstring failedMsg = resourceLoader.GetString(L"Message_ExportScalingModesFailed");
		ToastService::Get().ShowMessageInApp({}, failedMsg.c_str());
	}
}

fire_and_forget ScalingModesViewModel::Import() {
	const ResourceLoader resourceLoader =
		ResourceLoader::GetForCurrentView(CommonSharedConstants::APP_RESOURCE_MAP_ID);
	const hstring title = resourceLoader.GetString(L"Dialog_Import_Title");
	const hstring jsonFileStr = resourceLoader.GetString(L"Dialog_JsonFile");

	auto weakThis = get_weak();

	// 在主线程使用 IFileOpenDialog 有些问题，尤其在 Win10 中
	co_await resume_background();

	com_ptr<IFileOpenDialog> fileDialog = try_create_instance<IFileOpenDialog>(CLSID_FileOpenDialog);
	if (!fileDialog) {
		Logger::Get().Error("创建 FileOpenDialog 失败");
		co_return;
	}

	std::optional<std::filesystem::path> fileName =
		OpenFileDialogForJson(fileDialog.get(), title.c_str(), jsonFileStr.c_str());
	if (!fileName.has_value()) {
		co_return;
	}
	if (fileName->empty()) {
		co_return;
	}

	std::string json;
	Win32Helper::ReadTextFile(fileName->c_str(), json);

	co_await App::Get().Dispatcher();

	if (!weakThis.get()) {
		co_return;
	}

	if (!json.empty()) {
		rapidjson::Document doc;
		// 导入时放宽 json 格式限制
		doc.ParseInsitu<rapidjson::kParseCommentsFlag | rapidjson::kParseTrailingCommasFlag>(json.data());
		if (doc.HasParseError()) {
			Logger::Get().Error(fmt::format("解析 json 失败\n\t错误码: {}", (int)doc.GetParseError()));
		} else if (doc.IsObject() &&
			ScalingModesService::Get().Import(((const rapidjson::Document&)doc).GetObj(), false)) {
			// 导入成功
			co_return;
		}
	}

	const hstring failedMsg = resourceLoader.GetString(L"Message_ImportScalingModesFailed");
	ToastService::Get().ShowMessageInApp({}, failedMsg.c_str());
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

	if (total - curSize <= 5) {
		for (; curSize < total; ++curSize) {
			_scalingModes.Append(make<ScalingModeItem>(curSize, isInitialExpanded));
		}
	} else {
		assert(!isInitialExpanded);

		// 延迟加载
		for (int j = 0; j < 5; ++j) {
			_scalingModes.Append(make<ScalingModeItem>(curSize++, false));
		}

		auto weakThis = get_weak();

		while (true) {
			co_await 10ms;
			co_await App::Get().Dispatcher();

			if (!weakThis.get()) {
				co_return;
			}

			total = scalingModesService.GetScalingModeCount();
			curSize = _scalingModes.Size();

			if (curSize < total) {
				_scalingModes.Append(make<ScalingModeItem>(curSize++, false));
			}
			
			if (curSize >= total) {
				break;
			}
		}
	}

	_addingScalingModes = false;
}

void ScalingModesViewModel::_ScalingModesService_Added(EffectAddedWay way) {
	// 不支持在事件回调中修改事件本身，因此延迟执行
	App::Get().Dispatcher().RunAsync(CoreDispatcherPriority::Normal, [this, way]() {
		_AddScalingModes(way == EffectAddedWay::Add);
	});
}

void ScalingModesViewModel::_ScalingModesService_Moved(uint32_t index, bool isMoveUp) {
	const uint32_t targetIndex = isMoveUp ? index - 1 : index + 1;

	IInspectable targetItem = _scalingModes.GetAt(targetIndex);
	_scalingModes.RemoveAt(targetIndex);
	_scalingModes.InsertAt(index, targetItem);
}

void ScalingModesViewModel::_ScalingModesService_Removed(uint32_t index) {
	_scalingModes.RemoveAt(index);
}

}
