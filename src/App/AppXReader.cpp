#include "pch.h"
#include "AppXReader.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include "Utils.h"
#include "Logger.h"
#include <appmodel.h>
#include <Shlwapi.h>
#include <shellapi.h>
#include <propkey.h>
#include <regex>
#include <wincodec.h>

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

bool AppXReader::Initialize(HWND hWnd) noexcept {
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

std::wstring AppXReader::GetDisplayName() const noexcept {
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

class CandidateIcon {
public:
	CandidateIcon(const wchar_t* fileName) : _fileName(fileName) {
		size_t firstPointPos = _fileName.find_first_of(L'.');
		if (firstPointPos == std::wstring::npos) {
			_isValid = false;
			return;
		}

		size_t secondPointPos = _fileName.find_last_of(L'.');
		if (secondPointPos == firstPointPos) {
			_size = 44;
			return;
		} else if (secondPointPos == std::wstring::npos || secondPointPos <= firstPointPos + 1) {
			_isValid = false;
			return;
		}

		std::wstring_view suffix(_fileName.begin() + firstPointPos + 1, _fileName.begin() + secondPointPos);
		assert(suffix.find(L'.') == std::wstring_view::npos);

		for (std::wstring_view qualifier : StrUtils::Split(suffix, L'_')) {
			size_t delimPos = qualifier.find_first_of(L'-');
			if (delimPos == std::wstring_view::npos) {
				_isValid = false;
				return;
			}
			
			std::wstring_view name(qualifier.begin(), qualifier.begin() + delimPos);
			std::wstring_view value(qualifier.begin() + delimPos + 1, qualifier.end());

			if (name == L"targetsize") {
				_isTargetSize = true;

				if (value == L"16") {
					_size = 16;
				} else if (value == L"24") {
					_size = 24;
				} else if (value == L"30") {
					_size = 30;
				} else if (value == L"36") {
					_size = 36;
				} else if (value == L"44") {
					_size = 44;
				} else if (value == L"60") {
					_size = 60;
				} else if (value == L"72") {
					_size = 72;
				} else if (value == L"96") {
					_size = 96;
				} else if (value == L"128") {
					_size = 128;
				} else if (value == L"180") {
					_size = 180;
				} else if (value == L"256") {
					_size = 256;
				} else {
					_isValid = false;
				}
			} else if (name == L"scale") {
				_isScale = true;

				if (value == L"100") {
					_size = 44;
				} else if (value == L"125") {
					_size = 55;
				} else if (value == L"150") {
					_size = 66; 
				} else if (value == L"200") {
					_size = 88;
				} else if (value == L"400") {
					_size = 176;
				} else {
					_isValid = false;
				}
			} else if (name == L"altform") {
				if (value == L"lightunplated") {
					_isLightTheme = true;
				}
			} else if (name == L"theme") {
				if (value == L"light") {
					_isLightTheme = true;
				}
			} else if (name == L"contrast") {
				_isValid = false;
				return;
			}
		}
	}

	const std::wstring& FileName() const noexcept {
		return _fileName;
	}

	bool IsValid() const noexcept {
		return _isValid;
	}

	static bool Compare(const CandidateIcon& l, const CandidateIcon& r, uint32_t preferredSize, bool isLightTheme) {
		if (l._isLightTheme != r._isLightTheme) {
			return l._isLightTheme == isLightTheme;
		}

		if (l._size != r._size) {
			if (l._size == preferredSize) {
				return true;
			}
			if (r._size == preferredSize) {
				return false;
			}

			if (l._size > preferredSize) {
				if (r._size > preferredSize) {
					return l._size < r._size;
				} else {
					return true;
				}
			} else {
				if (r._size > preferredSize) {
					return false;
				} else {
					return l._size > r._size;
				}
			}
		}

		if (l._isTargetSize != r._isTargetSize) {
			return l._isTargetSize;
		}

		if (l._isScale != r._isScale) {
			return l._isScale;
		}

		return false;
	}

private:
	bool _isTargetSize = false;
	bool _isScale = false;
	bool _isValid = true;
	std::wstring _fileName;
	uint32_t _size = 0;
	bool _isLightTheme = false;
};

bool CheckNeedBackground(const std::wstring& iconPath, bool isLightTheme) {
	com_ptr<IWICImagingFactory2> wicImgFactory;

	HRESULT hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(wicImgFactory.put())
	);

	if (FAILED(hr)) {
		Logger::Get().ComError("创建 WICImagingFactory 失败", hr);
		return false;
	}

	// 读取图像文件
	winrt::com_ptr<IWICBitmapDecoder> decoder;
	hr = wicImgFactory->CreateDecoderFromFilename(
		iconPath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateDecoderFromFilename 失败", hr);
		return false;
	}

	winrt::com_ptr<IWICBitmapFrameDecode> frame;
	hr = decoder->GetFrame(0, frame.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameDecode::GetFrame 失败", hr);
		return false;
	}

	// 转换格式
	winrt::com_ptr<IWICFormatConverter> formatConverter;
	hr = wicImgFactory->CreateFormatConverter(formatConverter.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateFormatConverter 失败", hr);
		return false;
	}

	hr = formatConverter->Initialize(frame.get(),
		GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICFormatConverter::Initialize 失败", hr);
		return false;
	}

	UINT width, height;
	hr = formatConverter->GetSize(&width, &height);
	if (FAILED(hr)) {
		Logger::Get().ComError("GetSize 失败", hr);
		return false;
	}

	UINT stride = width * 4;
	UINT size = stride * height;
	std::unique_ptr<BYTE[]> buf(new BYTE[size]);

	hr = formatConverter->CopyPixels(nullptr, stride, size, buf.get());
	if (FAILED(hr)) {
		Logger::Get().ComError("CopyPixels 失败", hr);
		return false;
	}

	// 计算平均亮度
	double lumaTotal = 0;
	UINT lumaCount = 0;
	for (UINT i = 0, len = width * height; i < len; ++i) {
		BYTE* pixel = &buf.get()[i * 4];
		if (pixel[3] == 0) {
			continue;
		}

		double luma = 0.299 * pixel[0] + 0.587 * pixel[1] + 0.114 * pixel[2];
		double alpha = pixel[3] / 255.0;
		luma = luma * alpha + 255 * (1 - alpha);

		lumaTotal += luma;
		++lumaCount;
	}

	double lumaAvg = lumaTotal / lumaCount;
	return isLightTheme ? lumaAvg > 220 : lumaAvg < 30;
}

std::wstring AppXReader::GetIconPath(uint32_t preferredSize, bool isLightTheme, bool* hasBackground) const noexcept {
	if (!_appxApp) {
		return {};
	}

	std::wstring iconFileName;
	{
		wchar_t* logoUriVal = nullptr;
		if (FAILED(_appxApp->GetStringValue(L"Square44x44Logo", &logoUriVal)) || !logoUriVal) {
			return {};
		}

		std::wstring_view logoUri = logoUriVal;

		if (logoUri.find(L'\\') != std::wstring_view::npos) {
			iconFileName = _packagePath + logoUriVal;;
		} else {
			iconFileName = StrUtils::ConcatW(_packagePath, L"Assets\\", logoUri);;
		}

		CoTaskMemFree(logoUriVal);
	}

	size_t delimPos = iconFileName.find_last_of(L'\\');
	if (delimPos == std::wstring::npos) {
		return {};
	}
	size_t extensionPointPos = iconFileName.find_last_of(L'.');
	if (extensionPointPos == std::wstring::npos || extensionPointPos <= delimPos) {
		return {};
	}
	
	std::wstring_view prefix(iconFileName.begin(), iconFileName.begin() + extensionPointPos);
	std::wstring_view extension(iconFileName.begin() + extensionPointPos, iconFileName.end());

	std::wstring_view iconName(iconFileName.begin() + delimPos + 1, iconFileName.begin() + extensionPointPos);
	std::wstring iconNameExt = StrUtils::ConcatW(iconName, extension);

	std::wregex regex(fmt::format(L"^{}\\.[^\\.]+\\{}$", iconName, extension), std::wregex::optimize | std::wregex::nosubs);

	std::vector<CandidateIcon> candidateIcons;

	WIN32_FIND_DATA findData{};
	HANDLE hFind = Win32Utils::SafeHandle(FindFirstFileEx(StrUtils::ConcatW(prefix, L"*").c_str(),
		FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH));
	if (hFind) {
		while (FindNextFile(hFind, &findData)) {
			if (findData.cFileName != iconNameExt && !std::regex_match(findData.cFileName, regex)) {
				continue;
			}

			CandidateIcon ci(findData.cFileName);
			if (!ci.IsValid()) {
				continue;
			}
			
			candidateIcons.emplace_back(std::move(ci));
		}

		FindClose(hFind);
	}

	if (candidateIcons.empty()) {
		return {};
	}

	auto it = std::min_element(candidateIcons.begin(), candidateIcons.end(),
		[=](const CandidateIcon& l, const CandidateIcon& r) { return CandidateIcon::Compare(l, r, preferredSize, isLightTheme); });

	std::wstring result = StrUtils::ConcatW(std::wstring_view(iconFileName.begin(), iconFileName.begin() + delimPos + 1), it->FileName());
	if (hasBackground != nullptr) {
		*hasBackground = CheckNeedBackground(result, isLightTheme);
	}
	return result;
}

bool AppXReader::_ResolveApplication(const std::wstring& praid) noexcept {
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
