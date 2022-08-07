#include "pch.h"
#include "Win32Utils.h"
#include "Logger.h"
#include "StrUtils.h"
#include <io.h>
#include <Psapi.h>
#include <winternl.h>
#include <dwmapi.h>
#include <magnification.h>

#pragma comment(lib, "Magnification.lib")


UINT Win32Utils::GetOSBuild() {
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

std::wstring Win32Utils::GetWndClassName(HWND hWnd) {
	// 窗口类名最多 256 个字符
	std::wstring className(256, 0);
	int num = GetClassName(hWnd, &className[0], (int)className.size() + 1);
	if (num == 0) {
		Logger::Get().Win32Error("GetClassName 失败");
		return {};
	}

	className.resize(num);
	return className;
}

std::wstring Win32Utils::GetWndTitle(HWND hWnd) {
	int len = GetWindowTextLength(hWnd);
	if (len == 0) {
		return {};
	}

	std::wstring title(len, 0);
	len = GetWindowText(hWnd, title.data(), len + 1);
	title.resize(len);
	return title;
}

std::wstring Win32Utils::GetPathOfWnd(HWND hWnd) {
	DWORD dwProcId = 0;
	if (!GetWindowThreadProcessId(hWnd, &dwProcId)) {
		Logger::Get().Win32Error("GetWindowThreadProcessId 失败");
		return {};
	}

	ScopedHandle hProc(SafeHandle(OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcId)));
	if (!hProc) {
		Logger::Get().Win32Error("OpenProcess 失败");
		return {};
	}

	std::wstring fileName(MAX_PATH, 0);
	DWORD size = GetModuleFileNameEx(hProc.get(), NULL, fileName.data(), (DWORD)fileName.size() + 1);
	if (size == 0) {
		Logger::Get().Win32Error("GetModuleFileName 失败");
		return {};
	}

	fileName.resize(size);
	return fileName;
}

UINT Win32Utils::GetWindowShowCmd(HWND hWnd) {
	assert(hWnd != NULL);

	WINDOWPLACEMENT wp{};
	wp.length = sizeof(wp);
	if (!GetWindowPlacement(hWnd, &wp)) {
		Logger::Get().Win32Error("GetWindowPlacement 出错");
		assert(false);
	}

	return wp.showCmd;
}

