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
* @file protocolInterface_virtual_tests.cpp
* @author Christophe Calmejane
*/

// Internal API
#include "protocolInterface/protocolInterface_virtual.hpp"
#include "instrumentationObserver.hpp"

#include <gtest/gtest.h>
#include <future>
#include <chrono>

TEST(ProtocolInterfaceVirtual, InvalidName)
{
	// Not using EXPECT_THROW, we want to check the error code inside our custom exception
	try
	{
		std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }));
		EXPECT_FALSE(true); // We expect an exception to have been raised
	}
	catch (la::avdecc::protocol::ProtocolInterface::Exception const& e)
	{
		EXPECT_EQ(la::avdecc::protocol::ProtocolInterface::Error::InvalidParameters, e.getError());
	}
}

TEST(ProtocolInterfaceVirtual, InvalidMac)
{
	// Not using EXPECT_THROW, we want to check the error code inside our custom exception
	try
	{
		std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("InvalidMac", {}));
		EXPECT_FALSE(true); // We expect an exception to have been raised
	}
	catch (la::avdecc::protocol::ProtocolInterface::Exception const& e)
	{
		EXPECT_EQ(la::avdecc::protocol::ProtocolInterface::Error::InvalidParameters, e.getError());
	}
}

TEST(ProtocolInterfaceVirtual, ValidInterface)
{
	EXPECT_NO_THROW(std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("ValidInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } })););
}

TEST(ProtocolInterfaceVirtual, SendMessage)
{
	static std::promise<void> entityOnlinePromise;
	class Observer : public la::avdecc::protocol::ProtocolInterface::Observer
	{
	private:
		virtual void onRemoteEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& /*entity*/) noexcept override
		{
			entityOnlinePromise.set_value();
		}
		DECLARE_AVDECC_OBSERVER_GUARD(Observer);
	};
	Observer obs;
	auto intfc1 = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }));
	auto intfc2 = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b } }));
	intfc2->registerObserver(&obs);

	// Build adpdu frame
	auto adpdu = la::avdecc::protocol::Adpdu{};
	// Set Ether2 fields
	adpdu.setSrcAddress(intfc1->getMacAddress());
	adpdu.setDestAddress(la::avdecc::protocol::Adpdu::Multicast_Mac_Address);
	// Set ADP fields
	adpdu.setMessageType(la::avdecc::protocol::AdpMessageType::EntityAvailable);
	adpdu.setValidTime(2);
	adpdu.setEntityID(la::avdecc::UniqueIdentifier{ 0x0001020304050607 });
	adpdu.setEntityModelID(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier());
	adpdu.setEntityCapabilities({});
	adpdu.setTalkerStreamSources(0);
	adpdu.setTalkerCapabilities({});
	adpdu.setListenerStreamSinks(0);
	adpdu.setListenerCapabilities({});
	adpdu.setControllerCapabilities(la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented });
	adpdu.setAvailableIndex(1);
	adpdu.setGptpGrandmasterID(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier());
	adpdu.setGptpDomainNumber(0);
	adpdu.setIdentifyControlIndex(0);
	adpdu.setInterfaceIndex(0);
	adpdu.setAssociationID(la::avdecc::UniqueIdentifier{});

	// Send the adp message
	intfc1->sendAdpMessage(adpdu);

	// Message is synchroneous, timeout should never trigger
	auto const status = entityOnlinePromise.get_future().wait_for(std::chrono::milliseconds(10));
	ASSERT_NE(std::future_status::timeout, status);
}

