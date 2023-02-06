#pragma once
#include "ScalingModeEffectItem.g.h"

namespace Magpie::Core {
struct EffectOption;
}

namespace winrt::Magpie::App {
struct EffectInfo;
}

namespace winrt::Magpie::App::implementation {

struct ScalingModeEffectItem : ScalingModeEffectItemT<ScalingModeEffectItem> {
	ScalingModeEffectItem(uint32_t scalingModeIdx, uint32_t effectIdx);

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

	hstring Name() const noexcept {
		return hstring(_name);
	}

	uint32_t ScalingModeIdx() const noexcept {
		return _scalingModeIdx;
	}

	void ScalingModeIdx(uint32_t value) noexcept {
		_scalingModeIdx = value;
		_parametersViewModel.ScalingModeIdx(value);
	}

	uint32_t EffectIdx() const noexcept {
		return _effectIdx;
	}

	void EffectIdx(uint32_t value) noexcept;

	bool CanScale() const noexcept;

	bool HasParameters() const noexcept;

	int ScalingType() const noexcept;
	void ScalingType(int value);

	bool IsShowScalingFactors() const noexcept;
	bool IsShowScalingPixels() const noexcept;

	double ScalingFactorX() const noexcept;
	void ScalingFactorX(double value);

	double ScalingFactorY() const noexcept;
	void ScalingFactorY(double value);

	double ScalingPixelsX() const noexcept;
	void ScalingPixelsX(double value);

	double ScalingPixelsY() const noexcept;
	void ScalingPixelsY(double value);

	Magpie::App::EffectParametersViewModel Parameters() const noexcept {
		return _parametersViewModel;
	}

	void Remove();

	event_token Removed(EventHandler<uint32_t> const& handler) {
		return _removedEvent.add(handler);
	}

	void Removed(event_token const& token) noexcept {
		_removedEvent.remove(token);
	}

	bool CanMove() const noexcept;
	bool CanMoveUp() const noexcept;
	bool CanMoveDown() const noexcept;
	void MoveUp() noexcept;
	void MoveDown() noexcept;

	void EffectsChanged();

	// 上移为 true，下移为 false
	event_token Moved(EventHandler<bool> const& handler) {
		return _movedEvent.add(handler);
	}

	void Moved(event_token const& token) noexcept {
		_movedEvent.remove(token);
	}

private:
	::Magpie::Core::EffectOption& _Data() noexcept;
	const ::Magpie::Core::EffectOption& _Data() const noexcept;

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	uint32_t _scalingModeIdx = 0;
	uint32_t _effectIdx = 0;
	hstring _name;
	const EffectInfo* _effectInfo = nullptr;
	event<EventHandler<uint32_t>> _removedEvent;
	event<EventHandler<bool>> _movedEvent;

	Magpie::App::EffectParametersViewModel _parametersViewModel;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct ScalingModeEffectItem : ScalingModeEffectItemT<ScalingModeEffectItem, implementation::ScalingModeEffectItem> {
};

}
