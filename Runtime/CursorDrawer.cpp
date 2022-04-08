#include "pch.h"
#include "CursorDrawer.h"
#include "App.h"
#include "Utils.h"
#include <VertexTypes.h>

extern std::shared_ptr<spdlog::logger> logger;

constexpr const char* monochromeCursorPS = R"(
Texture2D originTex : register(t0);
Texture2D maskTex : register(t1);
SamplerState sam : register(s0);

float4 main(float2 coord : TEXCOORD) : SV_Target{
	float2 masks = maskTex.Sample(sam, coord).xy;
	if (masks.x > 0.5) {
		float3 origin = originTex.Sample(sam, coord).rgb;

		if (masks.y > 0.5) {
			return float4(1 - origin, 1);
		} else {
			return float4(origin, 1);
		}
	} else {
		if (masks.y > 0.5) {
			return float4(1, 1, 1, 1);
		} else {
			return float4(0, 0, 0, 1);
		}
	}
}
)";

bool CursorDrawer::Initialize(ComPtr<ID3D11Texture2D> renderTarget, const RECT& destRect) {
	App& app = App::GetInstance();
	if (!app.IsNoCursor()) {
		Renderer& renderer = app.GetRenderer();
		_d3dDC = renderer.GetD3DDC();
		_d3dDevice = renderer.GetD3DDevice();

		_zoomFactorX = _zoomFactorY = App::GetInstance().GetCursorZoomFactor();
		if (_zoomFactorX <= 0) {
			D3D11_TEXTURE2D_DESC desc{};
			app.GetFrameSource().GetOutput()->GetDesc(&desc);
			_zoomFactorX = float(destRect.right - destRect.left) / desc.Width;
			_zoomFactorY = float(destRect.bottom - destRect.top) / desc.Height;
		}

		if (!renderer.GetRenderTargetView(renderTarget.Get(), &_rtv)) {
			SPDLOG_LOGGER_ERROR(logger, "GetRenderTargetView 失败");
			return false;
		}

		if (!renderer.GetShaderResourceView(renderTarget.Get(), &_renderTargetSrv)) {
			SPDLOG_LOGGER_ERROR(logger, "GetShaderResourceView 失败");
			return false;
		}

		ComPtr<ID3DBlob> blob;
		if (!renderer.CompileShader(false, monochromeCursorPS, "main", &blob, "MonochromeCursorPS")) {
			SPDLOG_LOGGER_ERROR(logger, "编译 MonochromeCursorPS 失败");
			return false;
		}

		HRESULT hr = renderer.GetD3DDevice()->CreatePixelShader(
			blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_monoCursorPS);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 MonochromeCursorPS 失败", hr));
			return false;
		}

		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(VertexPositionTexture) * 4;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		hr = renderer.GetD3DDevice()->CreateBuffer(&bd, nullptr, &_vtxBuffer);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建顶点缓冲区失败", hr));
			return false;
		}

		if (!renderer.GetSampler(EffectSamplerFilterType::Linear, EffectSamplerAddressType::Clamp, &_linearSam)
			|| !renderer.GetSampler(EffectSamplerFilterType::Point, EffectSamplerAddressType::Clamp, &_pointSam)
		) {
			SPDLOG_LOGGER_ERROR(logger, "GetSampler 失败");
			return false;
		}

		_monoCursorSize = { GetSystemMetrics(SM_CXCURSOR), GetSystemMetrics(SM_CYCURSOR) };

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = lroundf(_monoCursorSize.cx * _zoomFactorX);
		desc.Height = lroundf(_monoCursorSize.cy * _zoomFactorY);
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		desc.Usage = D3D11_USAGE_DEFAULT;

		hr = _d3dDevice->CreateTexture2D(&desc, nullptr, &_monoTmpTexture);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
			return false;
		}

		if (!renderer.GetRenderTargetView(_monoTmpTexture.Get(), &_monoTmpRtv)) {
			SPDLOG_LOGGER_ERROR(logger, "GetRenderTargetView 失败");
			return false;
		}

		if (!renderer.GetShaderResourceView(_monoTmpTexture.Get(), &_monoTmpSrv)) {
			SPDLOG_LOGGER_ERROR(logger, "GetShaderResourceView 失败");
			return false;
		}

		D3D11_TEXTURE2D_DESC rtDesc;
		renderTarget->GetDesc(&rtDesc);

		_renderTargetSize = { (long)rtDesc.Width, (long)rtDesc.Height };
		_destRect = destRect;
	}

	const RECT& srcFrameRect = app.GetSrcFrameRect();
	SIZE srcSize = { srcFrameRect.right - srcFrameRect.left, srcFrameRect.bottom - srcFrameRect.top };

	_clientScaleX = float(destRect.right - destRect.left) / srcSize.cx;
	_clientScaleY = float(destRect.bottom - destRect.top) / srcSize.cy;
	
	if (!App::GetInstance().IsMultiMonitorMode() && !App::GetInstance().IsBreakpointMode()) {
		// 非多屏幕模式下，限制光标在窗口内
		
		if (!ClipCursor(&srcFrameRect)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("ClipCursor 失败"));
		}

		if (App::GetInstance().IsAdjustCursorSpeed()) {
			_AdjustCursorSpeed();
		}

		if (!MagShowSystemCursor(FALSE)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("MagShowSystemCursor 失败"));
		}
	}

	SPDLOG_LOGGER_INFO(logger, "CursorDrawer 初始化完成");
	return true;
}

