#include "pch.h"
#include "ScalingConfigPage.h"
#if __has_include("ScalingConfigPage.g.cpp")
#include "ScalingConfigPage.g.cpp"
#endif
#include "Win32Utils.h"
#include "ComboBoxHelper.h"
#include <dxgi1_6.h>

using namespace winrt;


namespace winrt::Magpie::App::implementation {

static std::vector<std::wstring> GetAllGraphicsAdapters() {
	winrt::com_ptr<IDXGIFactory1> dxgiFactory;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		return {};
	}

	std::vector<std::wstring> result;

	winrt::com_ptr<IDXGIAdapter1> adapter;
	for (UINT adapterIndex = 0;
		SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, adapter.put()));
		++adapterIndex
	) {
		DXGI_ADAPTER_DESC1 desc;
		hr = adapter->GetDesc1(&desc);
		result.emplace_back(SUCCEEDED(hr) ? desc.Description : L"???");
	}

	return result;
}

ScalingConfigPage::ScalingConfigPage() {
	InitializeComponent();

	App app = Application::Current().as<App>();
	_magSettings = app.Settings().GetMagSettings(0);

	if (Win32Utils::GetOSBuild() < 22000) {
		// Segoe MDL2 Assets 不存在 Move 图标
		AdjustCursorSpeedFontIcon().Glyph(L"\uE962");
	}

	CaptureModeComboBox().SelectedIndex((int32_t)_magSettings.CaptureMode());

	MultiMonitorUsageComboBox().SelectedIndex((int32_t)_magSettings.MultiMonitorUsage());
	if (GetSystemMetrics(SM_CMONITORS) <= 1) {
		// 只有一个显示器时隐藏多显示器选项
		MultiMonitorSettingItem().Visibility(Visibility::Collapsed);
		Is3DGameModeSettingItem().Margin({ 0,0,0,-2 });
	}

	{
		Controls::ItemCollection items = GraphicsAdapterComboBox().Items();
		for (const std::wstring& adapter : GetAllGraphicsAdapters()) {
			items.Append(box_value(adapter));
		}

		uint32_t adapter = _magSettings.GraphicsAdapter();
		if (adapter > 0 && adapter >= items.Size()) {
			_magSettings.GraphicsAdapter(0);
			adapter = 0;
		}

		GraphicsAdapterComboBox().SelectedIndex(adapter);
	}
	
	Is3DGameModeToggleSwitch().IsOn(_magSettings.Is3DGameMode());
	ShowFPSToggleSwitch().IsOn(_magSettings.IsShowFPS());

	VSyncToggleSwitch().IsOn(_magSettings.IsVSync());
	TripleBufferingToggleSwitch().IsOn(_magSettings.IsTripleBuffering());
	_UpdateVSync();

	_initialized = true;
}

void ScalingConfigPage::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ComboBoxHelper::DropDownOpened(*this, sender);
}

void ScalingConfigPage::CaptureModeComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&) {
	if (!_initialized) {
		return;
	}

	_magSettings.CaptureMode((Magpie::Runtime::CaptureMode)CaptureModeComboBox().SelectedIndex());
}

void ScalingConfigPage::MultiMonitorUsageComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&) {
	if (!_initialized) {
		return;
	}

	_magSettings.MultiMonitorUsage((Magpie::Runtime::MultiMonitorUsage)MultiMonitorUsageComboBox().SelectedIndex());
}

void ScalingConfigPage::GraphicsAdapterComboBox_SelectionChanged(IInspectable const&, Controls::SelectionChangedEventArgs const&) {
	if (!_initialized) {
		return;
	}

	_magSettings.GraphicsAdapter(GraphicsAdapterComboBox().SelectedIndex());
}

void ScalingConfigPage::Is3DGameModeToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	_magSettings.Is3DGameMode(Is3DGameModeToggleSwitch().IsOn());
}

void ScalingConfigPage::ShowFPSToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	_magSettings.IsShowFPS(ShowFPSToggleSwitch().IsOn());
}

void ScalingConfigPage::VSyncToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	if (!_initialized) {
		return;
	}

	_UpdateVSync();
}

void ScalingConfigPage::TripleBufferingToggleSwitch_Toggled(IInspectable const&, RoutedEventArgs const&) {
	_magSettings.IsTripleBuffering(TripleBufferingToggleSwitch().IsOn());
}

void ScalingConfigPage::_UpdateVSync() {
	bool isOn = VSyncToggleSwitch().IsOn();
	_magSettings.IsVSync(isOn);
	TripleBufferingSettingItem().IsEnabled(isOn);
}

}
