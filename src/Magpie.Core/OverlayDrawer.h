#pragma once
#include <deque>
#include "SmallVector.h"
#include <imgui.h>

namespace Magpie::Core {

struct EffectDesc;
class ImGuiImpl;

class OverlayDrawer {
public:
	OverlayDrawer();
	OverlayDrawer(const OverlayDrawer&) = delete;
	OverlayDrawer(OverlayDrawer&&) = delete;

	~OverlayDrawer();

	bool Initialize();

	void Draw();

	bool IsUIVisiable() const noexcept {
		return _isUIVisiable;
	}

	void SetUIVisibility(bool value);

private:
	bool _BuildFonts() noexcept;

	struct _EffectTimings {
		const EffectDesc* desc = nullptr;
		std::span<const float> passTimings;
		float totalTime = 0.0f;
	};

	int _DrawEffectTimings(const _EffectTimings& et, bool showPasses, float maxWindowWidth, std::span<const ImColor> colors, bool singleEffect) noexcept;

	void _DrawTimelineItem(ImU32 color, float dpiScale, std::string_view name, float time, float effectsTotalTime, bool selected = false);

	void _DrawFPS();

	void _DrawUI();

	void _RetrieveHardwareInfo();

	void _EnableSrcWnd(bool enable);

	float _dpiScale = 1.0f;

	ImFont* _fontUI = nullptr;	// 普通 UI 文字
	ImFont* _fontMonoNumbers = nullptr;	// 普通 UI 文字，但数字部分是等宽的
	ImFont* _fontFPS = nullptr;	// FPS

	std::deque<float> _frameTimes;
	UINT _validFrames = 0;

	SmallVector<UINT> _timelineColors;

	struct {
		std::string gpuName;
		std::string cpuName;
	} _hardwareInfo;

	std::unique_ptr<ImGuiImpl> _imguiImpl;

	bool _isUIVisiable = false;
	bool _isSrcMainWnd = false;
};

}
