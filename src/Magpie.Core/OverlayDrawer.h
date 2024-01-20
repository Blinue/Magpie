#pragma once
#include <deque>
#include "SmallVector.h"
#include <imgui.h>
#include "ImGuiImpl.h"

namespace Magpie::Core {

class OverlayDrawer {
public:
	OverlayDrawer();
	OverlayDrawer(const OverlayDrawer&) = delete;
	OverlayDrawer(OverlayDrawer&&) = delete;

	bool Initialize(DeviceResources* deviceResources) noexcept;
	
	void Draw(uint32_t count, const SmallVector<float>& effectTimings) noexcept;

	bool IsUIVisible() const noexcept {
		return _isUIVisiable;
	}

	void SetUIVisibility(bool value) noexcept;

	void MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	bool _BuildFonts() noexcept;
	void _BuildFontUI(std::wstring_view language, const std::vector<uint8_t>& fontData, ImVector<ImWchar>& uiRanges) noexcept;
	void _BuildFontFPS(const std::vector<uint8_t>& fontData) noexcept;

	struct _EffectTimings {
		const struct EffectDesc* desc = nullptr;
		std::span<const float> passTimings;
		float totalTime = 0.0f;
	};

	int _DrawEffectTimings(const _EffectTimings& et, bool showPasses, float maxWindowWidth, std::span<const ImColor> colors, bool singleEffect) noexcept;

	void _DrawTimelineItem(ImU32 color, float dpiScale, std::string_view name, float time, float effectsTotalTime, bool selected = false);

	void _DrawFPS() noexcept;

	void _DrawUI(const SmallVector<float>& effectTimings) noexcept;

	void _EnableSrcWnd(bool enable) noexcept;

	const std::string& _GetResourceString(const std::wstring_view& key) noexcept;

	float _dpiScale = 1.0f;

	ImFont* _fontUI = nullptr;	// 普通 UI 文字
	ImFont* _fontMonoNumbers = nullptr;	// 普通 UI 文字，但数字部分是等宽的，只支持 ASCII
	ImFont* _fontFPS = nullptr;	// FPS

	std::deque<float> _frameTimes;
	uint32_t _validFrames = 0;

	SmallVector<uint32_t> _timelineColors;

	struct {
		std::string gpuName;
	} _hardwareInfo;

	ImGuiImpl _imguiImpl;

	winrt::ResourceLoader _resourceLoader{ nullptr };

	bool _isUIVisiable = false;
	bool _isSrcMainWnd = false;

	bool _isFirstFrame = true;
};

}