CursorDrawer::~CursorDrawer() {
	if (App::GetInstance().IsMultiMonitorMode()) {
		if (_isUnderCapture) {
			POINT pt{};
			if (!GetCursorPos(&pt)) {
				SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetCursorPos 失败"));
			}
			_StopCapture(pt);
		}
	} else if (!App::GetInstance().IsBreakpointMode()) {
		// CursorDrawer 析构时计时器已销毁
		ClipCursor(nullptr);
		if (App::GetInstance().IsAdjustCursorSpeed()) {
			SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)_cursorSpeed, 0);
		}

		MagShowSystemCursor(TRUE);
	}

	SPDLOG_LOGGER_INFO(logger, "CursorDrawer 已析构");
}

void CursorDrawer::_DynamicClip(POINT cursorPt) {
	const RECT& srcFrameRect = App::GetInstance().GetSrcFrameRect();
	const RECT& hostRect = App::GetInstance().GetHostWndRect();

	POINT hostPt{};

	double t = double(cursorPt.x - srcFrameRect.left) / (srcFrameRect.right - srcFrameRect.left);
	hostPt.x = std::lround(t * (_destRect.right - _destRect.left)) + _destRect.left + hostRect.left;

	t = double(cursorPt.y - srcFrameRect.top) / (srcFrameRect.bottom - srcFrameRect.top);
	hostPt.y = std::lround(t * (_destRect.bottom - _destRect.top)) + _destRect.top + hostRect.top;

	std::array<bool, 4> clips{};
	// left
	RECT rect{ LONG_MIN, hostPt.y, hostRect.left, hostPt.y + 1 };
	clips[0] = !MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);
	// top
	rect = { hostPt.x, LONG_MIN, hostPt.x + 1,hostRect.top };
	clips[1] = !MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);
	// right
	rect = { hostRect.right, hostPt.y, LONG_MAX, hostPt.y + 1 };
	clips[2] = !MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);
	// bottom
	rect = { hostPt.x, hostRect.bottom, hostPt.x + 1, LONG_MAX };
	clips[3] = !MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);

	// 开启“在 3D 游戏中限制光标”则每帧都限制一次光标
	if (App::GetInstance().IsConfineCursorIn3DGames() || clips != _curClips) {
		_curClips = clips;

		RECT clipRect = {
			clips[0] ? srcFrameRect.left : LONG_MIN,
			clips[1] ? srcFrameRect.top : LONG_MIN,
			clips[2] ? srcFrameRect.right : LONG_MAX,
			clips[3] ? srcFrameRect.bottom : LONG_MAX
		};
		ClipCursor(&clipRect);
	}
}

