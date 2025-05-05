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
* @file entityModel_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/utils.hpp>
#include <la/avdecc/internals/entityModelTreeDynamic.hpp>

#include <gtest/gtest.h>

TEST(EntityModel, GetStreamOutputCounters)
{
	// Default constructor
	{
		auto const streamOutputCounters = la::avdecc::entity::model::StreamOutputCounters{};
		EXPECT_EQ(la::avdecc::entity::model::StreamOutputCounters::CounterType::Unknown, streamOutputCounters.getCounterType()) << "CounterType should be Unknown";
	}
	// Unknown type
	{
		auto const counters = la::avdecc::entity::model::DescriptorCounters{};
		auto const streamOutputCounters = la::avdecc::entity::model::StreamOutputCounters{ la::avdecc::entity::model::StreamOutputCounters::CounterType::Unknown, la::avdecc::entity::model::DescriptorCounterValidFlag{ 0u }, counters };
		EXPECT_EQ(la::avdecc::entity::model::StreamOutputCounters::CounterType::Unknown, streamOutputCounters.getCounterType()) << "CounterType should be Unknown";
		// Getting counters for other type should throw
		EXPECT_THROW(streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlagsMilan12>(), std::invalid_argument);
		EXPECT_THROW(streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlags17221>(), std::invalid_argument);
	}
	// Milan 1.2
	{
		auto const counters = la::avdecc::entity::model::DescriptorCounters{};
		auto const streamOutputCounters = la::avdecc::entity::model::StreamOutputCounters{ la::avdecc::entity::model::StreamOutputCounters::CounterType::Milan_12, la::avdecc::entity::model::DescriptorCounterValidFlag{ 0u }, counters };
		EXPECT_EQ(la::avdecc::entity::model::StreamOutputCounters::CounterType::Milan_12, streamOutputCounters.getCounterType()) << "CounterType should be Milan 1.2";
		// Getting counters for other type should throw
		EXPECT_THROW(streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlags17221>(), std::invalid_argument);
	}
	// IEEE1722.1-2021
	{
		auto const counters = la::avdecc::entity::model::DescriptorCounters{};
		auto const streamOutputCounters = la::avdecc::entity::model::StreamOutputCounters{ la::avdecc::entity::model::StreamOutputCounters::CounterType::IEEE17221_2021, la::avdecc::entity::model::DescriptorCounterValidFlag{ 0u }, counters };
		EXPECT_EQ(la::avdecc::entity::model::StreamOutputCounters::CounterType::IEEE17221_2021, streamOutputCounters.getCounterType()) << "CounterType should be IEEE1722.1-2021";
		// Getting counters for other type should throw
		EXPECT_THROW(streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlagsMilan12>(), std::invalid_argument);
	}
	// Milan 1.2 MediaReset
	{
		// MediaReset is bit 2 for Milan 1.2 (ie. 0x00000004) at index 2
		auto counters = la::avdecc::entity::model::DescriptorCounters{};
		counters[2] = la::avdecc::entity::model::DescriptorCounter{ 42u };
		auto const streamOutputCounters = la::avdecc::entity::model::StreamOutputCounters{ la::avdecc::entity::model::StreamOutputCounters::CounterType::Milan_12, la::avdecc::entity::model::DescriptorCounterValidFlag{ 0x00000004 }, counters };
		EXPECT_EQ(la::avdecc::entity::model::StreamOutputCounters::CounterType::Milan_12, streamOutputCounters.getCounterType()) << "CounterType should be Milan 1.2";
		auto const milan12Counters = streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlagsMilan12>();
		EXPECT_EQ(1u, milan12Counters.size()) << "Should have 1 counter";
		EXPECT_EQ(42u, milan12Counters.at(la::avdecc::entity::StreamOutputCounterValidFlagMilan12::MediaReset)) << "Counter value should be 42";
		// Getting counters for other type should throw
		EXPECT_THROW(streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlags17221>(), std::invalid_argument);
	}
	// Interpret Milan 1.2 MediaReset as IEEE1722.1-2021
	{
		// MediaReset is bit 2 for Milan 1.2 (ie. 0x00000004) at index 2
		auto counters = la::avdecc::entity::model::DescriptorCounters{};
		counters[2] = la::avdecc::entity::model::DescriptorCounter{ 42u };
		// TimestampUncertain is bit 3 for Milan 1.2 (ie. 0x00000008) at index 3
		counters[3] = la::avdecc::entity::model::DescriptorCounter{ 24u };
		auto const streamOutputCounters = la::avdecc::entity::model::StreamOutputCounters{ la::avdecc::entity::model::StreamOutputCounters::CounterType::Milan_12, la::avdecc::entity::model::DescriptorCounterValidFlag{ 0x00000004 + 0x00000008 }, counters };
		EXPECT_EQ(la::avdecc::entity::model::StreamOutputCounters::CounterType::Milan_12, streamOutputCounters.getCounterType()) << "CounterType should be Milan 1.2";
		auto const ieee17221Counters = streamOutputCounters.convertCounters<la::avdecc::entity::StreamOutputCounterValidFlags17221>();
		EXPECT_EQ(2u, ieee17221Counters.size()) << "Should have 2 counters";
		// MediaReset is bit 3 for IEEE1722.1-2021 (ie. 0x00000008) at index 3, where TimestampUncertain is for Milan 1.2
		EXPECT_EQ(24u, ieee17221Counters.at(la::avdecc::entity::StreamOutputCounterValidFlag17221::MediaReset)) << "Counter value should be 24 (the value of TimestampUncertain for Milan 1.2)";
		// StreamInterrupted is bit 2 for IEEE1722.1-2021 (ie. 0x00000004) at index 2, where MediaReset is for Milan 1.2
		EXPECT_EQ(42u, ieee17221Counters.at(la::avdecc::entity::StreamOutputCounterValidFlag17221::StreamInterrupted)) << "Counter value should be 42 (the value of MediaReset for Milan 1.2)";
	}
	// Milan 1.2 Undefined value
	{
		// Bit 5 for Milan 1.2 is not used (ie. 0x00000020) at index 5
		auto counters = la::avdecc::entity::model::DescriptorCounters{};
		counters[5] = la::avdecc::entity::model::DescriptorCounter{ 42u };
		auto const streamOutputCounters = la::avdecc::entity::model::StreamOutputCounters{ la::avdecc::entity::model::StreamOutputCounters::CounterType::Milan_12, la::avdecc::entity::model::DescriptorCounterValidFlag{ 0x00000020 }, counters };
		EXPECT_EQ(la::avdecc::entity::model::StreamOutputCounters::CounterType::Milan_12, streamOutputCounters.getCounterType()) << "CounterType should be Milan 1.2";
		auto const milan12Counters = streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlagsMilan12>();
		EXPECT_EQ(1u, milan12Counters.size()) << "Should have 1 counter";
		EXPECT_EQ(0x00000020, la::avdecc::utils::to_integral(milan12Counters.begin()->first)) << "Counter bit should be 0x00000020";
		EXPECT_EQ(42u, milan12Counters.begin()->second) << "Counter value should be 42";
		// Getting counters for other type should throw
		EXPECT_THROW(streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlags17221>(), std::invalid_argument);
	}
}

