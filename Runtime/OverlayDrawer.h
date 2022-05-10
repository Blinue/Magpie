#pragma once
#include "pch.h"
#include <deque>

struct ImFont;
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

	float _dpiScale = 1.0f;

	bool _isUIVisiable = false;

	ImFont* _fontUI = nullptr;
	ImFont* _fontFPS = nullptr;

	std::deque<float> _frameTimes;
	UINT _validFrames = 0;

	std::vector<UINT> _timelineColors;

	struct {
		std::string gpuName;
		std::string cpuName;
	} _hardwareInfo;

	std::unique_ptr<ImGuiImpl> _imguiImpl;
};