void CursorDrawer::_AdjustCursorSpeed() {
	if (!SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_cursorSpeed, 0)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("获取光标移速失败"));
		return;
	}

	// 鼠标加速默认打开
	bool isMouseAccelerationOn = true;
	{
		INT values[3]{ 0,0,1 };
		if (SystemParametersInfo(SPI_GETMOUSE, 0, &values, 0)) {
			isMouseAccelerationOn = values[2];
		} else {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("检索鼠标加速失败"));
		}
	}

	double scale = (_clientScaleX + _clientScaleY) / 2;

	INT newSpeed = 0;

	// “提高指针精确度”（鼠标加速）打开时光标移速的调整为线性，否则为非线性
	// 参见 https://liquipedia.net/counterstrike/Mouse_Settings#Windows_Sensitivity
	if (isMouseAccelerationOn) {
		newSpeed = std::clamp((INT)lround(_cursorSpeed / scale), 1, 20);
	} else {
		static constexpr std::array<double, 20> SENSITIVITIES = {
			0.03125, 0.0625, 0.125, 0.25, 0.375, 0.5, 0.625, 0.75, 0.875,
			1.0, 1.25, 1.5, 1.75, 2, 2.25, 2.5, 2.75, 3, 3.25, 3.5
		};

		_cursorSpeed = std::clamp(_cursorSpeed, 1, 20);
		double newSensitivity = SENSITIVITIES[static_cast<size_t>(_cursorSpeed) - 1] / scale;

		auto it = std::lower_bound(SENSITIVITIES.begin(), SENSITIVITIES.end(), newSensitivity - 1e-6);
		newSpeed = INT(it - SENSITIVITIES.begin()) + 1;

		if (it != SENSITIVITIES.begin() && it != SENSITIVITIES.end()) {
			// 找到两侧最接近的数值
			if (std::abs(*it - newSensitivity) > std::abs(*(it - 1) - newSensitivity)) {
				--newSpeed;
			}
		}
	}

	if (!SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)newSpeed, 0)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("设置光标移速失败"));
	}
}

bool CursorDrawer::Update() {
	if (!App::GetInstance().IsMultiMonitorMode()) {
		return true;
	}

	// _DynamicClip 根据当前光标位置的四个方向有无屏幕来确定应该在哪些方向限制光标，但这无法
	// 处理屏幕之间存在间隙的情况。解决办法是 _StopCapture 只在目标位置存在屏幕时才取消捕获，
	// 当光标试图移动到间隙中时将被挡住。如果光标的速度足以跨越间隙，则它依然可以在屏幕间移动。

	POINT cursorPt;
	if (!GetCursorPos(&cursorPt)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetCursorPos 失败"));
		return false;
	}

	if (_isUnderCapture) {
		const RECT& srcFrameRect = App::GetInstance().GetSrcFrameRect();

		if (PtInRect(&srcFrameRect, cursorPt)) {
			_DynamicClip(cursorPt);
		} else {
			_StopCapture(cursorPt);
		}
	} else {
		const RECT& hostRect = App::GetInstance().GetHostWndRect();

		if (PtInRect(&hostRect, cursorPt)) {
			_StartCapture(cursorPt);
			_DynamicClip(cursorPt);
		}
	}

	return true;
}

bool GetHBmpBits32(HBITMAP hBmp, int& width, int& height, std::vector<BYTE>& pixels) {
	BITMAP bmp{};
	if (!GetObject(hBmp, sizeof(bmp), &bmp)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetObject 失败"));
		return false;
	}
	width = bmp.bmWidth;
	height = bmp.bmHeight;

	BITMAPINFO bi{};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = width;
	bi.bmiHeader.biHeight = -height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biSizeImage = width * height * 4;

	pixels.resize(bi.bmiHeader.biSizeImage);
	HDC hdc = GetDC(NULL);
	Utils::ScopeExit se([hdc]() {
		ReleaseDC(NULL, hdc);
	});

	if (GetDIBits(hdc, hBmp, 0, height, &pixels[0], &bi, DIB_RGB_COLORS) != height) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDIBits 失败"));
		return false;
	}

	return true;
}

