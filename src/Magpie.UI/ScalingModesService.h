#pragma once
#include "pch.h"
#include "Magpie.Core.h"
#include <ScalingMode.h>
#include "WinRTUtils.h"


namespace winrt::Magpie::UI {

class ScalingModesService {
public:
	static ScalingModesService& Get() {
		static ScalingModesService instance;
		return instance;
	}

	ScalingModesService(const ScalingModesService&) = delete;
	ScalingModesService(ScalingModesService&&) = delete;

	ScalingMode& GetScalingMode(uint32_t idx);

	uint32_t GetScalingModeCount();

	// copyFrom < 0 表示新建空缩放配置
	void AddScalingMode(std::wstring_view name, int copyFrom);

	void RemoveScalingMode(uint32_t index);

	event_token ScalingModeRemoved(delegate<uint32_t> const& handler) {
		return _scalingModeRemovedEvent.add(handler);
	}

	WinRTUtils::EventRevoker ScalingModeRemoved(auto_revoke_t, delegate<uint32_t> const& handler) {
		event_token token = ScalingModeRemoved(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			ScalingModeRemoved(token);
		});
	}

	void ScalingModeRemoved(event_token const& token) {
		_scalingModeRemovedEvent.remove(token);
	}

	bool MoveScalingMode(uint32_t scalingModeIdx, bool isMoveUp);

	event_token ScalingModeMoved(delegate<uint32_t, bool> const& handler) {
		return _scalingModeMovedEvent.add(handler);
	}

	WinRTUtils::EventRevoker ScalingModeMoved(auto_revoke_t, delegate<uint32_t, bool> const& handler) {
		event_token token = ScalingModeMoved(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			ScalingModeMoved(token);
		});
	}

	void ScalingModeMoved(event_token const& token) {
		_scalingModeMovedEvent.remove(token);
	}
private:
	ScalingModesService() = default;

	event<delegate<uint32_t>> _scalingModeRemovedEvent;
	event<delegate<uint32_t, bool>> _scalingModeMovedEvent;
};

}
