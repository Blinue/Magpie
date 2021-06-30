#pragma once
#include <string>
#include <cassert>
#include <boost/format.hpp>
#include <chrono>
#include <CommonDebug.h>


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
        _whatMsg = boost::str(boost::wformat(L"%s (HRESULT=0x%x)") % magpie_exception::what() % _result);
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


class Debug : public CommonDebug {
public:
    static SIZE GetSize(const RECT& rect) {
        return { rect.right - rect.left, rect.bottom - rect.top };
    }

    static D2D1_SIZE_F GetSize(const D2D1_RECT_F& rect) {
        return { rect.right - rect.left,rect.bottom - rect.top };
    }

    // 将 COM 的错误转换为异常
    static void ThrowIfComFailed(HRESULT hr, const std::wstring_view& failMsg) {
        if (SUCCEEDED(hr)) {
            return;
        }

        com_exception e(hr, failMsg);
        WriteLine(L"com_exception: " + e.what());

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

        throw e;
    }

    template <typename T>
    static void Assert(T result, const std::wstring_view& failMsg) {
        if (result) {
            return;
        }

        magpie_exception e(failMsg);
        WriteLine(L"magpie_exception: " + e.what());

        throw e;
    }

    static int Measure(std::function<void()> func) {
        using namespace std::chrono;

        auto t = steady_clock::now();
        func();
        auto dura = duration_cast<milliseconds>(steady_clock::now() - t);

        return int(dura.count());
    }
};
