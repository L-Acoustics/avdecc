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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <chrono>
#include <future>
#include <la/avdecc/executor.hpp>

namespace
{
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

class LogObserver : public la::avdecc::logger::Logger::Observer
{
public:
	virtual ~LogObserver() noexcept override
	{
		la::avdecc::logger::Logger::getInstance().unregisterObserver(this);
	}

	MOCK_METHOD(void, onLogWarn, (), (const noexcept));

private:
	virtual void onLogItem(la::avdecc::logger::Level const level, la::avdecc::logger::LogItem const* const item) noexcept override
	{
		std::cout << "[" << la::avdecc::logger::Logger::getInstance().levelToString(level) << "] " << item->getMessage() << std::endl;
		if (level == la::avdecc::logger::Level::Warn)
		{
			onLogWarn();
		}
	}
};
} // namespace

static auto constexpr DefaultExecutorName = "avdecc::protocol::PI";

TEST(ControlledEntity, VirtualEntityLoad)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	{
		auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
		auto const [error, message] = controller->loadVirtualEntityFromJson("data/SimpleEntity.json", flags);
		EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
		EXPECT_STREQ("", message.c_str());
	}
	{
		auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
		auto const [error, message] = controller->loadVirtualEntityFromJson("data/Listener_EmptyMappings.json", flags);
		EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
		EXPECT_STREQ("", message.c_str());
	}
	{
		auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
		auto const [error, message] = controller->loadVirtualEntityFromJson("data/RedundantListener_EmptyMappings.json", flags);
		EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
		EXPECT_STREQ("", message.c_str());
	}
	{
		auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
		auto const [error, message] = controller->loadVirtualEntityFromJson("data/RedundantListener_InvertedStreamIndex_EmptyMappings.json", flags);
		EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
		EXPECT_STREQ("", message.c_str());
	}
}

TEST(ControlledEntity, AddInputChannelMappings)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Load entity
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/Listener_EmptyMappings.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 })));
	constexpr auto StreamPort = la::avdecc::entity::model::StreamPortIndex{ 0u };
	auto const Mapping = la::avdecc::entity::model::AudioMapping{ 0, 0, 0, 0 };

	// Add "no" mapping
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{}, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	EXPECT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).empty()) << "Mappings should still be empty";

	// Add one mapping
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	ASSERT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
	EXPECT_TRUE(Mapping == e.getStreamPortInputAudioMappings(StreamPort).at(0));

	// Add same mapping
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	ASSERT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
	EXPECT_TRUE(Mapping == e.getStreamPortInputAudioMappings(StreamPort).at(0));
}

TEST(ControlledEntity, ReplaceInputChannelMappingWithDifferentStream)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Load entity
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/Listener_EmptyMappings.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 })));
	constexpr auto StreamPort = la::avdecc::entity::model::StreamPortIndex{ 0u };
	auto const MappingS0 = la::avdecc::entity::model::AudioMapping{ 0, 0, 0, 0 };
	auto const MappingS1 = la::avdecc::entity::model::AudioMapping{ 1, 0, 0, 0 };

	// Add one mapping from Stream 0
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ MappingS0 }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	ASSERT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
	EXPECT_TRUE(MappingS0 == e.getStreamPortInputAudioMappings(StreamPort).at(0));

	// Add same "conflicting" mapping from Stream 1
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ MappingS1 }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	ASSERT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping (replaced)";
	EXPECT_TRUE(MappingS1 == e.getStreamPortInputAudioMappings(StreamPort).at(0));
}

TEST(ControlledEntity, AddDuplicateInputChannelMapping)
{
	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Load entity
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
	{
		EXPECT_CALL(obs, onLogWarn()).Times(testing::AnyNumber());
		auto const [error, message] = controller->loadVirtualEntityFromJson("data/Listener_EmptyMappings.json", flags);
		EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
		EXPECT_STREQ("", message.c_str());
	}

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 })));
	constexpr auto StreamPort = la::avdecc::entity::model::StreamPortIndex{ 0u };
	auto const Mapping = la::avdecc::entity::model::AudioMapping{ 0, 0, 0, 0 };

	// Add one mapping from Stream 0
	{
		EXPECT_CALL(obs, onLogWarn()).Times(0);
		e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
		ASSERT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
		EXPECT_TRUE(Mapping == e.getStreamPortInputAudioMappings(StreamPort).at(0));
	}

	// Add duplicate mapping
	{
		EXPECT_CALL(obs, onLogWarn()).Times(1);
		e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
		ASSERT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
		EXPECT_TRUE(Mapping == e.getStreamPortInputAudioMappings(StreamPort).at(0));
	}
}

