#pragma once
#include "pch.h"
#include "Logger.h"


namespace Magpie::Runtime {

struct API_DECLSPEC LoggerHelper {
	static void Initialize(Logger& logger);
};

}