TEST(EntityModel, SetStreamOutputCounters)
{
	// Milan 1.2
	{
		auto const milanCounters = std::map<la::avdecc::entity::StreamOutputCounterValidFlagMilan12, la::avdecc::entity::model::DescriptorCounter>{ { la::avdecc::entity::StreamOutputCounterValidFlagMilan12::MediaReset, 42u } };
		auto streamOutputCounters = la::avdecc::entity::model::StreamOutputCounters{};
		streamOutputCounters.setCounters(milanCounters);
		EXPECT_EQ(la::avdecc::entity::model::StreamOutputCounters::CounterType::Milan_12, streamOutputCounters.getCounterType()) << "CounterType should be Milan 1.2";
		auto const milan12Counters = streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlagsMilan12>();
		EXPECT_EQ(1u, milan12Counters.size()) << "Should have 1 counter";
		EXPECT_EQ(42u, milan12Counters.at(la::avdecc::entity::StreamOutputCounterValidFlagMilan12::MediaReset)) << "Counter value should be 42";
		EXPECT_EQ(milanCounters, milan12Counters) << "Counters should be equal";
		// Getting counters for other type should throw
		EXPECT_THROW(streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlags17221>(), std::invalid_argument);
	}
	// Invalid counter key
	{
		auto const milanCounters = std::map<la::avdecc::entity::StreamOutputCounterValidFlagMilan12, la::avdecc::entity::model::DescriptorCounter>{ { static_cast<la::avdecc::entity::StreamOutputCounterValidFlagMilan12>(3u), 42u } }; // '3' is not a valid flag (more than one bit set)
		auto streamOutputCounters = la::avdecc::entity::model::StreamOutputCounters{};
		streamOutputCounters.setCounters(milanCounters);
		EXPECT_EQ(la::avdecc::entity::model::StreamOutputCounters::CounterType::Milan_12, streamOutputCounters.getCounterType()) << "CounterType should be Milan 1.2";
		auto const milan12Counters = streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlagsMilan12>();
		EXPECT_EQ(0u, milan12Counters.size()) << "Should have 0 counter (invalid flag)";
		// Getting counters for other type should throw
		EXPECT_THROW(streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlags17221>(), std::invalid_argument);
	}
	// 'None' counter key
	{
		auto const milanCounters = std::map<la::avdecc::entity::StreamOutputCounterValidFlagMilan12, la::avdecc::entity::model::DescriptorCounter>{ { la::avdecc::entity::StreamOutputCounterValidFlagMilan12::None, 42u } };
		auto streamOutputCounters = la::avdecc::entity::model::StreamOutputCounters{};
		streamOutputCounters.setCounters(milanCounters);
		EXPECT_EQ(la::avdecc::entity::model::StreamOutputCounters::CounterType::Milan_12, streamOutputCounters.getCounterType()) << "CounterType should be Milan 1.2";
		auto const milan12Counters = streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlagsMilan12>();
		EXPECT_EQ(0u, milan12Counters.size()) << "Should have 0 counter (invalid flag, 'None' has no bit set)";
		// Getting counters for other type should throw
		EXPECT_THROW(streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlags17221>(), std::invalid_argument);
	}
	// Unknown counter key
	{
		auto const milanCounters = std::map<la::avdecc::entity::StreamOutputCounterValidFlagMilan12, la::avdecc::entity::model::DescriptorCounter>{ { static_cast<la::avdecc::entity::StreamOutputCounterValidFlagMilan12>(32u), 42u } };
		auto streamOutputCounters = la::avdecc::entity::model::StreamOutputCounters{};
		streamOutputCounters.setCounters(milanCounters);
		EXPECT_EQ(la::avdecc::entity::model::StreamOutputCounters::CounterType::Milan_12, streamOutputCounters.getCounterType()) << "CounterType should be Milan 1.2";
		auto const milan12Counters = streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlagsMilan12>();
		EXPECT_EQ(1u, milan12Counters.size()) << "Should have 1 counter";
		EXPECT_EQ(42u, milan12Counters.at(static_cast<la::avdecc::entity::StreamOutputCounterValidFlagMilan12>(32u))) << "Counter value should be 42";
		EXPECT_EQ(milanCounters, milan12Counters) << "Counters should be equal";
		// Getting counters for other type should throw
		EXPECT_THROW(streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlags17221>(), std::invalid_argument);
	}
	// First bit counter
	{
		auto const milanCounters = std::map<la::avdecc::entity::StreamOutputCounterValidFlagMilan12, la::avdecc::entity::model::DescriptorCounter>{ { la::avdecc::entity::StreamOutputCounterValidFlagMilan12::StreamStart, 42u } };
		auto streamOutputCounters = la::avdecc::entity::model::StreamOutputCounters{};
		streamOutputCounters.setCounters(milanCounters);
		EXPECT_EQ(la::avdecc::entity::model::StreamOutputCounters::CounterType::Milan_12, streamOutputCounters.getCounterType()) << "CounterType should be Milan 1.2";
		auto const milan12Counters = streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlagsMilan12>();
		EXPECT_EQ(1u, milan12Counters.size()) << "Should have 1 counter";
		EXPECT_EQ(42u, milan12Counters.at(la::avdecc::entity::StreamOutputCounterValidFlagMilan12::StreamStart)) << "Counter value should be 42";
		EXPECT_EQ(milanCounters, milan12Counters) << "Counters should be equal";
		// Getting counters for other type should throw
		EXPECT_THROW(streamOutputCounters.getCounters<la::avdecc::entity::StreamOutputCounterValidFlags17221>(), std::invalid_argument);
	}
}
