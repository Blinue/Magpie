#include "Utils.h"
#include "Logger.h"
#include "StrUtils.h"
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
#include <dwmapi.h>
#include <zstd.h>
#include <io.h>
#include <Psapi.h>
#include <winternl.h>


using namespace winrt;
using namespace Windows::UI::Xaml::Media;


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

void Utils::CloseAllXamlPopups(const XamlRoot& root) {
	if (!root) {
		return;
	}

	// https://github.com/microsoft/microsoft-ui-xaml/issues/4554
	for (const auto& popup : VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		UIElement child = popup.Child();
		if (get_class_name(child) != name_of<Controls::ContentDialog>()) {
			popup.IsOpen(false);
		}
	}
}

void Utils::CloseXamlDialog(const XamlRoot& root) {
	if (!root) {
		return;
	}

	for (const auto& popup : VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		UIElement child = popup.Child();
		if (get_class_name(child) == name_of<Controls::ContentDialog>()) {
			child.as<Controls::ContentDialog>().Hide();
			// 只能同时显示一个 ContentDialog
			return;
		}
	}
}

void Utils::RepositionXamlPopups(const XamlRoot& root) {
	if (!root) {
		return;
	}

	for (const auto& popup : VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		// 取自 https://github.com/CommunityToolkit/Microsoft.Toolkit.Win32/blob/229fa3cd245ff002906b2a594196b88aded25774/Microsoft.Toolkit.Forms.UI.XamlHost/WindowsXamlHostBase.cs#L180
		
		// Toggle the CompositeMode property, which will force all windowed Popups
		// to reposition themselves relative to the new position of the host window.
		auto compositeMode = popup.CompositeMode();

		// Set CompositeMode to some value it currently isn't set to.
		if (compositeMode == ElementCompositeMode::SourceOver) {
			popup.CompositeMode(ElementCompositeMode::MinBlend);
		} else {
			popup.CompositeMode(ElementCompositeMode::SourceOver);
		}

		// Restore CompositeMode to whatever it was originally set to.
		popup.CompositeMode(compositeMode);
	}
}

void Utils::UpdateThemeOfXamlPopups(const XamlRoot& root, const ElementTheme& theme) {
	if (!root) {
		return;
	}

	for (const auto& popup : VisualTreeHelper::GetOpenPopupsForXamlRoot(root)) {
		popup.Child().as<FrameworkElement>().RequestedTheme(theme);
	}
}

UINT Utils::GetWindowShowCmd(HWND hwnd) {
	assert(hwnd != NULL);

	WINDOWPLACEMENT wp{};
	wp.length = sizeof(wp);
	if (!GetWindowPlacement(hwnd, &wp)) {
		Logger::Get().Win32Error("GetWindowPlacement 出错");
		assert(false);
	}

	return wp.showCmd;
}

bool Utils::GetClientScreenRect(HWND hWnd, RECT& rect) {
	if (!GetClientRect(hWnd, &rect)) {
		Logger::Get().Win32Error("GetClientRect 出错");
		return false;
	}

	POINT p{};
	if (!ClientToScreen(hWnd, &p)) {
		Logger::Get().Win32Error("ClientToScreen 出错");
		return false;
	}

	rect.bottom += p.y;
	rect.left += p.x;
	rect.right += p.x;
	rect.top += p.y;

	return true;
}

bool Utils::GetWindowFrameRect(HWND hWnd, RECT& result) {
	HRESULT hr = DwmGetWindowAttribute(hWnd,
		DWMWA_EXTENDED_FRAME_BOUNDS, &result, sizeof(result));
	if (FAILED(hr)) {
		Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
		return false;
	}

	return true;
}

bool Utils::ReadFile(const wchar_t* fileName, std::vector<BYTE>& result) {
	Logger::Get().Info(StrUtils::Concat("读取文件：", StrUtils::UTF16ToUTF8(fileName)));

	CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
	extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
	extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
	extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
	extendedParams.lpSecurityAttributes = nullptr;
	extendedParams.hTemplateFile = nullptr;

	ScopedHandle hFile(SafeHandle(CreateFile2(fileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams)));

	if (!hFile) {
		Logger::Get().Error("打开文件失败");
		return false;
	}

	DWORD size = GetFileSize(hFile.get(), nullptr);
	result.resize(size);

	DWORD readed;
	if (!::ReadFile(hFile.get(), result.data(), size, &readed, nullptr)) {
		Logger::Get().Error("读取文件失败");
		return false;
	}

	return true;
}

bool Utils::ReadTextFile(const wchar_t* fileName, std::string& result) {
	FILE* hFile;
	if (_wfopen_s(&hFile, fileName, L"rt") || !hFile) {
		Logger::Get().Error(StrUtils::Concat("打开文件 ", StrUtils::UTF16ToUTF8(fileName), " 失败"));
		return false;
	}

	// 获取文件长度
	int fd = _fileno(hFile);
	long size = _filelength(fd);

	result.clear();
	result.resize(static_cast<size_t>(size) + 1, 0);

	size_t readed = fread(result.data(), 1, size, hFile);
	result.resize(readed);

	fclose(hFile);
	return true;
}

