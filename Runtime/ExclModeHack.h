#pragma once
#include "pch.h"
#include "Utils.h"


class ExclModeHack {
public:
	ExclModeHack();

	~ExclModeHack();

private:
	Utils::ScopedHandle _exclModeMutex;
};

