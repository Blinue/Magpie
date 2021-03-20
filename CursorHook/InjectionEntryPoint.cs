/*
 * ---------------------
 *   非常强大，非常脆弱
 * ---------------------
 * 
 * 我时常惊叹于 Windows 系统的健壮性，
 * 使用 Windows API 时唯一的限制是你的想象力。
 * 本 HOOK 大部分情况下可以工作，如果可以你就赚了！
 * 
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
    public class InjectionEntryPoint : EasyHook.IEntryPoint {
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

        private static readonly IntPtr _arrowCursor = NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_ARROW);
        private static readonly IntPtr _handCursor = NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_HAND);
        private static readonly IntPtr _appStartingCursor = NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_APPSTARTING);

        // 原光标到透明光标的映射
        // 不替换透明的系统光标
        private readonly Dictionary<IntPtr, IntPtr> _hCursorToTptCursor = new Dictionary<IntPtr, IntPtr>() {
            {_arrowCursor, _arrowCursor},
            {_handCursor, _handCursor},
            {_appStartingCursor, _appStartingCursor}
        };

        private const string HOST_WINDOW_CLASS_NAME = "Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";


        // 运行时注入的入口
        public InjectionEntryPoint(
            EasyHook.RemoteHooking.IContext context,
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
            EasyHook.RemoteHooking.IContext _
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
            EasyHook.RemoteHooking.IContext _,
#if DEBUG
            string _1,
#endif
            IntPtr _2, IntPtr _3
        ) {
            // 安装钩子
            EasyHook.LocalHook setCursorHook = null;
            try {
                // 截获 SetCursor
                setCursorHook = EasyHook.LocalHook.Create(
                    EasyHook.LocalHook.GetProcAddress("user32.dll", "SetCursor"),
                    new SetCursor_Delegate(SetCursor_Hook),
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

            // 清理资源
            DestroyCursors();

            // 卸载钩子
            setCursorHook.Dispose();
            EasyHook.LocalHook.Release();
        }

        // 启动时注入逻辑的入口
        public void Run(
            EasyHook.RemoteHooking.IContext _
#if DEBUG
            , string _1
#endif
        ) {
            // 安装钩子
            EasyHook.LocalHook setCursorHook = null;
            try {
                // 截获 SetCursor
                setCursorHook = EasyHook.LocalHook.Create(
                    EasyHook.LocalHook.GetProcAddress("user32.dll", "SetCursor"),
                    new SetCursor_Delegate(SetCursor_Hook),
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

            DestroyCursors();

            // 卸载钩子
            setCursorHook.Dispose();
            EasyHook.LocalHook.Release();
        }

        void DestroyCursors() {
            foreach (var item in _hCursorToTptCursor) {
                // 删除系统光标是无害的行为
                // 忽略错误
                NativeMethods.DestroyCursor(item.Value);
            }
        }

        void ReplaceHCursorsBack() {
            foreach (var hwnd in _replacedHwnds) {
                IntPtr hCursor = NativeMethods.GetClassLongAuto(hwnd, NativeMethods.GCLP_HCURSOR);
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
                    .FirstOrDefault(pair => pair.Value == hCursor);

                if (item.Key == IntPtr.Zero || item.Value == IntPtr.Zero) {
                    // 找不到就不替换
                    continue;
                }

                // 忽略错误
                NativeMethods.SetClassLongAuto(hwnd, NativeMethods.GCLP_HCURSOR, item.Key);
            }
        }

        void ReplaceHCursors() {
            // 将窗口类中的 HCURSOR 替换为透明光标
            void ReplaceHCursor(IntPtr hWnd) {
                if (_replacedHwnds.Contains(hWnd)) {
                    return;
                }

                IntPtr hCursor = NativeMethods.GetClassLongAuto(hWnd, NativeMethods.GCLP_HCURSOR);
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
                    IntPtr hTptCursor = _hCursorToTptCursor[hCursor];

                    // 替换窗口类的 HCURSOR
                    if (NativeMethods.SetClassLongAuto(hWnd, NativeMethods.GCLP_HCURSOR, hTptCursor) == hCursor) {
                        _replacedHwnds.Add(hWnd);
                    } else {
                        ReportIfFalse(false, "SetClassLongAuto 失败");
                    }
                } else {
                    // 以下代码如果出错不会有任何更改

                    IntPtr hTptCursor = CreateTransparentCursor(hCursor);
                    if (hTptCursor == IntPtr.Zero) {
                        return;
                    }

                    // 替换窗口类的 HCURSOR
                    if (NativeMethods.SetClassLongAuto(hWnd, NativeMethods.GCLP_HCURSOR, hTptCursor) == hCursor) {
                        // 向全屏窗口发送光标句柄
                        if (NativeMethods.PostMessage(_hwndHost, NativeMethods.MAGPIE_WM_NEWCURSOR, hTptCursor, hCursor)) {
                            // 替换成功
                            _replacedHwnds.Add(hWnd);
                            _hCursorToTptCursor[hCursor] = hTptCursor;
                        } else {
                            NativeMethods.SetClassLongAuto(hWnd, NativeMethods.GCLP_HCURSOR, hCursor);
                            NativeMethods.DestroyCursor(hTptCursor);
                            ReportIfFalse(false, "PostMessage 失败");
                        }
                    } else {
                        NativeMethods.DestroyCursor(hTptCursor);
                        ReportIfFalse(false, "SetClassLongAuto 失败");
                    }
                }
            }

            // 替换源窗口和它的所有子窗口的窗口类的 HCRUSOR
            ReplaceHCursor(_hwndSrc);
            NativeMethods.EnumChildWindows(_hwndSrc, (IntPtr hWnd, int _4) => {
                ReplaceHCursor(hWnd);
                return true;
            }, 0);

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
        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode, SetLastError = true)]
        public delegate IntPtr SetCursor_Delegate(IntPtr hCursor);

        // 取代 SetCursor 的钩子
        public IntPtr SetCursor_Hook(IntPtr hCursor) {
            // ReportToServer("SetCursor");

            if (_hwndHost == IntPtr.Zero || hCursor == IntPtr.Zero || !NativeMethods.IsWindow(_hwndHost)) {
                // 不存在全屏窗口时钩子不做任何操作
                return NativeMethods.SetCursor(hCursor);
            }

            if(_hCursorToTptCursor.ContainsKey(hCursor)) {
                return NativeMethods.SetCursor(_hCursorToTptCursor[hCursor]);
            }

            // 未出现过的 hCursor
            return NativeMethods.SetCursor(GetReplacedCursor(hCursor));
        }

        private IntPtr GetReplacedCursor(IntPtr hCursor) {
            if (hCursor == IntPtr.Zero) {
                return IntPtr.Zero;
            }

            IntPtr hTptCursor = CreateTransparentCursor(hCursor);
            if (hTptCursor == IntPtr.Zero) {
                return hCursor;
            }

            _hCursorToTptCursor[hCursor] = hTptCursor;

            // 向全屏窗口发送光标句柄
            ReportCursorMap(hTptCursor, hCursor);

            return hTptCursor;
        }

        private IntPtr CreateTransparentCursor(IntPtr hotSpot) {
            int len = _cursorSize.x * _cursorSize.y;

            // 全 0xff
            byte[] andPlane = new byte[len];
            for (int i = 0; i < len; ++i) {
                andPlane[i] = 0xff;
            }

            // 全 0
            byte[] xorPlane = new byte[len];

            var (xHotSpot, yHotSpot) = GetCursorHotSpot(hotSpot);

            IntPtr rt = NativeMethods.CreateCursor(
                NativeMethods.GetModule(),
                xHotSpot, yHotSpot,
                _cursorSize.x, _cursorSize.y,
                andPlane, xorPlane
            );

            ReportIfFalse(rt != IntPtr.Zero, "创建透明光标失败");
            return rt;
        }

        private (int, int) GetCursorHotSpot(IntPtr hCursor) {
            if (hCursor == IntPtr.Zero) {
                return (0, 0);
            }

            NativeMethods.ICONINFO ii = new NativeMethods.ICONINFO();
            if (!NativeMethods.GetIconInfo(hCursor, ref ii)) {

                return (0, 0);
            }
            // 忽略错误
            NativeMethods.DeleteObject(ii.hbmMask);
            NativeMethods.DeleteObject(ii.hbmColor);

            return (
                Math.Min((int)ii.xHotspot, _cursorSize.x),
                Math.Min((int)ii.yHotspot, _cursorSize.y)
            );
        }

        // 向全屏窗口发送光标句柄
        private void ReportCursorMap(IntPtr hTptCursor, IntPtr hCursor) {
            ReportIfFalse(
                NativeMethods.PostMessage(_hwndHost, NativeMethods.MAGPIE_WM_NEWCURSOR, hTptCursor, hCursor),
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
    }
}
