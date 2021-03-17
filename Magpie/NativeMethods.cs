using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;

namespace Magpie {
	// Win32 API
	public static class NativeMethods {
		// 获取用户当前正在使用的窗体的句柄
		[DllImport("user32", CharSet = CharSet.Unicode)]
		public static extern IntPtr GetForegroundWindow();

        [DllImport("user32", CharSet = CharSet.Unicode)]
        public static extern bool SetForegroundWindow(IntPtr hWnd);

        public static IntPtr HWND_BROADCAST = (IntPtr)0xffff;

        [DllImport("user32", CharSet = CharSet.Unicode)]
        public static extern bool PostMessage(IntPtr hwnd, int msg, IntPtr wparam, IntPtr lparam);

        [DllImport("user32", CharSet = CharSet.Unicode)]
        public static extern int RegisterWindowMessage(string message);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern IntPtr SetCursor(IntPtr hCursor);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetWindowThreadProcessId(IntPtr hWnd, ref int lpdwProcessId);

        public static int GetWindowProcessId(IntPtr hWnd) {
            int processId = 0;
            GetWindowThreadProcessId(hWnd, ref processId);
            return processId;
        }

        [DllImport("Runtime", CallingConvention = CallingConvention.StdCall)]
        public static extern bool CreateMagWindow(
            [MarshalAs(UnmanagedType.LPWStr)] string effectsJson,
            int captureMode,
            bool showFPS,
            bool noVSync,
            bool noDisturb = false
        );

        [DllImport("Runtime", CallingConvention = CallingConvention.StdCall)]
        public static extern void DestroyMagWindow();

        [DllImport("Runtime", CallingConvention = CallingConvention.StdCall)]
        public static extern bool HasMagWindow();

        // 由于无法理解的原因，这里不能直接封送为 string
        // 见 https://stackoverflow.com/questions/15793736/difference-between-marshalasunmanagedtype-lpwstr-and-marshal-ptrtostringuni
        [DllImport("Runtime", CallingConvention = CallingConvention.StdCall, EntryPoint = "GetLastErrorMsg")]
        private static extern IntPtr GetLastErrorMsgNative();

        public static string GetLastErrorMsg() {
            return Marshal.PtrToStringUni(GetLastErrorMsgNative());
        }

        [DllImport("Runtime", CallingConvention = CallingConvention.StdCall)]
        public static extern IntPtr GetSrcWnd();

        [DllImport("Runtime", CallingConvention = CallingConvention.StdCall)]
        public static extern IntPtr GetHostWnd();
    }
}
