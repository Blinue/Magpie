#include "pch.h"
#include "Shortcut.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include "ShortcutHelper.h"
#include "SmallVector.h"

namespace winrt::Magpie {

bool Shortcut::IsEmpty() const noexcept {
	return !win && !ctrl && !alt && !shift && code == 0;
}

void Shortcut::Clear() noexcept {
	win = false;
	ctrl = false;
	alt = false;
	shift = false;
	code = 0;
}

std::wstring Shortcut::ToString() const noexcept {
	std::wstring output;

	if (win) {
		output.append(L"Win+");
	}

	if (ctrl) {
		output.append(L"Ctrl+");
	}

	if (alt) {
		output.append(L"Alt+");
	}

	if (shift) {
		output.append(L"Shift+");
	}

	if (code > 0) {
		output.append(Win32Utils::GetKeyName(code));
	} else if (output.size() > 1) {
		output.pop_back();
	}

	return output;
}

}
