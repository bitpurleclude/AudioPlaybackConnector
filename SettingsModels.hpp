#pragma once

constexpr auto CONFIG_NAME = L"AudioPlaybackConnector.json";
constexpr size_t MAX_RECENT_DEVICES = 20;

struct SavedDevice
{
	std::wstring id;
	std::wstring displayName;
	int64_t lastConnectedTimestamp = 0;
};

struct AppSettings
{
	bool autostartEnabled = false;
	bool autoConnectRecentEnabled = false;
	bool autoConnectPreferredEnabled = false;
	std::vector<SavedDevice> preferredDevices;
	std::vector<SavedDevice> recentDevices;
};
