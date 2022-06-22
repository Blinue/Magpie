#include "pch.h"
#include "MagRuntime.h"
#if __has_include("MagRuntime.g.cpp")
#include "MagRuntime.g.cpp"
#endif


namespace winrt::Magpie::Runtime::implementation {

bool MagRuntime::Scale(uint64_t hwndSrc, MagSettings const& settings) {
	UNREFERENCED_PARAMETER(hwndSrc);
	UNREFERENCED_PARAMETER(settings);
	return false;
}

bool MagRuntime::IsRunning() const {
	return false;
}

}
