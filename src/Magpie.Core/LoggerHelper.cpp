#include "pch.h"
#include "LoggerHelper.h"
#include "Logger.h"

namespace Magpie::Core {

void LoggerHelper::Initialize(Logger& logger) {
	Logger::Get().Initialize(logger);
}

}
