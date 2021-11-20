#include "pch.h"
#include "TextureLoader.h"
#include <DDSTextureLoader.h>


extern std::shared_ptr<spdlog::logger> logger;


ComPtr<ID3D11Texture2D> LoadImg(const wchar_t* fileName) {
	ComPtr<IWICImagingFactory2> factory = App::GetInstance().GetWICImageFactory();
	if (!factory) {
		SPDLOG_LOGGER_ERROR(logger, "GetWICImageFactory 失败");
		return nullptr;
	}

	// 读取图像文件
	ComPtr<IWICBitmapDecoder> decoder;
	HRESULT hr = factory->CreateDecoderFromFilename(fileName, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateDecoderFromFilename 失败", hr));
		return nullptr;
	}

	ComPtr<IWICBitmapFrameDecode> frame;
	hr = decoder->GetFrame(0, &frame);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("IWICBitmapFrameDecode::GetFrame 失败", hr));
		return nullptr;
	}

	bool useFloatFormat = false;
	{
		WICPixelFormatGUID sourceFormat;
		hr = frame->GetPixelFormat(&sourceFormat);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("GetPixelFormat 失败", hr));
			return nullptr;
		}

		ComPtr<IWICComponentInfo> cInfo;
		hr = factory->CreateComponentInfo(sourceFormat, &cInfo);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateComponentInfo", hr));
			return nullptr;
		}
		ComPtr<IWICPixelFormatInfo2> formatInfo;
		hr = cInfo.As<IWICPixelFormatInfo2>(&formatInfo);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("IWICComponentInfo 转换为 IWICPixelFormatInfo2 时失败", hr));
			return nullptr;
		}

		UINT bitsPerPixel;
		WICPixelFormatNumericRepresentation type;
		hr = formatInfo->GetBitsPerPixel(&bitsPerPixel);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("GetBitsPerPixel", hr));
			return nullptr;
		}
		hr = formatInfo->GetNumericRepresentation(&type);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("GetNumericRepresentation", hr));
			return nullptr;
		}

		useFloatFormat = bitsPerPixel > 32 || type == WICPixelFormatNumericRepresentationFixed || type == WICPixelFormatNumericRepresentationFloat;
	}

	// 转换格式
	ComPtr<IWICFormatConverter> formatConverter;
	hr = factory->CreateFormatConverter(&formatConverter);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateFormatConverter 失败", hr));
		return nullptr;
	}

	WICPixelFormatGUID targetFormat = useFloatFormat ? GUID_WICPixelFormat64bppRGBAHalf : GUID_WICPixelFormat32bppBGRA;
	hr = formatConverter->Initialize(frame.Get(), targetFormat, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("IWICFormatConverter::Initialize 失败", hr));
		return nullptr;
	}

	// 检查 D3D 纹理尺寸限制
	UINT width, height;
	hr = formatConverter->GetSize(&width, &height);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("GetSize 失败", hr));
		return nullptr;
	}

	switch (App::GetInstance().GetRenderer().GetFeatureLevel()) {
	case D3D_FEATURE_LEVEL_10_0:
	case D3D_FEATURE_LEVEL_10_1:
		if (width > 8192 || height > 8192) {
			SPDLOG_LOGGER_ERROR(logger, "图像尺寸超出限制");
			return nullptr;
		}
		break;
	default:
		if (width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION) {
			SPDLOG_LOGGER_ERROR(logger, "图像尺寸超出限制");
			return nullptr;
		}
	}

	UINT stride = width * (useFloatFormat ? 8 : 4);
	UINT size = stride * height;
	std::unique_ptr<BYTE[]> buf(new BYTE[size]);

	hr = formatConverter->CopyPixels(nullptr, stride, size, buf.get());
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CopyPixels 失败", hr));
		return nullptr;
	}

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = useFloatFormat ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = buf.get();
	initData.SysMemPitch = stride;

	ComPtr<ID3D11Texture2D> result;
	hr = App::GetInstance().GetRenderer().GetD3DDevice()->CreateTexture2D(&desc, &initData, &result);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateTexture2D 失败", hr));
		return nullptr;
	}

	return result;
}

ComPtr<ID3D11Texture2D> LoadDDS(const wchar_t* fileName) {
	ComPtr<ID3D11Resource> result;

	DirectX::DDS_ALPHA_MODE alphaMode = DirectX::DDS_ALPHA_MODE_STRAIGHT;
	HRESULT hr = DirectX::CreateDDSTextureFromFileEx(
		App::GetInstance().GetRenderer().GetD3DDevice().Get(),
		fileName,
		0,
		D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
		0,
		0,
		false,
		&result,
		nullptr,
		&alphaMode
	);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_INFO(logger, MakeComErrorMsg("CreateDDSTextureFromFile 失败，将尝试不作为渲染目标", hr));

		// 第二次尝试，不作为渲染目标
		hr = DirectX::CreateDDSTextureFromFileEx(
			App::GetInstance().GetRenderer().GetD3DDevice().Get(),
			fileName,
			0,
			D3D11_USAGE_DEFAULT,
			D3D11_BIND_SHADER_RESOURCE,
			0,
			0,
			false,
			&result,
			nullptr,
			&alphaMode
		);

		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateDDSTextureFromFile 失败", hr));
			return nullptr;
		}
	}

	ComPtr<ID3D11Texture2D> tex;
	hr = result.As<ID3D11Texture2D>(&tex);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从 ID3D11Resource 获取 ID3D11Texture2D 失败", hr));
		return nullptr;
	}

	return tex;
}

ComPtr<ID3D11Texture2D> TextureLoader::Load(const wchar_t* fileName) {
	std::wstring_view sv(fileName);
	size_t npos = sv.find_last_of(L'.');
	if (npos == std::wstring_view::npos) {
		SPDLOG_LOGGER_ERROR(logger, "文件名无后缀名");
		return nullptr;
	}

	std::wstring_view suffix = sv.substr(npos + 1);
	
	static std::unordered_map<std::wstring_view, ComPtr<ID3D11Texture2D>(*)(const wchar_t*)> funcs = {
		{L"bmp", LoadImg},
		{L"jpg", LoadImg},
		{L"jpeg", LoadImg},
		{L"png", LoadImg},
		{L"tif", LoadImg},
		{L"tiff", LoadImg},
		{L"dds", LoadDDS}
	};

	auto it = funcs.find(suffix);
	if (it == funcs.end()) {
		SPDLOG_LOGGER_ERROR(logger, "不支持读取该格式");
		return nullptr;
	}

	return it->second(fileName);
}
