#pragma once
#include "MagService.g.h"
#include <winrt/Magpie.Runtime.h>


namespace winrt::Magpie::App::implementation {

struct MagService : MagServiceT<MagService> {
	MagService(Magpie::App::Settings const& settings, Magpie::Runtime::MagRuntime const& magRuntime);

private:
	Magpie::App::Settings _settings{ nullptr };
	Magpie::Runtime::MagRuntime _magRuntime{ nullptr };
};

}

namespace winrt::Magpie::App::factory_implementation {

struct MagService : MagServiceT<MagService, implementation::MagService> {
};

}
