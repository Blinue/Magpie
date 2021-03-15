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
        public static extern IntPtr LoadCursor(IntPtr hInstance, IntPtr lpCursorName);

        public readonly static IntPtr IDC_ARROW = new IntPtr(32512);
        public readonly static IntPtr IDC_HAND = new IntPtr(32649);
        public readonly static IntPtr IDC_APPSTARTING = new IntPtr(32650);

        public readonly static int GCLP_HCURSOR = -12;

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern int GetClassLong(IntPtr hWnd, int nIndex);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern IntPtr SetClassLong(IntPtr hWnd, int nIndex, IntPtr dwNewLong);


        public delegate bool EnumWindowsProc(IntPtr hwnd, int lParam);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, int lParam);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern bool SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

        public readonly static uint MAGPIE_WM_NEWCURSOR = RegisterWindowMessage("MAGPIE_WM_NEWCURSOR");

        public readonly static uint WM_SETCURSOR = 0x0020;
        public readonly static int HTCLIENT = 1;

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

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern bool DestroyCursor(IntPtr hCursor);

        [StructLayout(LayoutKind.Sequential)]
        public struct ICONINFO {
            public int fIcon;
            public uint xHotspot;
            public uint yHotspot;
            public IntPtr hbmMask;
            public IntPtr hbmColor;
        }

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern bool GetIconInfo(IntPtr hIcon, ref ICONINFO piconinfo);

        [DllImport("gdi32.dll", CharSet = CharSet.Unicode)]
        public static extern bool DeleteObject(IntPtr ho);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern uint RegisterWindowMessage([MarshalAs(UnmanagedType.LPWStr)]string lpString);

        private readonly static int SM_CXCURSOR = 13;
        private readonly static int SM_CYCURSOR = 14;

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetSystemMetrics(int nIndex);

        public static (int x, int y) GetCursorSize() {
            return (GetSystemMetrics(SM_CXCURSOR), GetSystemMetrics(SM_CYCURSOR));
        }

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern IntPtr CreateCursor(IntPtr hInst, int xHotSpot, int yHotSpot, int nWidth, int nHeight, byte[] pvANDPlane, byte[] pvXORPlane);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr GetModuleHandle(IntPtr lpModuleName);

        public static IntPtr GetModule() {
            return GetModuleHandle(IntPtr.Zero);
        }

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern bool IsWindow(IntPtr hWnd);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr GetParent(IntPtr hWnd);

        public static IntPtr GetTopWindow(IntPtr hWnd) {
            if(hWnd == IntPtr.Zero) {
                return IntPtr.Zero;
            }

            while(true) {
                IntPtr parent = GetParent(hWnd);
                if (parent == IntPtr.Zero) {
                    return hWnd;
                }

                hWnd = parent;
            }
        }

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetWindowThreadProcessId(IntPtr hWnd,ref int lpdwProcessId);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetWindowThreadProcessId(IntPtr hWnd, IntPtr lpdwProcessId);

        public static int GetWindowProcessId(IntPtr hWnd) {
            if(hWnd == IntPtr.Zero) {
                return 0;
            }

            int processId = 0;
            GetWindowThreadProcessId(hWnd, ref processId);
            return processId;
        }

        public static int GetWindowThreadId(IntPtr hWnd) {
            if (hWnd == IntPtr.Zero) {
                return 0;
            }

            return GetWindowThreadProcessId(hWnd, IntPtr.Zero);
        }
    }
}
