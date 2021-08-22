using Magpie.Properties;
using System.Windows.Controls;


namespace Magpie.Options {
	/// <summary>
	/// AdvancedOptionsPage.xaml 的交互逻辑
	/// </summary>
	public partial class AdvancedOptionsPage : Page {
		public AdvancedOptionsPage() {
			InitializeComponent();

			cbbLoggingLevel.SelectionChanged += CbbLoggingLevel_SelectionChanged;
		}

		private void CbbLoggingLevel_SelectionChanged(object sender, SelectionChangedEventArgs e) {
			App.UpdateLoggingLevel(Settings.Default.LoggingLevel);
		}
	}
}
