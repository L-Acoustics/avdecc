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
* @file avdeccController_tests.cpp.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/executor.hpp>
#include <la/avdecc/controller/avdeccController.hpp>
#include <la/avdecc/internals/protocolAemAecpdu.hpp>
#include <la/avdecc/internals/protocolAemPayloadSizes.hpp>
#include <la/avdecc/internals/entityModelControlValuesTraits.hpp>

// Internal API
#include "controller/avdeccControlledEntityImpl.hpp"
#include "controller/avdeccControllerImpl.hpp"
#include "entity/controllerEntityImpl.hpp"
#include "protocolInterface/protocolInterface_virtual.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <thread>
#include <chrono>
#include <future>
#include <vector>
#include <cstdint>

static auto constexpr DefaultExecutorName = "avdecc::protocol::PI";

namespace
{
class LogObserver : public la::avdecc::logger::Logger::Observer
{
public:
	virtual ~LogObserver() noexcept override
	{
		la::avdecc::logger::Logger::getInstance().unregisterObserver(this);
	}

private:
	virtual void onLogItem(la::avdecc::logger::Level const level, la::avdecc::logger::LogItem const* const item) noexcept override
	{
		if (item->getLayer() == la::avdecc::logger::Layer::Serialization)
		{
			auto const* const i = static_cast<la::avdecc::logger::LogItemSerialization const*>(item);
			std::cout << "[" << la::avdecc::logger::Logger::getInstance().levelToString(level) << "] [" << la::networkInterface::NetworkInterfaceHelper::macAddressToString(i->getSource(), true) << "] " << i->getMessage() << std::endl;
		}
		else
		{
			std::cout << "[" << la::avdecc::logger::Logger::getInstance().levelToString(level) << "] " << item->getMessage() << std::endl;
		}
	}
};

class EntityModelVisitor : public la::avdecc::controller::model::EntityModelVisitor
{
public:
	EntityModelVisitor() = default;

	std::string const& getSerializedModel() const noexcept
	{
		return _serializedModel;
	}

private:
	// Private methods
	void serializeNode(la::avdecc::controller::model::Node const* const node) noexcept
	{
		if (node == nullptr)
			_serializedModel += "nullptr,";
		else
			_serializedModel += "pdt" + std::to_string(la::avdecc::utils::to_integral(node->descriptorType)) + ",";
	}
	void serializeNode(la::avdecc::controller::model::EntityModelNode const& node) noexcept
	{
		_serializedModel += "dt" + std::to_string(la::avdecc::utils::to_integral(node.descriptorType)) + ",";
		_serializedModel += "di" + std::to_string(node.descriptorIndex) + ",";
	}
	void serializeNode(la::avdecc::controller::model::VirtualNode const& node) noexcept
	{
		_serializedModel += "dt" + std::to_string(la::avdecc::utils::to_integral(node.descriptorType)) + ",";
		_serializedModel += "vi" + std::to_string(node.virtualIndex) + ",";
	}

	// la::avdecc::controller::model::EntityModelVisitor overrides
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::EntityNode const& node) noexcept override
	{
		serializeNode(nullptr);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::EntityNode const* const parent, la::avdecc::controller::model::ConfigurationNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::AudioUnitNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::StreamInputNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::StreamOutputNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::JackInputNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::JackOutputNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::JackNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::AvbInterfaceNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::ClockSourceNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::MemoryObjectNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::LocaleNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::LocaleNode const* const parent, la::avdecc::controller::model::StringsNode const& node) noexcept override
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::AudioUnitNode const* const parent, la::avdecc::controller::model::StreamPortInputNode const& node) noexcept override
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::AudioUnitNode const* const parent, la::avdecc::controller::model::StreamPortOutputNode const& node) noexcept override
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::AudioUnitNode const* const grandParent, la::avdecc::controller::model::StreamPortNode const* const parent, la::avdecc::controller::model::AudioClusterNode const& node) noexcept override
	{
		serializeNode(grandGrandParent);
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::AudioUnitNode const* const grandParent, la::avdecc::controller::model::StreamPortNode const* const parent, la::avdecc::controller::model::AudioMapNode const& node) noexcept override
	{
		serializeNode(grandGrandParent);
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::AudioUnitNode const* const grandParent, la::avdecc::controller::model::StreamPortNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
	{
		serializeNode(grandGrandParent);
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::AudioUnitNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::ClockDomainNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::ClockDomainNode const* const parent, la::avdecc::controller::model::ClockSourceNode const& node) noexcept override
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::RedundantStreamInputNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
		_serializedModel += "rsi" + std::to_string(node.primaryStreamIndex) + "+";
		for (auto const streamIndex : node.redundantStreams)
		{
			_serializedModel += std::to_string(streamIndex) + "+";
		}
		_serializedModel += ",";
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::RedundantStreamOutputNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
		_serializedModel += "rso" + std::to_string(node.primaryStreamIndex) + "+";
		for (auto const streamIndex : node.redundantStreams)
		{
			_serializedModel += std::to_string(streamIndex) + "+";
		}
		_serializedModel += ",";
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::RedundantStreamNode const* const parent, la::avdecc::controller::model::StreamInputNode const& node) noexcept override
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::RedundantStreamNode const* const parent, la::avdecc::controller::model::StreamOutputNode const& node) noexcept override
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}

#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	std::string _serializedModel{};
};
}; // namespace

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
TEST(Controller, RedundantStreams)
{
#	ifdef ENABLE_AVDECC_STRICT_2018_REDUNDANCY
	// Invalid redundant association: More than 2 streams in the association
	{
		auto sharedLock = std::make_shared<la::avdecc::controller::ControlledEntityImpl::LockInformation>();
		auto const commonInformation{ la::avdecc::entity::Entity::CommonInformation{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0u, la::avdecc::entity::TalkerCapabilities{}, 0u, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented }, std::nullopt, std::nullopt } };
		auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ la::networkInterface::MacAddress{}, 31u, 0u, std::nullopt, std::nullopt } };
		auto const e{ la::avdecc::entity::Entity{ commonInformation, la::avdecc::entity::Entity::InterfacesInformation{ { la::avdecc::entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } } } };
		la::avdecc::controller::ControlledEntityImpl entity{ e, sharedLock, false };

		entity.setEntityDescriptor(la::avdecc::entity::model::EntityDescriptor{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0, la::avdecc::entity::TalkerCapabilities{}, 0, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{}, 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), std::string("Test entity"), la::avdecc::entity::model::LocalizedStringReference{}, la::avdecc::entity::model::LocalizedStringReference{}, std::string("Test firmware"), std::string("Test group"), std::string("Test serial number"), 1, 0 });
		entity.setConfigurationDescriptor(la::avdecc::entity::model::ConfigurationDescriptor{ std::string("Test configuration"), la::avdecc::entity::model::LocalizedStringReference{}, { { la::avdecc::entity::model::DescriptorType::StreamInput, std::uint16_t{ 4 } } } }, 0);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Test stream 1"), la::avdecc::entity::model::LocalizedStringReference{}, 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 0, 0, {}, {} }, 0, 0);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Secondary stream 2"), la::avdecc::entity::model::LocalizedStringReference{}, 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 0, 0, {}, { 3, 2 } }, 0, 1);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Primary stream 2"), la::avdecc::entity::model::LocalizedStringReference{}, 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 0, 0, {}, { 1, 3 } }, 0, 2);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Tertiary stream 2"), la::avdecc::entity::model::LocalizedStringReference{}, 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 0, 0, {}, { 2, 1 } }, 0, 3);
		entity.onEntityFullyLoaded();

		EntityModelVisitor serializer{};
		entity.accept(&serializer);
		auto const serialized = serializer.getSerializedModel();
		EXPECT_STREQ("nullptr,dt0,di0,pdt0,dt1,di0,pdt1,dt5,di0,pdt1,dt5,di1,pdt1,dt5,di2,pdt1,dt5,di3,", serialized.c_str());
	}
