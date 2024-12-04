#include "pch.h"
#include "AutoStartHelper.h"
#include <taskschd.h>
#include <Lmcons.h>
#include "Logger.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include <propkey.h>

#pragma comment(lib, "Taskschd.lib")


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// 实现开机启动
//
// 首先尝试使用任务计划程序，此方案的优点是以管理员身份启动时不会显示 UAC。
// 如果创建任务失败，则回落到在当前用户的启动文件夹中创建快捷方式。
//
// 任务计划程序的使用参考自
// https://github.com/microsoft/PowerToys/blob/3d54cb838504c12f59516afaf1a00fde2dd5d01b/src/runner/auto_start_helper.cpp
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


namespace winrt::Magpie {

static constexpr DWORD USERNAME_DOMAIN_LEN = DNLEN + UNLEN + 2; // Domain Name + '\' + User Name + '\0'
static constexpr DWORD USERNAME_LEN = UNLEN + 1; // User Name + '\0'


static std::wstring GetTaskName(std::wstring_view userName) noexcept {
	return StrUtils::Concat(L"Autorun for ", userName);
}

static com_ptr<ITaskService> CreateTaskService() noexcept {
	com_ptr<ITaskService> taskService = try_create_instance<ITaskService>(CLSID_TaskScheduler);
	if (!taskService) {
		Logger::Get().Error("创建 TaskService 失败");
		return nullptr;
	}

	HRESULT hr = taskService->Connect(Win32Utils::Variant(), Win32Utils::Variant(),
		Win32Utils::Variant(), Win32Utils::Variant());
	if (FAILED(hr)) {
		Logger::Get().ComError("ITaskService::Connect 失败", hr);
		return nullptr;
	}

	return taskService;
}

static bool CreateAutoStartTask(bool runElevated, const wchar_t* arguments) noexcept {
	WCHAR usernameDomain[USERNAME_DOMAIN_LEN];
	WCHAR username[USERNAME_LEN];

	// 检索用户域和用户名
	if (!GetEnvironmentVariable(L"USERNAME", username, USERNAME_LEN)) {
		Logger::Get().Win32Error("获取用户名失败");
		return false;
	}
	if (!GetEnvironmentVariable(L"USERDOMAIN", usernameDomain, USERNAME_DOMAIN_LEN)) {
		Logger::Get().Win32Error("获取用户域失败");
		return false;
	}

	wcscat_s(usernameDomain, L"\\");
	wcscat_s(usernameDomain, username);

	com_ptr<ITaskService> taskService = CreateTaskService();
	if (!taskService) {
		return false;
	}

	// 获取/创建 Magpie 文件夹
	com_ptr<ITaskFolder> taskFolder;
	HRESULT hr = taskService->GetFolder(wil::make_bstr_nothrow(L"\\Magpie").get(), taskFolder.put());
	if (FAILED(hr)) {
		com_ptr<ITaskFolder> rootFolder = NULL;
		hr = taskService->GetFolder(wil::make_bstr_nothrow(L"\\").get(), rootFolder.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("获取根目录失败", hr);
			return false;
		}

		hr = rootFolder->CreateFolder(wil::make_bstr_nothrow(L"\\Magpie").get(), Win32Utils::Variant(L""), taskFolder.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 Magpie 任务文件夹失败", hr);
			return false;
		}
	}

	// Create the task builder object to create the task.
	com_ptr<ITaskDefinition> task;
	hr = taskService->NewTask(0, task.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("创建 ITaskDefinition 失败", hr);
		return false;
	}

	// ------------------------------------------------------
	// Get the registration info for setting the identification.
	com_ptr<IRegistrationInfo> regInfo;
	hr = task->get_RegistrationInfo(regInfo.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("获取 IRegistrationInfo 失败", hr);
		return false;
	}

	hr = regInfo->put_Author(wil::make_bstr_nothrow(usernameDomain).get());
	if (FAILED(hr)) {
		Logger::Get().ComError("IRegistrationInfo::put_Author 失败", hr);
		return false;
	}

	 // ------------------------------------------------------
	// Create the settings for the task
	com_ptr<ITaskSettings> taskSettings;
	hr = task->get_Settings(taskSettings.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("获取 ITaskSettings 失败", hr);
		return false;
	}

	hr = taskSettings->put_StartWhenAvailable(VARIANT_FALSE);
	if (FAILED(hr)) {
		Logger::Get().ComError("ITaskSettings::put_StartWhenAvailable 失败", hr);
		return false;
	}
	hr = taskSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
	if (FAILED(hr)) {
		Logger::Get().ComError("ITaskSettings::put_StopIfGoingOnBatteries 失败", hr);
		return false;
	}
	hr = taskSettings->put_ExecutionTimeLimit(wil::make_bstr_nothrow(L"PT0S").get()); //Unlimited
	if (FAILED(hr)) {
		Logger::Get().ComError("ITaskSettings::put_ExecutionTimeLimit 失败", hr);
		return false;
	}
	hr = taskSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
	if (FAILED(hr)) {
		Logger::Get().ComError("ITaskSettings::put_DisallowStartIfOnBatteries 失败", hr);
		return false;
	}

	// ------------------------------------------------------
	// Get the trigger collection to insert the logon trigger.
	com_ptr<ITriggerCollection> triggerCollection;
	hr = task->get_Triggers(triggerCollection.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("获取 ITriggerCollection 失败", hr);
		return false;
	}

	// Add the logon trigger to the task.
	{
		com_ptr<ITrigger> trigger;
		hr = triggerCollection->Create(TASK_TRIGGER_LOGON, trigger.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 ITrigger 失败", hr);
			return false;
		}

		com_ptr<ILogonTrigger> logonTrigger = trigger.try_as<ILogonTrigger>();
		if (!logonTrigger) {
			Logger::Get().Error("获取 ILogonTrigger 失败");
			return false;
		}

		logonTrigger->put_Id(wil::make_bstr_nothrow(L"Trigger1").get());

		// Timing issues may make explorer not be started when the task runs.
		// Add a little delay to mitigate this.
		logonTrigger->put_Delay(wil::make_bstr_nothrow(L"PT03S").get());

		// Define the user. The task will execute when the user logs on.
		// The specified user must be a user on this computer.
		hr = logonTrigger->put_UserId(wil::make_bstr_nothrow(usernameDomain).get());
		if (FAILED(hr)) {
			Logger::Get().ComError("ILogonTrigger::put_UserId 失败", hr);
			return false;
		}
	}

	// ------------------------------------------------------
	// Add an Action to the task. This task will execute the path passed to this custom action.
	{
		com_ptr<IActionCollection> actionCollection;

		// Get the task action collection pointer.
		hr = task->get_Actions(actionCollection.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("获取 IActionCollection 失败", hr);
			return false;
		}

		// Create the action, specifying that it is an executable action.
		com_ptr<IAction> action;
		hr = actionCollection->Create(TASK_ACTION_EXEC, action.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 IAction 失败", hr);
			return false;
		}

		// QI for the executable task pointer.
		com_ptr<IExecAction> execAction = action.try_as<IExecAction>();
		if (!execAction) {
			Logger::Get().Error("获取 IExecAction 失败");
			return false;
		}

		// Set the path of the executable to Magpie (passed as CustomActionData).
		const std::wstring& exePath = Win32Utils::GetExePath();
		hr = execAction->put_Path(wil::make_bstr_nothrow(exePath.c_str()).get());
		if (FAILED(hr)) {
			Logger::Get().ComError("设置可执行文件路径失败", hr);
			return false;
		}

		if (arguments) {
			execAction->put_Arguments(wil::make_bstr_nothrow(arguments).get());
		}
	}

	// ------------------------------------------------------
	// Create the principal for the task
	{
		com_ptr<IPrincipal> principal;
		hr = task->get_Principal(principal.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("获取 IPrincipal 失败", hr);
			return false;
		}

		// Set up principal information:
		principal->put_Id(wil::make_bstr_nothrow(L"Principal1").get());
		principal->put_UserId(wil::make_bstr_nothrow(usernameDomain).get());
		principal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);

		if (runElevated) {
			hr = principal->put_RunLevel(_TASK_RUNLEVEL::TASK_RUNLEVEL_HIGHEST);
		} else {
			hr = principal->put_RunLevel(_TASK_RUNLEVEL::TASK_RUNLEVEL_LUA);
		}

		if (FAILED(hr)) {
			Logger::Get().ComError("IPrincipal::put_RunLevel 失败", hr);
			return false;
		}
	}

