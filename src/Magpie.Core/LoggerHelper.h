#pragma once
#include "pch.h"
#include "Logger.h"


namespace Magpie::Core {

struct API_DECLSPEC LoggerHelper {
	static void Initialize(Logger& logger);
};

}
