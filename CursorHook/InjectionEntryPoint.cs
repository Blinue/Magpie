/*
 * 用于注入源窗口进程的钩子，支持运行时注入和启动时注入两种模式
 * 原理见 光标映射.md
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;


namespace Magpie.CursorHook {
    /// <summary>
    /// 注入时 EasyHook 会寻找 <see cref="EasyHook.IEntryPoint"/> 的实现。
    /// 注入后此类将成为入口
    /// </summary>
    public class InjectionEntryPoint : EasyHook.IEntryPoint, IDisposable {
#if DEBUG
        // 用于向 Magpie 里的 IPC server 发送消息
        private ServerInterface _server = null;

        private readonly Queue<string> _messageQueue = new Queue<string>();
#endif

        private IntPtr _hwndHost = IntPtr.Zero;
        private IntPtr _hwndSrc = IntPtr.Zero;


        private readonly (int x, int y) _cursorSize = NativeMethods.GetCursorSize();

        // 保存已替换 HCURSOR 的窗口，以在卸载钩子时还原
        private readonly HashSet<IntPtr> _replacedHwnds = new HashSet<IntPtr>();

        private static readonly IntPtr _arrowCursor =
            NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_ARROW);
        private static readonly IntPtr _handCursor =
            NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_HAND);
        private static readonly IntPtr _appStartingCursor =
            NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_APPSTARTING);

        // 原光标到透明光标的映射
        // 不替换透明的系统光标
        private readonly Dictionary<IntPtr, SafeCursorHandle> _hCursorToTptCursor =
            new Dictionary<IntPtr, SafeCursorHandle>() {
                {_arrowCursor, new SafeCursorHandle(_arrowCursor, false)},
                {_handCursor, new SafeCursorHandle(_handCursor, false)},
                {_appStartingCursor, new SafeCursorHandle(_appStartingCursor, false)}
            };

        private const string HOST_WINDOW_CLASS_NAME = "Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";

        private readonly static uint MAGPIE_WM_NEWCURSOR = NativeMethods.RegisterWindowMessage(
            EasyHook.NativeAPI.Is64Bit ? "MAGPIE_WM_NEWCURSOR64" : "MAGPIE_WM_NEWCURSOR32");

        // 运行时注入的入口
        public InjectionEntryPoint(
            EasyHook.RemoteHooking.IContext _1,
#if DEBUG
            string channelName,
#endif
            IntPtr hwndHost, IntPtr hwndSrc
        ) {
            _hwndHost = hwndHost;
            _hwndSrc = hwndSrc;
#if DEBUG
            ConnectToServer(channelName);
#endif
        }

        // 启动时注入的入口
        public InjectionEntryPoint(
            EasyHook.RemoteHooking.IContext _1
#if DEBUG
            , string channelName
#endif
        ) {
#if DEBUG
            ConnectToServer(channelName);
#endif
        }

#if DEBUG
        // DEBUG 时连接 IPC server
        private void ConnectToServer(string channelName) {

            _server = EasyHook.RemoteHooking.IpcConnectClient<ServerInterface>(channelName);

            // 测试连接性，如果失败会抛出异常静默的失败因此 Run 方法不会执行
            _server.Ping();

        }
#endif

        private void ReportMessageQueue() {
#if DEBUG
            string[] queued = null;

            lock (_messageQueue) {
                queued = _messageQueue.ToArray();
                _messageQueue.Clear();
            }

            if (queued != null && queued.Length > 0) {
                _server.ReportMessages(queued);
            } else {
                _server.Ping();
            }
#endif
        }

        // 运行时注入逻辑的入口
        public void Run(
            EasyHook.RemoteHooking.IContext _1,
#if DEBUG
            string _2,
#endif
            IntPtr _3, IntPtr _4
        ) {
            // 安装钩子
            EasyHook.LocalHook setCursorHook = null;
            try {
                // 截获 SetCursor
                setCursorHook = EasyHook.LocalHook.Create(
                    EasyHook.LocalHook.GetProcAddress("user32.dll", "SetCursor"),
                    new SetCursorDelegate(SetCursorHook),
                    this
                );

                // 只 Hook 窗口线程，因为 SetCursor 必须在窗口线程上调用
                setCursorHook.ThreadACL.SetInclusiveACL(
                    new int[] { NativeMethods.GetWindowThreadId(_hwndSrc) }
                );
            } catch (Exception e) {
                // 安装失败，直接退出
                ReportIfFalse(false, "安装钩子失败：" + e.Message);
                return;
            }

            ReportToServer("SetCursor钩子安装成功");

            ReplaceHCursors();

            try {
                while (true) {
                    if (!NativeMethods.IsWindow(_hwndHost)) {
                        // 运行时注入且全屏窗口已关闭，卸载钩子
                        break;
                    }

                    ReportMessageQueue();

                    Thread.Sleep(200);
                }
            } catch {
                // 如果服务器关闭 Ping() 和 ReportMessages() 将抛出异常
                // 执行到此处表示 Magpie 已关闭
            }

            // 退出前重置窗口类的光标
            ReplaceHCursorsBack();

            // 卸载钩子
            setCursorHook.Dispose();
            EasyHook.LocalHook.Release();
        }

        // 启动时注入逻辑的入口
        public void Run(
            EasyHook.RemoteHooking.IContext _1
#if DEBUG
            , string _2
#endif
        ) {
            // 安装钩子
            EasyHook.LocalHook setCursorHook = null;
            try {
                // 截获 SetCursor
                setCursorHook = EasyHook.LocalHook.Create(
                    EasyHook.LocalHook.GetProcAddress("user32.dll", "SetCursor"),
                    new SetCursorDelegate(SetCursorHook),
                    this
                );

                // Hook 除当前线程的所有线程，因为此时窗口线程未知
                setCursorHook.ThreadACL.SetExclusiveACL(new int[] { 0 });
            } catch (Exception e) {
                // 安装失败，直接退出
                ReportIfFalse(false, "安装钩子失败：" + e.Message);
                return;
            }

            ReportToServer("SetCursor钩子安装成功");


            // 启动时注入完成，唤醒注入进程
            EasyHook.RemoteHooking.WakeUpProcess();

            try {
                // 启动时注入只有在 Magpie 关闭后才卸载钩子
                while (true) {
                    // 查找全屏窗口
                    IntPtr hwndHost = NativeMethods.FindWindow(HOST_WINDOW_CLASS_NAME, IntPtr.Zero);
                    if (hwndHost != IntPtr.Zero) {
                        if (hwndHost != _hwndHost) {
                            ReportToServer("检测到全屏窗口");

                            _hwndHost = hwndHost;
                            // hwndSrc 为前台窗口
                            _hwndSrc = NativeMethods.GetForegroundWindow();

                            // 向全屏窗口汇报至今为止的映射
                            foreach (var item in _hCursorToTptCursor) {
                                // 排除系统光标
                                if (item.Key == _arrowCursor
                                    || item.Key == _handCursor
                                    || item.Key == _appStartingCursor
                                ) {
                                    continue;
                                }

                                ReportCursorMap(item.Value, item.Key);
                            }

                            ReplaceHCursors();
                        }
                    } else {
                        if (_hwndHost != IntPtr.Zero) {
                            ReportToServer("全屏窗口已关闭");
                            _hwndHost = IntPtr.Zero;

                            ReplaceHCursorsBack();
                        }
                    }

                    ReportMessageQueue();

                    Thread.Sleep(200);
                }
            } catch {
                // 如果服务器关闭 Ping() 和 ReportMessages() 将抛出异常
                // 执行到此处表示 Magpie 已关闭
            }

            if(_hwndHost != IntPtr.Zero) {
                ReplaceHCursorsBack();
            }

            // 卸载钩子
            setCursorHook.Dispose();
            EasyHook.LocalHook.Release();
        }

        private void ReplaceHCursorsBack() {
            foreach (var hwnd in _replacedHwnds) {
                IntPtr hCursor = new IntPtr(NativeMethods.GetClassAuto(hwnd, NativeMethods.GCLP_HCURSOR));
                if (hCursor == IntPtr.Zero
                    || hCursor == _arrowCursor
                    || hCursor == _handCursor
                    || hCursor == _appStartingCursor
                ) {
                    // 不替换透明的系统光标
                    return;
                }

                // 在 _hCursorToTptCursor 中反向查找
                var item = _hCursorToTptCursor
                    .FirstOrDefault(pair => pair.Value.DangerousGetHandle() == hCursor);

                if (item.Key == IntPtr.Zero || item.Value == SafeCursorHandle.Zero) {
                    // 找不到就不替换
                    continue;
                }

                _ = NativeMethods.SetClassAuto(hwnd, NativeMethods.GCLP_HCURSOR, item.Key.ToInt64());
            }

            _replacedHwnds.Clear();
        }

        private void ReplaceHCursors() {
            // 将窗口类中的 HCURSOR 替换为透明光标
            void ReplaceHCursor(IntPtr hWnd) {
                if (_replacedHwnds.Contains(hWnd)) {
                    return;
                }

                IntPtr hCursor = new IntPtr(NativeMethods.GetClassAuto(hWnd, NativeMethods.GCLP_HCURSOR));
                if (hCursor == IntPtr.Zero
                    || hCursor == _arrowCursor
                    || hCursor == _handCursor
                    || hCursor == _appStartingCursor
                ) {
                    // 不替换透明的系统光标
                    return;
                }

                if (_hCursorToTptCursor.ContainsKey(hCursor)) {
                    // 之前已替换过
                    SafeCursorHandle hTptCursor = _hCursorToTptCursor[hCursor];

                    // 替换窗口类的 HCURSOR
                    if (NativeMethods.SetClassAuto(hWnd, NativeMethods.GCLP_HCURSOR, hTptCursor.DangerousGetHandle().ToInt64()) == hCursor.ToInt64()) {
                        _replacedHwnds.Add(hWnd);
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
                        if (NativeMethods.PostMessage(_hwndHost, MAGPIE_WM_NEWCURSOR, hTptCursor.DangerousGetHandle(), hCursor)) {
                            // 替换成功
                            _replacedHwnds.Add(hWnd);
                            _hCursorToTptCursor[hCursor] = hTptCursor;
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
            ReplaceHCursor(_hwndSrc);
            NativeMethods.EnumChildWindows(_hwndSrc, (IntPtr hWnd, int _1) => {
                ReplaceHCursor(hWnd);
                return true;
            }, IntPtr.Zero);

            // 向源窗口发送 WM_SETCURSOR，一般可以使其调用 SetCursor
            ReportIfFalse(
                NativeMethods.PostMessage(
                    _hwndSrc,
                    NativeMethods.WM_SETCURSOR,
                    _hwndSrc,
                    (IntPtr)NativeMethods.HTCLIENT
                ),
                "PostMessage 失败"
            );
        }


        // 用于创建 SetCursor 委托
        [UnmanagedFunctionPointer(CallingConvention.StdCall, SetLastError = true)]
        private delegate IntPtr SetCursorDelegate(IntPtr hCursor);

        // 取代 SetCursor 的钩子
        private IntPtr SetCursorHook(IntPtr hCursor) {
            // ReportToServer("SetCursor");
            IntPtr hCursorTar = hCursor;

            if (_hwndHost == IntPtr.Zero || hCursor == IntPtr.Zero || !NativeMethods.IsWindow(_hwndHost)) {
                // 不存在全屏窗口时钩子不做任何操作
            } else if (_hCursorToTptCursor.ContainsKey(hCursor)) {
                hCursorTar = _hCursorToTptCursor[hCursor].DangerousGetHandle();
            } else {
                // 未出现过的 hCursor
                SafeCursorHandle hTptCursor = CreateTransparentCursor(hCursor);
                if (hTptCursor == SafeCursorHandle.Zero) {
                    return NativeMethods.SetCursor(hCursor);
                }

                _hCursorToTptCursor[hCursor] = hTptCursor;

                // 向全屏窗口发送光标句柄
                ReportCursorMap(hTptCursor, hCursor);

                hCursorTar = hTptCursor.DangerousGetHandle();
            }

            return NativeMethods.SetCursor(hCursorTar);
        }


        private SafeCursorHandle CreateTransparentCursor(IntPtr hotSpot) {
            int len = _cursorSize.x * _cursorSize.y;

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
                _cursorSize.x, _cursorSize.y,
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
                Math.Min((int)ii.xHotspot, _cursorSize.x),
                Math.Min((int)ii.yHotspot, _cursorSize.y)
            );
        }

        // 向全屏窗口发送光标句柄
        private void ReportCursorMap(SafeCursorHandle hTptCursor, IntPtr hCursor) {
            ReportIfFalse(
                NativeMethods.PostMessage(_hwndHost, MAGPIE_WM_NEWCURSOR, hTptCursor.DangerousGetHandle(), hCursor),
                "PostMessage 失败"
            );
        }

        private void ReportToServer(string msg) {
#if DEBUG
            if(_messageQueue.Count < 1000) {
                _messageQueue.Enqueue(msg);
            }
#endif
        }

        private void ReportIfFalse(bool flag, string msg) {
#if DEBUG
            if(!flag) {
                ReportToServer("出错: " + msg);
            }
#endif
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing) {
            // 清理 HCURSOR
            foreach (var cursorHandle in _hCursorToTptCursor.Values) {
                if (cursorHandle != null && !cursorHandle.IsInvalid) {
                    cursorHandle.Dispose();
                }
            }
            // SafeHandle records the fact that we've called Dispose.
        }
    }
}
