#pragma once

namespace AutostartManager
{
	constexpr wchar_t TASK_NAME[] = L"AudioPlaybackConnector_Autostart";
	constexpr wchar_t TASK_AUTHOR[] = L"AudioPlaybackConnector";
	constexpr wchar_t TASK_DESCRIPTION[] = L"Start AudioPlaybackConnector after user sign-in.";
	constexpr wchar_t TASK_TRIGGER_ID[] = L"AudioPlaybackConnectorLogon";
	constexpr wchar_t TASK_DELAY[] = L"PT15S";

	struct BStr
	{
		BSTR value = nullptr;

		BStr() = default;

		explicit BStr(std::wstring_view text)
		{
			value = SysAllocStringLen(text.data(), static_cast<UINT>(text.size()));
			THROW_IF_NULL_ALLOC(value);
		}

		~BStr()
		{
			SysFreeString(value);
		}

		BStr(const BStr&) = delete;
		BStr& operator=(const BStr&) = delete;

		BStr(BStr&& other) noexcept : value(other.value)
		{
			other.value = nullptr;
		}

		BStr& operator=(BStr&& other) noexcept
		{
			if (this != &other)
			{
				SysFreeString(value);
				value = other.value;
				other.value = nullptr;
			}
			return *this;
		}

		operator BSTR() const
		{
			return value;
		}

		BSTR* put()
		{
			SysFreeString(value);
			value = nullptr;
			return &value;
		}

		std::wstring to_wstring() const
		{
			return value ? std::wstring(value, SysStringLen(value)) : std::wstring();
		}
	};

	struct Variant
	{
		VARIANT value;

		Variant()
		{
			VariantInit(&value);
		}

		~Variant()
		{
			VariantClear(&value);
		}

		Variant(const Variant&) = delete;
		Variant& operator=(const Variant&) = delete;
	};

	struct Status
	{
		bool exists = false;
		bool enabled = false;
		bool targetExists = false;
		bool targetMatchesCurrentExecutable = false;
		std::wstring targetPath;
	};

	inline winrt::com_ptr<ITaskService> ConnectService()
	{
		winrt::com_ptr<ITaskService> service;
		winrt::check_hresult(CoCreateInstance(CLSID_TaskScheduler, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(service.put())));

		Variant empty;
		winrt::check_hresult(service->Connect(empty.value, empty.value, empty.value, empty.value));
		return service;
	}

	inline winrt::com_ptr<ITaskFolder> GetRootFolder(const winrt::com_ptr<ITaskService>& service)
	{
		winrt::com_ptr<ITaskFolder> folder;
		BStr root(L"\\");
		winrt::check_hresult(service->GetFolder(root, folder.put()));
		return folder;
	}

	inline std::wstring GetCurrentUser(const winrt::com_ptr<ITaskService>& service)
	{
		BStr connectedUser;
		if (SUCCEEDED(service->get_ConnectedUser(connectedUser.put())))
		{
			auto currentUser = connectedUser.to_wstring();
			if (!currentUser.empty())
			{
				return currentUser;
			}
		}

		std::wstring username(256, L'\0');
		DWORD size = static_cast<DWORD>(username.size());
		while (!GetUserNameW(username.data(), &size))
		{
			auto error = GetLastError();
			THROW_LAST_ERROR_IF(error != ERROR_INSUFFICIENT_BUFFER);
			username.resize(size, L'\0');
		}

		if (size > 0)
		{
			username.resize(size - 1);
		}
		return username;
	}

	inline std::wstring GetCurrentExecutablePath()
	{
		return GetModuleFsPath(g_hInst).lexically_normal().native();
	}

