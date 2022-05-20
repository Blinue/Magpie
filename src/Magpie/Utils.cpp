#include "pch.h"
#include "Utils.h"

UINT Utils::GetOSBuild() {
	static UINT build = 0;

	if (build == 0) {
		HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
		if (!hNtDll) {
			return {};
		}

		auto rtlGetVersion = (LONG(WINAPI*)(PRTL_OSVERSIONINFOW))GetProcAddress(hNtDll, "RtlGetVersion");
		if (rtlGetVersion == nullptr) {
			return {};
		}

		OSVERSIONINFOW version{};
		version.dwOSVersionInfoSize = sizeof(version);
		rtlGetVersion(&version);

		build = version.dwBuildNumber;
	}

	return build;
}