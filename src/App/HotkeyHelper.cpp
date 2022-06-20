#include "pch.h"
#include "HotkeyHelper.h"


namespace winrt {

using namespace Magpie::App;

hstring to_hstring(HotkeyAction action) {
	switch (action) {
	case HotkeyAction::Scale:
		return L"Scale";
	case HotkeyAction::Overlay:
		return L"Overlay";
	case HotkeyAction::COUNT_OR_NONE:
		return L"None";
	default:
		break;
	}

	return {};
}

std::string Magpie::App::to_string(HotkeyAction action) {
	switch (action) {
	case HotkeyAction::Scale:
		return "Scale";
	case HotkeyAction::Overlay:
		return "Overlay";
	case HotkeyAction::COUNT_OR_NONE:
		return "None";
	default:
		break;
	}

	return {};
}

}
