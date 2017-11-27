/*
* Copyright (C) 2016-2017, L-Acoustics and its contributors

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
* @file protocolInterface_virtual_tests.cpp
* @author Christophe Calmejane
*/

#include <gtest/gtest.h>
#include <future>
#include <chrono>
#include "protocolInterface/protocolInterface_virtual.hpp"

TEST(ProtocolInterfaceVirtual, InvalidName)
{
	// Not using EXPECT_THROW, we want to check the error code inside our custom exception
	try
	{
		la::avdecc::protocol::ProtocolInterfaceVirtual::create("", { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 });
		EXPECT_FALSE(true); // We expect an exception to have been raised
	}
	catch (la::avdecc::protocol::ProtocolInterface::Exception const& e)
	{
		EXPECT_EQ(la::avdecc::protocol::ProtocolInterface::Error::InterfaceInvalid, e.getError());
	}
}

TEST(ProtocolInterfaceVirtual, InvalidMac)
{
	// Not using EXPECT_THROW, we want to check the error code inside our custom exception
	try
	{
		la::avdecc::protocol::ProtocolInterfaceVirtual::create("InvalidMac", { });
		EXPECT_FALSE(true); // We expect an exception to have been raised
	}
	catch (la::avdecc::protocol::ProtocolInterface::Exception const& e)
	{
		EXPECT_EQ(la::avdecc::protocol::ProtocolInterface::Error::InterfaceInvalid, e.getError());
	}
}

TEST(ProtocolInterfaceVirtual, ValidInterface)
{
	EXPECT_NO_THROW(
		la::avdecc::protocol::ProtocolInterfaceVirtual::create("ValidInterface", { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 });
	);
}

TEST(ProtocolInterfaceVirtual, SendMessage)
{
	static std::promise<void> entityOnlinePromise;
	class Observer : public la::avdecc::protocol::ProtocolInterface::Observer
	{
	public:
		virtual void onTransportError(la::avdecc::protocol::ProtocolInterface const* const pi) noexcept override {}
		virtual void onLocalEntityOnline(la::avdecc::protocol::ProtocolInterface const* const pi, la::avdecc::entity::DiscoveredEntity const& entity) noexcept override {}
		virtual void onLocalEntityOffline(la::avdecc::protocol::ProtocolInterface const* const pi, la::avdecc::UniqueIdentifier const entityID) noexcept override {}
		virtual void onLocalEntityUpdated(la::avdecc::protocol::ProtocolInterface const* const pi, la::avdecc::entity::DiscoveredEntity const& entity) noexcept override {}
		virtual void onRemoteEntityOnline(la::avdecc::protocol::ProtocolInterface const* const /*pi*/, la::avdecc::entity::DiscoveredEntity const& /*entity*/) noexcept override
		{
			entityOnlinePromise.set_value();
		}
		virtual void onRemoteEntityOffline(la::avdecc::protocol::ProtocolInterface const* const pi, la::avdecc::UniqueIdentifier const entityID) noexcept override {}
		virtual void onRemoteEntityUpdated(la::avdecc::protocol::ProtocolInterface const* const pi, la::avdecc::entity::DiscoveredEntity const& entity) noexcept override {}
		virtual void onAecpCommand(la::avdecc::protocol::ProtocolInterface const* const pi, la::avdecc::entity::LocalEntity const& entity, la::avdecc::protocol::Aecpdu const& aecpdu) noexcept override {}
		virtual void onAecpUnsolicitedResponse(la::avdecc::protocol::ProtocolInterface const* const pi, la::avdecc::entity::LocalEntity const& entity, la::avdecc::protocol::Aecpdu const& aecpdu) noexcept override {}
		virtual void onAcmpSniffedCommand(la::avdecc::protocol::ProtocolInterface const* const pi, la::avdecc::entity::LocalEntity const& entity, la::avdecc::protocol::Acmpdu const& acmpdu) noexcept override {}
		virtual void onAcmpSniffedResponse(la::avdecc::protocol::ProtocolInterface const* const pi, la::avdecc::entity::LocalEntity const& entity, la::avdecc::protocol::Acmpdu const& acmpdu) noexcept override {}
	};
	Observer obs;
	auto intfc1 = la::avdecc::protocol::ProtocolInterfaceVirtual::create("VirtualInterface", { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 });
	auto intfc2 = la::avdecc::protocol::ProtocolInterfaceVirtual::create("VirtualInterface", { 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b });
	intfc2->registerObserver(&obs);

	// Build adpdu frame
	auto adpdu = la::avdecc::protocol::Adpdu::create();
	// Set Ether2 fields
	adpdu->setSrcAddress(intfc1->getMacAddress());
	adpdu->setDestAddress(la::avdecc::protocol::Adpdu::Multicast_Mac_Address);
	// Set ADP fields
	adpdu->setMessageType(la::avdecc::protocol::AdpMessageType::EntityAvailable);
	adpdu->setValidTime(2);
	adpdu->setEntityID(0x0001020304050607);
	adpdu->setEntityModelID(0);
	adpdu->setEntityCapabilities(la::avdecc::entity::EntityCapabilities::None);
	adpdu->setTalkerStreamSources(0);
	adpdu->setTalkerCapabilities(la::avdecc::entity::TalkerCapabilities::None);
	adpdu->setListenerStreamSinks(0);
	adpdu->setListenerCapabilities(la::avdecc::entity::ListenerCapabilities::None);
	adpdu->setControllerCapabilities(la::avdecc::entity::ControllerCapabilities::Implemented);
	adpdu->setAvailableIndex(1);
	adpdu->setGptpGrandmasterID(0);
	adpdu->setGptpDomainNumber(0);
	adpdu->setIdentifyControlIndex(0);
	adpdu->setInterfaceIndex(intfc1->getInterfaceIndex());
	adpdu->setAssociationID(0);

	// Send the adp message
	intfc1->sendAdpMessage(std::move(adpdu));

	// Message is synchroneous, timeout should never trigger
	auto const status = entityOnlinePromise.get_future().wait_for(std::chrono::milliseconds(10));
	ASSERT_NE(std::future_status::timeout, status);
}
