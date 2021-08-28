#pragma once
#pragma once
#include <debugapi.h>
#include <string_view>



class CommonDebug {
public:
	CommonDebug() = delete;
	CommonDebug(const CommonDebug&) = delete;
	CommonDebug(CommonDebug&&) = delete;


	static void WriteLine(const std::wstring_view& msg) {
#ifdef _DEBUG
		OutputDebugString(L"##DEBUG##: ");
		OutputDebugString(msg.data());
		OutputDebugString(L"\n");
#endif // _DEBUG
	}

	template<typename T>
	static void WriteLine(T msg) {
		WriteLine(std::to_wstring(msg));
	}

	static void WriteLine(const std::wstring& msg) {
		WriteLine(std::wstring_view(msg));
	}

	static void WriteLine(const wchar_t* msg) {
		WriteLine(std::wstring_view(msg));
	}

	static void WriteErrorMessage(const std::wstring_view& msg) {
		WriteLine(msg);
	}

};
