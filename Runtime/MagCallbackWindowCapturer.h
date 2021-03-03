#pragma once
#include "pch.h"
#include "WindowCapturerBase.h"
#include "Utils.h"


// 使用 Magnification API 抓取窗口
// 见 https://docs.microsoft.com/en-us/previous-versions/windows/desktop/magapi/magapi-intro
class MagCallbackWindowCapturer : public WindowCapturerBase {
public:
	MagCallbackWindowCapturer(
        HINSTANCE hInstance,
        HWND hwndHost,
        const RECT& srcRect,
        IWICImagingFactory2* wicImgFactory
    ): _srcRect(srcRect), _wicImgFactory(wicImgFactory) {
        if (_instance) {
            Debug::Assert(false, L"已存在 MagCallbackWindowCapturer 实例");
        }

		Debug::ThrowIfWin32Failed(
			MagInitialize(),
			L"MagInitialize 失败"
		);

        // 创建不可见的放大镜控件
        // 大小为目标窗口客户区
        SIZE srcClientSize = Utils::GetSize(srcRect);

        _hwndMag = CreateWindow(WC_MAGNIFIER, L"MagnifierWindow",
            WS_CHILD,
            0, 0, srcClientSize.cx, srcClientSize.cy,
            hwndHost, NULL, hInstance, NULL);
        Debug::ThrowIfWin32Failed(_hwndMag, L"创建放大镜控件失败");

        Debug::ThrowIfWin32Failed(
            MagSetImageScalingCallback(_hwndMag, &_ImageScalingCallback),
            L"设置放大镜回调失败"
        );

        _instance = this;
	}

	~MagCallbackWindowCapturer() {
		MagUninitialize();

        _instance = nullptr;
	}

	ComPtr<IWICBitmapSource> GetFrame() override {
        // MagSetWindowSource 是同步执行的
        if (!MagSetWindowSource(_hwndMag, _srcRect)) {
            Debug::WriteErrorMessage(L"MagSetWindowSource 失败");
        }

        return _wicBmp;
	}

private:
    static BOOL CALLBACK _ImageScalingCallback(
        HWND hWnd,
        void* srcdata,
        MAGIMAGEHEADER srcheader,
        void* destdata,
        MAGIMAGEHEADER destheader,
        RECT unclipped,
        RECT clipped,
        HRGN dirty
    ) {
        if (!_instance) {
            return FALSE;
        }

        if (srcheader.cbSize / srcheader.width / srcheader.height != 4) {
            Debug::WriteErrorMessage(L"srcdata 不是BGRA格式");
            return FALSE;
        }

        Debug::ThrowIfComFailed(
            _instance->_wicImgFactory->CreateBitmapFromMemory(
                srcheader.width,
                srcheader.height,
                GUID_WICPixelFormat32bppPBGRA,
                srcheader.stride,
                (UINT)srcheader.cbSize,
                (BYTE*)srcdata,
                &_instance->_wicBmp
            ),
            L"从内存创建 WICBitmap 失败"
        );

        return TRUE;
    }

    HWND _hwndMag = NULL;
    const RECT& _srcRect;
    IWICImagingFactory2* _wicImgFactory;

    ComPtr<IWICBitmap> _wicBmp = nullptr;

    static MagCallbackWindowCapturer* _instance;
};
