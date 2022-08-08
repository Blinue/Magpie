#include "pch.h"
#include "LoggerHelper.h"


namespace Magpie::Runtime {

void LoggerHelper::Initialize(Logger& logger) {
	Logger::Get().Initialize(logger);
}

}
