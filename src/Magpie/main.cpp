// Copyright (c) Xu
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
#include "App.h"
#include "AppFolderManager.h"
#include "CommonSharedConstants.h"
#include "DebugInfo.h"
#include "Logger.h"
#include "TouchHelper.h"
#include "Win32Helper.h"
#ifdef _DEBUG
#include <d3d12sdklayers.h>
#endif
#include <dxgi1_6.h>

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 619; }
// D3D12 相关 dll 不能放在 dll 搜索目录，否则如果 OS 的 D3D12 运行时更新将会错误
// 加载随程序部署的旧版本依赖 dll（包括 D3D12SDKLayers.dll 和 d3d10warp.dll）。
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\app\\D3D12"; }

using namespace Magpie;
using namespace winrt::Magpie::implementation;

static void InitializeLogger(bool touchHelper) noexcept {
	// 最多两个日志文件，每个最多 500KB
	Logger::Get().Initialize(
		spdlog::level::info,
		StrHelper::Concat(AppFolderManager::Get().GetLogsDir(), L"\\", touchHelper ?
			CommonSharedConstants::TOUCH_HELPER_LOG_NAME : CommonSharedConstants::LOG_NAME),
		CommonSharedConstants::MAX_LOG_SIZE,
		1
	);
}

static void InitializeDirectX() noexcept {
#ifdef _DEBUG
	{
		winrt::com_ptr<ID3D12Debug1> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
			debugController->EnableDebugLayer();
			
#ifdef MP_DEBUG_INFO
			if (DEBUG_INFO.enableGPUBasedValidation) {
				// 会产生警告消息
				debugController->SetEnableGPUBasedValidation(TRUE);
			}
#endif

			// Win11 开始支持生成默认名字，包含资源的基本属性
			if (winrt::com_ptr<ID3D12Debug5> debugController5 = debugController.try_as<ID3D12Debug5>()) {
				debugController5->SetEnableAutoName(TRUE);
			}
		}
	}
#endif

	// 声明支持 TDR 恢复
	DXGIDeclareAdapterRemovalSupport();
}

int APIENTRY wWinMain(
	_In_ HINSTANCE /*hInstance*/,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ wchar_t* lpCmdLine,
	_In_ int /*nCmdShow*/
) {
#ifdef _DEBUG
	SetThreadDescription(GetCurrentThread(), L"Magpie-主线程");
#endif
	
	// 堆损坏时终止进程
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, nullptr, 0);

	FAIL_FAST_IF(!AppFolderManager::Get().Initialize());
	
	enum {
		Normal,
		RegisterTouchHelper,
		UnRegisterTouchHelper
	} mode = [&]() {
		if (lpCmdLine == L"-r"sv) {
			return RegisterTouchHelper;
		} else if (lpCmdLine == L"-ur"sv) {
			return UnRegisterTouchHelper;
		} else {
			return Normal;
		}
	}();

	InitializeLogger(mode != Normal);

	Logger::Get().Info(fmt::format("程序启动\n\t版本: {}\n\tOS 版本: {}\n\t管理员: {}\n\t便携模式: {}",
#ifdef MP_VERSION_STRING
		STRINGIFY(MP_VERSION_STRING),
#elif defined(MP_COMMIT_ID)
		"dev (" STRINGIFY(MP_COMMIT_ID) ")",
#else
		"dev",
#endif
		Win32Helper::GetOSVersion().ToString<char>(),
		Win32Helper::IsProcessElevated() ? "是" : "否",
		AppFolderManager::Get().IsPortableMode() ? "是" : "否"
	));

	if (mode == RegisterTouchHelper) {
		// 使 TouchHelper 获得 UIAccess 权限
		return Magpie::TouchHelper::Register() ? 0 : 1;
	} else if (mode == UnRegisterTouchHelper) {
		return Magpie::TouchHelper::Unregister() ? 0 : 1;
	}

	winrt::init_apartment(winrt::apartment_type::single_threaded);

	InitializeDirectX();

	auto& app = App::Get();
	if (!app.Initialize(lpCmdLine)) {
		return 0;
	}

	return app.Run();
}