	// ------------------------------------------------------
	//  Save the task in the Magpie folder.
	{
		com_ptr<IRegisteredTask> registeredTask;
		static constexpr const wchar_t* SDDL_FULL_ACCESS_FOR_EVERYONE = L"D:(A;;FA;;;WD)";

		// 如果用户是 Administrator 账户，但 Magpie 不是以提升权限运行的，此调用会因权限问题失败
		std::wstring taskName = GetTaskName(username);
		hr = taskFolder->RegisterTaskDefinition(
			wil::make_bstr_nothrow(taskName.c_str()).get(),
			task.get(),
			TASK_CREATE_OR_UPDATE,
			Win32Utils::Variant(usernameDomain),
			Win32Utils::Variant(),
			TASK_LOGON_INTERACTIVE_TOKEN,
			Win32Utils::Variant(SDDL_FULL_ACCESS_FOR_EVERYONE),
			registeredTask.put()
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("注册任务失败", hr);
			return false;
		}

		registeredTask->put_Enabled(VARIANT_TRUE);
	}

	return true;
}

static bool DeleteAutoStartTask() noexcept {
	WCHAR username[USERNAME_LEN];
	if (!GetEnvironmentVariable(L"USERNAME", username, USERNAME_LEN)) {
		Logger::Get().Win32Error("获取用户名失败");
		return false;
	}

	com_ptr<ITaskService> taskService = CreateTaskService();
	if (!taskService) {
		return false;
	}

	com_ptr<ITaskFolder> taskFolder;
	HRESULT hr = taskService->GetFolder(wil::make_bstr_nothrow(L"\\Magpie").get(), taskFolder.put());
	if (FAILED(hr)) {
		return true;
	}

	wil::unique_bstr taskName = wil::make_bstr_nothrow(GetTaskName(username).c_str());

	{
		com_ptr<IRegisteredTask> existingRegisteredTask;
		hr = taskFolder->GetTask(taskName.get(), existingRegisteredTask.put());
		if (FAILED(hr)) {
			// 不存在任务
			return true;
		}
	}

	hr = taskFolder->DeleteTask(taskName.get(), 0);
	if (FAILED(hr)) {
		Logger::Get().ComError("删除任务失败", hr);
		return false;
	}

	return true;
}

