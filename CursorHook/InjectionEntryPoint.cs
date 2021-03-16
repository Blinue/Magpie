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
        private readonly ServerInterface _server;

        private readonly Queue<string> _messageQueue = new Queue<string>();
#endif

        private readonly IntPtr _hwndHost;
        private readonly IntPtr _hwndSrc;

        private readonly (int x, int y) _cursorSize = NativeMethods.GetCursorSize();

        // 保存已替换 HCURSOR 的窗口，以在卸载钩子时还原
        private readonly HashSet<IntPtr> _replacedHwnds = new HashSet<IntPtr>();

        // 原光标到透明光标的映射
        private readonly Dictionary<IntPtr, IntPtr> _hCursorToTptCursor = new Dictionary<IntPtr, IntPtr>();


        // EasyHook 需要此方法作为入口
        public InjectionEntryPoint(
            EasyHook.RemoteHooking.IContext _,
#if DEBUG
            string channelName,
#endif
            IntPtr hwndHost, IntPtr hwndSrc
         ) {
            _hwndHost = hwndHost;
            _hwndSrc = hwndSrc;
#if DEBUG
            // DEBUG 时连接 IPC server
            _server = EasyHook.RemoteHooking.IpcConnectClient<ServerInterface>(channelName);

            // 测试连接性，如果失败会抛出异常静默的失败因此 Run 方法不会执行
            _server.Ping();
#endif
        }

        // 注入逻辑的入口
        public void Run(
            EasyHook.RemoteHooking.IContext _,
#if DEBUG
            string _1,
#endif
            IntPtr _2, IntPtr _3
        ) {
            // 安装钩子

            // 截获 SetCursor
            var setCursorHook = EasyHook.LocalHook.Create(
                EasyHook.LocalHook.GetProcAddress("user32.dll", "SetCursor"),
                new SetCursor_Delegate(SetCursor_Hook),
                this
            );

            // 只 Hook 窗口线程，因为 SetCursor 必须在窗口线程上调用
            setCursorHook.ThreadACL.SetInclusiveACL(
                new int[] { NativeMethods.GetWindowThreadId(_hwndSrc) }
            );

            ReportToServer("SetCursor钩子安装成功");

            // 不替换这些系统光标，因为已被全局替换
            var arrowCursor = NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_ARROW);
            var handCursor = NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_HAND);
            var appStartingCursor = NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_APPSTARTING);
            _hCursorToTptCursor[arrowCursor] = arrowCursor;
            _hCursorToTptCursor[handCursor] = handCursor;
            _hCursorToTptCursor[appStartingCursor] = appStartingCursor;

            // 将窗口类中的 HCURSOR 替换为透明光标
            void replaceHCursor(IntPtr hWnd) {
                if (_replacedHwnds.Contains(hWnd)) {
                    return;
                }

                // Get(Set)ClassLong 不能使用 Ptr 版本
                IntPtr hCursor = (IntPtr)NativeMethods.GetClassLong(hWnd, NativeMethods.GCLP_HCURSOR);
                if (hCursor == IntPtr.Zero) {
                    return;
                }

                if (_hCursorToTptCursor.ContainsKey(hCursor)) {
                    // 透明的系统光标或之前已替换过
                    IntPtr hTptCursor = _hCursorToTptCursor[hCursor];

                    // 替换窗口类的 HCURSOR
                    if (NativeMethods.SetClassLong(hWnd, NativeMethods.GCLP_HCURSOR, hTptCursor) == hCursor) {
                        _replacedHwnds.Add(hWnd);
                    } else {
                        ReportIfFalse(false, "SetClassLong 失败");
                    }
                } else {
                    // 如果出错不会有任何更改

                    IntPtr hTptCursor = CreateTransparentCursor(hCursor);
                    if (hTptCursor == IntPtr.Zero) {
                        return;
                    }

                    // 向全屏窗口发送光标句柄
                    if (NativeMethods.PostMessage(_hwndHost, NativeMethods.MAGPIE_WM_NEWCURSOR, hTptCursor, hCursor)) {
                        // 替换窗口类的 HCURSOR
                        if (NativeMethods.SetClassLong(hWnd, NativeMethods.GCLP_HCURSOR, hTptCursor) == hCursor) {
                            // 替换成功
                            _replacedHwnds.Add(hWnd);
                            _hCursorToTptCursor[hCursor] = hTptCursor;
                        } else {
                            NativeMethods.DestroyCursor(hTptCursor);
                            ReportIfFalse(false, "SetClassLong 失败");
                        }
                    } else {
                        NativeMethods.DestroyCursor(hTptCursor);
                        ReportIfFalse(false, "PostMessage 失败");
                    }
                }
            }

            // 替换进程中所有顶级窗口的窗口类的 HCURSOR
            /*NativeMethods.EnumChildWindows(IntPtr.Zero, (IntPtr hWnd, int processId) => {
                if (NativeMethods.GetWindowProcessId(hWnd) != processId) {
                    return true;
                }

                replaceHCursor(hWnd);
                NativeMethods.EnumChildWindows(hWnd, (IntPtr hWnd1, int _4) => {
                    replaceHCursor(hWnd1);
                    return true;
                }, 0);
                return true;
            }, NativeMethods.GetWindowProcessId(_hwndSrc));*/

            // 替换源窗口和它的所有子窗口的窗口类的 HCRUSOR
            // 因为通过窗口类的 HCURSOR 设置光标不会通过 SetCursor
            IntPtr hwndTop = NativeMethods.GetTopWindow(_hwndSrc);
            replaceHCursor(hwndTop);
            NativeMethods.EnumChildWindows(hwndTop, (IntPtr hWnd, int _4) => {
                replaceHCursor(hWnd);
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

            try {
                // Loop until FileMonitor closes (i.e. IPC fails)
                while (true) {
                    Thread.Sleep(200);

                    if(!NativeMethods.IsWindow(_hwndHost)) {
                        // 全屏窗口已关闭，卸载钩子
                        break;
                    }

#if DEBUG
                    string[] queued = _messageQueue.ToArray();
                    _messageQueue.Clear();

                    if (queued.Length > 0) {
                        _server.ReportMessages(queued);
                    } else {
                        _server.Ping();
                    }
#endif
                }
            } catch {
                // 如果服务器关闭 Ping() 和 ReportMessages() 将抛出异常
                // 执行到此处表示 Magpie 已关闭
            }

            // 退出前重置窗口类的光标
            foreach (var hwnd in _replacedHwnds) {
                IntPtr hCursor = (IntPtr)NativeMethods.GetClassLong(hwnd, NativeMethods.GCLP_HCURSOR);
                if (hCursor == IntPtr.Zero) {
                    continue;
                }

                // 在 _hCursorToTptCursor 中反向查找
                var item = _hCursorToTptCursor
                    .FirstOrDefault(pair => pair.Value == hCursor);

                if(item.Key == IntPtr.Zero || item.Value == IntPtr.Zero) {
                    // 找不到就不替换
                    continue;
                }

                // 忽略错误
                NativeMethods.SetClassLong(hwnd, NativeMethods.GCLP_HCURSOR, item.Key);
            }

            // 清理资源
            foreach (var item in _hCursorToTptCursor) {
                // 删除系统光标是无害的行为
                // 忽略错误
                NativeMethods.DestroyCursor(item.Value);
            }

            // 卸载钩子
            setCursorHook.Dispose();
            EasyHook.LocalHook.Release();
        }


        // 用于创建 SetCursor 委托
        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode, SetLastError = true)]
        public delegate IntPtr SetCursor_Delegate(IntPtr hCursor);

        // 取代 SetCursor 的钩子
        public IntPtr SetCursor_Hook(IntPtr hCursor) {
            // ReportToServer("SetCursor");

            if (!NativeMethods.IsWindow(_hwndHost) || hCursor == IntPtr.Zero) {
                // 全屏窗口关闭后钩子不做任何操作
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
            ReportIfFalse(
                NativeMethods.PostMessage(_hwndHost, NativeMethods.MAGPIE_WM_NEWCURSOR, hTptCursor, hCursor),
                "PostMessage 失败"
            );

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