TEST(ControlledEntity, GetInvalidInputMappings)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/Listener_EmptyMappings.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 })));
	constexpr auto StreamPort = la::avdecc::entity::model::StreamPortIndex{ 1u };

	// Add mapping to 2nd stream channel
	auto const Mapping = la::avdecc::entity::model::AudioMapping{ 0, 1, 0, 0 };
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);

	// Calculate invalid mappings for a stream format with a single channel (AAF 48kHz 1ch)
	auto const StreamFormat = la::avdecc::entity::model::StreamFormat(0x0205022000406000);
	auto const invalid = e.getStreamPortInputInvalidAudioMappingsForStreamFormat(0, StreamFormat);
	EXPECT_EQ(1u, invalid.size()) << "Exactly one stream port should have invalid mappings";
	ASSERT_EQ(1u, invalid.count(StreamPort)) << "Stream port with invalid mappings not in result";
	ASSERT_EQ(1u, invalid.at(StreamPort).size()) << "There should be exactly one invalid mapping";
	EXPECT_EQ(Mapping, invalid.at(StreamPort)[0]);
}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
TEST(ControlledEntity, AddRedundantInputChannelMappings)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Load entity
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/RedundantListener_InvertedStreamIndex_EmptyMappings.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 })));
	constexpr auto StreamPort = la::avdecc::entity::model::StreamPortIndex{ 0u };
	auto const Mapping = la::avdecc::entity::model::AudioMapping{ 0, 0, 0, 0 }; // This is actually the mapping for the Secondary Stream
	auto const RedundantMapping = la::avdecc::entity::model::AudioMapping{ 1, 0, 0, 0 }; // And this for the Primary Stream

	// Add "no" mapping
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{}, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	EXPECT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).empty()) << "Mappings should still be empty";

	// Add one mapping
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	ASSERT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
	EXPECT_TRUE(Mapping == e.getStreamPortInputAudioMappings(StreamPort).at(0));

	// Add same mapping
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	ASSERT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
	EXPECT_TRUE(Mapping == e.getStreamPortInputAudioMappings(StreamPort).at(0));

	// Add redundancy mapping
	e.addStreamPortInputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ RedundantMapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	ASSERT_TRUE(e.getStreamPortInputAudioMappings(StreamPort).size() == 2) << "Mappings should have changed";
	EXPECT_TRUE(Mapping == e.getStreamPortInputAudioMappings(StreamPort).at(0));
	EXPECT_TRUE(RedundantMapping == e.getStreamPortInputAudioMappings(StreamPort).at(1));
	ASSERT_TRUE(e.getStreamPortInputNonRedundantAudioMappings(StreamPort).size() == 1) << "NonRedundantMappings should not have changed though";
	EXPECT_TRUE(RedundantMapping == e.getStreamPortInputNonRedundantAudioMappings(StreamPort).at(0)) << "NonRedundantMappings should return the mappings for the Primary Stream";
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

