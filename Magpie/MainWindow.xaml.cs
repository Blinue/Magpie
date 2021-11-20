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


using Gma.System.MouseKeyHook;
using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using System.Windows.Threading;
using System.Windows.Media;
using System.Windows.Forms;
using Magpie.Properties;
using Magpie.Options;
using System.Windows.Media.Imaging;
using System.Linq;


namespace Magpie {
	/// <summary>
	/// MainWindow.xaml 的交互逻辑
	/// </summary>
	public partial class MainWindow : Window {
		private static NLog.Logger Logger { get; } = NLog.LogManager.GetCurrentClassLogger();

		private OptionsWindow optionsWindow = null;
		private readonly DispatcherTimer timerScale = new DispatcherTimer {
			Interval = new TimeSpan(0, 0, 1)
		};

		private IKeyboardMouseEvents keyboardEvents = null;
		private readonly NotifyIcon notifyIcon = new NotifyIcon();
		private ToolStripItem tsiHotkey;
		private ToolStripItem tsiScale;
		private MagWindow magWindow = null;

		private readonly ScaleModelManager scaleModelManager = new ScaleModelManager();

		// 倒计时时间
		private const int DOWN_COUNT = 5;
		private int countDownNum;

		private IntPtr Handle;

		// 不为零时表示全屏窗口不是因为Hotkey关闭的
		private IntPtr prevSrcWindow = IntPtr.Zero;
		private readonly DispatcherTimer timerRestore = new DispatcherTimer {
			Interval = new TimeSpan(0, 0, 0, 0, 300)
		};

		// 每秒检查一次系统主题
		private readonly DispatcherTimer timerCheckOSTheme = new DispatcherTimer {
			Interval = new TimeSpan(0, 0, 0, 1, 0),
		};

		private void Application_Closing() {
			magWindow.Dispose();

			if (optionsWindow != null) {
				optionsWindow.Close();
			}
		}

		public MainWindow() {
			InitializeComponent();

			InitNotifyIcon();

			CheckOSTheme();
			timerCheckOSTheme.Tick += (sender, e) => {
				CheckOSTheme();
			};
			timerCheckOSTheme.Start();

			BindScaleModels();
			scaleModelManager.ScaleModelsChanged += BindScaleModels;

			timerScale.Tick += TimerScale_Tick;
			timerRestore.Tick += TimerRestore_Tick;

			// 加载设置
			txtHotkey.Text = Settings.Default.Hotkey;

			if (Settings.Default.ScaleMode >= cbbScaleMode.Items.Count) {
				Settings.Default.ScaleMode = 0;
			}
			cbbScaleMode.SelectedIndex = (int)Settings.Default.ScaleMode;

			ShowAllCaptureMethods(Settings.Default.DebugShowAllCaptureMethods);

			// 延迟绑定，防止加载时改变设置
			cbbScaleMode.SelectionChanged += CbbScaleMode_SelectionChanged;
		}

		void InitNotifyIcon() {
			notifyIcon.Visible = false;
			
			notifyIcon.Text = Title;
			notifyIcon.MouseClick += NotifyIcon_MouseClick;

			ContextMenuStrip menu = new ContextMenuStrip();

			tsiHotkey = menu.Items.Add(Settings.Default.Hotkey, null);
			tsiHotkey.Enabled = false;

			tsiScale = menu.Items.Add(Properties.Resources.UI_SysTray_Scale_After_5S, null, (sender, e) => {
				ToggleScaleTimer();
			});

			menu.Items.Add(Properties.Resources.UI_SysTray_Main_Window, null, (sender, e) => {
				Show();
				WindowState = WindowState.Normal;
			});
			menu.Items.Add(Properties.Resources.UI_SysTray_Options, null, (sender, e) => {
				BtnOptions_Click(sender, new RoutedEventArgs());
			});
			menu.Items.Add(Properties.Resources.UI_SysTray_Exit, null, (sender, e) => {
				Application_Closing();
				Closing -= Window_Closing;
				Close();
			});
			notifyIcon.ContextMenuStrip = menu;
		}

		private void NotifyIcon_MouseClick(object sender, MouseEventArgs e) {
			if (e.Button != MouseButtons.Left) {
				return;
			}

			Show();
			WindowState = WindowState.Normal;
		}

		private void BindScaleModels() {
			int oldIdx = cbbScaleMode.SelectedIndex;
			cbbScaleMode.Items.Clear();

			ScaleModelManager.ScaleModel[] scaleModels = scaleModelManager.GetScaleModels();
			if (scaleModels == null || scaleModels.Length == 0) {
				_ = cbbScaleMode.Items.Add($"<{Properties.Resources.UI_Main_Parse_Failure}>");
				cbbScaleMode.SelectedIndex = 0;
			} else {
				foreach (ScaleModelManager.ScaleModel m in scaleModels) {
					_ = cbbScaleMode.Items.Add(m.Name);
				}

				if (oldIdx < 0 || oldIdx >= scaleModels.Length) {
					oldIdx = 0;
				}
				cbbScaleMode.SelectedIndex = oldIdx;
			}
		}

