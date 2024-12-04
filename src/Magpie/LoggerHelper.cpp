#include "pch.h"
#include "LoggerHelper.h"
#if __has_include("LoggerHelper.g.cpp")
#include "LoggerHelper.g.cpp"
#endif
#include "Logger.h"


namespace winrt::Magpie::implementation {

void LoggerHelper::Initialize(uint64_t pLogger) {
	Logger::Get().Initialize(*(Logger*)pLogger);
}

}