static bool IsAutoStartTaskActive(std::wstring& arguements) noexcept {
	WCHAR username[USERNAME_LEN];
	if (!GetEnvironmentVariable(L"USERNAME", username, USERNAME_LEN)) {
		Logger::Get().Win32Error("获取用户名失败");
		return false;
	}

	com_ptr<ITaskService> taskService = CreateTaskService();
	if (!taskService) {
		return false;
	}

	com_ptr<ITaskFolder> taskFolder;
	HRESULT hr = taskService->GetFolder(wil::make_bstr_nothrow(L"\\Magpie").get(), taskFolder.put());
	if (FAILED(hr)) {
		return false;
	}

	com_ptr<IRegisteredTask> existingRegisteredTask;
	hr = taskFolder->GetTask(wil::make_bstr_nothrow(GetTaskName(username).c_str()).get(), existingRegisteredTask.put());
	if (FAILED(hr)) {
		return false;
	}

	VARIANT_BOOL isEnabled;
	hr = existingRegisteredTask->get_Enabled(&isEnabled);
	if (FAILED(hr)) {
		Logger::Get().ComError("IRegisteredTask::get_Enabled 失败", hr);
		return false;
	}

	com_ptr<ITaskDefinition> task;
	hr = existingRegisteredTask->get_Definition(task.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("获取 ITaskDefinition 失败", hr);
		return false;
	}

	com_ptr<IActionCollection> actionCollection;
	task->get_Actions(actionCollection.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("获取 IActionCollection 失败", hr);
		return false;
	}

	com_ptr<IAction> action;
	// 索引从 1 开始
	hr = actionCollection->get_Item(1, action.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("获取 IAction 失败", hr);
		return false;
	}

	com_ptr<IExecAction> execAction = action.try_as<IExecAction>();
	wil::unique_bstr args;
	hr = execAction->get_Arguments(args.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("获取参数失败", hr);
		return false;
	}
	arguements = args ? args.get() : L"";

	return isEnabled == VARIANT_TRUE;
}

