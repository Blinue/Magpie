#include "pch.h"
#include "CursorDrawer.h"
#include "App.h"
#include "Utils.h"
#include <VertexTypes.h>
#include "Renderer.h"
#include "FrameSourceBase.h"
#include "DeviceResources.h"
#include "Logger.h"


constexpr const char* monochromeCursorPS = R"(
Texture2D originTex : register(t0);
Texture2D maskTex : register(t1);
SamplerState sam : register(s0);

float4 main(float4 pos : SV_POSITION, float2 coord : TEXCOORD) : SV_Target{
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

bool CursorDrawer::Initialize(ID3D11Texture2D* renderTarget) {
	App& app = App::GetInstance();
	auto& dr = app.GetDeviceResources();

	const RECT& outputRect = App::GetInstance().GetRenderer().GetOutputRect();

	if (!app.IsNoCursor()) {
		Renderer& renderer = app.GetRenderer();
		auto d3dDC = dr.GetD3DDC();
		auto d3dDevice = dr.GetD3DDevice();

		_zoomFactorX = _zoomFactorY = App::GetInstance().GetCursorZoomFactor();
		if (_zoomFactorX <= 0) {
			D3D11_TEXTURE2D_DESC desc{};
			app.GetFrameSource().GetOutput()->GetDesc(&desc);
			_zoomFactorX = float(outputRect.right - outputRect.left) / desc.Width;
			_zoomFactorY = float(outputRect.bottom - outputRect.top) / desc.Height;
		}

		if (!dr.GetRenderTargetView(renderTarget, &_rtv)) {
			Logger::Get().Error("GetRenderTargetView 失败");
			return false;
		}

		if (!dr.GetShaderResourceView(renderTarget, &_renderTargetSrv)) {
			Logger::Get().Error("GetShaderResourceView 失败");
			return false;
		}

		winrt::com_ptr<ID3DBlob> blob;
		if (!dr.CompileShader(false, monochromeCursorPS, "main", blob.put(), "MonochromeCursorPS")) {
			Logger::Get().Error("编译 MonochromeCursorPS 失败");
			return false;
		}

		HRESULT hr = d3dDevice->CreatePixelShader(
			blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, _monoCursorPS.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 MonochromeCursorPS 失败", hr);
			return false;
		}

		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(DirectX::VertexPositionTexture) * 4;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		hr = d3dDevice->CreateBuffer(&bd, nullptr, _vtxBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建顶点缓冲区失败", hr);
			return false;
		}

		if (!dr.GetSampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, &_linearSam)
			|| !dr.GetSampler(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, &_pointSam)
		) {
			Logger::Get().Error("GetSampler 失败");
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

		hr = d3dDevice->CreateTexture2D(&desc, nullptr, _monoTmpTexture.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 Texture2D 失败", hr);
			return false;
		}

		if (!dr.GetRenderTargetView(_monoTmpTexture.get(), &_monoTmpRtv)) {
			Logger::Get().Error("GetRenderTargetView 失败");
			return false;
		}

		if (!dr.GetShaderResourceView(_monoTmpTexture.get(), &_monoTmpSrv)) {
			Logger::Get().Error("GetShaderResourceView 失败");
			return false;
		}

		D3D11_TEXTURE2D_DESC rtDesc;
		renderTarget->GetDesc(&rtDesc);

		_renderTargetSize = { (long)rtDesc.Width, (long)rtDesc.Height };
	}

	const RECT& srcFrameRect = app.GetFrameSource().GetSrcFrameRect();
	SIZE srcSize = { srcFrameRect.right - srcFrameRect.left, srcFrameRect.bottom - srcFrameRect.top };

	_clientScaleX = float(outputRect.right - outputRect.left) / srcSize.cx;
	_clientScaleY = float(outputRect.bottom - outputRect.top) / srcSize.cy;
	
	if (!App::GetInstance().IsMultiMonitorMode() && !App::GetInstance().IsBreakpointMode()) {
		// 非多屏幕模式下，限制光标在窗口内
		
		if (!ClipCursor(&srcFrameRect)) {
			Logger::Get().Win32Error("ClipCursor 失败");
		}

		if (App::GetInstance().IsAdjustCursorSpeed()) {
			// 设置鼠标移动速度
			if (SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_cursorSpeed, 0)) {
				long newSpeed = std::clamp(lroundf(_cursorSpeed / (_clientScaleX + _clientScaleY) * 2), 1L, 20L);

				if (!SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)newSpeed, 0)) {
					Logger::Get().Win32Error("设置光标移速失败");
				}
			} else {
				Logger::Get().Win32Error("获取光标移速失败");
			}

			Logger::Get().Info("已调整光标移速");
		}

		if (!MagShowSystemCursor(FALSE)) {
			Logger::Get().Win32Error("MagShowSystemCursor 失败");
		}
	}

	Logger::Get().Info("CursorDrawer 初始化完成");
	return true;
}