		private void TimerRestore_Tick(object sender, EventArgs e) {
			if (!Settings.Default.AutoRestore || !NativeMethods.IsWindow(prevSrcWindow)) {
				StopWaitingForRestore();
				return;
			}

			if (NativeMethods.GetForegroundWindow() == prevSrcWindow) {
				ToggleMagWindow();
				return;
			}

			tbCurWndTitle.Text = $"{Properties.Resources.UI_Main_Current_Window}{NativeMethods.GetWindowTitle(prevSrcWindow)}";
			if (WindowState == WindowState.Normal) {
				btnForgetCurrentWnd.Visibility = gridCurWnd.Visibility = Visibility.Visible;
			}
		}

		private void TimerScale_Tick(object sender, EventArgs e) {
			if (--countDownNum != 0) {
				btnScale.Content = tsiScale.Text = countDownNum.ToString();
				return;
			}

			// 计时结束
			StopScaleTimer();

			ToggleMagWindow();
		}

		private void BtnOptions_Click(object sender, RoutedEventArgs e) {
			if (optionsWindow == null) {
				optionsWindow = new OptionsWindow();
				optionsWindow.Closed += (object _, EventArgs _1) => {
					optionsWindow = null;
				};
			}

			optionsWindow.Show();
			_ = optionsWindow.Focus();
		}

		private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e) {
			if (Settings.Default.MinimizeWhenClose) {
				WindowState = WindowState.Minimized;
				e.Cancel = true;
			} else {
				Application_Closing();
			}
		}

		private void TxtHotkey_TextChanged(object sender, TextChangedEventArgs e) {
			keyboardEvents?.Dispose();
			keyboardEvents = Hook.GlobalEvents();

			string hotkey = txtHotkey.Text.Trim();

			try {
				keyboardEvents.OnCombination(new Dictionary<Combination, Action> {{
					Combination.FromString(hotkey), () => ToggleMagWindow()
				}});

				txtHotkey.Foreground = Brushes.Black;
				Settings.Default.Hotkey = hotkey;

				tsiHotkey.Text = hotkey;

				Logger.Info($"当前热键：{txtHotkey.Text}");
			} catch (ArgumentException ex) {
				Logger.Error(ex, $"解析快捷键失败：{txtHotkey.Text}");
				txtHotkey.Foreground = Brushes.Red;
			}
		}

		private void ToggleMagWindow() {
			if (Settings.Default.AutoRestore) {
				StopWaitingForRestore();
				// 立即更新布局，因为窗口大小可能改变，如果接下来放大 Magpie 本身会立即退出
				UpdateLayout();
			}

			if (!scaleModelManager.IsValid()) {
				return;
			}

			if (magWindow.Running) {
				Logger.Info("通过热键退出全屏");
				magWindow.Destory();
				return;
			}

			string effectsJson = scaleModelManager.GetScaleModels()[Settings.Default.ScaleMode].Effects;

			int frameRate = 0;
			switch (Settings.Default.FrameRateType) {
				case 1:
					// 不限帧率
					frameRate = -1;
					break;
				case 2:
					// 限制帧率
					frameRate = (int)Settings.Default.FrameRateLimit;
					break;
				default:
					// 垂直同步
					break;
			}

			magWindow.Create(
				effectsJson,
				Settings.Default.CaptureMode,
				frameRate,
				Settings.Default.CursorZoomFactor,
				Settings.Default.CursorInterpolationMode,
				Settings.Default.AdapterIdx,
				Settings.Default.ShowFPS,
				Settings.Default.NoCursor,
				Settings.Default.AdjustCursorSpeed,
				Settings.Default.DisableRoundCorner,
				Settings.Default.DisableWindowResizing,
				Settings.Default.DisableLowLatency,
				Settings.Default.DebugBreakpointMode,
				Settings.Default.DisableDirectFlip,
				Settings.Default.ConfineCursorIn3DGames
			);

			prevSrcWindow = magWindow.SrcWindow;
		}

		private void Window_SourceInitialized(object sender, EventArgs e) {
			Handle = new WindowInteropHelper(this).Handle;

			HwndSource source = HwndSource.FromHwnd(Handle);
			source.AddHook(WndProc);

			magWindow = new MagWindow(this);
			magWindow.Closed += MagWindow_Closed;

			// 检查命令行参数
			string[] args = Environment.GetCommandLineArgs();
			foreach (string arg in args) {
				if (arg == "-st") {
					// 启动到系统托盘
					Logger.Info("已指定启动时缩放到系统托盘");
					WindowState = WindowState.Minimized;
					break;
				}
			}
		}

		private void StopWaitingForRestore() {
			if (WindowState == WindowState.Normal) {
				btnForgetCurrentWnd.Visibility = gridCurWnd.Visibility = Visibility.Collapsed;
			}
			tbCurWndTitle.Text = "";
			prevSrcWindow = IntPtr.Zero;
			timerRestore.Stop();
		}

