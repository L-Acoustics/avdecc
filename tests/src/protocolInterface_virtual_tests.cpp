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
* @file protocolInterface_virtual_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/executor.hpp>

// Internal API
#include "protocolInterface/protocolInterface_virtual.hpp"
#include "instrumentationObserver.hpp"

#include <gtest/gtest.h>
#include <future>
#include <chrono>

static auto constexpr DefaultExecutorName = "avdecc::protocol::PI";

TEST(ProtocolInterfaceVirtual, InvalidName)
{
	auto const executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(DefaultExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(DefaultExecutorName, la::avdecc::utils::ThreadPriority::Highest));

	// Not using EXPECT_THROW, we want to check the error code inside our custom exception
	try
	{
		std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, DefaultExecutorName));
		EXPECT_FALSE(true); // We expect an exception to have been raised
	}
	catch (la::avdecc::protocol::ProtocolInterface::Exception const& e)
	{
		EXPECT_EQ(la::avdecc::protocol::ProtocolInterface::Error::InvalidParameters, e.getError());
	}
}

TEST(ProtocolInterfaceVirtual, InvalidMac)
{
	auto const executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(DefaultExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(DefaultExecutorName, la::avdecc::utils::ThreadPriority::Highest));

	// Not using EXPECT_THROW, we want to check the error code inside our custom exception
	try
	{
		std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("InvalidMac", {}, DefaultExecutorName));
		EXPECT_FALSE(true); // We expect an exception to have been raised
	}
	catch (la::avdecc::protocol::ProtocolInterface::Exception const& e)
	{
		EXPECT_EQ(la::avdecc::protocol::ProtocolInterface::Error::InvalidParameters, e.getError());
	}
}

TEST(ProtocolInterfaceVirtual, ValidInterface)
{
	auto const executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(DefaultExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(DefaultExecutorName, la::avdecc::utils::ThreadPriority::Highest));

	EXPECT_NO_THROW(std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("ValidInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, DefaultExecutorName)););
}

TEST(ProtocolInterfaceVirtual, SendMessage)
{
	auto const executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(DefaultExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(DefaultExecutorName, la::avdecc::utils::ThreadPriority::Highest));
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
	auto intfc1 = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, DefaultExecutorName));
	auto intfc2 = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b } }, DefaultExecutorName));
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

TEST(ProtocolInterfaceVirtual, RegisterAfterDiscoveredEntities)
{
	auto const executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(DefaultExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(DefaultExecutorName, la::avdecc::utils::ThreadPriority::Highest));

	class Observer : public la::avdecc::protocol::ProtocolInterface::Observer
	{
	public:
		std::promise<void>& getPromise() noexcept
		{
			return _promise;
		}

	private:
		virtual void onRemoteEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& /*entity*/) noexcept override
		{
			_promise.set_value();
		}
		std::promise<void> _promise{};
		DECLARE_AVDECC_OBSERVER_GUARD(Observer);
	};
	auto intfc1 = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, DefaultExecutorName));
	auto intfc2 = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b } }, DefaultExecutorName));

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

	// Register an observer that will be notified of the new online entity
	{
		auto obs = Observer{};
		intfc2->registerObserver(&obs);

		// Send the adp message
		intfc1->sendAdpMessage(adpdu);

		// Wait for the discover message to be received by the second VirtualInterface (should be almost instant)
		auto const status = obs.getPromise().get_future().wait_for(std::chrono::milliseconds(100));
		ASSERT_NE(std::future_status::timeout, status);
	}

	// Register another observer and expect it to be notified of the already discovered online entity
	{
		auto obs = Observer{};
		intfc2->registerObserver(&obs);

		// Expect the observer to receive the EntityOnline notification, without having to send another message
		auto const status = obs.getPromise().get_future().wait_for(std::chrono::milliseconds(100));
		ASSERT_NE(std::future_status::timeout, status) << "Register Observer didn't properly notify of already discovered entities";
	}
}

TEST(ProtocolInterfaceVirtual, InjectPacket)
{
	auto const executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(DefaultExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(DefaultExecutorName, la::avdecc::utils::ThreadPriority::Highest));
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
	auto intfc = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, DefaultExecutorName));
	intfc->registerObserver(&obs);

	// Build adpdu frame
	auto adpdu = la::avdecc::protocol::Adpdu{};
	// Set Ether2 fields
	adpdu.setSrcAddress({ 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b });
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

	// PCap transport requires the full frame to be built
	auto buffer = la::avdecc::protocol::SerializationBuffer{};

	// Start with EtherLayer2
	la::avdecc::protocol::serialize<la::avdecc::protocol::EtherLayer2>(adpdu, buffer);
	// Then Avtp control
	la::avdecc::protocol::serialize<la::avdecc::protocol::AvtpduControl>(adpdu, buffer);
	// Then with Adp
	la::avdecc::protocol::serialize<la::avdecc::protocol::Adpdu>(adpdu, buffer);

	// Inject a raw packet
	intfc->injectRawPacket(la::avdecc::MemoryBuffer{ buffer.data(), buffer.size() });

	// Message is synchroneous, timeout should never trigger
	auto const status = entityOnlinePromise.get_future().wait_for(std::chrono::milliseconds(10));
	ASSERT_NE(std::future_status::timeout, status);
}
