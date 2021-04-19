#pragma once
#include "pch.h"
#include "D2DContext.h"

class Renderable {
public:
	Renderable(D2DContext& d2dContext): _d2dContext(d2dContext) {
	}

	virtual ~Renderable() {}

	virtual void Render() = 0;

protected:
	D2DContext& _d2dContext;
};