bool Utils::WriteFile(const wchar_t* fileName, const void* buffer, size_t bufferSize) {
	FILE* hFile;
	if (_wfopen_s(&hFile, fileName, L"wb") || !hFile) {
		Logger::Get().Error(StrUtils::Concat("打开文件 ", StrUtils::UTF16ToUTF8(fileName), " 失败"));
		return false;
	}

	size_t writed = fwrite(buffer, 1, bufferSize, hFile);
	assert(writed == bufferSize);
	UNREFERENCED_PARAMETER(writed);

	fclose(hFile);
	return true;
}

bool Utils::WriteTextFile(const wchar_t* fileName, std::string_view text) {
	FILE* hFile;
	if (_wfopen_s(&hFile, fileName, L"wt") || !hFile) {
		Logger::Get().Error(StrUtils::Concat("打开文件 ", StrUtils::UTF16ToUTF8(fileName), " 失败"));
		return false;
	}

	fwrite(text.data(), 1, text.size(), hFile);

	fclose(hFile);
	return true;
}

RTL_OSVERSIONINFOW _GetOSVersion() noexcept {
	HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
	if (!hNtDll) {
		Logger::Get().Win32Error("获取 ntdll.dll 句柄失败");
		return {};
	}

	auto rtlGetVersion = (LONG(WINAPI*)(PRTL_OSVERSIONINFOW))GetProcAddress(hNtDll, "RtlGetVersion");
	if (rtlGetVersion == nullptr) {
		Logger::Get().Win32Error("获取 RtlGetVersion 地址失败");
		assert(false);
		return {};
	}

	RTL_OSVERSIONINFOW version{};
	version.dwOSVersionInfoSize = sizeof(version);
	rtlGetVersion(&version);

	return version;
}

bool Utils::CreateDir(const std::wstring& path, bool recursive) {
	if (DirExists(path.c_str())) {
		return true;
	}

	if (path.empty()) {
		return false;
	}

	if (!recursive) {
		return CreateDirectory(path.c_str(), nullptr);
	}

	size_t searchOffset = 0;
	do {
		auto tokenPos = path.find_first_of(L"\\/", searchOffset);
		// treat the entire path as a folder if no folder separator not found
		if (tokenPos == std::wstring::npos) {
			tokenPos = path.size();
		}

		std::wstring subdir = path.substr(0, tokenPos);

		if (!subdir.empty() && !DirExists(subdir.c_str()) && !CreateDirectory(subdir.c_str(), nullptr)) {
			Logger::Get().Win32Error(StrUtils::Concat("创建文件夹", StrUtils::UTF16ToUTF8(subdir), "失败"));
			return false; // return error if failed creating dir
		}
		searchOffset = tokenPos + 1;
	} while (searchOffset < path.size());

	return true;
}

const RTL_OSVERSIONINFOW& Utils::GetOSVersion() noexcept {
	static RTL_OSVERSIONINFOW version = _GetOSVersion();
	return version;
}

std::string Utils::Bin2Hex(std::span<const BYTE> data) {
	if (data.size() == 0) {
		return {};
	}

	static char oct2Hex[16] = {
		'0','1','2','3','4','5','6','7',
		'8','9','a','b','c','d','e','f'
	};

	std::string result(data.size() * 2, 0);
	char* pResult = &result[0];

	for (BYTE b : data) {
		*pResult++ = oct2Hex[(b >> 4) & 0xf];
		*pResult++ = oct2Hex[b & 0xf];
	}

	return result;
}


struct TPContext {
	std::function<void(UINT)> func;
	std::atomic<UINT> id;
};

#pragma warning(push)
#pragma warning(disable: 4505)	// 已删除具有内部链接的未引用函数

static void CALLBACK TPCallback(PTP_CALLBACK_INSTANCE, PVOID context, PTP_WORK) {
	TPContext* ctxt = (TPContext*)context;
	UINT id = ++ctxt->id;
	ctxt->func(id);
}

#pragma warning(pop) 

void Utils::RunParallel(std::function<void(UINT)> func, UINT times) {
#ifdef _DEBUG
	// 为了便于调试，DEBUG 模式下不使用线程池
	for (UINT i = 0; i < times; ++i) {
		func(i);
	}
#else
	if (times == 0) {
		return;
	}

	if (times == 1) {
		return func(0);
	}

	TPContext ctxt = { func, 0 };
	PTP_WORK work = CreateThreadpoolWork(TPCallback, &ctxt, nullptr);
	if (work) {
		// 在线程池中执行 times - 1 次
		for (UINT i = 1; i < times; ++i) {
			SubmitThreadpoolWork(work);
		}

		func(0);

		WaitForThreadpoolWorkCallbacks(work, FALSE);
		CloseThreadpoolWork(work);
	} else {
		Logger::Get().Win32Error("CreateThreadpoolWork 失败，回退到单线程");

		// 回退到单线程
		for (UINT i = 0; i < times; ++i) {
			func(i);
		}
	}
#endif // _DEBUG
}

