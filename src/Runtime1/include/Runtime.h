#pragma once
#include "CommonPCH.h"
#include "MagOptions.h"

#ifndef API_DECLSPEC
#define API_DECLSPEC __declspec(dllimport)
#endif // !API_DECLSPEC


namespace Magpie::Runtime {

class API_DECLSPEC MagRuntime {
public:
	HWND HwndSrc() const;

private:
	HWND _hwndSrc = NULL;
};

}
