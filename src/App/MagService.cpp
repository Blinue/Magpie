#include "pch.h"
#include "MagService.h"
#if __has_include("MagService.g.cpp")
#include "MagService.g.cpp"
#endif

namespace winrt::Magpie::App::implementation {

MagService::MagService(
	Magpie::App::Settings const& settings,
	Magpie::Runtime::MagRuntime const& magRuntime
) : _settings(settings), _magRuntime(magRuntime) {
}

}
