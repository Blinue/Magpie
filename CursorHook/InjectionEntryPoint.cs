using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
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
        private ServerInterface _server = null;

        /// <summary>
        /// Message queue of all files accessed
        /// </summary>
        private Queue<string> _messageQueue = new Queue<string>();

        private IntPtr _hwndHost;


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
            string channelName, IntPtr hwndHost) {
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
        public void Run(
            EasyHook.RemoteHooking.IContext context,
            string channelName, IntPtr hwndHost) {
            
            // Injection is now complete and the server interface is connected
            _server.IsInstalled(EasyHook.RemoteHooking.GetCurrentProcessId());

            // Install hooks

            // SetCursor https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setcursor
            var setCursorHook = EasyHook.LocalHook.Create(
                EasyHook.LocalHook.GetProcAddress("user32.dll", "SetCursor"),
                new SetCursor_Delegate(SetCursor_Hook),
                this);

            // Activate hooks on all threads except the current thread
            setCursorHook.ThreadACL.SetExclusiveACL(new int[] { 0 });

            _server.ReportMessage("SetCursor钩子安装成功");


            NativeMethods.EnumChildWindows(IntPtr.Zero, (IntPtr hWnd, int lParam) => {
                NativeMethods.SetClassLong(hWnd, NativeMethods.GCLP_HCURSOR,
                    NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_ARROW));

                NativeMethods.EnumChildWindows(hWnd, (IntPtr hWnd1, int lParam1) => {
                    NativeMethods.SetClassLong(hWnd, NativeMethods.GCLP_HCURSOR,
                        NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_ARROW));
                    return true;
                }, 0);
                return true;
            }, 0);
            NativeMethods.SetCursor(NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_ARROW));

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
            _server.ReportMessage("SetCursor前:" + hCursor.ToString());
            if (!NativeMethods.PostMessage(_hwndHost, NativeMethods.WM_COPYDATA, hCursor, IntPtr.Zero)) {
                _server.ReportMessage("error: " + Marshal.GetLastWin32Error().ToString());
            }

            var r = NativeMethods.SetCursor(hCursor);

            NativeMethods.PostMessage(_hwndHost, NativeMethods.WM_COPYDATA, NativeMethods.GetCursor(), IntPtr.Zero);
            _server.ReportMessage("SetCursor后:" + NativeMethods.GetCursor().ToString());
            return r;
            //return IntPtr.Zero;
        }
    }
}
