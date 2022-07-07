#include "pch.h"
#include "NewProfileDialog.h"
#if __has_include("NewProfileDialog.g.cpp")
#include "NewProfileDialog.g.cpp"
#endif
#include "Win32Utils.h"
#include "Utils.h"
#include "ComboBoxHelper.h"


namespace winrt::Magpie::App::implementation {

bool IsCandidateWindow(HWND hWnd) {
	if (!IsWindowVisible(hWnd)) {
		return false;
	}

	// The window must not be cloaked by the shell
	UINT isCloaked{};
	DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &isCloaked, sizeof(isCloaked));
	if (isCloaked == DWM_CLOAKED_SHELL) {
		return false;
	}

	RECT frameRect;
	if (!Win32Utils::GetWindowFrameRect(hWnd, frameRect)) {
		return false;
	}

	SIZE frameSize = Win32Utils::GetSizeOfRect(frameRect);
	if (frameSize.cx < 50 && frameSize.cy < 50) {
		return false;
	}

	return true;
}

std::vector<HWND> GetDesktopWindows() {
	std::vector<HWND> windows;
	windows.reserve(1024);

	EnumChildWindows(
		GetDesktopWindow(),
		[](HWND hWnd, LPARAM lParam) {
			std::vector<HWND>& windows = *(std::vector<HWND>*)lParam;

			if (IsCandidateWindow(hWnd)) {
				windows.push_back(hWnd);
			}

			return TRUE;
		},
		(LPARAM)&windows
	);

	return windows;
}

NewProfileDialog::NewProfileDialog() {
	InitializeComponent();

	std::vector<IInspectable> candidateWindows;

	for (HWND hWnd : GetDesktopWindows()) {
		std::wstring title = Win32Utils::GetWndTitle(hWnd);
		if (title.empty()) {
			continue;
		}

		candidateWindows.emplace_back(box_value(title));
	}

	_candidateWindows = single_threaded_observable_vector(std::move(candidateWindows));
}

void NewProfileDialog::ComboBox_DropDownOpened(IInspectable const& sender, IInspectable const&) {
	ComboBoxHelper::DropDownOpened(*this, sender);
}

}
