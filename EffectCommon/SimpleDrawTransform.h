#pragma once
#include <d2d1effectauthor.h>
#include "CommonDebug.h"


// 用来取代简单的 DrawTransform
// 该 DrawTransform 必须满足以下条件：
//  * NINPUTS 个输入
//  * 输出与所有输入尺寸相同
//  * 只是简单地对输入应用了一个像素着色器
//  * 无状态
template <int NINPUTS = 1>
class SimpleDrawTransform : public ID2D1DrawTransform {
protected:
	explicit SimpleDrawTransform(const GUID &shaderID): _shaderID(shaderID) {}

    SimpleDrawTransform(SimpleDrawTransform&&) = default;

    // 将 hlsl 读取进 Effect Context
    static HRESULT LoadShader(_In_ ID2D1EffectContext* d2dEC, _In_ const wchar_t* path, const GUID& shaderID) {
        if (!d2dEC->IsShaderLoaded(shaderID)) {
	        const HANDLE hFile = CreateFile(
                path,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
            if (hFile == INVALID_HANDLE_VALUE) {
                CommonDebug::WriteLine(L"打开\""s + path + L"\"失败");
                return E_FAIL;
            }

            LARGE_INTEGER liSize{};
            if (!GetFileSizeEx(hFile, &liSize)) {
                CommonDebug::WriteLine(L"获取\""s + path + L"\"文件大小失败");
                return E_FAIL;
            }

	        const auto size = static_cast<DWORD>(liSize.QuadPart);
	        const auto buf = new BYTE[size];
            DWORD readed = 0;
            if (!ReadFile(hFile, buf, size, &readed, nullptr) || readed == 0) {
                CommonDebug::WriteLine(L"读取\""s + path + L"\"失败");
                return E_FAIL;
            }

	        const HRESULT hr = d2dEC->LoadPixelShader(shaderID, buf, size);
            delete[] buf;

            if (FAILED(hr)) {
                CommonDebug::WriteLine(L"加载着色器\""s + path + L"\"失败");
                return hr;
            }
        }

        return S_OK;
    }
public:
    virtual ~SimpleDrawTransform() = default;

    static HRESULT Create(
        _In_ ID2D1EffectContext* d2dEC,
        _Outptr_ SimpleDrawTransform** ppOutput,
        _In_ const wchar_t* shaderPath,
        const GUID &shaderID
    ) {
        if (!ppOutput) {
            return E_INVALIDARG;
        }

        HRESULT hr = LoadShader(d2dEC, shaderPath, shaderID);
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new SimpleDrawTransform(shaderID);
        return S_OK;
    }

    IFACEMETHODIMP_(UINT32) GetInputCount() const override {
        return NINPUTS;
    }

    IFACEMETHODIMP MapInputRectsToOutputRect(
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputRects,
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputOpaqueSubRects,
        UINT32 inputRectCount,
        _Out_ D2D1_RECT_L* pOutputRect,
        _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
    ) override {
        if (inputRectCount != NINPUTS) {
            return E_INVALIDARG;
        }

        _inputRects[0] = pInputRects[0];
        for (int i = 1; i < NINPUTS; ++i) {
            if (pInputRects[0].right - pInputRects[0].left != pInputRects[i].right - pInputRects[i].left
                || pInputRects[0].bottom - pInputRects[0].top != pInputRects[i].bottom - pInputRects[i].top)
            {
                return E_INVALIDARG;
            }

            _inputRects[i] = pInputRects[i];
        }

        *pOutputRect = pInputRects[0];
        *pOutputOpaqueSubRect = { 0,0,0,0 };

        _SetShaderConstantBuffer(SIZE{
            pInputRects->right - pInputRects->left,
            pInputRects->bottom - pInputRects->top
            });

        return S_OK;
    }

    IFACEMETHODIMP SetDrawInfo(ID2D1DrawInfo* pDrawInfo) override {
        _drawInfo = pDrawInfo;
        return pDrawInfo->SetPixelShader(_shaderID);
    }

    IFACEMETHODIMP MapOutputRectToInputRects(
        _In_ const D2D1_RECT_L* pOutputRect,
        _Out_writes_(inputRectCount) D2D1_RECT_L* pInputRects,
        UINT32 inputRectCount
    ) const override {
        if (inputRectCount != NINPUTS) {
            return E_INVALIDARG;
        }

        for (int i = 0; i < NINPUTS; ++i) {
            pInputRects[i] = _inputRects[i];
        }

        return S_OK;
    }

    IFACEMETHODIMP MapInvalidRect(
        UINT32 inputIndex,
        D2D1_RECT_L invalidInputRect,
        _Out_ D2D1_RECT_L* pInvalidOutputRect
    ) const override {
        // This transform is designed to only accept one input.
        if (inputIndex >= NINPUTS) {
            return E_INVALIDARG;
        }

        // If part of the transform's input is invalid, mark the corresponding
        // output region as invalid. 
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
		const ULONG ulRefCount = InterlockedDecrement(&_cRef);
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
    // 继承的类可以覆盖此方法向着色器传递参数
    virtual void _SetShaderConstantBuffer(const SIZE& srcSize) {
        struct {
            INT32 srcWidth;
            INT32 srcHeight;
        } shaderConstants{
            srcSize.cx,
            srcSize.cy
        };

        _drawInfo->SetPixelShaderConstantBuffer(reinterpret_cast<BYTE*>(&shaderConstants), sizeof(shaderConstants));
    }

    ComPtr<ID2D1DrawInfo> _drawInfo = nullptr;

    // 保存输入形状供 MapOutputRectToInputRects 使用，而不是使用 pOutputRect
    // 见 https://stackoverflow.com/questions/36920282/pixel-shader-in-direct2d-render-error-along-the-middle
    D2D1_RECT_L _inputRects[NINPUTS]{};

private:
    const GUID& _shaderID;

    // 引用计数
    // 因为引用计数从 1 开始，所以创建实例时无需 AddRef
    ULONG _cRef = 1;
};
