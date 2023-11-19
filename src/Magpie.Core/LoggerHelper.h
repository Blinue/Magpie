#pragma once

class Logger;

namespace Magpie::Core {

struct LoggerHelper {
	static void Initialize(Logger& logger);
};

}
