/*
* Copyright (C) 2016-2023, L-Acoustics and its contributors

* This file is part of LA_avdecc.

* LA_avdecc is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* LA_avdecc is distributed in the hope that it will be useful,
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
#include <string>
#include <iostream>
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
		std::thread::id threadId{};
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
				utils::setCurrentThreadName("avdecc::watchDog");
				while (!_shouldTerminate)
				{
					// Check all watch
					{
						auto const lg = std::lock_guard{ _lock };

						auto const currentTime = std::chrono::system_clock::now();
						for (auto& [threadId, watchedMap] : _watched)
						{
							for (auto& [name, watchInfo] : watchedMap)
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

									auto stream = std::stringstream{};
									stream << "WatchDog event '" << name << "' exceeded the maximum allowed time (ThreadId: 0x" << std::hex << watchInfo.threadId << "). Deadlock?";
									AVDECC_ASSERT(false, stream.str());

									watchInfo.ignore = true;
								}
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
	WatchDogImpl(WatchDogImpl&&) = delete;
	WatchDogImpl(WatchDogImpl const&) = delete;
	WatchDogImpl& operator=(WatchDogImpl const&) = delete;
	WatchDogImpl& operator=(WatchDogImpl&&) = delete;

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

	virtual void registerWatch(std::string const& name, std::chrono::milliseconds const maximumInterval, bool const isThreadSpecific) noexcept override
	{
		auto const lg = std::lock_guard{ _lock };

		auto const thisId = std::this_thread::get_id();
		auto const threadId = isThreadSpecific ? thisId : std::thread::id{};

		auto& watched = _watched[threadId];

		AVDECC_ASSERT(watched.count(name) == 0, "WatchDog already exists for this 'name'");
		watched[name] = { maximumInterval, thisId };
	}

	virtual void unregisterWatch(std::string const& name, bool const isThreadSpecific) noexcept override
	{
		auto const lg = std::lock_guard{ _lock };

		auto const threadId = isThreadSpecific ? std::this_thread::get_id() : std::thread::id{};

		if (auto watchedThreadIt = _watched.find(threadId); AVDECC_ASSERT_WITH_RET(watchedThreadIt != _watched.end(), "Cannot unregisterWatch, no watch for this thread"))
		{
			auto& watchedThread = watchedThreadIt->second;
			AVDECC_ASSERT_WITH_RET(watchedThread.erase(name) == 1, "Cannot unregisterWatch, 'name' not found");

			// Last one
			if (watchedThread.size() == 0)
			{
				_watched.erase(watchedThreadIt);
			}
		}
	}

	virtual void alive(std::string const& name, bool const isThreadSpecific) noexcept override
	{
		auto const lg = std::lock_guard{ _lock };

		auto const thisId = std::this_thread::get_id();
		auto const threadId = isThreadSpecific ? thisId : std::thread::id{};

		if (auto watchedThreadIt = _watched.find(threadId); AVDECC_ASSERT_WITH_RET(watchedThreadIt != _watched.end(), "Cannot alive, no watch for this thread"))
		{
			auto& watchedThread = watchedThreadIt->second;

			if (auto watchedIt = watchedThread.find(name); AVDECC_ASSERT_WITH_RET(watchedIt != watchedThread.end(), "Cannot alive, 'name' not found"))
			{
				watchedIt->second.threadId = thisId;
				watchedIt->second.lastAlive = std::chrono::system_clock::now();
			}
		}
	}

	using WatchedMap = std::unordered_map<std::string, WatchInfo>;

	// Private members
	std::mutex _lock{};
	std::unordered_map<std::thread::id, WatchedMap> _watched{};
	//WatchedMap _watched{};
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