bool Utils::ZstdCompress(std::span<const BYTE> src, std::vector<BYTE>& dest, int compressionLevel) {
	dest.resize(ZSTD_compressBound(src.size()));
	size_t size = ZSTD_compress(dest.data(), dest.size(), src.data(), src.size(), compressionLevel);

	if (ZSTD_isError(size)) {
		Logger::Get().Error(StrUtils::Concat("压缩失败：", ZSTD_getErrorName(size)));
		return false;
	}

	dest.resize(size);
	return true;
}

bool Utils::ZstdDecompress(std::span<const BYTE> src, std::vector<BYTE>& dest) {
	auto size = ZSTD_getFrameContentSize(src.data(), src.size());
	if (size == ZSTD_CONTENTSIZE_UNKNOWN || size == ZSTD_CONTENTSIZE_ERROR) {
		Logger::Get().Error("ZSTD_getFrameContentSize 失败");
		return false;
	}

	dest.resize(size);
	size = ZSTD_decompress(dest.data(), dest.size(), src.data(), src.size());
	if (ZSTD_isError(size)) {
		Logger::Get().Error(StrUtils::Concat("解压失败：", ZSTD_getErrorName(size)));
		return false;
	}

	dest.resize(size);

	return true;
}

bool Utils::IsStartMenu(HWND hwnd) {
	// 作为优化，首先检查窗口类
	wchar_t className[256]{};
	if (!GetClassName(hwnd, (LPWSTR)className, 256)) {
		Logger::Get().Win32Error("GetClassName 失败");
		return false;
	}

	if (std::wcscmp(className, L"Windows.UI.Core.CoreWindow")) {
		return false;
	}

	// 检查可执行文件名称
	DWORD dwProcId = 0;
	if (!GetWindowThreadProcessId(hwnd, &dwProcId)) {
		Logger::Get().Win32Error("GetWindowThreadProcessId 失败");
		return false;
	}

	Utils::ScopedHandle hProc(Utils::SafeHandle(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcId)));
	if (!hProc) {
		Logger::Get().Win32Error("OpenProcess 失败");
		return false;
	}

	wchar_t fileName[MAX_PATH] = { 0 };
	if (!GetModuleFileNameEx(hProc.get(), NULL, fileName, MAX_PATH)) {
		Logger::Get().Win32Error("GetModuleFileName 失败");
		return false;
	}

	std::string exeName = StrUtils::UTF16ToUTF8(fileName);
	exeName = exeName.substr(exeName.find_last_of(L'\\') + 1);
	StrUtils::ToLowerCase(exeName);

	// win10: searchapp.exe 和 startmenuexperiencehost.exe
	// win11: searchhost.exe 和 startmenuexperiencehost.exe
	return exeName == "searchapp.exe" || exeName == "searchhost.exe" || exeName == "startmenuexperiencehost.exe";
}

bool Utils::SetForegroundWindow(HWND hWnd) {
	if (::SetForegroundWindow(hWnd)) {
		return true;
	}

	// 有多种原因会导致 SetForegroundWindow 失败，因此使用一个 trick 强制切换前台窗口
	// 来自 https://pinvoke.net/default.aspx/user32.SetForegroundWindow
	DWORD foreThreadId = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
	DWORD curThreadId = GetCurrentThreadId();

	if (foreThreadId != curThreadId) {
		if (!AttachThreadInput(foreThreadId, curThreadId, TRUE)) {
			Logger::Get().Win32Error("AttachThreadInput 失败");
			return false;
		}
		BringWindowToTop(hWnd);
		ShowWindow(hWnd, SW_SHOW);
		AttachThreadInput(foreThreadId, curThreadId, FALSE);
	} else {
		BringWindowToTop(hWnd);
		ShowWindow(hWnd, SW_SHOW);
	}

	return true;
}

static bool MapKeycodeToUnicode(const int vCode, HKL layout, const BYTE* keyState, std::array<wchar_t, 3>& outBuffer) {
	// Get the scan code from the virtual key code
	const UINT scanCode = MapVirtualKeyEx(vCode, MAPVK_VK_TO_VSC, layout);
	// Get the unicode representation from the virtual key code and scan code pair
	const int result = ToUnicodeEx(vCode, scanCode, keyState, outBuffer.data(), (int)outBuffer.size(), 0, layout);
	return result != 0;
}

