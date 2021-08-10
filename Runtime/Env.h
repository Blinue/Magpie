#pragma once
#include "pch.h"
#include "Utils.h"


class Env {
public:
	static std::unique_ptr<Env> $instance;

	static void CreateInstance(
		HINSTANCE hInst,
		HWND hwndSrc,
		std::string_view scaleModel,
		int captureMode,
		int bufferPrecision,
		bool showFPS,
		bool noDisturb
	) {
		$instance.reset(new Env(hInst, hwndSrc, scaleModel, captureMode, bufferPrecision, showFPS, noDisturb));
	}

	void SetD2DContext(
		ComPtr<ID3D11Device> d3dDevice,
		ComPtr<ID2D1Factory1> d2dFactory,
		ComPtr<ID2D1Device> d2dDevice,
		ComPtr<ID2D1DeviceContext> d2dDC
	) {
		_d3dDevice = d3dDevice;
		_d2dFactory = d2dFactory;
		_d2dDevice = d2dDevice;
		_d2dDC = d2dDC;
	}

	void SetHwndHost(HWND hwndHost) {
		_hwndHost = hwndHost;
		Utils::GetClientScreenRect(_hwndHost, _hostClient);
	}

	ID2D1DeviceContext* GetD2DDC() {
		return _d2dDC.Get();
	}

	ID2D1Factory1* GetD2DFactory() {
		return _d2dFactory.Get();
	}

	ID3D11Device* GetD3DDevice() {
		return _d3dDevice.Get();
	}

	ID2D1Device* GetD2DDevice() {
		return _d2dDevice.Get();
	}

	const std::string_view& GetScaleModel() {
		return _scaleModel;
	}

	int GetCaptureMode() {
		return _captureMode;
	}

	int GetBufferPrecision() {
		return _bufferPrecision;
	}

	bool IsShowFPS() {
		return _showFPS;
	}

	bool IsNoDisturb() {
		return _noDisturb;
	}

	HINSTANCE GetHInstance() {
		return _hInst;
	}

	HWND GetHwndSrc() {
		return _hwndSrc;
	}

	HWND GetHwndHost() {
		return _hwndHost;
	}

	const RECT& GetSrcClient() {
		return _srcClient;
	}

	const RECT& GetHostClient() {
		return _hostClient;
	}

	void SetDestRect(const D2D_RECT_F& rect) {
		_destRect = rect;
	}

	const D2D_RECT_F& GetDestRect() {
		return _destRect;
	}

	IWICImagingFactory2* GetWICImageFactory() {
		if (_wicImgFactory == nullptr) {
			Debug::ThrowIfComFailed(
				CoCreateInstance(
					CLSID_WICImagingFactory,
					NULL,
					CLSCTX_INPROC_SERVER,
					IID_PPV_ARGS(&_wicImgFactory)
				),
				L"创建 WICImagingFactory 失败"
			);
		}

		return _wicImgFactory.Get();
	}

	IDWriteFactory* GetDWFactory() {
		if (_dwFactory == nullptr) {
			Debug::ThrowIfComFailed(
				DWriteCreateFactory(
					DWRITE_FACTORY_TYPE_SHARED,
					__uuidof(IDWriteFactory),
					&_dwFactory
				),
				L"创建 IDWriteFactory 失败"
			);
		}

		return _dwFactory.Get();
	}
private:
	Env(
		HINSTANCE hInst,
		HWND hwndSrc,
		std::string_view scaleModel,
		int captureMode,
		int bufferPrecision,
		bool showFPS,
		bool noDisturb
	) : _hInst(hInst), _hwndSrc(hwndSrc), _scaleModel(scaleModel),
		_captureMode(captureMode), _bufferPrecision(bufferPrecision), _showFPS(showFPS), _noDisturb(noDisturb)
	{
		Utils::GetClientScreenRect(_hwndSrc, _srcClient);
	}

	ComPtr<ID3D11Device> _d3dDevice = nullptr;
	ComPtr<ID2D1Factory1> _d2dFactory = nullptr;
	ComPtr<ID2D1Device> _d2dDevice = nullptr;
	ComPtr<ID2D1DeviceContext> _d2dDC = nullptr;

	HINSTANCE _hInst;
	std::string_view _scaleModel;
	int _captureMode;
	int _bufferPrecision;
	bool _showFPS;
	bool _noDisturb;

	HWND _hwndSrc;
	HWND _hwndHost = NULL;

	RECT _srcClient;
	RECT _hostClient{};

	D2D_RECT_F _destRect{};

	ComPtr<IWICImagingFactory2> _wicImgFactory = nullptr;
	ComPtr<IDWriteFactory> _dwFactory = nullptr;
};
