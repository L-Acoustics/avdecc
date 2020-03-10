/*
* Copyright (C) 2016-2020, L-Acoustics and its contributors

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
* @file avdeccControlledEntity_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/controller/avdeccController.hpp>
#include <la/avdecc/internals/protocolAemAecpdu.hpp>

// Internal API
#include "controller/avdeccControlledEntityImpl.hpp"
#include "entity/controllerEntityImpl.hpp"
#include "protocolInterface/protocolInterface_virtual.hpp"

#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <chrono>
#include <future>

//namespace
//{
//class ControlledEntity : public ::testing::Test
//{
//public:
//	virtual void SetUp() override
//	{
//		auto sharedLock = std::make_shared<la::avdecc::controller::ControlledEntityImpl::LockInformation>();
//		auto const commonInformation{ la::avdecc::entity::Entity::CommonInformation{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0u, la::avdecc::entity::TalkerCapabilities{}, 0u, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented }, std::nullopt, std::nullopt } };
//		auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ la::avdecc::networkInterface::MacAddress{}, 31u, 0u, std::nullopt, std::nullopt } };
//		auto const e{ la::avdecc::entity::Entity{ commonInformation, la::avdecc::entity::Entity::InterfacesInformation{ { la::avdecc::entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } } } };
//		la::avdecc::controller::ControlledEntityImpl entity{ e, sharedLock, false };
//	}
//	virtual void TearDown() override
//	{
//	}
//};
//}

TEST(ControlledEntity, VirtualEntityLoad)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	{
		auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en");
		auto const [error, message] = controller->loadVirtualEntityFromJson("data/SimpleEntity.json", flags);
		EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
		EXPECT_STREQ("", message.c_str());
	}
	{
		auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en");
		auto const [error, message] = controller->loadVirtualEntityFromJson("data/Listener_EmptyMappings.json", flags);
		EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
		EXPECT_STREQ("", message.c_str());
	}
	{
		auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en");
		auto const [error, message] = controller->loadVirtualEntityFromJson("data/RedundantListener_EmptyMappings.json", flags);
		EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
		EXPECT_STREQ("", message.c_str());
	}
	{
		auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en");
		auto const [error, message] = controller->loadVirtualEntityFromJson("data/RedundantListener_InvertedStreamIndex_EmptyMappings.json", flags);
		EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
		EXPECT_STREQ("", message.c_str());
	}
}

TEST(ControlledEntity, AddChannelMappings)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Load entity
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en");
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/Listener_EmptyMappings.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 })));
	constexpr auto StreamPort = la::avdecc::entity::model::StreamPortIndex{ 0u };
	auto const Mapping = la::avdecc::entity::model::AudioMapping{ 0, 0, 0, 0 };

	// Add "no" mapping
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{});
	EXPECT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).empty()) << "Mappings should still be empty";

	// Add one mapping
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping });
	ASSERT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
	EXPECT_TRUE(Mapping == e.getStreamPortInputAudioMappings(StreamPort).at(0));

	// Add same mapping
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping });
	ASSERT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
	EXPECT_TRUE(Mapping == e.getStreamPortInputAudioMappings(StreamPort).at(0));
}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
TEST(ControlledEntity, AddRedundantChannelMappings)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Load entity
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en");
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/RedundantListener_InvertedStreamIndex_EmptyMappings.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 })));
	constexpr auto StreamPort = la::avdecc::entity::model::StreamPortIndex{ 0u };
	auto const Mapping = la::avdecc::entity::model::AudioMapping{ 0, 0, 0, 0 }; // This is actually the mapping for the Secondary Stream
	auto const RedundantMapping = la::avdecc::entity::model::AudioMapping{ 1, 0, 0, 0 }; // And this for the Primary Stream

	// Add "no" mapping
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{});
	EXPECT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).empty()) << "Mappings should still be empty";

	// Add one mapping
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping });
	ASSERT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
	EXPECT_TRUE(Mapping == e.getStreamPortInputAudioMappings(StreamPort).at(0));

	// Add same mapping
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping });
	ASSERT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
	EXPECT_TRUE(Mapping == e.getStreamPortInputAudioMappings(StreamPort).at(0));

	// Add redundancy mapping
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ RedundantMapping });
	ASSERT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).size() == 2) << "Mappings should have changed";
	EXPECT_TRUE(Mapping == e.getStreamPortInputAudioMappings(StreamPort).at(0));
	EXPECT_TRUE(RedundantMapping == e.getStreamPortInputAudioMappings(StreamPort).at(1));
	ASSERT_TRUE(e.getStreamPortInputNonRedundantAudioMappings(StreamPort).size() == 1) << "NonRedundantMappings should not have changed though";
	EXPECT_TRUE(RedundantMapping == e.getStreamPortInputNonRedundantAudioMappings(StreamPort).at(0)) << "NonRedundantMappings should return the mappings for the Primary Stream";
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
