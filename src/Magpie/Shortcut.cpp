#include "pch.h"
#include "Shortcut.h"
#include "Win32Helper.h"
#include "StrHelper.h"

namespace Magpie {

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

std::string Shortcut::ToString() const noexcept {
	std::string output;

	if (win) {
		output.append("Win+");
	}

	if (ctrl) {
		output.append("Ctrl+");
	}

	if (alt) {
		output.append("Alt+");
	}

	if (shift) {
		output.append("Shift+");
	}

	if (code > 0) {
		output.append(StrHelper::UTF16ToUTF8(Win32Helper::GetKeyName(code)));
	} else if (output.size() > 1) {
		output.pop_back();
	}

	return output;
}

}
