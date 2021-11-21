using Magpie.Properties;
using NLog;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;


namespace Magpie.Options {
	/// <summary>
	/// ScaleOptionsPage.xaml 的交互逻辑
	/// </summary>
	public partial class ScaleOptionsPage : Page {
		private static Logger Logger { get; } = LogManager.GetCurrentClassLogger();

		private static readonly float[] cursorZoomFactors = { 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, -1.0f };

		private static readonly string[] graphicsAdapters = NativeMethods.GetAllGraphicsAdapters();

		public ScaleOptionsPage() {
			InitializeComponent();

			if (NativeMethods.GetOSVersion() < new Version(10, 0, 22000)) {
				ckbDisableRoundCorner.Visibility = Visibility.Collapsed;
			}
			
			foreach (string adapter in graphicsAdapters) {
				cbbAdapter.Items.Add(adapter);
			}

			if (Settings.Default.AdapterIdx < 0 || Settings.Default.AdapterIdx >= graphicsAdapters.Length) {
				Settings.Default.AdapterIdx = 0;
			}

			cbbCursorZoomFactor.Items.Clear();
			for (int i = 0; i < cursorZoomFactors.Length - 1; ++i) {
				_ = cbbCursorZoomFactor.Items.Add(new ComboBoxItem {
					Content = cursorZoomFactors[i].ToString() + "x"
				});

				if (Math.Abs(Settings.Default.CursorZoomFactor - cursorZoomFactors[i]) < 1e-5) {
					cbbCursorZoomFactor.SelectedIndex = i;
				}
			}

			_ = cbbCursorZoomFactor.Items.Add(new ComboBoxItem {
				Content = Properties.Resources.UI_Options_Scale_Cursor_Same_As_Source_Window
			});

			if (Settings.Default.CursorZoomFactor <= 0) {
				cbbCursorZoomFactor.SelectedIndex = cursorZoomFactors.Length - 1;
			}

			if (cbbCursorZoomFactor.SelectedIndex < 0) {
				Settings.Default.CursorZoomFactor = 1.0f;
				cbbCursorZoomFactor.SelectedIndex = 2;
			}
		}

		private void BtnScale_Click(object sender, RoutedEventArgs e) {
			ProcessStartInfo psi = new ProcessStartInfo(App.SCALE_MODELS_JSON_PATH) {
				UseShellExecute = true
			};

			try {
				_ = Process.Start(psi);
			} catch (Win32Exception ex) {
				Logger.Warn(ex, "打开缩放配置失败，将尝试使用 Notepad 打开");

				psi.FileName = "notepad";
				psi.Arguments = App.SCALE_MODELS_JSON_PATH;
				try {
					_ = Process.Start(psi);
				} catch (Exception ex1) {
					Logger.Error(ex1, $"使用 Notepad 打开缩放配置失败\n\t缩放配置路径：{App.SCALE_MODELS_JSON_PATH}");
				}
			} catch (Exception ex) {
				Logger.Error(ex, $"打开缩放配置失败\n\t缩放配置路径：{App.SCALE_MODELS_JSON_PATH}");
			}
		}

		private void CbbCursorZoomFactor_SelectionChanged(object sender, SelectionChangedEventArgs e) {
			if (cbbCursorZoomFactor.SelectedIndex < 0 || cbbCursorZoomFactor.SelectedIndex >= cursorZoomFactors.Length) {
				return;
			}

			Settings.Default.CursorZoomFactor = cursorZoomFactors[cbbCursorZoomFactor.SelectedIndex];
		}
	}
}
