#pragma once

#include "LoggerHelper.g.h"

namespace winrt::Magpie::Runtime::implementation {

struct LoggerHelper : LoggerHelperT<LoggerHelper> {
    static void Initialize(uint64_t pLogger);
};

}

namespace winrt::Magpie::Runtime::factory_implementation {

struct LoggerHelper : LoggerHelperT<LoggerHelper, implementation::LoggerHelper> {
};

}
