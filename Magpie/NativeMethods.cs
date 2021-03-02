using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;

namespace Magpie {
	// Win32 API
	public static class NativeMethods {
		// 获取用户当前正在使用的窗体的句柄
		[DllImport("user32.dll")]
		public static extern IntPtr GetForegroundWindow();

        [DllImport("user32.dll")]
        public static extern bool SetForegroundWindow(IntPtr hWnd);

        public static IntPtr HWND_BROADCAST = (IntPtr)0xffff;
        public static readonly int WM_SHOWME = RegisterWindowMessage("WM_SHOWME");

        [DllImport("user32")]
        public static extern bool PostMessage(IntPtr hwnd, int msg, IntPtr wparam, IntPtr lparam);

        [DllImport("user32")]
        public static extern int RegisterWindowMessage(string message);


        [DllImport("Runtime.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern bool CreateMagWindow(
            uint frameRate,
            [MarshalAs(UnmanagedType.LPWStr)] string effectsJson,
            bool noDisturb = false
        );

        [DllImport("Runtime.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void DestroyMagWindow();

        [DllImport("Runtime.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern bool HasMagWindow();

        // 由于无法理解的原因，这里不能直接封送为 string
        // 见 https://stackoverflow.com/questions/15793736/difference-between-marshalasunmanagedtype-lpwstr-and-marshal-ptrtostringuni
        [DllImport("Runtime.dll", CallingConvention = CallingConvention.StdCall, EntryPoint = "GetLastErrorMsg")]
        private static extern IntPtr GetLastErrorMsgNative();

        public static string GetLastErrorMsg() {
            return Marshal.PtrToStringUni(GetLastErrorMsgNative());
        }
    }
}
