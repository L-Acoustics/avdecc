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
* @file logItems.hpp
* @author Christophe Calmejane
* @brief Avdecc library LogItem declarations for the simple logger.
*/

#pragma once

#include "la/avdecc/logger.hpp"
#include "la/avdecc/utils.hpp"
#include "la/avdecc/networkInterfaceHelper.hpp"

#include "uniqueIdentifier.hpp"

namespace la
{
namespace avdecc
{
namespace logger
{
class LogItemGeneric : public la::avdecc::logger::LogItem
{
public:
	LogItemGeneric(std::string message) noexcept
		: LogItem(Layer::Generic)
		, _message(message)
	{
	}

	virtual std::string getMessage() const noexcept override
	{
		return _message;
	}

private:
	std::string _message{};
};

class LogItemSerialization : public la::avdecc::logger::LogItem
{
public:
	LogItemSerialization(la::avdecc::networkInterface::MacAddress const& source, std::string message) noexcept
		: LogItem(Layer::Serialization)
		, _source(source)
		, _message(message)
	{
	}

	virtual std::string getMessage() const noexcept override
	{
		return std::string("[") + la::avdecc::networkInterface::macAddressToString(_source, true) + "] " + _message;
	}

	la::avdecc::networkInterface::MacAddress const& getSource() const noexcept
	{
		return _source;
	}

private:
	la::avdecc::networkInterface::MacAddress const& _source;
	std::string _message{};
};

class LogItemProtocolInterface : public la::avdecc::logger::LogItem
{
public:
	LogItemProtocolInterface(la::avdecc::networkInterface::MacAddress const& source, la::avdecc::networkInterface::MacAddress const& dest, std::string message) noexcept
		: LogItem(Layer::ProtocolInterface)
		, _source(source)
		, _dest(dest)
		, _message(message)
	{
	}

	virtual std::string getMessage() const noexcept override
	{
		return std::string("[") + la::avdecc::networkInterface::macAddressToString(_source, true) + " -> " + la::avdecc::networkInterface::macAddressToString(_dest, true) + "] " + _message;
	}

	la::avdecc::networkInterface::MacAddress const& getSource() const noexcept
	{
		return _source;
	}

	la::avdecc::networkInterface::MacAddress const& getDest() const noexcept
	{
		return _dest;
	}

private:
	la::avdecc::networkInterface::MacAddress const& _source;
	la::avdecc::networkInterface::MacAddress const& _dest;
	std::string _message{};
};

class LogItemAemPayload : public la::avdecc::logger::LogItem
{
public:
	LogItemAemPayload(std::string message) noexcept
		: LogItem(Layer::AemPayload)
		, _message(message)
	{
	}

	virtual std::string getMessage() const noexcept override
	{
		return _message;
	}

private:
	std::string _message{};
};

class LogItemEntity : public la::avdecc::logger::LogItem
{
public:
	LogItemEntity(la::avdecc::UniqueIdentifier const& targetID, std::string message) noexcept
		: LogItem(Layer::Entity)
		, _targetID(targetID)
		, _message(message)
	{
	}

	virtual std::string getMessage() const noexcept override
	{
		return std::string("[") + la::avdecc::utils::toHexString(_targetID, true, false) + "] " + _message;
	}

	la::avdecc::UniqueIdentifier const& getTargetID() const noexcept
	{
		return _targetID;
	}

private:
	la::avdecc::UniqueIdentifier const& _targetID;
	std::string _message{};
};

class LogItemControllerEntity : public la::avdecc::logger::LogItem
{
public:
	LogItemControllerEntity(la::avdecc::UniqueIdentifier const& targetID, std::string message) noexcept
		: LogItem(Layer::ControllerEntity)
		, _targetID(targetID)
		, _message(message)
	{
	}

	virtual std::string getMessage() const noexcept override
	{
		return std::string("[") + la::avdecc::utils::toHexString(_targetID, true, false) + "] " + _message;
	}

	la::avdecc::UniqueIdentifier const& getTargetID() const noexcept
	{
		return _targetID;
	}

private:
	la::avdecc::UniqueIdentifier const& _targetID;
	std::string _message{};
};

class LogItemEndpointEntity : public la::avdecc::logger::LogItem
{
public:
	LogItemEndpointEntity(la::avdecc::UniqueIdentifier const& targetID, std::string message) noexcept
		: LogItem(Layer::EndpointEntity)
		, _targetID(targetID)
		, _message(message)
	{
	}

	virtual std::string getMessage() const noexcept override
	{
		return std::string("[") + la::avdecc::utils::toHexString(_targetID, true, false) + "] " + _message;
	}

	la::avdecc::UniqueIdentifier const& getTargetID() const noexcept
	{
		return _targetID;
	}

private:
	la::avdecc::UniqueIdentifier const& _targetID;
	std::string _message{};
};

class LogItemControllerStateMachine : public la::avdecc::logger::LogItem
{
public:
	LogItemControllerStateMachine(la::avdecc::UniqueIdentifier const& targetID, std::string message) noexcept
		: LogItem(Layer::ControllerStateMachine)
		, _targetID(targetID)
		, _message(message)
	{
	}

	virtual std::string getMessage() const noexcept override
	{
		return std::string("[") + la::avdecc::utils::toHexString(_targetID, true, false) + "] " + _message;
	}

	la::avdecc::UniqueIdentifier const& getTargetID() const noexcept
	{
		return _targetID;
	}

private:
	la::avdecc::UniqueIdentifier const& _targetID;
	std::string _message{};
};

class LogItemJsonSerializer : public la::avdecc::logger::LogItem
{
public:
	LogItemJsonSerializer(std::string const& message) noexcept
		: LogItem(Layer::JsonSerializer)
		, _message(message)
	{
	}

	virtual std::string getMessage() const noexcept override
	{
		return _message;
	}

private:
	std::string _message{};
};

} // namespace logger
} // namespace avdecc
} // namespace la
