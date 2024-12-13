#include "pch.h"
#include "TouchHelper.h"
#include "StrUtils.h"
#include <ImageHlp.h>
#include "Logger.h"
#include "Win32Utils.h"
#include "CommonSharedConstants.h"

using namespace Magpie::Core;

namespace Magpie {

// 证书的 SHA1 哈希值，也是“指纹”
static constexpr std::array<uint8_t, 20> CERT_FINGERPRINT{
	0xad, 0x5a, 0x50, 0x3d, 0xda, 0xec, 0x08, 0x5b, 0xf4, 0x48,
	0xd8, 0x63, 0xcf, 0x90, 0x3a, 0xb4, 0x72, 0x0e, 0x0b, 0x12
};

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

static bool InstallCertificateFromPE(const wchar_t* exePath) noexcept {
	// 打开本地计算机的根证书存储区
	unique_cert_store hRootCertStore(CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL,
		CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_OPEN_EXISTING_FLAG, L"ROOT"));
	if (!hRootCertStore) {
		Logger::Get().Win32Error("CertOpenStore 失败");
		return false;
	}

	// 检查证书是否已安装
	{
		const CRYPT_DATA_BLOB blob{
			.cbData = (DWORD)CERT_FINGERPRINT.size(),
			.pbData = (BYTE*)CERT_FINGERPRINT.data()
		};
		unique_cert_context context(CertFindCertificateInStore(
			hRootCertStore.get(), PKCS_7_ASN_ENCODING, 0, CERT_FIND_SHA1_HASH, &blob, nullptr));
		if (context) {
			return true;
		}
	}

	// 从可执行文件中提取证书
	std::vector<uint8_t> certData = GetCertificateDataFromPE(exePath);
	if (certData.empty()) {
		Logger::Get().Error("GetCertificateDataFromPE 失败");
		return false;
	}

	WIN_CERTIFICATE* winCert = (WIN_CERTIFICATE*)certData.data();

	if (winCert->wCertificateType != WIN_CERT_TYPE_PKCS_SIGNED_DATA) {
		Logger::Get().Error("未知证书");
		return false;
	}

	CRYPT_DATA_BLOB blob{
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
			Logger::Get().Error("证书指纹不匹配");
			return false;
		}
	}

	// 设置友好名称
	{
		wchar_t friendlyName[] = L"Magpie Self-Signed Certificate";
		blob.cbData = sizeof(friendlyName);
		blob.pbData = (BYTE*)friendlyName;
		if (!CertSetCertificateContextProperty(context.get(), CERT_FRIENDLY_NAME_PROP_ID, 0, &blob)) {
			Logger::Get().Error("CertSetCertificateContextProperty 失败");
		}
	}
	
	// 安装证书
	if (!CertAddCertificateContextToStore(hRootCertStore.get(), context.get(), CERT_STORE_ADD_NEWER, NULL)) {
		if (GetLastError() != CRYPT_E_EXISTS) {
			Logger::Get().Win32Error("CertAddCertificateContextToStore 失败");
			return false;
		}
	}

	return true;
}

// 为了获得 UIAccess 权限需满足两个条件：
// 1. 必须签名，且证书必须由本地计算机的受信任的根证书颁发机构验证
// 2. 必须位于只能由管理员写入的本地文件夹
bool TouchHelper::Register() noexcept {
	static constexpr const wchar_t* exePath = CommonSharedConstants::TOUCH_HELPER_EXE_NAME;
	
	if (!Win32Utils::IsProcessElevated()) {
		Logger::Get().Error("没有管理员权限");
		return false;
	}

	if (!Win32Utils::FileExists(exePath)) {
		Logger::Get().Error("找不到可执行文件");
		return false;
	}

	if (!InstallCertificateFromPE(exePath)) {
		Logger::Get().Error("InstallCert 失败");
		return false;
	}

	Logger::Get().Info("安装证书成功");

	// 将可执行文件复制到 System32 文件夹中
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

	std::wstring targetPath = StrUtils::Concat(magpieDir, L"\\", exePath);
	if (!CopyFile(exePath, targetPath.c_str(), FALSE)) {
		Logger::Get().Win32Error("CopyFile 失败");
		return false;
	}

	// 记录版本
	targetPath += L".ver";
	static const uint32_t version = CommonSharedConstants::TOUCH_HELPER_VERSION;
	if (!Win32Utils::WriteFile(targetPath.c_str(), &version, sizeof(version))) {
		Logger::Get().Error("写入资源文件失败");
		return false;
	}

	Logger::Get().Info("复制可执行文件成功");
	return true;
}

