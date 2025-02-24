#include "pch.h"
#include "Win32Helper.h"
#include "Logger.h"
#include "StrHelper.h"
#include <io.h>
#include <Psapi.h>
#include <winternl.h>
#include <dwmapi.h>
#include <parallel_hashmap/phmap.h>
#include <wil/token_helpers.h>
#include <ShlObj.h>
#include <shellapi.h>

namespace Magpie {

std::wstring Win32Helper::GetWndClassName(HWND hWnd) noexcept {
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

std::wstring Win32Helper::GetWndTitle(HWND hWnd) noexcept {
	int len = GetWindowTextLength(hWnd);
	if (len == 0) {
		return {};
	}

	std::wstring title(len, 0);
	len = GetWindowText(hWnd, title.data(), len + 1);
	title.resize(len);
	return title;
}

wil::unique_process_handle Win32Helper::GetWndProcessHandle(HWND hWnd) noexcept {
	wil::unique_process_handle hProc;

	if (DWORD dwProcId = 0; GetWindowThreadProcessId(hWnd, &dwProcId)) {
		hProc.reset(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcId));
		if (!hProc) {
			Logger::Get().Win32Error("OpenProcess 失败");
		}
	} else {
		Logger::Get().Win32Error("GetWindowThreadProcessId 失败");
	}

	if (!hProc) {
		// 在某些窗口上 OpenProcess 会失败（如暗黑 2），尝试使用 GetProcessHandleFromHwnd
		static const auto getProcessHandleFromHwnd = (HANDLE(WINAPI*)(HWND))GetProcAddress(
			LoadLibraryEx(L"Oleacc.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32),
			"GetProcessHandleFromHwnd"
		);
		if (getProcessHandleFromHwnd) {
			hProc.reset(getProcessHandleFromHwnd(hWnd));
			if (!hProc) {
				Logger::Get().Win32Error("GetProcessHandleFromHwnd 失败");
			}
		}
	}

	return hProc;
}

std::wstring Win32Helper::GetPathOfWnd(HWND hWnd) noexcept {
	wil::unique_process_handle hProc = GetWndProcessHandle(hWnd);
	if (!hProc) {
		Logger::Get().Error("GetWndProcessHandle 失败");
		return {};
	}

	std::wstring fileName;
	HRESULT hr = wil::QueryFullProcessImageNameW(hProc.get(), 0, fileName);
	if (FAILED(hr)) {
		Logger::Get().ComError("QueryFullProcessImageNameW 失败", hr);
		return {};
	}

	return fileName;
}

UINT Win32Helper::GetWindowShowCmd(HWND hWnd) noexcept {
	assert(hWnd != NULL);

	WINDOWPLACEMENT wp{ .length = sizeof(wp) };
	if (!GetWindowPlacement(hWnd, &wp)) {
		Logger::Get().Win32Error("GetWindowPlacement 出错");
	}

	return wp.showCmd;
}

bool Win32Helper::GetClientScreenRect(HWND hWnd, RECT& rect) noexcept {
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

bool Win32Helper::GetWindowFrameRect(HWND hWnd, RECT& rect) noexcept {
	HRESULT hr = DwmGetWindowAttribute(hWnd,
		DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect));
	if (FAILED(hr)) {
		Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
		return false;
	}

	// Win11 中最大化的窗口的 extended frame bounds 有一部分在屏幕外面，
	// 不清楚 Win10 是否有这种情况
	if (GetWindowShowCmd(hWnd) == SW_SHOWMAXIMIZED) {
		HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi{ .cbSize = sizeof(mi) };
		if (!GetMonitorInfo(hMon, &mi)) {
			Logger::Get().Win32Error("GetMonitorInfo 失败");
			return false;
		}

		// 不能裁剪到工作区，因为窗口可以使用 SetWindowPos 以任意尺寸显示“最大化”
		// 的窗口，缩放窗口就使用了这个技术以和 Wallpaper Engine 兼容。OS 虽然不
		// 会阻止跨越多个屏幕，但只在一个屏幕上有画面，因此可以认为最大化的窗口只在
		// 一个屏幕上。
		// 注意 Win11 中最大化窗口的 extended frame bounds 包含了下边框，但对我们
		// 没有影响，因为缩放时下边框始终会被裁剪掉。
		IntersectRect(&rect, &rect, &mi.rcMonitor);
	}

	// 对于使用 SetWindowRgn 自定义形状的窗口，裁剪到最小矩形边框
	RECT rgnRect;
	int regionType = GetWindowRgnBox(hWnd, &rgnRect);
	if (regionType == SIMPLEREGION || regionType == COMPLEXREGION) {
		RECT windowRect;
		if (!GetWindowRect(hWnd, &windowRect)) {
			Logger::Get().Win32Error("GetWindowRect 失败");
			return false;
		}

		// 转换为屏幕坐标
		OffsetRect(&rgnRect, windowRect.left, windowRect.top);

		IntersectRect(&rect, &rect, &rgnRect);
	}

	return true;
}

uint32_t Win32Helper::GetNativeWindowBorderThickness(uint32_t dpi) noexcept {
	if (GetOSVersion().IsWin11()) {
		// 这里的计算方式是通过实验总结出来的。DwmGetWindowAttribute 有两个问题:
		// 1. 它要求窗口存在，而有些时候我们需要在创建窗口前计算窗口尺寸。
		// 2. 如果窗口被 DPI 虚拟化，它返回的结果是错误的。
		return (dpi + USER_DEFAULT_SCREEN_DPI / 2) / USER_DEFAULT_SCREEN_DPI;
	} else {
		return 1;
	}
}

bool Win32Helper::ReadFile(const wchar_t* fileName, std::vector<uint8_t>& result) noexcept {
	Logger::Get().Info(StrHelper::Concat("读取文件: ", StrHelper::UTF16ToUTF8(fileName)));

	CREATEFILE2_EXTENDED_PARAMETERS extendedParams{
		.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS),
		.dwFileAttributes = FILE_ATTRIBUTE_NORMAL,
		.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN,
		.dwSecurityQosFlags = SECURITY_ANONYMOUS
	};

	wil::unique_hfile hFile(CreateFile2(fileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));

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

bool Win32Helper::ReadTextFile(const wchar_t* fileName, std::string& result) noexcept {
	wil::unique_file hFile;
	if (_wfopen_s(hFile.put(), fileName, L"rt") || !hFile) {
		Logger::Get().Error(StrHelper::Concat("打开文件 ", StrHelper::UTF16ToUTF8(fileName), " 失败"));
		return false;
	}

	// 获取文件长度
	int fd = _fileno(hFile.get());
	long size = _filelength(fd);

	result.clear();
	result.resize(static_cast<size_t>(size) + 1, 0);

	size_t readed = fread(result.data(), 1, size, hFile.get());
	result.resize(readed);

	return true;
}

bool Win32Helper::WriteFile(const wchar_t* fileName, const void* buffer, size_t bufferSize) noexcept {
	wil::unique_file hFile;
	if (_wfopen_s(hFile.put(), fileName, L"wb") || !hFile) {
		Logger::Get().Error(StrHelper::Concat("打开文件 ", StrHelper::UTF16ToUTF8(fileName), " 失败"));
		return false;
	}

	if (bufferSize > 0) {
		[[maybe_unused]] size_t writed = fwrite(buffer, 1, bufferSize, hFile.get());
		assert(writed == bufferSize);
	}

	return true;
}

bool Win32Helper::WriteTextFile(const wchar_t* fileName, std::string_view text) noexcept {
	wil::unique_file hFile;
	if (_wfopen_s(hFile.put(), fileName, L"wt") || !hFile) {
		Logger::Get().Error(StrHelper::Concat("打开文件 ", StrHelper::UTF16ToUTF8(fileName), " 失败"));
		return false;
	}

	fwrite(text.data(), 1, text.size(), hFile.get());
	return true;
}

bool Win32Helper::FileExists(const wchar_t* fileName) noexcept {
	DWORD attrs = GetFileAttributes(fileName);
	// 排除文件夹
	return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool Win32Helper::DirExists(const wchar_t* fileName) noexcept {
	DWORD attrs = GetFileAttributes(fileName);
	return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}

bool Win32Helper::CreateDir(const std::wstring& path, bool recursive) noexcept {
	assert(!path.empty());

	if (DirExists(path.c_str())) {
		return true;
	}

	if (!recursive) {
		return CreateDirectory(path.c_str(), nullptr);
	}

	size_t searchOffset = 0;
	do {
		auto segPos = path.find_first_of(L'\\', searchOffset);
		if (segPos == std::wstring::npos) {
			// 没有分隔符则将整个路径视为文件夹
			segPos = path.size();
		}

		std::wstring subdir = path.substr(0, segPos);
		if (!subdir.empty() && !DirExists(subdir.c_str()) && !CreateDirectory(subdir.c_str(), nullptr)) {
			return false;
		}

		searchOffset = segPos + 1;
	} while (searchOffset < path.size());

	return true;
}

const Win32Helper::OSVersion& Win32Helper::GetOSVersion() noexcept {
	static OSVersion version = []() -> OSVersion {
		HMODULE hNtDll = GetModuleHandle(L"ntdll.dll");
		assert(hNtDll);

		auto rtlGetVersion = (LONG(WINAPI*)(PRTL_OSVERSIONINFOW))GetProcAddress(hNtDll, "RtlGetVersion");
		if (!rtlGetVersion) {
			Logger::Get().Win32Error("获取 RtlGetVersion 地址失败");
			assert(false);
			return {};
		}

		RTL_OSVERSIONINFOW versionInfo{ .dwOSVersionInfoSize = sizeof(versionInfo) };
		rtlGetVersion(&versionInfo);

		return { versionInfo.dwMajorVersion, versionInfo.dwMinorVersion, versionInfo.dwBuildNumber };
	}();

	return version;
}

struct TPContext {
	std::function<void(uint32_t)> func;
	std::atomic<uint32_t> id;
};

[[maybe_unused]]
static void CALLBACK TPCallback(PTP_CALLBACK_INSTANCE, PVOID context, PTP_WORK) {
	TPContext* ctxt = (TPContext*)context;
	const uint32_t id = ctxt->id.fetch_add(1, std::memory_order_relaxed) + 1;
	ctxt->func(id);
}

void Win32Helper::RunParallel(std::function<void(uint32_t)> func, uint32_t times) noexcept {
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
		for (uint32_t i = 1; i < times; ++i) {
			SubmitThreadpoolWork(work);
		}

		func(0);

		WaitForThreadpoolWorkCallbacks(work, FALSE);
		CloseThreadpoolWork(work);
	} else {
		Logger::Get().Win32Error("CreateThreadpoolWork 失败，回退到单线程");

		// 回退到单线程
		for (uint32_t i = 0; i < times; ++i) {
			func(i);
		}
	}
#endif // _DEBUG
}

bool Win32Helper::SetForegroundWindow(HWND hWnd) noexcept {
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

static bool MapKeycodeToUnicode(
	const int vCode,
	HKL layout,
	const BYTE* keyState,
	std::array<wchar_t, 3>& outBuffer
) noexcept {
	// Get the scan code from the virtual key code
	const UINT scanCode = MapVirtualKeyEx(vCode, MAPVK_VK_TO_VSC, layout);
	// Get the unicode representation from the virtual key code and scan code pair
	const int result = ToUnicodeEx(vCode, scanCode, keyState, outBuffer.data(), (int)outBuffer.size(), 0, layout);
	return result != 0;
}

static const std::array<std::wstring, 256>& GetKeyNames() noexcept {
	// 取自 https://github.com/microsoft/PowerToys/blob/fa3a5f80a113568155d9c2dbbcea8af16e15afa1/src/common/interop/keyboard_layout.cpp#L63
	static HKL previousLayout = 0;
	static std::array<std::wstring, 256> keyboardLayoutMap;

	// Get keyboard layout for current thread
	const HKL layout = GetKeyboardLayout(0);
	if (layout == previousLayout && !keyboardLayoutMap[0].empty()) {
		return keyboardLayoutMap;
	}
	previousLayout = layout;

	// 0 为非法
	keyboardLayoutMap[0] = L"Undefined";

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

const std::wstring& Win32Helper::GetKeyName(uint8_t key) noexcept {
	return GetKeyNames()[key];
}

bool Win32Helper::IsProcessElevated() noexcept {
	static bool result = []() {
		// https://web.archive.org/web/20100418044859/http://msdn.microsoft.com/en-us/windows/ff420334.aspx
		BYTE adminSID[SECURITY_MAX_SID_SIZE]{};
		DWORD dwLength = sizeof(adminSID);
		if (!CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, &adminSID, &dwLength)) {
			Logger::Get().Win32Error("CreateWellKnownSid 失败");
			return false;
		}

		BOOL isAdmin;
		if (!CheckTokenMembership(NULL, adminSID, &isAdmin)) {
			Logger::Get().Win32Error("CheckTokenMembership 失败");
			return false;
		}

		return (bool)isAdmin;
	}();

	return result;
}

// 获取进程的完整性级别
// https://devblogs.microsoft.com/oldnewthing/20221017-00/?p=107291
bool Win32Helper::GetProcessIntegrityLevel(HANDLE hQueryToken, DWORD& integrityLevel) noexcept {
	wil::unique_tokeninfo_ptr<TOKEN_MANDATORY_LABEL> info;
	HRESULT hr = wil::get_token_information_nothrow(info, hQueryToken);
	if (FAILED(hr)) {
		Logger::Get().ComError("get_token_information_nothrow 失败", hr);
		return false;
	}

	PSID sid = info->Label.Sid;
	integrityLevel = *GetSidSubAuthority(sid, *GetSidSubAuthorityCount(sid) - 1);
	return true;
}

static winrt::com_ptr<IShellView> FindDesktopFolderView() noexcept {
	winrt::com_ptr<IShellWindows> shellWindows =
		winrt::try_create_instance<IShellWindows>(CLSID_ShellWindows, CLSCTX_LOCAL_SERVER);
	if (!shellWindows) {
		Logger::Get().Error("创建 ShellWindows 失败");
		return nullptr;
	}

	winrt::com_ptr<IDispatch> dispatch;
	Win32Helper::Variant vtLoc(CSIDL_DESKTOP);
	Win32Helper::Variant vtEmpty;
	long hWnd;
	HRESULT hr = shellWindows->FindWindowSW(
		&vtLoc, &vtEmpty, SWC_DESKTOP, &hWnd, SWFO_NEEDDISPATCH, dispatch.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IShellWindows::FindWindowSW 失败", hr);
		return nullptr;
	}

	winrt::com_ptr<IShellBrowser> shellBrowser;
	hr = dispatch.as<IServiceProvider>()->QueryService(
		SID_STopLevelBrowser, IID_PPV_ARGS(&shellBrowser));
	if (FAILED(hr)) {
		Logger::Get().ComError("IServiceProvider::QueryService 失败", hr);
		return nullptr;
	}

	winrt::com_ptr<IShellView> shellView;
	hr = shellBrowser->QueryActiveShellView(shellView.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IShellBrowser::QueryActiveShellView 失败", hr);
		return nullptr;
	}

	return shellView;
}

static winrt::com_ptr<IShellFolderViewDual> GetDesktopAutomationObject() noexcept {
	winrt::com_ptr<IShellView> shellView = FindDesktopFolderView();
	if (!shellView) {
		Logger::Get().Error("FindDesktopFolderView 失败");
		return nullptr;
	}

	winrt::com_ptr<IDispatch> dispatch;
	HRESULT hr = shellView->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&dispatch));
	if (FAILED(hr)) {
		Logger::Get().ComError("IShellView::GetItemObject 失败", hr);
		return nullptr;
	}

	return dispatch.try_as<IShellFolderViewDual>();
}

static std::wstring_view ExtractDirectory(std::wstring_view path) noexcept {
	size_t delimPos = path.find_last_of(L'\\');
	return delimPos == std::wstring_view::npos ? path : path.substr(0, delimPos + 1);
}

// 在提升的进程中启动未提升的进程
// 原理见 https://devblogs.microsoft.com/oldnewthing/20131118-00/?p=2643
static bool OpenNonElevated(const wchar_t* path, const wchar_t* parameters) noexcept {
	static winrt::com_ptr<IShellDispatch2> shellDispatch = ([]() -> winrt::com_ptr<IShellDispatch2> {
		winrt::com_ptr<IShellFolderViewDual> folderView = GetDesktopAutomationObject();
		if (!folderView) {
			Logger::Get().Error("GetDesktopAutomationObject 失败");
			return nullptr;
		}

		winrt::com_ptr<IDispatch> dispatch;
		HRESULT hr = folderView->get_Application(dispatch.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("IShellFolderViewDual::get_Application 失败", hr);
			return nullptr;
		}

		return dispatch.try_as<IShellDispatch2>();
	})();

	if (!shellDispatch) {
		return false;
	}

#pragma push_macro("ShellExecute")
#undef ShellExecute
	HRESULT hr = shellDispatch->ShellExecute(
#pragma pop_macro("ShellExecute")
		wil::make_bstr_nothrow(path).get(),
		Win32Helper::Variant(parameters ? parameters : L""),
		Win32Helper::Variant(ExtractDirectory(path)),
		Win32Helper::Variant(L"open"),
		Win32Helper::Variant(SW_SHOWNORMAL)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("IShellDispatch2::ShellExecute 失败", hr);
		return false;
	}

	return true;
}

bool Win32Helper::ShellOpen(const wchar_t* path, const wchar_t* parameters, bool nonElevated) noexcept {
	if (nonElevated && IsProcessElevated()) {
		if (OpenNonElevated(path, parameters)) {
			return true;
		}
		// OpenNonElevated 失败则回落到 ShellExecuteEx
	}

	// 指定工作目录为程序所在目录，否则某些程序不能正常运行
	std::wstring workingDir(ExtractDirectory(path));

	SHELLEXECUTEINFO execInfo{
		.cbSize = sizeof(execInfo),
		.fMask = SEE_MASK_ASYNCOK,
		.lpVerb = L"open",
		.lpFile = path,
		.lpParameters = parameters,
		.lpDirectory = workingDir.c_str(),
		.nShow = SW_SHOWNORMAL
	};

	if (!ShellExecuteEx(&execInfo)) {
		Logger::Get().Win32Error("ShellExecuteEx 失败");
		return false;
	}

	return true;
}

bool Win32Helper::OpenFolderAndSelectFile(const wchar_t* fileName) noexcept {
	// 根据 https://docs.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shparsedisplayname，
	// SHParseDisplayName 不能在主线程调用
	wil::unique_any<PIDLIST_ABSOLUTE, decltype(&CoTaskMemFree), CoTaskMemFree> pidl;
	HRESULT hr = SHParseDisplayName(fileName, nullptr, pidl.put(), 0, nullptr);
	if (FAILED(hr)) {
		Logger::Get().ComError("SHParseDisplayName 失败", hr);
		return false;
	}

	hr = SHOpenFolderAndSelectItems(pidl.get(), 0, nullptr, 0);
	if (FAILED(hr)) {
		Logger::Get().ComError("SHOpenFolderAndSelectItems 失败", hr);
		return false;
	}

	return true;
}

const std::wstring& Win32Helper::GetExePath() noexcept {
	// 会在日志初始化前调用
	static std::wstring result = []() -> std::wstring {
		std::wstring exePath;
		FAIL_FAST_IF_FAILED(wil::GetModuleFileNameW(NULL, exePath));

		if (!wil::is_extended_length_path(exePath.c_str())) {
			return exePath;
		}

		// 去除 \\?\ 前缀
		wil::unique_hlocal_string canonicalPath;
		FAIL_FAST_IF_FAILED(PathAllocCanonicalize(
			exePath.c_str(),
			PATHCCH_ALLOW_LONG_PATHS,
			canonicalPath.put()
		));

		return canonicalPath.get();
	}();

	return result;
}

}
