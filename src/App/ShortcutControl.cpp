#include "pch.h"
#include "ShortcutControl.h"
#if __has_include("ShortcutControl.g.cpp")
#include "ShortcutControl.g.cpp"
#endif

using namespace winrt;


namespace winrt::Magpie::implementation {

ShortcutControl::ShortcutControl() {
	InitializeComponent();
}

}
