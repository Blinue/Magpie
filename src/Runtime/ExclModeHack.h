#pragma once
#include "pch.h"
#include "Win32Utils.h"


namespace Magpie::Runtime {

class ExclModeHack {
public:
	ExclModeHack();

	~ExclModeHack();

private:
	Win32Utils::ScopedHandle _exclModeMutex;
};

}
