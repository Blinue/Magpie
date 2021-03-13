/*
 * ---------------------
 *   非常舒服，非常脆弱
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
        /// <summary>
        /// Reference to the server interface within FileMonitor
        /// </summary>
        private readonly ServerInterface _server;

        /// <summary>
        /// Message queue of all files accessed
        /// </summary>
        private readonly Queue<string> _messageQueue = new Queue<string>();

        private readonly IntPtr _hwndHost;

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
            EasyHook.RemoteHooking.IContext context,
            string channelName, IntPtr hwndHost, IntPtr hwndSrc) {
            _hwndHost = hwndHost;

            // Connect to server object using provided channel name
            _server = EasyHook.RemoteHooking.IpcConnectClient<ServerInterface>(channelName);

            // If Ping fails then the Run method will be not be called
            _server.Ping();
        }

        /// <summary>
        /// The main entry point for our logic once injected within the target process. 
        /// This is where the hooks will be created, and a loop will be entered until host process exits.
        /// EasyHook requires a matching Run method for the constructor
        /// </summary>
        /// <param name="context">The RemoteHooking context</param>
        /// <param name="channelName">The name of the IPC channel</param>
        public void Run(EasyHook.RemoteHooking.IContext _, string _1, IntPtr _2, IntPtr hwndSrc) {
            // Injection is now complete and the server interface is connected
            _server.IsInstalled(EasyHook.RemoteHooking.GetCurrentProcessId());
            
            // Install hooks

            // SetCursor https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setcursor
            var setCursorHook = EasyHook.LocalHook.Create(
                EasyHook.LocalHook.GetProcAddress("user32.dll", "SetCursor"),
                new SetCursor_Delegate(SetCursor_Hook),
                this
            );

            // Activate hooks on all threads except the current thread
            setCursorHook.ThreadACL.SetExclusiveACL(new int[] { 0 });

            _server.ReportMessage("SetCursor钩子安装成功");

            // 不替换这些系统光标，因为已被全局替换
            var arrowCursor = NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_ARROW);
            var handCursor = NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_HAND);
            var appStartingCursor = NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_APPSTARTING);
            _hCursorToTptCursor[arrowCursor] = arrowCursor;
            _hCursorToTptCursor[handCursor] = handCursor;
            _hCursorToTptCursor[appStartingCursor] = appStartingCursor;

            // 将窗口类中的 HCURSOR 替换为透明光标
            void replaceHCursor(IntPtr hWnd) {
                // Get(Set)ClassLong 不能使用 Ptr 版本
                IntPtr hCursor = (IntPtr)NativeMethods.GetClassLong(hWnd, NativeMethods.GCLP_HCURSOR);
                if (hCursor == IntPtr.Zero) {
                    return;
                }
                
                if (!_hwndTohCursor.ContainsKey(hWnd)) {
                    _hwndTohCursor.Add(hWnd, hCursor);

                    // 排除透明的系统光标
                    if (_hCursorToTptCursor.ContainsKey(hCursor)) {
                        return;
                    }

                    IntPtr hTptCursor = CreateTransparentCursor(0, 0);
                    if(hTptCursor == IntPtr.Zero) {
                        // 创建透明光标失败
                        hTptCursor = hCursor;
                    }

                    NativeMethods.SetClassLong(hWnd, NativeMethods.GCLP_HCURSOR, hTptCursor);

                    _hCursorToTptCursor[hCursor] = hTptCursor;

                    // 向全屏窗口发送光标句柄
                    NativeMethods.SendMessage(_hwndHost, NativeMethods.MAGPIE_WM_NEWCURSOR, hTptCursor, hCursor);
                }
            }
            
            replaceHCursor(hwndSrc);
            NativeMethods.EnumChildWindows(hwndSrc, (IntPtr hWnd, int lParam) => {
                replaceHCursor(hWnd);
                return true;
            }, 0);

            // 向源窗口发送 WM_SETCURSOR，有时有用
            NativeMethods.PostMessage(hwndSrc, 0x0020, hwndSrc, (IntPtr)1);
            
            try {
                // Loop until FileMonitor closes (i.e. IPC fails)
                while (true) {
                    Thread.Sleep(500);

                    string[] queued = null;

                    lock (_messageQueue) {
                        queued = _messageQueue.ToArray();
                        _messageQueue.Clear();
                    }

                    // Send newly monitored file accesses to FileMonitor
                    if (queued != null && queued.Length > 0) {
                        _server.ReportMessages(queued);
                    } else {
                        _server.Ping();
                    }
                }
            } catch {
                // Ping() or ReportMessages() will raise an exception if host is unreachable
            }

            // 退出前重置窗口类的光标
            foreach (var item in _hwndTohCursor) {
                NativeMethods.SetClassLong(item.Key, NativeMethods.GCLP_HCURSOR, item.Value);
            }

            // Remove hooks
            setCursorHook.Dispose();

            // Finalise cleanup of hooks
            EasyHook.LocalHook.Release();
        }


        // The SetCursor delegate, this is needed to create a delegate of our hook function
        [UnmanagedFunctionPointer(CallingConvention.StdCall,
                    CharSet = CharSet.Unicode,
                    SetLastError = true)]
        delegate IntPtr SetCursor_Delegate(IntPtr hCursor);


        // The SetCursor hook function. This will be called instead of the original SetCursor once hooked.
        IntPtr SetCursor_Hook(IntPtr hCursor) {
            if (!NativeMethods.IsWindow(_hwndHost) || hCursor == IntPtr.Zero) {
                // 全屏窗口关闭后钩子不做任何操作
                return NativeMethods.SetCursor(hCursor);
            }

            if(_hCursorToTptCursor.ContainsKey(hCursor)) {
                return NativeMethods.SetCursor(_hCursorToTptCursor[hCursor]);
            }

            // 未出现过的 hCursor
            _messageQueue.Enqueue("create");
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

        IntPtr CreateTransparentCursor(int xHotSpot, int yHotSpot) {
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
    }
}