static void StopTouchHelper() noexcept {
	// 查找 TouchHelper.exe 的隐藏窗口
	HWND hwndTouchHelper = NULL;
	for (int i = 0; i < 10; ++i) {
		hwndTouchHelper = FindWindow(CommonSharedConstants::TOUCH_HELPER_WINDOW_CLASS_NAME, nullptr);
		if (hwndTouchHelper) {
			break;
		}

		// 等待 TouchHelper.exe 初始化
		Sleep(100);
	}

	if (!hwndTouchHelper) {
		Logger::Get().Info("未找到 TouchHelper 窗口");
		return;
	}

	DWORD processId;
	if (!GetWindowThreadProcessId(hwndTouchHelper, &processId)) {
		Logger::Get().Win32Error("GetWindowThreadProcessId 失败");
		return;
	}

	wil::unique_process_handle hTouchHelperProcess(OpenProcess(SYNCHRONIZE, FALSE, processId));
	if (!hTouchHelperProcess) {
		Logger::Get().Win32Error("OpenProcess 失败");
		return;
	}

	const UINT WM_MAGPIE_TOUCHHELPER =
		RegisterWindowMessage(CommonSharedConstants::WM_MAGPIE_TOUCHHELPER);

	// 通知 TouchHelper 退出
	PostMessage(hwndTouchHelper, WM_MAGPIE_TOUCHHELPER, 0, 0);

	// 等待退出
	if (wil::handle_wait(hTouchHelperProcess.get(), 3000)) {
		Logger::Get().Info("TouchHelper 已退出");
	} else {
		Logger::Get().Error("TouchHelper 未退出");
	}
}

static bool DeleteTouchHelperExe(const wchar_t* exePath) noexcept {
	if (DeleteFile(exePath)) {
		return true;
	}

	if (GetLastError() != ERROR_ACCESS_DENIED) {
		Logger::Get().Win32Error("DeleteFile 失败");
		return false;
	}

	StopTouchHelper();

	if (DeleteFile(exePath)) {
		return true;
	} else {
		Logger::Get().Win32Error("DeleteFile 失败");
		return false;
	}
}

bool TouchHelper::Unregister() noexcept {
	if (!Win32Utils::IsProcessElevated()) {
		Logger::Get().Error("没有管理员权限");
		return false;
	}

	// 删除 system32\Magpie 文件夹

	wil::unique_cotaskmem_string system32Dir;
	HRESULT hr = SHGetKnownFolderPath(
		FOLDERID_System, KF_FLAG_DEFAULT, NULL, system32Dir.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("SHGetKnownFolderPath 失败", hr);
		return false;
	}

	// 如果 TouchHelper 正在运行，则使它退出
	if (DeleteTouchHelperExe(StrUtils::Concat(system32Dir.get(), L"\\Magpie\\",
							 CommonSharedConstants::TOUCH_HELPER_EXE_NAME).c_str())) {
		Logger::Get().Info("已删除 TouchHelper.exe");
	} else {
		Logger::Get().Error("删除 TouchHelper.exe 失败");
		return false;
	}

	hr = wil::RemoveDirectoryRecursiveNoThrow(
		StrUtils::Concat(system32Dir.get(), L"\\Magpie").c_str());
	if (FAILED(hr)) {
		Logger::Get().ComError("RemoveDirectoryRecursiveNoThrow 失败", hr);
		return false;
	}

	// 删除证书

	// 打开本地计算机的根证书存储区
	unique_cert_store hRootCertStore(CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL,
		CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_OPEN_EXISTING_FLAG, L"ROOT"));
	if (!hRootCertStore) {
		Logger::Get().Win32Error("CertOpenStore 失败");
		return false;
	}

	const CRYPT_DATA_BLOB blob{
		.cbData = (DWORD)CERT_FINGERPRINT.size(),
		.pbData = (BYTE*)CERT_FINGERPRINT.data()
	};
	unique_cert_context context(CertFindCertificateInStore(
		hRootCertStore.get(), PKCS_7_ASN_ENCODING, 0, CERT_FIND_SHA1_HASH, &blob, nullptr));
	if (context) {
		if (!CertDeleteCertificateFromStore(context.get())) {
			Logger::Get().Win32Error("CertDeleteCertificateFromStore 失败");
			return false;
		}
	}

	return true;
}

