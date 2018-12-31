/*
* Copyright (C) 2016-2018, L-Acoustics and its contributors

* This file is part of LA_avdecc.

* LA_avdecc is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* LA_avdecc is distributed in the hope that it will be usefu_state,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public License
* along with LA_avdecc.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* @file watchDog.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/watchDog.hpp"
#include <unordered_map>
#include <thread>
#ifdef _WIN32
#	include <Windows.h>
#endif // _WIN32

namespace la
{
namespace avdecc
{
namespace watchDog
{
class WatchDogImpl final : public WatchDog
{
private:
	struct WatchInfo
	{
		std::chrono::milliseconds maximumInterval{ 0u };
		std::chrono::time_point<std::chrono::system_clock> lastAlive{ std::chrono::system_clock::now() };
		bool ignore{ false };
	};

public:
	WatchDogImpl() noexcept
	{
		// Create the watch dog thread
		_watchThread = std::thread(
			[this]
			{
				setCurrentThreadName("avdecc::watchDog");
				while (!_shouldTerminate)
				{
					// Check all watch
					{
						auto const lg = std::lock_guard{ _lock };

						auto const currentTime = std::chrono::system_clock::now();
						for (auto& [name, watchInfo] : _watched)
						{
#ifdef _WIN32
							// If debugger is present, update the last alive time and don't check the timeout
							if (IsDebuggerPresent())
							{
								watchInfo.lastAlive = currentTime;
							}
#endif // _WIN32

							// Check if we timed out
							if (!watchInfo.ignore && std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - watchInfo.lastAlive).count() > watchInfo.maximumInterval.count())
							{
								_observers.notifyObserversMethod<Observer>(&Observer::onIntervalExceeded, name, watchInfo.maximumInterval);
								AVDECC_ASSERT(false, "WatchDog event '" + name + "' exceeded the maximum allowed time. Deadlock?");
								watchInfo.ignore = true;
							}
						}
					}
					// Wait a little bit so we don't burn the CPU
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
			});
	}
	virtual ~WatchDogImpl() noexcept override
	{
		// Notify the thread we are shutting down
		_shouldTerminate = true;

		// Wait for the thread to complete its pending tasks
		if (_watchThread.joinable())
			_watchThread.join();
	}

	// Defaulted compiler auto-generated methods
	WatchDogImpl(WatchDogImpl&&) = default;
	WatchDogImpl(WatchDogImpl const&) = default;
	WatchDogImpl& operator=(WatchDogImpl const&) = default;
	WatchDogImpl& operator=(WatchDogImpl&&) = default;

private:
	// WatchDog overrides
	virtual void registerObserver(Observer* const observer) noexcept override
	{
		_observers.registerObserver(observer);
	}

	virtual void unregisterObserver(Observer* const observer) noexcept override
	{
		_observers.unregisterObserver(observer);
	}

	virtual void registerWatch(std::string const& name, std::chrono::milliseconds const maximumInterval) noexcept override
	{
		auto const lg = std::lock_guard{ _lock };
		_watched[name] = { maximumInterval };
	}

	virtual void unregisterWatch(std::string const& name) noexcept override
	{
		auto const lg = std::lock_guard{ _lock };
		_watched.erase(name);
	}

	virtual void alive(std::string const& name) noexcept override
	{
		auto const lg = std::lock_guard{ _lock };
		auto const watchIt = _watched.find(name);
		if (watchIt != _watched.end())
		{
			watchIt->second.lastAlive = std::chrono::system_clock::now();
		}
	}

	// Private members
	std::mutex _lock{};
	std::unordered_map<std::string, WatchInfo> _watched{};
	bool _shouldTerminate{ false };
	std::thread _watchThread{};
	Subject _observers{};
};

WatchDog::SharedPointer LA_AVDECC_CALL_CONVENTION WatchDog::getInstance() noexcept
{
	static auto s_Instance{ std::make_shared<WatchDogImpl>() };

	return s_Instance;
}

} // namespace watchDog
} // namespace avdecc
} // namespace la
