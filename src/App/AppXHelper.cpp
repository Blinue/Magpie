#include "pch.h"
#include "AppXHelper.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include "Utils.h"
#include "Logger.h"
#include <appmodel.h>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")


namespace winrt::Magpie::App {

std::wstring AppXHelper::GetPackageFullName(HWND hWnd) {
	if (Win32Utils::GetWndClassName(hWnd) == L"ApplicationFrameWindow") {
		EnumChildWindows(
			hWnd,
			[](HWND hWnd, LPARAM lParam) {
				if (Win32Utils::GetWndClassName(hWnd) != L"Windows.UI.Core.CoreWindow") {
					return TRUE;
				}

				*(HWND*)lParam = hWnd;
				return FALSE;
			},
			(LPARAM)&hWnd
		);
	}

	DWORD dwProcId = 0;
	if (!GetWindowThreadProcessId(hWnd, &dwProcId)) {
		Logger::Get().Win32Error("GetWindowThreadProcessId 失败");
		return {};
	}

	Win32Utils::ScopedHandle hProc(Win32Utils::SafeHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcId)));
	if (!hProc) {
		Logger::Get().Win32Error("OpenProcess 失败");
		return {};
	}

	UINT32 length = 0;
	if (::GetPackageFullName(hProc.get(), &length, nullptr) == APPMODEL_ERROR_NO_PACKAGE) {
		// 不是打包应用
		return {};
	}

	std::wstring result(length - 1, 0);
	if (::GetPackageFullName(hProc.get(), &length, result.data()) != ERROR_SUCCESS) {
		return {};
	}

	return result;
}

// 移植自 https://github.com/microsoft/PowerToys/blob/c36a80dad571db26d6cf9e40e70099815ed56049/src/modules/launcher/Plugins/Microsoft.Plugin.Program/Programs/UWPApplication.cs#L299
static std::wstring ResourceFromPri(std::wstring_view packageFullName, std::wstring_view resourceReference) {
	std::wstring_view prefix = L"ms-resource:";

	StrUtils::Trim(resourceReference);

	std::wstring lowerCaseResourceReference = StrUtils::ToLowerCase(resourceReference);
	
	// Using OrdinalIgnoreCase since this is used internally
	if (!lowerCaseResourceReference.starts_with(prefix)) {
		return std::wstring(resourceReference);
	}
	
	// magic comes from @talynone
	// https://github.com/talynone/Wox.Plugin.WindowsUniversalAppLauncher/blob/master/StoreAppLauncher/Helpers/NativeApiHelper.cs#L139-L153
	std::wstring_view key = resourceReference.substr(prefix.size());
	std::wstring_view lowerCaseKey(
		lowerCaseResourceReference.c_str() + prefix.size(),
		lowerCaseResourceReference.size() - prefix.size()
	);
	std::wstring parsed;
	std::wstring parsedFallback;

	// Using Ordinal/OrdinalIgnoreCase since these are used internally
	if (key.starts_with(L"//")) {
		parsed = StrUtils::ConcatW(prefix, key);
	} else if (key.starts_with(L'/')) {
		parsed = StrUtils::ConcatW(prefix, L"//", key);
	} else if (lowerCaseKey.starts_with(L"resources")) {
		parsed = StrUtils::ConcatW(prefix, key);
	} else {
		parsed = StrUtils::ConcatW(prefix, L"///resources/", key);

		// e.g. for Windows Terminal version >= 1.12 DisplayName and Description resources are not in the 'resources' subtree
		parsedFallback = StrUtils::ConcatW(prefix, L"///", key);
	}
	
	std::wstring source = fmt::format(L"@{{{}? {}}}", packageFullName, parsed);
	
	std::wstring result(128, 0);
	HRESULT hr = SHLoadIndirectString(source.c_str(), result.data(), (UINT)result.size() + 1, nullptr);
	if (FAILED(hr)) {
		return {};
	}

	result.resize(StrUtils::StrLen(result.c_str()));
	return result;
}

