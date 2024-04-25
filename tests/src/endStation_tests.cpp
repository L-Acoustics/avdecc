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
* @file endStation_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/executor.hpp>
#include <la/avdecc/internals/endStation.hpp>

// Internal API

#include <gtest/gtest.h>

TEST(EndStation, DefaultExecutor)
{
	try
	{
		auto const endStation = la::avdecc::EndStation::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", std::nullopt);
		EXPECT_NE(nullptr, endStation);
	}
	catch (...)
	{
		FAIL() << "Exception thrown";
	}
}

TEST(EndStation, MultipleDefaultExecutor)
{
	// Global scope for the endStation, we need it to stay alive until we create the second EndStation
	auto endStation = la::avdecc::EndStation::UniquePointer{ nullptr, nullptr };
	try
	{
		endStation = la::avdecc::EndStation::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", std::nullopt);
		EXPECT_NE(nullptr, endStation);
	}
	catch (...)
	{
		FAIL() << "Exception thrown";
	}

	// Try to create another EndStation with the same default executor, which should fail
	try
	{
		la::avdecc::EndStation::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface 2", std::nullopt);
		FAIL() << "Exception not thrown";
	}
	catch (la::avdecc::EndStation::Exception const& ex)
	{
		EXPECT_EQ(la::avdecc::EndStation::Error::DuplicateExecutorName, ex.getError());
	}

	// Release the first EndStation
	endStation.reset();

	// Try to create another EndStation with the same default executor, which should now succeed
	try
	{
		endStation = la::avdecc::EndStation::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface 2", std::nullopt);
		EXPECT_NE(nullptr, endStation);
	}
	catch (...)
	{
		FAIL() << "Exception thrown";
	}
}

TEST(EndStation, UnknownExecutor)
{
	// Try to create an EndStation with an unknown executor, which should fail
	try
	{
		la::avdecc::EndStation::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", "UnknownExecutor");
		FAIL() << "Exception not thrown";
	}
	catch (la::avdecc::EndStation::Exception const& ex)
	{
		EXPECT_EQ(la::avdecc::EndStation::Error::UnknownExecutorName, ex.getError());
	}
}

TEST(EndStation, ProvidedExecutor)
{
	auto const exName = "ProvidedExecutor";

	// Create an executor
	auto const executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(exName, la::avdecc::ExecutorWithDispatchQueue::create(exName, la::avdecc::utils::ThreadPriority::Highest));
	EXPECT_NE(nullptr, executorWrapper);

	// Try to create an EndStation with the provided executor, which should succeed
	// Global scope for the endStation, we need it to stay alive until we create the second EndStation
	auto endStation = la::avdecc::EndStation::UniquePointer{ nullptr, nullptr };
	try
	{
		endStation = la::avdecc::EndStation::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", exName);
		EXPECT_NE(nullptr, endStation);
	}
	catch (...)
	{
		FAIL() << "Exception thrown";
	}

	// Try to create another EndStation with the same provided executor, which should succeed
	try
	{
		auto const endStation2 = la::avdecc::EndStation::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface 2", exName);
		EXPECT_NE(nullptr, endStation2);
	}
	catch (...)
	{
		FAIL() << "Exception thrown";
	}
}
