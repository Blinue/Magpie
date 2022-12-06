#pragma once
#include "CommonPch.h"

#ifdef MAGPIE_CORE_EXPORTS
#define API_DECLSPEC __declspec(dllexport)
#else
#define API_DECLSPEC __declspec(dllimport)
#endif
