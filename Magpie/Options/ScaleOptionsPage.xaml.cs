using Magpie.Properties;
using NLog;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using System.IO;
using System.Windows.Forms;

namespace Magpie.Options {
	/// <summary>
	/// ScaleOptionsPage.xaml 的交互逻辑
	/// </summary>
	public partial class ScaleOptionsPage : Page {
		private static Logger Logger { get; } = LogManager.GetCurrentClassLogger();

		private static readonly float[] cursorZoomFactors = { 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f, 2.5f, 3.0f, -1.0f };

		private static readonly string[] graphicsAdapters = NativeMethods.GetAllGraphicsAdapters();

		public ScaleOptionsPage() {
			InitializeComponent();

			// 图形适配器
			cbbAdapter.Items.Add(Properties.Resources.UI_Options_Scale_Adapter_Default);
			foreach (string adapter in graphicsAdapters) {
				cbbAdapter.Items.Add(adapter);
			}

			if (Settings.Default.AdapterIdx < -1 || Settings.Default.AdapterIdx >= graphicsAdapters.Length - 1) {
				Settings.Default.AdapterIdx = -1;
			}
			cbbAdapter.SelectedIndex = Settings.Default.AdapterIdx + 1;

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

			spMutliMonitorUsage.Visibility = Screen.AllScreens.Length > 1 ? Visibility.Visible : Visibility.Collapsed;
		}

		private void BtnOpenScaleConfig_Click(object sender, RoutedEventArgs e) {
			if (!File.Exists(App.SCALE_MODELS_JSON_PATH)) {
				Logger.Error("ScaleModels.json 不存在");
				Debug.Assert(false);
				return;
			}

			ProcessStartInfo psi = new(App.SCALE_MODELS_JSON_PATH) {
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

		private void CbbAdapter_SelectionChanged(object sender, SelectionChangedEventArgs e) {
			Settings.Default.AdapterIdx = cbbAdapter.SelectedIndex - 1;
		}
	}
}