static const std::map<DWORD, std::wstring>& GetKeyboardLayout() {
	// 取自 https://github.com/microsoft/PowerToys/blob/fa3a5f80a113568155d9c2dbbcea8af16e15afa1/src/common/interop/keyboard_layout.cpp#L63
	static HKL previousLayout = 0;
	static std::map<DWORD, std::wstring> keyboardLayoutMap;

	// Get keyboard layout for current thread
	const HKL layout = GetKeyboardLayout(0);
	if (layout == previousLayout && !keyboardLayoutMap.empty()) {
		return keyboardLayoutMap;
	}
	previousLayout = layout;

	std::array<BYTE, 256> btKeys = { 0 };
	// Only set the Caps Lock key to on for the key names in uppercase
	btKeys[VK_CAPITAL] = 1;

	// Iterate over all the virtual key codes. virtual key 0 is not used
	for (int i = 1; i < 256; i++) {
		std::array<wchar_t, 3> szBuffer = { 0 };
		if (MapKeycodeToUnicode(i, layout, btKeys.data(), szBuffer)) {
			keyboardLayoutMap[i] = szBuffer.data();
			continue;
		}

		// Store the virtual key code as string
		std::wstring vk = L"VK ";
		vk += std::to_wstring(i);
		keyboardLayoutMap[i] = vk;
	}

	// Override special key names like Shift, Ctrl etc because they don't have unicode mappings and key names like Enter, Space as they appear as "\r", " "
	// To do: localization
	keyboardLayoutMap[VK_CANCEL] = L"Break";
	keyboardLayoutMap[VK_BACK] = L"Backspace";
	keyboardLayoutMap[VK_TAB] = L"Tab";
	keyboardLayoutMap[VK_CLEAR] = L"Clear";
	keyboardLayoutMap[VK_RETURN] = L"Enter";
	keyboardLayoutMap[VK_SHIFT] = L"Shift";
	keyboardLayoutMap[VK_CONTROL] = L"Ctrl";
	keyboardLayoutMap[VK_MENU] = L"Alt";
	keyboardLayoutMap[VK_PAUSE] = L"Pause";
	keyboardLayoutMap[VK_CAPITAL] = L"Caps Lock";
	keyboardLayoutMap[VK_ESCAPE] = L"Esc";
	keyboardLayoutMap[VK_SPACE] = L"Space";
	keyboardLayoutMap[VK_PRIOR] = L"PgUp";
	keyboardLayoutMap[VK_NEXT] = L"PgDn";
	keyboardLayoutMap[VK_END] = L"End";
	keyboardLayoutMap[VK_HOME] = L"Home";
	keyboardLayoutMap[VK_LEFT] = L"Left";
	keyboardLayoutMap[VK_UP] = L"Up";
	keyboardLayoutMap[VK_RIGHT] = L"Right";
	keyboardLayoutMap[VK_DOWN] = L"Down";
	keyboardLayoutMap[VK_SELECT] = L"Select";
	keyboardLayoutMap[VK_PRINT] = L"Print";
	keyboardLayoutMap[VK_EXECUTE] = L"Execute";
	keyboardLayoutMap[VK_SNAPSHOT] = L"Print Screen";
	keyboardLayoutMap[VK_INSERT] = L"Insert";
	keyboardLayoutMap[VK_DELETE] = L"Delete";
	keyboardLayoutMap[VK_HELP] = L"Help";
	keyboardLayoutMap[VK_LWIN] = L"Win (Left)";
	keyboardLayoutMap[VK_RWIN] = L"Win (Right)";
	keyboardLayoutMap[VK_APPS] = L"Apps/Menu";
	keyboardLayoutMap[VK_SLEEP] = L"Sleep";
	keyboardLayoutMap[VK_NUMPAD0] = L"NumPad 0";
	keyboardLayoutMap[VK_NUMPAD1] = L"NumPad 1";
	keyboardLayoutMap[VK_NUMPAD2] = L"NumPad 2";
	keyboardLayoutMap[VK_NUMPAD3] = L"NumPad 3";
	keyboardLayoutMap[VK_NUMPAD4] = L"NumPad 4";
	keyboardLayoutMap[VK_NUMPAD5] = L"NumPad 5";
	keyboardLayoutMap[VK_NUMPAD6] = L"NumPad 6";
	keyboardLayoutMap[VK_NUMPAD7] = L"NumPad 7";
	keyboardLayoutMap[VK_NUMPAD8] = L"NumPad 8";
	keyboardLayoutMap[VK_NUMPAD9] = L"NumPad 9";
	keyboardLayoutMap[VK_SEPARATOR] = L"Separator";
	keyboardLayoutMap[VK_F1] = L"F1";
	keyboardLayoutMap[VK_F2] = L"F2";
	keyboardLayoutMap[VK_F3] = L"F3";
	keyboardLayoutMap[VK_F4] = L"F4";
	keyboardLayoutMap[VK_F5] = L"F5";
	keyboardLayoutMap[VK_F6] = L"F6";
	keyboardLayoutMap[VK_F7] = L"F7";
	keyboardLayoutMap[VK_F8] = L"F8";
	keyboardLayoutMap[VK_F9] = L"F9";
	keyboardLayoutMap[VK_F10] = L"F10";
	keyboardLayoutMap[VK_F11] = L"F11";
	keyboardLayoutMap[VK_F12] = L"F12";
	keyboardLayoutMap[VK_F13] = L"F13";
	keyboardLayoutMap[VK_F14] = L"F14";
	keyboardLayoutMap[VK_F15] = L"F15";
	keyboardLayoutMap[VK_F16] = L"F16";
	keyboardLayoutMap[VK_F17] = L"F17";
	keyboardLayoutMap[VK_F18] = L"F18";
	keyboardLayoutMap[VK_F19] = L"F19";
	keyboardLayoutMap[VK_F20] = L"F20";
	keyboardLayoutMap[VK_F21] = L"F21";
	keyboardLayoutMap[VK_F22] = L"F22";
	keyboardLayoutMap[VK_F23] = L"F23";
	keyboardLayoutMap[VK_F24] = L"F24";
	keyboardLayoutMap[VK_NUMLOCK] = L"Num Lock";
	keyboardLayoutMap[VK_SCROLL] = L"Scroll Lock";
	keyboardLayoutMap[VK_LSHIFT] = L"Shift (Left)";
	keyboardLayoutMap[VK_RSHIFT] = L"Shift (Right)";
	keyboardLayoutMap[VK_LCONTROL] = L"Ctrl (Left)";
	keyboardLayoutMap[VK_RCONTROL] = L"Ctrl (Right)";
	keyboardLayoutMap[VK_LMENU] = L"Alt (Left)";
	keyboardLayoutMap[VK_RMENU] = L"Alt (Right)";
	keyboardLayoutMap[VK_BROWSER_BACK] = L"Browser Back";
	keyboardLayoutMap[VK_BROWSER_FORWARD] = L"Browser Forward";
	keyboardLayoutMap[VK_BROWSER_REFRESH] = L"Browser Refresh";
	keyboardLayoutMap[VK_BROWSER_STOP] = L"Browser Stop";
	keyboardLayoutMap[VK_BROWSER_SEARCH] = L"Browser Search";
	keyboardLayoutMap[VK_BROWSER_FAVORITES] = L"Browser Favorites";
	keyboardLayoutMap[VK_BROWSER_HOME] = L"Browser Home";
	keyboardLayoutMap[VK_VOLUME_MUTE] = L"Volume Mute";
	keyboardLayoutMap[VK_VOLUME_DOWN] = L"Volume Down";
	keyboardLayoutMap[VK_VOLUME_UP] = L"Volume Up";
	keyboardLayoutMap[VK_MEDIA_NEXT_TRACK] = L"Next Track";
	keyboardLayoutMap[VK_MEDIA_PREV_TRACK] = L"Previous Track";
	keyboardLayoutMap[VK_MEDIA_STOP] = L"Stop Media";
	keyboardLayoutMap[VK_MEDIA_PLAY_PAUSE] = L"Play/Pause Media";
	keyboardLayoutMap[VK_LAUNCH_MAIL] = L"Start Mail";
	keyboardLayoutMap[VK_LAUNCH_MEDIA_SELECT] = L"Select Media";
	keyboardLayoutMap[VK_LAUNCH_APP1] = L"Start App 1";
	keyboardLayoutMap[VK_LAUNCH_APP2] = L"Start App 2";
	keyboardLayoutMap[VK_PACKET] = L"Packet";
	keyboardLayoutMap[VK_ATTN] = L"Attn";
	keyboardLayoutMap[VK_CRSEL] = L"CrSel";
	keyboardLayoutMap[VK_EXSEL] = L"ExSel";
	keyboardLayoutMap[VK_EREOF] = L"Erase EOF";
	keyboardLayoutMap[VK_PLAY] = L"Play";
	keyboardLayoutMap[VK_ZOOM] = L"Zoom";
	keyboardLayoutMap[VK_PA1] = L"PA1";
	keyboardLayoutMap[VK_OEM_CLEAR] = L"Clear";
	keyboardLayoutMap[0xFF] = L"Undefined";
	// keyboardLayoutMap[CommonSharedConstants::VK_WIN_BOTH] = L"Win";
	keyboardLayoutMap[VK_KANA] = L"IME Kana";
	keyboardLayoutMap[VK_HANGEUL] = L"IME Hangeul";
	keyboardLayoutMap[VK_HANGUL] = L"IME Hangul";
	keyboardLayoutMap[VK_JUNJA] = L"IME Junja";
	keyboardLayoutMap[VK_FINAL] = L"IME Final";
	keyboardLayoutMap[VK_HANJA] = L"IME Hanja";
	keyboardLayoutMap[VK_KANJI] = L"IME Kanji";
	keyboardLayoutMap[VK_CONVERT] = L"IME Convert";
	keyboardLayoutMap[VK_NONCONVERT] = L"IME Non-Convert";
	keyboardLayoutMap[VK_ACCEPT] = L"IME Kana";
	keyboardLayoutMap[VK_MODECHANGE] = L"IME Mode Change";
	// keyboardLayoutMap[CommonSharedConstants::VK_DISABLED] = L"Disable";

	return keyboardLayoutMap;
}

