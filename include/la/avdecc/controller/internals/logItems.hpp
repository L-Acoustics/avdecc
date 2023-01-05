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
* @file logItems.hpp
* @author Christophe Calmejane
* @brief Avdecc controller library LogItem declarations for the simple logger.
*/

#pragma once

#include <la/avdecc/logger.hpp>
#include <la/avdecc/utils.hpp>
#include <la/avdecc/internals/uniqueIdentifier.hpp>

namespace la
{
namespace avdecc
{
namespace logger
{
class LogItemController : public la::avdecc::logger::LogItem
{
public:
	LogItemController(la::avdecc::UniqueIdentifier const& targetID, std::string message)
		: LogItem(Layer::Controller)
		, _targetID(targetID)
		, _message(message)
	{
	}

	virtual std::string getMessage() const noexcept override
	{
		return std::string("[") + utils::toHexString(_targetID, true, false) + "] " + _message;
	}

	la::avdecc::UniqueIdentifier const& getTargetID() const noexcept
	{
		return _targetID;
	}

private:
	la::avdecc::UniqueIdentifier const& _targetID;
	std::string _message{};
};

} // namespace logger
} // namespace avdecc
} // namespace la
