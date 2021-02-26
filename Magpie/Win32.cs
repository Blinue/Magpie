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

		// 窗口消息类型
		[Flags]
		public enum WM_ENUM : uint {
			WM_KEYDOWN = 0x100,
			WM_KEYUP = 0x101,
			WM_SYSKEYDOWN = 0x104,
			WM_SYSKEYUP = 0x105
		}

		[Flags]
		private enum GCLP_NINDEX_ENUM {
			GCLP_HICON = -14,
			GCLP_HICONSM = -34
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

		[Flags]
		public enum KBDLLHOOKSTRUCTFlags : uint {
			LLKHF_EXTENDED = 0x01,
			LLKHF_INJECTED = 0x10,
			LLKHF_ALTDOWN = 0x20,
			LLKHF_UP = 0x80
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct KBDLLHOOKSTRUCT {
			public uint vkCode;
			public uint scanCode;
			public KBDLLHOOKSTRUCTFlags flags;
			public uint time;
			public UIntPtr dwExtraInfo;
		}


		public enum SWHE_HOOK_TYPE {
			WH_KEYBOARD = 2,
			WH_KEYBOARD_LL = 13
		}

		private delegate int HookProc(int code, uint wParam, ref KBDLLHOOKSTRUCT lparam);

		private static class Native {
			public static class User {
				// 获取用户当前正在使用的窗体的句柄
				[DllImport("user32.dll")]
				public static extern IntPtr GetForegroundWindow();

				// 从与指定窗体关联的 WNDCLASSEX 结构中检索指定字段
				[DllImport("user32.dll")]
				public static extern IntPtr GetClassLongPtr(IntPtr hWnd, GCLP_NINDEX_ENUM nIndex);

				// 获取窗体属性
				[DllImport("user32.dll")]
				public static extern IntPtr GetWindowLongPtr(IntPtr hWnd, WLP_NINDEX_ENUM nIndex);

				// 更改子窗口，弹出窗口或顶级窗口的尺寸，位置和Z轴顺序
				[DllImport("user32.dll")]
				public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

				[DllImport("user32.dll")]
				public static extern IntPtr SetWindowsHookEx(SWHE_HOOK_TYPE hookType, HookProc lpfn, IntPtr hMod, uint dwThreadId);

				[DllImport("user32.dll")]
				public static extern bool UnhookWindowsHookEx(IntPtr hhk);

				[DllImport("user32.dll")]
				public static extern int CallNextHookEx(IntPtr hhk, int nCode, uint wParam, ref KBDLLHOOKSTRUCT lParam);
			}

			public static class Kernel {
				// 获取调用线程的最后一个错误代码值
				[DllImport("kernel32.dll")]
				public static extern uint GetLastError();
			}
		}

		public delegate void KeyboardHookCallback(WM_ENUM wparam, ref KBDLLHOOKSTRUCT lparam);

		public static Action SetGlobalKeyboardHook(KeyboardHookCallback lpfn) {
			IntPtr hook = IntPtr.Zero;
			// 用钩子获取全局键盘消息
			// 关于 WH_KEYBOARD_LL 和 WH_KEYBOARD 的区别: https://www.jianshu.com/p/513d4893248d
			hook = Native.User.SetWindowsHookEx(SWHE_HOOK_TYPE.WH_KEYBOARD_LL, (int code, uint wparam, ref KBDLLHOOKSTRUCT lparam) => {
				if (code >= 0) {
					lpfn((WM_ENUM)wparam, ref lparam);
				}

				// 调用下一个 hook
				return Native.User.CallNextHookEx(hook, code, wparam, ref lparam);
			}, IntPtr.Zero, 0); // WH_KEYBOARD_LL 无需使用 DLL

			return () => Native.User.UnhookWindowsHookEx(hook);
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
