#pragma once
#include "pch.h"


class Renderable {
public:
	Renderable() {
	}

	virtual ~Renderable() {}

	virtual void Render() = 0;
};
