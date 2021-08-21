using Magpie.Properties;
using System;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Threading;
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

		private readonly (string displayName, string cultureName)[] supportedCultures = {
			("English", "en-US"),
			("中文", "zh-CN"),
			("Ру́сский язы́к", "ru-RU")
		};

		public ApplicationOptionsPage() {
			InitializeComponent();

			// 第一项为系统默认语言
			_ = cbbLanguage.Items.Add(Properties.Resources.UI_Options_Application_Language_Default);
			foreach ((string displayName, string cultureName) in supportedCultures) {
				_ = cbbLanguage.Items.Add(displayName);
			}

			if (string.IsNullOrEmpty(Settings.Default.CultureName)) {
				cbbLanguage.SelectedIndex = 0;
			} else {
				int idx = Array.FindIndex(supportedCultures, culture => {
					return culture.cultureName == Settings.Default.CultureName;
				});
				if (idx == -1) {
					// 未找到，重置为默认
					Settings.Default.CultureName = "";
					// idx 为 -1
				}
				cbbLanguage.SelectedIndex = idx + 1;
			}

			cbbLanguage.SelectionChanged += CbbLanguage_SelectionChanged;

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
		}

		private void CbbLanguage_SelectionChanged(object sender, SelectionChangedEventArgs e) {
			if (cbbLanguage.SelectedIndex < 0 || cbbLanguage.SelectedIndex > supportedCultures.Length) {
				return;
			}

			Settings.Default.CultureName = cbbLanguage.SelectedIndex == 0 ? ""
				: supportedCultures[cbbLanguage.SelectedIndex - 1].cultureName;

			string cultureName = Settings.Default.CultureName == "" ? NativeMethods.GetUserDefaultLocalName() : Settings.Default.CultureName;
			CultureInfo cultureInfo = CultureInfo.GetCultureInfo(cultureName);

			Thread.CurrentThread.CurrentCulture = cultureInfo;
			Thread.CurrentThread.CurrentUICulture = cultureInfo;
			Properties.Resources.Culture = cultureInfo;
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
	}
}
