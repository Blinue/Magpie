#include "pch.h"
#include "LoggerHelper.h"


namespace Magpie::Core {

void LoggerHelper::Initialize(Logger& logger) {
	Logger::Get().Initialize(logger);
}

}
