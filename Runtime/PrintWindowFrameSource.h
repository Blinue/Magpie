#pragma once
#include "pch.h"
#include "GDIFrameSource.h"


class PrintWindowFrameSource : public GDIFrameSource {
public:
	PrintWindowFrameSource() {}

	virtual ~PrintWindowFrameSource() {}

	bool Update() override;

	bool HasRoundCornerInWin11() override {
		return true;
	}
};

