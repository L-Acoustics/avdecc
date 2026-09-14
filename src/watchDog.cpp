/*
* Copyright (C) 2016-2026, L-Acoustics and its contributors

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

#include "utils.hpp"
#include "la/avdecc/watchDog.hpp"

#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <iostream>
#include <stdlib.h> // std::getenv
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

	// Interval between two checks of all watches (the shortest maximumInterval registered is 500 msec)
	static constexpr auto CheckInterval = std::chrono::milliseconds{ 100u };
	// Interval between two checks for a debugger, which can be costly (on Linux, it reads /proc/self/status)
	static constexpr auto DebuggerCheckInterval = std::chrono::milliseconds{ 1000u };
	// Without asserts compiled in, a missed watch is only reported to observers, so with none registered nothing needs checking.
	// registerObserver wakes the idle thread without taking the lock (so that an observer can register from a notification),
	// so the idle thread also looks again this often, in case it missed that wakeup
	static constexpr auto IdleInterval = std::chrono::milliseconds{ 10000u };
#if defined(DEBUG) || defined(COMPILE_AVDECC_ASSERT)
	static constexpr auto IsAssertCompiled = true;
#else // !DEBUG && !COMPILE_AVDECC_ASSERT
	static constexpr auto IsAssertCompiled = false;
#endif // DEBUG || COMPILE_AVDECC_ASSERT

public:
	WatchDogImpl() noexcept
	{
		// Create the watch dog thread
		_watchThread = std::thread(
			[this]
			{
				utils::setCurrentThreadName("avdecc::watchDog");

				auto isDebuggerPresent = false;
				auto lastDebuggerCheck = std::chrono::steady_clock::time_point{};

				auto lock = std::unique_lock{ _lock };
				while (!_shouldTerminate)
				{
					// Nothing would act on a missed watch: wait for an observer rather than check
					if (!IsAssertCompiled && _observers.countObservers() == 0)
					{
						_wakeCondition.wait_for(lock, IdleInterval,
							[this]
							{
								return _shouldTerminate || _observers.countObservers() != 0;
							});
						continue;
					}

					// Look for a debugger once in a while, rather than for every watch on every check
					if (!_watched.empty() && std::chrono::steady_clock::now() - lastDebuggerCheck >= DebuggerCheckInterval)
					{
						isDebuggerPresent = utils::isDebuggerPresent();
						lastDebuggerCheck = std::chrono::steady_clock::now();
					}
					checkWatches(isDebuggerPresent);

					// Wait until the next check, unless asked to terminate first
					_wakeCondition.wait_for(lock, CheckInterval,
						[this]
						{
							return _shouldTerminate;
						});
				}
			});
	}
	virtual ~WatchDogImpl() noexcept override
	{
		// Notify the thread we are shutting down
		{
			auto const lg = std::scoped_lock{ _lock };
			_shouldTerminate = true;
		}
		_wakeCondition.notify_all();

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

		// Wake the thread, which may be idle for want of an observer (not under the lock, see IdleInterval)
		_wakeCondition.notify_all();
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

	// Checks all watches, with the lock held
	void checkWatches(bool const isDebuggerPresent) noexcept
	{
		auto const currentTime = std::chrono::system_clock::now();
		for (auto& [threadId, watchedMap] : _watched)
		{
			for (auto& [name, watchInfo] : watchedMap)
			{
				checkWatch(name, watchInfo, currentTime, isDebuggerPresent);
			}
		}
	}

	void checkWatch(std::string const& name, WatchInfo& watchInfo, std::chrono::time_point<std::chrono::system_clock> const currentTime, bool const isDebuggerPresent) noexcept
	{
		// If debugger is present, update the last alive time and don't check the timeout
		if (isDebuggerPresent)
		{
			watchInfo.lastAlive = currentTime;
		}

		// Check if we timed out
		if (!watchInfo.ignore && std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - watchInfo.lastAlive).count() > watchInfo.maximumInterval.count())
		{
			_observers.notifyObserversMethod<Observer>(&Observer::onIntervalExceeded, name, watchInfo.maximumInterval);

			// Only print message if "AVDECC_NO_WATCHDOG_ASSERT" is not defined
			if (std::getenv("AVDECC_NO_WATCHDOG_ASSERT") == nullptr)
			{
				auto stream = std::stringstream{};
				stream << "WatchDog event '" << name << "' exceeded the maximum allowed time (ThreadId: 0x" << std::hex << watchInfo.threadId << "). Deadlock?";
				AVDECC_ASSERT(false, stream.str());
			}

			watchInfo.ignore = true;
		}
	}

	using WatchedMap = std::unordered_map<std::string, WatchInfo>;

	// Private members
	std::mutex _lock{};
	std::unordered_map<std::thread::id, WatchedMap> _watched{};
	//WatchedMap _watched{};
	bool _shouldTerminate{ false };
	std::condition_variable _wakeCondition{};
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
