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

// Public API
#include <la/avdecc/logger.hpp>
#include <la/avdecc/internals/logItems.hpp>

// Internal API
#include "logHelper.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include <typeinfo>

namespace
{

class Observer : public la::avdecc::logger::Logger::Observer
{
private:
	virtual void onLogItem(la::avdecc::logger::Level const level, la::avdecc::logger::LogItem const* const item) noexcept
	{
		if (item->getLayer() == la::avdecc::logger::Layer::Serialization)
		{
			auto const* const i = static_cast<la::avdecc::logger::LogItemSerialization const*>(item);
			std::cout << "[" << la::avdecc::logger::Logger::getInstance().levelToString(level) << "] [" << la::avdecc::networkInterface::macAddressToString(i->getSource(), true) << "] " << i->getMessage() << std::endl;
		}
		else
		{
			std::cout << "[" << la::avdecc::logger::Logger::getInstance().levelToString(level) << "] " << item->getMessage() << std::endl;
		}
	}
};

}

TEST(Logger, Log)
{
	Observer obs;

	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Info);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	la::avdecc::logger::log<la::avdecc::logger::Level::Info, la::avdecc::logger::LogItemSerialization>(la::avdecc::networkInterface::MacAddress{}, "Info message");
	la::avdecc::logger::log<la::avdecc::logger::Level::Warn, la::avdecc::logger::LogItemSerialization>(la::avdecc::networkInterface::MacAddress{}, "Warn message");
	la::avdecc::logger::log<la::avdecc::logger::Level::Error, la::avdecc::logger::LogItemSerialization>(la::avdecc::networkInterface::MacAddress{}, "Error message");

	LOG_SERIALIZATION(Info, la::avdecc::networkInterface::MacAddress{}, "Test");
	LOG_SERIALIZATION_DEBUG(la::avdecc::networkInterface::MacAddress{}, "Test");

	// TODO: Proper unit test, this code was only written as development code
}