#	endif // ENABLE_AVDECC_STRICT_2018_REDUNDANCY

	// Invalid redundant association:
	//{
	//	auto const e{ la::avdecc::entity::Entity{ 0x0102030405060708, la::networkInterface::MacAddress{}, 0x1122334455667788, la::avdecc::entity::EntityCapabilities::AemSupported, 0, la::avdecc::entity::TalkerCapabilities{}, 0, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{}, 0, 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier() } };
	//	la::avdecc::controller::ControlledEntityImpl entity{ e };
	//
	//	entity.setEntityDescriptor(la::avdecc::entity::model::EntityDescriptor{ 0x0102030405060708 , 0x1122334455667788 , la::avdecc::entity::EntityCapabilities::AemSupported, 0, la::avdecc::entity::TalkerCapabilities{}, 0, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{}, 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier() , std::string("Test entity"), la::avdecc::entity::model::LocalizedStringReference{}, la::avdecc::entity::model::LocalizedStringReference{}, std::string("Test firmware"), std::string("Test group"), std::string("Test serial number"), 1, 0 });
	//	entity.setConfigurationDescriptor(la::avdecc::entity::model::ConfigurationDescriptor{ std::string("Test configuration"), la::avdecc::entity::model::LocalizedStringReference{},{ { la::avdecc::entity::model::DescriptorType::StreamInput, 3 } } }, 0);
	//	entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Test stream 1"), la::avdecc::entity::model::LocalizedStringReference{} , 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 0, 0,{},{} }, 0, 0);
	//	entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Secondary stream 2"), la::avdecc::entity::model::LocalizedStringReference{} , 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 0, 0,{},{ 2 } }, 0, 1);
	//	entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Primary stream 2"), la::avdecc::entity::model::LocalizedStringReference{} , 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 0, 0,{},{ 1 } }, 0, 2);
	//
	//	EntityModelVisitor serializer{};
	//	entity.accept(&serializer);
	//	auto const serialized = serializer.getSerializedModel();
	//	EXPECT_STREQ("nullptr,dt0,di0,pdt0,dt1,di0,pdt1,dt5,di0,pdt1,dt5,di1,pdt1,dt5,di2,", serialized.c_str());
	//}

	// Invalid redundant association: Stream referencing itself
	{
		auto sharedLock = std::make_shared<la::avdecc::controller::ControlledEntityImpl::LockInformation>();
		auto const commonInformation{ la::avdecc::entity::Entity::CommonInformation{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0u, la::avdecc::entity::TalkerCapabilities{}, 0u, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented }, std::nullopt, std::nullopt } };
		auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ la::networkInterface::MacAddress{}, 31u, 0u, std::nullopt, std::nullopt } };
		auto const e{ la::avdecc::entity::Entity{ commonInformation, la::avdecc::entity::Entity::InterfacesInformation{ { la::avdecc::entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } } } };
		la::avdecc::controller::ControlledEntityImpl entity{ e, sharedLock, false };

		entity.setEntityDescriptor(la::avdecc::entity::model::EntityDescriptor{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0, la::avdecc::entity::TalkerCapabilities{}, 0, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{}, 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), std::string("Test entity"), la::avdecc::entity::model::LocalizedStringReference{}, la::avdecc::entity::model::LocalizedStringReference{}, std::string("Test firmware"), std::string("Test group"), std::string("Test serial number"), 1, 0 });
		entity.setConfigurationDescriptor(la::avdecc::entity::model::ConfigurationDescriptor{ std::string("Test configuration"), la::avdecc::entity::model::LocalizedStringReference{}, { { la::avdecc::entity::model::DescriptorType::StreamInput, std::uint16_t{ 2 } } } }, 0);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Primary stream 1"), la::avdecc::entity::model::LocalizedStringReference{}, 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 0, 0, {}, { 0 } }, 0, 0);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Secondary stream 1"), la::avdecc::entity::model::LocalizedStringReference{}, 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 1, 0, {}, { 0 } }, 0, 1);
		entity.onEntityFullyLoaded();

		EntityModelVisitor serializer{};
		entity.accept(&serializer);
		auto const serialized = serializer.getSerializedModel();
		EXPECT_STREQ("nullptr,dt0,di0,pdt0,dt1,di0,pdt1,dt5,di0,pdt1,dt5,di1,", serialized.c_str());
	}

	// Valid redundant association (primary stream declared first)
	{
		auto sharedLock = std::make_shared<la::avdecc::controller::ControlledEntityImpl::LockInformation>();
		auto const commonInformation{ la::avdecc::entity::Entity::CommonInformation{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0u, la::avdecc::entity::TalkerCapabilities{}, 0u, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented }, std::nullopt, std::nullopt } };
		auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ la::networkInterface::MacAddress{}, 31u, 0u, std::nullopt, std::nullopt } };
		auto const e{ la::avdecc::entity::Entity{ commonInformation, la::avdecc::entity::Entity::InterfacesInformation{ { la::avdecc::entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } } } };
		la::avdecc::controller::ControlledEntityImpl entity{ e, sharedLock, false };

		entity.setEntityDescriptor(la::avdecc::entity::model::EntityDescriptor{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0, la::avdecc::entity::TalkerCapabilities{}, 0, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{}, 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), std::string("Test entity"), la::avdecc::entity::model::LocalizedStringReference{}, la::avdecc::entity::model::LocalizedStringReference{}, std::string("Test firmware"), std::string("Test group"), std::string("Test serial number"), 1, 0 });
		entity.setConfigurationDescriptor(la::avdecc::entity::model::ConfigurationDescriptor{ std::string("Test configuration"), la::avdecc::entity::model::LocalizedStringReference{}, { { la::avdecc::entity::model::DescriptorType::StreamInput, std::uint16_t{ 2 } } } }, 0);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Primary stream 1"), la::avdecc::entity::model::LocalizedStringReference{}, 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 0, 0, {}, { 1 } }, 0, 0);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Secondary stream 1"), la::avdecc::entity::model::LocalizedStringReference{}, 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 1, 0, {}, { 0 } }, 0, 1);
		entity.onEntityFullyLoaded();

		EntityModelVisitor serializer{};
		entity.accept(&serializer);
		auto const serialized = serializer.getSerializedModel();
		EXPECT_STREQ("nullptr,dt0,di0,pdt0,dt1,di0,pdt1,dt5,di0,pdt1,dt5,di1,pdt1,dt5,vi0,rsi0+0+1+,pdt1,pdt5,dt5,di0,pdt1,pdt5,dt5,di1,", serialized.c_str());
	}

	// Valid redundant association (secondary stream declared first)
	{
		auto sharedLock = std::make_shared<la::avdecc::controller::ControlledEntityImpl::LockInformation>();
		auto const commonInformation{ la::avdecc::entity::Entity::CommonInformation{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0u, la::avdecc::entity::TalkerCapabilities{}, 0u, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented }, std::nullopt, std::nullopt } };
		auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ la::networkInterface::MacAddress{}, 31u, 0u, std::nullopt, std::nullopt } };
		auto const e{ la::avdecc::entity::Entity{ commonInformation, la::avdecc::entity::Entity::InterfacesInformation{ { la::avdecc::entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } } } };
		la::avdecc::controller::ControlledEntityImpl entity{ e, sharedLock, false };

		entity.setEntityDescriptor(la::avdecc::entity::model::EntityDescriptor{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0, la::avdecc::entity::TalkerCapabilities{}, 0, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{}, 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), std::string("Test entity"), la::avdecc::entity::model::LocalizedStringReference{}, la::avdecc::entity::model::LocalizedStringReference{}, std::string("Test firmware"), std::string("Test group"), std::string("Test serial number"), 1, 0 });
		entity.setConfigurationDescriptor(la::avdecc::entity::model::ConfigurationDescriptor{ std::string("Test configuration"), la::avdecc::entity::model::LocalizedStringReference{}, { { la::avdecc::entity::model::DescriptorType::StreamInput, std::uint16_t{ 2 } } } }, 0);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Secondary stream 1"), la::avdecc::entity::model::LocalizedStringReference{}, 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 1, 0, {}, { 1 } }, 0, 0);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Primary stream 1"), la::avdecc::entity::model::LocalizedStringReference{}, 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 0, 0, {}, { 0 } }, 0, 1);
		entity.onEntityFullyLoaded();

		EntityModelVisitor serializer{};
		entity.accept(&serializer);
		auto const serialized = serializer.getSerializedModel();
		EXPECT_STREQ("nullptr,dt0,di0,pdt0,dt1,di0,pdt1,dt5,di0,pdt1,dt5,di1,pdt1,dt5,vi0,rsi1+0+1+,pdt1,pdt5,dt5,di0,pdt1,pdt5,dt5,di1,", serialized.c_str());
	}

	// Valid redundant association (single stream declared as well as redundant pair)
	{
		auto sharedLock = std::make_shared<la::avdecc::controller::ControlledEntityImpl::LockInformation>();
		auto const commonInformation{ la::avdecc::entity::Entity::CommonInformation{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0u, la::avdecc::entity::TalkerCapabilities{}, 0u, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented }, std::nullopt, std::nullopt } };
		auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ la::networkInterface::MacAddress{}, 31u, 0u, std::nullopt, std::nullopt } };
		auto const e{ la::avdecc::entity::Entity{ commonInformation, la::avdecc::entity::Entity::InterfacesInformation{ { la::avdecc::entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } } } };
		la::avdecc::controller::ControlledEntityImpl entity{ e, sharedLock, false };

		entity.setEntityDescriptor(la::avdecc::entity::model::EntityDescriptor{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0, la::avdecc::entity::TalkerCapabilities{}, 0, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{}, 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), std::string("Test entity"), la::avdecc::entity::model::LocalizedStringReference{}, la::avdecc::entity::model::LocalizedStringReference{}, std::string("Test firmware"), std::string("Test group"), std::string("Test serial number"), 1, 0 });
		entity.setConfigurationDescriptor(la::avdecc::entity::model::ConfigurationDescriptor{ std::string("Test configuration"), la::avdecc::entity::model::LocalizedStringReference{}, { { la::avdecc::entity::model::DescriptorType::StreamInput, std::uint16_t{ 3 } } } }, 0);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Test stream 1"), la::avdecc::entity::model::LocalizedStringReference{}, 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 0, 0, {}, {} }, 0, 0);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Secondary stream 2"), la::avdecc::entity::model::LocalizedStringReference{}, 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 1, 0, {}, { 2 } }, 0, 1);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Primary stream 2"), la::avdecc::entity::model::LocalizedStringReference{}, 0, la::avdecc::entity::StreamFlags{}, la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), 0, 0, 0, {}, { 1 } }, 0, 2);
		entity.onEntityFullyLoaded();

		EntityModelVisitor serializer{};
		entity.accept(&serializer);
		auto const serialized = serializer.getSerializedModel();
		EXPECT_STREQ("nullptr,dt0,di0,pdt0,dt1,di0,pdt1,dt5,di0,pdt1,dt5,di1,pdt1,dt5,di2,pdt1,dt5,vi0,rsi2+1+2+,pdt1,pdt5,dt5,di1,pdt1,pdt5,dt5,di2,", serialized.c_str());
	}
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

