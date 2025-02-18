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
* @file controllerCapabilityDelegate_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/executor.hpp>
//#include <la/avdecc/internals/protocolAemAecpdu.hpp>

// Internal API
#include "entity/controllerEntityImpl.hpp"
#include "protocolInterface/protocolInterface_virtual.hpp"
#include "protocol/protocolAemPayloads.hpp"

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <future>

static auto constexpr DefaultExecutorName = "avdecc::protocol::PI";

namespace
{
class ControllerCapabilityDelegate_F : public ::testing::Test
{
public:
	virtual void SetUp() override
	{
		_ew = la::avdecc::ExecutorManager::getInstance().registerExecutor(DefaultExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(DefaultExecutorName, la::avdecc::utils::ThreadPriority::Highest));
		_pi = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, DefaultExecutorName));
		auto const commonInformation = la::avdecc::entity::Entity::CommonInformation{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0u, la::avdecc::entity::TalkerCapabilities{}, 0u, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented }, std::nullopt, std::nullopt };
		auto const interfaceInfo = la::avdecc::entity::Entity::InterfaceInformation{ la::networkInterface::MacAddress{ { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, 31u, 0u, std::nullopt, std::nullopt };
		_controllerGuard = std::make_unique<la::avdecc::entity::LocalEntityGuard<la::avdecc::entity::ControllerEntityImpl>>(_pi.get(), commonInformation, la::avdecc::entity::Entity::InterfacesInformation{ { la::avdecc::entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } }, nullptr, nullptr);
	}

	virtual void TearDown() override
	{
		_controllerGuard.reset();
		_pi.reset();
	}

	la::avdecc::entity::ControllerEntity& getController() noexcept
	{
		return *_controllerGuard;
	}

private:
	la::avdecc::ExecutorManager::ExecutorWrapper::UniquePointer _ew{ nullptr, nullptr };
	std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual> _pi{ nullptr };
	std::unique_ptr<la::avdecc::entity::LocalEntityGuard<la::avdecc::entity::ControllerEntityImpl>> _controllerGuard{ nullptr };
};
} // namespace

/*
 * TESTING https://github.com/L-Acoustics/avdecc/issues/83
 * Callback triggered when there is a serialization exception, with a ProtocolError status
 */
TEST_F(ControllerCapabilityDelegate_F, AddStreamPortInputAudioMappings)
{
	// In order to trigger an exception we have to pass more than 63 mappings
	auto mappings = la::avdecc::entity::model::AudioMappings{};
	for (auto i = 0u; i <= 63; ++i)
	{
		mappings.push_back({});
	}

	static auto handlerPromise = std::promise<la::avdecc::entity::LocalEntity::AemCommandStatus>{};
	getController().addStreamPortInputAudioMappings({}, {}, mappings,
		[](auto const* const /*controller*/, auto const /*entityID*/, auto const status, auto const /*streamPortIndex*/, auto const& /*mappings*/)
		{
			handlerPromise.set_value(status);
		});

	auto fut = handlerPromise.get_future();
	auto status = fut.wait_for(std::chrono::seconds(1));
	ASSERT_NE(std::future_status::timeout, status) << "Handler not called";
	EXPECT_EQ(la::avdecc::entity::LocalEntity::AemCommandStatus::ProtocolError, fut.get());
}

TEST_F(ControllerCapabilityDelegate_F, AddStreamPortOutputAudioMappings)
{
	// In order to trigger an exception we have to pass more than 63 mappings
	auto mappings = la::avdecc::entity::model::AudioMappings{};
	for (auto i = 0u; i <= 63; ++i)
	{
		mappings.push_back({});
	}

	static auto handlerPromise = std::promise<la::avdecc::entity::LocalEntity::AemCommandStatus>{};
	getController().addStreamPortOutputAudioMappings({}, {}, mappings,
		[](auto const* const /*controller*/, auto const /*entityID*/, auto const status, auto const /*streamPortIndex*/, auto const& /*mappings*/)
		{
			handlerPromise.set_value(status);
		});

	auto fut = handlerPromise.get_future();
	auto status = fut.wait_for(std::chrono::seconds(1));
	ASSERT_NE(std::future_status::timeout, status) << "Handler not called";
	EXPECT_EQ(la::avdecc::entity::LocalEntity::AemCommandStatus::ProtocolError, fut.get());
}