static std::wstring GetShortcutPath() noexcept {
	// 获取用户的启动文件夹路径
	wil::unique_cotaskmem_string startupDir;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_Startup, 0, NULL, startupDir.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("获取启动文件夹失败", hr);
		return {};
	}

	return StrUtils::Concat(startupDir.get(), L"\\Magpie.lnk");
}

static bool CreateAutoStartShortcut(const wchar_t* arguments) noexcept {
	com_ptr<IShellLink> shellLink = try_create_instance<IShellLink>(CLSID_ShellLink);
	if (!shellLink) {
		Logger::Get().Error("创建 ShellLink 失败");
		return false;
	}

	shellLink->SetPath(Win32Utils::GetExePath().c_str());

	if (arguments) {
		shellLink->SetArguments(arguments);
	}

	com_ptr<IPersistFile> persistFile = shellLink.try_as<IPersistFile>();
	if (!persistFile) {
		Logger::Get().Error("获取 IPersistFile 失败");
		return false;
	}

	HRESULT hr = persistFile->Save(GetShortcutPath().c_str(), TRUE);
	if (FAILED(hr)) {
		Logger::Get().ComError("保存快捷方式失败", hr);
		return false;
	}

	return true;
}

static bool DeleteAutoStartShortcut() noexcept {
	std::wstring shortcutPath = GetShortcutPath();
	if (shortcutPath.empty()) {
		return false;
	}

	if (!DeleteFile(shortcutPath.c_str()) && GetLastError() != ERROR_FILE_NOT_FOUND) {
		Logger::Get().Win32Error("删除快捷方式失败");
		return false;
	}

	return true;
}

static bool IsAutoStartShortcutExist(std::wstring& arguments) noexcept {
	std::wstring shortcutPath = GetShortcutPath();
	if (shortcutPath.empty()) {
		return false;
	}

	if (!Win32Utils::FileExists(shortcutPath.c_str())) {
		return false;
	}

	com_ptr<IShellLink> shellLink = try_create_instance<IShellLink>(CLSID_ShellLink);
	if (!shellLink) {
		Logger::Get().Error("创建 ShellLink 失败");
		return false;
	}

	com_ptr<IPersistFile> persistFile = shellLink.try_as<IPersistFile>();
	if (!persistFile) {
		Logger::Get().Error("获取 IPersistFile 失败");
		return false;
	}

	HRESULT hr = persistFile->Load(shortcutPath.c_str(), STGM_READ);
	if (FAILED(hr)) {
		Logger::Get().ComError("读取快捷方式失败", hr);
		return false;
	}

	hr = shellLink->Resolve(NULL, SLR_NO_UI);
	if (FAILED(hr)) {
		Logger::Get().ComError("解析快捷方式失败", hr);
		return false;
	}

	// https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ishelllinka-getarguments
	// 推荐从 IPropertyStore 检索参数
	com_ptr<IPropertyStore> propertyStore = shellLink.as<IPropertyStore>();
	if (!propertyStore) {
		Logger::Get().Error("获取 IPropertyStore 失败");
		return false;
	}

	wil::unique_prop_variant prop;
	hr = propertyStore->GetValue(PKEY_Link_Arguments, &prop);
	if (FAILED(hr)) {
		Logger::Get().ComError("检索 Arguments 参数失败", hr);
		return false;
	}

	if (prop.vt == VT_LPWSTR) {
		arguments = prop.pwszVal;
	} else if (prop.vt == VT_BSTR) {
		arguments = prop.bstrVal;
	}

	return true;
}

bool AutoStartHelper::EnableAutoStart(bool runElevated, const wchar_t* arguments) noexcept {
	if (CreateAutoStartTask(runElevated, arguments)) {
		DeleteAutoStartShortcut();
		return true;
	}

	return CreateAutoStartShortcut(arguments);
}

bool AutoStartHelper::DisableAutoStart() noexcept {
	// 避免或运算符的短路，确保两者都被删除
	bool result1 = DeleteAutoStartTask();
	bool result2 = DeleteAutoStartShortcut();
	return result1 || result2;
}

bool AutoStartHelper::IsAutoStartEnabled(std::wstring& arguments) noexcept {
	return IsAutoStartTaskActive(arguments) || IsAutoStartShortcutExist(arguments);
}

}
