using System;
using System.Threading;
using System.Windows;


namespace Magpie {
	/// <summary>
	/// App.xaml 的交互逻辑
	/// </summary>
	public partial class App : Application {
		public static readonly Version APP_VERSION = new Version("0.6.0.0");
		public static readonly string APPLICATION_DIR = AppDomain.CurrentDomain.SetupInformation.ApplicationBase;
		public static readonly string SCALE_MODELS_JSON_PATH =
			AppDomain.CurrentDomain.SetupInformation.ApplicationBase + "ScaleModels.json";

		private static NLog.Logger Logger { get; } = NLog.LogManager.GetCurrentClassLogger();

		private static readonly Mutex mutex = new Mutex(true, "{4C416227-4A30-4A2F-8F23-8701544DD7D6}");

		private void Application_Startup(object sender, StartupEventArgs e) {
			Logger.Info("程序启动");
			Logger.Info("OS版本：" + NativeMethods.GetOSVersion());

			// 不允许多个实例同时运行
			if (!mutex.WaitOne(TimeSpan.Zero, true)) {
				Logger.Info("已有实例，即将退出");

				Current.Shutdown();
				// 已存在实例时广播 WM_SHOWME，唤醒该实例
				_ = NativeMethods.BroadcastMessage(NativeMethods.MAGPIE_WM_SHOWME);
			}
		}

		private void Application_Exit(object sender, ExitEventArgs e) {
			mutex.ReleaseMutex();

			Logger.Info("程序关闭");
		}
	}
}
