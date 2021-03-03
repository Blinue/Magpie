#pragma once
#include <string>
#include <cassert>


// c++ 原生 exception 不支持宽字符串
class magpie_exception {
public:
    magpie_exception() noexcept {}

    magpie_exception(const std::wstring_view& msg) noexcept: _msg(msg) {
    }

    virtual const std::wstring& what() const {
        return _msg;
    }

    virtual ~magpie_exception() {}

private:
    std::wstring _msg;
};


// COM 出错时产生的异常
class com_exception : public magpie_exception {
public:
    com_exception(HRESULT hr) noexcept: _result(hr) {
        _whatMsg = std::move(L"HRESULT=" + std::to_wstring(_result));
    }

    com_exception(HRESULT hr, const std::wstring_view& msg) noexcept
        : magpie_exception(msg), _result(hr) {
        _whatMsg = std::move(magpie_exception::what() + L"(HRESULT=" + std::to_wstring(_result) + L")");
    }

    const std::wstring& what() const override {
        return _whatMsg;
    }

private:
    HRESULT _result;
    std::wstring _whatMsg;
};

// 调用 WIN32 API 出错时产生的异常
class win32_exception : public magpie_exception {
public:
    win32_exception() noexcept : magpie_exception() {};

    win32_exception(const std::wstring_view& msg) noexcept : magpie_exception(msg) {};
};


class Debug {
public:
    Debug() = delete;
    Debug(const Debug&) = delete;
    Debug(Debug&&) = delete;


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
        SetLastErrorMessage(msg);
    }

    template<typename T>
    static void WriteErrorMessage(T msg) {
        WriteErrorMessage(std::to_wstring(msg));
    }

    static void WriteErrorMessage(const std::wstring& msg) {
        WriteErrorMessage(std::wstring_view(msg));
    }

    static void WriteErrorMessage(const wchar_t* msg) {
        WriteErrorMessage(std::wstring_view(msg));
    }


    // 将 COM 的错误转换为异常
    static void ThrowIfComFailed(HRESULT hr, const std::wstring_view& failMsg) {
        if (SUCCEEDED(hr)) {
            return;
        }

        com_exception e(hr, failMsg);
        WriteLine(L"com_exception: " + e.what());
        SetLastErrorMessage(e.what());

        throw e;
    }

    // 将 Win32 错误转换成异常
    template <typename T>
    static void ThrowIfWin32Failed(T result, const std::wstring_view& failMsg) {
        if (result) {
            return;
        }

        win32_exception e(failMsg);
        WriteLine(L"win32_exception: " + e.what());
        SetLastErrorMessage(e.what());

        throw e;
    }

    template <typename T>
    static void Assert(T result, const std::wstring_view& failMsg) {
        if (result) {
            return;
        }

        magpie_exception e(failMsg);
        WriteLine(L"magpie_exception: " + e.what());
        SetLastErrorMessage(e.what());

        throw e;
    }

    static void SetLastErrorMessage(const std::wstring_view& msg) {
        _lastErrorMessage = msg;
    }

    static const std::wstring& GetLastErrorMessage() {
        return _lastErrorMessage;
    }

private:
    static std::wstring _lastErrorMessage;
};