TEST(ProtocolInterfaceVirtual, TransportError)
{
	static std::promise<void> entityOnlinePromise;
	static std::promise<void> transportErrorPromise;
	static std::promise<void> entityOfflinePromise;
	static std::promise<void> completedPromise;

	class Observer : public la::avdecc::protocol::ProtocolInterface::Observer, public la::avdecc::InstrumentationNotifier::Observer
	{
	private:
		// la::avdecc::protocol::ProtocolInterface::Observer overrides
		virtual void onTransportError(la::avdecc::protocol::ProtocolInterface* const pi) noexcept override
		{
			// This is the ProtocolInterface Capture thread
			transportErrorPromise.set_value();
			// Wait for onRemoteEntityOffline pre notify before continuing
			auto status = entityOfflinePromise.get_future().wait_for(std::chrono::milliseconds(50));
			ASSERT_NE(std::future_status::timeout, status);
			// Now we are sure ProtocolInterface (from avdecc::CommandStateMachine thread) wants to acquire the observers lock, but we currently own this lock
			// So let's call something that wants to acquire the CommandStateMachine and see if we deadlock or not
			pi->lock(); // This will use CSM's lock
			pi->unlock();
		}
		virtual void onRemoteEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& /*entity*/) noexcept override
		{
			entityOnlinePromise.set_value();
		}
		// la::avdecc::InstrumentationNotifier::Observer overrides
		virtual void onEvent(std::string const& eventName) noexcept override
		{
			if (eventName == "ProtocolInterfaceVirtual::onRemoteEntityOffline::PreNotify")
			{
				entityOfflinePromise.set_value();
				// Wait for transport error before continuing
				auto status = transportErrorPromise.get_future().wait_for(std::chrono::milliseconds(50));
				ASSERT_NE(std::future_status::timeout, status);
			}
			else if (eventName == "ProtocolInterfaceVirtual::onRemoteEntityOffline::PostNotify")
			{
				completedPromise.set_value();
			}
		}
		DECLARE_AVDECC_OBSERVER_GUARD_NAME(la::avdecc::protocol::ProtocolInterface::Observer, _guard1);
		DECLARE_AVDECC_OBSERVER_GUARD_NAME(la::avdecc::InstrumentationNotifier::Observer, _guard2);
	};

	Observer obs;
	la::avdecc::InstrumentationNotifier::getInstance().registerObserver(&obs);
	auto intfc = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }));
	intfc->registerObserver(&obs);

	// Build adpdu frame
	auto adpdu = la::avdecc::protocol::Adpdu{};
	// Set Ether2 fields
	adpdu.setSrcAddress(intfc->getMacAddress());
	adpdu.setDestAddress(la::avdecc::protocol::Adpdu::Multicast_Mac_Address);
	// Set ADP fields
	adpdu.setMessageType(la::avdecc::protocol::AdpMessageType::EntityAvailable);
	adpdu.setValidTime(0);
	adpdu.setEntityID(la::avdecc::UniqueIdentifier{ 0x0001020304050607 });
	adpdu.setEntityModelID(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier());
	adpdu.setEntityCapabilities({});
	adpdu.setTalkerStreamSources(0);
	adpdu.setTalkerCapabilities({});
	adpdu.setListenerStreamSinks(0);
	adpdu.setListenerCapabilities({});
	adpdu.setControllerCapabilities(la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented });
	adpdu.setAvailableIndex(0);
	adpdu.setGptpGrandmasterID(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier());
	adpdu.setGptpDomainNumber(0);
	adpdu.setIdentifyControlIndex(0);
	adpdu.setInterfaceIndex(0);
	adpdu.setAssociationID(la::avdecc::UniqueIdentifier{});

	// Send the adp message
	intfc->sendAdpMessage(adpdu);

	auto status = entityOnlinePromise.get_future().wait_for(std::chrono::milliseconds(50));
	ASSERT_NE(std::future_status::timeout, status);

	// Force a transport error
	auto const& virtIntfc = static_cast<la::avdecc::protocol::ProtocolInterfaceVirtual const&>(*intfc);
	virtIntfc.forceTransportError();

	status = completedPromise.get_future().wait_for(std::chrono::seconds(1));
	ASSERT_NE(std::future_status::timeout, status) << "Deadlock!";
}
