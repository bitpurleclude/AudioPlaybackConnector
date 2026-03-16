#pragma once

namespace SettingsWindow
{
	constexpr wchar_t WINDOW_CLASS_NAME[] = L"AudioPlaybackConnectorSettings";
	constexpr int WINDOW_WIDTH = 520;
	constexpr int WINDOW_HEIGHT = 640;

	struct State
	{
		HWND hWnd = nullptr;
		HWND hWndXaml = nullptr;
		DesktopWindowXamlSource desktopSource = nullptr;
		winrt::com_ptr<IDesktopWindowXamlSourceNative2> desktopSourceNative2;
		ScrollViewer scrollViewer = nullptr;
		StackPanel contentPanel = nullptr;
		CheckBox autostartCheckBox = nullptr;
		CheckBox autoConnectRecentCheckBox = nullptr;
		CheckBox autoConnectPreferredCheckBox = nullptr;
		Button addPreferredDeviceButton = nullptr;
		Button clearRecentDevicesButton = nullptr;
		StackPanel preferredDevicesPanel = nullptr;
		StackPanel recentDevicesPanel = nullptr;
		DevicePicker preferredDevicePicker = nullptr;
		bool isRefreshing = false;
	};

	inline State& GetState()
	{
		static State state;
		return state;
	}

	inline bool IsOpen()
	{
		return GetState().hWnd != nullptr;
	}

	inline HWND GetHwnd()
	{
		return GetState().hWnd;
	}

	inline const winrt::com_ptr<IDesktopWindowXamlSourceNative2>& GetDesktopSourceNative2()
	{
		return GetState().desktopSourceNative2;
	}

	inline bool IsChecked(const CheckBox& checkBox)
	{
		auto value = checkBox.IsChecked();
		return value ? value.Value() : false;
	}

	inline std::wstring ShortDeviceId(std::wstring_view deviceId)
	{
		if (deviceId.size() <= 40)
		{
			return std::wstring(deviceId);
		}

		return std::wstring(deviceId.substr(0, 18)) + L"..." + std::wstring(deviceId.substr(deviceId.size() - 18));
	}

	inline std::wstring FormatSavedDevice(const SavedDevice& device)
	{
		if (device.displayName.empty() || device.displayName == device.id)
		{
			return ShortDeviceId(device.id);
		}

		return device.displayName + L"\n" + ShortDeviceId(device.id);
	}

