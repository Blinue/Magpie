#include "pch.h"
#include "Dummy.h"
#if __has_include("Controls.Dummy.g.cpp")
#include "Controls.Dummy.g.cpp"
#endif

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Automation::Peers;


namespace winrt::Magpie::App::Controls::implementation {

Dummy::Dummy() {
	DefaultStyleKey(box_value(name_of<Magpie::App::Controls::Dummy>()));
}

}
