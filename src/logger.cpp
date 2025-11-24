/*
* Copyright (C) 2016-2025, L-Acoustics and its contributors

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
* @file logger.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/logger.hpp"
#include "la/avdecc/utils.hpp"

#include <vector>
#include <mutex>
#include <algorithm>
#include <cassert>

namespace la
{
namespace avdecc
{
namespace logger
{
class LoggerImpl final : public Logger
{
public:
	virtual void registerObserver(Observer* const observer) noexcept override
	{
		std::lock_guard<decltype(_lock)> const lg(_lock);
		_observers.push_back(observer);
	}

	virtual void unregisterObserver(Observer* const observer) noexcept override
	{
		std::lock_guard<decltype(_lock)> const lg(_lock);
		_observers.erase(std::remove_if(std::begin(_observers), std::end(_observers),
											 [observer](Observer* const o)
											 {
												 return o == observer;
											 }),
			_observers.end());
	}

	virtual void logItem(la::avdecc::logger::Level const level, LogItem const* const item) noexcept override
	{
		// Check if message level is currently active
		if (level < _level)
		{
			return;
		}

		std::lock_guard<decltype(_lock)> const lg(_lock);
		for (auto* o : _observers)
		{
			utils::invokeProtectedMethod(&Observer::onLogItem, o, level, item);
		}
	}

	virtual void setLevel(Level const level) noexcept override
	{
		_level = level;
#ifndef DEBUG
		// In release, we don't want Trace nor Debug levels, setting to next possible Level (Info)
		if (_level == Level::Trace || _level == Level::Debug)
		{
			_level = Level::Info;
		}
#endif // !DEBUG
	}

	virtual Level getLevel() const noexcept override
	{
		return _level;
	}

	virtual std::string layerToString(Layer const layer) const noexcept override
	{
		if (layer < Layer::FirstUserLayer)
		{
			switch (layer)
			{
				case Layer::Generic:
					return "Generic";
				case Layer::Serialization:
					return "Serialization";
				case Layer::ProtocolInterface:
					return "Protocol Interface";
				case Layer::AemPayload:
					return "Aem Payload";
				case Layer::Entity:
					return "Entity";
				case Layer::ControllerEntity:
					return "Controller Entity";
				case Layer::ControllerStateMachine:
					return "Controller State Machine";
				case Layer::Controller:
					return "Controller";
				case Layer::JsonSerializer:
					return "Json Serializer";
				default:
					AVDECC_ASSERT(false, "Layer not handled");
			}
		}
		return "Layer" + std::to_string(std::underlying_type_t<Layer>(layer));
	}

	virtual std::string levelToString(Level const level) const noexcept override
	{
		switch (level)
		{
#ifdef DEBUG
			case Level::Trace:
				return "Trace";
			case Level::Debug:
				return "Debug";
#endif // DEBUG
			case Level::Info:
				return "Info";
			case Level::Warn:
				return "Warn";
			case Level::Error:
				return "Error";
			case Level::Compat:
				return "Compatibility";
			case Level::None:
				return "None";
			default:
				AVDECC_ASSERT(false, "Level not handled");
		}
		return "Unknown Level";
	}

	// Defaulted compiler auto-generated methods
	LoggerImpl() noexcept {}
	virtual ~LoggerImpl() noexcept override = default;
	LoggerImpl(LoggerImpl&&) = delete;
	LoggerImpl(LoggerImpl const&) = delete;
	LoggerImpl& operator=(LoggerImpl const&) = delete;
	LoggerImpl& operator=(LoggerImpl&&) = delete;

private:
	std::mutex _lock{};
	std::vector<Observer*> _observers{};
	Level _level{ Level::None };
};

Logger& LA_AVDECC_CALL_CONVENTION Logger::getInstance() noexcept
{
	static LoggerImpl s_Instance{};

	return s_Instance;
}

} // namespace logger
} // namespace avdecc
} // namespace la
