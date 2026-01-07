/*
* Copyright (C) 2016-2026, L-Acoustics and its contributors

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

#include <optional>
#include <string>

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

TEST(EndStation, LoadEntityModel)
{
	auto [errorCode, errorMessage, entityModelTree] = la::avdecc::EndStation::deserializeEntityModelFromJson("data/SimpleControllerModelV2.json", true, false);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, errorCode);
	EXPECT_TRUE(errorMessage.empty());
	EXPECT_FALSE(entityModelTree.configurationTrees.empty());
}

static std::optional<std::string> getFirstAvailableNetworkInterface()
{
	auto interfaceName = std::optional<std::string>{};
	la::networkInterface::NetworkInterfaceHelper::getInstance().enumerateInterfaces(
		[&interfaceName](la::networkInterface::Interface const& intfc)
		{
			if (intfc.isEnabled && intfc.isConnected && !intfc.isVirtual && (intfc.type == la::networkInterface::Interface::Type::Ethernet || intfc.type == la::networkInterface::Interface::Type::WiFi))
			{
				interfaceName = intfc.id;
			}
		});

	return interfaceName;
}

static void destroyWhileMessageInflight(la::avdecc::protocol::ProtocolInterface::Type const interfaceType)
{
	// Check if the requested protocol interface is available
	auto const interfaces = la::avdecc::protocol::ProtocolInterface::getSupportedProtocolInterfaceTypes();
	if (interfaces.test(interfaceType))
	{
		auto const interfaceName = getFirstAvailableNetworkInterface();
		if (interfaceName)
		{
			std::promise<void> commandResultPromise{};

			// Create an EndStation
			auto endStation = la::avdecc::EndStation::create(interfaceType, *interfaceName, std::nullopt);
			ASSERT_TRUE(endStation);

			// Add a controller
			auto controller = endStation->addControllerEntity(1, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), nullptr, nullptr);

			// Send a message that we know will not reach its destination (non-existant entityID) before we shutdown the EndStation
			controller->getListenerStreamState(la::avdecc::entity::model::StreamIdentification{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, 0 },
				[&commandResultPromise](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, std::uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const status)
				{
					EXPECT_EQ(la::avdecc::entity::LocalEntity::ControlStatus::UnknownEntity, status);
					commandResultPromise.set_value();
				});

			// Destroy the EndStation while the message is inflight
			endStation.reset();

			// Wait for the command to complete
			auto status = commandResultPromise.get_future().wait_for(std::chrono::seconds(10)); // Wait a bit longer than the default timeout (ACMP message timeout is 5s)
			ASSERT_NE(std::future_status::timeout, status);
		}
	}
}

TEST(INTEGRATION_EndStation, DestroyWhileMessageInflight_macOSNative)
{
	destroyWhileMessageInflight(la::avdecc::protocol::ProtocolInterface::Type::MacOSNative);
}

TEST(INTEGRATION_EndStation, DestroyWhileMessageInflight_pcap)
{
	destroyWhileMessageInflight(la::avdecc::protocol::ProtocolInterface::Type::PCap);
}