TEST(ControlledEntity, AddOutputChannelMappings)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Load entity
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/Talker_EmptyMappings.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(la::avdecc::UniqueIdentifier{ 0x001B92FFFF000002 })));
	constexpr auto StreamPort = la::avdecc::entity::model::StreamPortIndex{ 0u };
	auto const Mapping = la::avdecc::entity::model::AudioMapping{ 0, 0, 0, 0 };

	// Add "no" mapping
	e.addStreamPortOutputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{}, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	EXPECT_TRUE(e.getStreamPortOutputAudioMappings(StreamPort).empty()) << "Mappings should still be empty";

	// Add one mapping
	e.addStreamPortOutputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	ASSERT_TRUE(e.getStreamPortOutputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
	EXPECT_TRUE(Mapping == e.getStreamPortOutputAudioMappings(StreamPort).at(0));

	// Add same mapping
	e.addStreamPortOutputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	ASSERT_TRUE(e.getStreamPortOutputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
	EXPECT_TRUE(Mapping == e.getStreamPortOutputAudioMappings(StreamPort).at(0));
}

TEST(ControlledEntity, ReplaceOutputChannelMappingWithDifferentStream)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Load entity
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/Talker_EmptyMappings.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(la::avdecc::UniqueIdentifier{ 0x001B92FFFF000002 })));
	constexpr auto StreamPort = la::avdecc::entity::model::StreamPortIndex{ 0u };
	auto const MappingS0 = la::avdecc::entity::model::AudioMapping{ 0, 0, 0, 0 };
	auto const MappingS1 = la::avdecc::entity::model::AudioMapping{ 0, 0, 1, 0 };

	// Add one mapping from Cluster 0 Channel 0 to Stream 0 Channel 0
	e.addStreamPortOutputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ MappingS0 }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	ASSERT_TRUE(e.getStreamPortOutputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
	EXPECT_TRUE(MappingS0 == e.getStreamPortOutputAudioMappings(StreamPort).at(0));

	// Add "conflicting" mapping from Cluster 0 Channel 1 to Stream 0 Channel 0
	e.addStreamPortOutputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ MappingS1 }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	ASSERT_TRUE(e.getStreamPortOutputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping (replaced)";
	EXPECT_TRUE(MappingS1 == e.getStreamPortOutputAudioMappings(StreamPort).at(0));
}

