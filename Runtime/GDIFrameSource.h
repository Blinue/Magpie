#pragma once
#include "pch.h"
#include "FrameSourceBase.h"


class GDIFrameSource : public FrameSourceBase {
public:
	GDIFrameSource() {};
	virtual ~GDIFrameSource() {}

	bool Initialize() override;

	UpdateState Update() override;

	bool IsScreenCapture() override {
		return false;
	}

	const char* GetName() const noexcept override {
		return "GDI";
	}

protected:
	bool _HasRoundCornerInWin11() override {
		return false;
	}

private:
	RECT _frameRect{};
	winrt::com_ptr<IDXGISurface1> _dxgiSurface;
};
