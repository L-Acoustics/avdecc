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
* @file controllerEntity_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/internals/protocolAemAecpdu.hpp>

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

TEST(ControllerEntity, DispatchWhileSending)
{
	static std::promise<void> dispatchDiscoveryPromise;
	static std::promise<void> testCompletedPromise;

	InstrumentationObserver instrumentationObserver{
		{
			// Dispatch ADP (discovery message) - Slow down the dispatcher so it still owns the ProtocolInterfaceVirtual lock when the getListenerStreamState is pushed
			{ "ProtocolInterfaceVirtual::onMessage::PostLock", []()
				{
					dispatchDiscoveryPromise.set_value();
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
				}
			},
			// Send ACMP (getListenerStreamState message, from main thread) - Lock has successfully been taken
			{ "ProtocolInterfaceVirtual::PushMessage::PostLock", []()
				{
					testCompletedPromise.set_value();
				}
			},
		}
	};
	la::avdecc::InstrumentationNotifier::getInstance().registerObserver(&instrumentationObserver);

	auto pi = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }));
	auto controllerGuard = std::make_unique<la::avdecc::entity::LocalEntityGuard<la::avdecc::entity::ControllerEntityImpl>>(pi.get(), std::uint16_t{ 1 }, la::avdecc::UniqueIdentifier{ 0u }, nullptr);
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

//TEST(ControllerEntity, DestroyWhileSending)
//{
//	static std::promise<void> commandResultPromise{};
//	{
//		auto pi = la::avdecc::protocol::ProtocolInterfaceVirtual::create("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } });
//		auto controllerGuard = std::make_unique<la::avdecc::entity::LocalEntityGuard<la::avdecc::entity::ControllerEntityImpl>>(pi.get(), 1, 0, nullptr);
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
