/*
 * 用于注入源窗口进程的钩子，支持运行时注入和启动时注入两种模式
 * 原理见 光标映射.md
 */

using System;
using System.IO;
using NLog;
using NLog.Config;

namespace Magpie.CursorHook {
	/// <summary>
	/// 注入时 EasyHook 会寻找 <see cref="EasyHook.IEntryPoint"/> 的实现。
	/// 注入后此类将成为入口
	/// </summary>
	public class InjectionEntryPoint : EasyHook.IEntryPoint {
		private static Logger Logger { get; } = LogManager.GetCurrentClassLogger();

		private readonly CursorHookBase cursorHook = null;

		static InjectionEntryPoint() {
			string configPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "CursorHook.dll.config");
			LogManager.Configuration = new XmlLoggingConfiguration(configPath);
		}

		public InjectionEntryPoint(int logLevel) {
			UpdateLoggingLevel(logLevel);

			AppDomainSetup info = AppDomain.CurrentDomain.SetupInformation;
			Logger.Info($"正在初始化 CursorHook\n\t程序名：{info.ApplicationName}\n\t架构：{(EasyHook.NativeAPI.Is64Bit ? "x64" : "x86")}");
		}

		// 运行时注入的入口
		public InjectionEntryPoint(EasyHook.RemoteHooking.IContext _, int logLevel, IntPtr hwndSrc) : this(logLevel) {
			Logger.Info($"源窗口句柄：{hwndSrc}");
			cursorHook = new RuntimeCursorHook(hwndSrc);
		}

		// 启动时注入的入口
		public InjectionEntryPoint(EasyHook.RemoteHooking.IContext _, int logLevel) : this(logLevel) {
			cursorHook = new StartUpCursorHook();
		}

		// 运行时注入逻辑的入口
		public void Run(EasyHook.RemoteHooking.IContext _1, int _2, IntPtr _3) {
			Logger.Info("正在执行运行时注入");

			cursorHook.Run();
		}

		// 启动时注入逻辑的入口
		public void Run(EasyHook.RemoteHooking.IContext _1, int _2) {
			Logger.Info("正在执行启动时注入");

			cursorHook.Run();
		}

		private static void UpdateLoggingLevel(int logLevel) {
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

			Logger.Info($"日志级别变更为 {minLogLevel}");
		}
	}
}
