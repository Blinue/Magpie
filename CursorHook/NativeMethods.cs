using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Text;

namespace Magpie.CursorHook {
	// Win32 API
	public static class NativeMethods {
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern IntPtr SetCursor(IntPtr hCursor);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern IntPtr GetCursor();

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern IntPtr LoadCursor(IntPtr hInstance, IntPtr lpCursorName);

        public readonly static IntPtr IDC_ARROW = new IntPtr(32512);
        public readonly static IntPtr IDC_HAND = new IntPtr(32649);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern IntPtr SetClassLong(IntPtr hWnd, int nIndex, IntPtr dwNewLong);

        public readonly static int GCLP_HCURSOR = -12;

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern int ShowCursor(bool bShow);


        public delegate bool EnumWindowsProc(IntPtr hwnd, int lParam);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, int lParam);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

        public readonly static uint WM_COPYDATA = 0x004A + 1;

        [StructLayout(LayoutKind.Sequential)]
        public struct POINT {
            public int x;
            public int y;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct CURSORINFO {
            public int cbSize;
            public int flags;
            public IntPtr hCursor;
            public POINT ptScreenPos;
        }

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern bool GetCursorInfo(ref CURSORINFO pci);
    }
}
