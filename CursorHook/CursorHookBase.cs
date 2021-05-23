using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Magpie.CursorHook {
    // 运行时钩子和启动时钩子通用的部分
    // 继承此类只需实现 Run
    abstract class CursorHookBase : IDisposable {
        private readonly IpcServer ipcServer;

        protected IntPtr hwndHost = IntPtr.Zero;
        protected IntPtr hwndSrc = IntPtr.Zero;


        private readonly (int x, int y) cursorSize = NativeMethods.GetCursorSize();

        // 保存已替换 HCURSOR 的窗口，以在卸载钩子时还原
        private readonly HashSet<IntPtr> replacedHwnds = new HashSet<IntPtr>();

        private static readonly IntPtr arrowCursor =
            NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_ARROW);
        private static readonly IntPtr handCursor =
            NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_HAND);
        private static readonly IntPtr appStartingCursor =
            NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_APPSTARTING);
        private static readonly IntPtr iBeamCursor =
            NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_IBEAM);

        // 原光标到透明光标的映射
        // 不替换透明的系统光标
        private readonly Dictionary<IntPtr, SafeCursorHandle> hCursorToTptCursor =
            new Dictionary<IntPtr, SafeCursorHandle>() {
                {arrowCursor, new SafeCursorHandle(arrowCursor, false)},
                {handCursor, new SafeCursorHandle(handCursor, false)},
                {appStartingCursor, new SafeCursorHandle(appStartingCursor, false)}/*,
                {iBeamCursor, new SafeCursorHandle(iBeamCursor, false)}*/
            };

        protected const string HOST_WINDOW_CLASS_NAME = "Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";

        private readonly static uint MAGPIE_WM_NEWCURSOR = NativeMethods.RegisterWindowMessage(
            EasyHook.NativeAPI.Is64Bit ? "MAGPIE_WM_NEWCURSOR64" : "MAGPIE_WM_NEWCURSOR32");

        public CursorHookBase(IpcServer server) {
            ipcServer = server;
        }

        public abstract void Run();

        // 用于创建 SetCursor 委托
        [UnmanagedFunctionPointer(CallingConvention.StdCall, SetLastError = true)]
        protected delegate IntPtr SetCursorDelegate(IntPtr hCursor);

        // 取代 SetCursor 的钩子
        protected IntPtr SetCursorHook(IntPtr hCursor) {
            // ReportToServer("SetCursor");
            IntPtr hCursorTar = hCursor;

            if (hwndHost == IntPtr.Zero || hCursor == IntPtr.Zero || !NativeMethods.IsWindow(hwndHost)) {
                // 不存在全屏窗口时钩子不做任何操作
            } else if (hCursorToTptCursor.ContainsKey(hCursor)) {
                hCursorTar = hCursorToTptCursor[hCursor].DangerousGetHandle();
            } else {
                // 未出现过的 hCursor
                SafeCursorHandle hTptCursor = CreateTransparentCursor(hCursor);
                if (hTptCursor == SafeCursorHandle.Zero) {
                    return NativeMethods.SetCursor(hCursor);
                }

                hCursorToTptCursor[hCursor] = hTptCursor;

                // 向全屏窗口发送光标句柄
                ReportCursorMap(hTptCursor, hCursor);

                hCursorTar = hTptCursor.DangerousGetHandle();
            }

            return NativeMethods.SetCursor(hCursorTar);
        }


        private SafeCursorHandle CreateTransparentCursor(IntPtr hotSpot) {
            int len = cursorSize.x * cursorSize.y;

            // 全 0xff
            byte[] andPlane = new byte[len];
            for (int i = 0; i < len; ++i) {
                andPlane[i] = 0xff;
            }

            // 全 0
            byte[] xorPlane = new byte[len];

            var (xHotSpot, yHotSpot) = GetCursorHotSpot(hotSpot);

            SafeCursorHandle rt = NativeMethods.CreateCursor(
                NativeMethods.GetModule(),
                xHotSpot, yHotSpot,
                cursorSize.x, cursorSize.y,
                andPlane, xorPlane
            );

            ReportIfFalse(rt != SafeCursorHandle.Zero, "创建透明光标失败");
            return rt;
        }

        private (int, int) GetCursorHotSpot(IntPtr hCursor) {
            if (hCursor == IntPtr.Zero) {
                return (0, 0);
            }

            if (!NativeMethods.GetIconInfo(hCursor, out NativeMethods.ICONINFO ii)) {
                return (0, 0);
            }

            _ = NativeMethods.DeleteObject(ii.hbmMask);
            _ = NativeMethods.DeleteObject(ii.hbmColor);

            return (
                Math.Min((int)ii.xHotspot, cursorSize.x),
                Math.Min((int)ii.yHotspot, cursorSize.y)
            );
        }

        // 向全屏窗口发送光标句柄
        private void ReportCursorMap(SafeCursorHandle hTptCursor, IntPtr hCursor) {
            ReportIfFalse(
                NativeMethods.PostMessage(hwndHost, MAGPIE_WM_NEWCURSOR, hTptCursor.DangerousGetHandle(), hCursor),
                "PostMessage 失败"
            );
        }

        // 向全屏窗口汇报至今为止的映射
        protected void ReportCursorMap() {
            foreach (var item in hCursorToTptCursor) {
                // 排除系统光标
                if (item.Key == arrowCursor
                    || item.Key == handCursor
                    || item.Key == appStartingCursor
                ) {
                    continue;
                }

                ReportCursorMap(item.Value, item.Key);
            }
        }

        protected void ReportToServer(string msg) {
            lock (ipcServer) {
                ipcServer.AddMessage(msg);
            }
        }

        protected void ReportIfFalse(bool flag, string msg) {
            if (!flag) {
                ReportToServer("出错: " + msg);
            }
        }

        protected void SendMessages() {
            lock (ipcServer) {
                ipcServer.Send();
            }
        }

        protected void ReplaceHCursors() {
            // 将窗口类中的 HCURSOR 替换为透明光标
            void ReplaceHCursor(IntPtr hWnd) {
                if (replacedHwnds.Contains(hWnd)) {
                    return;
                }

                IntPtr hCursor = new IntPtr(NativeMethods.GetClassAuto(hWnd, NativeMethods.GCLP_HCURSOR));
                if (hCursor == IntPtr.Zero
                    || hCursor == arrowCursor
                    || hCursor == handCursor
                    || hCursor == appStartingCursor
                ) {
                    // 不替换透明的系统光标
                    return;
                }

                if (hCursorToTptCursor.ContainsKey(hCursor)) {
                    // 之前已替换过
                    SafeCursorHandle hTptCursor = hCursorToTptCursor[hCursor];

                    // 替换窗口类的 HCURSOR
                    if (NativeMethods.SetClassAuto(hWnd, NativeMethods.GCLP_HCURSOR, hTptCursor.DangerousGetHandle().ToInt64()) == hCursor.ToInt64()) {
                        replacedHwnds.Add(hWnd);
                    } else {
                        ReportIfFalse(false, "SetClassLongAuto 失败");
                    }
                } else {
                    // 以下代码如果出错不会有任何更改

                    SafeCursorHandle hTptCursor = CreateTransparentCursor(hCursor);
                    if (hTptCursor == SafeCursorHandle.Zero) {
                        return;
                    }

                    // 替换窗口类的 HCURSOR
                    if (NativeMethods.SetClassAuto(hWnd, NativeMethods.GCLP_HCURSOR, hTptCursor.DangerousGetHandle().ToInt64()) == hCursor.ToInt64()) {
                        // 向全屏窗口发送光标句柄
                        if (NativeMethods.PostMessage(hwndHost, MAGPIE_WM_NEWCURSOR, hTptCursor.DangerousGetHandle(), hCursor)) {
                            // 替换成功
                            replacedHwnds.Add(hWnd);
                            hCursorToTptCursor[hCursor] = hTptCursor;
                        } else {
                            _ = NativeMethods.SetClassAuto(hWnd, NativeMethods.GCLP_HCURSOR, hCursor.ToInt64());
                            ReportIfFalse(false, "PostMessage 失败");
                        }
                    } else {
                        ReportIfFalse(false, "SetClassLongAuto 失败");
                    }
                }
            }

            // 替换源窗口和它的所有子窗口的窗口类的 HCRUSOR
            ReplaceHCursor(hwndSrc);
            NativeMethods.EnumChildWindows(hwndSrc, (IntPtr hWnd, int _) => {
                ReplaceHCursor(hWnd);
                return true;
            }, IntPtr.Zero);

            // 向源窗口发送 WM_SETCURSOR，一般可以使其调用 SetCursor
            ReportIfFalse(
                NativeMethods.PostMessage(
                    hwndSrc,
                    NativeMethods.WM_SETCURSOR,
                    hwndSrc,
                    (IntPtr)NativeMethods.HTCLIENT
                ),
                "PostMessage 失败"
            );
        }

        protected void ReplaceHCursorsBack() {
            foreach (var hwnd in replacedHwnds) {
                IntPtr hCursor = new IntPtr(NativeMethods.GetClassAuto(hwnd, NativeMethods.GCLP_HCURSOR));
                if (hCursor == IntPtr.Zero
                    || hCursor == arrowCursor
                    || hCursor == handCursor
                    || hCursor == appStartingCursor
                ) {
                    // 不替换透明的系统光标
                    return;
                }

                // 在 _hCursorToTptCursor 中反向查找
                var item = hCursorToTptCursor
                    .FirstOrDefault(pair => pair.Value.DangerousGetHandle() == hCursor);

                if (item.Key == IntPtr.Zero || item.Value == SafeCursorHandle.Zero) {
                    // 找不到就不替换
                    continue;
                }

                _ = NativeMethods.SetClassAuto(hwnd, NativeMethods.GCLP_HCURSOR, item.Key.ToInt64());
            }

            replacedHwnds.Clear();
        }

        public void Dispose() {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing) {
            // 清理 HCURSOR
            foreach (var cursorHandle in hCursorToTptCursor.Values) {
                if (cursorHandle != null && !cursorHandle.IsInvalid) {
                    cursorHandle.Dispose();
                }
            }
            // SafeHandle records the fact that we've called Dispose.
        }
    }
}
