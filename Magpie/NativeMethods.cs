using System;
using System.Runtime.InteropServices;
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

		public static Version GetOSVersion() {
			OsVersionInfo osVersionInfo = new OsVersionInfo();
			osVersionInfo.dwOSVersionInfoSize = (uint)Marshal.SizeOf(osVersionInfo);
			_ = RtlGetVersion(ref osVersionInfo);
			return new Version(
				(int)osVersionInfo.dwMajorVersion,
				(int)osVersionInfo.dwMinorVersion,
				(int)osVersionInfo.dwBuildNumber
				);
		}

		private static readonly int LOCALE_NAME_MAX_LENGTH = 85;

		[DllImport("kernel32.dll", SetLastError = true)]
		private static extern int GetUserDefaultLocaleName([MarshalAs(UnmanagedType.LPWStr)] StringBuilder lpLocaleName, int cchLocaleName);

		public static string GetUserDefaultLocalName() {
			StringBuilder sb = new StringBuilder(LOCALE_NAME_MAX_LENGTH);
			_ = GetUserDefaultLocaleName(sb, LOCALE_NAME_MAX_LENGTH);
			return sb.ToString();
		}

		/*
         * Runtime.dll
         */

		public delegate void ReportStatus(int status, IntPtr errorMsg);

		[DllImport("Runtime", CallingConvention = CallingConvention.StdCall)]
		public static extern void Run(
			ReportStatus reportStatus,
			IntPtr hwndSrc,
			[MarshalAs(UnmanagedType.U1)] bool adjustCursorSpeed
		);
	}
}
