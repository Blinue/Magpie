/*
 * ---------------------
 *   非常强大，非常脆弱
 * ---------------------
 */

using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;


namespace Magpie.CursorHook {
    /// <summary>
    /// EasyHook will look for a class implementing <see cref="EasyHook.IEntryPoint"/> during injection. This
    /// becomes the entry point within the target process after injection is complete.
    /// </summary>
    public class InjectionEntryPoint : EasyHook.IEntryPoint {
#if DEBUG
        /// <summary>
        /// Reference to the server interface within FileMonitor
        /// </summary>
        private readonly ServerInterface _server;

        /// <summary>
        /// Message queue of all files accessed
        /// </summary>
        private readonly Queue<string> _messageQueue = new Queue<string>();
#endif

        private readonly IntPtr _hwndHost;
        private readonly IntPtr _hwndSrc;

        private readonly (int x, int y) _cursorSize = NativeMethods.GetCursorSize();
        
        private readonly Dictionary<IntPtr, IntPtr> _hwndTohCursor = new Dictionary<IntPtr, IntPtr>();
        private readonly Dictionary<IntPtr, IntPtr> _hCursorToTptCursor = new Dictionary<IntPtr, IntPtr>();

        /// <summary>
        /// EasyHook requires a constructor that matches <paramref name="context"/> and any additional parameters as provided
        /// in the original call to <see cref="EasyHook.RemoteHooking.Inject(int, EasyHook.InjectionOptions, string, string, object[])"/>.
        /// 
        /// Multiple constructors can exist on the same <see cref="EasyHook.IEntryPoint"/>, providing that each one has a corresponding Run method (e.g. <see cref="Run(EasyHook.RemoteHooking.IContext, string)"/>).
        /// </summary>
        /// <param name="context">The RemoteHooking context</param>
        /// <param name="channelName">The name of the IPC channel</param>
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
            // Connect to server object using provided channel name
            _server = EasyHook.RemoteHooking.IpcConnectClient<ServerInterface>(channelName);

            // If Ping fails then the Run method will be not be called
            _server.Ping();
#endif
        }

        /// <summary>
        /// The main entry point for our logic once injected within the target process. 
        /// This is where the hooks will be created, and a loop will be entered until host process exits.
        /// EasyHook requires a matching Run method for the constructor
        /// </summary>
        /// <param name="context">The RemoteHooking context</param>
        /// <param name="channelName">The name of the IPC channel</param>
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

            // Hook 当前线程外的所有线程
            setCursorHook.ThreadACL.SetExclusiveACL(new int[] { 0 });

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
                if (_hwndTohCursor.ContainsKey(hWnd)) {
                    return;
                }

                // Get(Set)ClassLong 不能使用 Ptr 版本
                IntPtr hCursor = (IntPtr)NativeMethods.GetClassLong(hWnd, NativeMethods.GCLP_HCURSOR);
                if (hCursor == IntPtr.Zero) {
                    return;
                }

                _hwndTohCursor[hWnd] = hCursor;

                IntPtr hTptCursor = IntPtr.Zero;
                if (_hCursorToTptCursor.ContainsKey(hCursor)) {
                    // 透明的系统光标或之前已替换过的
                    hTptCursor = _hCursorToTptCursor[hCursor];
                } else {
                    hTptCursor = CreateTransparentCursor(0, 0);
                    if (hTptCursor != IntPtr.Zero) {
                        _hCursorToTptCursor[hCursor] = hTptCursor;

                        // 向全屏窗口发送光标句柄
                        NativeMethods.SendMessage(_hwndHost, NativeMethods.MAGPIE_WM_NEWCURSOR, hTptCursor, hCursor);
                    } else {
                        // 创建透明光标失败
                        hTptCursor = hCursor;
                    }
                }

                // 替换窗口类的 HCURSOR
                NativeMethods.SetClassLong(hWnd, NativeMethods.GCLP_HCURSOR, hTptCursor);
            }

            // 替换进程中所有顶级窗口的窗口类的 HCURSOR
            /*NativeMethods.EnumChildWindows(IntPtr.Zero, (IntPtr hWnd, int processId) => {
                if (NativeMethods.GetWindowProcessId(hWnd) != processId) {
                    return true;
                }

                replaceHCursor(hWnd);
                NativeMethods.EnumChildWindows(hWnd, (IntPtr hWnd1, int _3) => {
                    replaceHCursor(hWnd1);
                    return true;
                }, 0);
                return true;
            }, NativeMethods.GetWindowProcessId(hwndSrc));*/

            // 替换源窗口和它的所有子窗口的窗口类的 HCRUSOR
            // 因为通过窗口类的 HCURSOR 设置光标不会通过 SetCursor
            IntPtr hwndTop = NativeMethods.GetTopWindow(_hwndSrc);
            replaceHCursor(hwndTop);
            NativeMethods.EnumChildWindows(hwndTop, (IntPtr hWnd, int _4) => {
                replaceHCursor(hWnd);
                return true;
            }, 0);

            // 向源窗口发送 WM_SETCURSOR，一般可以使其调用 SetCursor
            NativeMethods.PostMessage(
                _hwndSrc,
                NativeMethods.WM_SETCURSOR,
                _hwndSrc,
                (IntPtr)NativeMethods.HTCLIENT
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
            foreach (var item in _hwndTohCursor) {
                NativeMethods.SetClassLong(item.Key, NativeMethods.GCLP_HCURSOR, item.Value);
            }

            // 卸载钩子
            setCursorHook.Dispose();
            EasyHook.LocalHook.Release();
        }


        // 用于创建 SetCursor 委托
        [UnmanagedFunctionPointer(CallingConvention.StdCall, CharSet = CharSet.Unicode, SetLastError = true)]
        delegate IntPtr SetCursor_Delegate(IntPtr hCursor);


        // 取代 SetCursor 的钩子
        IntPtr SetCursor_Hook(IntPtr hCursor) {
            // ReportToServer("setcursor");

            if (!NativeMethods.IsWindow(_hwndHost) || hCursor == IntPtr.Zero) {
                // 全屏窗口关闭后钩子不做任何操作
                return NativeMethods.SetCursor(hCursor);
            }

            if(_hCursorToTptCursor.ContainsKey(hCursor)) {
                return NativeMethods.SetCursor(_hCursorToTptCursor[hCursor]);
            }

            // 未出现过的 hCursor
            IntPtr hTptCursor = CreateTransparentCursor(0, 0);
            if(hTptCursor != IntPtr.Zero) {
                _hCursorToTptCursor[hCursor] = hTptCursor;
            } else {
                // 创建透明光标失败
                hTptCursor = hCursor;
            }

            var rt = NativeMethods.SetCursor(hTptCursor);
            // 向全屏窗口发送光标句柄
            NativeMethods.PostMessage(_hwndHost, NativeMethods.MAGPIE_WM_NEWCURSOR, NativeMethods.GetCursor(), hCursor);

            return rt;
        }

        private IntPtr CreateTransparentCursor(int xHotSpot, int yHotSpot) {
            int len = _cursorSize.x * _cursorSize.y;

            // 全 0xff
            byte[] andPlane = new byte[len];
            for (int i = 0; i < len; ++i) {
                andPlane[i] = 0xff;
            }

            // 全 0
            byte[] xorPlane = new byte[len];

            return NativeMethods.CreateCursor(NativeMethods.GetModule(), xHotSpot, yHotSpot,
                _cursorSize.x, _cursorSize.y, andPlane, xorPlane);
        }

        private void ReportToServer(string msg) {
#if DEBUG
            _messageQueue.Enqueue(msg);
#endif
        }
    }
}