TEST(Controller, DestroyWhileSending)
{
	static std::promise<void> commandResultPromise{};
	{
		auto pi = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, DefaultExecutorName));
		auto const commonInformation{ la::avdecc::entity::Entity::CommonInformation{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0u, la::avdecc::entity::TalkerCapabilities{}, 0u, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented }, std::nullopt, std::nullopt } };
		auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ la::networkInterface::MacAddress{ { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, 31u, 0u, std::nullopt, std::nullopt } };
		auto controllerGuard = std::make_unique<la::avdecc::entity::LocalEntityGuard<la::avdecc::entity::ControllerEntityImpl>>(pi.get(), commonInformation, la::avdecc::entity::Entity::InterfacesInformation{ { la::avdecc::entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } }, nullptr, nullptr);
		auto* const controller = static_cast<la::avdecc::entity::ControllerEntity*>(controllerGuard.get());

		controller->getListenerStreamState({ la::avdecc::UniqueIdentifier{ 0x000102FFFE030405 }, 0u },
			[](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const /*status*/)
			{
				// Wait a little bit so the controllerGuard has time to go out of scope and release
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				commandResultPromise.set_value();
			});
		// Let the ControllerGuard go out of scope for destruction
	}

	// Wait for the handler to complete
	auto status = commandResultPromise.get_future().wait_for(std::chrono::seconds(1));
	ASSERT_NE(std::future_status::timeout, status);
}

TEST(StreamConnectionState, Comparison)
{
	// Not connected
	{
		auto const s1{ la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamInputConnectionInfo::State::NotConnected } };
		auto const s2{ la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{ la::avdecc::UniqueIdentifier{ 0x1 }, 1 }, la::avdecc::entity::model::StreamInputConnectionInfo::State::NotConnected } };
		EXPECT_EQ(s2, s1) << "Talker StreamIdentification ignored when not connected";
	}

	// FastConnecting
	{
		auto const s1{ la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamInputConnectionInfo::State::FastConnecting } };
		auto const s2{ la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{ la::avdecc::UniqueIdentifier{ 0x1 }, 1 }, la::avdecc::entity::model::StreamInputConnectionInfo::State::FastConnecting } };
		EXPECT_NE(s2, s1) << "Talker StreamIdentification not ignored when fast connecting";
	}

	// Connected
	{
		auto const s1{ la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected } };
		auto const s2{ la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{ la::avdecc::UniqueIdentifier{ 0x1 }, 1 }, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected } };
		EXPECT_NE(s2, s1) << "Talker StreamIdentification not ignored when connected";
	}
}

namespace
{
class Controller_F : public ::testing::Test
{
public:
	virtual void SetUp() override
	{
		_controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
	}

	virtual void TearDown() override {}

	la::avdecc::controller::Controller& getController() noexcept
	{
		return *_controller;
	}

private:
	la::avdecc::controller::Controller::UniquePointer _controller{ nullptr, nullptr };
};
} // namespace

TEST_F(Controller_F, VirtualEntityLoad)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	//static std::promise<void> commandResultPromise{};
	{
		auto& controller = getController();
		auto const [error, message] = controller.loadVirtualEntityFromJson("data/SimpleEntity.json", flags);
		EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
		EXPECT_STREQ("", message.c_str());
	}

	// Wait for the handler to complete
	//auto status = commandResultPromise.get_future().wait_for(std::chrono::seconds(1));
	//ASSERT_NE(std::future_status::timeout, status);
}

TEST_F(Controller_F, VirtualEntityLoadUTF8)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	//static std::promise<void> commandResultPromise{};
	{
		auto& controller = getController();
		auto const [error, message] = controller.loadVirtualEntityFromJson("data/テスト.json", flags);
		EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
		EXPECT_STREQ("", message.c_str());
	}

	// Wait for the handler to complete
	//auto status = commandResultPromise.get_future().wait_for(std::chrono::seconds(1));
	//ASSERT_NE(std::future_status::timeout, status);
}

/*
 * TESTING https://github.com/L-Acoustics/avdecc/issues/84
 * Callback returns BadArguments if passed too many mappings
 */
TEST_F(Controller_F, BadArgumentsIfTooManyMappingsPassed)
{
	auto constexpr MaxMappingsInAddRemove = (la::avdecc::protocol::AemAecpdu::MaximumSendPayloadBufferLength - la::avdecc::protocol::aemPayload::AecpAemAddAudioMappingsCommandPayloadMinSize) / 8;

	// In order to trigger an exception we have to pass more than MaxMappingsInAddRemove mappings (63 for standard IEEE1722.1 spec)
	auto validMappings = la::avdecc::entity::model::AudioMappings{};
	for (auto i = 1u; i <= MaxMappingsInAddRemove; ++i)
	{
		validMappings.push_back({});
	}
	auto invalidMappings = validMappings;
	invalidMappings.push_back({});

	auto& controller = getController();

	// Valid AddStreamPortInputAudioMappings
	{
		static auto handlerPromise = std::promise<la::avdecc::entity::LocalEntity::AemCommandStatus>{};
		controller.addStreamPortInputAudioMappings({}, {}, validMappings,
			[](auto const* const /*entity*/, auto const status)
			{
				handlerPromise.set_value(status);
			});

		auto fut = handlerPromise.get_future();
		auto status = fut.wait_for(std::chrono::seconds(1));
		ASSERT_NE(std::future_status::timeout, status) << "Handler not called";
		EXPECT_EQ(la::avdecc::entity::LocalEntity::AemCommandStatus::UnknownEntity, fut.get());
	}

	// Invalid AddStreamPortInputAudioMappings
	{
		static auto handlerPromise = std::promise<la::avdecc::entity::LocalEntity::AemCommandStatus>{};
		controller.addStreamPortInputAudioMappings({}, {}, invalidMappings,
			[](auto const* const /*entity*/, auto const status)
			{
				handlerPromise.set_value(status);
			});

		auto fut = handlerPromise.get_future();
		auto status = fut.wait_for(std::chrono::seconds(1));
		ASSERT_NE(std::future_status::timeout, status) << "Handler not called";
		EXPECT_EQ(la::avdecc::entity::LocalEntity::AemCommandStatus::BadArguments, fut.get());
	}

	// Valid AddStreamPortOutputAudioMappings
	{
		static auto handlerPromise = std::promise<la::avdecc::entity::LocalEntity::AemCommandStatus>{};
		controller.addStreamPortOutputAudioMappings({}, {}, validMappings,
			[](auto const* const /*entity*/, auto const status)
			{
				handlerPromise.set_value(status);
			});

		auto fut = handlerPromise.get_future();
		auto status = fut.wait_for(std::chrono::seconds(1));
		ASSERT_NE(std::future_status::timeout, status) << "Handler not called";
		EXPECT_EQ(la::avdecc::entity::LocalEntity::AemCommandStatus::UnknownEntity, fut.get());
	}

	// Invalid AddStreamPortOutputAudioMappings
	{
		static auto handlerPromise = std::promise<la::avdecc::entity::LocalEntity::AemCommandStatus>{};
		controller.addStreamPortOutputAudioMappings({}, {}, invalidMappings,
			[](auto const* const /*entity*/, auto const status)
			{
				handlerPromise.set_value(status);
			});

		auto fut = handlerPromise.get_future();
		auto status = fut.wait_for(std::chrono::seconds(1));
		ASSERT_NE(std::future_status::timeout, status) << "Handler not called";
		EXPECT_EQ(la::avdecc::entity::LocalEntity::AemCommandStatus::BadArguments, fut.get());
	}

	// Valid RemoveStreamPortInputAudioMappings
	{
		static auto handlerPromise = std::promise<la::avdecc::entity::LocalEntity::AemCommandStatus>{};
		controller.removeStreamPortInputAudioMappings({}, {}, validMappings,
			[](auto const* const /*entity*/, auto const status)
			{
				handlerPromise.set_value(status);
			});

		auto fut = handlerPromise.get_future();
		auto status = fut.wait_for(std::chrono::seconds(1));
		ASSERT_NE(std::future_status::timeout, status) << "Handler not called";
		EXPECT_EQ(la::avdecc::entity::LocalEntity::AemCommandStatus::UnknownEntity, fut.get());
	}

	// Invalid RemoveStreamPortInputAudioMappings
	{
		static auto handlerPromise = std::promise<la::avdecc::entity::LocalEntity::AemCommandStatus>{};
		controller.removeStreamPortInputAudioMappings({}, {}, invalidMappings,
			[](auto const* const /*entity*/, auto const status)
			{
				handlerPromise.set_value(status);
			});

		auto fut = handlerPromise.get_future();
		auto status = fut.wait_for(std::chrono::seconds(1));
		ASSERT_NE(std::future_status::timeout, status) << "Handler not called";
		EXPECT_EQ(la::avdecc::entity::LocalEntity::AemCommandStatus::BadArguments, fut.get());
	}

	// Valid RemoveStreamPortOutputAudioMappings
	{
		static auto handlerPromise = std::promise<la::avdecc::entity::LocalEntity::AemCommandStatus>{};
		controller.removeStreamPortOutputAudioMappings({}, {}, validMappings,
			[](auto const* const /*entity*/, auto const status)
			{
				handlerPromise.set_value(status);
			});

		auto fut = handlerPromise.get_future();
		auto status = fut.wait_for(std::chrono::seconds(1));
		ASSERT_NE(std::future_status::timeout, status) << "Handler not called";
		EXPECT_EQ(la::avdecc::entity::LocalEntity::AemCommandStatus::UnknownEntity, fut.get());
	}

	// Invalid RemoveStreamPortOutputAudioMappings
	{
		static auto handlerPromise = std::promise<la::avdecc::entity::LocalEntity::AemCommandStatus>{};
		controller.removeStreamPortOutputAudioMappings({}, {}, invalidMappings,
			[](auto const* const /*entity*/, auto const status)
			{
				handlerPromise.set_value(status);
			});

		auto fut = handlerPromise.get_future();
		auto status = fut.wait_for(std::chrono::seconds(1));
		ASSERT_NE(std::future_status::timeout, status) << "Handler not called";
		EXPECT_EQ(la::avdecc::entity::LocalEntity::AemCommandStatus::BadArguments, fut.get());
	}
}