std::wstring Utils::GetKeyName(DWORD key) {
	const std::map<DWORD, std::wstring>& keyboardLayoutMap = GetKeyboardLayout();

	auto it = keyboardLayoutMap.find(key);
	if (it != keyboardLayoutMap.end()) {
		return it->second;
	} else {
		return L"Undefined";
	}
}

Utils::Hasher::~Hasher() {
	if (_hAlg) {
		BCryptCloseAlgorithmProvider(_hAlg, 0);
	}
	if (_hashObj) {
		HeapFree(GetProcessHeap(), 0, _hashObj);
	}
	if (_hHash) {
		BCryptDestroyHash(_hHash);
	}
}

bool Utils::Hasher::Initialize() {
	NTSTATUS status = BCryptOpenAlgorithmProvider(&_hAlg, BCRYPT_SHA1_ALGORITHM, NULL, 0);
	if (!NT_SUCCESS(status)) {
		Logger::Get().Error(fmt::format("BCryptOpenAlgorithmProvider 失败\n\tNTSTATUS={}", status));
		return false;
	}

	ULONG result;

	status = BCryptGetProperty(_hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&_hashObjLen, sizeof(_hashObjLen), &result, 0);
	if (!NT_SUCCESS(status)) {
		Logger::Get().Error(fmt::format("BCryptGetProperty 失败\n\tNTSTATUS={}", status));
		return false;
	}

	_hashObj = HeapAlloc(GetProcessHeap(), 0, _hashObjLen);
	if (!_hashObj) {
		Logger::Get().Error("HeapAlloc 失败");
		return false;
	}

	status = BCryptGetProperty(_hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&_hashLen, sizeof(_hashLen), &result, 0);
	if (!NT_SUCCESS(status)) {
		Logger::Get().Error(fmt::format("BCryptGetProperty 失败\n\tNTSTATUS={}", status));
		return false;
	}

	status = BCryptCreateHash(_hAlg, &_hHash, (PUCHAR)_hashObj, _hashObjLen, NULL, 0, BCRYPT_HASH_REUSABLE_FLAG);
	if (!NT_SUCCESS(status)) {
		Logger::Get().Error(fmt::format("BCryptCreateHash 失败\n\tNTSTATUS={}", status));
		return false;
	}

	Logger::Get().Info("Utils::Hasher 初始化成功");
	return true;
}