bool CursorDrawer::_ResolveCursor(HCURSOR hCursor, _CursorInfo& result) const {
	assert(hCursor != NULL);

	ICONINFO ii{};
	if (!GetIconInfo(hCursor, &ii)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetIconInfo 失败"));
		return false;
	}

	result.xHotSpot = ii.xHotspot;
	result.yHotSpot = ii.yHotspot;

	Utils::ScopeExit se([&ii]() {
		if (ii.hbmColor) {
			DeleteBitmap(ii.hbmColor);
		}
		DeleteBitmap(ii.hbmMask);
	});

	ComPtr<ID3D11Texture2D> texture;

	if(ii.hbmColor == NULL) {
		// 单色光标
		BITMAP bmp{};
		if (!GetObject(ii.hbmMask, sizeof(bmp), &bmp)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetObject 失败"));
			return false;
		}
		result.width = bmp.bmWidth;
		result.height = bmp.bmHeight;

		BITMAPINFO bi{};
		bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bi.bmiHeader.biWidth = result.width;
		bi.bmiHeader.biHeight = -(LONG)result.height;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biCompression = BI_RGB;
		bi.bmiHeader.biBitCount = 32;
		bi.bmiHeader.biSizeImage = result.width * result.height * 4;

		std::vector<BYTE> pixels(bi.bmiHeader.biSizeImage);
		HDC hdc = GetDC(NULL);
		if (GetDIBits(hdc, ii.hbmMask, 0, result.height, &pixels[0], &bi, DIB_RGB_COLORS) != result.height) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDIBits 失败"));
			ReleaseDC(NULL, hdc);
			return false;
		}
		ReleaseDC(NULL, hdc);

		// 红色通道是 AND 掩码，绿色通道是 XOR 掩码
		const int halfSize = bi.bmiHeader.biSizeImage / 8;
		BYTE* upPtr = &pixels[1];
		BYTE* downPtr = &pixels[static_cast<size_t>(halfSize) * 4];
		for (int i = 0; i < halfSize; ++i) {
			*upPtr = *downPtr;
			// 判断光标是否存在反色部分
			if (!result.hasInv && *(upPtr - 1) == 255 && *upPtr == 255) {
				result.hasInv = true;
			}

			upPtr += 4;
			downPtr += 4;
		}

		if (result.hasInv) {
			SPDLOG_LOGGER_INFO(logger, "此光标含反色部分");
			result.height /= 2;

			if (result.width != _monoCursorSize.cx || result.height != _monoCursorSize.cy) {
				SPDLOG_LOGGER_ERROR(logger, "单色光标的尺寸不合法");
				return false;
			}
			
			D3D11_TEXTURE2D_DESC desc{};
			desc.Width = result.width;
			desc.Height = result.height;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.Usage = D3D11_USAGE_DEFAULT;

			D3D11_SUBRESOURCE_DATA initData{};
			initData.pSysMem = &pixels[0];
			initData.SysMemPitch = result.width * 4;

			HRESULT hr = _d3dDevice->CreateTexture2D(&desc, &initData, &texture);
			if (FAILED(hr)) {
				SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
				return false;
			}
		}
	}

	if(!result.hasInv) {
		// 光标无反色部分，使用 WIC 将光标转换为带 Alpha 通道的图像
		ComPtr<IWICImagingFactory2> wicFactory = App::GetInstance().GetWICImageFactory();
		if (!wicFactory) {
			SPDLOG_LOGGER_ERROR(logger, "获取 WICImageFactory 失败");
			return false;
		}

		ComPtr<IWICBitmap> wicBitmap;
		HRESULT hr = wicFactory->CreateBitmapFromHICON(hCursor, &wicBitmap);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateBitmapFromHICON 失败", hr));
			return false;
		}

		hr = wicBitmap->GetSize(&result.width, &result.height);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("GetSize 失败", hr));
			return false;
		}
		
		std::vector<BYTE> pixels(result.width * result.height * 4);
		hr = wicBitmap->CopyPixels(nullptr, result.width * 4, (UINT)pixels.size(), pixels.data());
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CopyPixels 失败", hr));
			return false;
		}

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = result.width;
		desc.Height = result.height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.Usage = D3D11_USAGE_IMMUTABLE;

		D3D11_SUBRESOURCE_DATA initData{};
		initData.pSysMem = &pixels[0];
		initData.SysMemPitch = result.width * 4;

		hr = _d3dDevice->CreateTexture2D(&desc, &initData, &texture);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
			return false;
		}
	}

	HRESULT hr = _d3dDevice->CreateShaderResourceView(texture.Get(), nullptr, &result.texture);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 ShaderResourceView 失败", hr));
		return false;
	}

	return true;
}

