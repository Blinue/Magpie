#pragma once
#include "ScalingProfileViewModel.g.h"


namespace winrt::Magpie::UI {
struct ScalingProfile;
class AppXReader;
}

namespace winrt::Magpie::UI::implementation {

struct ScalingProfileViewModel : ScalingProfileViewModelT<ScalingProfileViewModel> {
	ScalingProfileViewModel(int32_t profileIdx);
	~ScalingProfileViewModel();

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	Controls::IconElement Icon() const noexcept {
		return _icon;
	}

	bool IsNotDefaultScalingProfile() const noexcept;

	bool IsProgramExist() const noexcept;
	fire_and_forget OpenProgramLocation() const noexcept;

	hstring Name() const noexcept;

	void Launch() const noexcept;

	hstring RenameText() const noexcept {
		return _renameText;
	}

	void RenameText(const hstring& value);

	bool IsRenameConfirmButtonEnabled() const noexcept {
		return _isRenameConfirmButtonEnabled;
	}

	void Rename();

	bool CanMoveUp() const noexcept;

	bool CanMoveDown() const noexcept;

	void MoveUp();

	void MoveDown();

	void Delete();

	IVector<IInspectable> ScalingModes() const noexcept {
		return _scalingModes;
	}

	int32_t ScalingMode() const noexcept;
	void ScalingMode(int32_t value);

	int32_t CaptureMode() const noexcept;
	void CaptureMode(int32_t value);

	bool Is3DGameMode() const noexcept;
	void Is3DGameMode(bool value);

	int32_t MultiMonitorUsage() const noexcept;
	void MultiMonitorUsage(int32_t value);

	IVector<IInspectable> GraphicsAdapters() const noexcept {
		return _graphicsAdapters;
	}

	int32_t GraphicsAdapter() const noexcept;
	void GraphicsAdapter(int32_t value);

	bool IsShowFPS() const noexcept;
	void IsShowFPS(bool value);

	bool IsVSync() const noexcept;
	void IsVSync(bool value);

	bool IsTripleBuffering() const noexcept;
	void IsTripleBuffering(bool value);

	bool IsDisableWindowResizing() const noexcept;
	void IsDisableWindowResizing(bool value);

	bool IsReserveTitleBar() const noexcept;
	void IsReserveTitleBar(bool value);

	bool IsCroppingEnabled() const noexcept;
	void IsCroppingEnabled(bool value);

	double CroppingLeft() const noexcept;
	void CroppingLeft(double value);

	double CroppingTop() const noexcept;
	void CroppingTop(double value);

	double CroppingRight() const noexcept;
	void CroppingRight(double value);

	double CroppingBottom() const noexcept;
	void CroppingBottom(double value);

	bool IsAdjustCursorSpeed() const noexcept;
	void IsAdjustCursorSpeed(bool value);

	bool IsDrawCursor() const noexcept;
	void IsDrawCursor(bool value);

	int32_t CursorScaling() const noexcept;
	void CursorScaling(int32_t value);

	double CustomCursorScaling() const noexcept;
	void CustomCursorScaling(double value);

	int32_t CursorInterpolationMode() const noexcept;
	void CursorInterpolationMode(int32_t value);

	bool IsDisableDirectFlip() const noexcept;
	void IsDisableDirectFlip(bool value);

private:
	fire_and_forget _LoadIcon(FrameworkElement const& mainPage);

	std::unique_ptr<AppXReader> _appxReader;

	hstring _renameText;
	std::wstring_view _trimedRenameText;
	bool _isRenameConfirmButtonEnabled = false;

	IVector<IInspectable> _scalingModes{ nullptr };
	IVector<IInspectable> _graphicsAdapters{ nullptr };

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	const bool _isDefaultProfile = true;
	uint32_t _index = 0;
	// 可以保存此指针的原因是：用户停留在此页面时不会有缩放配置被创建或删除
	ScalingProfile* _data = nullptr;

	MainPage::ActualThemeChanged_revoker _themeChangedRevoker;
	Windows::Graphics::Display::DisplayInformation _displayInformation{ nullptr };
	Windows::Graphics::Display::DisplayInformation::DpiChanged_revoker _dpiChangedRevoker;

	Controls::IconElement _icon{ nullptr };
};

}

namespace winrt::Magpie::UI::factory_implementation {

struct ScalingProfileViewModel : ScalingProfileViewModelT<ScalingProfileViewModel, implementation::ScalingProfileViewModel> {
};

}
