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
* @file aatlv_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/internals/entityAddressAccessTypes.hpp>

#include <gtest/gtest.h>

TEST(AddressAccessTlv, Contructor)
{
	// Default constructor
	{
		auto const tlv = la::avdecc::entity::addressAccess::Tlv{};
		EXPECT_FALSE(tlv) << "Default TLV should not be valid";
		EXPECT_FALSE(tlv.isValid()) << "Default TLV should not be valid";
	}

	// Constructor for a Read command with address=0
	{
		std::uint64_t const address{ 0u };
		size_t const length{ 15 };
		auto const tlv = la::avdecc::entity::addressAccess::Tlv{ address, length };
		EXPECT_TRUE(tlv);
		EXPECT_TRUE(tlv.isValid());
		EXPECT_EQ(la::avdecc::protocol::AaMode::Read, tlv.getMode());
		EXPECT_EQ(address, tlv.getAddress());
		EXPECT_EQ(length, tlv.size());
		EXPECT_EQ(length, tlv.getMemoryData().size());
	}

	// Constructor for a Read command
	{
		std::uint64_t const address{ 0x123456789ABCDEF };
		size_t const length{ 15 };
		auto const tlv = la::avdecc::entity::addressAccess::Tlv{ address, length };
		EXPECT_TRUE(tlv);
		EXPECT_TRUE(tlv.isValid());
		EXPECT_EQ(la::avdecc::protocol::AaMode::Read, tlv.getMode());
		EXPECT_EQ(address, tlv.getAddress());
		EXPECT_EQ(length, tlv.size());
		EXPECT_EQ(length, tlv.getMemoryData().size());
	}

	// Constructor for a Write command using raw buffer
	{
		std::uint64_t const address{ 0x123456789ABCDEF };
		char buf[15] = { '\1' };
		auto const tlv = la::avdecc::entity::addressAccess::Tlv{ address, la::avdecc::protocol::AaMode::Write, &buf, sizeof(buf) };
		EXPECT_TRUE(tlv);
		EXPECT_TRUE(tlv.isValid());
		EXPECT_EQ(la::avdecc::protocol::AaMode::Write, tlv.getMode());
		EXPECT_EQ(address, tlv.getAddress());
		EXPECT_EQ(sizeof(buf), tlv.size());
		EXPECT_EQ(sizeof(buf), tlv.getMemoryData().size());
		EXPECT_EQ(buf[0], static_cast<char const*>(tlv.data())[0]);
		EXPECT_EQ(buf[0], tlv.getMemoryData().data()[0]);
	}

	// Constructor for a Write command using MemoryData
	{
		std::uint64_t const address{ 0x123456789ABCDEF };
		auto const buffer = la::avdecc::entity::addressAccess::Tlv::memory_data_type{ 5 };
		auto const tlv = la::avdecc::entity::addressAccess::Tlv{ address, la::avdecc::protocol::AaMode::Write, buffer };
		EXPECT_TRUE(tlv);
		EXPECT_TRUE(tlv.isValid());
		EXPECT_EQ(la::avdecc::protocol::AaMode::Write, tlv.getMode());
		EXPECT_EQ(address, tlv.getAddress());
		EXPECT_EQ(buffer.size(), tlv.size());
		EXPECT_EQ(buffer.size(), tlv.getMemoryData().size());
		EXPECT_EQ(buffer[0], static_cast<char const*>(tlv.data())[0]);
		EXPECT_EQ(buffer[0], tlv.getMemoryData().data()[0]);
	}

	// Constructor for an Execute command using raw buffer
	{
		std::uint64_t const address{ 0x123456789ABCDEF };
		char buf[15] = { '\1' };
		auto const tlv = la::avdecc::entity::addressAccess::Tlv{ address, la::avdecc::protocol::AaMode::Execute, &buf, sizeof(buf) };
		EXPECT_TRUE(tlv);
		EXPECT_TRUE(tlv.isValid());
		EXPECT_EQ(la::avdecc::protocol::AaMode::Execute, tlv.getMode());
		EXPECT_EQ(address, tlv.getAddress());
		EXPECT_EQ(sizeof(buf), tlv.size());
		EXPECT_EQ(sizeof(buf), tlv.getMemoryData().size());
		EXPECT_EQ(buf[0], static_cast<char const*>(tlv.data())[0]);
		EXPECT_EQ(buf[0], tlv.getMemoryData().data()[0]);
	}

	// Constructor for a Execute command using MemoryData
	{
		std::uint64_t const address{ 0x123456789ABCDEF };
		auto const buffer = la::avdecc::entity::addressAccess::Tlv::memory_data_type{ 5 };
		auto const tlv = la::avdecc::entity::addressAccess::Tlv{ address, la::avdecc::protocol::AaMode::Execute, buffer };
		EXPECT_TRUE(tlv);
		EXPECT_TRUE(tlv.isValid());
		EXPECT_EQ(la::avdecc::protocol::AaMode::Execute, tlv.getMode());
		EXPECT_EQ(address, tlv.getAddress());
		EXPECT_EQ(buffer.size(), tlv.size());
		EXPECT_EQ(buffer.size(), tlv.getMemoryData().size());
		EXPECT_EQ(buffer[0], static_cast<char const*>(tlv.data())[0]);
		EXPECT_EQ(buffer[0], tlv.getMemoryData().data()[0]);
	}

}
