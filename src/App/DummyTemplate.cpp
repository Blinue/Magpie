#include "pch.h"
#include "DummyTemplate.h"
#if __has_include("DummyTemplate.g.cpp")
#include "DummyTemplate.g.cpp"
#endif

using namespace winrt;


namespace winrt::Magpie::App::implementation {

DummyTemplate::DummyTemplate() {
	DefaultStyleKey(box_value(GetRuntimeClassName()));
}

}