/*
 * TESTING https://github.com/L-Acoustics/avdecc/issues/85
 * Controller should properly handle cable redundancy
 */
TEST(Controller, AdpduFromSameDeviceDifferentInterfaces)
{
	// Create a controller
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);

	auto const sendAdpAvailable = [gPTP = controller->getControllerEID()](auto const& entityID, auto const interfaceIndex)
	{
		auto intfc = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { static_cast<la::networkInterface::MacAddress::value_type>(interfaceIndex), 0x06, 0x05, 0x04, 0x03, 0x02 } }, DefaultExecutorName));

		// Build adpdu frame
		auto adpdu = la::avdecc::protocol::Adpdu{};
		// Set Ether2 fields
		adpdu.setSrcAddress(intfc->getMacAddress());
		adpdu.setDestAddress(la::avdecc::protocol::Adpdu::Multicast_Mac_Address);
		// Set ADP fields
		adpdu.setMessageType(la::avdecc::protocol::AdpMessageType::EntityAvailable);
		adpdu.setValidTime(2);
		adpdu.setEntityID(entityID);
		adpdu.setEntityModelID(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier());
		adpdu.setEntityCapabilities(la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemInterfaceIndexValid, la::avdecc::entity::EntityCapability::GptpSupported });
		adpdu.setTalkerStreamSources(0);
		adpdu.setTalkerCapabilities({});
		adpdu.setListenerStreamSinks(0);
		adpdu.setListenerCapabilities({});
		adpdu.setControllerCapabilities(la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented });
		adpdu.setAvailableIndex(1);
		adpdu.setGptpGrandmasterID(gPTP);
		adpdu.setGptpDomainNumber(0);
		adpdu.setIdentifyControlIndex(0);
		adpdu.setInterfaceIndex(interfaceIndex);
		adpdu.setAssociationID(la::avdecc::UniqueIdentifier{});

		// Send the adp message
		intfc->sendAdpMessage(adpdu);

		// Wait for the message to actually be sent (destroying the protocol interface won't flush pending messages, not at this date with the current code)
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	};

	// Simulate ADP messages from the 2 interfaces of the same Entity
	constexpr auto EntityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	sendAdpAvailable(EntityID, la::avdecc::entity::model::AvbInterfaceIndex{ 0 });
	sendAdpAvailable(EntityID, la::avdecc::entity::model::AvbInterfaceIndex{ 1 });

	{
		auto const entity = controller->getControlledEntityGuard(EntityID);
		ASSERT_TRUE(!!entity);
		EXPECT_EQ(2u, entity->getEntity().getInterfacesInformation().size());
	}
}

/*
 * TESTING https://github.com/L-Acoustics/avdecc/issues/86
 * Controller should properly handle cable redundancy
 */
TEST(Controller, AdpRedundantInterfaceNotifications)
{
	static auto s_CallOrder = std::vector<std::uint8_t>{};

	class Obs final : public la::avdecc::controller::Controller::DefaultedObserver
	{
	private:
		virtual void onEntityOnline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity) noexcept override
		{
			if (entity->getEntity().getInterfacesInformation().size() == 1)
			{
				s_CallOrder.push_back(std::uint8_t{ 1 });
			}
			else
			{
				s_CallOrder.push_back(std::uint8_t{ 0 });
			}
		}
		virtual void onEntityRedundantInterfaceOnline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::Entity::InterfaceInformation const& /*interfaceInfo*/) noexcept override
		{
			if (avbInterfaceIndex == la::avdecc::entity::model::AvbInterfaceIndex{ 1 })
			{
				s_CallOrder.push_back(std::uint8_t{ 2 });
			}
			else
			{
			}
		}
		virtual void onEntityRedundantInterfaceOffline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex) noexcept override
		{
			if (avbInterfaceIndex == la::avdecc::entity::model::AvbInterfaceIndex{ 1 })
			{
				s_CallOrder.push_back(std::uint8_t{ 3 });
			}
			else
			{
				s_CallOrder.push_back(std::uint8_t{ 0 });
			}
		}
		DECLARE_AVDECC_OBSERVER_GUARD(Obs);
	};

	// Create a controller
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);

	// Add an observer
	auto obs = Obs{};
	controller->registerObserver(&obs);

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

	// Simulate ADP messages from the 2 interfaces of the same Entity
	constexpr auto EntityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	sendAdpAvailable(EntityID, la::avdecc::entity::model::AvbInterfaceIndex{ 0 }, std::uint8_t{ 20 });
	sendAdpAvailable(EntityID, la::avdecc::entity::model::AvbInterfaceIndex{ 1 }, std::uint8_t{ 2 });

	// Should have 2 interfaces
	{
		auto const entity = controller->getControlledEntityGuard(EntityID);
		ASSERT_TRUE(!!entity);
		EXPECT_EQ(2u, entity->getEntity().getInterfacesInformation().size());
	}

	// Wait for the "secondary" interface to timeout
	std::this_thread::sleep_for(std::chrono::seconds(5));

	// Should only have one interface left
	{
		auto const entity = controller->getControlledEntityGuard(EntityID);
		ASSERT_TRUE(!!entity);
		EXPECT_EQ(1u, entity->getEntity().getInterfacesInformation().size());
	}

	// Validate we passed all required events in the correct order
	ASSERT_EQ(3u, s_CallOrder.size());
	auto order = decltype(s_CallOrder)::value_type{ 1u };
	for (auto const val : s_CallOrder)
	{
		EXPECT_EQ(order++, val);
	}
}

TEST(Controller, ValidControlValues)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Load entity
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/SimpleEntity.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 };
	auto constexpr ControlIndex = la::avdecc::entity::model::ControlIndex{ 0u };

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(EntityID)));
	auto& c = static_cast<la::avdecc::controller::ControllerImpl&>(*controller);

	EXPECT_TRUE(e.getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221));

	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	try
	{
		// Get ControlNode
		auto const& controlNode = e.getControlNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, ControlIndex);
		auto const& staticValues = controlNode.staticModel.values;

		ASSERT_EQ(1u, staticValues.size()) << "VirtualEntity should have 1 value in its ControlNode";
		ASSERT_EQ(la::avdecc::entity::model::ControlValueType::Type::ControlLinearUInt8, staticValues.getType()) << "VirtualEntity should have ControlLinearUInt8 type in its ControlNode";
		ASSERT_TRUE(!!staticValues) << "VirtualEntity should have valid values in its ControlNode";
		ASSERT_TRUE(!staticValues.areDynamicValues()) << "VirtualEntity should have static values in its ControlNode";

		// Expect to pass ControlValues validation with a value set to minimum
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResult::Valid, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::uint8_t>>{ { { 0u } } } }));

		// Expect to pass ControlValues validation with a value set to maximum
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResult::Valid, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::uint8_t>>{ { { 255u } } } }));
	}
	catch (la::avdecc::controller::ControlledEntity::Exception const&)
	{
		ASSERT_FALSE(true) << "ControlNode not found";
	}
}