	inline bool ArePathsEqual(std::wstring_view left, std::wstring_view right)
	{
		return CompareStringOrdinal(left.data(), static_cast<int>(left.size()), right.data(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
	}

	inline Status GetStatus()
	{
		Status status;

		auto service = ConnectService();
		auto folder = GetRootFolder(service);

		winrt::com_ptr<IRegisteredTask> task;
		BStr taskName(TASK_NAME);
		auto hr = folder->GetTask(taskName, task.put());
		if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND)
		{
			return status;
		}
		winrt::check_hresult(hr);

		status.exists = true;

		VARIANT_BOOL enabled = VARIANT_FALSE;
		winrt::check_hresult(task->get_Enabled(&enabled));
		status.enabled = enabled == VARIANT_TRUE;

		winrt::com_ptr<ITaskDefinition> taskDefinition;
		winrt::check_hresult(task->get_Definition(taskDefinition.put()));

		winrt::com_ptr<IActionCollection> actionCollection;
		winrt::check_hresult(taskDefinition->get_Actions(actionCollection.put()));

		LONG actionCount = 0;
		winrt::check_hresult(actionCollection->get_Count(&actionCount));
		if (actionCount <= 0)
		{
			return status;
		}

		winrt::com_ptr<IAction> action;
		winrt::check_hresult(actionCollection->get_Item(1, action.put()));

		TASK_ACTION_TYPE actionType = TASK_ACTION_EXEC;
		winrt::check_hresult(action->get_Type(&actionType));
		if (actionType != TASK_ACTION_EXEC)
		{
			return status;
		}

		auto execAction = action.as<IExecAction>();

		BStr taskPath;
		winrt::check_hresult(execAction->get_Path(taskPath.put()));
		status.targetPath = taskPath.to_wstring();
		if (status.targetPath.empty())
		{
			return status;
		}

		std::error_code errorCode;
		status.targetExists = fs::exists(fs::path(status.targetPath), errorCode);
		status.targetMatchesCurrentExecutable = ArePathsEqual(status.targetPath, GetCurrentExecutablePath());
		return status;
	}

	inline bool IsEnabled()
	{
		return GetStatus().enabled;
	}

	inline void Enable()
	{
		auto service = ConnectService();
		auto folder = GetRootFolder(service);
		auto currentUser = GetCurrentUser(service);

		winrt::com_ptr<ITaskDefinition> taskDefinition;
		winrt::check_hresult(service->NewTask(0, taskDefinition.put()));

		winrt::com_ptr<IRegistrationInfo> registrationInfo;
		winrt::check_hresult(taskDefinition->get_RegistrationInfo(registrationInfo.put()));
		winrt::check_hresult(registrationInfo->put_Author(BStr(TASK_AUTHOR)));
		winrt::check_hresult(registrationInfo->put_Description(BStr(TASK_DESCRIPTION)));

		winrt::com_ptr<IPrincipal> principal;
		winrt::check_hresult(taskDefinition->get_Principal(principal.put()));
		winrt::check_hresult(principal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN));
		winrt::check_hresult(principal->put_RunLevel(TASK_RUNLEVEL_LUA));
		winrt::check_hresult(principal->put_UserId(BStr(currentUser)));

		winrt::com_ptr<ITaskSettings> taskSettings;
		winrt::check_hresult(taskDefinition->get_Settings(taskSettings.put()));
		winrt::check_hresult(taskSettings->put_StartWhenAvailable(VARIANT_TRUE));
		winrt::check_hresult(taskSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE));
		winrt::check_hresult(taskSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE));
		winrt::check_hresult(taskSettings->put_MultipleInstances(TASK_INSTANCES_IGNORE_NEW));
		winrt::check_hresult(taskSettings->put_ExecutionTimeLimit(BStr(L"PT0S")));

		winrt::com_ptr<ITriggerCollection> triggerCollection;
		winrt::check_hresult(taskDefinition->get_Triggers(triggerCollection.put()));

		winrt::com_ptr<ITrigger> trigger;
		winrt::check_hresult(triggerCollection->Create(TASK_TRIGGER_LOGON, trigger.put()));
		auto logonTrigger = trigger.as<ILogonTrigger>();
		winrt::check_hresult(logonTrigger->put_Id(BStr(TASK_TRIGGER_ID)));
		winrt::check_hresult(logonTrigger->put_Delay(BStr(TASK_DELAY)));
		winrt::check_hresult(logonTrigger->put_UserId(BStr(currentUser)));

		winrt::com_ptr<IActionCollection> actionCollection;
		winrt::check_hresult(taskDefinition->get_Actions(actionCollection.put()));

		winrt::com_ptr<IAction> action;
		winrt::check_hresult(actionCollection->Create(TASK_ACTION_EXEC, action.put()));
		auto execAction = action.as<IExecAction>();

		auto executablePath = GetModuleFsPath(g_hInst);
		const auto executableDirectory = executablePath.parent_path();
		winrt::check_hresult(execAction->put_Path(BStr(executablePath.native())));
		winrt::check_hresult(execAction->put_WorkingDirectory(BStr(executableDirectory.native())));

		Variant empty;
		winrt::com_ptr<IRegisteredTask> registeredTask;
		winrt::check_hresult(folder->RegisterTaskDefinition(
			BStr(TASK_NAME),
			taskDefinition.get(),
			TASK_CREATE_OR_UPDATE,
			empty.value,
			empty.value,
			TASK_LOGON_INTERACTIVE_TOKEN,
			empty.value,
			registeredTask.put()));
	}

	inline void Disable()
	{
		auto service = ConnectService();
		auto folder = GetRootFolder(service);

		BStr taskName(TASK_NAME);
		auto hr = folder->DeleteTask(taskName, 0);
		if (HRESULT_CODE(hr) == ERROR_FILE_NOT_FOUND)
		{
			return;
		}
		winrt::check_hresult(hr);
	}

	inline Status RepairIfNeeded()
	{
		auto status = GetStatus();
		if (!status.enabled)
		{
			return status;
		}

		if (status.targetExists && status.targetMatchesCurrentExecutable)
		{
			return status;
		}

		Enable();
		return GetStatus();
	}
}
