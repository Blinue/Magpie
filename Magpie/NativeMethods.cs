using System;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;

namespace Magpie {
	// Win32 API
	internal static class NativeMethods {
		public static readonly int MAGPIE_WM_SHOWME = RegisterWindowMessage("WM_SHOWME");
		public static readonly int MAGPIE_WM_DESTORYHOST = RegisterWindowMessage("MAGPIE_WM_DESTORYHOST");
		public static readonly int SW_NORMAL = 1;

		[DllImport("user32", CharSet = CharSet.Unicode)]
		public static extern IntPtr GetForegroundWindow();

		[DllImport("user32", CharSet = CharSet.Unicode)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static extern bool IsWindow(IntPtr hWnd);

		[DllImport("user32", CharSet = CharSet.Unicode)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static extern bool IsWindowVisible(IntPtr hWnd);

		[DllImport("user32", CharSet = CharSet.Unicode)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static extern bool SetForegroundWindow(IntPtr hWnd);

		[DllImport("user32", CharSet = CharSet.Unicode)]
		[return: MarshalAs(UnmanagedType.Bool)]
		private static extern bool PostMessage(IntPtr hwnd, int msg, IntPtr wparam, IntPtr lparam);

		private static readonly IntPtr HWND_BROADCAST = (IntPtr)0xffff;
		public static bool BroadcastMessage(int msg) {
			return PostMessage(HWND_BROADCAST, msg, IntPtr.Zero, IntPtr.Zero);
		}

		[DllImport("user32", CharSet = CharSet.Unicode)]
		public static extern int RegisterWindowMessage(string message);

		[DllImport("user32.dll", CharSet = CharSet.Unicode)]
		private static extern int GetWindowThreadProcessId(IntPtr hWnd, ref int lpdwProcessId);

		public static int GetWindowProcessId(IntPtr hWnd) {
			int processId = 0;

			return GetWindowThreadProcessId(hWnd, ref processId) == 0 ? 0 : processId;
		}

		[StructLayout(LayoutKind.Sequential)]
		private struct POINT {
			public int x;
			public int y;
		}
		[StructLayout(LayoutKind.Sequential)]
		private struct RECT {
			public int left;
			public int top;
			public int right;
			public int bottom;
		}
		[StructLayout(LayoutKind.Sequential)]
		private struct WINDOWPLACEMENT {
			public uint length;
			public uint flags;
			public uint showCmd;
			public POINT ptMinPosition;
			public POINT ptMaxPosition;
			public RECT rcNormalPosition;
		}

		[DllImport("user32.dll", CharSet = CharSet.Unicode)]
		[return: MarshalAs(UnmanagedType.Bool)]
		private static extern bool GetWindowPlacement(IntPtr hWnd, ref WINDOWPLACEMENT lpwndpl);

		public static int GetWindowShowCmd(IntPtr hWnd) {
			WINDOWPLACEMENT wp = new WINDOWPLACEMENT();
			wp.length = (uint)Marshal.SizeOf(wp);
			return !GetWindowPlacement(hWnd, ref wp) ? -1 : (int)wp.showCmd;
		}

		[DllImport("user32.dll", CharSet = CharSet.Unicode)]
		private static extern int GetWindowTextLength(IntPtr hWnd);

		[DllImport("user32.dll", CharSet = CharSet.Unicode)]
		private static extern int GetWindowText(
			IntPtr hWnd,
			[MarshalAs(UnmanagedType.LPWStr)] StringBuilder lpString,
			int nMaxCount
		);

		public static string GetWindowTitle(IntPtr hWnd) {
			int len = GetWindowTextLength(hWnd);
			if (len <= 0) {
				return "";
			}

			StringBuilder sb = new StringBuilder(len + 1);
			len = GetWindowText(hWnd, sb, sb.Capacity);
			return len > 0 ? sb.ToString() : "";
		}

		[StructLayout(LayoutKind.Sequential)]
		private struct OsVersionInfo {
			public uint dwOSVersionInfoSize;

			public uint dwMajorVersion;
			public uint dwMinorVersion;

			public uint dwBuildNumber;

			public uint dwPlatformId;

			[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
			public string szCSDVersion;
		}

		[DllImport("ntdll.dll", SetLastError = true)]
		private static extern uint RtlGetVersion(ref OsVersionInfo versionInformation);

		private static Version version = null;
		public static Version GetOSVersion() {
			if (version == null) {
				OsVersionInfo osVersionInfo = new OsVersionInfo();
				osVersionInfo.dwOSVersionInfoSize = (uint)Marshal.SizeOf(osVersionInfo);
				_ = RtlGetVersion(ref osVersionInfo);
				version = new Version(
					(int)osVersionInfo.dwMajorVersion,
					(int)osVersionInfo.dwMinorVersion,
					(int)osVersionInfo.dwBuildNumber
				);
			}

			return version;
		}

		private static readonly int LOCALE_NAME_MAX_LENGTH = 85;

		[DllImport("kernel32.dll", SetLastError = true)]
		private static extern int GetUserDefaultLocaleName([MarshalAs(UnmanagedType.LPWStr)] StringBuilder lpLocaleName, int cchLocaleName);

		public static string GetUserDefaultLocalName() {
			StringBuilder sb = new StringBuilder(LOCALE_NAME_MAX_LENGTH);
			_ = GetUserDefaultLocaleName(sb, LOCALE_NAME_MAX_LENGTH);
			return sb.ToString();
		}

		public static readonly IntPtr HKEY_CURRENT_USER = new IntPtr(0x80000001);
		public static readonly int KEY_READ = 131097;

		[DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
		public static extern int RegOpenKeyEx(IntPtr hKey, string lpSubKey, int ulOptions, int samDesired, ref IntPtr phkResult);

		[DllImport("advapi32.dll", SetLastError = true)]
		public static extern int RegCloseKey(IntPtr hKey);

		[DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
		public static extern int RegQueryValueEx(IntPtr hKey, string lpValueName, IntPtr lpReserved, IntPtr lpType, byte[] lpData, ref int lpcbData);

		/*
		 * Runtime.dll
		 */

		[DllImport("Runtime", CallingConvention = CallingConvention.StdCall)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static extern bool Initialize(uint logLevel);

		[DllImport("Runtime", CallingConvention = CallingConvention.StdCall)]
		public static extern void SetLogLevel(uint logLevel);

		[DllImport("Runtime", EntryPoint = "Run", CallingConvention = CallingConvention.StdCall)]
		private static extern IntPtr RunNative(
			IntPtr hwndSrc,
			[MarshalAs(UnmanagedType.LPUTF8Str)] string effectsJson,
			uint captureMode,
			int frameRate,
			float cursorZoomFactor,
			uint cursorInterpolationMode,
			uint adapterIdx,
			uint flags
		);

		[DllImport("kernel32.dll", CharSet = CharSet.Ansi, ExactSpelling = true)]
		private static extern int lstrlenA(IntPtr ptr);

		private static unsafe string PtrToUTF8String(IntPtr ptr) {
			if (ptr == IntPtr.Zero) {
				return null;
			}
			
			return Encoding.UTF8.GetString((byte*)ptr, lstrlenA(ptr));
		}

		public static string Run(
			IntPtr hwndSrc,
			string effectsJson,
			uint captureMode,
			int frameRate,
			float cursorZoomFactor,
			uint cursorInterpolationMode,
			uint adapterIdx,
			uint flags
		) {
			return PtrToUTF8String(RunNative(hwndSrc, effectsJson, captureMode,
				frameRate, cursorZoomFactor, cursorInterpolationMode, adapterIdx, flags));
		}

		[DllImport("Runtime", EntryPoint = "GetAllGraphicsAdapters", CallingConvention = CallingConvention.StdCall)]
		private static extern IntPtr GetAllGraphicsAdaptersNative();



		public static string[] GetAllGraphicsAdapters() {
			string result = PtrToUTF8String(GetAllGraphicsAdaptersNative());
			return result.Split(new string[] { @"/$@\" }, StringSplitOptions.RemoveEmptyEntries);
		}
	}
}
