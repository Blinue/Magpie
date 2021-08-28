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
		bool adjustCursorSpeed,
		bool noDisturb
	) {
		$instance.reset(new Env(hInst, hwndSrc, scaleModel, captureMode, bufferPrecision, showFPS, adjustCursorSpeed, noDisturb));
	}

	void SetD3DContext(ComPtr<ID3D11Device3> d3dDevice, ComPtr<ID3D11DeviceContext4> d3dDC) {
		_d3dDevice = d3dDevice;
		_d3dDC = d3dDC;
	}

	void SetHwndHost(HWND hwndHost) {
		_hwndHost = hwndHost;
		Utils::GetClientScreenRect(_hwndHost, _hostClient);
	}

	ID3D11Device3* GetD3DDevice() const {
		return _d3dDevice.Get();
	}

	ID3D11DeviceContext4* GetD3DDC() const {
		return _d3dDC.Get();
	}

	const std::string_view& GetScaleModel() const {
		return _scaleModel;
	}

	int GetCaptureMode() const {
		return _captureMode;
	}

	int GetBufferPrecision() const {
		return _bufferPrecision;
	}

	bool IsShowFPS() const {
		return _showFPS;
	}

	bool IsAdjustCursorSpeed() const {
		return _adjustCursorSpeed;
	}

	bool IsNoDisturb() const {
		return _noDisturb;
	}

	HINSTANCE GetHInstance() const {
		return _hInst;
	}

	HWND GetHwndSrc() const {
		return _hwndSrc;
	}

	HWND GetHwndHost() const {
		return _hwndHost;
	}

	const RECT& GetSrcClient() const {
		return _srcClient;
	}

	const RECT& GetHostClient() const {
		return _hostClient;
	}

	void SetDestRect(const D2D_RECT_F& rect) {
		_destRect = rect;
	}

	const D2D_RECT_F& GetDestRect() const {
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
		bool adjustCursorSpeed,
		bool noDisturb
	) : _hInst(hInst), _hwndSrc(hwndSrc), _scaleModel(scaleModel), _captureMode(captureMode),
		_bufferPrecision(bufferPrecision), _showFPS(showFPS), _adjustCursorSpeed(adjustCursorSpeed), _noDisturb(noDisturb)
	{
		Utils::GetClientScreenRect(_hwndSrc, _srcClient);
	}

	ComPtr<ID3D11Device3> _d3dDevice = nullptr;
	ComPtr<ID3D11DeviceContext4> _d3dDC = nullptr;

	HINSTANCE _hInst;
	std::string_view _scaleModel;
	int _captureMode;
	int _bufferPrecision;
	bool _showFPS;
	bool _adjustCursorSpeed;
	bool _noDisturb;

	HWND _hwndSrc;
	HWND _hwndHost = NULL;

	RECT _srcClient;
	RECT _hostClient{};

	D2D_RECT_F _destRect{};

	ComPtr<IWICImagingFactory2> _wicImgFactory = nullptr;
	ComPtr<IDWriteFactory> _dwFactory = nullptr;
};
