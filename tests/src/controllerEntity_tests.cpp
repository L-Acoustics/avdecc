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
* @file controllerEntity_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/internals/protocolAemAecpdu.hpp>
#include <la/avdecc/executor.hpp>

// Internal API
#include "entity/controllerEntityImpl.hpp"
#include "protocolInterface/protocolInterface_virtual.hpp"
#include "instrumentationObserver.hpp"

#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <chrono>
#include <list>
#include <future>

static auto constexpr DefaultExecutorName = "avdecc::protocol::PI";

TEST(ControllerEntity, DispatchWhileSending)
{
	static std::promise<void> dispatchDiscoveryPromise;
	static std::promise<void> testCompletedPromise;

	InstrumentationObserver instrumentationObserver{ {
		// Dispatch ADP (discovery message) - Slow down the dispatcher so it still owns the ProtocolInterfaceVirtual lock when the getListenerStreamState is pushed
		{ "ProtocolInterfaceVirtual::onMessage::PostLock",
			[]()
			{
				dispatchDiscoveryPromise.set_value();
				std::this_thread::sleep_for(std::chrono::milliseconds(200));
			} },
		// Send ACMP (getListenerStreamState message, from main thread) - Lock has successfully been taken
		{ "ProtocolInterfaceVirtual::PushMessage::PostLock",
			[]()
			{
				testCompletedPromise.set_value();
			} },
	} };
	la::avdecc::InstrumentationNotifier::getInstance().registerObserver(&instrumentationObserver);

	auto pi = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, DefaultExecutorName));
	auto const commonInformation = la::avdecc::entity::Entity::CommonInformation{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0u, la::avdecc::entity::TalkerCapabilities{}, 0u, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented }, std::nullopt, std::nullopt };
	auto const interfaceInfo = la::avdecc::entity::Entity::InterfaceInformation{ la::networkInterface::MacAddress{ { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, 31u, 0u, std::nullopt, std::nullopt };
	auto controllerGuard = std::make_unique<la::avdecc::entity::LocalEntityGuard<la::avdecc::entity::ControllerEntityImpl>>(pi.get(), commonInformation, la::avdecc::entity::Entity::InterfacesInformation{ { la::avdecc::entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } }, nullptr, nullptr);
	auto* const controller = static_cast<la::avdecc::entity::ControllerEntity*>(controllerGuard.get());

	// Wait for ProtocolInterfaceVirtual dispatch thread to process the discovery message
	auto status = dispatchDiscoveryPromise.get_future().wait_for(std::chrono::seconds(1));
	ASSERT_NE(std::future_status::timeout, status) << "Test conception failure";

	// Ok we can send a message
	controller->getListenerStreamState({ la::avdecc::UniqueIdentifier{ 0x000102FFFE030405 }, 0u }, nullptr);

	// Wait for the test to be completed
	status = testCompletedPromise.get_future().wait_for(std::chrono::seconds(5));
	ASSERT_NE(std::future_status::timeout, status) << "Dead lock!";
}

/*
 * TESTING https://github.com/L-Acoustics/avdecc/issues/55
 * ControllerEntity should detect when the main AvbInterface is lost
 */
TEST(ControllerEntity, DetectMainAvbInterfaceLost)
{
	auto const executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(DefaultExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(DefaultExecutorName, la::avdecc::utils::ThreadPriority::Highest));

	static constexpr auto EntityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	static auto entityOfflinePromise = std::promise<void>{};

	class Delegate final : public la::avdecc::entity::controller::DefaultedDelegate
	{
	private:
		virtual void onEntityOffline(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID) noexcept override
		{
			if (entityID == EntityID)
			{
				entityOfflinePromise.set_value();
			}
		}
	};

	// Create a ControllerEntity
	auto controllerProtocolInterface = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, DefaultExecutorName));
	auto const commonInformation = la::avdecc::entity::Entity::CommonInformation{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{}, 0u, la::avdecc::entity::TalkerCapabilities{}, 0u, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented }, std::nullopt, std::nullopt };
	auto const interfaceInfo = la::avdecc::entity::Entity::InterfaceInformation{ la::networkInterface::MacAddress{ { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, 31u, 0u, std::nullopt, std::nullopt };
	auto controllerGuard = std::make_unique<la::avdecc::entity::LocalEntityGuard<la::avdecc::entity::ControllerEntityImpl>>(controllerProtocolInterface.get(), commonInformation, la::avdecc::entity::Entity::InterfacesInformation{ { la::avdecc::entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } }, nullptr, nullptr);
	auto delegate = Delegate{};
	static_cast<la::avdecc::entity::ControllerEntity&>(*controllerGuard).setControllerDelegate(&delegate);

	auto const sendAdpAvailable = [](auto const& entityID, auto const interfaceIndex, auto const validTime)
	{
		auto intfc = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { static_cast<la::networkInterface::MacAddress::value_type>(interfaceIndex), 0x06, 0x05, 0x04, 0x03, 0x02 } }, DefaultExecutorName));

		// Build adpdu frame
		auto adpdu = la::avdecc::protocol::Adpdu{};
		// Set Ether2 fields
		adpdu.setSrcAddress(intfc->getMacAddress());
		adpdu.setDestAddress(la::avdecc::protocol::Adpdu::Multicast_Mac_Address);
		// Set ADP fields
		adpdu.setMessageType(la::avdecc::protocol::AdpMessageType::EntityAvailable);
		adpdu.setValidTime(validTime);
		adpdu.setEntityID(entityID);
		adpdu.setEntityModelID(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier());
		adpdu.setEntityCapabilities(la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemInterfaceIndexValid });
		adpdu.setTalkerStreamSources(0);
		adpdu.setTalkerCapabilities({});
		adpdu.setListenerStreamSinks(0);
		adpdu.setListenerCapabilities({});
		adpdu.setControllerCapabilities(la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented });
		adpdu.setAvailableIndex(1);
		adpdu.setGptpGrandmasterID({});
		adpdu.setGptpDomainNumber(0);
		adpdu.setIdentifyControlIndex(0);
		adpdu.setInterfaceIndex(interfaceIndex);
		adpdu.setAssociationID(la::avdecc::UniqueIdentifier{});

		// Send the adp message
		intfc->sendAdpMessage(adpdu);

		// Wait for the message to actually be sent (destroying the protocol interface won't flush pending messages, not at this date with the current code)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	};

	// Simulate ADP Available messages from the 2 interfaces of the same Entity
	// The first discovered interface will be used as the "main" interface
	// Use a low validTime for the main interface so it will timeout quickly
	sendAdpAvailable(EntityID, la::avdecc::entity::model::AvbInterfaceIndex{ 0 }, std::uint8_t{ 2 });
	sendAdpAvailable(EntityID, la::avdecc::entity::model::AvbInterfaceIndex{ 1 }, std::uint8_t{ 20 });

	// Wait for the "main" interface to timeout
	std::this_thread::sleep_for(std::chrono::seconds(5));

	// Wait for the handler to complete
	auto status = entityOfflinePromise.get_future().wait_for(std::chrono::seconds(1));
	ASSERT_NE(std::future_status::timeout, status);
}

//TEST(ControllerEntity, DestroyWhileSending)
//{
//	static std::promise<void> commandResultPromise{};
//	{
//		auto pi = la::avdecc::protocol::ProtocolInterfaceVirtual::create("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } });
//		auto controllerGuard = std::make_unique<la::avdecc::entity::LocalEntityGuard<la::avdecc::entity::ControllerEntityImpl>>(pi.get(), 1, 0, nullptr, nullptr);
//		auto* const controller = static_cast<la::avdecc::entity::ControllerEntity*>(controllerGuard.get());
//
//		controller->getListenerStreamState(0x000102FFFE030405, 0, [](la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const listenerEntityID, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, la::avdecc::UniqueIdentifier const talkerEntityID, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status)
//		{
//			// Wait a little bit so the controllerGuard has time to go out of scope and release
//			std::this_thread::sleep_for(std::chrono::milliseconds(100));
//			commandResultPromise.set_value();
//		});
//		// Let the ControllerGuard go out of scope for destruction
//	}
//
//	// Wait for the handler to complete
//	auto status = commandResultPromise.get_future().wait_for(std::chrono::seconds(1));
//	ASSERT_NE(std::future_status::timeout, status);
//}
