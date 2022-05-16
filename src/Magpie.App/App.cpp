#include "pch.h"
#include "App.h"
#if __has_include("App.g.cpp")
#include "App.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::Magpie::App::implementation {

App::App() {
	Initialize();

	AddRef();
	m_inner.as<::IUnknown>()->Release();
}

App::~App() {
	Close();
}

}
