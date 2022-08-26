// Copyright (c) 2021 - present, Liu Xu
//
//  This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "pch.h"
#include "XamlApp.h"


int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ LPWSTR lpCmdLine,
	_In_ int /*nCmdShow*/
) {
	// 程序结束时也不应调用 uninit_apartment
	// 见 https://kennykerr.ca/2018/03/24/cppwinrt-hosting-the-windows-runtime/
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	auto& app = Magpie::XamlApp::Get();
	if (!app.Initialize(hInstance, lpCmdLine)) {
		return -1;
	}

	return app.Run();
}
