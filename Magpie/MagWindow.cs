using System;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows;
using System.Windows.Interop;


namespace Magpie {
	internal enum MagWindowStatus : int {
		Idle = 0,       // 未启动或者已关闭
		Starting = 1,   // 启动中，此状态下无法执行操作
		Running = 2     // 运行中
	}

	// 用于管理全屏窗口，该窗口在一个新线程中启动，通过事件与主线程通信
	internal class MagWindow {
		private static NLog.Logger Logger { get; } = NLog.LogManager.GetCurrentClassLogger();

		public event Action Closed;

		public IntPtr SrcWindow { get; private set; } = IntPtr.Zero;

		private Thread magThread = null;

		// 用于从全屏窗口的线程接收消息
		private event Action<int, string> StatusEvent;


		private MagWindowStatus status = MagWindowStatus.Idle;
		public MagWindowStatus Status {
			get => status;
			private set {
				status = value;
				if (status == MagWindowStatus.Idle) {
					magThread = null;
				}
			}
		}

		public MagWindow(Window parent) {
			StatusEvent += (int status, string errorMsgId) => {
				if (status < 0 || status > 3) {
					return;
				}
				bool noError = string.IsNullOrEmpty(errorMsgId);

				MagWindowStatus status_ = (MagWindowStatus)status;
				if (status_ == Status) {
					return;
				}

				if (status_ == MagWindowStatus.Idle) {
					if (noError && Closed != null) {
						Closed.Invoke();
					}
					SrcWindow = IntPtr.Zero;
				}
				Status = status_;

				if (!noError) {
					parent.Dispatcher.Invoke(new Action(() => {
						_ = NativeMethods.SetForegroundWindow(new WindowInteropHelper(parent).Handle);

						/*string errorMsg = Resources.ResourceManager.GetString(errorMsgId, Resources.Culture);
						if (errorMsg == null) {
							errorMsg = Resources.Msg_Error_Generic;
						}*/
						_ = MessageBox.Show(errorMsgId);
					}));
				}
			};
		}

		public void Create(
			string scaleModel,
			int captureMode,
			int bufferPrecision,
			bool showFPS,
			bool adjustCursorSpeed,
			bool hookCursorAtRuntime,
			bool noDisturb = false
		) {
			if (Status != MagWindowStatus.Idle) {
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

			Status = MagWindowStatus.Starting;
			// 使用 WinRT Capturer API 需要在 MTA 中
			// C# 窗体必须使用 STA，因此将全屏窗口创建在新的线程里
			magThread = new Thread(() => {
				Logger.Info("正在新线程中创建全屏窗口");

				NativeMethods.Run(
					(int status, IntPtr errorMsg) => StatusEvent(status, Marshal.PtrToStringUni(errorMsg)),
					hwndSrc
				);
			});
			magThread.SetApartmentState(ApartmentState.MTA);
			magThread.Start();

			SrcWindow = hwndSrc;
		}

		public void Destory() {
			if (Status != MagWindowStatus.Running) {
				return;
			}

			// 广播 MAGPIE_WM_DESTORYMAG
			// 可以在没有全屏窗口句柄的情况下关闭它
			_ = NativeMethods.BroadcastMessage(NativeMethods.MAGPIE_WM_DESTORYHOST);
		}
	}
}
