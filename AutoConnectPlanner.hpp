#pragma once

namespace AutoConnectPlanner
{
	inline std::vector<std::wstring> BuildQueue()
	{
		std::vector<std::wstring> queue;
		queue.reserve(g_settings.recentDevices.size() + g_settings.preferredDevices.size());

		std::unordered_set<std::wstring> seen;
		auto appendIfNeeded = [&](std::wstring_view deviceId) {
			if (!deviceId.empty() && seen.emplace(deviceId).second)
			{
				queue.emplace_back(deviceId);
			}
		};

		if (g_settings.autoConnectRecentEnabled)
		{
			for (const auto& device : g_settings.recentDevices)
			{
				appendIfNeeded(device.id);
			}
		}

		if (g_settings.autoConnectPreferredEnabled)
		{
			for (const auto& device : g_settings.preferredDevices)
			{
				appendIfNeeded(device.id);
			}
		}

		return queue;
	}
}