TEST_F(ControllerCapabilityDelegate_F, RemoveStreamPortInputAudioMappings)
{
	// In order to trigger an exception we have to pass more than 63 mappings
	auto mappings = la::avdecc::entity::model::AudioMappings{};
	for (auto i = 0u; i <= 63; ++i)
	{
		mappings.push_back({});
	}

	static auto handlerPromise = std::promise<la::avdecc::entity::LocalEntity::AemCommandStatus>{};
	getController().removeStreamPortInputAudioMappings({}, {}, mappings,
		[](auto const* const /*controller*/, auto const /*entityID*/, auto const status, auto const /*streamPortIndex*/, auto const& /*mappings*/)
		{
			handlerPromise.set_value(status);
		});

	auto fut = handlerPromise.get_future();
	auto status = fut.wait_for(std::chrono::seconds(1));
	ASSERT_NE(std::future_status::timeout, status) << "Handler not called";
	EXPECT_EQ(la::avdecc::entity::LocalEntity::AemCommandStatus::ProtocolError, fut.get());
}

TEST_F(ControllerCapabilityDelegate_F, RemoveStreamPortOutputAudioMappings)
{
	// In order to trigger an exception we have to pass more than 63 mappings
	auto mappings = la::avdecc::entity::model::AudioMappings{};
	for (auto i = 0u; i <= 63; ++i)
	{
		mappings.push_back({});
	}

	static auto handlerPromise = std::promise<la::avdecc::entity::LocalEntity::AemCommandStatus>{};
	getController().removeStreamPortOutputAudioMappings({}, {}, mappings,
		[](auto const* const /*controller*/, auto const /*entityID*/, auto const status, auto const /*streamPortIndex*/, auto const& /*mappings*/)
		{
			handlerPromise.set_value(status);
		});

	auto fut = handlerPromise.get_future();
	auto status = fut.wait_for(std::chrono::seconds(1));
	ASSERT_NE(std::future_status::timeout, status) << "Handler not called";
	EXPECT_EQ(la::avdecc::entity::LocalEntity::AemCommandStatus::ProtocolError, fut.get());
}


/*
 * TESTING https://github.com/L-Acoustics/avdecc/issues/97
 * Callback triggered when there is a base protocol violation, with a BaseProtocolViolation status
 * This happens when the remote entity does not respond with the same type of message that was sent
 * (eg. Responding with an EntityAvailable when asked for AcquireEntity, or responding with ENTITY Descriptor when asked for CONFIGURATION Descriptor)
 */
