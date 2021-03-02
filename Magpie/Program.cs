using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Magpie {
    static class Program {
        static readonly Mutex mutex = new Mutex(true, "{4C416227-4A30-4A2F-8F23-8701544DD7D6}");
        
        /// <summary>
        /// 应用程序的主入口点。
        /// </summary>
        [STAThread]
        static void Main() {
            // 不允许多个实例同时运行
            if (mutex.WaitOne(TimeSpan.Zero, true)) {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new MainForm());
                mutex.ReleaseMutex();
            } else {
                // 已存在实例时广播 WM_SHOWME，唤醒该实例
                NativeMethods.PostMessage(
                    NativeMethods.HWND_BROADCAST,
                    MainForm.WM_SHOWME,
                    IntPtr.Zero,
                    IntPtr.Zero
                );
            }
        }
    }
}