bool AppXHelper::AppXReader::Initialize(HWND hWnd) noexcept {
	if (Win32Utils::GetWndClassName(hWnd) == L"ApplicationFrameWindow") {
		// UWP 应用被托管在 ApplicationFrameHost 进程中
		EnumChildWindows(
			hWnd,
			[](HWND hWnd, LPARAM lParam) {
				if (Win32Utils::GetWndClassName(hWnd) != L"Windows.UI.Core.CoreWindow") {
					return TRUE;
				}

				*(HWND*)lParam = hWnd;
				return FALSE;
			},
			(LPARAM)&hWnd
		);
	}

	DWORD dwProcId = 0;
	if (!GetWindowThreadProcessId(hWnd, &dwProcId)) {
		Logger::Get().Win32Error("GetWindowThreadProcessId 失败");
		return false;
	}

	Win32Utils::ScopedHandle hProc(Win32Utils::SafeHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcId)));
	if (!hProc) {
		Logger::Get().Win32Error("OpenProcess 失败");
		return false;
	}

	UINT32 length = 0;
	if (GetApplicationUserModelId(hProc.get(), &length, nullptr) == APPMODEL_ERROR_NO_APPLICATION || length == 0) {
		// 不是打包应用
		return false;
	}

	std::wstring aumid(length - 1, 0);
	if (GetApplicationUserModelId(hProc.get(), &length, aumid.data()) != ERROR_SUCCESS) {
		return false;
	}

	UINT pfnLen = 0, praidLen = 0;
	if (ParseApplicationUserModelId(aumid.c_str(), &pfnLen, nullptr, &praidLen, nullptr) != ERROR_INSUFFICIENT_BUFFER || pfnLen == 0 || praidLen == 0) {
		return false;
	}

	// 不使用 packageFamilyName
	std::wstring packageFamilyName(pfnLen - 1, 0);
	std::wstring praid(praidLen - 1, 0);
	if (ParseApplicationUserModelId(aumid.c_str(), &pfnLen, packageFamilyName.data(), &praidLen, praid.data()) != ERROR_SUCCESS) {
		return false;
	}

	length = 0;
	if (::GetPackageFullName(hProc.get(), &length, nullptr) == APPMODEL_ERROR_NO_PACKAGE) {
		return false;
	}

	_packageFullName.resize((size_t)length - 1);
	if (::GetPackageFullName(hProc.get(), &length, _packageFullName.data()) != ERROR_SUCCESS) {
		return false;
	}

	return _ResolveApplication(praid);
}

std::wstring AppXHelper::AppXReader::GetDisplayName() const noexcept {
	if (!_appxApp) {
		return {};
	}

	wchar_t* value = nullptr;
	if (FAILED(_appxApp->GetStringValue(L"DisplayName", &value)) || !value) {
		return {};
	}

	std::wstring result = ResourceFromPri(_packageFullName, value);
	CoTaskMemFree(value);
	return result;
}

bool AppXHelper::AppXReader::_ResolveApplication(const std::wstring& praid) noexcept {
	PACKAGE_INFO_REFERENCE infoRef{};
	if (OpenPackageInfoByFullName(_packageFullName.c_str(), 0, &infoRef) != ERROR_SUCCESS) {
		Logger::Get().Error("OpenPackageInfoByFullName 失败");
		return false;
	}

	UINT bufferSize = 0;
	GetPackageInfo(infoRef, PACKAGE_FILTER_HEAD, &bufferSize, nullptr, nullptr);
	if (bufferSize == 0) {
		return false;
	}

	std::unique_ptr<uint8_t[]> packageInfoBuffer(new uint8_t[bufferSize]);
	UINT packageCount = 0;
	if (GetPackageInfo(infoRef, PACKAGE_FILTER_HEAD, &bufferSize, packageInfoBuffer.get(), &packageCount) != ERROR_SUCCESS) {
		Logger::Get().Error("GetPackageInfo 失败");
		ClosePackageInfo(infoRef);
		return false;
	}

	ClosePackageInfo(infoRef);

	com_ptr<IAppxFactory> factory;

	HRESULT hr = CoCreateInstance(
		CLSID_AppxFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&factory));
	if (FAILED(hr)) {
		Logger::Get().ComError("创建 IAppxFactory 失败", hr);
		return false;
	}

	for (UINT32 i = 0; i < packageCount; ++i) {
		const PACKAGE_INFO& packageInfo = ((PACKAGE_INFO*)packageInfoBuffer.get())[i];
		std::wstring manifestPath = StrUtils::ConcatW(packageInfo.path, L"\\AppXManifest.xml");

		com_ptr<IStream> inputStream;
		hr = SHCreateStreamOnFileEx(
			manifestPath.c_str(),
			STGM_READ | STGM_SHARE_DENY_WRITE,
			0,
			FALSE,
			nullptr,
			inputStream.put()
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("打开 AppXManifest.xml 失败", hr);
			continue;
		}

		com_ptr<IAppxManifestReader> manifestReader;
		hr = factory->CreateManifestReader(
			inputStream.get(),
			manifestReader.put()
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateManifestReader 失败", hr);
			continue;
		}

		com_ptr<IAppxManifestApplicationsEnumerator> appEnumerator;
		hr = manifestReader->GetApplications(appEnumerator.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("GetApplications 失败", hr);
			continue;
		}

		BOOL hasCurrent = FALSE;
		hr = appEnumerator->GetHasCurrent(&hasCurrent);

		while (SUCCEEDED(hr) && hasCurrent) {
			com_ptr<IAppxManifestApplication> appxApp;
			if (FAILED(appEnumerator->GetCurrent(appxApp.put()))) {
				break;
			}

			wchar_t* curPraid = nullptr;
			if (FAILED(appxApp->GetStringValue(L"Id", &curPraid))) {
				break;
			}

			if (curPraid) {
				if (praid == curPraid) {
					_appxApp = std::move(appxApp);
					CoTaskMemFree(curPraid);
					return true;
				}

				CoTaskMemFree(curPraid);
			}
			
			hr = appEnumerator->MoveNext(&hasCurrent);
		}
	}

	// 未找到 Id 为 praid 的应用
	return false;
}

}
