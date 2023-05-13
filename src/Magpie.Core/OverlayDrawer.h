#pragma once
#include <deque>
#include "SmallVector.h"
#include <imgui.h>

namespace Magpie::Core {

struct EffectDesc;
class ImGuiImpl;

class OverlayDrawer {
public:
	OverlayDrawer() noexcept;
	OverlayDrawer(const OverlayDrawer&) = delete;
	OverlayDrawer(OverlayDrawer&&) = delete;

	~OverlayDrawer();

	bool Initialize() noexcept;

	void Draw() noexcept;

	bool IsUIVisiable() const noexcept {
		return _isUIVisiable;
	}

	void SetUIVisibility(bool value) noexcept;

private:
	bool _BuildFonts() noexcept;

	struct _EffectTimings {
		const EffectDesc* desc = nullptr;
		std::span<const float> passTimings;
		float totalTime = 0.0f;
	};

	int _DrawEffectTimings(const _EffectTimings& et, bool showPasses, float maxWindowWidth, std::span<const ImColor> colors, bool singleEffect) noexcept;

	void _DrawTimelineItem(ImU32 color, float dpiScale, std::string_view name, float time, float effectsTotalTime, bool selected = false);

	void _DrawFPS() noexcept;

	void _DrawUI() noexcept;

	void _RetrieveHardwareInfo() noexcept;

	void _EnableSrcWnd(bool enable) noexcept;

	float _dpiScale = 1.0f;

	ImFont* _fontUI = nullptr;	// 普通 UI 文字
	ImFont* _fontMonoNumbers = nullptr;	// 普通 UI 文字，但数字部分是等宽的，只支持 ASCII
	ImFont* _fontFPS = nullptr;	// FPS

	std::deque<float> _frameTimes;
	UINT _validFrames = 0;

	SmallVector<UINT> _timelineColors;

	struct {
		std::string gpuName;
	} _hardwareInfo;

	std::unique_ptr<ImGuiImpl> _imguiImpl;

	winrt::ResourceLoader _resourceLoader = winrt::ResourceLoader::GetForViewIndependentUse();

	bool _isUIVisiable = false;
	bool _isSrcMainWnd = false;
};

}
