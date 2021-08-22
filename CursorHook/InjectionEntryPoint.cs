/*
 * 用于注入源窗口进程的钩子，支持运行时注入和启动时注入两种模式
 * 原理见 光标映射.md
 */

using System;
using System.IO;
using NLog;


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
			LogManager.Configuration = new NLog.Config.XmlLoggingConfiguration(configPath);
		}

		// 运行时注入的入口
		public InjectionEntryPoint(
			EasyHook.RemoteHooking.IContext _,
			string channelName,
			IntPtr hwndSrc
		) {
			Logger.Info("正在执行运行时注入");

			cursorHook = new RuntimeCursorHook(hwndSrc, new IpcServer(channelName));
		}

		// 启动时注入的入口
		public InjectionEntryPoint(EasyHook.RemoteHooking.IContext _, string channelName) {
			Logger.Info("正在执行启动时注入");

			cursorHook = new StartUpCursorHook(new IpcServer(channelName));
		}

		// 运行时注入逻辑的入口
		public void Run(EasyHook.RemoteHooking.IContext _1, string _2, IntPtr _3) {
			cursorHook.Run();
		}

		// 启动时注入逻辑的入口
		public void Run(EasyHook.RemoteHooking.IContext _1, string _2) {
			cursorHook.Run();
		}
	}
}
