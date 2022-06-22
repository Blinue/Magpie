#pragma once
#include "pch.h"
#include "Win32Utils.h"


class ExclModeHack {
public:
	ExclModeHack();

	~ExclModeHack();

private:
	Win32Utils::ScopedHandle _exclModeMutex;
};
