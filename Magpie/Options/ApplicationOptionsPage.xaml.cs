using Magpie.Properties;
using System;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Windows;
using System.Windows.Controls;


namespace Magpie.Options {
	/// <summary>
	/// ApplicationOptionsPage.xaml 的交互逻辑
	/// </summary>
	public partial class ApplicationOptionsPage : Page {
		private readonly IWshRuntimeLibrary.WshShell shell = new IWshRuntimeLibrary.WshShell();
		private readonly IWshRuntimeLibrary.IWshShortcut shortcut = null;
		private readonly string pathLink;

		private readonly CultureInfo[] supportedCultures = {
			CultureInfo.GetCultureInfo("en-US"),
			CultureInfo.GetCultureInfo("zh-CN"),
			CultureInfo.GetCultureInfo("ru-RU")
		};

		private static string originCulture = null;
		private static bool originRunAsAdmin = false;

		public ApplicationOptionsPage() {
			InitializeComponent();

			if (originCulture == null) {
				originCulture = Settings.Default.CultureName;
				originRunAsAdmin = Settings.Default.RunAsAdmin;
			}

			// 第一项为系统默认语言
			_ = cbbLanguage.Items.Add(Properties.Resources.UI_Options_Application_Language_Default);
			foreach (CultureInfo cultureInfo in supportedCultures) {
				_ = cbbLanguage.Items.Add(cultureInfo.NativeName);
			}

			cbbLanguage.SelectionChanged += CbbLanguage_SelectionChanged;

			if (string.IsNullOrEmpty(Settings.Default.CultureName)) {
				cbbLanguage.SelectedIndex = 0;
			} else {
				int idx = Array.FindIndex(supportedCultures, culture => {
					return culture.Name == Settings.Default.CultureName;
				});
				if (idx == -1) {
					// 未找到，重置为默认
					Settings.Default.CultureName = "";
					// idx 为 -1
				}
				cbbLanguage.SelectedIndex = idx + 1;
			}

			string startUpFolder = Environment.GetFolderPath(Environment.SpecialFolder.Startup);
			pathLink = startUpFolder + "\\Magpie.lnk";

			shortcut = (IWshRuntimeLibrary.IWshShortcut)shell.CreateShortcut(pathLink);

			if (File.Exists(pathLink)) {
				// 开机启动的选项取决于快捷方式是否存在
				// 最小化的选项取决于设置
				ckbRunAtStartUp.IsChecked = true;

				if (Settings.Default.MinimizeAtWindowsStartUp != (shortcut.Arguments == "-st")) {
					// 设置和实际不一致，重新创建快捷方式
					CreateShortCut();
				}
			}

			shortcut.TargetPath = Assembly.GetExecutingAssembly().Location;
			shortcut.WorkingDirectory = AppDomain.CurrentDomain.SetupInformation.ApplicationBase;

			// 防止初始化时调用事件处理
			ckbRunAtStartUp.Checked += CkbRunAtStartUp_Checked;
			ckbMinimizeAtStartUp.Checked += CkbMinimizeAtStartUp_Checked;
			ckbRunAsAdmin.Checked += CkbRunAsAdmin_Checked;
			ckbRunAsAdmin.Unchecked += CkbRunAsAdmin_Unchecked;

			if (ckbRunAsAdmin.IsChecked.Value) {
				CkbRunAsAdmin_Checked(ckbRunAsAdmin, new RoutedEventArgs());
			} else {
				CkbRunAsAdmin_Unchecked(ckbRunAsAdmin, new RoutedEventArgs());
			}
		}

		private void CbbLanguage_SelectionChanged(object sender, SelectionChangedEventArgs e) {
			Settings.Default.CultureName = cbbLanguage.SelectedIndex == 0 ? "" : supportedCultures[cbbLanguage.SelectedIndex - 1].Name;

			if (Settings.Default.CultureName != originCulture) {
				CultureInfo cultureInfo = cbbLanguage.SelectedIndex == 0
					? CultureInfo.GetCultureInfo(NativeMethods.GetUserDefaultLocalName())
					: supportedCultures[cbbLanguage.SelectedIndex - 1];

				// 提示信息的语言为将要切换的语言
				lblRestartToApply1.Content = Properties.Resources.ResourceManager.GetString(
					"UI_Options_Common_Restart_To_Apply", cultureInfo);
				lblRestartToApply1.Visibility = Visibility.Visible;
			} else {
				lblRestartToApply1.Visibility = Visibility.Collapsed;
			}
		}

		// 在用户的“启动”文件夹创建快捷方式以实现开机启动
		private void CreateShortCut() {
			shortcut.Arguments = ckbMinimizeAtStartUp.IsChecked.GetValueOrDefault(false) ? "-st" : "";
			shortcut.Save();
		}

		private void CkbRunAtStartUp_Checked(object sender, RoutedEventArgs e) {
			CreateShortCut();
		}

		private void CkbRunAtStartUp_Unchecked(object sender, RoutedEventArgs e) {
			if (File.Exists(pathLink)) {
				File.Delete(pathLink);
			}
		}

		private void CkbMinimizeAtStartUp_Checked(object sender, RoutedEventArgs e) {
			CreateShortCut();
		}

		private void CkbMinimizeAtStartUp_Unchecked(object sender, RoutedEventArgs e) {
			CreateShortCut();
		}

		private void CkbRunAsAdmin_Checked(object sender, RoutedEventArgs e) {
			lblRestartToApply2.Visibility = originRunAsAdmin ? Visibility.Collapsed : Visibility.Visible;
		}

		private void CkbRunAsAdmin_Unchecked(object sender, RoutedEventArgs e) {
			lblRestartToApply2.Visibility = originRunAsAdmin ? Visibility.Visible : Visibility.Collapsed;
		}
	}
}
