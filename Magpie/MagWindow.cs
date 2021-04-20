using EasyHook;
using Magpie.CursorHook;
using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Remoting;
using System.Threading;
using System.Windows.Forms;


namespace Magpie {
    enum MagWindowStatus : int {
        Idle = 0, Starting = 1, Running = 2
    }

    class MagWindow {
        private Thread magThread = null;

        // 用于从全屏窗口的线程接收消息
        private event Action<int, string> StatusEvent;

        public MagWindowStatus Status { get; private set; } = MagWindowStatus.Idle;


        public MagWindow() {
            StatusEvent += (int status, string errorMsg) => {
                if(status < 0 || status > 3) {
                    return;
                }

                Status = (MagWindowStatus)status;

                if(Status == MagWindowStatus.Idle) {
                    magThread = null;
                }

                if (errorMsg != null) {
                    MessageBox.Show("创建全屏窗口出错：" + errorMsg);
                }
            };
        }

        public void Create(
            string effectsJson,
            int captureMode,
            bool showFPS,
            bool lowLatencyMode,
            bool noVSync,
            bool hookCursorAtRuntime,
            bool noDisturb = false
        ) {
            if (Status != MagWindowStatus.Idle) {
                return;
            }

            // 使用 WinRT Capturer API 需要在 MTA 中
            // C# 窗体必须使用 STA，因此将全屏窗口创建在新的线程里
            magThread = new Thread(() => {
                NativeMethods.RunMagWindow(
                    (int status, IntPtr errorMsg) => StatusEvent(status, Marshal.PtrToStringUni(errorMsg)),
                    effectsJson,    // 缩放模式
                    captureMode,    // 抓取模式
                    showFPS,        // 显示 FPS
                    lowLatencyMode, // 低延迟模式
                    noVSync,        // 关闭垂直同步
                    noDisturb       // 用于调试
                );
            });
            magThread.SetApartmentState(ApartmentState.MTA);
            magThread.Start();

            if (hookCursorAtRuntime) {
                HookCursorAtRuntime();
            }
        }

        public void Destory() {
            NativeMethods.BroadcastMessage(NativeMethods.MAGPIE_WM_DESTORYMAG);
            magThread = null;
        }

        private void HookCursorAtRuntime() {
            IntPtr hwndSrc = NativeMethods.GetForegroundWindow();
            int pid = NativeMethods.GetWindowProcessId(hwndSrc);
            if (pid == 0 || pid == Process.GetCurrentProcess().Id) {
                // 不能 hook 本进程
                return;
            }

#if DEBUG
            string channelName = null;
            // DEBUG 时创建 IPC server
            RemoteHooking.IpcCreateServer<ServerInterface>(ref channelName, WellKnownObjectMode.Singleton);
#endif

            // 获取 CursorHook.dll 的绝对路径
            string injectionLibrary = Path.Combine(
                Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "CursorHook.dll"
            );

            // 使用 EasyHook 注入
            try {
                EasyHook.RemoteHooking.Inject(
                pid,                // 要注入的进程的 ID
                injectionLibrary,   // 32 位 DLL
                injectionLibrary,   // 64 位 DLL
                // 下面为传递给注入 DLL 的参数
#if DEBUG
                channelName,
#endif
                hwndSrc
                );
            } catch (Exception e) {
                Console.WriteLine("CursorHook 注入失败：" + e.Message);
            }
        }

        public void HookCursorAtStartUp(string exePath) {
#if DEBUG
            string channelName = null;
            // DEBUG 时创建 IPC server
            RemoteHooking.IpcCreateServer<ServerInterface>(ref channelName, WellKnownObjectMode.Singleton);
#endif

            // 获取 CursorHook.dll 的绝对路径
            string injectionLibrary = Path.Combine(
                Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "CursorHook.dll"
            );

            try {
                EasyHook.RemoteHooking.CreateAndInject(
                    exePath,    // 可执行文件路径
                    "",         // 命令行参数
                    0,          // 传递给 CreateProcess 的标志
                    injectionLibrary,   // 32 位 DLL
                    injectionLibrary,   // 64 位 DLL
                    out int _  // 忽略进程 ID
                               // 下面为传递给注入 DLL 的参数
#if DEBUG
                    , channelName
#endif
                );
            } catch (Exception e) {
                Console.WriteLine("CursorHook 注入失败：" + e.Message);
            }
        }
    }
}