TEST(ControlledEntity, AddDuplicateOutputChannelMapping)
{
	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);


	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Load entity
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/Talker_EmptyMappings.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(la::avdecc::UniqueIdentifier{ 0x001B92FFFF000002 })));
	constexpr auto StreamPort = la::avdecc::entity::model::StreamPortIndex{ 0u };
	auto const Mapping = la::avdecc::entity::model::AudioMapping{ 0, 0, 0, 0 };

	// Add one mapping from Stream 0
	{
		EXPECT_CALL(obs, onLogWarn()).Times(0);
		e.addStreamPortOutputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
		ASSERT_TRUE(e.getStreamPortOutputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
		EXPECT_TRUE(Mapping == e.getStreamPortOutputAudioMappings(StreamPort).at(0));
	}

	// Add duplicate mapping
	{
		EXPECT_CALL(obs, onLogWarn()).Times(1);
		e.addStreamPortOutputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ Mapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
		ASSERT_TRUE(e.getStreamPortOutputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
		EXPECT_TRUE(Mapping == e.getStreamPortOutputAudioMappings(StreamPort).at(0));
	}
}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
TEST(ControlledEntity, AddRedundantOutputChannelMappings)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Load entity
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/Talker_EmptyMappings.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(la::avdecc::UniqueIdentifier{ 0x001B92FFFF000002 })));
	constexpr auto StreamPort = la::avdecc::entity::model::StreamPortIndex{ 0u };
	auto const PrimaryMapping = la::avdecc::entity::model::AudioMapping{ 3, 0, 0, 0 };
	auto const SecondaryMapping = la::avdecc::entity::model::AudioMapping{ 4, 0, 0, 0 };

	// Add "no" mapping
	e.addStreamPortOutputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{}, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	EXPECT_TRUE(e.getStreamPortOutputAudioMappings(StreamPort).empty()) << "Mappings should still be empty";

	// Add one mapping
	e.addStreamPortOutputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ SecondaryMapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	ASSERT_TRUE(e.getStreamPortOutputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
	EXPECT_TRUE(SecondaryMapping == e.getStreamPortOutputAudioMappings(StreamPort).at(0));

	// Add same mapping
	e.addStreamPortOutputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ SecondaryMapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	ASSERT_TRUE(e.getStreamPortOutputAudioMappings(StreamPort).size() == 1) << "Mappings should have one mapping";
	EXPECT_TRUE(SecondaryMapping == e.getStreamPortOutputAudioMappings(StreamPort).at(0));

	// Add redundancy mapping
	e.addStreamPortOutputAudioMappings(StreamPort, la::avdecc::entity::model::AudioMappings{ PrimaryMapping }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	ASSERT_TRUE(e.getStreamPortOutputAudioMappings(StreamPort).size() == 2) << "Mappings should have changed";
	EXPECT_TRUE(SecondaryMapping == e.getStreamPortOutputAudioMappings(StreamPort).at(0));
	EXPECT_TRUE(PrimaryMapping == e.getStreamPortOutputAudioMappings(StreamPort).at(1));
	ASSERT_TRUE(e.getStreamPortOutputNonRedundantAudioMappings(StreamPort).size() == 1) << "NonRedundantMappings should not have changed though";
	EXPECT_TRUE(PrimaryMapping == e.getStreamPortOutputNonRedundantAudioMappings(StreamPort).at(0)) << "NonRedundantMappings should return the mappings for the Primary Stream";
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

TEST(ControlledEntity, VisitorValidation)
{
	class Visitor : public la::avdecc::controller::model::EntityModelVisitor
	{
	public:
		Visitor() noexcept = default;

	private:
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::EntityNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Entity, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::EntityNode const* const parent, la::avdecc::controller::model::ConfigurationNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Entity, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::AudioUnitNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::AudioUnit, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::StreamInputNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::StreamInput, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::StreamOutputNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::StreamOutput, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::JackInputNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::JackInput, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::JackOutputNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::JackOutput, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::JackNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandParent->descriptorType);
			EXPECT_TRUE((parent->descriptorType == la::avdecc::entity::model::DescriptorType::JackInput || parent->descriptorType == la::avdecc::entity::model::DescriptorType::JackOutput));
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Control, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::AvbInterfaceNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::AvbInterface, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::ClockSourceNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::ClockSource, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::MemoryObjectNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::MemoryObject, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::LocaleNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Locale, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::LocaleNode const* const parent, la::avdecc::controller::model::StringsNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Locale, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Strings, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::AudioUnitNode const* const parent, la::avdecc::controller::model::StreamPortInputNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::AudioUnit, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::StreamPortInput, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::AudioUnitNode const* const parent, la::avdecc::controller::model::StreamPortOutputNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::AudioUnit, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::StreamPortOutput, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::AudioUnitNode const* const grandParent, la::avdecc::controller::model::StreamPortNode const* const parent, la::avdecc::controller::model::AudioClusterNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandGrandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::AudioUnit, grandParent->descriptorType);
			EXPECT_TRUE((parent->descriptorType == la::avdecc::entity::model::DescriptorType::StreamPortInput || parent->descriptorType == la::avdecc::entity::model::DescriptorType::StreamPortOutput));
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::AudioCluster, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::AudioUnitNode const* const grandParent, la::avdecc::controller::model::StreamPortNode const* const parent, la::avdecc::controller::model::AudioMapNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandGrandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::AudioUnit, grandParent->descriptorType);
			EXPECT_TRUE((parent->descriptorType == la::avdecc::entity::model::DescriptorType::StreamPortInput || parent->descriptorType == la::avdecc::entity::model::DescriptorType::StreamPortOutput));
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::AudioMap, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::AudioUnitNode const* const grandParent, la::avdecc::controller::model::StreamPortNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandGrandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::AudioUnit, grandParent->descriptorType);
			EXPECT_TRUE((parent->descriptorType == la::avdecc::entity::model::DescriptorType::StreamPortInput || parent->descriptorType == la::avdecc::entity::model::DescriptorType::StreamPortOutput));
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Control, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::AudioUnitNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::AudioUnit, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Control, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Control, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::ClockDomainNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::ClockDomain, node.descriptorType);
		}
		// Virtual parenting to show ClockSourceNode which have the specified ClockDomainNode as parent
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::ClockDomainNode const* const parent, la::avdecc::controller::model::ClockSourceNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::ClockDomain, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::ClockSource, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::TimingNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Timing, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::PtpInstanceNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::PtpInstance, node.descriptorType);
		}
		// Virtual parenting to show PtpInstanceNode which have the specified TimingNode as parent
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::TimingNode const* const parent, la::avdecc::controller::model::PtpInstanceNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Timing, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::PtpInstance, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::PtpInstanceNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::PtpInstance, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Control, node.descriptorType);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::PtpInstanceNode const* const parent, la::avdecc::controller::model::PtpPortNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::PtpInstance, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::PtpPort, node.descriptorType);
		}
		// Virtual parenting to show ControlNode which have the specified TimingNode as grandParent and PtpInstanceNode as parent
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::TimingNode const* const grandParent, la::avdecc::controller::model::PtpInstanceNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandGrandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Timing, grandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::PtpInstance, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Control, node.descriptorType);
		}
		// Virtual parenting to show PtpPortNode which have the specified TimingNode as grandParent and PtpInstanceNode as parent
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::TimingNode const* const grandParent, la::avdecc::controller::model::PtpInstanceNode const* const parent, la::avdecc::controller::model::PtpPortNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandGrandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Timing, grandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::PtpInstance, parent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::PtpPort, node.descriptorType);
		}
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::RedundantStreamInputNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::RedundantStreamOutputNode const& /*node*/) noexcept override {}
		// Virtual parenting to show StreamInputNode which have the specified RedundantStreamNode as parent
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::RedundantStreamNode const* const /*parent*/, la::avdecc::controller::model::StreamInputNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::StreamInput, node.descriptorType);
		}
		// Virtual parenting to show StreamOutputNode which have the specified RedundantStreamNode as parent
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::RedundantStreamNode const* const /*parent*/, la::avdecc::controller::model::StreamOutputNode const& node) noexcept override
		{
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::Configuration, grandParent->descriptorType);
			EXPECT_EQ(la::avdecc::entity::model::DescriptorType::StreamOutput, node.descriptorType);
		}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
	};

	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Load entity
	auto const [error, message, entity] = la::avdecc::controller::Controller::deserializeControlledEntityFromJson("data/TalkerListener.json", flags);
	ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto visitor = Visitor{};
	entity->accept(&visitor);
}

