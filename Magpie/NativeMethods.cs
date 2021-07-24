using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;


namespace Magpie {
	// Win32 API
	static class NativeMethods {
        public static readonly int MAGPIE_WM_SHOWME = RegisterWindowMessage("WM_SHOWME");
        public static readonly int MAGPIE_WM_DESTORYMAG = RegisterWindowMessage("MAGPIE_WM_DESTORYMAG");
        public static readonly int SW_NORMAL = 1;

        // 获取用户当前正在使用的窗体的句柄
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

            if(GetWindowThreadProcessId(hWnd, ref processId) == 0) {
                return 0;
            }

            return processId;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct POINT {
            public int x;
            public int y;
        }
        [StructLayout(LayoutKind.Sequential)]
        struct RECT {
            public int left;
            public int top;
            public int right;
            public int bottom;
        }
        [StructLayout(LayoutKind.Sequential)]
        struct WINDOWPLACEMENT {
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
            if(!GetWindowPlacement(hWnd, ref wp)) {
                return -1;
            }
            return (int)wp.showCmd;
        }

        /*
         * Runtime.dll
         */

        public delegate void ReportStatus(int status, IntPtr errorMsg);

        [DllImport("Runtime", CallingConvention = CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern void RunMagWindow(
            ReportStatus reportStatus,
            IntPtr hwndSrc,
            [MarshalAs(UnmanagedType.LPUTF8Str)] string scaleModel,
            int captureMode,
            [MarshalAs(UnmanagedType.U1)] bool showFPS,
            [MarshalAs(UnmanagedType.U1)] bool noDisturb
        );

    }
}