static std::wstring GetTouchHelperPath() noexcept {
	wil::unique_cotaskmem_string system32Dir;
	HRESULT hr = SHGetKnownFolderPath(
		FOLDERID_System, KF_FLAG_DEFAULT, NULL, system32Dir.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("SHGetKnownFolderPath 失败", hr);
		return {};
	}

	return StrUtils::Concat(system32Dir.get(),
		L"\\Magpie\\", CommonSharedConstants::TOUCH_HELPER_EXE_NAME);
}

bool TouchHelper::IsTouchSupportEnabled() noexcept {
	// 不检查版本号
	return Win32Utils::FileExists(GetTouchHelperPath().c_str());
}

void TouchHelper::IsTouchSupportEnabled(bool value) noexcept {
	SHELLEXECUTEINFO execInfo{
		.cbSize = sizeof(execInfo),
		.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS,
		.lpVerb = L"runas",
		.lpFile = Win32Utils::GetExePath().c_str(),
		.lpParameters = value ? L" -r" : L" -ur"
	};

	if (ShellExecuteEx(&execInfo)) {
		wil::unique_process_handle hProcess(execInfo.hProcess);
		if (hProcess) {
			wil::handle_wait(hProcess.get());
		}
	} else if (GetLastError() != ERROR_CANCELLED) {
		Logger::Get().Win32Error("ShellExecuteEx 失败");
	}
}

static bool CheckAndFixTouchHelper(std::wstring& path) noexcept {
	// 检查版本号
	path += L".ver";

	std::vector<uint8_t> versionData;

	const auto checkVersion = [&]() {
		if (!Win32Utils::ReadFile(path.c_str(), versionData)) {
			Logger::Get().Error("读取版本号失败");
		}

		return versionData.size() == 4 &&
			*(uint32_t*)versionData.data() == CommonSharedConstants::TOUCH_HELPER_VERSION;
	};

	if (!checkVersion()) {
		// 版本号不匹配，尝试修复，这会请求管理员权限
		TouchHelper::IsTouchSupportEnabled(true);

		if (!checkVersion()) {
			Logger::Get().Error("修复触控支持失败");
			return false;
		}
	}

	path.erase(path.size() - 4);
	return true;
}

bool TouchHelper::TryLaunchTouchHelper(bool& isTouchSupportEnabled) noexcept {
	std::wstring path = GetTouchHelperPath();
	isTouchSupportEnabled = Win32Utils::FileExists(path.c_str());
	if (!isTouchSupportEnabled) {
		// 未启用触控支持
		return true;
	}

	wil::unique_mutex_nothrow hSingleInstanceMutex;

	bool alreadyExists = false;
	if (!hSingleInstanceMutex.try_create(
		CommonSharedConstants::TOUCH_HELPER_SINGLE_INSTANCE_MUTEX_NAME,
		CREATE_MUTEX_INITIAL_OWNER,
		MUTEX_ALL_ACCESS,
		nullptr,
		&alreadyExists
		) || alreadyExists) {
		Logger::Get().Info("TouchHelper.exe 正在运行");
		return true;
	}

	hSingleInstanceMutex.ReleaseMutex();

	// TouchHelper.exe 未在运行则启动它

	// 检查版本是否匹配并尝试修复
	if (!CheckAndFixTouchHelper(path)) {
		// 修复失败
		Logger::Get().Error("CheckAndFixTouchHelper 失败");
		return false;
	}

	// GH#992: 必须委托 explorer 启动 ToucherHelper，否则如果启用了“以管理者身份运行该程序”
	// 兼容性选项，ToucherHelper 将无法获得 UIAccess 权限
	if (!Win32Utils::ShellOpen(path.c_str(), nullptr, true)) {
		Logger::Get().Error("启动 TouchHelper.exe 失败");
		return false;
	}

	return true;
}

}
