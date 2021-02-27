using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;

namespace Magpie {
	// Win32 API
	public static class Win32 {
		[Flags]
		public enum SetWindowPosFlags : uint {
			SWP_FRAMECHANGED = 0x0020,
			SWP_NOREPOSITION = 0x0200,
			SWP_NOSIZE = 0x0001
		}

		[Flags]
		private enum WLP_NINDEX_ENUM {
			GWL_STYLE = -16,
			GWL_EXSTYLE = -20
		}

		[Flags]
		public enum WS_ENUM {
			WS_EX_LAYERED = 0x00080000,
			WS_CHILD = 0x40000000,
			WS_VISIBLE = 0x10000000,
			MS_SHOWMAGNIFIEDCURSOR = 0x0001
		}

		public enum SWHE_HOOK_TYPE {
			WH_KEYBOARD = 2,
			WH_KEYBOARD_LL = 13
		}

		private static class Native {
			public static class User {
				// 获取用户当前正在使用的窗体的句柄
				[DllImport("user32.dll")]
				public static extern IntPtr GetForegroundWindow();

				// 获取窗体属性
				[DllImport("user32.dll")]
				public static extern IntPtr GetWindowLongPtr(IntPtr hWnd, WLP_NINDEX_ENUM nIndex);

				// 更改子窗口，弹出窗口或顶级窗口的尺寸，位置和Z轴顺序
				[DllImport("user32.dll")]
				public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);
			}

			public static class Kernel {
				// 获取调用线程的最后一个错误代码值
				[DllImport("kernel32.dll")]
				public static extern uint GetLastError();
			}
		}

		public static IntPtr GetForegroundWindow() {
			return Native.User.GetForegroundWindow();
		}

		public static uint GetLastError() {
			return Native.Kernel.GetLastError();
		}

		public static IntPtr GetWindowStyle(IntPtr hwnd) {
			return Native.User.GetWindowLongPtr(hwnd, WLP_NINDEX_ENUM.GWL_STYLE);
		}

		public static IntPtr GetExtendedWindowStyle(IntPtr hwnd) {
			return Native.User.GetWindowLongPtr(hwnd, WLP_NINDEX_ENUM.GWL_EXSTYLE);
		}

	}
}
