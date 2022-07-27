#pragma once
#include "ScalingProfileViewModel.g.h"


namespace winrt::Magpie::App {
class ScalingProfile;
}

namespace winrt::Magpie::App::implementation {

struct ScalingProfileViewModel : ScalingProfileViewModelT<ScalingProfileViewModel> {
	ScalingProfileViewModel(int32_t profileIdx);

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

	hstring Name() const noexcept;

	hstring RenameText() const noexcept {
		return _renameText;
	}

	void RenameText(const hstring& value);

	bool IsRenameConfirmButtonEnabled() const noexcept {
		return _isRenameConfirmButtonEnabled;
	}

	void Rename();

	void MoveUp();

	void MoveDown();

	void Delete();

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

	hstring _renameText;
	std::wstring_view _trimedRenameText;
	bool _isRenameConfirmButtonEnabled = false;

	IVector<IInspectable> _graphicsAdapters;

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	const bool _isDefaultProfile = true;
	uint32_t _profileIdx = 0;
	ScalingProfile* _profile = nullptr;

	MainPage::ActualThemeChanged_revoker _themeChangedRevoker;
	Windows::Graphics::Display::DisplayInformation _displayInformation{ nullptr };
	Windows::Graphics::Display::DisplayInformation::DpiChanged_revoker _dpiChangedRevoker;

	Controls::IconElement _icon{ nullptr };
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ScalingProfileViewModel : ScalingProfileViewModelT<ScalingProfileViewModel, implementation::ScalingProfileViewModel> {
};

}