TEST(Controller, InvalidControlValues)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Load entity
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/ControlValueError.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 };

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(EntityID)));
	auto& c = static_cast<la::avdecc::controller::ControllerImpl&>(*controller);

	EXPECT_TRUE(e.getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221));

	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	// Get ControlNode.0 (Type: Identify)
	try
	{
		auto constexpr ControlIndex = la::avdecc::entity::model::ControlIndex{ 0u };
		auto const& controlNode = e.getControlNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, ControlIndex);
		ASSERT_EQ(la::avdecc::utils::to_integral(la::avdecc::entity::model::StandardControlType::Identify), controlNode.staticModel.controlType.getValue()) << "VirtualEntity should have Identify type in its ControlNode";
		auto const& staticValues = controlNode.staticModel.values;

		ASSERT_EQ(1u, staticValues.size()) << "VirtualEntity should have 1 value in its ControlNode";
		ASSERT_EQ(la::avdecc::entity::model::ControlValueType::Type::ControlLinearUInt8, staticValues.getType()) << "VirtualEntity should have ControlLinearUInt8 type in its ControlNode";
		ASSERT_TRUE(!!staticValues) << "VirtualEntity should have valid values in its ControlNode";
		ASSERT_TRUE(!staticValues.areDynamicValues()) << "VirtualEntity should have static values in its ControlNode";

		// Expect to have InvalidValues validation result with non-initialized dynamic values (might be an unknown type of ControlValues)
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResult::InvalidValues, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, {}));

		// Expect to have InvalidValues validation result with static values instead of dynamic values
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResult::InvalidValues, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueStatic<std::uint8_t>>{} }));

		// Expect to have InvalidValues validation result with a different type of dynamic values
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResult::InvalidValues, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::int8_t>>{} }));

		// Expect to have InvalidValues validation result with a different count of values
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResult::InvalidValues, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::uint8_t>>{} }));

		// Expect to have InvalidValues validation result with a value not multiple of Step for LinearValues
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResult::InvalidValues, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::uint8_t>>{ { { 1u } } } }));
	}
	catch (la::avdecc::controller::ControlledEntity::Exception const&)
	{
		ASSERT_FALSE(true) << "ControlNode not found";
	}

	// Get ControlNode.1 (Type: FanStatus)
	try
	{
		auto constexpr ControlIndex = la::avdecc::entity::model::ControlIndex{ 1u };
		auto const& controlNode = e.getControlNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, ControlIndex);
		ASSERT_EQ(la::avdecc::utils::to_integral(la::avdecc::entity::model::StandardControlType::FanStatus), controlNode.staticModel.controlType.getValue()) << "VirtualEntity should have Identify type in its ControlNode";
		auto const& staticValues = controlNode.staticModel.values;

		ASSERT_EQ(1u, staticValues.size()) << "VirtualEntity should have 1 value in its ControlNode";
		ASSERT_EQ(la::avdecc::entity::model::ControlValueType::Type::ControlLinearUInt8, staticValues.getType()) << "VirtualEntity should have ControlLinearUInt8 type in its ControlNode";
		ASSERT_TRUE(!!staticValues) << "VirtualEntity should have valid values in its ControlNode";
		ASSERT_TRUE(!staticValues.areDynamicValues()) << "VirtualEntity should have static values in its ControlNode";

		// Expect to have CurrentValueOutOfRange validation result with a value outside bounds
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResult::CurrentValueOutOfRange, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, controlNode.dynamicModel.values));
	}
	catch (la::avdecc::controller::ControlledEntity::Exception const&)
	{
		ASSERT_FALSE(true) << "ControlNode not found";
	}

	// Get ControlNode.2 (Type: VendorSpecific)
	try
	{
		auto constexpr ControlIndex = la::avdecc::entity::model::ControlIndex{ 2u };
		auto const& controlNode = e.getControlNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, ControlIndex);
		ASSERT_EQ(la::avdecc::UniqueIdentifier{ 0x480BB2FFFED40000 }, controlNode.staticModel.controlType) << "VirtualEntity should have Identify type in its ControlNode";
		auto const& staticValues = controlNode.staticModel.values;

		ASSERT_EQ(1u, staticValues.size()) << "VirtualEntity should have 1 value in its ControlNode";
		ASSERT_EQ(la::avdecc::entity::model::ControlValueType::Type::ControlArrayUInt8, staticValues.getType()) << "VirtualEntity should have ControlLinearUInt8 type in its ControlNode";
		ASSERT_TRUE(!!staticValues) << "VirtualEntity should have valid values in its ControlNode";
		ASSERT_TRUE(!staticValues.areDynamicValues()) << "VirtualEntity should have static values in its ControlNode";

		// Expect to have CurrentValueOutOfRange validation result with a value outside bounds
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResult::CurrentValueOutOfRange, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, controlNode.dynamicModel.values));
	}
	catch (la::avdecc::controller::ControlledEntity::Exception const&)
	{
		ASSERT_FALSE(true) << "ControlNode not found";
	}

	// Get ControlNode.3 (Type: VendorSpecific, Subnode of AudioUnit)
	try
	{
		auto constexpr ControlIndex = la::avdecc::entity::model::ControlIndex{ 3u };
		auto const& controlNode = e.getControlNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, ControlIndex);
		ASSERT_EQ(la::avdecc::UniqueIdentifier{ 0x480BB2FFFED40000 }, controlNode.staticModel.controlType) << "VirtualEntity should have Identify type in its ControlNode";
		auto const& staticValues = controlNode.staticModel.values;

		ASSERT_EQ(1u, staticValues.size()) << "VirtualEntity should have 1 value in its ControlNode";
		ASSERT_EQ(la::avdecc::entity::model::ControlValueType::Type::ControlArrayUInt8, staticValues.getType()) << "VirtualEntity should have ControlLinearUInt8 type in its ControlNode";
		ASSERT_TRUE(!!staticValues) << "VirtualEntity should have valid values in its ControlNode";
		ASSERT_TRUE(!staticValues.areDynamicValues()) << "VirtualEntity should have static values in its ControlNode";

		// Expect to have CurrentValueOutOfRange validation result with a value outside bounds
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResult::CurrentValueOutOfRange, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, controlNode.dynamicModel.values));
	}
	catch (la::avdecc::controller::ControlledEntity::Exception const&)
	{
		ASSERT_FALSE(true) << "ControlNode not found";
	}
}

TEST(Controller, IdentifyAdvertisedButNoSuchIndex)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Create controller
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);

	// Setup logging
	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	// Load entity
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/IdentifyAdvertisedButNoSuchIndex.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 };

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(EntityID)));

	// Entity should not be IEEE17221 compatible because of the invalid ControlIndex in the ADP advertise
	EXPECT_FALSE(e.getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221));

	// Entity should have a valid Identify Control Index, at index 0 (even though the Identify ControlIndex advertised in the ADP is invalid, there is a valid one at CONFIGURATION level)
	ASSERT_TRUE(e.getIdentifyControlIndex().has_value());
	EXPECT_EQ(la::avdecc::entity::model::ControlIndex{ 0u }, *e.getIdentifyControlIndex());
}

TEST(Controller, IdentifyAdvertisedButInvalid)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Create controller
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);

	// Setup logging
	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	// Load entity
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/IdentifyAdvertisedButInvalid.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 };

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(EntityID)));

	// Entity should not be IEEE17221 compatible because of the invalid ControlIndex in the ADP advertise
	EXPECT_FALSE(e.getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221));

	// Entity should not have a valid Identify Control Index
	EXPECT_FALSE(e.getIdentifyControlIndex().has_value());
}

TEST(Controller, IdentifyAdvertisedInAudioUnit)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Create controller
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);

	// Setup logging
	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	// Load entity
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/IdentifyAdvertisedInAudioUnit.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 };

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(EntityID)));

	// Entity should not be IEEE17221 compatible because of the invalid ControlIndex in the ADP advertise
	EXPECT_FALSE(e.getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221));

	// Entity should not have a valid Identify Control Index
	EXPECT_FALSE(e.getIdentifyControlIndex().has_value());
}

TEST(Controller, IdentifyAdvertisedInConfiguration)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Create controller
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);

	// Setup logging
	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	// Load entity
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/IdentifyAdvertisedInConfiguration.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 };

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(EntityID)));

	// Entity should be IEEE17221 compatible
	EXPECT_TRUE(e.getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221));

	// Entity should have a valid Identify Control Index
	EXPECT_TRUE(e.getIdentifyControlIndex().has_value());
}

TEST(Controller, IdentifyAdvertisedInJack)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Create controller
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);

	// Setup logging
	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	// Load entity
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/IdentifyAdvertisedInJack.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 };

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(EntityID)));

	// Entity should be IEEE17221 compatible
	EXPECT_TRUE(e.getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221));

	// Entity should have a valid Identify Control Index
	EXPECT_TRUE(e.getIdentifyControlIndex().has_value());
}

TEST(Controller, NotAdvertisedButFoundInConfiguration)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Create controller
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);

	// Setup logging
	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	// Load entity
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/NotAdvertisedButFoundInConfiguration.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 };

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(EntityID)));

	// Entity should be IEEE17221 compatible
	EXPECT_TRUE(e.getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221));

	// Entity should have a valid Identify Control Index, at index 0
	ASSERT_TRUE(e.getIdentifyControlIndex().has_value());
	EXPECT_EQ(la::avdecc::entity::model::ControlIndex{ 0u }, *e.getIdentifyControlIndex());
}

