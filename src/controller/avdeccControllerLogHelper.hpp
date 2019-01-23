/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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
* @file avdeccControllerLogHelper.hpp
* @author Christophe Calmejane
* @brief Helper templates and defines for the simple logger.
*/

#pragma once

#include "la/avdecc/controller/internals/logItems.hpp"
#ifdef HAVE_FMT
#	ifdef _WIN32
#		pragma warning(push)
#		pragma warning(disable : 4702)
#	endif // _WIN32
#	include <fmt/format.h>
#	ifdef _WIN32
#		pragma warning(pop)
#	endif // _WIN32
#	define FORMAT_ARGS(...) fmt::format(__VA_ARGS__)
#else // !HAVE_FMT
#	include <string>
#	define FORMAT_ARGS(...) ""
#endif // HAVE_FMT

namespace la
{
namespace avdecc
{
namespace logger
{
/** Template to remove at compile time some of the most time-consuming log messages (Trace and Debug) - Forward arguments to the Logger */
template<Level LevelValue, class LogItemType, typename... Ts>
constexpr void log(Ts&&... params)
{
#ifndef DEBUG
	// In release, we don't want Trace nor Debug levels
	if constexpr (LevelValue == Level::Trace || LevelValue == Level::Debug)
	{
	}
	else
#endif // !DEBUG
	{
		auto const item = LogItemType{ std::forward<Ts>(params)... };
		Logger::getInstance().logItem(LevelValue, &item);
	}
}

} // namespace logger
} // namespace avdecc
} // namespace la

/** Preprocessor defines to remove at compile time some of the most time-consuming log messages (Trace and Debug) - Creation of the arguments */
#define LOG_CONTROLLER(LogLevel, TargetID, ...) la::avdecc::logger::log<la::avdecc::logger::Level::LogLevel, la::avdecc::logger::LogItemController>(TargetID, FORMAT_ARGS(__VA_ARGS__))
#ifdef DEBUG
#	define LOG_CONTROLLER_TRACE(TargetID, ...) LOG_CONTROLLER(Trace, TargetID, __VA_ARGS__)
#	define LOG_CONTROLLER_DEBUG(TargetID, ...) LOG_CONTROLLER(Debug, TargetID, __VA_ARGS__)
#else // !DEBUG
#	define LOG_CONTROLLER_TRACE(TargetID, ...)
#	define LOG_CONTROLLER_DEBUG(TargetID, ...)
#endif // DEBUG
#define LOG_CONTROLLER_INFO(TargetID, ...) LOG_CONTROLLER(Info, TargetID, __VA_ARGS__)
#define LOG_CONTROLLER_WARN(TargetID, ...) LOG_CONTROLLER(Warn, TargetID, __VA_ARGS__)
#define LOG_CONTROLLER_ERROR(TargetID, ...) LOG_CONTROLLER(Error, TargetID, __VA_ARGS__)
