/*
* Copyright (C) 2016-2024, L-Acoustics and its contributors

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
* @file entity_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/internals/entity.hpp>

#include <gtest/gtest.h>

TEST(Entity, GenerateEID)
{
	constexpr auto MacAddress = la::networkInterface::MacAddress{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };

	// Using deprecated algorithm
	{
		auto eid = la::avdecc::entity::Entity::generateEID(MacAddress, 0x7788, true);
		EXPECT_EQ(0x0102037788040506u, eid.getValue());
	}

	// Using recommended algorithm
	{
		auto eid = la::avdecc::entity::Entity::generateEID(MacAddress, 0x7788, false);
		EXPECT_EQ(0x0102030405067788u, eid.getValue());
	}
}
