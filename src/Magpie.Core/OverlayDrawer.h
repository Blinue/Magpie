#pragma once
#include <deque>
#include "SmallVector.h"
#include <imgui.h>
#include "ImGuiImpl.h"
#include "Renderer.h"

namespace Magpie {

class OverlayDrawer {
public:
	OverlayDrawer();
	OverlayDrawer(const OverlayDrawer&) = delete;
	OverlayDrawer(OverlayDrawer&&) = delete;

	~OverlayDrawer();

	bool Initialize(DeviceResources* deviceResources) noexcept;
	
	void Draw(
		uint32_t count,
		uint32_t fps,
		const SmallVector<float>& effectTimings,
		POINT drawOffset
	) noexcept;

	bool IsUIVisible() const noexcept {
		return _isUIVisiable;
	}

	void SetUIVisibility(bool value) noexcept;

	void MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	bool _BuildFonts() noexcept;
	ImVector<ImWchar> _BuildFontUI(std::wstring_view language, const std::vector<uint8_t>& fontData) noexcept;
	void _BuildFontIcons(const char* fontPath) noexcept;

	struct _EffectDrawInfo {
		const Renderer::EffectInfo* info = nullptr;
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

	bool _DrawToolbar(uint32_t fps) noexcept;

	bool _DrawUI(const SmallVector<float>& effectTimings, uint32_t fps) noexcept;

	const std::string& _GetResourceString(const std::wstring_view& key) noexcept;

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

	winrt::ResourceLoader _resourceLoader{ nullptr };

	bool _isUIVisiable = false;
	bool _isFirstFrame = true;
	bool _isToolbarPinned = false;
};

}
