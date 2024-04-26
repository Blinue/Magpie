// Copyright (c) 2021 - present, Liu Xu
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "pch.h"
#include "XamlApp.h"
#include "Win32Utils.h"
#include "StrUtils.h"
#include <ImageHlp.h>

// 将当前目录设为程序所在目录
static void SetWorkingDir() noexcept {
	std::wstring path = Win32Utils::GetExePath();

	FAIL_FAST_IF_FAILED(PathCchRemoveFileSpec(
		path.data(),
		path.size() + 1
	));

	FAIL_FAST_IF_WIN32_BOOL_FALSE(SetCurrentDirectory(path.c_str()));
}

static void InitializeLogger() noexcept {
	Logger& logger = Logger::Get();
	logger.Initialize(
		spdlog::level::info,
		CommonSharedConstants::LOG_PATH,
		100000,
		2
	);
}

static std::vector<uint8_t> GetCertificateDataFromPE(const wchar_t* fileName) noexcept {
	wil::unique_hfile hFile(CreateFile(
		fileName, FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
	if (!hFile) {
		Logger::Get().Win32Error("CreateFile 失败");
		return {};
	}
	
	DWORD len = 0;
	WIN_CERTIFICATE cert;
	if (!ImageGetCertificateData(hFile.get(), 0, &cert, &len) && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
		Logger::Get().Win32Error("ImageGetCertificateData 失败");
		return {};
	}

	std::vector<uint8_t> data(len);
	if (!ImageGetCertificateData(hFile.get(), 0, (WIN_CERTIFICATE*)data.data(), &len)) {
		Logger::Get().Win32Error("ImageGetCertificateData 失败");
		return {};
	}

	return data;
}

static void CloseCertStore(HCERTSTORE hCertStore) noexcept {
	CertCloseStore(hCertStore, 0);
}

using unique_cert_store = wil::unique_any<HCERTSTORE, decltype(&CloseCertStore), CloseCertStore>;
using unique_cert_context =
	wil::unique_any<PCCERT_CONTEXT, decltype(&CertFreeCertificateContext), CertFreeCertificateContext>;

static bool InstallCertificate(const wchar_t* fileName) noexcept {
	static constexpr std::array<uint8_t, 20> CERT_FINGERPRINT{
		0xad, 0x5a, 0x50, 0x3d, 0xda, 0xec, 0x08, 0x5b, 0xf4, 0x48,
		0xd8, 0x63, 0xcf, 0x90, 0x3a, 0xb4, 0x72, 0x0e, 0x0b, 0x12
	};

	std::vector<uint8_t> certData = GetCertificateDataFromPE(fileName);
	if (certData.empty()) {
		Logger::Get().Error("GetCertificateDataFromPE 失败");
		return false;
	}

	WIN_CERTIFICATE* winCert = (WIN_CERTIFICATE*)certData.data();

	if (winCert->wCertificateType != WIN_CERT_TYPE_PKCS_SIGNED_DATA) {
		return false;
	}

	const CRYPT_DATA_BLOB blob{
		.cbData = DWORD(certData.size() - offsetof(WIN_CERTIFICATE, bCertificate)),
		.pbData = winCert->bCertificate
	};
	unique_cert_store hMemStore(CertOpenStore(
		CERT_STORE_PROV_PKCS7, 0, NULL, CERT_STORE_READONLY_FLAG, &blob));
	if (!hMemStore) {
		Logger::Get().Win32Error("CertOpenStore 失败");
		return false;
	}

	unique_cert_context context(CertFindCertificateInStore(
		hMemStore.get(), PKCS_7_ASN_ENCODING, 0, CERT_FIND_ANY, 0, NULL));
	if (!context) {
		Logger::Get().Win32Error("CertFindCertificateInStore 失败");
		return false;
	}

	// 验证指纹
	{
		std::array<uint8_t, 20> fingerprint{};
		DWORD fingerprintSize = (DWORD)fingerprint.size();
		if (!CertGetCertificateContextProperty(context.get(),
			CERT_HASH_PROP_ID, fingerprint.data(), &fingerprintSize)) {
			Logger::Get().Win32Error("CertGetCertificateContextProperty 失败");
			return false;
		}

		if (fingerprint != CERT_FINGERPRINT) {
			Logger::Get().Error("证书指纹不匹配！");
			return false;
		}
	}

	// 打开当前用户的根证书存储区
	unique_cert_store hCertStore(CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL,
		CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_OPEN_EXISTING_FLAG, L"ROOT"));
	if (hCertStore) {
		if (!CertAddCertificateContextToStore(hCertStore.get(), context.get(), CERT_STORE_ADD_NEWER, NULL)) {
			if (GetLastError() != CRYPT_E_EXISTS) {
				Logger::Get().Win32Error("CertAddCertificateContextToStore 失败");
				return false;
			}
		}
	}

	return true;
}

static bool RegisterTouchHelper() noexcept {
	if (!Win32Utils::IsProcessElevated()) {
		Logger::Get().Error("没有管理员权限");
		return false;
	}

	constexpr const wchar_t* touchHelperExe = L"TouchHelper.exe";

	if (!Win32Utils::FileExists(touchHelperExe)) {
		Logger::Get().Error("找不到 TouchHelper.exe");
		return false;
	}

	if (!InstallCertificate(touchHelperExe)) {
		Logger::Get().Error("InstallCert 失败");
		return false;
	}

	Logger::Get().Info("安装证书成功");

	// 将 TouchHelper 复制到 System32 文件夹中
	// 1. 不能选择 Program Files，某些环境下该文件夹中的程序无法获得 UIAccess 权限
	// 2. System32 比 Windows 更好，因为前者是“安全位置”，见 https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-10/security/threat-protection/security-policy-settings/user-account-control-only-elevate-uiaccess-applications-that-are-installed-in-secure-locations
	wil::unique_cotaskmem_string system32Dir;
	HRESULT hr = SHGetKnownFolderPath(
		FOLDERID_System, KF_FLAG_DEFAULT, NULL, system32Dir.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("SHGetKnownFolderPath 失败", hr);
		return false;
	}

	std::wstring magpieDir = StrUtils::Concat(system32Dir.get(), L"\\Magpie");
	hr = wil::CreateDirectoryDeepNoThrow(magpieDir.c_str());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateDirectoryDeepNoThrow 失败", hr);
		return false;
	}

	std::wstring targetPath = StrUtils::Concat(magpieDir, L"\\", touchHelperExe);
	if (!CopyFile(touchHelperExe, targetPath.c_str(), FALSE)) {
		Logger::Get().Win32Error("CopyFile 失败");
		return false;
	}

	Logger::Get().Info("复制 TouchHelper.exe 成功");
	return true;
}

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ wchar_t* lpCmdLine,
	_In_ int /*nCmdShow*/
) {
#ifdef _DEBUG
	SetThreadDescription(GetCurrentThread(), L"Magpie 主线程");
#endif

	// 堆损坏时终止进程
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, nullptr, 0);

	SetWorkingDir();

	InitializeLogger();

	Logger::Get().Info(fmt::format("程序启动\n\t版本: {}\n\t管理员: {}",
#ifdef MAGPIE_VERSION_TAG
		STRING(MAGPIE_VERSION_TAG)
#else
		"dev"
#endif
		, Win32Utils::IsProcessElevated() ? "是" : "否"));

	if (lpCmdLine == L"-r"sv) {
		return RegisterTouchHelper() ? 0 : 1;
	}

	auto& app = Magpie::XamlApp::Get();
	if (!app.Initialize(hInstance, lpCmdLine)) {
		return 0;
	}

	return app.Run();
}
