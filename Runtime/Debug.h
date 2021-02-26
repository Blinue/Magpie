#pragma once
#include <string>
#include <cassert>

using namespace std::literals::string_literals;


// COM 出错时产生的异常
class com_exception : public std::exception {
public:
    com_exception(HRESULT hr) : _result(hr) {}

    const char* what() const override {
        static char s_str[64] = {};
        sprintf_s(s_str, "Failure with HRESULT of %08X",
            static_cast<unsigned int>(_result));
        return s_str;
    }

private:
    HRESULT _result;
};

// 调用 WIN32 API 出错时产生的异常
class win32_exception : public std::exception {
};

class Debug {
public:
    Debug() = delete;
    Debug(const Debug&) = delete;
    Debug(Debug&&) = delete;

    template<typename T>
    static void writeLine(T msg) {
#ifdef _DEBUG
        writeLine(std::to_wstring(msg));
#endif // _DEBUG
    }

    static void writeLine(std::wstring_view msg) {
#ifdef _DEBUG
        OutputDebugString(L"##DEBUG##: ");
        OutputDebugString(msg.data());
        OutputDebugString(L"\n");
#endif // _DEBUG
    }

    static void writeLine(std::wstring msg) {
#ifdef _DEBUG
        writeLine(std::wstring_view(msg));
#endif // _DEBUG
    }

    static void writeLine(const wchar_t* msg) {
#ifdef _DEBUG
        writeLine(std::wstring_view(msg));
#endif // _DEBUG
    }

    // 将 COM 的错误转换为异常
    static void ThrowIfFailed(HRESULT hr, std::wstring_view failMsg) {
        if (FAILED(hr)) {
            writeLine(std::wstring(failMsg) + L", hr=" + std::to_wstring(static_cast<unsigned int>(hr)));

            throw com_exception(hr);
        }
    }

    // 将 WIN32 API 的错误转换成异常
    template <typename T>
    static void ThrowIfFalse(T result, std::wstring_view failMsg) {
        if (result) {
            return;
        }

        writeLine(failMsg);
        throw win32_exception();
    }
};
