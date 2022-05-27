#include "pch.h"
#include "Dummy.h"
#if __has_include("Dummy.g.cpp")
#include "Dummy.g.cpp"
#endif

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Automation::Peers;


namespace winrt::Magpie::App::implementation {

Dummy::Dummy() {
	DefaultStyleKey(box_value(name_of<Magpie::App::Dummy>()));
}

}