bool Utils::Hasher::Hash(std::span<const BYTE> data, std::vector<BYTE>& result) {
	// BCrypt API 内部保存状态，因此需要同步对它们访问
	std::scoped_lock lk(_cs);

	result.resize(_hashLen);

	NTSTATUS status = BCryptHashData(_hHash, (PUCHAR)data.data(), (ULONG)data.size(), 0);
	if (!NT_SUCCESS(status)) {
		Logger::Get().Error(fmt::format("BCryptCreateHash 失败\n\tNTSTATUS={}", status));
		return false;
	}

	status = BCryptFinishHash(_hHash, result.data(), (ULONG)result.size(), 0);
	if (!NT_SUCCESS(status)) {
		Logger::Get().Error(fmt::format("BCryptFinishHash 失败\n\tNTSTATUS={}", status));
		return false;
	}

	return true;
}

hstring winrt::to_hstring(Windows::System::VirtualKey status) {
	switch (status) {
	case VirtualKey::None:
		return L"None";
	case VirtualKey::LeftButton:
		return L"LeftButton";
	case VirtualKey::RightButton:
		return L"RightButton";
	case VirtualKey::Cancel:
		return L"Cancel";
	case VirtualKey::MiddleButton:
		return L"MiddleButton";
	case VirtualKey::XButton1:
		return L"XButton1";
	case VirtualKey::XButton2:
		return L"XButton2";
	case VirtualKey::Back:
		return L"Back";
	case VirtualKey::Tab:
		return L"Tab";
	case VirtualKey::Clear:
		return L"Clear";
	case VirtualKey::Enter:
		return L"Enter";
	case VirtualKey::Shift:
		return L"Shift";
	case VirtualKey::Control:
		return L"Control";
	case VirtualKey::Menu:
		return L"Menu";
	case VirtualKey::Pause:
		return L"Pause";
	case VirtualKey::CapitalLock:
		return L"CapitalLock";
	case VirtualKey::Kana:
	/*case VirtualKey::Hangul:*/
		return L"Kana";
	case VirtualKey::ImeOn:
		return L"ImeOn";
	case VirtualKey::Junja:
		return L"Junja";
	case VirtualKey::Final:
		return L"Final";
	case VirtualKey::Hanja:
	/*case VirtualKey::Kanji:*/
		return L"Hanja";
	case VirtualKey::ImeOff:
		return L"ImeOff";
	case VirtualKey::Escape:
		return L"Escape";
	case VirtualKey::Convert:
		return L"Convert";
	case VirtualKey::NonConvert:
		return L"NonConvert";
	case VirtualKey::Accept:
		return L"Accept";
	case VirtualKey::ModeChange:
		return L"ModeChange";
	case VirtualKey::Space:
		return L"Space";
	case VirtualKey::PageUp:
		return L"PageUp";
	case VirtualKey::PageDown:
		return L"PageDown";
	case VirtualKey::End:
		return L"End";
	case VirtualKey::Home:
		return L"Home";
	case VirtualKey::Left:
		return L"Left";
	case VirtualKey::Up:
		return L"Up";
	case VirtualKey::Right:
		return L"Right";
	case VirtualKey::Down:
		return L"Down";
	case VirtualKey::Select:
		return L"Select";
	case VirtualKey::Print:
		return L"Print";
	case VirtualKey::Execute:
		return L"Execute";
	case VirtualKey::Snapshot:
		return L"Snapshot";
	case VirtualKey::Insert:
		return L"Insert";
	case VirtualKey::Delete:
		return L"Delete";
	case VirtualKey::Help:
		return L"Help";
	case VirtualKey::Number0:
		return L"Number0";
	case VirtualKey::Number1:
		return L"Number1";
	case VirtualKey::Number2:
		return L"Number2";
	case VirtualKey::Number3:
		return L"Number3";
	case VirtualKey::Number4:
		return L"Number4";
	case VirtualKey::Number5:
		return L"Number5";
	case VirtualKey::Number6:
		return L"Number6";
	case VirtualKey::Number7:
		return L"Number7";
	case VirtualKey::Number8:
		return L"Number8";
	case VirtualKey::Number9:
		return L"Number9";
	case VirtualKey::A:
		return L"A";
	case VirtualKey::B:
		return L"B";
	case VirtualKey::C:
		return L"C";
	case VirtualKey::D:
		return L"D";
	case VirtualKey::E:
		return L"E";
	case VirtualKey::F:
		return L"F";
	case VirtualKey::G:
		return L"G";
	case VirtualKey::H:
		return L"H";
	case VirtualKey::I:
		return L"I";
	case VirtualKey::J:
		return L"J";
	case VirtualKey::K:
		return L"K";
	case VirtualKey::L:
		return L"L";
	case VirtualKey::M:
		return L"M";
	case VirtualKey::N:
		return L"N";
	case VirtualKey::O:
		return L"O";
	case VirtualKey::P:
		return L"P";
	case VirtualKey::Q:
		return L"Q";
	case VirtualKey::R:
		return L"R";
	case VirtualKey::S:
		return L"S";
	case VirtualKey::T:
		return L"T";
	case VirtualKey::U:
		return L"U";
	case VirtualKey::V:
		return L"V";
	case VirtualKey::W:
		return L"W";
	case VirtualKey::X:
		return L"X";
	case VirtualKey::Y:
		return L"Y";
	case VirtualKey::Z:
		return L"Z";
	case VirtualKey::LeftWindows:
		return L"LeftWindows";
	case VirtualKey::RightWindows:
		return L"RightWindows";
	case VirtualKey::Application:
		return L"Application";
	case VirtualKey::Sleep:
		return L"Sleep";
	case VirtualKey::NumberPad0:
		return L"NumberPad0";
	case VirtualKey::NumberPad1:
		return L"NumberPad1";
	case VirtualKey::NumberPad2:
		return L"NumberPad2";
	case VirtualKey::NumberPad3:
		return L"NumberPad3";
	case VirtualKey::NumberPad4:
		return L"NumberPad4";
	case VirtualKey::NumberPad5:
		return L"NumberPad5";
	case VirtualKey::NumberPad6:
		return L"NumberPad6";
	case VirtualKey::NumberPad7:
		return L"NumberPad7";
	case VirtualKey::NumberPad8:
		return L"NumberPad8";
	case VirtualKey::NumberPad9:
		return L"NumberPad9";
	case VirtualKey::Multiply:
		return L"Multiply";
	case VirtualKey::Add:
		return L"Add";
	case VirtualKey::Separator:
		return L"Separator";
	case VirtualKey::Subtract:
		return L"Subtract";
	case VirtualKey::Decimal:
		return L"Decimal";
	case VirtualKey::Divide:
		return L"Divide";
	case VirtualKey::F1:
		return L"F1";
	case VirtualKey::F2:
		return L"F2";
	case VirtualKey::F3:
		return L"F3";
	case VirtualKey::F4:
		return L"F4";
	case VirtualKey::F5:
		return L"F5";
	case VirtualKey::F6:
		return L"F6";
	case VirtualKey::F7:
		return L"F7";
	case VirtualKey::F8:
		return L"F8";
	case VirtualKey::F9:
		return L"F9";
	case VirtualKey::F10:
		return L"F10";
	case VirtualKey::F11:
		return L"F11";
	case VirtualKey::F12:
		return L"F12";
	case VirtualKey::F13:
		return L"F13";
	case VirtualKey::F14:
		return L"F14";
	case VirtualKey::F15:
		return L"F15";
	case VirtualKey::F16:
		return L"F16";
	case VirtualKey::F17:
		return L"F17";
	case VirtualKey::F18:
		return L"F18";
	case VirtualKey::F19:
		return L"F19";
	case VirtualKey::F20:
		return L"F20";
	case VirtualKey::F21:
		return L"F21";
	case VirtualKey::F22:
		return L"F22";
	case VirtualKey::F23:
		return L"F23";
	case VirtualKey::F24:
		return L"F24";
	case VirtualKey::NavigationView:
		return L"NavigationView";
	case VirtualKey::NavigationMenu:
		return L"NavigationMenu";
	case VirtualKey::NavigationUp:
		return L"NavigationUp";
	case VirtualKey::NavigationDown:
		return L"NavigationDown";
	case VirtualKey::NavigationLeft:
		return L"NavigationLeft";
	case VirtualKey::NavigationRight:
		return L"NavigationRight";
	case VirtualKey::NavigationAccept:
		return L"NavigationAccept";
	case VirtualKey::NavigationCancel:
		return L"NavigationCancel";
	case VirtualKey::NumberKeyLock:
		return L"NumberKeyLock";
	case VirtualKey::Scroll:
		return L"Scroll";
	case VirtualKey::LeftShift:
		return L"LeftShift";
	case VirtualKey::RightShift:
		return L"RightShift";
	case VirtualKey::LeftControl:
		return L"LeftControl";
	case VirtualKey::RightControl:
		return L"RightControl";
	case VirtualKey::LeftMenu:
		return L"LeftMenu";
	case VirtualKey::RightMenu:
		return L"RightMenu";
	case VirtualKey::GoBack:
		return L"GoBack";
	case VirtualKey::GoForward:
		return L"GoForward";
	case VirtualKey::Refresh:
		return L"Refresh";
	case VirtualKey::Stop:
		return L"Stop";
	case VirtualKey::Search:
		return L"Search";
	case VirtualKey::Favorites:
		return L"Favorites";
	case VirtualKey::GoHome:
		return L"GoHome";
	case VirtualKey::GamepadA:
		return L"GamepadA";
	case VirtualKey::GamepadB:
		return L"GamepadB";
	case VirtualKey::GamepadX:
		return L"GamepadX";
	case VirtualKey::GamepadY:
		return L"GamepadY";
	case VirtualKey::GamepadRightShoulder:
		return L"GamepadRightShoulder";
	case VirtualKey::GamepadLeftShoulder:
		return L"GamepadLeftShoulder";
	case VirtualKey::GamepadLeftTrigger:
		return L"GamepadLeftTrigger";
	case VirtualKey::GamepadRightTrigger:
		return L"GamepadRightTrigger";
	case VirtualKey::GamepadDPadUp:
		return L"GamepadDPadUp";
	case VirtualKey::GamepadDPadDown:
		return L"GamepadDPadDown";
	case VirtualKey::GamepadDPadLeft:
		return L"GamepadDPadLeft";
	case VirtualKey::GamepadDPadRight:
		return L"GamepadDPadRight";
	case VirtualKey::GamepadMenu:
		return L"GamepadMenu";
	case VirtualKey::GamepadView:
		return L"GamepadView";
	case VirtualKey::GamepadLeftThumbstickButton:
		return L"GamepadLeftThumbstickButton";
	case VirtualKey::GamepadRightThumbstickButton:
		return L"GamepadRightThumbstickButton";
	case VirtualKey::GamepadLeftThumbstickUp:
		return L"GamepadLeftThumbstickUp";
	case VirtualKey::GamepadLeftThumbstickDown:
		return L"GamepadLeftThumbstickDown";
	case VirtualKey::GamepadLeftThumbstickRight:
		return L"GamepadLeftThumbstickRight";
	case VirtualKey::GamepadLeftThumbstickLeft:
		return L"GamepadLeftThumbstickLeft";
	case VirtualKey::GamepadRightThumbstickUp:
		return L"GamepadRightThumbstickUp";
	case VirtualKey::GamepadRightThumbstickDown:
		return L"NonGamepadRightThumbstickDowne";
	case VirtualKey::GamepadRightThumbstickRight:
		return L"GamepadRightThumbstickRight";
	case VirtualKey::GamepadRightThumbstickLeft:
		return L"GamepadRightThumbstickLeft";
	default:
		break;
	}

	return L"";
}
