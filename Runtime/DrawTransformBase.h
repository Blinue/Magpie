#pragma once
#include "pch.h"
#include "GUIDs.h"
#include <d2d1effectauthor.h>


// 自定义 Effect 使用的 Transform 基类
// 实现了 IUnkown 接口
// 默认实现假设输入个数为 1 且不改变形状，不对输入到输出的映射作任何假设
class DrawTransformBase : public ID2D1DrawTransform {
public:
    virtual ~DrawTransformBase() {}

    // 不可复制，不可移动
    DrawTransformBase(const DrawTransformBase&) = delete;
    DrawTransformBase(DrawTransformBase&&) = delete;

protected:
    // 将 hlsl 读取进 Effect Context
    static HRESULT LoadShader(_In_ ID2D1EffectContext* d2dEC, _In_ const wchar_t* path, const GUID& shaderID) {
        if (!d2dEC->IsShaderLoaded(shaderID)) {
            HANDLE hFile = CreateFile(
                path,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
            if (hFile == NULL) {
                Debug::WriteLine(L"打开\""s + path + L"\"失败");
                return E_FAIL;
            }

            LARGE_INTEGER liSize{};
            if (!GetFileSizeEx(hFile, &liSize)) {
                Debug::WriteLine(L"获取\""s + path + L"\"文件大小失败");
                return E_FAIL;
            }

            DWORD size = (DWORD)liSize.QuadPart;
            BYTE* buf = new BYTE[size];
            DWORD readed = 0;
            if (!ReadFile(hFile, buf, size, &readed, nullptr) || readed == 0) {
                Debug::WriteLine(L"读取\""s + path + L"\"失败");
                return E_FAIL;
            }

            HRESULT hr = d2dEC->LoadPixelShader(shaderID, buf, size);
            delete[] buf;

            if (FAILED(hr)) {
                Debug::WriteLine(L"加载着色器\""s + path + L"\"失败");
                return hr;
            }
        }

        return S_OK;
    }

public:
    /*
    * 以下为 ID2D1DrawTransform 的方法
    */

    // 第一次加入 Effect 时被调用
    IFACEMETHODIMP SetDrawInfo(ID2D1DrawInfo* d2dDrawInfo) override {
        return S_OK;
    }

    /*
    * 以下为 ID2D1Transform 的方法
    */

    // 指定输入个数
    IFACEMETHODIMP_(UINT32) GetInputCount() const override {
        return 1;
    }

    // D2D 在每次渲染时调用此函数，决定输入到输出的映射
    IFACEMETHODIMP MapInputRectsToOutputRect(
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputRects,
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputOpaqueSubRects,
        UINT32 inputRectCount,
        _Out_ D2D1_RECT_L* pOutputRect,
        _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
    ) override {
        if (inputRectCount != 1) {
            return E_INVALIDARG;
        }

        // 输出形状与输入相同
        *pOutputRect = *pInputRects;
        _inputRect = *pInputRects;

        // 对不透明区域不作假设
        *pOutputOpaqueSubRect = { 0,0,0,0 };

        return S_OK;
    }

    // 决定输出到输入的映射
    // 不能有副作用，因为没有明确的被调用时间
    IFACEMETHODIMP MapOutputRectToInputRects(
        _In_ const D2D1_RECT_L* pOutputRect,
        _Out_writes_(inputRectCount) D2D1_RECT_L* pInputRects,
        UINT32 inputRectCount
    ) const override {
        if (inputRectCount != 1) {
            return E_INVALIDARG;
        }

        // 输入形状与输出相同
        pInputRects[0] = _inputRect;

        return S_OK;
    }

    // 决定输入变化时输出的哪个区域需要重新渲染
    IFACEMETHODIMP MapInvalidRect(
        UINT32 inputIndex,
        D2D1_RECT_L invalidInputRect,
        _Out_ D2D1_RECT_L* pInvalidOutputRect
    ) const override {
        if (inputIndex != 0) {
            return E_INVALIDARG;
        }

        // 不对输入到输出的映射作任何假设，所以将无效区域设为整个输出
        *pInvalidOutputRect = D2D1::RectL(LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX);

        return S_OK;
    }

    /*
    * 以下为 IUnkown 的方法
    */

    IFACEMETHODIMP_(ULONG) AddRef() override {
        InterlockedIncrement(&_cRef);
        return _cRef;
    }

    IFACEMETHODIMP_(ULONG) Release() override {
        ULONG ulRefCount = InterlockedDecrement(&_cRef);
        if (0 == _cRef) {
            delete this;
        }
        return ulRefCount;
    }

    IFACEMETHODIMP QueryInterface(REFIID riid, _Outptr_ void** ppOutput) override {
        if (!ppOutput)
            return E_INVALIDARG;

        *ppOutput = nullptr;
        if (riid == __uuidof(ID2D1DrawTransform)
            || riid == __uuidof(ID2D1Transform)
            || riid == __uuidof(ID2D1TransformNode)
            || riid == __uuidof(IUnknown)
            ) {
            // 复制指针，增加计数
            *ppOutput = static_cast<void*>(this);
            AddRef();
            return NOERROR;
        }
        return E_NOINTERFACE;
    }

protected:
    // 实现不能公开构造函数
    DrawTransformBase() {}

    // 保存输入形状供 MapOutputRectToInputRects 使用，而不是使用 pOutputRect
    // 见 https://stackoverflow.com/questions/36920282/pixel-shader-in-direct2d-render-error-along-the-middle
    D2D1_RECT_L _inputRect{};

private:
    // 引用计数
    // 因为引用计数从 1 开始，所以创建实例时无需 AddRef
    ULONG _cRef = 1;
};