using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;


namespace Magpie {
	internal class RadioBoolToUIntConverter : IValueConverter {
		public object Convert(object value, Type targetType, object parameter, CultureInfo culture) {
			return value == null ? DependencyProperty.UnsetValue : (uint)value == uint.Parse(parameter.ToString() ?? "0");
		}

		public object? ConvertBack(object value, Type targetType, object parameter, CultureInfo culture) {
			return (bool)value ? parameter : null;
		}
	}
}
