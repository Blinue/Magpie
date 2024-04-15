#pragma once
#include "ProfileViewModel.g.h"
#include "SmallVector.h"

namespace winrt::Magpie::App {
struct Profile;
}

namespace winrt::Magpie::App::implementation {

struct ProfileViewModel : ProfileViewModelT<ProfileViewModel>,
                          wil::notify_property_changed_base<ProfileViewModel> {
	ProfileViewModel(int profileIdx);
	~ProfileViewModel();

	Controls::IconElement Icon() const noexcept {
		return _icon;
	}

	bool IsNotDefaultProfile() const noexcept;

	bool IsProgramExist() const noexcept {
		return _isProgramExist;
	}

	bool IsNotPackaged() const noexcept;

	fire_and_forget OpenProgramLocation() const noexcept;

	void ChangeExeForLaunching() const noexcept;

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

	int ScalingMode() const noexcept;
	void ScalingMode(int value);

	IVector<IInspectable> CaptureMethods() const noexcept {
		return _captureMethods;
	}

	int CaptureMethod() const noexcept;
	void CaptureMethod(int value);

	bool IsAutoScale() const noexcept;
	void IsAutoScale(bool value);

	bool Is3DGameMode() const noexcept;
	void Is3DGameMode(bool value);

	bool HasMultipleMonitors() const noexcept;

	int MultiMonitorUsage() const noexcept;
	void MultiMonitorUsage(int value);

	IVector<IInspectable> GraphicsCards() const noexcept;

	int GraphicsCard() const noexcept;
	void GraphicsCard(int value);

	bool IsShowGraphicsCardSettingsCard() const noexcept;

	bool IsFrameRateLimiterEnabled() const noexcept;
	void IsFrameRateLimiterEnabled(bool value);

	double MaxFrameRate() const noexcept;
	void MaxFrameRate(double value);

	bool IsShowFPS() const noexcept;
	void IsShowFPS(bool value);

	bool IsWindowResizingDisabled() const noexcept;
	void IsWindowResizingDisabled(bool value);

	bool IsCaptureTitleBar() const noexcept;
	void IsCaptureTitleBar(bool value);

	bool CanCaptureTitleBar() const noexcept;

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

	int CursorScaling() const noexcept;
	void CursorScaling(int value);

	double CustomCursorScaling() const noexcept;
	void CustomCursorScaling(double value);

	int CursorInterpolationMode() const noexcept;
	void CursorInterpolationMode(int value);

	hstring LaunchParameters() const noexcept;
	void LaunchParameters(const hstring& value);

	bool IsDirectFlipDisabled() const noexcept;
	void IsDirectFlipDisabled(bool value);

private:
	fire_and_forget _LoadIcon(FrameworkElement const& rootPage);

	bool _isProgramExist = true;

	hstring _renameText;
	std::wstring_view _trimedRenameText;

	IVector<IInspectable> _scalingModes{ nullptr };
	IVector<IInspectable> _captureMethods{ nullptr };
	SmallVector<std::wstring> _graphicsCards;

	uint32_t _index = 0;
	// 可以保存此指针的原因是: 用户停留在此页面时不会有缩放配置被创建或删除
	Profile* _data = nullptr;

	RootPage::ActualThemeChanged_revoker _themeChangedRevoker;
	Windows::Graphics::Display::DisplayInformation _displayInformation{ nullptr };
	Windows::Graphics::Display::DisplayInformation::DpiChanged_revoker _dpiChangedRevoker;

	Controls::IconElement _icon{ nullptr };

	const bool _isDefaultProfile = true;
	bool _isRenameConfirmButtonEnabled = false;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ProfileViewModel : ProfileViewModelT<ProfileViewModel, implementation::ProfileViewModel> {
};

}
