#include "pch.h"
#include "Settings.h"
#if __has_include("Settings.g.cpp")
#include "Settings.g.cpp"
#endif

#include "Utils.h"


namespace winrt::Magpie::App::implementation {

Settings::Settings() {
	
}

bool Settings::IsPortableMode() const {
	return false;
}

}
