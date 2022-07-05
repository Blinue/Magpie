#include "pch.h"
#include "ScalingProfileViewModel.h"
#if __has_include("ScalingProfileViewModel.g.cpp")
#include "ScalingProfileViewModel.g.cpp"
#endif
#include "AppSettings.h"
#include "ScalingProfile.h"
#include <dxgi1_6.h>


namespace winrt::Magpie::App::implementation {

static std::vector<std::wstring> GetAllGraphicsAdapters() {
	com_ptr<IDXGIFactory1> dxgiFactory;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		return {};
	}

	std::vector<std::wstring> result;

	com_ptr<IDXGIAdapter1> adapter;
	for (UINT adapterIndex = 0;
		SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, adapter.put()));
		++adapterIndex
		) {
		DXGI_ADAPTER_DESC1 desc;
		hr = adapter->GetDesc1(&desc);

		// 不包含 WARP
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}

		result.emplace_back(SUCCEEDED(hr) ? desc.Description : L"???");
	}

	return result;
}

ScalingProfileViewModel::ScalingProfileViewModel(uint32_t profileId) 
	: _profile(profileId == 0 ? AppSettings::Get().DefaultScalingProfile() : AppSettings::Get().ScalingProfiles()[profileId - 1]) {

	std::vector<IInspectable> graphicsAdapters;
	graphicsAdapters.push_back(box_value(L"默认"));

	for (const std::wstring& adapter : GetAllGraphicsAdapters()) {
		graphicsAdapters.push_back(box_value(adapter));
	}

	if (graphicsAdapters.size() <= 2 || GraphicsAdapter() >= graphicsAdapters.size()) {
		GraphicsAdapter(0);
	}

	_graphicsAdapters = single_threaded_vector(std::move(graphicsAdapters));
}

hstring ScalingProfileViewModel::Name() const noexcept {
	return hstring(_profile.Name().empty() ? L"默认" : _profile.Name());
}

void ScalingProfileViewModel::Name(const hstring& value) {
	if (_profile.Name() == value) {
		return;
	}

	_profile.Name(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Name"));
}

int32_t ScalingProfileViewModel::CaptureMode() const noexcept {
	return (int32_t)_profile.MagSettings().CaptureMode();
}

void ScalingProfileViewModel::CaptureMode(int32_t value) {
	if (value < 0) {
		return;
	}

	Magpie::Runtime::CaptureMode captureMode = (Magpie::Runtime::CaptureMode)value;
	if (_profile.MagSettings().CaptureMode() == captureMode) {
		return;
	}

	_profile.MagSettings().CaptureMode(captureMode);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CaptureMode"));
}

bool ScalingProfileViewModel::Is3DGameMode() const noexcept {
	return _profile.MagSettings().Is3DGameMode();
}

void ScalingProfileViewModel::Is3DGameMode(bool value) {
	if (_profile.MagSettings().Is3DGameMode() == value) {
		return;
	}

	_profile.MagSettings().Is3DGameMode(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Is3DGameMode"));
}

int32_t ScalingProfileViewModel::MultiMonitorUsage() const noexcept {
	return (int32_t)_profile.MagSettings().MultiMonitorUsage();
}

void ScalingProfileViewModel::MultiMonitorUsage(int32_t value) {
	if (value < 0) {
		return;
	}

	Magpie::Runtime::MultiMonitorUsage multiMonitorUsage = (Magpie::Runtime::MultiMonitorUsage)value;
	if (_profile.MagSettings().MultiMonitorUsage() == multiMonitorUsage) {
		return;
	}

	_profile.MagSettings().MultiMonitorUsage(multiMonitorUsage);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"MultiMonitorUsage"));
}

int32_t ScalingProfileViewModel::GraphicsAdapter() const noexcept {
	return (int32_t)_profile.MagSettings().GraphicsAdapter();
}

void ScalingProfileViewModel::GraphicsAdapter(int32_t value) {
	if (value < 0) {
		return;
	}

	uint32_t graphicsAdapter = (uint32_t)value;
	if (_profile.MagSettings().GraphicsAdapter() == graphicsAdapter) {
		return;
	}

	_profile.MagSettings().GraphicsAdapter(graphicsAdapter);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"GraphicsAdapter"));
}

