#pragma once
#include "WinRTUtils.h"
#include <rapidjson/prettywriter.h>
#include <rapidjson/document.h>

namespace winrt::Magpie::App {

struct ScalingMode;

enum class EffectAddedWay {
	Add,
	Import
};

class ScalingModesService {
public:
	static ScalingModesService& Get() noexcept {
		static ScalingModesService instance;
		return instance;
	}

	ScalingModesService(const ScalingModesService&) = delete;
	ScalingModesService(ScalingModesService&&) = delete;

	ScalingMode& GetScalingMode(uint32_t idx);

	uint32_t GetScalingModeCount();

	// copyFrom < 0 表示新建空缩放配置
	void AddScalingMode(std::wstring_view name, int copyFrom);

	event_token ScalingModeAdded(delegate<EffectAddedWay> const& handler) {
		return _scalingModeAddedEvent.add(handler);
	}

	WinRTUtils::EventRevoker ScalingModeAdded(auto_revoke_t, delegate<EffectAddedWay> const& handler) {
		event_token token = ScalingModeAdded(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			ScalingModeAdded(token);
		});
	}

	void ScalingModeAdded(event_token const& token) {
		_scalingModeAddedEvent.remove(token);
	}

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

	// 不能使用 rapidjson::Writer 类型，因为 PrettyWriter 没有重写 Writer 中的方法
	// 不合理的 API 设计
	void Export(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const noexcept;

	bool Import(const rapidjson::GenericObject<true, rapidjson::Value>& root, bool loadingSettings) noexcept;

	bool ImportLegacy(const rapidjson::Document& doc) noexcept;
private:
	ScalingModesService() = default;

	event<delegate<EffectAddedWay>> _scalingModeAddedEvent;
	event<delegate<uint32_t>> _scalingModeRemovedEvent;
	event<delegate<uint32_t, bool>> _scalingModeMovedEvent;
};

}
