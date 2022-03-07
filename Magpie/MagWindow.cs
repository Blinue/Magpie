using Magpie.Properties;
using System;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows;
using System.Windows.Interop;


namespace Magpie {
	// 用于管理全屏窗口，该窗口在一个新线程中启动
	internal class MagWindow : IDisposable {
		private static NLog.Logger Logger { get; } = NLog.LogManager.GetCurrentClassLogger();

		// 全屏窗口关闭时引发此事件
		public event Action? Closed;

		public IntPtr SrcWindow { get; private set; } = IntPtr.Zero;

		private readonly Thread magThread;

		// 用于指示 magThread 进入全屏
		private readonly AutoResetEvent runEvent = new(false);

		private enum MagWindowCmd {
			None,
			Run,
			Exit,
			SetLogLevel
		}

		// 传递给 magThread 的参数
		private class MagWindowParams {
			public volatile IntPtr hwndSrc;
			public volatile string effectsJson = "";
			public volatile int logLevel;
			public volatile MagWindowCmd cmd = MagWindowCmd.None;
		}

		private enum FlagMasks : uint {
			NoCursor = 0x1,
			AdjustCursorSpeed = 0x2,
			SaveEffectSources = 0x4,
			SimulateExclusiveFullscreen = 0x8,
			DisableLowLatency = 0x10,
			BreakpointMode = 0x20,
			DisableWindowResizing = 0x40,
			DisableDirectFlip = 0x80,
			ConfineCursorIn3DGames = 0x100,
			CropTitleBarOfUWP = 0x200,
			DisableEffectCache = 0x400,
			DisableVSync = 0x800,
			WarningsAreErrors = 0x1000
		}

		private readonly MagWindowParams magWindowParams = new();

		// 用于从全屏窗口的线程接收消息
		private event Action<string?> CloseEvent;

		public bool Running { get; private set; }

