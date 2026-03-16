#pragma once

namespace SettingsStore
{
	constexpr auto BUFFER_SIZE = 4096;

	inline fs::path GetConfigPath()
	{
		return GetModuleFsPath(g_hInst).remove_filename() / CONFIG_NAME;
	}

	inline int64_t CurrentTimestamp()
	{
		return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

	inline SavedDevice MakeSavedDevice(std::wstring id, std::wstring displayName, int64_t lastConnectedTimestamp = 0)
	{
		if (displayName.empty())
		{
			displayName = id;
		}

		return SavedDevice{ std::move(id), std::move(displayName), lastConnectedTimestamp };
	}

	inline void DefaultSettings()
	{
		g_settings = {};
	}

	inline bool TryReadBoolean(const JsonObject& jsonObject, const wchar_t* key, bool& value)
	{
		if (!jsonObject.HasKey(key))
		{
			return false;
		}

		auto jsonValue = jsonObject.GetNamedValue(key);
		if (jsonValue.ValueType() != JsonValueType::Boolean)
		{
			return false;
		}

		value = jsonValue.GetBoolean();
		return true;
	}

	inline int64_t TryReadTimestamp(const JsonObject& jsonObject, const wchar_t* key, int64_t fallbackValue = 0)
	{
		if (!jsonObject.HasKey(key))
		{
			return fallbackValue;
		}

		auto jsonValue = jsonObject.GetNamedValue(key);
		if (jsonValue.ValueType() != JsonValueType::Number)
		{
			return fallbackValue;
		}

		return static_cast<int64_t>(jsonValue.GetNumber());
	}

	inline void NormalizePreferredDevices()
	{
		std::unordered_set<std::wstring> seen;
		std::vector<SavedDevice> normalized;
		normalized.reserve(g_settings.preferredDevices.size());

		for (auto device : g_settings.preferredDevices)
		{
			if (device.id.empty())
			{
				continue;
			}

			if (!seen.emplace(device.id).second)
			{
				continue;
			}

			if (device.displayName.empty())
			{
				device.displayName = device.id;
			}

			normalized.push_back(std::move(device));
		}

		g_settings.preferredDevices = std::move(normalized);
	}

	inline void NormalizeRecentDevices()
	{
		std::sort(g_settings.recentDevices.begin(), g_settings.recentDevices.end(), [](const auto& left, const auto& right) {
			if (left.lastConnectedTimestamp != right.lastConnectedTimestamp)
			{
				return left.lastConnectedTimestamp > right.lastConnectedTimestamp;
			}
			return left.id < right.id;
		});

		std::unordered_set<std::wstring> seen;
		std::vector<SavedDevice> normalized;
		normalized.reserve(g_settings.recentDevices.size());

		for (auto device : g_settings.recentDevices)
		{
			if (device.id.empty())
			{
				continue;
			}

			if (!seen.emplace(device.id).second)
			{
				continue;
			}

			if (device.displayName.empty())
			{
				device.displayName = device.id;
			}

			if (device.lastConnectedTimestamp <= 0)
			{
				device.lastConnectedTimestamp = CurrentTimestamp();
			}

			normalized.push_back(std::move(device));
			if (normalized.size() >= MAX_RECENT_DEVICES)
			{
				break;
			}
		}

		g_settings.recentDevices = std::move(normalized);
	}

	inline void NormalizeSettings()
	{
		NormalizePreferredDevices();
		NormalizeRecentDevices();
	}

	inline void ParsePreferredDevices(const JsonObject& jsonObject)
	{
		if (!jsonObject.HasKey(L"preferredDevices"))
		{
			return;
		}

		auto jsonValue = jsonObject.GetNamedValue(L"preferredDevices");
		if (jsonValue.ValueType() != JsonValueType::Array)
		{
			return;
		}

		for (const auto& entry : jsonValue.GetArray())
		{
			if (entry.ValueType() != JsonValueType::Object)
			{
				continue;
			}

			auto item = entry.GetObject();
			if (!item.HasKey(L"id"))
			{
				continue;
			}

			auto idValue = item.GetNamedValue(L"id");
			if (idValue.ValueType() != JsonValueType::String)
			{
				continue;
			}

			auto id = std::wstring(idValue.GetString());
			auto displayName = item.HasKey(L"displayName") && item.GetNamedValue(L"displayName").ValueType() == JsonValueType::String
				? std::wstring(item.GetNamedValue(L"displayName").GetString())
				: id;

			g_settings.preferredDevices.push_back(MakeSavedDevice(std::move(id), std::move(displayName)));
		}
	}

	inline void ParseRecentDevices(const JsonObject& jsonObject)
	{
		if (jsonObject.HasKey(L"recentDevices"))
		{
			auto jsonValue = jsonObject.GetNamedValue(L"recentDevices");
			if (jsonValue.ValueType() == JsonValueType::Array)
			{
				for (const auto& entry : jsonValue.GetArray())
				{
					if (entry.ValueType() != JsonValueType::Object)
					{
						continue;
					}

					auto item = entry.GetObject();
					if (!item.HasKey(L"id"))
					{
						continue;
					}

					auto idValue = item.GetNamedValue(L"id");
					if (idValue.ValueType() != JsonValueType::String)
					{
						continue;
					}

					auto id = std::wstring(idValue.GetString());
					auto displayName = item.HasKey(L"displayName") && item.GetNamedValue(L"displayName").ValueType() == JsonValueType::String
						? std::wstring(item.GetNamedValue(L"displayName").GetString())
						: id;
					auto timestamp = TryReadTimestamp(item, L"lastConnectedTimestamp", 0);

					g_settings.recentDevices.push_back(MakeSavedDevice(std::move(id), std::move(displayName), timestamp));
				}
				return;
			}
		}

		if (!jsonObject.HasKey(L"lastDevices"))
		{
			return;
		}

		auto lastDevicesValue = jsonObject.GetNamedValue(L"lastDevices");
		if (lastDevicesValue.ValueType() != JsonValueType::Array)
		{
			return;
		}

		auto timestamp = CurrentTimestamp();
		for (const auto& entry : lastDevicesValue.GetArray())
		{
			if (entry.ValueType() != JsonValueType::String)
			{
				continue;
			}

			auto id = std::wstring(entry.GetString());
			g_settings.recentDevices.push_back(MakeSavedDevice(id, id, timestamp--));
		}
	}

	inline void LoadSettings()
	{
		try
		{
			DefaultSettings();

			wil::unique_hfile hFile(CreateFileW(GetConfigPath().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
			if (!hFile)
			{
				if (GetLastError() == ERROR_FILE_NOT_FOUND)
				{
					return;
				}
				THROW_LAST_ERROR();
			}

			std::string jsonUtf8;
			while (1)
			{
				auto currentSize = jsonUtf8.size();
				jsonUtf8.resize(currentSize + BUFFER_SIZE);
				DWORD bytesRead = 0;
				THROW_IF_WIN32_BOOL_FALSE(ReadFile(hFile.get(), jsonUtf8.data() + currentSize, BUFFER_SIZE, &bytesRead, nullptr));
				jsonUtf8.resize(currentSize + bytesRead);
				if (bytesRead == 0)
				{
					break;
				}
			}

			auto jsonObject = JsonObject::Parse(Utf8ToUtf16(jsonUtf8));

			TryReadBoolean(jsonObject, L"autostartEnabled", g_settings.autostartEnabled);
			if (!TryReadBoolean(jsonObject, L"autoConnectRecentEnabled", g_settings.autoConnectRecentEnabled))
			{
				TryReadBoolean(jsonObject, L"reconnect", g_settings.autoConnectRecentEnabled);
			}
			TryReadBoolean(jsonObject, L"autoConnectPreferredEnabled", g_settings.autoConnectPreferredEnabled);

			ParsePreferredDevices(jsonObject);
			ParseRecentDevices(jsonObject);
			NormalizeSettings();
		}
		CATCH_LOG();
	}

	inline JsonObject SerializeSavedDevice(const SavedDevice& device, bool includeTimestamp)
	{
		JsonObject jsonObject;
		jsonObject.Insert(L"id", JsonValue::CreateStringValue(device.id));
		jsonObject.Insert(L"displayName", JsonValue::CreateStringValue(device.displayName));
		if (includeTimestamp)
		{
			jsonObject.Insert(L"lastConnectedTimestamp", JsonValue::CreateNumberValue(static_cast<double>(device.lastConnectedTimestamp)));
		}
		return jsonObject;
	}

	inline void SaveSettings()
	{
		try
		{
			NormalizeSettings();

			JsonObject jsonObject;
			jsonObject.Insert(L"autostartEnabled", JsonValue::CreateBooleanValue(g_settings.autostartEnabled));
			jsonObject.Insert(L"autoConnectRecentEnabled", JsonValue::CreateBooleanValue(g_settings.autoConnectRecentEnabled));
			jsonObject.Insert(L"autoConnectPreferredEnabled", JsonValue::CreateBooleanValue(g_settings.autoConnectPreferredEnabled));

			JsonArray preferredDevices;
			for (const auto& device : g_settings.preferredDevices)
			{
				preferredDevices.Append(SerializeSavedDevice(device, false));
			}
			jsonObject.Insert(L"preferredDevices", preferredDevices);

			JsonArray recentDevices;
			for (const auto& device : g_settings.recentDevices)
			{
				recentDevices.Append(SerializeSavedDevice(device, true));
			}
			jsonObject.Insert(L"recentDevices", recentDevices);

			wil::unique_hfile hFile(CreateFileW(GetConfigPath().c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
			THROW_LAST_ERROR_IF(!hFile);

			auto jsonUtf8 = Utf16ToUtf8(jsonObject.Stringify());
			DWORD bytesWritten = 0;
			THROW_IF_WIN32_BOOL_FALSE(WriteFile(hFile.get(), jsonUtf8.data(), static_cast<DWORD>(jsonUtf8.size()), &bytesWritten, nullptr));
			THROW_HR_IF(E_FAIL, bytesWritten != jsonUtf8.size());
		}
		CATCH_LOG();
	}

	inline void RememberRecentDevice(std::wstring id, std::wstring displayName)
	{
		g_settings.recentDevices.erase(std::remove_if(g_settings.recentDevices.begin(), g_settings.recentDevices.end(), [&](const auto& device) {
			return device.id == id;
		}), g_settings.recentDevices.end());

		g_settings.recentDevices.push_back(MakeSavedDevice(std::move(id), std::move(displayName), CurrentTimestamp()));
		SaveSettings();
	}

	inline bool AddPreferredDevice(std::wstring id, std::wstring displayName)
	{
		auto it = std::find_if(g_settings.preferredDevices.begin(), g_settings.preferredDevices.end(), [&](const auto& device) {
			return device.id == id;
		});
		if (it != g_settings.preferredDevices.end())
		{
			if (!displayName.empty())
			{
				it->displayName = std::move(displayName);
			}
			SaveSettings();
			return false;
		}

		g_settings.preferredDevices.push_back(MakeSavedDevice(std::move(id), std::move(displayName)));
		SaveSettings();
		return true;
	}

	inline void RemovePreferredDevice(std::wstring_view id)
	{
		g_settings.preferredDevices.erase(std::remove_if(g_settings.preferredDevices.begin(), g_settings.preferredDevices.end(), [&](const auto& device) {
			return device.id == id;
		}), g_settings.preferredDevices.end());
		SaveSettings();
	}

	inline void RemoveRecentDevice(std::wstring_view id)
	{
		g_settings.recentDevices.erase(std::remove_if(g_settings.recentDevices.begin(), g_settings.recentDevices.end(), [&](const auto& device) {
			return device.id == id;
		}), g_settings.recentDevices.end());
		SaveSettings();
	}

	inline void ClearRecentDevices()
	{
		g_settings.recentDevices.clear();
		SaveSettings();
	}
}
