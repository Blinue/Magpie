#include "pch.h"
#include "AppXHelper.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include "Utils.h"
#include "Logger.h"
#include <appmodel.h>
#include <Shlwapi.h>
#include <shellapi.h>
#include <propkey.h>

#pragma comment(lib, "Shlwapi.lib")

using namespace winrt;
using namespace Windows::UI::Xaml::Media::Imaging;


namespace winrt::Magpie::App {

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
	
	std::wstring parsed;
	std::wstring parsedFallback;

	// Using Ordinal/OrdinalIgnoreCase since these are used internally
	if (key.starts_with(L"//")) {
		parsed = StrUtils::ConcatW(prefix, key);
	} else if (key.starts_with(L'/')) {
		parsed = StrUtils::ConcatW(prefix, L"//", key);
	} else {
		std::wstring_view lowerCaseKey(
			lowerCaseResourceReference.begin() + prefix.size(),
			lowerCaseResourceReference.end()
		);

		if (lowerCaseKey.starts_with(L"resources")) {
			parsed = StrUtils::ConcatW(prefix, key);
		} else {
			parsed = StrUtils::ConcatW(prefix, L"///resources/", key);

			// e.g. for Windows Terminal version >= 1.12 DisplayName and Description resources are not in the 'resources' subtree
			parsedFallback = StrUtils::ConcatW(prefix, L"///", key);
		}
	}
	
	std::wstring source = fmt::format(L"@{{{}? {}}}", packageFullName, parsed);
	
	std::wstring result(128, 0);
	HRESULT hr = SHLoadIndirectString(source.c_str(), result.data(), (UINT)result.size() + 1, nullptr);
	if (FAILED(hr)) {
		if (parsedFallback.empty()) {
			return {};
		}

		source = fmt::format(L"@{{{}? {}}}", packageFullName, parsedFallback);
		hr = SHLoadIndirectString(source.c_str(), result.data(), (UINT)result.size() + 1, nullptr);
		if (FAILED(hr)) {
			Logger::Get().ComError("SHLoadIndirectString 失败", hr);
			return {};
		}
	}

	result.resize(StrUtils::StrLen(result.c_str()));
	return result;
}

