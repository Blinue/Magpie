#pragma once
#include "pch.h"


// 所有可在屏幕上渲染的类的基类
class Renderable {
public:
	Renderable() {
	}

	virtual ~Renderable() {}

	virtual void Render() = 0;
};
