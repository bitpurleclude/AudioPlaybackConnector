#pragma once

namespace ConnectionController
{
	inline std::wstring GetDeviceDisplayName(const DeviceInformation& device)
	{
		auto name = std::wstring(device.Name());
		return name.empty() ? std::wstring(device.Id()) : name;
	}

	inline winrt::fire_and_forget ConnectDevice(DevicePicker picker, DeviceInformation device)
	{
		if (g_audioPlaybackConnections.find(std::wstring(device.Id())) != g_audioPlaybackConnections.end())
		{
			picker.SetDisplayStatus(device, _(L"Connected"), DevicePickerDisplayStatusOptions::ShowDisconnectButton);
			co_return;
		}

		picker.SetDisplayStatus(device, _(L"Connecting"), DevicePickerDisplayStatusOptions::ShowProgress | DevicePickerDisplayStatusOptions::ShowDisconnectButton);

		bool success = false;
		std::wstring errorMessage;

		try
		{
			auto connection = AudioPlaybackConnection::TryCreateFromId(device.Id());
			if (connection)
			{
				g_audioPlaybackConnections.emplace(device.Id(), std::pair(device, connection));

				connection.StateChanged([](const auto& sender, const auto&) {
					if (sender.State() == AudioPlaybackConnectionState::Closed)
					{
						auto it = g_audioPlaybackConnections.find(std::wstring(sender.DeviceId()));
						if (it != g_audioPlaybackConnections.end())
						{
							g_devicePicker.SetDisplayStatus(it->second.first, {}, DevicePickerDisplayStatusOptions::None);
							g_audioPlaybackConnections.erase(it);
						}
						sender.Close();
					}
				});

				co_await connection.StartAsync();
				auto result = co_await connection.OpenAsync();

				switch (result.Status())
				{
				case AudioPlaybackConnectionOpenResultStatus::Success:
					success = true;
					break;
				case AudioPlaybackConnectionOpenResultStatus::RequestTimedOut:
					errorMessage = _(L"The request timed out");
					break;
				case AudioPlaybackConnectionOpenResultStatus::DeniedBySystem:
					errorMessage = _(L"The operation was denied by the system");
					break;
				case AudioPlaybackConnectionOpenResultStatus::UnknownFailure:
					winrt::throw_hresult(result.ExtendedError());
					break;
				}
			}
			else
			{
				errorMessage = _(L"Unknown error");
			}
		}
		catch (const winrt::hresult_error& ex)
		{
			errorMessage.resize(64);
			while (1)
			{
				auto result = swprintf(errorMessage.data(), errorMessage.size(), L"%s (0x%08X)", ex.message().c_str(), static_cast<uint32_t>(ex.code()));
				if (result < 0)
				{
					errorMessage.resize(errorMessage.size() * 2);
				}
				else
				{
					errorMessage.resize(result);
					break;
				}
			}
			LOG_CAUGHT_EXCEPTION();
		}

		if (!success && errorMessage.empty())
		{
			errorMessage = _(L"Unknown error");
		}

		if (success)
		{
			SettingsStore::RememberRecentDevice(std::wstring(device.Id()), GetDeviceDisplayName(device));
			picker.SetDisplayStatus(device, _(L"Connected"), DevicePickerDisplayStatusOptions::ShowDisconnectButton);
			SettingsWindow::Refresh();
		}
		else
		{
			auto it = g_audioPlaybackConnections.find(std::wstring(device.Id()));
			if (it != g_audioPlaybackConnections.end())
			{
				it->second.second.Close();
				g_audioPlaybackConnections.erase(it);
			}
			picker.SetDisplayStatus(device, errorMessage, DevicePickerDisplayStatusOptions::ShowRetryButton);
		}
	}

	inline winrt::fire_and_forget ConnectDevice(DevicePicker picker, std::wstring_view deviceId)
	{
		try
		{
			auto device = co_await DeviceInformation::CreateFromIdAsync(deviceId);
			ConnectDevice(picker, device);
		}
		catch (const winrt::hresult_error&)
		{
			LOG_CAUGHT_EXCEPTION();
		}
	}

	inline void ConnectConfiguredDevices()
	{
		for (const auto& deviceId : AutoConnectPlanner::BuildQueue())
		{
			ConnectDevice(g_devicePicker, deviceId);
		}
	}
}