TEST_F(ControllerCapabilityDelegate_F, BaseProtocolViolation)
{
	static constexpr auto EntityID = la::avdecc::UniqueIdentifier{ 0x060504030201FFFE };
	static auto testCompletePromise = std::promise<void>{};
	static auto entityOnlinePromise = std::promise<void>{};
	static auto acquireResultPromise = std::promise<la::avdecc::entity::LocalEntity::AemCommandStatus>{};
	static auto readDescriptorResultPromise = std::promise<la::avdecc::entity::LocalEntity::AemCommandStatus>{};

	// Define the controller delegate
	class Delegate final : public la::avdecc::entity::controller::DefaultedDelegate
	{
	private:
		virtual void onEntityOnline(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::Entity const& /*entity*/) noexcept override
		{
			if (entityID == EntityID)
			{
				entityOnlinePromise.set_value();
			}
		}
	};

	// Define virtual entity behavior
	auto virtualEntity = std::thread(
		[]()
		{
			// Define entity observer
			class Obs final : public la::avdecc::protocol::ProtocolInterface::Observer
			{
			private:
				virtual void onAecpduReceived(la::avdecc::protocol::ProtocolInterface* const pi, la::avdecc::protocol::Aecpdu const& aecpdu) noexcept override
				{
					auto const& aem = static_cast<la::avdecc::protocol::AemAecpdu const&>(aecpdu);
					if (aem.getCommandType() == la::avdecc::protocol::AemCommandType::AcquireEntity)
					{
						auto response = la::avdecc::protocol::AemAecpdu{ true };
						response.setSrcAddress(aem.getDestAddress());
						response.setDestAddress(aem.getSrcAddress());
						response.setStatus(la::avdecc::protocol::AecpStatus::Success);
						response.setTargetEntityID(aem.getTargetEntityID());
						response.setControllerEntityID(aem.getControllerEntityID());
						response.setSequenceID(aem.getSequenceID());
						// Respond with EntityAvalable instead of requested AcquireEntity
						response.setCommandType(la::avdecc::protocol::AemCommandType::EntityAvailable);
						pi->sendAecpMessage(response);
					}
					else if (aem.getCommandType() == la::avdecc::protocol::AemCommandType::ReadDescriptor)
					{
						auto response = la::avdecc::protocol::AemAecpdu{ true };
						response.setSrcAddress(aem.getDestAddress());
						response.setDestAddress(aem.getSrcAddress());
						response.setStatus(la::avdecc::protocol::AecpStatus::Success);
						response.setTargetEntityID(aem.getTargetEntityID());
						response.setControllerEntityID(aem.getControllerEntityID());
						response.setSequenceID(aem.getSequenceID());
						response.setCommandType(la::avdecc::protocol::AemCommandType::ReadDescriptor);
						auto ser = la::avdecc::Serializer<la::avdecc::protocol::AemAecpdu::MaximumSendPayloadBufferLength>{};
						ser << std::uint16_t{ 0u } << std::uint16_t{ 0 }; // ConfigurationIndex / Reserved
						// Respond with ENTITY Descriptor instead of requested CONFIGURATION Descriptor
						ser << la::avdecc::entity::model::DescriptorType::Entity << std::uint16_t{ 1 }; // DescriptorType / DescriptorIndex
						// Fake the size of an ENTITY descriptor, we don't care about the actual payload
						response.setCommandSpecificData(ser.data(), la::avdecc::protocol::aemPayload::AecpAemReadEntityDescriptorResponsePayloadSize);
						pi->sendAecpMessage(response);
					}
				}
				DECLARE_AVDECC_OBSERVER_GUARD(Obs);
			};
			auto intfc = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 } }, DefaultExecutorName));
			auto obs = Obs{};
			intfc->registerObserver(&obs);

			// Build adpdu frame
			auto adpdu = la::avdecc::protocol::Adpdu{};
			// Set Ether2 fields
			adpdu.setSrcAddress(intfc->getMacAddress());
			adpdu.setDestAddress(la::avdecc::protocol::Adpdu::Multicast_Mac_Address);
			// Set ADP fields
			adpdu.setMessageType(la::avdecc::protocol::AdpMessageType::EntityAvailable);
			adpdu.setValidTime(10);
			adpdu.setEntityID(EntityID);
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
			adpdu.setInterfaceIndex(0u);
			adpdu.setAssociationID(la::avdecc::UniqueIdentifier{});

			// Send the adp message
			intfc->sendAdpMessage(adpdu);

			// Wait for test completion
			auto const status = testCompletePromise.get_future().wait_for(std::chrono::seconds(3));
			ASSERT_NE(std::future_status::timeout, status);
		});

	// Set Controller Delegate
	auto delegate = Delegate{};
	auto& controller = getController();
	controller.setControllerDelegate(&delegate);

	// Wait for the entity to become online
	{
		auto const status = entityOnlinePromise.get_future().wait_for(std::chrono::seconds(1));
		ASSERT_NE(std::future_status::timeout, status);
	}

	// Send an Acquire Entity message
	controller.acquireEntity(EntityID, false, la::avdecc::entity::model::DescriptorType::Entity, 0u,
		[](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const /*owningEntity*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/)
		{
			acquireResultPromise.set_value(status);
		});

	// Wait for acquire result with incorrect response
	{
		auto fut = acquireResultPromise.get_future();
		ASSERT_NE(std::future_status::timeout, fut.wait_for(std::chrono::seconds(1)));
		ASSERT_EQ(la::avdecc::entity::LocalEntity::AemCommandStatus::BaseProtocolViolation, fut.get());
	}

	// Send a Read Descriptor message
	controller.readConfigurationDescriptor(EntityID, 0u,
		[](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ConfigurationDescriptor const& /*descriptor*/)
		{
			readDescriptorResultPromise.set_value(status);
		});

	// Wait for read descriptor result with incorrect response
	{
		auto fut = readDescriptorResultPromise.get_future();
		ASSERT_NE(std::future_status::timeout, fut.wait_for(std::chrono::seconds(1)));
		ASSERT_EQ(la::avdecc::entity::LocalEntity::AemCommandStatus::BaseProtocolViolation, fut.get());
	}

	// Complete the test
	testCompletePromise.set_value();
	virtualEntity.join();
}