bool AppXHelper::AppXReader::Initialize(HWND hWnd) noexcept {
	bool isSuspended = false;
	if (Win32Utils::GetWndClassName(hWnd) == L"ApplicationFrameWindow") {
		// UWP 应用被托管在 ApplicationFrameHost 进程中
		HWND childHwnd = NULL;
		EnumChildWindows(
			hWnd,
			[](HWND hWnd, LPARAM lParam) {
				if (Win32Utils::GetWndClassName(hWnd) != L"Windows.UI.Core.CoreWindow") {
					return TRUE;
				}

				*(HWND*)lParam = hWnd;
				return FALSE;
			},
			(LPARAM)&childHwnd
		);

		
		if (childHwnd == NULL) {
			// 特殊情况下 UWP 应用无法通过子窗口找到
			// 比如被最小化（挂起）或尚未完成初始化
			isSuspended = true;
		} else {
			hWnd = childHwnd;
		}
	}

	Win32Utils::ScopedHandle hProc;
	std::wstring aumid;

	if (!isSuspended) {
		// 使用 GetApplicationUserModelId 获取 AUMID
		DWORD dwProcId = 0;
		if (!GetWindowThreadProcessId(hWnd, &dwProcId)) {
			Logger::Get().Win32Error("GetWindowThreadProcessId 失败");
			return false;
		}

		hProc.reset(Win32Utils::SafeHandle(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcId)));
		if (!hProc) {
			Logger::Get().Win32Error("OpenProcess 失败");
			return false;
		}

		UINT32 length = 0;
		if (GetApplicationUserModelId(hProc.get(), &length, nullptr) != APPMODEL_ERROR_NO_APPLICATION && length > 0) {
			aumid.resize((size_t)length - 1);
			if (GetApplicationUserModelId(hProc.get(), &length, aumid.data()) != ERROR_SUCCESS) {
				aumid.clear();
			}
		}
	} else {
		// 窗口被挂起，此时 UWP 进程无法通过子窗口找到
		// 回落到从窗口的 PropertyStore 中检索 AUMID
		// 来自 https://github.com/valinet/sws/blob/bc8b04e451649964ee3d74255f9e9eda13ef24c3/SimpleWindowSwitcher/sws_IconPainter.c#L257
		com_ptr<IPropertyStore> propStore;
		HRESULT hr = SHGetPropertyStoreForWindow(hWnd, IID_PPV_ARGS(&propStore));
		if (FAILED(hr)) {
			Logger::Get().ComError("SHGetPropertyStoreForWindow 失败", hr);
			return false;
		}

		PROPVARIANT prop;
		hr = propStore->GetValue(PKEY_AppUserModel_ID, &prop);
		if (FAILED(hr) || prop.vt != VT_LPWSTR || !prop.pwszVal) {
			return false;
		}

		aumid = prop.pwszVal;
		PropVariantClear(&prop);
	}

	UINT pfnLen = 0, praidLen = 0;
	if (ParseApplicationUserModelId(aumid.c_str(), &pfnLen, nullptr, &praidLen, nullptr) != ERROR_INSUFFICIENT_BUFFER || pfnLen == 0 || praidLen == 0) {
		return false;
	}

	std::wstring packageFamilyName(pfnLen - 1, 0);
	std::wstring praid(praidLen - 1, 0);
	if (ParseApplicationUserModelId(aumid.c_str(), &pfnLen, packageFamilyName.data(), &praidLen, praid.data()) != ERROR_SUCCESS) {
		return false;
	}

	if (!isSuspended) {
		UINT length = 0;
		if (::GetPackageFullName(hProc.get(), &length, nullptr) == APPMODEL_ERROR_NO_PACKAGE) {
			return false;
		}

		_packageFullName.resize((size_t)length - 1);
		if (::GetPackageFullName(hProc.get(), &length, _packageFullName.data()) != ERROR_SUCCESS) {
			return false;
		}
	} else {
		// 已挂起的 UWP 窗口无法获得进程句柄，因此使用 PackageFamilyName 检索 PackageFullName
		UINT32 packageCount = 0;
		UINT32 bufferLength = 0;
		if (FindPackagesByPackageFamily(packageFamilyName.c_str(), PACKAGE_FILTER_HEAD, &packageCount, nullptr, &bufferLength, nullptr, nullptr) != ERROR_INSUFFICIENT_BUFFER || packageCount == 0 || bufferLength == 0) {
			return false;
		}

		std::unique_ptr<wchar_t* []> packageFullNames(new wchar_t* [packageCount]);
		std::unique_ptr<wchar_t[]> buffer(new wchar_t[bufferLength]);
		if (FindPackagesByPackageFamily(packageFamilyName.c_str(), PACKAGE_FILTER_HEAD, &packageCount, packageFullNames.get(), &bufferLength, buffer.get(), nullptr) != ERROR_SUCCESS) {
			return false;
		}

		// 只使用第一个包，一般也只有一个
		_packageFullName = packageFullNames[0];
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

std::wstring AppXHelper::AppXReader::GetIconPath(SIZE preferredSize) const noexcept {
	if (!_appxApp) {
		return {};
	}

	wchar_t* logoUriVal = nullptr;
	if (FAILED(_appxApp->GetStringValue(L"Square44x44Logo", &logoUriVal)) || !logoUriVal) {
		return {};
	}

	Utils::ScopeExit se([logoUriVal]() {
		CoTaskMemFree(logoUriVal);
	});

	std::wstring_view logoUri = logoUriVal;

	std::wstring path;
	if (logoUri.find(L'\\') != std::wstring_view::npos) {
		path = _packagePath + logoUriVal;;
	} else {
		path = StrUtils::ConcatW(_packagePath, L"Assets\\", logoUri);;
	}

	size_t extensionPointPos = path.find_last_of(L'.');
	if (extensionPointPos == std::wstring::npos) {
		return {};
	}
	std::wstring_view prefix(path.begin(), path.begin() + extensionPointPos);
	std::wstring_view extension(path.begin() + extensionPointPos, path.end());

	struct CandidateIcon {
		UINT width;
		std::wstring_view coloredSuffix;
	};
	static constexpr const CandidateIcon candidateIcons[] = {
		{ 16, L".targetsize-16" },
		{ 24, L".targetsize-24" },
		{ 30, L".targetsize-30" },
		{ 36, L".targetsize-36" },
		{ 44, L".scale-100" },
		{ 44, L".targetsize-44" },
		{ 55, L".scale-125" },
		{ 60, L".targetsize-60" },
		{ 66, L".scale-150" },
		{ 72, L".targetsize-72" },
		{ 88, L".scale-200" },
		{ 96, L".targetsize-96" },
		{ 128, L".targetsize-128" },
		{ 176, L".scale-400" },
		{ 180, L".targetsize-180" },
		{ 256 , L".targetsize-256" }
	};

	const CandidateIcon* candiate = nullptr;

	auto it = std::lower_bound(std::begin(candidateIcons), std::end(candidateIcons), preferredSize.cx, [](const CandidateIcon& l, LONG r) {
		return l.width < r;
	});
	if (it == std::end(candidateIcons)) {
		candiate = it - 1;
	} else {
		candiate = it;
	}

	for (; it != std::end(candidateIcons); ++it) {
		std::wstring fileName = StrUtils::ConcatW(prefix, it->coloredSuffix, extension);
		if (Win32Utils::FileExists(fileName.c_str())) {
			return fileName;
		}
	}

	return {};
}

bool AppXHelper::AppXReader::_ResolveApplication(const std::wstring& praid) noexcept {
	UINT32 pathLen = 0;
	GetPackagePathByFullName(_packageFullName.c_str(), &pathLen, nullptr);
	if (pathLen == 0) {
		return false;
	}

	_packagePath.resize((size_t)pathLen - 1);
	if (GetPackagePathByFullName(_packageFullName.c_str(), &pathLen, _packagePath.data()) != ERROR_SUCCESS) {
		return false;
	}
	if (_packagePath.back() != L'\\') {
		_packagePath.push_back(L'\\');
	}

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

	com_ptr<IStream> inputStream;
	hr = SHCreateStreamOnFileEx(
		(_packagePath + L"AppXManifest.xml").c_str(),
		STGM_READ | STGM_SHARE_DENY_WRITE,
		0,
		FALSE,
		nullptr,
		inputStream.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("打开 AppXManifest.xml 失败", hr);
		return false;
	}

	com_ptr<IAppxManifestReader> manifestReader;
	hr = factory->CreateManifestReader(
		inputStream.get(),
		manifestReader.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateManifestReader 失败", hr);
		return false;
	}

	com_ptr<IAppxManifestApplicationsEnumerator> appEnumerator;
	hr = manifestReader->GetApplications(appEnumerator.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("GetApplications 失败", hr);
		return false;
	}

	// 枚举所有应用查找 praid
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

	// 未找到 Id 为 praid 的应用
	return false;
}

}
