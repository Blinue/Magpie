// MPVHookTextureParser.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#define NOMINMAX
#include <iostream>
#include <fstream>
#include <vector>
#include <DirectXTex.h>
#include <DirectXPackedVector.h>
#include <string_view>


std::wstring UTF8ToUTF16(std::string_view str) {
	int convertResult = MultiByteToWideChar(CP_ACP, 0, str.data(), (int)str.size(), nullptr, 0);
	if (convertResult <= 0) {
		assert(false);
		return {};
	}

	std::wstring r(convertResult + 10, L'\0');
	convertResult = MultiByteToWideChar(CP_ACP, 0, str.data(), (int)str.size(), &r[0], (int)r.size());
	if (convertResult <= 0) {
		assert(false);
		return {};
	}

	return std::wstring(r.begin(), r.begin() + convertResult);
}

BYTE ResolveHex(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	} else {
		return 0;
	}
}


int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cout << "非法参数" << std::endl;
		return 1;
	}

	const char* inFile = argv[1];
	const char* outFile = argv[2];

	std::ifstream ifs(inFile);
	if (!ifs) {
		std::cout << "打开" << inFile << "失败" << std::endl;
		return 1;
	}

	size_t width, height;
	ifs >> width >> height;
	ifs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	std::vector<DirectX::PackedVector::HALF> data(width * height * 4);

	for (size_t i = 0; i < data.size(); ++i) {
		// 解析 32 位浮点数
		union {
			UINT i;
			FLOAT f;
		} binary{};
		
		for (int j = 0; j < 4; ++j) {
			char c1 = ifs.get();
			char c2 = ifs.get();

			if (!ifs) {
				std::cout << "非法的文件格式" << std::endl;
				return 1;
			}

			binary.i |= ((ResolveHex(c1) << 4) + ResolveHex(c2)) << (j * 8);
		}

		// 转换为半精度浮点数
		data[i] = DirectX::PackedVector::XMConvertFloatToHalf(binary.f);
	}

	// 保存 DDS
	DirectX::Image img{};
	img.width = width;
	img.height = height;
	img.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	img.pixels = (uint8_t*)data.data();
	img.rowPitch = width * 8;
	img.slicePitch = img.rowPitch * height;
	
	HRESULT hr = DirectX::SaveToDDSFile(img, DirectX::DDS_FLAGS_NONE, UTF8ToUTF16(outFile).c_str());
	if (FAILED(hr)) {
		std::cout << "保存 DDS 失败";
		return 1;
	}
	
	std::cout << "已生成 " << outFile << std::endl;
	return 0;
}