		public MagWindow(Window parent) {
			// 使用 WinRT Capturer API 需要在 MTA 中
			// C# 窗体必须使用 STA，因此将全屏窗口创建在新的线程里
			magThread = new Thread(() => {
				Environment.CurrentDirectory = AppDomain.CurrentDomain.BaseDirectory;

				string logFileName = App.LOGS_FOLDER + "Runtime.log";
				const int logArchiveAboveSize = 100000;
				const int logMaxArchiveFiles = 1;

				static uint ResolveLogLevel(uint logLevel) {
					return logLevel switch {
						1 => 2,
						2 => 3,
						3 => 4,
						_ => 6,
					};
				}

				bool initSuccess = false;
				try {
					initSuccess = NativeMethods.Initialize(
						ResolveLogLevel(Settings.Default.LoggingLevel),
						logFileName,
						logArchiveAboveSize,
						logMaxArchiveFiles
					);
				} catch (DllNotFoundException e) {
					// 解决某些 DllImport 失败的问题
					Logger.Warn(e, "未找到 Runtime.dll");

					// 显式加载 Runtime.dll，而不是通过 DllImport
					if (NativeMethods.LoadLibrary("Runtime.dll") == IntPtr.Zero) {
						Logger.Warn($"LoadLibrary 失败\n\tLastErrorCode={Marshal.GetLastWin32Error()}");
					}

					// 再次尝试
					try {
						initSuccess = NativeMethods.Initialize(
							ResolveLogLevel(Settings.Default.LoggingLevel),
							logFileName,
							logArchiveAboveSize,
							logMaxArchiveFiles
						);
					} catch (Exception e1) {
						Logger.Error(e1, "Initialize 失败");
					}
				} catch (Exception e) {
					Logger.Error(e, "Initialize 失败");
				}

				if (!initSuccess) {
					// 初始化失败
					Logger.Fatal("初始化 Runtime 失败");
					CloseEvent?.Invoke("Msg_Error_Init");
					parent.Dispatcher.Invoke(() => {
						parent.Close();
					});
					return;
				}

				Logger.Info("初始化 Runtime 成功");

				while (magWindowParams.cmd != MagWindowCmd.Exit) {
					_ = runEvent.WaitOne(1000);

					MagWindowCmd cmd = magWindowParams.cmd;
					magWindowParams.cmd = MagWindowCmd.None;

					if (cmd == MagWindowCmd.Exit) {
						break;
					}

					if(cmd == MagWindowCmd.None) {
						continue;
					}

					if (cmd == MagWindowCmd.SetLogLevel) {
						NativeMethods.SetLogLevel(ResolveLogLevel((uint)magWindowParams.logLevel));
					} else {
						uint flags = (Settings.Default.NoCursor ? (uint)FlagMasks.NoCursor : 0) |
							(Settings.Default.AdjustCursorSpeed ? (uint)FlagMasks.AdjustCursorSpeed : 0) |
							(Settings.Default.DebugSaveEffectSources ? (uint)FlagMasks.SaveEffectSources : 0) |
							(Settings.Default.DisableLowLatency ? (uint)FlagMasks.DisableLowLatency : 0) |
							(Settings.Default.DebugBreakpointMode ? (uint)FlagMasks.BreakpointMode : 0) |
							(Settings.Default.DisableWindowResizing ? (uint)FlagMasks.DisableWindowResizing : 0) |
							(Settings.Default.DisableDirectFlip ? (uint)FlagMasks.DisableDirectFlip : 0) |
							(Settings.Default.ConfineCursorIn3DGames ? (uint)FlagMasks.ConfineCursorIn3DGames : 0) |
							(Settings.Default.CropTitleBarOfUWP ? (uint)FlagMasks.CropTitleBarOfUWP : 0) |
							(Settings.Default.DebugDisableEffectCache ? (uint)FlagMasks.DisableEffectCache : 0) |
							(Settings.Default.SimulateExclusiveFullscreen ? (uint)FlagMasks.SimulateExclusiveFullscreen : 0) |
							(Settings.Default.DebugWarningsAreErrors ? (uint)FlagMasks.WarningsAreErrors : 0) |
							(Settings.Default.VSync ? 0 : (uint)FlagMasks.DisableVSync);

						bool customCropping = Settings.Default.CustomCropping;

						string? msg = NativeMethods.Run(
							magWindowParams.hwndSrc,
							magWindowParams.effectsJson,
							flags,
							Settings.Default.CaptureMode,
							Settings.Default.CursorZoomFactor,
							Settings.Default.CursorInterpolationMode,
							Settings.Default.AdapterIdx,
							Settings.Default.MultiMonitorUsage,
							customCropping ? Settings.Default.CropLeft : 0,
							customCropping ? Settings.Default.CropTop : 0,
							customCropping ? Settings.Default.CropRight : 0,
							customCropping ? Settings.Default.CropBottom : 0
						);

						CloseEvent?.Invoke(msg);
					}
				}
			});

			magThread.SetApartmentState(ApartmentState.MTA);
			magThread.Start();

			CloseEvent += (string? errorMsgId) => {
				bool noError = string.IsNullOrEmpty(errorMsgId);

				if (noError && Closed != null) {
					Closed.Invoke();
				}
				SrcWindow = IntPtr.Zero;
				Running = false;

				if (!noError) {
					parent.Dispatcher.Invoke(new Action(() => {
						_ = NativeMethods.SetForegroundWindow(new WindowInteropHelper(parent).Handle);

						string? errorMsg = Resources.ResourceManager.GetString(errorMsgId!, Resources.Culture);
						if (errorMsg == null) {
							errorMsg = Resources.ResourceManager.GetString(Resources.Msg_Error_Generic);
						}
						_ = MessageBox.Show(errorMsg, Resources.Msg_Error_Caption);
					}));
				}
			};
		}

		public void Create(string effectsJson) {
			if (Running) {
				Logger.Info("已存在全屏窗口，取消进入全屏");
				return;
			}

			IntPtr hwndSrc = NativeMethods.GetForegroundWindow();
			if (!NativeMethods.IsWindow(hwndSrc)
				|| !NativeMethods.IsWindowVisible(hwndSrc)
				|| NativeMethods.GetWindowShowCmd(hwndSrc) != NativeMethods.SW_NORMAL
			) {
				Logger.Warn("源窗口不合法");
				return;
			}

			SrcWindow = hwndSrc;

			magWindowParams.cmd = MagWindowCmd.Run;
			magWindowParams.hwndSrc = hwndSrc;
			magWindowParams.effectsJson = effectsJson;

			_ = runEvent.Set();
			Running = true;
		}

		public void SetLogLevel(uint logLevel) {
			magWindowParams.cmd = MagWindowCmd.SetLogLevel;
			magWindowParams.logLevel = (int)logLevel;

			_ = runEvent.Set();
		}

		public void Destory() {
			if (!Running) {
				return;
			}

			// 广播 MAGPIE_WM_DESTORYMAG
			// 可以在没有全屏窗口句柄的情况下关闭它
			_ = NativeMethods.BroadcastMessage(NativeMethods.MAGPIE_WM_DESTORYHOST);
		}

		private bool disposed = false;

		public void Dispose() {
			if (disposed) {
				return;
			}
			disposed = true;

			magWindowParams.cmd = MagWindowCmd.Exit;

			if (Running) {
				Destory();

				while (Running) {
					Thread.Sleep(1);
				}
			} else {
				_ = runEvent.Set();
				Thread.Sleep(1);
			}

			runEvent.Dispose();
		}
	}
}