void CursorDrawer::_StartCapture(POINT cursorPt) {
	// 在以下情况下进入捕获状态：
	// 1. 当前未捕获
	// 2. 光标进入全屏区域
	// 
	// 进入捕获状态时：
	// 1. 调整光标速度，全局隐藏光标
	// 2. 将光标移到源窗口的对应位置
	//
	// 在有黑边的情况下自动将光标调整到画面内

	// 全局隐藏光标
	if (!MagShowSystemCursor(FALSE)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("MagShowSystemCursor 失败"));
	}

	if (App::GetInstance().IsAdjustCursorSpeed()) {
		_AdjustCursorSpeed();
	}

	// 移动光标位置
	const RECT& srcFrameRect = App::GetInstance().GetSrcFrameRect();
	const RECT& hostRect = App::GetInstance().GetHostWndRect();
	// 跳过黑边
	cursorPt.x = std::clamp(cursorPt.x, hostRect.left + _destRect.left, hostRect.left + _destRect.right - 1);
	cursorPt.y = std::clamp(cursorPt.y, hostRect.top + _destRect.top, hostRect.top + _destRect.bottom - 1);

	// 从全屏窗口映射到源窗口
	double posX = double(cursorPt.x - hostRect.left - _destRect.left) / (_destRect.right - _destRect.left);
	double posY = double(cursorPt.y - hostRect.top - _destRect.top) / (_destRect.bottom - _destRect.top);

	posX = posX * (srcFrameRect.right - srcFrameRect.left) + srcFrameRect.left;
	posY = posY * (srcFrameRect.bottom - srcFrameRect.top) + srcFrameRect.top;

	posX = std::clamp<double>(posX, srcFrameRect.left, srcFrameRect.right - 1);
	posY = std::clamp<double>(posY, srcFrameRect.top, srcFrameRect.bottom - 1);

	SetCursorPos(std::lround(posX), std::lround(posY));

	_isUnderCapture = true;
}

void CursorDrawer::_StopCapture(POINT cursorPt) {
	// 在以下情况下离开捕获状态：
	// 1. 当前处于捕获状态
	// 2. 光标离开源窗口客户区
	// 3. 目标位置存在屏幕
	//
	// 离开捕获状态时
	// 1. 还原光标速度，全局显示光标
	// 2. 将光标移到全屏窗口外的对应位置
	//
	// 在有黑边的情况下自动将光标调整到全屏窗口外

	const RECT& srcFrameRect = App::GetInstance().GetSrcFrameRect();
	const RECT& hostRect = App::GetInstance().GetHostWndRect();

	POINT newCursorPt{};

	if (cursorPt.x >= srcFrameRect.right) {
		newCursorPt.x = hostRect.right + cursorPt.x - srcFrameRect.right + 1;
	} else if (cursorPt.x < srcFrameRect.left) {
		newCursorPt.x = hostRect.left + cursorPt.x - srcFrameRect.left;
	} else {
		double pos = double(cursorPt.x - srcFrameRect.left) / (srcFrameRect.right - srcFrameRect.left);
		newCursorPt.x = std::lround(pos * (_destRect.right - _destRect.left)) + _destRect.left + hostRect.left;
	}

	if (cursorPt.y >= srcFrameRect.bottom) {
		newCursorPt.y = hostRect.bottom + cursorPt.y - srcFrameRect.bottom + 1;
	} else if (cursorPt.y < srcFrameRect.top) {
		newCursorPt.y = hostRect.top + cursorPt.y - srcFrameRect.top;
	} else {
		double pos = double(cursorPt.y - srcFrameRect.top) / (srcFrameRect.bottom - srcFrameRect.top);
		newCursorPt.y = std::lround(pos * (_destRect.bottom - _destRect.top)) + _destRect.top + hostRect.top;
	}

	if (MonitorFromPoint(newCursorPt, MONITOR_DEFAULTTONULL)) {
		ClipCursor(nullptr);
		_curClips = {};

		SetCursorPos(newCursorPt.x, newCursorPt.y);

		if (App::GetInstance().IsAdjustCursorSpeed()) {
			SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)_cursorSpeed, 0);
		}

		if (!MagShowSystemCursor(TRUE)) {
			SPDLOG_LOGGER_ERROR(logger, "MagShowSystemCursor 失败");
		}
		// WGC 捕获模式会随机使 MagShowSystemCursor(TRUE) 失效，重新加载光标可以解决这个问题
		SystemParametersInfo(SPI_SETCURSORS, 0, 0, 0);

		_isUnderCapture = false;
	} else {
		// 目标位置不存在屏幕，则将光标限制在源窗口内
		SetCursorPos(
		 std::clamp(cursorPt.x, srcFrameRect.left, srcFrameRect.right - 1),
		 std::clamp(cursorPt.y, srcFrameRect.top, srcFrameRect.bottom - 1)
		);
	}
}

