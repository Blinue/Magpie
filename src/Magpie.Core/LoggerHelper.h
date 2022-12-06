#pragma once
#include "ExportHelper.h"

class Logger;

namespace Magpie::Core {

struct API_DECLSPEC LoggerHelper {
	static void Initialize(Logger& logger);
};

}