bool Win32Utils::GetClientScreenRect(HWND hWnd, RECT& rect) {
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

bool Win32Utils::GetWindowFrameRect(HWND hWnd, RECT& result) {
	HRESULT hr = DwmGetWindowAttribute(hWnd,
		DWMWA_EXTENDED_FRAME_BOUNDS, &result, sizeof(result));
	if (FAILED(hr)) {
		return false;
	}

	return true;
}


bool Win32Utils::ReadFile(const wchar_t* fileName, std::vector<BYTE>& result) {
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

bool Win32Utils::ReadTextFile(const wchar_t* fileName, std::string& result) {
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

bool Win32Utils::WriteFile(const wchar_t* fileName, const void* buffer, size_t bufferSize) {
	FILE* hFile;
	if (_wfopen_s(&hFile, fileName, L"wb") || !hFile) {
		Logger::Get().Error(StrUtils::Concat("打开文件 ", StrUtils::UTF16ToUTF8(fileName), " 失败"));
		return false;
	}

	[[maybe_unused]] size_t writed = fwrite(buffer, 1, bufferSize, hFile);
	assert(writed == bufferSize);

	fclose(hFile);
	return true;
}

bool Win32Utils::WriteTextFile(const wchar_t* fileName, std::string_view text) {
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

bool Win32Utils::CreateDir(const std::wstring& path, bool recursive) {
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

const RTL_OSVERSIONINFOW& Win32Utils::GetOSVersion() noexcept {
	static RTL_OSVERSIONINFOW version = _GetOSVersion();
	return version;
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

void Win32Utils::RunParallel(std::function<void(UINT)> func, UINT times) {
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


bool Win32Utils::IsStartMenu(HWND hWnd) {
	// 作为优化，首先检查窗口类
	std::wstring className = GetWndClassName(hWnd);

	if (className != L"Windows.UI.Core.CoreWindow") {
		return false;
	}

	// 检查可执行文件名称
	std::wstring exeName = GetPathOfWnd(hWnd);
	exeName = exeName.substr(exeName.find_last_of(L'\\') + 1);
	StrUtils::ToLowerCase(exeName);

	// win10: searchapp.exe 和 startmenuexperiencehost.exe
	// win11: searchhost.exe 和 startmenuexperiencehost.exe
	return exeName == L"searchapp.exe" || exeName == L"searchhost.exe" || exeName == L"startmenuexperiencehost.exe";
}

bool Win32Utils::SetForegroundWindow(HWND hWnd) {
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

bool Win32Utils::ShowSystemCursor(bool value) {
	bool result = false;

	static void (WINAPI* const showSystemCursor)(BOOL bShow) = []()->void(WINAPI*)(BOOL) {
		HMODULE lib = LoadLibrary(L"user32.dll");
		if (!lib) {
			return nullptr;
		}

		return (void(WINAPI*)(BOOL))GetProcAddress(lib, "ShowSystemCursor");
	}();

	if (showSystemCursor) {
		showSystemCursor((BOOL)value);
		result = true;
	} else {
		// 获取 ShowSystemCursor 失败则回落到 Magnification API
		static bool initialized = []() {
			if (!MagInitialize()) {
				Logger::Get().Win32Error("MagInitialize 失败");
				return false;
			}

			return true;
		}();

		result = initialized ? (bool)MagShowSystemCursor(value) : false;
	}

	if (result && value) {
		// 修复有时不会立即生效的问题
		SystemParametersInfo(SPI_SETCURSORS, 0, 0, 0);
	}
	return result;
}

static bool MapKeycodeToUnicode(const int vCode, HKL layout, const BYTE* keyState, std::array<wchar_t, 3>& outBuffer) {
	// Get the scan code from the virtual key code
	const UINT scanCode = MapVirtualKeyEx(vCode, MAPVK_VK_TO_VSC, layout);
	// Get the unicode representation from the virtual key code and scan code pair
	const int result = ToUnicodeEx(vCode, scanCode, keyState, outBuffer.data(), (int)outBuffer.size(), 0, layout);
	return result != 0;
}

static const std::unordered_map<DWORD, std::wstring>& GetKeyboardLayout() {
	// 取自 https://github.com/microsoft/PowerToys/blob/fa3a5f80a113568155d9c2dbbcea8af16e15afa1/src/common/interop/keyboard_layout.cpp#L63
	static HKL previousLayout = 0;
	static std::unordered_map<DWORD, std::wstring> keyboardLayoutMap;

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

std::wstring Win32Utils::GetKeyName(DWORD key) {
	const std::unordered_map<DWORD, std::wstring>& keyboardLayoutMap = GetKeyboardLayout();

	auto it = keyboardLayoutMap.find(key);
	if (it != keyboardLayoutMap.end()) {
		return it->second;
	} else {
		return L"Undefined";
	}
}

bool Win32Utils::IsProcessElevated() noexcept {
	static INT result = 0;

	if (result == 0) {
		// https://web.archive.org/web/20100418044859/http://msdn.microsoft.com/en-us/windows/ff420334.aspx
		BYTE adminSID[SECURITY_MAX_SID_SIZE]{};
		DWORD dwLength = sizeof(adminSID);
		if (!CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, &adminSID, &dwLength)) {
			Logger::Get().Win32Error("CreateWellKnownSid 失败");
			result = -1;
			return false;
		}

		BOOL isAdmin;
		if (!CheckTokenMembership(NULL, adminSID, &isAdmin)) {
			Logger::Get().Win32Error("CheckTokenMembership 失败");
			result = -1;
			return false;
		}

		result = isAdmin ? 1 : -1;
	}

	return bool(result == 1);
}

Win32Utils::Hasher::~Hasher() {
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

bool Win32Utils::Hasher::_Initialize() {
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

	return true;
}

bool Win32Utils::Hasher::Hash(std::span<const BYTE> data, std::vector<BYTE>& result) {
	// BCrypt API 内部保存状态，因此需要同步对它们访问
	std::scoped_lock lk(_srwMutex);

	if (!_hAlg && !_Initialize()) {
		Logger::Get().Error("初始化失败");
		return false;
	}

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

DWORD Win32Utils::Hasher::GetHashLength() noexcept {
	if (!_hAlg && !_Initialize()) {
		Logger::Get().Error("初始化失败");
		return 0;
	}

	return _hashLen;
}

Win32Utils::BStr::BStr(std::wstring_view str) {
	_str = SysAllocStringLen(str.data(), (UINT)str.size());;
}

Win32Utils::BStr::BStr(const BStr& other) {
	_str = SysAllocStringLen(other._str, SysStringLen(other._str));
}

Win32Utils::BStr::~BStr() {
	_Release();
}

Win32Utils::BStr& Win32Utils::BStr::operator=(const BStr& other) {
	_Release();
	_str = SysAllocStringLen(other._str, SysStringLen(other._str));
	return *this;
}

Win32Utils::BStr& Win32Utils::BStr::operator=(BStr&& other) {
	_Release();
	_str = other._str;
	other._str = NULL;
	return *this;
}

std::string Win32Utils::BStr::ToUTF8() const {
	if (!_str) {
		return {};
	}

	std::wstring_view str(_str, SysStringLen(_str));
	return StrUtils::UTF16ToUTF8(str);
}

void Win32Utils::BStr::_Release() {
	if (_str) {
		SysFreeString(_str);
	}
}
