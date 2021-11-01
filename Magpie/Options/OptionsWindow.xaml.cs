using Magpie.Properties;
using System;
using System.Windows;
using System.Windows.Controls;


namespace Magpie.Options {
	/// <summary>
	/// OptionsWindow.xaml 的交互逻辑
	/// </summary>
	public partial class OptionsWindow : Window {
		private static readonly string[] OPTIONS_PAGES = new string[] {
			"ApplicationOptionsPage.xaml",
			"ScaleOptionsPage.xaml",
			"AdvancedOptionsPage.xaml",
			"AboutOptionsPage.xaml"
		};

		public OptionsWindow() {
			InitializeComponent();

			lbxOptionsPage.SelectedIndex = 0;
			_ = ((ListBoxItem)lbxOptionsPage.SelectedItem).Focus();
		}

		private void LxbOptionsPage_SelectionChanged(object sender, SelectionChangedEventArgs e) {
			int idx = lbxOptionsPage.SelectedIndex;
			if (idx < 0 || idx >= OPTIONS_PAGES.Length) {
				idx = 0;
			}

			contentFrame.Source = new Uri(OPTIONS_PAGES[idx], UriKind.Relative);

			Settings.Default.Save();
		}

		private void Window_Deactivated(object sender, EventArgs e) {
			Settings.Default.Save();
		}
	}
}