TEST(Controller, NotAdvertisedButFoundInJack)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Create controller
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);

	// Setup logging
	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	// Load entity
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/NotAdvertisedButFoundInJack.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 };

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(EntityID)));

	// Entity should be IEEE17221 compatible
	EXPECT_TRUE(e.getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221));

	// Entity should have a valid Identify Control Index, at index 1
	ASSERT_TRUE(e.getIdentifyControlIndex().has_value());
	EXPECT_EQ(la::avdecc::entity::model::ControlIndex{ 1u }, *e.getIdentifyControlIndex());
}

TEST(Controller, NotAdvertisedAndIncorrectlyFoundInAudioUnit)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Create controller
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);

	// Setup logging
	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	// Load entity
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/NotAdvertisedAndIncorrectlyFoundInAudioUnit.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 };

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(EntityID)));

	// Entity should not be IEEE17221 compatible because of the invalid ControlIndex in the ADP advertise
	EXPECT_FALSE(e.getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221));

	// Entity should not have a valid Identify Control Index
	EXPECT_FALSE(e.getIdentifyControlIndex().has_value());
}

TEST(Controller, InvalidAdvertiseButFoundInConfiguration)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Create controller
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);

	// Setup logging
	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	// Load entity
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/InvalidAdvertiseButFoundInConfiguration.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 };

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(EntityID)));

	// Entity should not be IEEE17221 compatible because of the invalid ControlIndex in the ADP advertise
	EXPECT_FALSE(e.getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221));

	// Entity should have a valid Identify Control Index, at index 0
	ASSERT_TRUE(e.getIdentifyControlIndex().has_value());
	EXPECT_EQ(la::avdecc::entity::model::ControlIndex{ 0u }, *e.getIdentifyControlIndex());
}

TEST(Controller, InvalidAdvertiseButFoundInJack)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Create controller
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);

	// Setup logging
	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	// Load entity
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/InvalidAdvertiseButFoundInJack.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 };

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(EntityID)));

	// Entity should not be IEEE17221 compatible because of the invalid ControlIndex in the ADP advertise
	EXPECT_FALSE(e.getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221));

	// Entity should have a valid Identify Control Index, at index 1
	ASSERT_TRUE(e.getIdentifyControlIndex().has_value());
	EXPECT_EQ(la::avdecc::entity::model::ControlIndex{ 1u }, *e.getIdentifyControlIndex());
}

TEST(Controller, InvalidAdvertiseAndIncorrectlyFoundInAudioUnit)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };
	// Create controller
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);

	// Setup logging
	auto obs = LogObserver{};
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Warn);
	la::avdecc::logger::Logger::getInstance().registerObserver(&obs);

	// Load entity
	auto const [error, message] = controller->loadVirtualEntityFromJson("data/InvalidAdvertiseAndIncorrectlyFoundInAudioUnit.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 };

	auto& e = const_cast<la::avdecc::controller::ControlledEntityImpl&>(static_cast<la::avdecc::controller::ControlledEntityImpl const&>(*controller->getControlledEntityGuard(EntityID)));

	// Entity should not be IEEE17221 compatible because of the invalid ControlIndex in the ADP advertise
	EXPECT_FALSE(e.getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221));

	// Entity should not have a valid Identify Control Index
	EXPECT_FALSE(e.getIdentifyControlIndex().has_value());
}

namespace
{
class MediaClockModel_F : public ::testing::Test, public la::avdecc::controller::Controller::DefaultedObserver
{
public:
	virtual void SetUp() override
	{
		_controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en", nullptr, std::nullopt, nullptr);
	}

	virtual void TearDown() override {}

	void registerMockObserver() noexcept
	{
		_controller->registerObserver(this);
	}

	void unregisterMockObserver() noexcept
	{
		_controller->unregisterObserver(this);
	}

	la::avdecc::controller::Controller& getController() noexcept
	{
		return *_controller;
	}

	la::avdecc::controller::ControllerImpl& getControllerImpl() noexcept
	{
		return static_cast<la::avdecc::controller::ControllerImpl&>(*_controller);
	}

	void loadANSFile(std::string const& filePath) noexcept
	{
		auto const [error, msg] = _controller->loadVirtualEntitiesFromJsonNetworkState(filePath, la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::BinaryFormat, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDiagnostics }, false);
		ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	}

	void loadEntityFile(std::string const& filePath) noexcept
	{
		auto const [error, msg] = _controller->loadVirtualEntityFromJson(filePath, la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDiagnostics });
		ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	}

	// la::avdecc::controller::Controller::Observer Mock overrides
	MOCK_METHOD(void, onMediaClockChainChanged, (la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::controller::model::MediaClockChain const& /*mcChain*/), (noexcept, override));

private:
	la::avdecc::controller::Controller::UniquePointer _controller{ nullptr, nullptr };
};
} // namespace

static auto constexpr Entity01 = la::avdecc::UniqueIdentifier{ 0x0000000000000001 }; // SI.0 connected to E11.0  // CD.0 on SI.0
static auto constexpr Entity02 = la::avdecc::UniqueIdentifier{ 0x0000000000000002 }; // No connections           // CD.0 on SI.0
static auto constexpr Entity03 = la::avdecc::UniqueIdentifier{ 0x0000000000000003 }; // SI.0 connected to E12.0  // CD.0 on SI.0
static auto constexpr Entity04 = la::avdecc::UniqueIdentifier{ 0x0000000000000004 }; // SI.0 connected to E11.1  // CD.0 on SI.0
static auto constexpr Entity11 = la::avdecc::UniqueIdentifier{ 0x0000000000000011 }; // SI.2 connected to E12.2  // CD.0 on Internal, CD.1 on SI.2 // SI.2, SO.2 on CD.1
static auto constexpr Entity12 = la::avdecc::UniqueIdentifier{ 0x0000000000000012 }; // No connections           // CD.0 on External
static auto constexpr Entity13 = la::avdecc::UniqueIdentifier{ 0x0000000000000013 }; // SI.2 connected to E14.2  // CD.0 on SI.2
static auto constexpr Entity14 = la::avdecc::UniqueIdentifier{ 0x0000000000000014 }; // SI.2 connected to E13.2  // CD.0 on Internal, CD.1 on SI.2 // SI.2, SO.2 on CD.1

// *****************************
// Testing static state

