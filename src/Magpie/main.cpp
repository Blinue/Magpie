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
#include "Win32Utils.h"

// 将当前目录设为程序所在目录
static void SetWorkingDir() noexcept {
	std::wstring exePath = Win32Utils::GetExePath();

	FAIL_FAST_IF_FAILED(PathCchRemoveFileSpec(
		exePath.data(),
		exePath.size() + 1
	));

	FAIL_FAST_IF_WIN32_BOOL_FALSE(SetCurrentDirectory(exePath.c_str()));
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

	SetWorkingDir();

	auto& app = Magpie::XamlApp::Get();
	if (!app.Initialize(hInstance, lpCmdLine)) {
		return 0;
	}

	return app.Run();
}
