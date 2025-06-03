#include "pch.h"
#include "AppXReader.h"
#include "Win32Helper.h"
#include "StrHelper.h"
#include "Logger.h"
#include <appmodel.h>
#include <propkey.h>
#include <regex>
#include <wincodec.h>
#include <parallel_hashmap/phmap.h>
#include <AppxPackaging.h>
#include <propsys.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <Shlwapi.h>
#include <shellapi.h>

using namespace winrt;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::UI::ViewManagement;

namespace Magpie {

struct AppxCacheData {
	std::wstring praid;
	std::wstring packageFullName;
	std::wstring packagePath;
	std::wstring displayName;
	std::wstring executable;
	std::wstring square44x44Logo;
};
static phmap::flat_hash_map<std::wstring, AppxCacheData> appxCache;
// 用于同步对 appxCache 的访问
static wil::srwlock appxCacheLock;


static std::wstring ResourceFromPri(std::wstring_view packageFullName, std::wstring_view resourceReference) {
	// 移植自 https://github.com/microsoft/PowerToys/blob/c36a80dad571db26d6cf9e40e70099815ed56049/src/modules/launcher/Plugins/Microsoft.Plugin.Program/Programs/UWPApplication.cs#L299

	std::wstring_view prefix = L"ms-resource:";

	StrHelper::Trim(resourceReference);

	std::wstring lowerCaseResourceReference = StrHelper::ToLowerCase(resourceReference);
	
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
		parsed = StrHelper::Concat(prefix, key);
	} else if (key.starts_with(L'/')) {
		parsed = StrHelper::Concat(prefix, L"//", key);
	} else {
		std::wstring_view lowerCaseKey(
			lowerCaseResourceReference.begin() + prefix.size(),
			lowerCaseResourceReference.end()
		);

		if (lowerCaseKey.starts_with(L"resources")) {
			parsed = StrHelper::Concat(prefix, key);
		} else {
			parsed = StrHelper::Concat(prefix, L"///resources/", key);

			// e.g. for Windows Terminal version >= 1.12 DisplayName and Description resources are not in the 'resources' subtree
			parsedFallback = StrHelper::Concat(prefix, L"///", key);
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

	result.resize(StrHelper::StrLen(result.c_str()));
	return result;
}

bool AppXReader::Initialize(HWND hWnd) noexcept {
	if (Win32Helper::GetWndClassName(hWnd) == L"ApplicationFrameWindow") {
		// UWP 应用被托管在 ApplicationFrameHost 进程中
		HWND childHwnd = NULL;
		EnumChildWindows(
			hWnd,
			[](HWND hWnd, LPARAM lParam) {
				if (Win32Helper::GetWndClassName(hWnd) != L"Windows.UI.Core.CoreWindow") {
					return TRUE;
				}

				*(HWND*)lParam = hWnd;
				return FALSE;
			},
			(LPARAM)&childHwnd
		);
		
		if (childHwnd) {
			hWnd = childHwnd;
		} else {
			// 被最小化（挂起）或尚未完成初始化时 UWP 进程无法通过子窗口找到，
			// 回落到从窗口的 PropertyStore 中检索 AUMID。
			// 来自 https://github.com/valinet/sws/blob/bc8b04e451649964ee3d74255f9e9eda13ef24c3/SimpleWindowSwitcher/sws_IconPainter.c#L257
			com_ptr<IPropertyStore> propStore;
			HRESULT hr = SHGetPropertyStoreForWindow(hWnd, IID_PPV_ARGS(&propStore));
			if (FAILED(hr)) {
				Logger::Get().ComError("SHGetPropertyStoreForWindow 失败", hr);
				return false;
			}

			wil::unique_prop_variant prop;
			hr = propStore->GetValue(PKEY_AppUserModel_ID, &prop);
			if (FAILED(hr) || prop.vt != VT_LPWSTR || !prop.pwszVal) {
				return false;
			}

			return Initialize(prop.pwszVal);
		}
	}

	// 使用 GetApplicationUserModelId 获取 AUMID
	DWORD dwProcId = 0;
	if (!GetWindowThreadProcessId(hWnd, &dwProcId)) {
		Logger::Get().Win32Error("GetWindowThreadProcessId 失败");
		return false;
	}

	wil::unique_process_handle hProc(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcId));
	if (!hProc) {
		Logger::Get().Win32Error("OpenProcess 失败");
		return false;
	}

	std::wstring aumid;
	HRESULT hr = wil::AdaptFixedSizeToAllocatedResult<std::wstring, 128>(
		aumid, [&](_Out_writes_(valueLength) PWSTR value, size_t valueLength, _Out_ size_t* valueLengthNeededWithNul) -> HRESULT {
			UINT32 length = (UINT32)valueLength;
			LONG rc = GetApplicationUserModelId(hProc.get(), &length, value);
			if (rc != ERROR_SUCCESS && rc != ERROR_INSUFFICIENT_BUFFER) {
				return HRESULT_FROM_WIN32(rc);
			}

			*valueLengthNeededWithNul = length;
			return S_OK;
		}
	);
	if (FAILED(hr)) {
		return false;
	}

	return Initialize(aumid);
}

bool AppXReader::Initialize(std::wstring_view aumid) noexcept {
	{
		auto lock = appxCacheLock.lock_exclusive();

		auto it = appxCache.find(aumid);
		if (it != appxCache.end()) {
			if (it->second.packagePath.empty()) {
				// 之前的解析失败
				return false;
			}

			_aumid = aumid;
			_praid = it->second.praid;
			_packageFullName = it->second.packageFullName;
			_packagePath = it->second.packagePath;
			_displayName = it->second.displayName;
			_executable = it->second.executable;
			_square44x44Logo = it->second.square44x44Logo;
			return true;
		}
		
		appxCache.emplace(aumid, AppxCacheData());
	}

	_aumid = aumid;

	if (!_ResolvePackagePath()) {
		Logger::Get().Error("_ResolvePackagePath 失败");
		return false;
	}

	com_ptr<IAppxFactory> factory = try_create_instance<IAppxFactory>(CLSID_AppxFactory);
	if (!factory) {
		Logger::Get().Error("创建 AppxFactory 失败");
		return false;
	}

	com_ptr<IStream> inputStream;
	HRESULT hr = SHCreateStreamOnFileEx(
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

		wil::unique_cotaskmem_string curPraid;
		if (FAILED(appxApp->GetStringValue(L"Id", curPraid.put()))) {
			break;
		}

		if (curPraid && _praid == curPraid.get()) {
			wil::unique_cotaskmem_string value;
			if (SUCCEEDED(appxApp->GetStringValue(L"DisplayName", value.put())) && value) {
				_displayName = value.get();
			}

			value = nullptr;
			if (SUCCEEDED(appxApp->GetStringValue(L"Executable", value.put())) && value) {
				_executable = value.get();
			}

			value = nullptr;
			if (SUCCEEDED(appxApp->GetStringValue(L"Square44x44Logo", value.put())) && value) {
				_square44x44Logo = value.get();
			}

			auto lock = appxCacheLock.lock_exclusive();
			AppxCacheData& cacheData = appxCache[aumid];
			cacheData.praid = _praid;
			cacheData.packageFullName = _packageFullName;
			cacheData.packagePath = _packagePath;
			cacheData.displayName = _displayName;
			cacheData.executable = _executable;
			cacheData.square44x44Logo = _square44x44Logo;
			return true;
		}

		hr = appEnumerator->MoveNext(&hasCurrent);
	}

	// 未找到 Id 为 praid 的应用
	return false;
}

std::wstring AppXReader::GetDisplayName() noexcept {
	assert(!_packagePath.empty());

	if (_displayName.empty()) {
		return {};
	}

	return ResourceFromPri(_packageFullName, _displayName);
}

const std::wstring& AppXReader::GetPackagePath() noexcept {
	assert(!_packagePath.empty());
	return _packagePath;
}

std::wstring AppXReader::GetExecutablePath() noexcept {
	assert(!_packagePath.empty());

	if (_executable.empty()) {
		return {};
	}

	return _packagePath + _executable;
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

		for (std::wstring_view qualifier : StrHelper::Split(suffix, L'_')) {
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
				} else if (value == L"32") {
					_size = 32;
				} else if (value == L"36") {
					_size = 36;
				} else if (value == L"44") {
					_size = 44;
				} else if (value == L"48") {
					_size = 48;
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
					_isUnplated = true;
				} else if (value == L"unplated") {
					_isUnplated = true;
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

	// 用以选择更合适的图标。规则:
	// 1. 匹配当前主题的优先
	// 2. 没有边框的优先
	// 3. 和 preferredSize 相同的优先
	// 4. 如果一个过大，另一个过小，取大的
	// 5. 如果都过大或过小，取更接近 preferredSize 的
	// 6. 优先选择 targetsize 前缀，然后是 scale 前缀
	static bool Compare(const CandidateIcon& l, const CandidateIcon& r, uint32_t preferredSize, bool isLightTheme) {
		if (l._isLightTheme != r._isLightTheme) {
			return l._isLightTheme == isLightTheme;
		}

		// 优先选择没有边框的图标
		if (l._isUnplated != r._isUnplated) {
			return l._isUnplated;
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
	bool _isUnplated = false;
};

// 如果图标和背景的对比度太低，使用主题色填充背景
static SoftwareBitmap AutoFillBackground(const std::wstring& iconPath, bool isLightTheme, bool noPath) {
	com_ptr<IWICImagingFactory2> wicImgFactory = try_create_instance<IWICImagingFactory2>(CLSID_WICImagingFactory);
	if (!wicImgFactory) {
		Logger::Get().Error("创建 WICImagingFactory2 失败");
		return nullptr;
	}

	// 读取图像文件
	winrt::com_ptr<IWICBitmapDecoder> decoder;
	HRESULT hr = wicImgFactory->CreateDecoderFromFilename(
		iconPath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateDecoderFromFilename 失败", hr);
		return nullptr;
	}

	winrt::com_ptr<IWICBitmapFrameDecode> frame;
	hr = decoder->GetFrame(0, frame.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameDecode::GetFrame 失败", hr);
		return nullptr;
	}

	// 转换格式
	winrt::com_ptr<IWICFormatConverter> formatConverter;
	hr = wicImgFactory->CreateFormatConverter(formatConverter.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateFormatConverter 失败", hr);
		return nullptr;
	}

	hr = formatConverter->Initialize(frame.get(),
		GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICFormatConverter::Initialize 失败", hr);
		return nullptr;
	}

	UINT width, height;
	hr = formatConverter->GetSize(&width, &height);
	if (FAILED(hr)) {
		Logger::Get().ComError("GetSize 失败", hr);
		return nullptr;
	}

	UINT stride = width * 4;
	UINT size = stride * height;
	std::unique_ptr<uint8_t[]> buf(new uint8_t[size]);

	hr = formatConverter->CopyPixels(nullptr, stride, size, buf.get());
	if (FAILED(hr)) {
		Logger::Get().ComError("CopyPixels 失败", hr);
		return nullptr;
	}

	// 计算平均亮度
	float lumaTotal = 0;
	uint32_t lumaCount = 0;
	for (uint32_t i = 0, len = width * height; i < len; ++i) {
		uint8_t* pixel = &buf.get()[i * 4];

		uint8_t alpha = pixel[3];
		if (alpha == 0) {
			continue;
		}

		float luma = 0.299f * pixel[0] + 0.587f * pixel[1] + 0.114f * pixel[2];
		if (alpha != 255) {
			float alphaNorm = alpha / 255.0f;
			luma = luma * alphaNorm + (isLightTheme ? 255 : 0) * (1 - alphaNorm);
		}

		lumaTotal += luma;
		++lumaCount;
	}

	const float lumaAvg = lumaTotal / lumaCount;
	if (isLightTheme ? lumaAvg <= 220 : lumaAvg >= 30) {
		if (!noPath) {
			return nullptr;
		}

		SoftwareBitmap bitmap(BitmapPixelFormat::Bgra8, width, height, BitmapAlphaMode::Premultiplied);
		{
			BitmapBuffer buffer = bitmap.LockBuffer(BitmapBufferAccessMode::Write);
			uint8_t* pixels = buffer.CreateReference().data();

			const uint8_t* origin = buf.get();
			for (size_t i = 0, pixelsSize = static_cast<size_t>(width) * height * 4; i < pixelsSize; i += 4) {
				// 预乘 Alpha 通道
				const float alpha = origin[i + 3] / 255.0f;

				pixels[i] = (BYTE)std::lround(origin[i] * alpha);
				pixels[i + 1] = (BYTE)std::lround(origin[i + 1] * alpha);
				pixels[i + 2] = (BYTE)std::lround(origin[i + 2] * alpha);
				pixels[i + 3] = origin[i + 3];
			}
		}
		return bitmap;
	}

	// 和背景的对比度太低，需要填充背景
	const uint32_t borderWidth = width / 6;
	const uint32_t borderHeight = height / 6;
	const uint32_t totalWidth = width + borderWidth * 2;
	const uint32_t totalHeight = height + borderHeight * 2;

	const Color accentColor = UISettings().GetColorValue(UIColorType::Accent);

	SoftwareBitmap bitmap(BitmapPixelFormat::Bgra8, totalWidth, totalHeight, BitmapAlphaMode::Premultiplied);
	{
		BitmapBuffer buffer = bitmap.LockBuffer(BitmapBufferAccessMode::Write);
		uint8_t* pixels = buffer.CreateReference().data();

		const uint32_t fillColor = (0xff << 24) | (accentColor.R << 16) | (accentColor.G << 8) | accentColor.B;
		std::fill_n((uint32_t*)pixels, totalWidth * totalHeight, fillColor);

		pixels += (borderHeight * totalWidth + borderWidth) * 4;
		const uint8_t* origin = buf.get();
		for (UINT i = 0; i < height; ++i) {
			for (UINT j = 0; j < width; ++j, origin += 4, pixels += 4) {
				const float alpha = origin[3] / 255.0f;
				if (alpha < FLOAT_EPSILON<float>) {
					continue;
				}

				const float reverseAlpha = 1 - alpha;
				if (reverseAlpha < FLOAT_EPSILON<float>) {
					pixels[0] = origin[0];
					pixels[1] = origin[1];
					pixels[2] = origin[2];
					pixels[3] = 255;
					continue;
				}

				pixels[0] = (uint8_t)std::lroundf(origin[0] * alpha + accentColor.B * reverseAlpha);
				pixels[1] = (uint8_t)std::lroundf(origin[1] * alpha + accentColor.G * reverseAlpha);
				pixels[2] = (uint8_t)std::lroundf(origin[2] * alpha + accentColor.R * reverseAlpha);
				pixels[3] = 255;
			}

			pixels += borderWidth * 2 * 4;
		}
	}
	return bitmap;
}

std::variant<std::wstring, SoftwareBitmap> AppXReader::GetIcon(
	uint32_t preferredSize,
	bool isLightTheme,
	bool noPath
) noexcept {
	assert(!_packagePath.empty());

	if (_square44x44Logo.empty()) {
		return {};
	}

	std::wstring iconFileName;
	if (_square44x44Logo.find(L'\\') != std::wstring_view::npos) {
		iconFileName = _packagePath + _square44x44Logo;
	} else {
		iconFileName = StrHelper::Concat(_packagePath, L"Assets\\", _square44x44Logo);;
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
	std::wstring iconNameExt = StrHelper::Concat(iconName, extension);

	std::wregex regex(fmt::format(L"^{}\\.[^\\.]+\\{}$", iconName, extension), std::wregex::nosubs);

	std::vector<CandidateIcon> candidateIcons;

	WIN32_FIND_DATA findData{};
	wil::unique_hfind hFind(FindFirstFileEx(StrHelper::Concat(prefix, L"*").c_str(),
		FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH));
	if (hFind) {
		do {
			if (findData.cFileName != iconNameExt && !std::regex_match(findData.cFileName, regex)) {
				continue;
			}

			CandidateIcon ci(findData.cFileName);
			if (!ci.IsValid()) {
				continue;
			}

			candidateIcons.emplace_back(std::move(ci));
		} while (FindNextFile(hFind.get(), &findData));
	}

	if (candidateIcons.empty()) {
		return {};
	}

	auto it = std::min_element(
		candidateIcons.begin(),
		candidateIcons.end(),
		[=](const CandidateIcon& l, const CandidateIcon& r) {
			return CandidateIcon::Compare(l, r, preferredSize, isLightTheme);
		}
	);

	std::wstring iconPath = StrHelper::Concat(
		std::wstring_view(iconFileName.begin(), iconFileName.begin() + delimPos + 1),
		it->FileName()
	);
	SoftwareBitmap bkgIcon = AutoFillBackground(iconPath, isLightTheme, noPath);
	if (bkgIcon || noPath) {
		return std::move(bkgIcon);
	} else {
		return std::move(iconPath);
	}
}

void AppXReader::ClearCache() noexcept {
	auto lock = appxCacheLock.lock_exclusive();
	appxCache.clear();
}

bool AppXReader::_ResolvePackagePath() {
	if (!_packagePath.empty()) {
		return true;
	}

	uint32_t pfnLen = 0, praidLen = 0;
	std::ignore = ParseApplicationUserModelId(_aumid.c_str(), &pfnLen, nullptr, &praidLen, nullptr);
	if (pfnLen == 0 || praidLen == 0) {
		Logger::Get().Error("ParseApplicationUserModelId 失败");
		return false;
	}

	std::wstring packageFamilyName(pfnLen - 1, 0);
	_praid.assign((size_t)praidLen - 1, 0);
	if (ParseApplicationUserModelId(_aumid.c_str(), &pfnLen, packageFamilyName.data(),
		&praidLen, _praid.data()) != ERROR_SUCCESS)
	{
		Logger::Get().Error("ParseApplicationUserModelId 失败");
		return false;
	}

	//使用 PackageFamilyName 检索 PackageFullName
	uint32_t packageCount = 0;
	uint32_t bufferLength = 0;
	std::ignore = FindPackagesByPackageFamily(packageFamilyName.c_str(), PACKAGE_FILTER_HEAD,
		&packageCount, nullptr, &bufferLength, nullptr, nullptr);
	if (packageCount == 0 || bufferLength == 0) {
		Logger::Get().Error("FindPackagesByPackageFamily 失败");
		return false;
	}

	std::unique_ptr<wchar_t* []> packageFullNames(new wchar_t* [packageCount]);
	std::unique_ptr<wchar_t[]> buffer(new wchar_t[bufferLength]);
	if (FindPackagesByPackageFamily(packageFamilyName.c_str(), PACKAGE_FILTER_HEAD,
		&packageCount, packageFullNames.get(), &bufferLength, buffer.get(), nullptr) != ERROR_SUCCESS) 
	{
		Logger::Get().Error("FindPackagesByPackageFamily 失败");
		return false;
	}

	// 只使用第一个包，一般也只有一个
	_packageFullName = packageFullNames[0];

	HRESULT hr = wil::AdaptFixedSizeToAllocatedResult(
		_packagePath, [&](_Out_writes_(valueLength) PWSTR value, size_t valueLength, _Out_ size_t* valueLengthNeededWithNul) -> HRESULT {
			UINT32 length = (UINT32)valueLength;
			LONG rc = GetPackagePathByFullName(_packageFullName.c_str(), &length, value);
			if (rc != ERROR_SUCCESS && rc != ERROR_INSUFFICIENT_BUFFER) {
				return HRESULT_FROM_WIN32(rc);
			}

			*valueLengthNeededWithNul = length;
			return S_OK;
		}
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("GetPackagePathByFullName 失败", hr);
		return false;
	}

	if (_packagePath.back() != L'\\') {
		_packagePath.push_back(L'\\');
	}

	return true;
}

}
