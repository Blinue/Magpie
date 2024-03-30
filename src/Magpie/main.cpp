// Copyright (c) 2021 - present, Liu Xu
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "pch.h"
#include "XamlApp.h"
#include "StrUtils.h"

// 将当前目录设为程序所在目录
static std::wstring SetCurDir() noexcept {
	std::wstring curDir(MAX_PATH, L'\0');
	curDir.resize(GetModuleFileName(NULL, curDir.data(), MAX_PATH));

	int i = (int)curDir.size() - 1;
	for (; i >= 0; --i) {
		if (curDir[i] == L'\\' || curDir[i] == L'/') {
			break;
		} else {
			curDir[i] = L'\0';
		}
	}
	curDir.resize(i);

	SetCurrentDirectory(curDir.c_str());
	return curDir;
}

static void IncreaseTimerResolution() noexcept {
	// 我们需要尽可能高的时钟分辨率来提高渲染帧率。
	// 通常 Magpie 被 OS 认为是后台进程，下面的调用避免 OS 自动降低时钟分辨率。
	// 见 https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessinformation
	PROCESS_POWER_THROTTLING_STATE powerThrottling{
		.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION,
		.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED |
					   PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION,
		.StateMask = 0
	};
	SetProcessInformation(
		GetCurrentProcess(),
		ProcessPowerThrottling,
		&powerThrottling,
		sizeof(powerThrottling)
	);
}

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ wchar_t* lpCmdLine,
	_In_ int /*nCmdShow*/
) {
#ifdef _DEBUG
	SetThreadDescription(GetCurrentThread(), L"Magpie 主线程");
#endif

	// 堆损坏时终止进程
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, nullptr, 0);

	// 提高时钟分辨率
	IncreaseTimerResolution();

	// 程序结束时也不应调用 uninit_apartment
	// 见 https://kennykerr.ca/2018/03/24/cppwinrt-hosting-the-windows-runtime/
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	std::wstring curDir = SetCurDir();

	SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	curDir += L"\\third_party";
	AddDllDirectory(curDir.c_str());

	auto& app = Magpie::XamlApp::Get();
	if (!app.Initialize(hInstance, lpCmdLine)) {
		return -1;
	}

	return app.Run();
}
