#pragma once
#include "pch.h"
#include <deque>

struct ImFont;

class OverlayDrawer {
public:
	OverlayDrawer() = default;
	OverlayDrawer(const OverlayDrawer&) = delete;
	OverlayDrawer(OverlayDrawer&&) = delete;

	~OverlayDrawer();

	bool Initialize(ID3D11Texture2D* renderTarget);

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

	UINT _handlerID = 0;
	ID3D11RenderTargetView* _rtv = nullptr;
	bool _isUIVisiable = false;

	ImFont* _fontUI = nullptr;
	ImFont* _fontFPS = nullptr;

	std::deque<float> _frameTimes;
	UINT _validFrames = 0;

	struct {
		std::string gpuName;
		std::string cpuName;
	} _hardwareInfo;
};
