#pragma once
#include "pch.h"
#include <deque>
#include "SmallVector.h"


struct ImFont;

namespace Magpie::Core {

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
	void _DrawFPS();

	void _DrawUI();

	void _RetrieveHardwareInfo();

	void _EnableSrcWnd(bool enable);

	float _dpiScale = 1.0f;

	ImFont* _fontUI = nullptr;
	ImFont* _fontFPS = nullptr;

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
