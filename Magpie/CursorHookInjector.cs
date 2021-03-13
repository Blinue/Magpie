using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Runtime.Remoting;
using EasyHook;
using System.IO;
using System.Reflection;
using Magpie.CursorHook;
using System.Runtime.Remoting.Channels.Ipc;

namespace Magpie {
    public class CursorHookInjector {
        private string channelName = null;
        private readonly int targetPID;
        private IpcServerChannel ipcServer;

        public CursorHookInjector(int targetPID, IntPtr hwndHost, IntPtr hwndSrc) {
            Debug.Assert(targetPID > 0);

            this.targetPID = targetPID;

            // Create the IPC server using the FileMonitorIPC.ServiceInterface class as a singleton
            ipcServer = RemoteHooking.IpcCreateServer<ServerInterface>(ref channelName, WellKnownObjectMode.Singleton);

            // Get the full path to the assembly we want to inject into the target process
            string injectionLibrary = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "CursorHook.dll");

            EasyHook.RemoteHooking.Inject(
                    targetPID,          // ID of process to inject into
                    injectionLibrary,   // 32-bit library to inject (if target is 32-bit)
                    injectionLibrary,   // 64-bit library to inject (if target is 64-bit)
                    channelName,        // the parameters to pass into injected library
                    hwndHost,
                    hwndSrc             // ...
                );
        }

    }
}
