/*
* Copyright (C) 2016-2020, L-Acoustics and its contributors

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
* @file logHelper.hpp
* @author Christophe Calmejane
* @brief Helper templates and defines for the simple logger.
*/

#pragma once

#include "la/avdecc/internals/logItems.hpp"

#ifdef HAVE_FMT
#	ifdef _WIN32
#		pragma warning(push)
#		pragma warning(disable : 4702)
#		ifdef __clang__
#			pragma clang diagnostic push
#			pragma clang diagnostic ignored "-Wdeprecated-declarations"
#		endif // __clang__
#	endif // _WIN32
#	include <fmt/format.h>
#	ifdef _WIN32
#		ifdef __clang__
#			pragma clang diagnostic pop
#		endif // __clang__
#		pragma warning(pop)
#	endif // _WIN32
#	define FORMAT_ARGS(...) fmt::format(__VA_ARGS__)
#else // !HAVE_FMT
#	include <string>
#	define FORMAT_ARGS(...) la::avdecc::logger::format(__VA_ARGS__)
#endif // HAVE_FMT

namespace la
{
namespace avdecc
{
namespace logger
{
/** Template to format args if not using lib fmt */
template<typename... Ts>
inline std::string format(std::string&& message, Ts&&... /*params*/)
{
	// Right now, not formatting anything, just returning the message with untouched format specifiers - Please enable libfmt!
	return std::move(message);
}

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
#define LOG_GENERIC(LogLevel, Message) la::avdecc::logger::log<la::avdecc::logger::Level::LogLevel, la::avdecc::logger::LogItemGeneric>(Message)
#ifdef DEBUG
#	define LOG_GENERIC_TRACE(Message) LOG_GENERIC(Trace, Message)
#	define LOG_GENERIC_DEBUG(Message) LOG_GENERIC(Debug, Message)
#else // !DEBUG
#	define LOG_GENERIC_TRACE(Message)
#	define LOG_GENERIC_DEBUG(Message)
#endif // DEBUG
#define LOG_GENERIC_INFO(Message) LOG_GENERIC(Info, Message)
#define LOG_GENERIC_WARN(Message) LOG_GENERIC(Warn, Message)
#define LOG_GENERIC_ERROR(Message) LOG_GENERIC(Error, Message)

#define LOG_SERIALIZATION(LogLevel, Source, ...) la::avdecc::logger::log<la::avdecc::logger::Level::LogLevel, la::avdecc::logger::LogItemSerialization>(Source, FORMAT_ARGS(__VA_ARGS__))
#ifdef DEBUG
#	define LOG_SERIALIZATION_TRACE(Source, ...) LOG_SERIALIZATION(Trace, Source, __VA_ARGS__)
#	define LOG_SERIALIZATION_DEBUG(Source, ...) LOG_SERIALIZATION(Debug, Source, __VA_ARGS__)
#else // !DEBUG
#	define LOG_SERIALIZATION_TRACE(Source, ...)
#	define LOG_SERIALIZATION_DEBUG(Source, ...)
#endif // DEBUG
#define LOG_SERIALIZATION_INFO(Source, ...) LOG_SERIALIZATION(Info, Source, __VA_ARGS__)
#define LOG_SERIALIZATION_WARN(Source, ...) LOG_SERIALIZATION(Warn, Source, __VA_ARGS__)
#define LOG_SERIALIZATION_ERROR(Source, ...) LOG_SERIALIZATION(Error, Source, __VA_ARGS__)

#define LOG_PROTOCOL_INTERFACE(LogLevel, Source, Dest, ...) la::avdecc::logger::log<la::avdecc::logger::Level::LogLevel, la::avdecc::logger::LogItemProtocolInterface>(Source, Dest, FORMAT_ARGS(__VA_ARGS__))
#ifdef DEBUG
#	define LOG_PROTOCOL_INTERFACE_TRACE(Source, Dest, ...) LOG_PROTOCOL_INTERFACE(Trace, Source, Dest, __VA_ARGS__)
#	define LOG_PROTOCOL_INTERFACE_DEBUG(Source, Dest, ...) LOG_PROTOCOL_INTERFACE(Debug, Source, Dest, __VA_ARGS__)
#else // !DEBUG
#	define LOG_PROTOCOL_INTERFACE_TRACE(Source, Dest, ...)
#	define LOG_PROTOCOL_INTERFACE_DEBUG(Source, Dest, ...)
#endif // DEBUG
#define LOG_PROTOCOL_INTERFACE_WARN(Source, Dest, ...) LOG_PROTOCOL_INTERFACE(Warn, Source, Dest, __VA_ARGS__)
#define LOG_PROTOCOL_INTERFACE_ERROR(Source, Dest, ...) LOG_PROTOCOL_INTERFACE(Error, Source, Dest, __VA_ARGS__)

#define LOG_AEM_PAYLOAD(LogLevel, ...) la::avdecc::logger::log<la::avdecc::logger::Level::LogLevel, la::avdecc::logger::LogItemAemPayload>(FORMAT_ARGS(__VA_ARGS__))
#ifdef DEBUG
#	define LOG_AEM_PAYLOAD_TRACE(...) LOG_AEM_PAYLOAD(Trace, __VA_ARGS__)
#	define LOG_AEM_PAYLOAD_DEBUG(...) LOG_AEM_PAYLOAD(Debug, __VA_ARGS__)
#else // !DEBUG
#	define LOG_AEM_PAYLOAD_TRACE(...)
#	define LOG_AEM_PAYLOAD_DEBUG(...)
#endif // DEBUG
#define LOG_AEM_PAYLOAD_WARN(...) LOG_AEM_PAYLOAD(Warn, __VA_ARGS__)
#define LOG_AEM_PAYLOAD_ERROR(...) LOG_AEM_PAYLOAD(Error, __VA_ARGS__)

#define LOG_ENTITY(LogLevel, TargetID, ...) la::avdecc::logger::log<la::avdecc::logger::Level::LogLevel, la::avdecc::logger::LogItemEntity>(TargetID, FORMAT_ARGS(__VA_ARGS__))
#ifdef DEBUG
#	define LOG_ENTITY_TRACE(TargetID, ...) LOG_ENTITY(Trace, TargetID, __VA_ARGS__)
#	define LOG_ENTITY_DEBUG(TargetID, ...) LOG_ENTITY(Debug, TargetID, __VA_ARGS__)
#else // !DEBUG
#	define LOG_ENTITY_TRACE(TargetID, ...)
#	define LOG_ENTITY_DEBUG(TargetID, ...)
#endif // DEBUG
#define LOG_ENTITY_INFO(TargetID, ...) LOG_ENTITY(Info, TargetID, __VA_ARGS__)
#define LOG_ENTITY_WARN(TargetID, ...) LOG_ENTITY(Warn, TargetID, __VA_ARGS__)
#define LOG_ENTITY_ERROR(TargetID, ...) LOG_ENTITY(Error, TargetID, __VA_ARGS__)

#define LOG_CONTROLLER_ENTITY(LogLevel, TargetID, ...) la::avdecc::logger::log<la::avdecc::logger::Level::LogLevel, la::avdecc::logger::LogItemControllerEntity>(TargetID, FORMAT_ARGS(__VA_ARGS__))
#ifdef DEBUG
#	define LOG_CONTROLLER_ENTITY_TRACE(TargetID, ...) LOG_CONTROLLER_ENTITY(Trace, TargetID, __VA_ARGS__)
#	define LOG_CONTROLLER_ENTITY_DEBUG(TargetID, ...) LOG_CONTROLLER_ENTITY(Debug, TargetID, __VA_ARGS__)
#else // !DEBUG
#	define LOG_CONTROLLER_ENTITY_TRACE(TargetID, ...)
#	define LOG_CONTROLLER_ENTITY_DEBUG(TargetID, ...)
#endif // DEBUG
#define LOG_CONTROLLER_ENTITY_INFO(TargetID, ...) LOG_CONTROLLER_ENTITY(Info, TargetID, __VA_ARGS__)
#define LOG_CONTROLLER_ENTITY_WARN(TargetID, ...) LOG_CONTROLLER_ENTITY(Warn, TargetID, __VA_ARGS__)
#define LOG_CONTROLLER_ENTITY_ERROR(TargetID, ...) LOG_CONTROLLER_ENTITY(Error, TargetID, __VA_ARGS__)

#define LOG_CONTROLLER_STATE_MACHINE(LogLevel, TargetID, ...) la::avdecc::logger::log<la::avdecc::logger::Level::LogLevel, la::avdecc::logger::LogItemControllerStateMachine>(TargetID, FORMAT_ARGS(__VA_ARGS__))
#ifdef DEBUG
#	define LOG_CONTROLLER_STATE_MACHINE_TRACE(TargetID, ...) LOG_CONTROLLER_STATE_MACHINE(Trace, TargetID, __VA_ARGS__)
#	define LOG_CONTROLLER_STATE_MACHINE_DEBUG(TargetID, ...) LOG_CONTROLLER_STATE_MACHINE(Debug, TargetID, __VA_ARGS__)
#else // !DEBUG
#	define LOG_CONTROLLER_STATE_MACHINE_TRACE(TargetID, ...)
#	define LOG_CONTROLLER_STATE_MACHINE_DEBUG(TargetID, ...)
#endif // DEBUG
#define LOG_CONTROLLER_STATE_MACHINE_INFO(TargetID, ...) LOG_CONTROLLER_STATE_MACHINE(Info, TargetID, __VA_ARGS__)
#define LOG_CONTROLLER_STATE_MACHINE_WARN(TargetID, ...) LOG_CONTROLLER_STATE_MACHINE(Warn, TargetID, __VA_ARGS__)
#define LOG_CONTROLLER_STATE_MACHINE_ERROR(TargetID, ...) LOG_CONTROLLER_STATE_MACHINE(Error, TargetID, __VA_ARGS__)

#define LOG_ENDPOINT_ENTITY(LogLevel, TargetID, ...) la::avdecc::logger::log<la::avdecc::logger::Level::LogLevel, la::avdecc::logger::LogItemEndpointEntity>(TargetID, FORMAT_ARGS(__VA_ARGS__))
#ifdef DEBUG
#	define LOG_ENDPOINT_ENTITY_TRACE(TargetID, ...) LOG_ENDPOINT_ENTITY(Trace, TargetID, __VA_ARGS__)
#	define LOG_ENDPOINT_ENTITY_DEBUG(TargetID, ...) LOG_ENDPOINT_ENTITY(Debug, TargetID, __VA_ARGS__)
#else // !DEBUG
#	define LOG_ENDPOINT_ENTITY_TRACE(TargetID, ...)
#	define LOG_ENDPOINT_ENTITY_DEBUG(TargetID, ...)
#endif // DEBUG
#define LOG_ENDPOINT_ENTITY_INFO(TargetID, ...) LOG_ENDPOINT_ENTITY(Info, TargetID, __VA_ARGS__)
#define LOG_ENDPOINT_ENTITY_WARN(TargetID, ...) LOG_ENDPOINT_ENTITY(Warn, TargetID, __VA_ARGS__)
#define LOG_ENDPOINT_ENTITY_ERROR(TargetID, ...) LOG_ENDPOINT_ENTITY(Error, TargetID, __VA_ARGS__)
