#include "pch.h"
#include "DummyTemplate.h"
#if __has_include("DummyTemplate.g.cpp")
#include "DummyTemplate.g.cpp"
#endif

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Automation::Peers;


namespace winrt::Magpie::implementation {

DummyTemplate::DummyTemplate() {
	DefaultStyleKey(box_value(name_of<Magpie::DummyTemplate>()));
}

}
