// Copyright (c) 2021 - present, Liu Xu
//
//  This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


using Magpie.Properties;
using NLog;
using NLog.Config;
using System;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Security.Principal;
using System.Threading;
using System.Windows;


// 关于 PerMonitor DPI 感知：
// 已禁用警告 WFAC010：ApplicationHighDpiMode 只对 winforms 起作用
namespace Magpie {
	/// <summary>
	/// Interaction logic for App.xaml
	/// </summary>
	public partial class App : Application {
		public static readonly Version APP_VERSION = new("0.8.1.0");
		public static readonly string SCALE_MODELS_JSON_PATH = ".\\ScaleModels.json";
		public static readonly string LOGS_FOLDER = ".\\logs\\";

		private static Logger Logger { get; } = LogManager.GetCurrentClassLogger();

		private static Mutex? mutex = new(true, "{4C416227-4A30-4A2F-8F23-8701544DD7D6}");

		public static void SetLogLevel(uint logLevel) {
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

		private static void InitNLog() {
			NLog.Targets.FileTarget logfile = new("logfile") {
				FileName = LOGS_FOLDER + "Magpie.log",
				ArchiveAboveSize= 100000,
				MaxArchiveFiles = 1,
				ArchiveFileName = LOGS_FOLDER + "Magpie.1.log",
				Encoding = System.Text.Encoding.UTF8,
				KeepFileOpen = true,
				Layout = "${longdate}|${level:uppercase=true}|${logger}|${message}${onexception:inner=|${exception}}"
			};

			LoggingConfiguration config = new();
			config.AddRule(LogLevel.Info, LogLevel.Off, logfile);
			LogManager.Configuration = config;
		}

		private void Application_Startup(object sender, StartupEventArgs e) {
			Environment.CurrentDirectory = AppDomain.CurrentDomain.BaseDirectory;

			InitNLog();
			SetLogLevel(Settings.Default.LoggingLevel);

			Logger.Info($"程序启动\n\t进程 ID：{Environment.ProcessId}\n\tMagpie 版本：{APP_VERSION}\n\tOS 版本：{Environment.OSVersion.Version}");

			if (!string.IsNullOrEmpty(Settings.Default.CultureName)) {
				Thread.CurrentThread.CurrentCulture = Thread.CurrentThread.CurrentUICulture =
					CultureInfo.GetCultureInfo(Settings.Default.CultureName);
			}
			Logger.Info($"当前语言：{Thread.CurrentThread.CurrentUICulture.Name}");

			// 检测管理员权限
			if (Settings.Default.RunAsAdmin && !IsRunAsAdministrator()) {
				// 创建一个管理员权限的新进程
				bool success = true;
				try {
					string exePath = Process.GetCurrentProcess().MainModule?.FileName ?? "";
					if (exePath.Length == 0) {
						throw new Exception("获取 Magpie 可执行文件路径失败");
					}

					ProcessStartInfo processInfo = new(exePath) {
						UseShellExecute = true,
						Verb = "runas",
						Arguments = string.Join(" ", e.Args)
					};
				
					_ = Process.Start(processInfo);
				} catch (Exception ex) {
					// 失败时以普通权限运行
					success = false;
					_ = MessageBox.Show(Magpie.Properties.Resources.Msg_Error_Run_As_Admin);
					Logger.Error(ex, "启动管理员权限进程失败");
				}

				if (success) {
					// 关闭当前进程
					Shutdown();
					return;
				}
			}

			// 不允许多个实例同时运行
			if (!mutex!.WaitOne(TimeSpan.Zero, true)) {
				Logger.Info("已有实例，即将退出");

				Current.Shutdown();

				if (!e.Args.Contains("-st")) {
					// 已存在实例，且命令行参数不含 -st 则唤起已有实例
					_ = NativeMethods.BroadcastMessage(NativeMethods.MAGPIE_WM_SHOWME);
				}

				mutex = null;
				return;
			}

			MainWindow window = new();
			MainWindow = window;
			window.Show();
		}

		private static bool IsRunAsAdministrator() {
			WindowsIdentity wi = WindowsIdentity.GetCurrent();
			WindowsPrincipal wp = new(wi);

			return wp.IsInRole(WindowsBuiltInRole.Administrator);
		}

		private void Application_Exit(object sender, ExitEventArgs e) {
			if (mutex != null) {
				Settings.Default.Save();

				mutex.ReleaseMutex();
			}

			Logger.Info($"程序关闭\n\t进程 ID：{Environment.ProcessId}");
		}
	}
}