	inline void ShowErrorDialog(std::wstring_view message)
	{
		auto dialogMessage = std::wstring(message);
		TaskDialog(GetState().hWnd, nullptr, _(L"Settings Error"), nullptr, dialogMessage.c_str(), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
	}

	inline void Refresh();

	inline StackPanel CreateDeviceRow(const SavedDevice& device, bool isRecentList)
	{
		StackPanel row;
		row.Margin({ 0, 0, 0, 12 });

		TextBlock label;
		label.Text(FormatSavedDevice(device));
		label.TextWrapping(TextWrapping::Wrap);
		label.Margin({ 0, 0, 0, 4 });

		Button removeButton;
		removeButton.Content(winrt::box_value(_(L"Remove")));
		removeButton.HorizontalAlignment(HorizontalAlignment::Left);

		auto deviceId = std::wstring(device.id);
		removeButton.Click([deviceId = std::move(deviceId), isRecentList](const auto&, const auto&) {
			if (isRecentList)
			{
				SettingsStore::RemoveRecentDevice(deviceId);
			}
			else
			{
				SettingsStore::RemovePreferredDevice(deviceId);
			}
			Refresh();
		});

		row.Children().Append(label);
		row.Children().Append(removeButton);
		return row;
	}

	inline void RefreshDevicePanels()
	{
		auto& state = GetState();
		if (!state.hWnd)
		{
			return;
		}

		state.preferredDevicesPanel.Children().Clear();
		if (g_settings.preferredDevices.empty())
		{
			TextBlock emptyText;
			emptyText.Text(_(L"No preferred devices"));
			state.preferredDevicesPanel.Children().Append(emptyText);
		}
		else
		{
			for (const auto& device : g_settings.preferredDevices)
			{
				state.preferredDevicesPanel.Children().Append(CreateDeviceRow(device, false));
			}
		}

		state.recentDevicesPanel.Children().Clear();
		if (g_settings.recentDevices.empty())
		{
			TextBlock emptyText;
			emptyText.Text(_(L"No recent devices"));
			state.recentDevicesPanel.Children().Append(emptyText);
		}
		else
		{
			for (const auto& device : g_settings.recentDevices)
			{
				state.recentDevicesPanel.Children().Append(CreateDeviceRow(device, true));
			}
		}

		state.clearRecentDevicesButton.IsEnabled(!g_settings.recentDevices.empty());
	}

	inline Rect GetPickerRect(HWND hWnd)
	{
		RECT rect{};
		GetWindowRect(hWnd, &rect);
		auto dpi = GetDpiForWindow(hWnd);

		return Rect{
			static_cast<float>(rect.left * USER_DEFAULT_SCREEN_DPI / dpi),
			static_cast<float>(rect.top * USER_DEFAULT_SCREEN_DPI / dpi),
			static_cast<float>((rect.right - rect.left) * USER_DEFAULT_SCREEN_DPI / dpi),
			static_cast<float>((rect.bottom - rect.top) * USER_DEFAULT_SCREEN_DPI / dpi)
		};
	}

	inline void Refresh()
	{
		auto& state = GetState();
		if (!state.hWnd)
		{
			return;
		}

		state.isRefreshing = true;
		try
		{
			auto autostartStatus = AutostartManager::RepairIfNeeded();
			auto actualAutostartEnabled = autostartStatus.enabled;
			if (actualAutostartEnabled != g_settings.autostartEnabled)
			{
				g_settings.autostartEnabled = actualAutostartEnabled;
				SettingsStore::SaveSettings();
			}
		}
		catch (const winrt::hresult_error&)
		{
			LOG_CAUGHT_EXCEPTION();
		}

		state.autostartCheckBox.IsChecked(g_settings.autostartEnabled);
		state.autoConnectRecentCheckBox.IsChecked(g_settings.autoConnectRecentEnabled);
		state.autoConnectPreferredCheckBox.IsChecked(g_settings.autoConnectPreferredEnabled);
		RefreshDevicePanels();
		state.isRefreshing = false;
	}

	inline void ShowPreferredDevicePicker()
	{
		auto& state = GetState();
		if (!state.preferredDevicePicker)
		{
			return;
		}

		state.preferredDevicePicker.Show(GetPickerRect(state.hWnd), Placement::Below);
	}

	inline void CreateXamlContent()
	{
		auto& state = GetState();

		state.desktopSource = DesktopWindowXamlSource();
		state.desktopSourceNative2 = state.desktopSource.as<IDesktopWindowXamlSourceNative2>();
		winrt::check_hresult(state.desktopSourceNative2->AttachToWindow(state.hWnd));
		winrt::check_hresult(state.desktopSourceNative2->get_WindowHandle(&state.hWndXaml));

		state.contentPanel = StackPanel();
		state.contentPanel.Margin({ 16, 16, 16, 16 });

		TextBlock startupHeader;
		startupHeader.Text(_(L"Startup"));
		startupHeader.Margin({ 0, 0, 0, 8 });
		state.contentPanel.Children().Append(startupHeader);

		state.autostartCheckBox = CheckBox();
		state.autostartCheckBox.Content(winrt::box_value(_(L"Start on Windows sign-in")));
		state.autostartCheckBox.Margin({ 0, 0, 0, 4 });
		state.autostartCheckBox.Click([](const auto&, const auto&) {
			auto& currentState = GetState();
			if (currentState.isRefreshing)
			{
				return;
			}

			auto desiredState = IsChecked(currentState.autostartCheckBox);
			try
			{
				if (desiredState)
				{
					AutostartManager::Enable();
				}
				else
				{
					AutostartManager::Disable();
				}

				g_settings.autostartEnabled = desiredState;
				SettingsStore::SaveSettings();
			}
			catch (const winrt::hresult_error& ex)
			{
				LOG_CAUGHT_EXCEPTION();
				ShowErrorDialog(std::wstring(_(L"Failed to update startup task")) + L"\n" + ex.message().c_str());
			}

			Refresh();
		});
		state.contentPanel.Children().Append(state.autostartCheckBox);

		TextBlock startupHint;
		startupHint.Text(_(L"Uses Task Scheduler and starts after a 15 second delay"));
		startupHint.TextWrapping(TextWrapping::Wrap);
		startupHint.Margin({ 0, 0, 0, 16 });
		state.contentPanel.Children().Append(startupHint);

		TextBlock autoConnectHeader;
		autoConnectHeader.Text(_(L"Auto connect"));
		autoConnectHeader.Margin({ 0, 0, 0, 8 });
		state.contentPanel.Children().Append(autoConnectHeader);

		state.autoConnectRecentCheckBox = CheckBox();
		state.autoConnectRecentCheckBox.Content(winrt::box_value(_(L"Automatically connect recent devices")));
		state.autoConnectRecentCheckBox.Margin({ 0, 0, 0, 4 });
		state.autoConnectRecentCheckBox.Click([](const auto&, const auto&) {
			auto& currentState = GetState();
			if (currentState.isRefreshing)
			{
				return;
			}

			g_settings.autoConnectRecentEnabled = IsChecked(currentState.autoConnectRecentCheckBox);
			SettingsStore::SaveSettings();
			Refresh();
		});
		state.contentPanel.Children().Append(state.autoConnectRecentCheckBox);

		state.autoConnectPreferredCheckBox = CheckBox();
		state.autoConnectPreferredCheckBox.Content(winrt::box_value(_(L"Automatically connect preferred devices")));
		state.autoConnectPreferredCheckBox.Margin({ 0, 0, 0, 16 });
		state.autoConnectPreferredCheckBox.Click([](const auto&, const auto&) {
			auto& currentState = GetState();
			if (currentState.isRefreshing)
			{
				return;
			}

			g_settings.autoConnectPreferredEnabled = IsChecked(currentState.autoConnectPreferredCheckBox);
			SettingsStore::SaveSettings();
			Refresh();
		});
		state.contentPanel.Children().Append(state.autoConnectPreferredCheckBox);

		TextBlock preferredHeader;
		preferredHeader.Text(_(L"Preferred devices"));
		preferredHeader.Margin({ 0, 0, 0, 8 });
		state.contentPanel.Children().Append(preferredHeader);

		state.addPreferredDeviceButton = Button();
		state.addPreferredDeviceButton.Content(winrt::box_value(_(L"Add device")));
		state.addPreferredDeviceButton.HorizontalAlignment(HorizontalAlignment::Left);
		state.addPreferredDeviceButton.Margin({ 0, 0, 0, 8 });
		state.addPreferredDeviceButton.Click([](const auto&, const auto&) {
			ShowPreferredDevicePicker();
		});
		state.contentPanel.Children().Append(state.addPreferredDeviceButton);

		state.preferredDevicesPanel = StackPanel();
		state.preferredDevicesPanel.Margin({ 0, 0, 0, 16 });
		state.contentPanel.Children().Append(state.preferredDevicesPanel);

		TextBlock recentHeader;
		recentHeader.Text(_(L"Recent devices"));
		recentHeader.Margin({ 0, 0, 0, 8 });
		state.contentPanel.Children().Append(recentHeader);

		state.clearRecentDevicesButton = Button();
		state.clearRecentDevicesButton.Content(winrt::box_value(_(L"Clear")));
		state.clearRecentDevicesButton.HorizontalAlignment(HorizontalAlignment::Left);
		state.clearRecentDevicesButton.Margin({ 0, 0, 0, 8 });
		state.clearRecentDevicesButton.Click([](const auto&, const auto&) {
			SettingsStore::ClearRecentDevices();
			Refresh();
		});
		state.contentPanel.Children().Append(state.clearRecentDevicesButton);

		state.recentDevicesPanel = StackPanel();
		state.contentPanel.Children().Append(state.recentDevicesPanel);

		state.scrollViewer = ScrollViewer();
		state.scrollViewer.Content(state.contentPanel);
		state.desktopSource.Content(state.scrollViewer);

		state.preferredDevicePicker = DevicePicker();
		winrt::check_hresult(state.preferredDevicePicker.as<IInitializeWithWindow>()->Initialize(state.hWnd));
		state.preferredDevicePicker.Filter().SupportedDeviceSelectors().Append(AudioPlaybackConnection::GetDeviceSelector());
		state.preferredDevicePicker.DeviceSelected([](const auto& sender, const auto& args) {
			auto device = args.SelectedDevice();
			SettingsStore::AddPreferredDevice(std::wstring(device.Id()), std::wstring(device.Name()));
			g_settings.autoConnectPreferredEnabled = true;
			SettingsStore::SaveSettings();
			sender.SetDisplayStatus(device, _(L"Added"), DevicePickerDisplayStatusOptions::None);
			Refresh();
		});

		RECT clientRect{};
		GetClientRect(state.hWnd, &clientRect);
		SetWindowPos(state.hWndXaml, nullptr, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_SHOWWINDOW);

		Refresh();
	}

	inline void DestroyContent()
	{
		auto& state = GetState();
		if (state.desktopSource)
		{
			state.desktopSource.Content(nullptr);
		}
		state.preferredDevicePicker = nullptr;
		state.scrollViewer = nullptr;
		state.contentPanel = nullptr;
		state.autostartCheckBox = nullptr;
		state.autoConnectRecentCheckBox = nullptr;
		state.autoConnectPreferredCheckBox = nullptr;
		state.addPreferredDeviceButton = nullptr;
		state.clearRecentDevicesButton = nullptr;
		state.preferredDevicesPanel = nullptr;
		state.recentDevicesPanel = nullptr;
		state.desktopSource = nullptr;
		state.desktopSourceNative2 = nullptr;
		state.hWndXaml = nullptr;
		state.hWnd = nullptr;
		state.isRefreshing = false;
	}

	inline LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CREATE:
			GetState().hWnd = hWnd;
			CreateXamlContent();
			return 0;
		case WM_SIZE:
			if (GetState().hWndXaml)
			{
				SetWindowPos(GetState().hWndXaml, nullptr, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_SHOWWINDOW);
			}
			return 0;
		case WM_CLOSE:
			DestroyWindow(hWnd);
			return 0;
		case WM_DESTROY:
			DestroyContent();
			return 0;
		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);
		}
	}

	inline void RegisterWindowClass()
	{
		static bool registered = false;
		if (registered)
		{
			return;
		}

		WNDCLASSEXW wcex = {
			.cbSize = sizeof(wcex),
			.lpfnWndProc = WndProc,
			.hInstance = g_hInst,
			.hIcon = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_AUDIOPLAYBACKCONNECTOR)),
			.hCursor = LoadCursorW(nullptr, IDC_ARROW),
			.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
			.lpszClassName = WINDOW_CLASS_NAME,
			.hIconSm = wcex.hIcon
		};

		FAIL_FAST_LAST_ERROR_IF(RegisterClassExW(&wcex) == 0);
		registered = true;
	}

	inline void Show()
	{
		RegisterWindowClass();

		auto& state = GetState();
		if (state.hWnd)
		{
			ShowWindow(state.hWnd, SW_SHOWNORMAL);
			SetForegroundWindow(state.hWnd);
			Refresh();
			return;
		}

		auto title = std::wstring(_(L"AudioPlaybackConnector Settings"));
		auto hWnd = CreateWindowExW(
			0,
			WINDOW_CLASS_NAME,
			title.c_str(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			WINDOW_WIDTH,
			WINDOW_HEIGHT,
			nullptr,
			nullptr,
			g_hInst,
			nullptr);
		FAIL_FAST_LAST_ERROR_IF_NULL(hWnd);

		ShowWindow(hWnd, SW_SHOWNORMAL);
		UpdateWindow(hWnd);
		SetForegroundWindow(hWnd);
	}
}