TEST_F(MediaClockModel_F, StreamInput_Connected_Offline)
{
	loadEntityFile("data/MediaClockModel/Entity_0x01.json");

	try
	{
		auto& c = getController();
		{
			auto const& e = *c.getControlledEntityGuard(Entity01);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(01, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity11, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Undefined, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::EntityOffline, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamOutputIndex);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

static void check_StreamInput_Connected_Online(la::avdecc::controller::Controller& c)
{
	{
		auto const& e = *c.getControlledEntityGuard(Entity01);
		auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
		ASSERT_EQ(2u, node.mediaClockChain.size());
		{
			auto const& n = node.mediaClockChain[0];
			EXPECT_EQ(Entity01, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
			EXPECT_EQ(std::nullopt, n.streamOutputIndex);
		}
		{
			auto const& n = node.mediaClockChain[1];
			EXPECT_EQ(Entity11, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
			EXPECT_EQ(std::nullopt, n.streamInputIndex);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamOutputIndex);
		}
	}
}

TEST_F(MediaClockModel_F, StreamInput_Connected_Online)
{
	loadEntityFile("data/MediaClockModel/Entity_0x01.json");
	loadEntityFile("data/MediaClockModel/Entity_0x11.json");

	try
	{
		auto& c = getController();
		check_StreamInput_Connected_Online(c);
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, StreamInput_Connected_Online_Reverse)
{
	loadEntityFile("data/MediaClockModel/Entity_0x11.json");
	loadEntityFile("data/MediaClockModel/Entity_0x01.json");

	try
	{
		auto& c = getController();
		check_StreamInput_Connected_Online(c);
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, StreamInput_NotConnected)
{
	loadEntityFile("data/MediaClockModel/Entity_0x02.json");

	try
	{
		auto& c = getController();
		{
			auto const& e = *c.getControlledEntityGuard(Entity02);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(1u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity02, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::StreamNotConnected, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

static void check_External_Connected(la::avdecc::controller::Controller& c)
{
	{
		auto const& e = *c.getControlledEntityGuard(Entity03);
		auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
		ASSERT_EQ(2u, node.mediaClockChain.size());
		{
			auto const& n = node.mediaClockChain[0];
			EXPECT_EQ(Entity03, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
			EXPECT_EQ(std::nullopt, n.streamOutputIndex);
		}
		{
			auto const& n = node.mediaClockChain[1];
			EXPECT_EQ(Entity12, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::External, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
			EXPECT_EQ(std::nullopt, n.streamInputIndex);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamOutputIndex);
		}
	}
}

TEST_F(MediaClockModel_F, External_Connected)
{
	loadEntityFile("data/MediaClockModel/Entity_0x03.json");
	loadEntityFile("data/MediaClockModel/Entity_0x12.json");

	try
	{
		auto& c = getController();
		check_External_Connected(c);
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, External_Connected_Reverse)
{
	loadEntityFile("data/MediaClockModel/Entity_0x12.json");
	loadEntityFile("data/MediaClockModel/Entity_0x03.json");

	try
	{
		auto& c = getController();
		check_External_Connected(c);
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

static void check_DoubleStreamInput_Connected_CrossDomain_Online(la::avdecc::controller::Controller& c)
{
	{
		auto const& e = *c.getControlledEntityGuard(Entity04);
		auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
		ASSERT_EQ(3u, node.mediaClockChain.size());
		{
			auto const& n = node.mediaClockChain[0];
			EXPECT_EQ(Entity04, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
			EXPECT_EQ(std::nullopt, n.streamOutputIndex);
		}
		{
			auto const& n = node.mediaClockChain[1];
			EXPECT_EQ(Entity11, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 1u }, n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, n.streamOutputIndex);
		}
		{
			auto const& n = node.mediaClockChain[2];
			EXPECT_EQ(Entity12, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::External, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
			EXPECT_EQ(std::nullopt, n.streamInputIndex);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
		}
	}
	// Also check ClockDomain.0 on Entity11
	{
		auto const& e = *c.getControlledEntityGuard(Entity11);
		auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
		ASSERT_EQ(1u, node.mediaClockChain.size());
		{
			auto const& n = node.mediaClockChain[0];
			EXPECT_EQ(Entity11, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
			EXPECT_EQ(std::nullopt, n.streamInputIndex);
			EXPECT_EQ(std::nullopt, n.streamOutputIndex);
		}
	}
	// Also check ClockDomain.1 on Entity11
	{
		auto const& e = *c.getControlledEntityGuard(Entity11);
		auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 1u });
		ASSERT_EQ(2u, node.mediaClockChain.size());
		{
			auto const& n = node.mediaClockChain[0];
			EXPECT_EQ(Entity11, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 1u }, n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
			EXPECT_EQ(std::nullopt, n.streamOutputIndex);
		}
		{
			auto const& n = node.mediaClockChain[1];
			EXPECT_EQ(Entity12, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::External, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
			EXPECT_EQ(std::nullopt, n.streamInputIndex);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
		}
	}
}

TEST_F(MediaClockModel_F, DoubleStreamInput_Connected_CrossDomain_Online_04_11_12)
{
	loadEntityFile("data/MediaClockModel/Entity_0x04.json");
	loadEntityFile("data/MediaClockModel/Entity_0x11.json");
	loadEntityFile("data/MediaClockModel/Entity_0x12.json");

	try
	{
		auto& c = getController();
		check_DoubleStreamInput_Connected_CrossDomain_Online(c);
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, DoubleStreamInput_Connected_CrossDomain_Online_04_12_11)
{
	loadEntityFile("data/MediaClockModel/Entity_0x04.json");
	loadEntityFile("data/MediaClockModel/Entity_0x12.json");
	loadEntityFile("data/MediaClockModel/Entity_0x11.json");

	try
	{
		auto& c = getController();
		check_DoubleStreamInput_Connected_CrossDomain_Online(c);
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, DoubleStreamInput_Connected_CrossDomain_Online_11_04_12)
{
	loadEntityFile("data/MediaClockModel/Entity_0x11.json");
	loadEntityFile("data/MediaClockModel/Entity_0x04.json");
	loadEntityFile("data/MediaClockModel/Entity_0x12.json");

	try
	{
		auto& c = getController();
		check_DoubleStreamInput_Connected_CrossDomain_Online(c);
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, DoubleStreamInput_Connected_CrossDomain_Online_11_12_04)
{
	loadEntityFile("data/MediaClockModel/Entity_0x11.json");
	loadEntityFile("data/MediaClockModel/Entity_0x12.json");
	loadEntityFile("data/MediaClockModel/Entity_0x04.json");

	try
	{
		auto& c = getController();
		check_DoubleStreamInput_Connected_CrossDomain_Online(c);
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, DoubleStreamInput_Connected_CrossDomain_Online_12_04_11)
{
	loadEntityFile("data/MediaClockModel/Entity_0x12.json");
	loadEntityFile("data/MediaClockModel/Entity_0x04.json");
	loadEntityFile("data/MediaClockModel/Entity_0x11.json");

	try
	{
		auto& c = getController();
		check_DoubleStreamInput_Connected_CrossDomain_Online(c);
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, DoubleStreamInput_Connected_CrossDomain_Online_12_11_04)
{
	loadEntityFile("data/MediaClockModel/Entity_0x12.json");
	loadEntityFile("data/MediaClockModel/Entity_0x11.json");
	loadEntityFile("data/MediaClockModel/Entity_0x04.json");

	try
	{
		auto& c = getController();
		check_DoubleStreamInput_Connected_CrossDomain_Online(c);
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

static void check_Recursive(la::avdecc::controller::Controller& c)
{
	{
		auto const& e = *c.getControlledEntityGuard(Entity13);
		auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
		ASSERT_EQ(3u, node.mediaClockChain.size());
		{
			auto const& n = node.mediaClockChain[0];
			EXPECT_EQ(Entity13, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
			EXPECT_EQ(std::nullopt, n.streamOutputIndex);
		}
		{
			auto const& n = node.mediaClockChain[1];
			EXPECT_EQ(Entity14, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 1u }, n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
		}
		{
			auto const& n = node.mediaClockChain[2];
			EXPECT_EQ(Entity13, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Undefined, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Recursive, n.status);
			EXPECT_EQ(std::nullopt, n.streamInputIndex);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
		}
	}
	// Also check ClockDomain.1 on Entity14
	{
		auto const& e = *c.getControlledEntityGuard(Entity14);
		auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 1u });
		ASSERT_EQ(3u, node.mediaClockChain.size());
		{
			auto const& n = node.mediaClockChain[0];
			EXPECT_EQ(Entity14, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 1u }, n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
			EXPECT_EQ(std::nullopt, n.streamOutputIndex);
		}
		{
			auto const& n = node.mediaClockChain[1];
			EXPECT_EQ(Entity13, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
		}
		{
			auto const& n = node.mediaClockChain[2];
			EXPECT_EQ(Entity14, n.entityID);
			EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockDomainIndex);
			EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockSourceIndex);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Undefined, n.type);
			EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Recursive, n.status);
			EXPECT_EQ(std::nullopt, n.streamInputIndex);
			EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
		}
	}
}

TEST_F(MediaClockModel_F, Recursive)
{
	loadEntityFile("data/MediaClockModel/Entity_0x13.json");
	loadEntityFile("data/MediaClockModel/Entity_0x14.json");

	try
	{
		auto& c = getController();
		check_Recursive(c);
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, Recursive_Reverse)
{
	loadEntityFile("data/MediaClockModel/Entity_0x14.json");
	loadEntityFile("data/MediaClockModel/Entity_0x13.json");

	try
	{
		auto& c = getController();
		check_Recursive(c);
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

// *****************************
// Testing dynamic state change

TEST_F(MediaClockModel_F, StreamInput_Connected_Online_SwitchOffline)
{
	loadEntityFile("data/MediaClockModel/Entity_0x01.json");
	loadEntityFile("data/MediaClockModel/Entity_0x11.json");

	try
	{
		auto& c = getController();
		{
			auto const& e = *c.getControlledEntityGuard(Entity01);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity01, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity11, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamOutputIndex);
			}
		}

		// Expect Controller::Observer::onMediaClockChainChanged() to be called
		registerMockObserver();
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity01).get(), la::avdecc::entity::model::ClockDomainIndex{ 0u }, ::testing::_));

		// Entity coming offline
		c.unloadVirtualEntity(Entity11);

		// Validate chain has been updated
		{
			auto const& e = *c.getControlledEntityGuard(Entity01);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(01, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity11, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Undefined, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::EntityOffline, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamOutputIndex);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, StreamInput_Connected_Offline_SwitchOnline)
{
	loadEntityFile("data/MediaClockModel/Entity_0x01.json");

	try
	{
		auto& c = getController();
		{
			auto const& e = *c.getControlledEntityGuard(Entity01);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(01, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity11, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Undefined, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::EntityOffline, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamOutputIndex);
			}
		}

		// Expect Controller::Observer::onMediaClockChainChanged() to be called
		registerMockObserver();
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity01).get(), la::avdecc::entity::model::ClockDomainIndex{ 0u }, ::testing::_));

		// Entity coming online
		loadEntityFile("data/MediaClockModel/Entity_0x11.json");

		// Validate chain has been updated
		{
			auto const& e = *c.getControlledEntityGuard(Entity01);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity01, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity11, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamOutputIndex);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, StreamInput_Connected_Online_SwitchDisconnect)
{
	loadEntityFile("data/MediaClockModel/Entity_0x01.json");
	loadEntityFile("data/MediaClockModel/Entity_0x11.json");

	try
	{
		auto& c = getControllerImpl();
		{
			auto const& e = *c.getControlledEntityGuard(Entity01);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity01, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity11, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamOutputIndex);
			}
		}

		// Expect Controller::Observer::onMediaClockChainChanged() to be called
		registerMockObserver();
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity01).get(), la::avdecc::entity::model::ClockDomainIndex{ 0u }, ::testing::_));

		// Disconnect the stream
		c.handleListenerStreamStateNotification(la::avdecc::entity::model::StreamIdentification{ Entity11, la::avdecc::entity::model::StreamIndex{ 0u } }, la::avdecc::entity::model::StreamIdentification{ Entity01, la::avdecc::entity::model::StreamIndex{ 0u } }, false, la::avdecc::entity::ConnectionFlags{}, true);

		// Validate chain has been updated
		{
			auto const& e = *c.getControlledEntityGuard(Entity01);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(1u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(01, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::StreamNotConnected, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, DoubleStreamInput_Connected_CrossDomain_Online_SwitchDisconnect)
{
	loadEntityFile("data/MediaClockModel/Entity_0x04.json");
	loadEntityFile("data/MediaClockModel/Entity_0x11.json");
	loadEntityFile("data/MediaClockModel/Entity_0x12.json");

	try
	{
		auto& c = getControllerImpl();
		{
			auto const& e = *c.getControlledEntityGuard(Entity04);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(3u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity04, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity11, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 1u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[2];
				EXPECT_EQ(Entity12, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::External, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
		}
		// Also check ClockDomain.0 on Entity11
		{
			auto const& e = *c.getControlledEntityGuard(Entity11);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(1u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity11, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
		}
		// Also check ClockDomain.1 on Entity11
		{
			auto const& e = *c.getControlledEntityGuard(Entity11);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 1u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity11, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 1u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity12, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::External, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
		}

		// Expect Controller::Observer::onMediaClockChainChanged() to be called
		registerMockObserver();
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity04).get(), la::avdecc::entity::model::ClockDomainIndex{ 0u }, ::testing::_));
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity11).get(), la::avdecc::entity::model::ClockDomainIndex{ 1u }, ::testing::_));

		// Disconnect the stream
		c.handleListenerStreamStateNotification(la::avdecc::entity::model::StreamIdentification{ Entity12, la::avdecc::entity::model::StreamIndex{ 2u } }, la::avdecc::entity::model::StreamIdentification{ Entity11, la::avdecc::entity::model::StreamIndex{ 2u } }, false, la::avdecc::entity::ConnectionFlags{}, true);

		// Validate chains has been updated
		{
			auto const& e = *c.getControlledEntityGuard(Entity04);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity04, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity11, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 1u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::StreamNotConnected, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, n.streamOutputIndex);
			}
		}
		// Also check ClockDomain.0 on Entity11
		{
			auto const& e = *c.getControlledEntityGuard(Entity11);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(1u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity11, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
		}
		// Also check ClockDomain.1 on Entity11
		{
			auto const& e = *c.getControlledEntityGuard(Entity11);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 1u });
			ASSERT_EQ(1u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity11, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 1u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::StreamNotConnected, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, StreamInput_Recursive_SwitchDisconnect)
{
	loadEntityFile("data/MediaClockModel/Entity_0x13.json");
	loadEntityFile("data/MediaClockModel/Entity_0x14.json");

	try
	{
		auto& c = getControllerImpl();
		{
			auto const& e = *c.getControlledEntityGuard(Entity13);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(3u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity13, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity14, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 1u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[2];
				EXPECT_EQ(Entity13, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Undefined, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Recursive, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
		}
		// Also check ClockDomain.1 on Entity14
		{
			auto const& e = *c.getControlledEntityGuard(Entity14);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 1u });
			ASSERT_EQ(3u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity14, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 1u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity13, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[2];
				EXPECT_EQ(Entity14, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Undefined, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Recursive, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
		}

		// Expect Controller::Observer::onMediaClockChainChanged() to be called
		registerMockObserver();
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity13).get(), la::avdecc::entity::model::ClockDomainIndex{ 0u }, ::testing::_));
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity14).get(), la::avdecc::entity::model::ClockDomainIndex{ 1u }, ::testing::_));

		// Disconnect the stream
		c.handleListenerStreamStateNotification(la::avdecc::entity::model::StreamIdentification{ Entity14, la::avdecc::entity::model::StreamIndex{ 2u } }, la::avdecc::entity::model::StreamIdentification{ Entity13, la::avdecc::entity::model::StreamIndex{ 2u } }, false, la::avdecc::entity::ConnectionFlags{}, true);

		// Validate chain has been updated
		{
			auto const& e = *c.getControlledEntityGuard(Entity13);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(1u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity13, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::StreamNotConnected, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
		}
		// Also check ClockDomain.1 on Entity14
		{
			auto const& e = *c.getControlledEntityGuard(Entity14);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 1u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity14, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 1u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity13, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::StreamNotConnected, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, StreamInput_Recursive_SwitchConnect)
{
	loadEntityFile("data/MediaClockModel/Entity_0x13.json");
	loadEntityFile("data/MediaClockModel/Entity_0x14.json");

	try
	{
		auto& c = getControllerImpl();
		// Disconnect the stream
		c.handleListenerStreamStateNotification(la::avdecc::entity::model::StreamIdentification{ Entity14, la::avdecc::entity::model::StreamIndex{ 2u } }, la::avdecc::entity::model::StreamIdentification{ Entity13, la::avdecc::entity::model::StreamIndex{ 2u } }, false, la::avdecc::entity::ConnectionFlags{}, true);

		// Check initial state
		{
			auto const& e = *c.getControlledEntityGuard(Entity13);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(1u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity13, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::StreamNotConnected, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
		}
		// Also check ClockDomain.1 on Entity14
		{
			auto const& e = *c.getControlledEntityGuard(Entity14);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 1u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity14, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 1u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity13, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::StreamNotConnected, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
		}

		// Expect Controller::Observer::onMediaClockChainChanged() to be called
		registerMockObserver();
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity13).get(), la::avdecc::entity::model::ClockDomainIndex{ 0u }, ::testing::_));
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity14).get(), la::avdecc::entity::model::ClockDomainIndex{ 1u }, ::testing::_));

		// Connect the stream
		c.handleListenerStreamStateNotification(la::avdecc::entity::model::StreamIdentification{ Entity14, la::avdecc::entity::model::StreamIndex{ 2u } }, la::avdecc::entity::model::StreamIdentification{ Entity13, la::avdecc::entity::model::StreamIndex{ 2u } }, true, la::avdecc::entity::ConnectionFlags{}, true);

		// Validate chain has been updated
		{
			auto const& e = *c.getControlledEntityGuard(Entity13);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(3u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity13, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity14, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 1u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[2];
				EXPECT_EQ(Entity13, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Undefined, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Recursive, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
		}
		// Also check ClockDomain.1 on Entity14
		{
			auto const& e = *c.getControlledEntityGuard(Entity14);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 1u });
			ASSERT_EQ(3u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity14, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 1u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity13, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[2];
				EXPECT_EQ(Entity14, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Undefined, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Recursive, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(MediaClockModel_F, StreamInput_Connected_Online_SwitchClockSource)
{
	loadEntityFile("data/MediaClockModel/Entity_0x01.json");
	loadEntityFile("data/MediaClockModel/Entity_0x11.json");

	try
	{
		auto& c = getControllerImpl();
		{
			auto const& e = *c.getControlledEntityGuard(Entity01);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity01, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity11, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamOutputIndex);
			}
		}

		// Expect Controller::Observer::onMediaClockChainChanged() to be called
		registerMockObserver();
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity01).get(), la::avdecc::entity::model::ClockDomainIndex{ 0u }, ::testing::_));

		// Change the clock source
		c.updateClockSource(*c.getControlledEntityImplGuard(Entity01, true, false), la::avdecc::entity::model::ClockDomainIndex{ 0u }, la::avdecc::entity::model::ClockSourceIndex{ 1u }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::Throw);

		// Validate chain has been updated
		{
			auto const& e = *c.getControlledEntityGuard(Entity01);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(1u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(01, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 1u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

// Test for #125
TEST_F(MediaClockModel_F, NotCrashing_Issue125)
{
	loadEntityFile("data/MediaClockModel/Entity_0x01.json");

	try
	{
		auto& c = getController();
		{
			auto const& e = *c.getControlledEntityGuard(Entity01);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(01, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity11, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::getInvalidDescriptorIndex(), n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Undefined, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::EntityOffline, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamOutputIndex);
			}
		}

		// Expect Controller::Observer::onMediaClockChainChanged() to be called
		registerMockObserver();
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity01).get(), la::avdecc::entity::model::ClockDomainIndex{ 0u }, ::testing::_))
			.WillOnce(testing::Invoke(
				[](la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::controller::model::MediaClockChain const& /*mcChain*/)
				{
					// Try to get a ControlledEntity inside the handler should not crash
					auto const entityID = entity->getEntity().getEntityID();
					auto const c = controller->getControlledEntityGuard(entityID);
					// Dummy code to force variables
					EXPECT_EQ(entityID, c->getEntity().getEntityID());
				}));

		// Entity coming online
		loadEntityFile("data/MediaClockModel/Entity_0x11.json");
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST(Controller, HashEntityModel)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel };
	auto const& [error, msg, controlledEntity] = la::avdecc::controller::Controller::deserializeControlledEntityFromJson("data/SimpleEntity.json", flags);
	ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	auto const checksum = la::avdecc::controller::Controller::computeEntityModelChecksum(*controlledEntity);
	EXPECT_FALSE(checksum.empty());
	EXPECT_EQ(64u, checksum.size());
}
