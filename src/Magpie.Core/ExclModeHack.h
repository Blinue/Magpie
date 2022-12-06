#pragma once
#include "Win32Utils.h"

namespace Magpie::Core {

class ExclModeHack {
public:
	ExclModeHack();

	~ExclModeHack();

private:
	Win32Utils::ScopedHandle _exclModeMutex;
};

}
