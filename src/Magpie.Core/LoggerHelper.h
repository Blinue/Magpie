#pragma once
#include "ExportHelper.h"
#include "Logger.h"

namespace Magpie::Core {

struct API_DECLSPEC LoggerHelper {
	static void Initialize(Logger& logger);
};

}
