#include "pch.h"
#include "ScalingConfigurationViewModel.h"
#if __has_include("ScalingConfigurationViewModel.g.cpp")
#include "ScalingConfigurationViewModel.g.cpp"
#endif
#include "EffectsService.h"
#include "AppSettings.h"
#include "EffectHelper.h"
#include "Logger.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include "ScalingMode.h"

using namespace ::Magpie::Core;

namespace winrt::Magpie::App::implementation {

ScalingConfigurationViewModel::ScalingConfigurationViewModel() {
	_scalingModesListTransitions.Append(Animation::ContentThemeTransition());
	Animation::RepositionThemeTransition respositionAnime;
	respositionAnime.IsStaggeringEnabled(false);
	_scalingModesListTransitions.Append(std::move(respositionAnime));
	
	std::vector<IInspectable> downscalingEffects;
	downscalingEffects.reserve(7);

	ResourceLoader resourceLoader = ResourceLoader::GetForCurrentView();
	downscalingEffects.push_back(box_value(resourceLoader.GetString(
		L"ScalingConfiguration_General_DefaultDownscalingEffect_None")));

	_downscalingEffectNames.reserve(6);
	for (const EffectInfo& effectInfo : EffectsService::Get().Effects()) {
		if (effectInfo.IsGenericDownscaler()) {
			_downscalingEffectNames.emplace_back(effectInfo.name,
				StrUtils::ToLowerCase(EffectHelper::GetDisplayName(effectInfo.name)));
		}
	}

	// 根据显示名排序，不区分大小写
	std::sort(_downscalingEffectNames.begin(), _downscalingEffectNames.end(),
		[](const auto& l, const auto& r) { return l.second < r.second; });
	for (const auto& pair : _downscalingEffectNames) {
		downscalingEffects.push_back(box_value(EffectHelper::GetDisplayName(pair.first)));
	}
	_downscalingEffects = single_threaded_vector(std::move(downscalingEffects));

	DownscalingEffect& downscalingEffect = AppSettings::Get().DownscalingEffect();
	if (!downscalingEffect.name.empty()) {
		auto it = std::lower_bound(
			_downscalingEffectNames.begin(),
			_downscalingEffectNames.end(),
			downscalingEffect.name,
			[](const auto& l, const std::wstring& r) { return l.first < r; }
		);

		if (it == _downscalingEffectNames.end() || it->first != downscalingEffect.name) {
			Logger::Get().Warn(fmt::format("降采样效果 {} 不存在",
				StrUtils::UTF16ToUTF8(downscalingEffect.name)));
			downscalingEffect.name.clear();
			downscalingEffect.parameters.clear();
		} else {
			_downscalingEffectIndex = int(it - _downscalingEffectNames.begin() + 1);
		}
	}

	_AddScalingModes();

	_scalingModeAddedRevoker = ScalingModesService::Get().ScalingModeAdded(
		auto_revoke, { this, &ScalingConfigurationViewModel::_ScalingModesService_Added });
	_scalingModeMovedRevoker = ScalingModesService::Get().ScalingModeMoved(
		auto_revoke, { this, &ScalingConfigurationViewModel::_ScalingModesService_Moved });
	_scalingModeRemovedRevoker = ScalingModesService::Get().ScalingModeRemoved(
		auto_revoke, { this, &ScalingConfigurationViewModel::_ScalingModesService_Removed });
}

static std::optional<std::wstring> OpenFileDialog(IFileDialog* fileDialog) {
	const COMDLG_FILTERSPEC fileType{ L"JSON 文件", L"*.json" };
	fileDialog->SetFileTypes(1, &fileType);
	fileDialog->SetDefaultExtension(L"json");

	FILEOPENDIALOGOPTIONS options;
	fileDialog->GetOptions(&options);
	fileDialog->SetOptions(options | FOS_STRICTFILETYPES | FOS_FORCEFILESYSTEM);

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

void ScalingConfigurationViewModel::Export() const noexcept {
	com_ptr<IFileSaveDialog> fileDialog = try_create_instance<IFileSaveDialog>(CLSID_FileSaveDialog);
	if (!fileDialog) {
		Logger::Get().Error("创建 FileSaveDialog 失败");
		return;
	}

	fileDialog->SetFileName(L"ScalingModes");
	hstring title = ResourceLoader::GetForCurrentView().GetString(L"ExportDialog_Title");
	fileDialog->SetTitle(title.c_str());

	std::optional<std::wstring> fileName = OpenFileDialog(fileDialog.get());
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

static bool ImportImpl(bool legacy) {
	com_ptr<IFileOpenDialog> fileDialog = try_create_instance<IFileOpenDialog>(CLSID_FileOpenDialog);
	if (!fileDialog) {
		Logger::Get().Error("创建 FileOpenDialog 失败");
		return false;
	}

	ResourceLoader resourceLoader = ResourceLoader::GetForCurrentView();
	hstring title = resourceLoader.GetString(legacy ? L"ImportLegacyDialog_Title" : L"ImportDialog_Title");
	fileDialog->SetTitle(title.c_str());

	std::optional<std::wstring> fileName = OpenFileDialog(fileDialog.get());
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
		Logger::Get().Error(fmt::format("解析缩放模式失败\n\t错误码：{}", (int)doc.GetParseError()));
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

void ScalingConfigurationViewModel::_Import(bool legacy) {
	ShowErrorMessage(false);
	if (!ImportImpl(legacy)) {
		ShowErrorMessage(true);
	}
}

void ScalingConfigurationViewModel::DownscalingEffectIndex(int value) {
	if (_downscalingEffectIndex == value) {
		return;
	}
	_downscalingEffectIndex = value;

	DownscalingEffect& downscalingEffect = AppSettings::Get().DownscalingEffect();
	downscalingEffect.parameters.clear();
	if (value <= 0) {
		downscalingEffect.name.clear();
	} else {
		downscalingEffect.name = _downscalingEffectNames[(size_t)value - 1].first;
	}

	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"DownscalingEffectIndex"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"DownscalingEffectHasParameters"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"DownscalingEffectParameters"));

	AppSettings::Get().SaveAsync();
}

bool ScalingConfigurationViewModel::DownscalingEffectHasParameters() noexcept {
	if (_downscalingEffectIndex == 0) {
		return false;
	}

	const std::wstring& effectName = _downscalingEffectNames[(size_t)_downscalingEffectIndex - 1].first;
	return !EffectsService::Get().GetEffect(effectName)->params.empty();
}

void ScalingConfigurationViewModel::PrepareForAdd() {
	std::vector<IInspectable> copyFromList;

	ResourceLoader resourceLoader = ResourceLoader::GetForCurrentView();
	copyFromList.push_back(box_value(resourceLoader.GetString(
		L"ScalingConfiguration_ScalingModes_NewScalingModeFlyout_CopyFrom_None")));
	
	for (const auto& scalingMode : AppSettings::Get().ScalingModes()) {
		copyFromList.push_back(box_value(scalingMode.name));
	}
	_newScalingModeCopyFromList = single_threaded_vector(std::move(copyFromList));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"NewScalingModeCopyFromList"));

