#pragma once
#include <deque>
#include <imgui.h>
#include "SmallVector.h"
#include "ImGuiImpl.h"
#include "ScalingOptions.h"

namespace Magpie {

struct EffectDesc;

class OverlayDrawer {
public:
	OverlayDrawer() = default;
	OverlayDrawer(const OverlayDrawer&) = delete;
	OverlayDrawer(OverlayDrawer&&) = delete;

	bool Initialize(DeviceResources& deviceResources, OverlayOptions& overlayOptions) noexcept;
	
	void Draw(
		uint32_t count,
		uint32_t fps,
		const SmallVector<float>& effectTimings,
		POINT drawOffset
	) noexcept;

	ToolbarState ToolbarState() const noexcept;

	void ToolbarState(Magpie::ToolbarState value) noexcept;

	bool AnyVisibleWindow() const noexcept;

	void MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	bool NeedRedraw(uint32_t fps) const noexcept;

	void UpdateAfterActiveEffectsChanged() noexcept;

	bool IsCursorOnCaptionArea() const noexcept {
		return _isCursorOnCaptionArea;
	}

private:
	bool _BuildFonts() noexcept;
	SmallVector<ImWchar> _BuildFontUI(std::wstring_view language, const std::vector<uint8_t>& fontData) noexcept;
	void _BuildFontIcons(const char* fontPath) noexcept;

	struct _EffectDrawInfo {
		const EffectDesc* desc = nullptr;
		std::span<const float> passTimings;
		float totalTime = 0.0f;
	};

	bool _DrawTimingItem(
		int& itemId,
		const char* text,
		const ImColor* color,
		float time,
		bool isExpanded = false
	) const noexcept;

	int _DrawEffectTimings(
		int& itemId,
		const _EffectDrawInfo& drawInfo,
		bool showPasses,
		std::span<const ImColor> colors,
		bool singleEffect
	) const noexcept;

	void _DrawTimelineItem(
		int& itemId,
		ImU32 color,
		float dpiScale,
		std::string_view name,
		float time,
		float effectsTotalTime,
		bool selected = false
	);

	bool _DrawToolbar(uint32_t fps, int& itemId) noexcept;

	bool _DrawProfiler(const SmallVector<float>& effectTimings, uint32_t fps, int& itemId) noexcept;

	const std::string& _GetResourceString(const std::wstring_view& key) noexcept;

	float _CalcToolbarAlpha() const noexcept;

	void _ClearStatesIfNoVisibleWindow() noexcept;

	OverlayOptions* _overlayOptions = nullptr;
	float _dpiScale = 1.0f;

	ImFont* _fontUI = nullptr; // 普通 UI 文字
	ImFont* _fontMonoNumbers = nullptr; // 普通 UI 文字，但数字部分是等宽的，只支持 ASCII
	ImFont* _fontIcons = nullptr; // 图标字体

	std::chrono::steady_clock::time_point _lastUpdateTime{};
	// (总计时间, 帧数)
	SmallVector<std::pair<float, uint32_t>, 0> _effectTimingsStatistics;
	SmallVector<float> _lastestAvgEffectTimings;

	SmallVector<uint32_t> _timelineColors;

	struct {
		std::string gpuName;
	} _hardwareInfo;

	ImGuiImpl _imguiImpl;

	uint32_t _lastFPS = std::numeric_limits<uint32_t>::max();
	float _lastToolbarAlpha = -1.0f;

	bool _isToolbarVisible = false;
	bool _isFirstFrame = true;
	bool _isToolbarPinned = false;
	bool _isCursorOnCaptionArea = false;
	bool _isToolbarItemActive = false;
	bool _isProfilerVisible = false;
#ifdef _DEBUG
	bool _isDemoWindowVisible = false;
#endif
};

}
