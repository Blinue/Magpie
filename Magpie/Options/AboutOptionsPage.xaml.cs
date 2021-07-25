using System.Diagnostics;
using System.Windows.Controls;
using System.Windows.Navigation;


namespace Magpie {
    /// <summary>
    /// AboutOptionsPage.xaml 的交互逻辑
    /// </summary>
    public partial class AboutOptionsPage : Page {
        public AboutOptionsPage() {
            InitializeComponent();
        }

        private void Hyperlink_RequestNavigate(object sender, RequestNavigateEventArgs e) {
            _ = Process.Start(new ProcessStartInfo(e.Uri.AbsoluteUri));
            e.Handled = true;
        }
    }
}