	_newScalingModeName.clear();
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"NewScalingModeName"));

	_newScalingModeCopyFrom = 0;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"NewScalingModeCopyFrom"));
}

void ScalingConfigurationViewModel::NewScalingModeName(const hstring& value) noexcept {
	_newScalingModeName = value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"NewScalingModeName"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsAddButtonEnabled"));
}

void ScalingConfigurationViewModel::AddScalingMode() {
	ScalingModesService::Get().AddScalingMode(_newScalingModeName, _newScalingModeCopyFrom - 1);
}

fire_and_forget ScalingConfigurationViewModel::_AddScalingModes(bool isInitialExpanded) {
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

	if (!_scalingModesInitialized) {
		_scalingModesInitialized = true;

		// 在所有缩放模式初始化完毕后再展示添加/删除动画
		if (dispatcher) {
			auto weakThis = get_weak();
			co_await 10ms;
			co_await dispatcher;
			if (!weakThis.get()) {
				co_return;
			}
		}
		
		_scalingModesListTransitions.Append(Animation::AddDeleteThemeTransition());
	}
}

void ScalingConfigurationViewModel::_ScalingModesService_Added(EffectAddedWay way) {
	_AddScalingModes(way == EffectAddedWay::Add);
}

void ScalingConfigurationViewModel::_ScalingModesService_Moved(uint32_t index, bool isMoveUp) {
	uint32_t targetIndex = isMoveUp ? index - 1 : index + 1;

	ScalingModeItem targetItem = _scalingModes.GetAt(targetIndex).as<ScalingModeItem>();
	_scalingModes.RemoveAt(targetIndex);
	_scalingModes.InsertAt(index, targetItem);
}

void ScalingConfigurationViewModel::_ScalingModesService_Removed(uint32_t index) {
	_scalingModes.RemoveAt(index);
}

}