bool ScalingProfileViewModel::IsShowFPS() const noexcept {
	return _profile.MagSettings().IsShowFPS();
}

void ScalingProfileViewModel::IsShowFPS(bool value) {
	if (_profile.MagSettings().IsShowFPS() == value) {
		return;
	}

	_profile.MagSettings().IsShowFPS(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsShowFPS"));
}

bool ScalingProfileViewModel::IsVSync() const noexcept {
	return _profile.MagSettings().IsVSync();
}

void ScalingProfileViewModel::IsVSync(bool value) {
	if (_profile.MagSettings().IsVSync() == value) {
		return;
	}

	_profile.MagSettings().IsVSync(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsVSync"));
}

bool ScalingProfileViewModel::IsTripleBuffering() const noexcept {
	return _profile.MagSettings().IsTripleBuffering();
}

void ScalingProfileViewModel::IsTripleBuffering(bool value) {
	if (_profile.MagSettings().IsTripleBuffering() == value) {
		return;
	}

	_profile.MagSettings().IsTripleBuffering(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsTripleBuffering"));
}

bool ScalingProfileViewModel::IsDisableWindowResizing() const noexcept {
	return _profile.MagSettings().IsDisableWindowResizing();
}

void ScalingProfileViewModel::IsDisableWindowResizing(bool value) {
	if (_profile.MagSettings().IsDisableWindowResizing() == value) {
		return;
	}

	_profile.MagSettings().IsDisableWindowResizing(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsDisableWindowResizing"));
}

bool ScalingProfileViewModel::IsReserveTitleBar() const noexcept {
	return _profile.MagSettings().IsReserveTitleBar();
}

void ScalingProfileViewModel::IsReserveTitleBar(bool value) {
	if (_profile.MagSettings().IsReserveTitleBar() == value) {
		return;
	}

	_profile.MagSettings().IsReserveTitleBar(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsReserveTitleBar"));
}

bool ScalingProfileViewModel::IsCroppingEnabled() const noexcept {
	return _profile.IsCroppingEnabled();
}

void ScalingProfileViewModel::IsCroppingEnabled(bool value) {
	if (_profile.IsCroppingEnabled() == value) {
		return;
	}

	_profile.IsCroppingEnabled(value);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsCroppingEnabled"));
}

double ScalingProfileViewModel::CrppingLeft() const noexcept {
	return _profile.MagSettings().Cropping().Left;
}

void ScalingProfileViewModel::CrppingLeft(double value) {
	Magpie::Runtime::Cropping cropping = _profile.MagSettings().Cropping();
	if (cropping.Left == value) {
		return;
	}

	cropping.Left = std::isnan(value) ? 0 : value;
	_profile.MagSettings().Cropping(cropping);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CrppingLeft"));
}

double ScalingProfileViewModel::CrppingTop() const noexcept {
	return _profile.MagSettings().Cropping().Top;
}

void ScalingProfileViewModel::CrppingTop(double value) {
	Magpie::Runtime::Cropping cropping = _profile.MagSettings().Cropping();
	if (cropping.Top == value) {
		return;
	}

	// 用户已清空数字框则重置为 0
	cropping.Top = std::isnan(value) ? 0 : value;
	_profile.MagSettings().Cropping(cropping);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CrppingTop"));
}

double ScalingProfileViewModel::CrppingRight() const noexcept {
	return _profile.MagSettings().Cropping().Right;
}

void ScalingProfileViewModel::CrppingRight(double value) {
	Magpie::Runtime::Cropping cropping = _profile.MagSettings().Cropping();
	if (cropping.Right == value) {
		return;
	}

	cropping.Right = std::isnan(value) ? 0 : value;
	_profile.MagSettings().Cropping(cropping);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CrppingRight"));
}

double ScalingProfileViewModel::CrppingBottom() const noexcept {
	return _profile.MagSettings().Cropping().Bottom;
}

void ScalingProfileViewModel::CrppingBottom(double value) {
	Magpie::Runtime::Cropping cropping = _profile.MagSettings().Cropping();
	if (cropping.Bottom == value) {
		return;
	}

	cropping.Bottom = std::isnan(value) ? 0 : value;
	_profile.MagSettings().Cropping(cropping);
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CrppingBottom"));
}

}
