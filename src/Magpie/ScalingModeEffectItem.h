#pragma once
#include "ScalingModeEffectItem.g.h"

namespace Magpie::Core {
struct EffectOption;
}

namespace Magpie {
struct EffectInfo;
}

namespace winrt::Magpie::implementation {

struct ScalingModeEffectItem : ScalingModeEffectItemT<ScalingModeEffectItem>,
                               wil::notify_property_changed_base<ScalingModeEffectItem> {
	ScalingModeEffectItem(uint32_t scalingModeIdx, uint32_t effectIdx);

	hstring Name() const noexcept {
		return _name;
	}

	uint32_t ScalingModeIdx() const noexcept {
		return _scalingModeIdx;
	}

	void ScalingModeIdx(uint32_t value) noexcept;

	uint32_t EffectIdx() const noexcept {
		return _effectIdx;
	}

	void EffectIdx(uint32_t value) noexcept;

	bool CanScale() const noexcept;

	bool HasParameters() const noexcept;

	IVector<IInspectable> ScalingTypes() noexcept;

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

	Magpie::EffectParametersViewModel Parameters() const noexcept {
		return _parametersViewModel;
	}

	void Remove();

	bool CanMove() const noexcept;
	bool CanMoveUp() const noexcept;
	bool CanMoveDown() const noexcept;
	void MoveUp() noexcept;
	void MoveDown() noexcept;

	void RefreshMoveState();

	// 上移为 true，下移为 false
	wil::typed_event<Magpie::ScalingModeEffectItem, bool> Moved;
	wil::untyped_event<uint32_t> Removed;

private:
	::Magpie::Core::EffectOption& _Data() noexcept;
	const ::Magpie::Core::EffectOption& _Data() const noexcept;

	uint32_t _scalingModeIdx = 0;
	uint32_t _effectIdx = 0;
	hstring _name;
	const ::Magpie::EffectInfo* _effectInfo = nullptr;

	Magpie::EffectParametersViewModel _parametersViewModel{ nullptr };
};

}

namespace winrt::Magpie::factory_implementation {

struct ScalingModeEffectItem : ScalingModeEffectItemT<ScalingModeEffectItem, implementation::ScalingModeEffectItem> {
};

}