		private void MagWindow_Closed() {
			// 不监视 Magpie 主窗口
			if (!Settings.Default.AutoRestore || prevSrcWindow == Handle) {
				return;
			}

			Dispatcher.Invoke(() => {
				if (NativeMethods.IsWindow(prevSrcWindow)) {
					Logger.Info("正在监视源窗口是否为前台窗口");
					timerRestore.Start();
				} else {
					Logger.Info("停止监视源窗口是否为前台窗口");
					StopWaitingForRestore();
				}
			});
		}

		private IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled) {
			if (msg == NativeMethods.MAGPIE_WM_SHOWME) {
				Logger.Info("收到 WM_SHOWME 消息");

				// 收到 WM_SHOWME 激活窗口
				if (WindowState == WindowState.Minimized) {
					Show();
					WindowState = WindowState.Normal;
				}

				_ = NativeMethods.SetForegroundWindow(Handle);
				handled = true;
			}
			return IntPtr.Zero;
		}

		private void CbbScaleMode_SelectionChanged(object sender, SelectionChangedEventArgs e) {
			Settings.Default.ScaleMode = (uint)cbbScaleMode.SelectedIndex;
		}

		private void StartScaleTimer() {
			countDownNum = DOWN_COUNT;
			btnScale.Content = tsiScale.Text = countDownNum.ToString();

			timerScale.Start();
		}

		private void StopScaleTimer() {
			timerScale.Stop();
			
			btnScale.Content = Properties.Resources.UI_Main_Scale_After_5S;
			tsiScale.Text = Properties.Resources.UI_SysTray_Scale_After_5S;
		}

		private void ToggleScaleTimer() {
			if (timerScale.IsEnabled) {
				StopScaleTimer();
			} else {
				StartScaleTimer();
			}
		}

		private void Window_StateChanged(object sender, EventArgs e) {
			if (WindowState == WindowState.Minimized) {
				Hide();
				notifyIcon.Visible = true;
			} else {
				notifyIcon.Visible = false;
				btnForgetCurrentWnd.Visibility = gridCurWnd.Visibility = timerRestore.IsEnabled ? Visibility.Visible : Visibility.Collapsed;
			}
		}

		private void BtnScale_Click(object sender, RoutedEventArgs e) {
			ToggleScaleTimer();
		}

		private void BtnCancelRestore_Click(object sender, RoutedEventArgs e) {
			StopWaitingForRestore();
		}

		public void SetRuntimeLogLevel(uint logLevel) {
			magWindow.SetLogLevel(logLevel);
		}

		private void Window_Deactivated(object sender, EventArgs e) {
			Settings.Default.Save();
		}

		public void ShowAllCaptureMethods(bool isShow) {
			if (isShow) {
				if (cbbCaptureMethod.Items.Count != 3) {
					return;
				}

				_ = cbbCaptureMethod.Items.Add(new ComboBoxItem {
					Content = "Legacy GDI"
				});
				_ = cbbCaptureMethod.Items.Add(new ComboBoxItem {
					Content = "MagCallback"
				});
			} else {
				if (cbbCaptureMethod.Items.Count != 5) {
					return;
				}

				if (cbbCaptureMethod.SelectedIndex >= 3) {
					cbbCaptureMethod.SelectedIndex = 0;
				}
				cbbCaptureMethod.Items.RemoveAt(4);
				cbbCaptureMethod.Items.RemoveAt(3);
			}
		}

		private bool? isLightTheme = null;

		private void CheckOSTheme() {
			// 检查注册表获取系统主题，迁移到新版本的 .NET 后将使用 WinRT
			// 较老版本的 Windows 无该注册表项，始终使用黑色图标
			bool newTheme = true;

			IntPtr hKey = IntPtr.Zero;
			if (NativeMethods.RegOpenKeyEx(
				NativeMethods.HKEY_CURRENT_USER,
				"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
				0,
				NativeMethods.KEY_READ,
				ref hKey
			) == 0) {
				int dataSize = 4;
				byte[] data = new byte[dataSize];
				if (NativeMethods.RegQueryValueEx(
					hKey,
					"SystemUsesLightTheme",
					IntPtr.Zero,
					IntPtr.Zero,
					data,
					ref dataSize
				) == 0) {
					newTheme = data.Any(b => { return b != 0; });
				}

				NativeMethods.RegCloseKey(hKey);
			}

			if (!isLightTheme.HasValue || isLightTheme != newTheme) {
				isLightTheme = newTheme;
				Uri logoUri = new Uri($"pack://application:,,,/Magpie;component/Resources/Logo_{(isLightTheme.Value ? "Black" : "White")}.ico");

				notifyIcon.Icon = new System.Drawing.Icon(App.GetResourceStream(logoUri).Stream);
				System.Windows.Application.Current.Resources["Logo"] = new BitmapImage(logoUri);
			}
		}
	}
}
