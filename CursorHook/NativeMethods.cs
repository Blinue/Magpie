using System;
using System.Runtime.InteropServices;


namespace Magpie.CursorHook {
	// Win32 API
	static class NativeMethods {
        [DllImport("user32.dll")]
        public static extern IntPtr SetCursor(IntPtr hCursor);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern IntPtr LoadCursor(IntPtr hInstance, IntPtr lpCursorName);

        public readonly static IntPtr IDC_ARROW = new IntPtr(32512);
        public readonly static IntPtr IDC_HAND = new IntPtr(32649);
        public readonly static IntPtr IDC_APPSTARTING = new IntPtr(32650);
        public readonly static IntPtr IDC_IBEAM = new IntPtr(32513);

        public const int GCLP_HCURSOR = -12;

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern long GetClassLongPtr(IntPtr hWnd, int nIndex);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetClassLong(IntPtr hWnd, int nIndex);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern long SetClassLongPtr(IntPtr hWnd, int nIndex, long dwNewLong);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        private static extern int SetClassLong(IntPtr hWnd, int nIndex, int dwNewLong);

        public static long GetClassAuto(IntPtr hWnd, int nIndex) {
            // 尽管文档表示 GetClassLongPtr 可以在 32 位程序里使用，但实际上会失败
            if (EasyHook.NativeAPI.Is64Bit) {
                return GetClassLongPtr(hWnd, nIndex);
            } else {
                return GetClassLong(hWnd, nIndex);
            }
        }

        public static long SetClassAuto(IntPtr hWnd, int nIndex, long dwNew) {
            if (EasyHook.NativeAPI.Is64Bit) {
                return SetClassLongPtr(hWnd, nIndex, dwNew);
            } else {
                return SetClassLong(hWnd, nIndex, (int)dwNew);
            }
        }

        public delegate bool EnumWindowsProc(IntPtr hwnd, int lParam);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

        public const uint WM_SETCURSOR = 0x0020;
        public const int HTCLIENT = 1;

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

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool GetCursorInfo(ref CURSORINFO pci);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool DestroyCursor(IntPtr hCursor);

        [StructLayout(LayoutKind.Sequential)]
        public struct ICONINFO {
            public int fIcon;
            public uint xHotspot;
            public uint yHotspot;
            public IntPtr hbmMask;
            public IntPtr hbmColor;
        }

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool GetIconInfo(IntPtr hIcon, out ICONINFO piconinfo);

        [DllImport("user32.dll")]
        public static extern IntPtr CopyIcon(IntPtr hIcon);

        [DllImport("gdi32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool DeleteObject(IntPtr ho);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern uint RegisterWindowMessage([MarshalAs(UnmanagedType.LPWStr)]string lpString);

        private const int SM_CXCURSOR = 13;
        private const int SM_CYCURSOR = 14;

        [DllImport("user32.dll")]
        private static extern int GetSystemMetrics(int nIndex);

        public static (int x, int y) GetCursorSize() {
            return (GetSystemMetrics(SM_CXCURSOR), GetSystemMetrics(SM_CYCURSOR));
        }

        [DllImport("user32.dll")]
        public static extern SafeCursorHandle CreateCursor(IntPtr hInst, int xHotSpot, int yHotSpot, int nWidth, int nHeight, byte[] pvANDPlane, byte[] pvXORPlane);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr GetModuleHandle(IntPtr lpModuleName);

        public static IntPtr GetModule() {
            return GetModuleHandle(IntPtr.Zero);
        }

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool IsWindow(IntPtr hWnd);


        [DllImport("user32.dll")]
        private static extern int GetWindowThreadProcessId(IntPtr hWnd, ref int lpdwProcessId);

        [DllImport("user32.dll")]
        private static extern int GetWindowThreadProcessId(IntPtr hWnd, IntPtr lpdwProcessId);

        public static int GetWindowProcessId(IntPtr hWnd) {
            if(hWnd == IntPtr.Zero) {
                return 0;
            }

            int processId = 0;
            if(GetWindowThreadProcessId(hWnd, ref processId) == 0) {
                return 0;
            }

            return processId;
        }

        public static int GetWindowThreadId(IntPtr hWnd) {
            if (hWnd == IntPtr.Zero) {
                return 0;
            }

            return GetWindowThreadProcessId(hWnd, IntPtr.Zero);
        }

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        public static extern IntPtr FindWindow(
            [MarshalAs(UnmanagedType.LPWStr)] string lpClassName,
            IntPtr lpWindowName
        );

        [DllImport("user32.dll")]
        public static extern IntPtr GetForegroundWindow();
    }
}
