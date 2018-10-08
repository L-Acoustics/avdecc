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
* @file logger.hpp
* @author Christophe Calmejane
* @brief Simple logger class.
*/

#pragma once

#include "internals/exports.hpp"
#include <string>

namespace la
{
namespace avdecc
{
namespace logger
{
/** Log layers */
enum class Layer
{
	Generic = 0,
	Serialization = 1,
	ProtocolInterface = 2,
	AemPayload = 3,
	Entity = 4,
	ControllerEntity = 5,
	ControllerStateMachine = 6,
	Controller = 7,
	FirstUserLayer = 100,
};

/** Log levels */
enum class Level
{
	Trace = 0, /**< Very verbose level (always disabled in Release) */
	Debug = 1, /**< Verbose level (always disabled in Release) */
	Info = 2, /**< Information level */
	Warn = 3, /**< Warning level */
	Error = 4, /**< Error level */
	None = 99, /**< No logging level */
};

/** Base class for a LogItem to be logged */
class LogItem
{
public:
	LogItem(Layer const layer) noexcept
		: _layer(layer)
	{
	}
	virtual ~LogItem() noexcept {}

	Layer getLayer() const noexcept
	{
		return _layer;
	}
	virtual std::string getMessage() const noexcept = 0;

	// Defaulted compiler auto-generated methods
	LogItem(LogItem&&) = default;
	LogItem(LogItem const&) = default;
	LogItem& operator=(LogItem const&) = default;
	LogItem& operator=(LogItem&&) = default;

private:
	Layer const _layer{ Layer::Generic };
};

/** Simple logger class declaration */
class Logger
{
public:
	/** Observer interface for the Logger */
	class Observer
	{
	public:
		virtual void onLogItem(la::avdecc::logger::Level const /*level*/, la::avdecc::logger::LogItem const* const /*item*/) noexcept {}
	};

	static LA_AVDECC_API Logger& LA_AVDECC_CALL_CONVENTION getInstance() noexcept;

	virtual void registerObserver(Observer* const observer) noexcept = 0;
	virtual void unregisterObserver(Observer* const observer) noexcept = 0;

	virtual void logItem(Level const level, LogItem const* const item) noexcept = 0;
	virtual void setLevel(Level const level) noexcept = 0;
	virtual Level getLevel() const noexcept = 0;

	virtual std::string layerToString(Layer const layer) const noexcept = 0;
	virtual std::string levelToString(Level const level) const noexcept = 0;

	// Deleted compiler auto-generated methods
	Logger(Logger&&) = delete;
	Logger(Logger const&) = delete;
	Logger& operator=(Logger const&) = delete;
	Logger& operator=(Logger&&) = delete;

protected:
	Logger() noexcept = default;
	virtual ~Logger() noexcept = default;
};

} // namespace logger
} // namespace avdecc
} // namespace la
