// MPVHookTextureParser.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <fstream>
#include <vector>
#include <DirectXTex.h>
#include <DirectXPackedVector.h>
#include <string_view>


std::wstring UTF8ToUTF16(std::string_view str) {
	int convertResult = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
	if (convertResult <= 0) {
		assert(false);
		return {};
	}

	std::wstring r(convertResult + 10, L'\0');
	convertResult = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &r[0], (int)r.size());
	if (convertResult <= 0) {
		assert(false);
		return {};
	}

	return std::wstring(r.begin(), r.begin() + convertResult);
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

	std::vector<DirectX::PackedVector::HALF> data(width * height * 4);
	for (size_t i = 0; i < data.size(); ++i) {
		float f;
		ifs >> f;
		data[i] = DirectX::PackedVector::XMConvertFloatToHalf(f);
	}

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

