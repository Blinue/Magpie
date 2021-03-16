#pragma once
#include "pch.h"

class Renderable {
public:
	Renderable(ID2D1DeviceContext* d2dDC): _d2dDC(d2dDC) {
	}

	virtual ~Renderable() {}

	virtual void Render() = 0;
protected:
	ID2D1DeviceContext* _d2dDC;
};
