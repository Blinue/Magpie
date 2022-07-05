#pragma once
#include "ScalingProfileViewModel.g.h"


namespace winrt::Magpie::App {
class ScalingProfile;
}

namespace winrt::Magpie::App::implementation {

struct ScalingProfileViewModel : ScalingProfileViewModelT<ScalingProfileViewModel> {
	ScalingProfileViewModel(uint32_t profileId);

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	hstring Name() const noexcept;
	void Name(const hstring& value);

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

private:
	IVector<IInspectable> _graphicsAdapters;

	event<PropertyChangedEventHandler> _propertyChangedEvent;
	ScalingProfile& _profile;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ScalingProfileViewModel : ScalingProfileViewModelT<ScalingProfileViewModel, implementation::ScalingProfileViewModel> {
};

}
