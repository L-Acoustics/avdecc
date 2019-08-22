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
* @file protocolInterface_pcap_tests.cpp
* @author Christophe Calmejane
*/

// Internal API
#include "protocolInterface/protocolInterface_pcap.hpp"

#include <gtest/gtest.h>
#include <future>
#include <chrono>
#include <iostream>
#include <string>

static la::avdecc::networkInterface::Interface getFirstInterface()
{
	auto interface = la::avdecc::networkInterface::Interface{};

	// COMMENTED CODE TO FORCE ALL TESTS USING THIS TO BE DISABLED AUTOMATICALLY ;)

	//la::avdecc::networkInterface::enumerateInterfaces(
	//	[&interface](la::avdecc::networkInterface::Interface const& intfc)
	//	{
	//		if (intfc.type == la::avdecc::networkInterface::Interface::Type::Ethernet && intfc.isEnabled && interface.type == la::avdecc::networkInterface::Interface::Type::None)
	//		{
	//			interface = intfc;
	//		}
	//	});
	return interface;
}

TEST(ProtocolInterfacePCap, InvalidName)
{
	// Not using EXPECT_THROW, we want to check the error code inside our custom exception
	try
	{
		std::unique_ptr<la::avdecc::protocol::ProtocolInterfacePcap>(la::avdecc::protocol::ProtocolInterfacePcap::createRawProtocolInterfacePcap(""));
		EXPECT_FALSE(true); // We expect an exception to have been raised
	}
	catch (la::avdecc::protocol::ProtocolInterface::Exception const& e)
	{
		EXPECT_EQ(la::avdecc::protocol::ProtocolInterface::Error::InterfaceNotFound, e.getError());
	}
}

TEST(ProtocolInterfacePCap, TransportError)
{
	static std::promise<void> entityOnlinePromise;
	static std::promise<void> completedPromise;

	class Observer : public la::avdecc::protocol::ProtocolInterface::Observer
	{
	private:
		// la::avdecc::protocol::ProtocolInterface::Observer overrides
		virtual void onTransportError(la::avdecc::protocol::ProtocolInterface* const pi) noexcept override
		{
			std::this_thread::sleep_for(std::chrono::seconds(15)); // Wait for an entity to go offline
			// Now we are sure ProtocolInterface (from avdecc::CommandStateMachine thread) wants to acquire the observers lock, but we currently own this lock
			// So let's call something that wants to acquire the CommandStateMachine and see if we deadlock or not
			pi->lock(); // This will use CSM's lock
			pi->unlock();
		}
		virtual void onRemoteEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& /*entity*/) noexcept override
		{
			static auto done{ false };
			if (!done)
			{
				entityOnlinePromise.set_value();
				std::cout << "Found an entity, now trigger a transport error by disabling the network interface\n";
				done = true;
			}
		}
		virtual void onRemoteEntityOffline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept override
		{
			completedPromise.set_value();
		}
		DECLARE_AVDECC_OBSERVER_GUARD(Observer);
	};

	auto const interface = getFirstInterface();
	if (interface.type != la::avdecc::networkInterface::Interface::Type::None)
	{
		std::cout << "Using interface " << interface.alias << std::endl;

		Observer obs;
		auto intfc = std::unique_ptr<la::avdecc::protocol::ProtocolInterfacePcap>(la::avdecc::protocol::ProtocolInterfacePcap::createRawProtocolInterfacePcap(interface.id));
		intfc->registerObserver(&obs);

		auto status = entityOnlinePromise.get_future().wait_for(std::chrono::seconds(5));
		ASSERT_NE(std::future_status::timeout, status) << "Failed to detect an online entity... stopping the test";

		status = completedPromise.get_future().wait_for(std::chrono::seconds(60));
		ASSERT_NE(std::future_status::timeout, status) << "Either deadlock or you didn't follow instructions quickly enough";
	}
}