namespace
{
class JobPusherObserver : public la::avdecc::controller::Controller::DefaultedObserver
{
public:
	JobPusherObserver(la::avdecc::ExecutorManager::ExecutorWrapper* executor)
		: _executor(executor)
	{
	}
	virtual void onEntityOnline(la::avdecc::controller::Controller const* const /* controller */, la::avdecc::controller::ControlledEntity const* const /* entity*/) noexcept override
	{
		_executor->pushJob(
			[this]()
			{
			});
	}

	virtual void onEntityOffline(la::avdecc::controller::Controller const* const /* controller */, la::avdecc::controller::ControlledEntity const* const /* entity */) noexcept override
	{
		_executor->pushJob(
			[this]()
			{
			});
	}

private:
	la::avdecc::ExecutorManager::ExecutorWrapper* _executor;
};
} // namespace

TEST(ControllerEntity, PushJobToExecutorWhileLoadVirtualEntities)
{
	auto const executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(DefaultExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(DefaultExecutorName, la::avdecc::utils::ThreadPriority::Highest));
	auto jobPusherObserver = JobPusherObserver(executorWrapper.get());
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, DefaultExecutorName, nullptr);
	controller->registerObserver(&jobPusherObserver);

	// Verify if load does not dead lock
	auto const future = std::async(std::launch::async,
		[this, &controller, flags]()
		{
			auto const [error, message] = controller->loadVirtualEntityFromJson("data/SimpleEntity.json", flags);
			EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
			EXPECT_STREQ("", message.c_str());
		});

	auto const status = future.wait_for(std::chrono::seconds(4));
	ASSERT_NE(std::future_status::timeout, status) << "dead lock!";
}

TEST(ControllerEntity, PushJobToExecutorWhileUnloadVirtualEntities)
{
	auto const executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(DefaultExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(DefaultExecutorName, la::avdecc::utils::ThreadPriority::Highest));
	auto jobPusherObserver = JobPusherObserver(executorWrapper.get());
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, DefaultExecutorName, nullptr);
	controller->registerObserver(&jobPusherObserver);
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/SimpleEntity.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	// Verify if unload does not dead lock
	auto const future = std::async(std::launch::async,
		[this, &controller, flags]()
		{
			controller->unloadVirtualEntity(la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 });
		});

	auto const status = future.wait_for(std::chrono::seconds(4));
	ASSERT_NE(std::future_status::timeout, status) << "dead lock!";
}

TEST(ControllerEntity, PushJobToExecutorWhileRefreshVirtualEntities)
{
	auto const executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(DefaultExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(DefaultExecutorName, la::avdecc::utils::ThreadPriority::Highest));
	auto jobPusherObserver = JobPusherObserver(executorWrapper.get());
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, DefaultExecutorName, nullptr);
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/SimpleEntity.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());
	controller->registerObserver(&jobPusherObserver);

	// Verify if refresh does not dead lock
	auto const future = std::async(std::launch::async,
		[this, &controller, flags]()
		{
			controller->refreshEntity(la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 });
		});

	auto const status = future.wait_for(std::chrono::seconds(4));
	ASSERT_NE(std::future_status::timeout, status) << "dead lock!";
}
