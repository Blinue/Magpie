#pragma once
#include "InfoGridData.g.h"
#include "InfoGrid.g.h"
#include "SmallVector.h"
#include <variant>

namespace winrt::Magpie::implementation {

struct InfoGridData : InfoGridDataT<InfoGridData> {
	using RowData = std::pair<std::variant<hstring, UIElement>, std::variant<hstring, UIElement>>;

	InfoGridData(::Magpie::SmallVector<RowData, 0>&& rows_) noexcept : rows(std::move(rows_)) {}

	::Magpie::SmallVector<RowData, 0> rows;
};

struct InfoGrid : InfoGridT<InfoGrid> {
	void InitializeComponent();

	Magpie::InfoGridData Data() const noexcept;

	void Data(Magpie::InfoGridData value) noexcept;

private:
	winrt::Style _contentTextBlockStyle{ nullptr };
	Magpie::InfoGridData _data{ nullptr };
};

}

BASIC_FACTORY(InfoGrid)
