#pragma once
#include "Event.h"
#include <rapidjson/prettywriter.h>
#include <rapidjson/document.h>

namespace Magpie {

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

	void RemoveScalingMode(uint32_t index);

	bool MoveScalingMode(uint32_t scalingModeIdx, bool isMoveUp);

	// 不能使用 rapidjson::Writer 类型，因为 PrettyWriter 没有重写 Writer 中的方法
	// 不合理的 API 设计
	void Export(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer) const noexcept;

	bool Import(const rapidjson::GenericObject<true, rapidjson::Value>& root, bool loadingSettings) noexcept;

	bool ImportLegacy(const rapidjson::Document& doc) noexcept;

	Core::Event<EffectAddedWay> ScalingModeAdded;
	Core::Event<uint32_t> ScalingModeRemoved;
	Core::Event<uint32_t, bool> ScalingModeMoved;

private:
	ScalingModesService() = default;
};

}
