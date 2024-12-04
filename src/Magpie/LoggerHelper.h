#pragma once
#include "LoggerHelper.g.h"

namespace winrt::Magpie::implementation {

struct LoggerHelper {
    static void Initialize(uint64_t pLogger);
};

}

namespace winrt::Magpie::factory_implementation {

struct LoggerHelper : LoggerHelperT<LoggerHelper, implementation::LoggerHelper> {
};

}
