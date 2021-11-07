using Magpie.Properties;
using System;
using System.Threading;
using System.Windows;
using System.Windows.Interop;


namespace Magpie {
	// 用于管理全屏窗口，该窗口在一个新线程中启动
	internal class MagWindow : IDisposable {
		private static NLog.Logger Logger { get; } = NLog.LogManager.GetCurrentClassLogger();

		// 全屏窗口关闭时引发此事件
		public event Action Closed;

		public IntPtr SrcWindow { get; private set; } = IntPtr.Zero;

		private readonly Thread magThread = null;

		// 用于指示 magThread 进入全屏
		private readonly AutoResetEvent runEvent = new AutoResetEvent(false);

		private enum MagWindowCmd {
			Run,
			Exit,
			SetLogLevel
		}

		// 传递给 magThread 的参数
		private class MagWindowParams {
			public volatile IntPtr hwndSrc;
			public volatile int captureMode;
			public volatile string effectsJson;
			public volatile bool noCursor;
			public volatile bool adjustCursorSpeed;
			public volatile bool showFPS;
			public volatile bool disableRoundCorner;
			public volatile int frameRateOrLogLevel;
			public volatile bool disableLowLatency;
			public volatile bool breakpointMode;
			public volatile MagWindowCmd cmd = MagWindowCmd.Run;
		}

		private readonly MagWindowParams magWindowParams = new MagWindowParams();

		// 用于从全屏窗口的线程接收消息
		private event Action<string> CloseEvent;

		public bool Running { get; private set; }

		public MagWindow(Window parent) {
			// 使用 WinRT Capturer API 需要在 MTA 中
			// C# 窗体必须使用 STA，因此将全屏窗口创建在新的线程里
			magThread = new Thread(() => {
				Logger.Info("正在新线程中创建全屏窗口");

				int ResolveLogLevel(int logLevel) {
					switch (logLevel) {
						case 1:
							return 2;
						case 2:
							return 3;
						case 3:
							return 4;
						default:
							return 6;
					}
				}

				bool initSuccess = false;
				try {
					initSuccess = NativeMethods.Initialize(ResolveLogLevel(Settings.Default.LoggingLevel));
					if (!initSuccess) {
						Logger.Error("Initialize 失败");
					}
				} catch (Exception e) {
					Logger.Error(e, "Initialize 失败");
				}

				if (!initSuccess) {
					// 初始化失败
					CloseEvent("Msg_Error_Init");
					parent.Dispatcher.Invoke(() => {
						parent.Close();
					});
					return;
				}

				while (magWindowParams.cmd != MagWindowCmd.Exit) {
					_ = runEvent.WaitOne();

					if (magWindowParams.cmd == MagWindowCmd.Exit) {
						break;
					}

					if (magWindowParams.cmd == MagWindowCmd.SetLogLevel) {
						NativeMethods.SetLogLevel(ResolveLogLevel(magWindowParams.frameRateOrLogLevel));
					} else {
						string msg = NativeMethods.Run(
							magWindowParams.hwndSrc,
							magWindowParams.effectsJson,
							magWindowParams.captureMode,
							magWindowParams.noCursor,
							magWindowParams.adjustCursorSpeed,
							magWindowParams.showFPS,
							magWindowParams.disableRoundCorner,
							magWindowParams.frameRateOrLogLevel,
							magWindowParams.disableLowLatency,
							magWindowParams.breakpointMode
						);

						CloseEvent(msg);
					}
				}
			});

			magThread.SetApartmentState(ApartmentState.MTA);
			magThread.Start();

			CloseEvent += (string errorMsgId) => {
				bool noError = string.IsNullOrEmpty(errorMsgId);

				if (noError && Closed != null) {
					Closed.Invoke();
				}
				SrcWindow = IntPtr.Zero;
				Running = false;

				if (!noError) {
					parent.Dispatcher.Invoke(new Action(() => {
						_ = NativeMethods.SetForegroundWindow(new WindowInteropHelper(parent).Handle);

						string errorMsg = Resources.ResourceManager.GetString(errorMsgId, Resources.Culture);
						if (errorMsg == null) {
							errorMsg = Resources.ResourceManager.GetString(Resources.Msg_Error_Generic);
						}
						_ = MessageBox.Show(errorMsg);
					}));
				}
			};
		}

		public void Create(
			string effectsJson,
			int captureMode,
			bool showFPS,
			bool noCursor,
			bool adjustCursorSpeed,
			bool disableRoundCorner,
			int frameRate,
			bool disableLowLatency,
			bool breakpointMode
		) {
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
			magWindowParams.captureMode = captureMode;
			magWindowParams.effectsJson = effectsJson;
			magWindowParams.showFPS = showFPS;
			magWindowParams.noCursor = noCursor;
			magWindowParams.adjustCursorSpeed = adjustCursorSpeed;
			magWindowParams.disableRoundCorner = disableRoundCorner;
			magWindowParams.frameRateOrLogLevel = frameRate;
			magWindowParams.disableLowLatency = disableLowLatency;
			magWindowParams.breakpointMode = breakpointMode;

			_ = runEvent.Set();
			Running = true;
		}

		public void SetLogLevel(int logLevel) {
			magWindowParams.cmd = MagWindowCmd.SetLogLevel;
			magWindowParams.frameRateOrLogLevel = logLevel;

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