void CursorDrawer::Draw() {
	if (App::GetInstance().IsNoCursor()) {
		// 不绘制光标
		return;
	}

	if (App::GetInstance().IsMultiMonitorMode()) {
		// 多屏幕模式下不处于捕获状态则不绘制光标
		if (!_isUnderCapture) {
			return;
		}
	} else if (!App::GetInstance().IsBreakpointMode() && App::GetInstance().IsConfineCursorIn3DGames()) {
		// 开启“在 3D 游戏中限制光标”则每帧都限制一次光标
		ClipCursor(&App::GetInstance().GetSrcFrameRect());
	}

	CURSORINFO ci{};
	ci.cbSize = sizeof(ci);
	if (!GetCursorInfo(&ci)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetCursorInfo 失败"));
		return;
	}

	if (!ci.hCursor || ci.flags != CURSOR_SHOWING) {
		return;
	}

	_CursorInfo* info = nullptr;

	auto it = _cursorMap.find(ci.hCursor);
	if (it != _cursorMap.end()) {
		info = &it->second;
	} else {
		// 未在映射中找到，创建新映射
		_CursorInfo t;
		if (_ResolveCursor(ci.hCursor, t)) {
			info = &_cursorMap[ci.hCursor];
			*info = t;

			SPDLOG_LOGGER_INFO(logger, fmt::format("已解析光标：{}", (void*)ci.hCursor));
		} else {
			SPDLOG_LOGGER_ERROR(logger, "解析光标失败");
			return;
		}
	}

	SIZE cursorSize = { lroundf(info->width * _zoomFactorX), lroundf(info->height * _zoomFactorY) };

	// 映射坐标
	const RECT& srcClient = App::GetInstance().GetSrcFrameRect();
	POINT targetScreenPos = {
		lroundf((ci.ptScreenPos.x - srcClient.left) * _clientScaleX - info->xHotSpot * _zoomFactorX),
		lroundf((ci.ptScreenPos.y - srcClient.top) * _clientScaleY - info->yHotSpot * _zoomFactorY)
	};

	RECT cursorRect{
		targetScreenPos.x + _destRect.left,
		targetScreenPos.y + _destRect.top,
		targetScreenPos.x + cursorSize.cx + _destRect.left,
		targetScreenPos.y + cursorSize.cy + _destRect.top
	};

	if (cursorRect.right <= _destRect.left || cursorRect.bottom <= _destRect.top
		|| cursorRect.left >= _destRect.right || cursorRect.top >= _destRect.bottom
	) {
		// 光标在窗口外
		return;
	}


	float left = targetScreenPos.x / FLOAT(_destRect.right - _destRect.left) * 2 - 1.0f;
	float top = 1.0f - targetScreenPos.y / FLOAT(_destRect.bottom - _destRect.top) * 2;
	float right = left + cursorSize.cx / FLOAT(_destRect.right - _destRect.left) * 2;
	float bottom = top - cursorSize.cy / FLOAT(_destRect.bottom - _destRect.top) * 2;

	Renderer& renderer = App::GetInstance().GetRenderer();
	renderer.SetSimpleVS(_vtxBuffer.Get());
	_d3dDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	
	if (!info->hasInv) {
		_d3dDC->OMSetRenderTargets(1, &_rtv, nullptr);
		D3D11_VIEWPORT vp{
			(FLOAT)_destRect.left,
			(FLOAT)_destRect.top,
			FLOAT(_destRect.right - _destRect.left),
			FLOAT(_destRect.bottom - _destRect.top),
			0.0f,
			1.0f
		};
		_d3dDC->RSSetViewports(1, &vp);

		D3D11_MAPPED_SUBRESOURCE ms;
		HRESULT hr = _d3dDC->Map(_vtxBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("Map 失败", hr));
			return;
		}

		VertexPositionTexture* data = (VertexPositionTexture*)ms.pData;
		data[0] = { XMFLOAT3(left, top, 0.5f), XMFLOAT2(0.0f, 0.0f) };
		data[1] = { XMFLOAT3(right, top, 0.5f), XMFLOAT2(1.0f, 0.0f) };
		data[2] = { XMFLOAT3(left, bottom, 0.5f), XMFLOAT2(0.0f, 1.0f) };
		data[3] = { XMFLOAT3(right, bottom, 0.5f), XMFLOAT2(1.0f, 1.0f) };

		_d3dDC->Unmap(_vtxBuffer.Get(), 0);

		if (!renderer.SetCopyPS(
			App::GetInstance().GetCursorInterpolationMode() == 0 ? _pointSam : _linearSam,
			info->texture.Get())
		) {
			SPDLOG_LOGGER_ERROR(logger, "SetCopyPS 失败");
			return;
		}
		if (!renderer.SetAlphaBlend(true)) {
			SPDLOG_LOGGER_ERROR(logger, "SetAlphaBlend 失败");
			return;
		}
		_d3dDC->Draw(4, 0);

		if (!renderer.SetAlphaBlend(false)) {
			SPDLOG_LOGGER_ERROR(logger, "SetAlphaBlend 失败");
			return;
		}
	} else {
		// 绘制带有反色部分的光标，首先将光标覆盖的纹理复制到 _monoTmpTexture 中
		// 不知为何 CopySubresourceRegion 会大幅增加 GPU 占用
		_d3dDC->OMSetRenderTargets(1, &_monoTmpRtv, nullptr);
		D3D11_VIEWPORT vp{};
		vp.Width = (FLOAT)cursorSize.cx;
		vp.Height = (FLOAT)cursorSize.cy;
		vp.MaxDepth = 1.0f;
		_d3dDC->RSSetViewports(1, &vp);
		
		D3D11_MAPPED_SUBRESOURCE ms;
		HRESULT hr = _d3dDC->Map(_vtxBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("Map 失败", hr));
			return;
		}

		VertexPositionTexture* data = (VertexPositionTexture*)ms.pData;
		float leftPos = (float)cursorRect.left / _renderTargetSize.cx;
		float rightPos = (float)cursorRect.right / _renderTargetSize.cx;
		float topPos = (float)cursorRect.top / _renderTargetSize.cy;
		float bottomPos = (float)cursorRect.bottom / _renderTargetSize.cy;
		data[0] = { XMFLOAT3(-1, 1, 0.5f), XMFLOAT2(leftPos, topPos) };
		data[1] = { XMFLOAT3(1, 1, 0.5f), XMFLOAT2(rightPos, topPos) };
		data[2] = { XMFLOAT3(-1, -1, 0.5f), XMFLOAT2(leftPos, bottomPos) };
		data[3] = { XMFLOAT3(1, -1, 0.5f), XMFLOAT2(rightPos, bottomPos) };

		_d3dDC->Unmap(_vtxBuffer.Get(), 0);

		if (!renderer.SetCopyPS(_pointSam, _renderTargetSrv)) {
			SPDLOG_LOGGER_ERROR(logger, "SetCopyPS 失败");
			return;
		}
		_d3dDC->Draw(4, 0);
		
		// 绘制光标
		hr = _d3dDC->Map(_vtxBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("Map 失败", hr));
			return;
		}

		data = (VertexPositionTexture*)ms.pData;
		data[0] = { XMFLOAT3(left, top, 0.5f), XMFLOAT2(0.0f, 0.0f) };
		data[1] = { XMFLOAT3(right, top, 0.5f), XMFLOAT2(1.0f, 0.0f) };
		data[2] = { XMFLOAT3(left, bottom, 0.5f), XMFLOAT2(0.0f, 1.0f) };
		data[3] = { XMFLOAT3(right, bottom, 0.5f), XMFLOAT2(1.0f, 1.0f) };

		_d3dDC->Unmap(_vtxBuffer.Get(), 0);

		_d3dDC->PSSetShader(_monoCursorPS.Get(), nullptr, 0);
		_d3dDC->PSSetConstantBuffers(0, 0, nullptr);
		_d3dDC->OMSetRenderTargets(0, nullptr, nullptr);
		ID3D11ShaderResourceView* srv[2] = { _monoTmpSrv, info->texture.Get() };
		_d3dDC->PSSetShaderResources(0, 2, srv);
		_d3dDC->PSSetSamplers(0, 1, App::GetInstance().GetCursorInterpolationMode() == 0 ? &_pointSam : &_linearSam);

		_d3dDC->OMSetRenderTargets(1, &_rtv, nullptr);
		vp.TopLeftX = (FLOAT)_destRect.left;
		vp.TopLeftY = (FLOAT)_destRect.top;
		vp.Width = FLOAT(_destRect.right - _destRect.left);
		vp.Height = FLOAT(_destRect.bottom - _destRect.top);
		_d3dDC->RSSetViewports(1, &vp);

		_d3dDC->Draw(4, 0);
	}
}