CursorDrawer::~CursorDrawer() {
	if (App::GetInstance().IsMultiMonitorMode()) {
		if (_isUnderCapture) {
			POINT pt{};
			if (!GetCursorPos(&pt)) {
				Logger::Get().Win32Error("GetCursorPos 失败");
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

	Logger::Get().Info("CursorDrawer 已析构");
}

void CursorDrawer::_DynamicClip(POINT cursorPt) {
	const RECT& srcFrameRect = App::GetInstance().GetFrameSource().GetSrcFrameRect();
	const RECT& hostRect = App::GetInstance().GetHostWndRect();
	const RECT& outputRect = App::GetInstance().GetRenderer().GetOutputRect();

	POINT hostPt{};

	double t = double(cursorPt.x - srcFrameRect.left) / (srcFrameRect.right - srcFrameRect.left);
	hostPt.x = std::lround(t * (outputRect.right - outputRect.left)) + outputRect.left + hostRect.left;

	t = double(cursorPt.y - srcFrameRect.top) / (srcFrameRect.bottom - srcFrameRect.top);
	hostPt.y = std::lround(t * (outputRect.bottom - outputRect.top)) + outputRect.top + hostRect.top;

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

bool CursorDrawer::Update() {
	if (!App::GetInstance().IsMultiMonitorMode()) {
		return true;
	}

	// _DynamicClip 根据当前光标位置的四个方向有无屏幕来确定应该在哪些方向限制光标，但这无法
	// 处理屏幕之间存在间隙的情况。解决办法是 _StopCapture 只在目标位置存在屏幕时才取消捕获，
	// 当光标试图移动到间隙中时将被挡住。如果光标的速度足以跨越间隙，则它依然可以在屏幕间移动。

	POINT cursorPt;
	if (!GetCursorPos(&cursorPt)) {
		Logger::Get().Win32Error("GetCursorPos 失败");
		return false;
	}

	if (_isUnderCapture) {
		const RECT& srcFrameRect = App::GetInstance().GetFrameSource().GetSrcFrameRect();

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
		Logger::Get().Win32Error("GetObject 失败");
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
		Logger::Get().Win32Error("GetDIBits 失败");
		return false;
	}

	return true;
}

bool CursorDrawer::_ResolveCursor(HCURSOR hCursor, _CursorInfo& result) const {
	assert(hCursor != NULL);

	ICONINFO ii{};
	if (!GetIconInfo(hCursor, &ii)) {
		Logger::Get().Win32Error("GetIconInfo 失败");
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

	auto d3dDevice = App::GetInstance().GetDeviceResources().GetD3DDevice();
	winrt::com_ptr<ID3D11Texture2D> texture;

	if(ii.hbmColor == NULL) {
		// 单色光标
		BITMAP bmp{};
		if (!GetObject(ii.hbmMask, sizeof(bmp), &bmp)) {
			Logger::Get().Win32Error("GetObject 失败");
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
			Logger::Get().Win32Error("GetDIBits 失败");
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
			Logger::Get().Info("此光标含反色部分");
			result.height /= 2;

			if (result.width != _monoCursorSize.cx || result.height != _monoCursorSize.cy) {
				Logger::Get().Error("单色光标的尺寸不合法");
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

			HRESULT hr = d3dDevice->CreateTexture2D(&desc, &initData, texture.put());
			if (FAILED(hr)) {
				Logger::Get().ComError("创建 Texture2D 失败", hr);
				return false;
			}
		}
	}

	if(!result.hasInv) {
		// 光标无反色部分，使用 WIC 将光标转换为带 Alpha 通道的图像
		winrt::com_ptr<IWICImagingFactory2> wicFactory = App::GetInstance().GetWICImageFactory();
		if (!wicFactory) {
			Logger::Get().Error("获取 WICImageFactory 失败");
			return false;
		}

		winrt::com_ptr<IWICBitmap> wicBitmap;
		HRESULT hr = wicFactory->CreateBitmapFromHICON(hCursor, wicBitmap.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBitmapFromHICON 失败", hr);
			return false;
		}

		hr = wicBitmap->GetSize(&result.width, &result.height);
		if (FAILED(hr)) {
			Logger::Get().ComError("GetSize 失败", hr);
			return false;
		}
		
		std::vector<BYTE> pixels(result.width * result.height * 4);
		hr = wicBitmap->CopyPixels(nullptr, result.width * 4, (UINT)pixels.size(), pixels.data());
		if (FAILED(hr)) {
			Logger::Get().ComError("CopyPixels 失败", hr);
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

		hr = d3dDevice->CreateTexture2D(&desc, &initData, texture.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 Texture2D 失败", hr);
			return false;
		}
	}

	HRESULT hr = d3dDevice->CreateShaderResourceView(texture.get(), nullptr, result.texture.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("创建 ShaderResourceView 失败", hr);
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
		Logger::Get().Win32Error("MagShowSystemCursor 失败");
	}

	if (App::GetInstance().IsAdjustCursorSpeed()) {
		// 设置鼠标移动速度
		if (SystemParametersInfo(SPI_GETMOUSESPEED, 0, &_cursorSpeed, 0)) {
			long newSpeed = std::clamp(lroundf(_cursorSpeed / (_clientScaleX + _clientScaleY) * 2), 1L, 20L);

			if (!SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)newSpeed, 0)) {
				Logger::Get().Win32Error("设置光标移速失败");
			}
		} else {
			Logger::Get().Win32Error("获取光标移速失败");
		}
	}

	// 移动光标位置
	const RECT& srcFrameRect = App::GetInstance().GetFrameSource().GetSrcFrameRect();
	const RECT& hostRect = App::GetInstance().GetHostWndRect();
	const RECT& outputRect = App::GetInstance().GetRenderer().GetOutputRect();
	// 跳过黑边
	cursorPt.x = std::clamp(cursorPt.x, hostRect.left + outputRect.left, hostRect.left + outputRect.right - 1);
	cursorPt.y = std::clamp(cursorPt.y, hostRect.top + outputRect.top, hostRect.top + outputRect.bottom - 1);

	// 从全屏窗口映射到源窗口
	double posX = double(cursorPt.x - hostRect.left - outputRect.left) / (outputRect.right - outputRect.left);
	double posY = double(cursorPt.y - hostRect.top - outputRect.top) / (outputRect.bottom - outputRect.top);

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

	const RECT& srcFrameRect = App::GetInstance().GetFrameSource().GetSrcFrameRect();
	const RECT& hostRect = App::GetInstance().GetHostWndRect();
	const RECT& outputRect = App::GetInstance().GetRenderer().GetOutputRect();

	POINT newCursorPt{};

	if (cursorPt.x >= srcFrameRect.right) {
		newCursorPt.x = hostRect.right + cursorPt.x - srcFrameRect.right + 1;
	} else if (cursorPt.x < srcFrameRect.left) {
		newCursorPt.x = hostRect.left + cursorPt.x - srcFrameRect.left;
	} else {
		double pos = double(cursorPt.x - srcFrameRect.left) / (srcFrameRect.right - srcFrameRect.left);
		newCursorPt.x = std::lround(pos * (outputRect.right - outputRect.left)) + outputRect.left + hostRect.left;
	}

	if (cursorPt.y >= srcFrameRect.bottom) {
		newCursorPt.y = hostRect.bottom + cursorPt.y - srcFrameRect.bottom + 1;
	} else if (cursorPt.y < srcFrameRect.top) {
		newCursorPt.y = hostRect.top + cursorPt.y - srcFrameRect.top;
	} else {
		double pos = double(cursorPt.y - srcFrameRect.top) / (srcFrameRect.bottom - srcFrameRect.top);
		newCursorPt.y = std::lround(pos * (outputRect.bottom - outputRect.top)) + outputRect.top + hostRect.top;
	}

	if (MonitorFromPoint(newCursorPt, MONITOR_DEFAULTTONULL)) {
		ClipCursor(nullptr);
		_curClips = {};

		SetCursorPos(newCursorPt.x, newCursorPt.y);

		if (App::GetInstance().IsAdjustCursorSpeed()) {
			SystemParametersInfo(SPI_SETMOUSESPEED, 0, (PVOID)(intptr_t)_cursorSpeed, 0);
		}

		if (!MagShowSystemCursor(TRUE)) {
			Logger::Get().Error("MagShowSystemCursor 失败");
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
		ClipCursor(&App::GetInstance().GetFrameSource().GetSrcFrameRect());
	}

	CURSORINFO ci{};
	ci.cbSize = sizeof(ci);
	if (!GetCursorInfo(&ci)) {
		Logger::Get().Win32Error("GetCursorInfo 失败");
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

			Logger::Get().Info(fmt::format("已解析光标：{}", (void*)ci.hCursor));
		} else {
			Logger::Get().Error("解析光标失败");
			return;
		}
	}

	SIZE cursorSize = { lroundf(info->width * _zoomFactorX), lroundf(info->height * _zoomFactorY) };

	// 映射坐标
	const RECT& srcClient = App::GetInstance().GetFrameSource().GetSrcFrameRect();
	POINT targetScreenPos = {
		lroundf((ci.ptScreenPos.x - srcClient.left) * _clientScaleX - info->xHotSpot * _zoomFactorX),
		lroundf((ci.ptScreenPos.y - srcClient.top) * _clientScaleY - info->yHotSpot * _zoomFactorY)
	};

	const RECT& outputRect = App::GetInstance().GetRenderer().GetOutputRect();

	RECT cursorRect{
		targetScreenPos.x + outputRect.left,
		targetScreenPos.y + outputRect.top,
		targetScreenPos.x + cursorSize.cx + outputRect.left,
		targetScreenPos.y + cursorSize.cy + outputRect.top
	};

	if (cursorRect.right <= outputRect.left || cursorRect.bottom <= outputRect.top
		|| cursorRect.left >= outputRect.right || cursorRect.top >= outputRect.bottom
	) {
		// 光标在窗口外
		return;
	}


	float left = targetScreenPos.x / FLOAT(outputRect.right - outputRect.left) * 2 - 1.0f;
	float top = 1.0f - targetScreenPos.y / FLOAT(outputRect.bottom - outputRect.top) * 2;
	float right = left + cursorSize.cx / FLOAT(outputRect.right - outputRect.left) * 2;
	float bottom = top - cursorSize.cy / FLOAT(outputRect.bottom - outputRect.top) * 2;

	Renderer& renderer = App::GetInstance().GetRenderer();
	renderer.SetSimpleVS(_vtxBuffer.get());
	auto d3dDC = App::GetInstance().GetDeviceResources().GetD3DDC();

	d3dDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	
	using namespace DirectX;

	if (!info->hasInv) {
		d3dDC->OMSetRenderTargets(1, &_rtv, nullptr);
		D3D11_VIEWPORT vp{
			(FLOAT)outputRect.left,
			(FLOAT)outputRect.top,
			FLOAT(outputRect.right - outputRect.left),
			FLOAT(outputRect.bottom - outputRect.top),
			0.0f,
			1.0f
		};
		d3dDC->RSSetViewports(1, &vp);

		D3D11_MAPPED_SUBRESOURCE ms;
		HRESULT hr = d3dDC->Map(_vtxBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		if (FAILED(hr)) {
			Logger::Get().ComError("Map 失败", hr);
			return;
		}

		VertexPositionTexture* data = (VertexPositionTexture*)ms.pData;
		data[0] = { XMFLOAT3(left, top, 0.5f), XMFLOAT2(0.0f, 0.0f) };
		data[1] = { XMFLOAT3(right, top, 0.5f), XMFLOAT2(1.0f, 0.0f) };
		data[2] = { XMFLOAT3(left, bottom, 0.5f), XMFLOAT2(0.0f, 1.0f) };
		data[3] = { XMFLOAT3(right, bottom, 0.5f), XMFLOAT2(1.0f, 1.0f) };

		d3dDC->Unmap(_vtxBuffer.get(), 0);

		if (!renderer.SetCopyPS(
			App::GetInstance().GetCursorInterpolationMode() == 0 ? _pointSam : _linearSam,
			info->texture.get())
		) {
			Logger::Get().Error("SetCopyPS 失败");
			return;
		}
		if (!renderer.SetAlphaBlend(true)) {
			Logger::Get().Error("SetAlphaBlend 失败");
			return;
		}
		d3dDC->Draw(4, 0);

		if (!renderer.SetAlphaBlend(false)) {
			Logger::Get().Error("SetAlphaBlend 失败");
			return;
		}
	} else {
		// 绘制带有反色部分的光标，首先将光标覆盖的纹理复制到 _monoTmpTexture 中
		// 不知为何 CopySubresourceRegion 会大幅增加 GPU 占用
		d3dDC->OMSetRenderTargets(1, &_monoTmpRtv, nullptr);
		D3D11_VIEWPORT vp{};
		vp.Width = (FLOAT)cursorSize.cx;
		vp.Height = (FLOAT)cursorSize.cy;
		vp.MaxDepth = 1.0f;
		d3dDC->RSSetViewports(1, &vp);
		
		D3D11_MAPPED_SUBRESOURCE ms;
		HRESULT hr = d3dDC->Map(_vtxBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		if (FAILED(hr)) {
			Logger::Get().ComError("Map 失败", hr);
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

		d3dDC->Unmap(_vtxBuffer.get(), 0);

		if (!renderer.SetCopyPS(_pointSam, _renderTargetSrv)) {
			Logger::Get().Error("SetCopyPS 失败");
			return;
		}
		d3dDC->Draw(4, 0);
		
		// 绘制光标
		hr = d3dDC->Map(_vtxBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
		if (FAILED(hr)) {
			Logger::Get().ComError("Map 失败", hr);
			return;
		}

		data = (VertexPositionTexture*)ms.pData;
		data[0] = { XMFLOAT3(left, top, 0.5f), XMFLOAT2(0.0f, 0.0f) };
		data[1] = { XMFLOAT3(right, top, 0.5f), XMFLOAT2(1.0f, 0.0f) };
		data[2] = { XMFLOAT3(left, bottom, 0.5f), XMFLOAT2(0.0f, 1.0f) };
		data[3] = { XMFLOAT3(right, bottom, 0.5f), XMFLOAT2(1.0f, 1.0f) };

		d3dDC->Unmap(_vtxBuffer.get(), 0);

		d3dDC->PSSetShader(_monoCursorPS.get(), nullptr, 0);
		d3dDC->PSSetConstantBuffers(0, 0, nullptr);
		d3dDC->OMSetRenderTargets(0, nullptr, nullptr);
		ID3D11ShaderResourceView* srv[2] = { _monoTmpSrv, info->texture.get() };
		d3dDC->PSSetShaderResources(0, 2, srv);
		d3dDC->PSSetSamplers(0, 1, App::GetInstance().GetCursorInterpolationMode() == 0 ? &_pointSam : &_linearSam);

		d3dDC->OMSetRenderTargets(1, &_rtv, nullptr);
		vp.TopLeftX = (FLOAT)outputRect.left;
		vp.TopLeftY = (FLOAT)outputRect.top;
		vp.Width = FLOAT(outputRect.right - outputRect.left);
		vp.Height = FLOAT(outputRect.bottom - outputRect.top);
		d3dDC->RSSetViewports(1, &vp);

		d3dDC->Draw(4, 0);
	}
}
