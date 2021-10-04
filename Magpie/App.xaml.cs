using Magpie.Properties;
using NLog;
using NLog.Config;
using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
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
			Path.Combine(AppDomain.CurrentDomain.SetupInformation.ApplicationBase, "ScaleModels.json");

		private static Logger Logger { get; } = LogManager.GetCurrentClassLogger();

		private static Mutex mutex = new Mutex(true, "{4C416227-4A30-4A2F-8F23-8701544DD7D6}");

		public static void SetLogLevel(int logLevel) {
			LogLevel minLogLevel = LogLevel.Info;
			switch (logLevel) {
				case 0:
					minLogLevel = LogLevel.Off;
					break;
				case 1:
					minLogLevel = LogLevel.Info;
					break;
				case 2:
					minLogLevel = LogLevel.Warn;
					break;
				case 3:
					minLogLevel = LogLevel.Error;
					break;
				default:
					break;
			}

			foreach (LoggingRule rule in LogManager.Configuration.LoggingRules) {
				rule.SetLoggingLevels(minLogLevel, LogLevel.Off);
			}
			LogManager.ReconfigExistingLoggers();

			Logger.Info($"当前日志级别：{minLogLevel}");
		}

		private void Application_Startup(object sender, StartupEventArgs e) {
			SetLogLevel(Settings.Default.LoggingLevel);

			Logger.Info($"程序启动\n\t进程 ID：{Process.GetCurrentProcess().Id}\n\tMagpie 版本：{APP_VERSION}\n\tOS 版本：{NativeMethods.GetOSVersion()}");

			// 不允许多个实例同时运行
			if (!mutex.WaitOne(TimeSpan.Zero, true)) {
				Logger.Info("已有实例，即将退出");

				Current.Shutdown();
				// 已存在实例时广播 WM_SHOWME，唤醒该实例
				_ = NativeMethods.BroadcastMessage(NativeMethods.MAGPIE_WM_SHOWME);

				mutex = null;
				return;
			}

			if (!string.IsNullOrEmpty(Settings.Default.CultureName)) {
				Thread.CurrentThread.CurrentCulture = Thread.CurrentThread.CurrentUICulture =
					CultureInfo.GetCultureInfo(Settings.Default.CultureName);
			}
			Logger.Info($"当前语言：{Thread.CurrentThread.CurrentUICulture.Name}");

			MainWindow window = new MainWindow();
			MainWindow = window;
			window.Show();
		}

		private void Application_Exit(object sender, ExitEventArgs e) {
			if (mutex != null) {
				Settings.Default.Save();

				mutex.ReleaseMutex();
			}

			Logger.Info($"程序关闭\n\t进程 ID：{Process.GetCurrentProcess().Id}");
		}
	}
}
