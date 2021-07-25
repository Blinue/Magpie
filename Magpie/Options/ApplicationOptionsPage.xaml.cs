using Magpie.Properties;
using System;
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

        public ApplicationOptionsPage() {
            InitializeComponent();

            string startUpFolder = Environment.GetFolderPath(Environment.SpecialFolder.Startup);
            pathLink = startUpFolder + "\\Magpie.lnk";

            shortcut = (IWshRuntimeLibrary.IWshShortcut)shell.CreateShortcut(pathLink);

            if (File.Exists(pathLink)) {
                // 开机启动的选项取决于快捷方式是否存在
                // 最小化的选项取决于设置
                ckbRunAtStartUp.IsChecked = true;
                ckbMinimizeAtStartUp.IsEnabled = true;

                if (Settings.Default.MinimizeAtWindowsStartUp != (shortcut.Arguments == "-st")) {
                    // 设置和实际不一致，重新创建快捷方式
                    CreateShortCut();
                }
            } else {
                ckbMinimizeAtStartUp.IsEnabled = false;
            }

            ckbMinimizeAtStartUp.IsChecked = Settings.Default.MinimizeAtWindowsStartUp;

            shortcut.TargetPath = Assembly.GetExecutingAssembly().Location;
            shortcut.WorkingDirectory = AppDomain.CurrentDomain.SetupInformation.ApplicationBase;

            // 延迟绑定，防止在初始化时执行
            ckbRunAtStartUp.Checked += CkbRunAtStartUp_Checked;
            ckbMinimizeAtStartUp.Checked += CkbMinimizeAtStartUp_Checked;
        }

        // 在用户的“启动”文件夹创建快捷方式以实现开机启动
        private void CreateShortCut() {
            shortcut.Arguments = ckbMinimizeAtStartUp.IsChecked.GetValueOrDefault(false) ? "-st" : "";
            shortcut.Save();
        }

        private void CkbRunAtStartUp_Checked(object sender, RoutedEventArgs e) {
            CreateShortCut();

            ckbMinimizeAtStartUp.IsEnabled = true;
        }

        private void CkbRunAtStartUp_Unchecked(object sender, RoutedEventArgs e) {
            if (File.Exists(pathLink)) {
                File.Delete(pathLink);
            }

            ckbMinimizeAtStartUp.IsEnabled = false;
        }

        private void CkbMinimizeAtStartUp_Checked(object sender, RoutedEventArgs e) {
            CreateShortCut();
            Settings.Default.MinimizeAtWindowsStartUp = true;
        }

        private void CkbMinimizeAtStartUp_Unchecked(object sender, RoutedEventArgs e) {
            CreateShortCut();
            Settings.Default.MinimizeAtWindowsStartUp = false;
        }
    }
}
