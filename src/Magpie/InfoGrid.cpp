#include "pch.h"
#include "InfoGrid.h"
#if __has_include("InfoGridData.g.cpp")
#include "InfoGridData.g.cpp"
#endif
#if __has_include("InfoGrid.g.cpp")
#include "InfoGrid.g.cpp"
#endif

using namespace ::Magpie;

namespace winrt::Magpie::implementation {

void InfoGrid::InitializeComponent() {
	InfoGridT::InitializeComponent();

	_contentTextBlockStyle =
		Resources().TryLookup(box_value(L"ContentTextBlockStyle")).try_as<winrt::Style>();
}

Magpie::InfoGridData InfoGrid::Data() const noexcept {
	return _data;
}

void InfoGrid::Data(Magpie::InfoGridData value) noexcept {
	_data = std::move(value);

	const auto& rows = get_self<InfoGridData>(_data)->rows;

	const uint32_t rowCount = (uint32_t)rows.size();
	UIElementCollection children = Children();
	children.Clear();

	{
		RowDefinitionCollection rowDefs = RowDefinitions();
		uint32_t oldRowCount = rowDefs.Size();
		if (rowCount > oldRowCount) {
			for (uint32_t i = oldRowCount; i < rowCount; ++i) {
				rowDefs.Append(RowDefinition());
			}
		} else if (rowCount < oldRowCount) {
			for (uint32_t i = rowCount; i < oldRowCount; ++i) {
				rowDefs.RemoveAtEnd();
			}
		}
	}

	if (rowCount == 0) {
		return;
	}

	for (uint32_t i = 0; i < rowCount; ++i) {
		const auto& rowData = rows[i];

		UIElement control{ nullptr };

		if (rowData.first.index() == 0) {
			TextBlock textBlock;
			textBlock.Text(std::get<0>(rowData.first));
			control = std::move(textBlock);
		} else {
			control = std::get<1>(rowData.first);
		}

		control.SetValue(Grid::RowProperty(), box_value(i));
		control.SetValue(Grid::ColumnProperty(), box_value(0));
		children.Append(control);

		if (rowData.second.index() == 0) {
			TextBlock textBlock;
			textBlock.Text(std::get<0>(rowData.second));
			textBlock.Style(_contentTextBlockStyle);
			control = std::move(textBlock);
		} else {
			control = std::get<1>(rowData.second);
		}

		control.SetValue(Grid::RowProperty(), box_value(i));
		control.SetValue(Grid::ColumnProperty(), box_value(1));
		children.Append(control);
	}
}

}
