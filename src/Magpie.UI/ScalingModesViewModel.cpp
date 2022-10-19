#include "pch.h"
#include "ScalingModesViewModel.h"
#if __has_include("ScalingModesViewModel.g.cpp")
#include "ScalingModesViewModel.g.cpp"
#endif
#include "ScalingModesService.h"
#include "EffectsService.h"
#include "AppSettings.h"
#include "EffectHelper.h"
#include "Logger.h"
#include "StrUtils.h"
#include "Win32Utils.h"

using namespace ::Magpie::Core;
using namespace winrt;


namespace winrt::Magpie::UI::implementation {

ScalingModesViewModel::ScalingModesViewModel() {
	std::vector<IInspectable> downscalingEffects;
	downscalingEffects.reserve(7);
	downscalingEffects.push_back(box_value(L"无"));

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
			[](const std::pair<std::wstring, std::wstring>& l, const std::wstring& r) { return l.first < r; }
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

	std::vector<IInspectable> scalingModes;
	for (uint32_t i = 0, count = ScalingModesService::Get().GetScalingModeCount(); i < count;++i) {
		scalingModes.push_back(ScalingModeItem(i));
	}
	_scalingModes = single_threaded_observable_vector(std::move(scalingModes));

	_scalingModeAddedRevoker = ScalingModesService::Get().ScalingModeAdded(
		auto_revoke, { this, &ScalingModesViewModel::_ScalingModesService_Added });
	_scalingModeMovedRevoker = ScalingModesService::Get().ScalingModeMoved(
		auto_revoke, { this, &ScalingModesViewModel::_ScalingModesService_Moved });
	_scalingModeRemovedRevoker = ScalingModesService::Get().ScalingModeRemoved(
		auto_revoke, { this, &ScalingModesViewModel::_ScalingModesService_Removed });
}

static std::optional<std::wstring> OpenFileDialog(IFileDialog* fileDialog) {
	const COMDLG_FILTERSPEC fileType{ L"JSON", L"*.json" };
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

void ScalingModesViewModel::Export() const noexcept {
	com_ptr<IFileSaveDialog> fileDialog;
	HRESULT hr = CoCreateInstance(
		CLSID_FileSaveDialog,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&fileDialog)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("创建 IFileSaveDialog 失败", hr);
		return;
	}

	fileDialog->SetFileName(L"ScalingModes");
	fileDialog->SetTitle(L"导出缩放模式");

	std::optional<std::wstring> fileName = OpenFileDialog(fileDialog.get());
	if (!fileName.has_value() || fileName.value().empty()) {
		return;
	}

	rapidjson::StringBuffer json;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(json);
	writer.StartObject();
	ScalingModesService::Get().Export(writer);
	writer.EndObject();

	Win32Utils::WriteTextFile(fileName.value().c_str(), {json.GetString(), json.GetLength()});
}

static bool ImportImpl(bool legacy) {
	com_ptr<IFileOpenDialog> fileDialog;
	HRESULT hr = CoCreateInstance(
		CLSID_FileOpenDialog,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&fileDialog)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("创建 IFileOpenDialog 失败", hr);
		return false;
	}

	fileDialog->SetTitle(legacy ? L"导入旧版缩放模式" : L"导入缩放模式");

	std::optional<std::wstring> fileName = OpenFileDialog(fileDialog.get());
	if (!fileName.has_value()) {
		return false;
	}
	if (fileName.value().empty()) {
		return true;
	}

	std::string json;
	if (!Win32Utils::ReadTextFile(fileName.value().c_str(), json)) {
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

	return ScalingModesService::Get().Import(((const rapidjson::Document&)doc).GetObj());
}

void ScalingModesViewModel::_Import(bool legacy) {
	ShowErrorMessage(false);
	if (!ImportImpl(legacy)) {
		ShowErrorMessage(true);
	}
}

void ScalingModesViewModel::DownscalingEffectIndex(int value) {
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
}

bool ScalingModesViewModel::DownscalingEffectHasParameters() noexcept {
	if (_downscalingEffectIndex == 0) {
		return false;
	}

	const std::wstring& effectName = _downscalingEffectNames[(size_t)_downscalingEffectIndex - 1].first;
	return !EffectsService::Get().GetEffect(effectName)->params.empty();
}

void ScalingModesViewModel::PrepareForAdd() {
	std::vector<IInspectable> copyFromList;
	copyFromList.push_back(box_value(L"无"));
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

void ScalingModesViewModel::NewScalingModeName(const hstring& value) noexcept {
	_newScalingModeName = value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"NewScalingModeName"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsAddButtonEnabled"));
}

void ScalingModesViewModel::AddScalingMode() {
	ScalingModesService::Get().AddScalingMode(_newScalingModeName, _newScalingModeCopyFrom - 1);
}

void ScalingModesViewModel::_ScalingModesService_Added() {
	const uint32_t count = ScalingModesService::Get().GetScalingModeCount();
	for (uint32_t i = _scalingModes.Size(); i < count; ++i) {
		_scalingModes.Append(ScalingModeItem(i));
	}
}

void ScalingModesViewModel::_ScalingModesService_Moved(uint32_t index, bool isMoveUp) {
	uint32_t targetIndex = isMoveUp ? index - 1 : index + 1;

	ScalingModeItem targetItem = _scalingModes.GetAt(targetIndex).as<ScalingModeItem>();
	_scalingModes.RemoveAt(targetIndex);
	_scalingModes.InsertAt(index, targetItem);
}

void ScalingModesViewModel::_ScalingModesService_Removed(uint32_t index) {
	_scalingModes.RemoveAt(index);
}

}
