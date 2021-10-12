/*
* Copyright (C) 2016-2021, L-Acoustics and its contributors

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
#include <string>
#include <thread>
#include <chrono>
#include <future>
#include <vector>
#include <cstdint>

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
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::AudioUnitNode const* const parent, la::avdecc::controller::model::StreamPortNode const& node) noexcept override
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
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::RedundantStreamNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
		_serializedModel += "rsi";
		for (auto const& streamKV : node.redundantStreams)
		{
			auto const streamIndex = streamKV.first;
			_serializedModel += std::to_string(streamIndex) + "+";
		}
		serializeNode(*node.primaryStream);
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
		entity.buildEntityModelGraph();

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
		entity.buildEntityModelGraph();

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
		entity.buildEntityModelGraph();

		EntityModelVisitor serializer{};
		entity.accept(&serializer);
		auto const serialized = serializer.getSerializedModel();
		EXPECT_STREQ("nullptr,dt0,di0,pdt0,dt1,di0,pdt1,dt5,di0,pdt1,dt5,di1,pdt1,dt5,vi0,rsi0+1+dt5,di0,pdt1,pdt5,dt5,di0,pdt1,pdt5,dt5,di1,", serialized.c_str());
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
		entity.buildEntityModelGraph();

		EntityModelVisitor serializer{};
		entity.accept(&serializer);
		auto const serialized = serializer.getSerializedModel();
		EXPECT_STREQ("nullptr,dt0,di0,pdt0,dt1,di0,pdt1,dt5,di0,pdt1,dt5,di1,pdt1,dt5,vi0,rsi0+1+dt5,di1,pdt1,pdt5,dt5,di0,pdt1,pdt5,dt5,di1,", serialized.c_str());
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
		entity.buildEntityModelGraph();

		EntityModelVisitor serializer{};
		entity.accept(&serializer);
		auto const serialized = serializer.getSerializedModel();
		EXPECT_STREQ("nullptr,dt0,di0,pdt0,dt1,di0,pdt1,dt5,di0,pdt1,dt5,di1,pdt1,dt5,di2,pdt1,dt5,vi0,rsi1+2+dt5,di2,pdt1,pdt5,dt5,di1,pdt1,pdt5,dt5,di2,", serialized.c_str());
	}
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

TEST(Controller, DestroyWhileSending)
{
	static std::promise<void> commandResultPromise{};
	{
		auto pi = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }));
		auto const commonInformation{ la::avdecc::entity::Entity::CommonInformation{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0u, la::avdecc::entity::TalkerCapabilities{}, 0u, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented }, std::nullopt, std::nullopt } };
		auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ la::networkInterface::MacAddress{ { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, 31u, 0u, std::nullopt, std::nullopt } };
		auto controllerGuard = std::make_unique<la::avdecc::entity::LocalEntityGuard<la::avdecc::entity::ControllerEntityImpl>>(pi.get(), commonInformation, la::avdecc::entity::Entity::InterfacesInformation{ { la::avdecc::entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } }, nullptr);
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
		auto const s3{ la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamInputConnectionInfo::State::NotConnected } };
		EXPECT_EQ(s2, s1) << "Talker StreamIdentification ignored when not connected";
		EXPECT_NE(s3, s1);
		EXPECT_NE(s3, s2);
	}

	// FastConnecting
	{
		auto const s1{ la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamInputConnectionInfo::State::FastConnecting } };
		auto const s2{ la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{ la::avdecc::UniqueIdentifier{ 0x1 }, 1 }, la::avdecc::entity::model::StreamInputConnectionInfo::State::FastConnecting } };
		auto const s3{ la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamInputConnectionInfo::State::FastConnecting } };
		EXPECT_NE(s2, s1) << "Talker StreamIdentification not ignored when fast connecting";
		EXPECT_NE(s3, s1);
		EXPECT_NE(s3, s2);
	}

	// Connected
	{
		auto const s1{ la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected } };
		auto const s2{ la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{ la::avdecc::UniqueIdentifier{ 0x1 }, 1 }, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected } };
		auto const s3{ la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected } };
		EXPECT_NE(s2, s1) << "Talker StreamIdentification not ignored when connected";
		EXPECT_NE(s3, s1);
		EXPECT_NE(s3, s2);
	}
}

namespace
{
class Controller_F : public ::testing::Test
{
public:
	virtual void SetUp() override
	{
		_controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en");
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
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en");

	auto const sendAdpAvailable = [gPTP = controller->getControllerEID()](auto const& entityID, auto const interfaceIndex)
	{
		auto intfc = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { static_cast<la::networkInterface::MacAddress::value_type>(interfaceIndex), 0x06, 0x05, 0x04, 0x03, 0x02 } }));

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

	class Obs final : public la::avdecc::controller::Controller::Observer
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
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en");

	// Add an observer
	auto obs = Obs{};
	controller->registerObserver(&obs);

	auto const sendAdpAvailable = [](auto const& entityID, auto const interfaceIndex, auto const validTime)
	{
		auto intfc = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { static_cast<la::networkInterface::MacAddress::value_type>(interfaceIndex), 0x06, 0x05, 0x04, 0x03, 0x02 } }));

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
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en");
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
		auto const& staticValues = controlNode.staticModel->values;

		ASSERT_EQ(1u, staticValues.size()) << "VirtualEntity should have 1 value in its ControlNode";
		ASSERT_EQ(la::avdecc::entity::model::ControlValueType::Type::ControlLinearUInt8, staticValues.getType()) << "VirtualEntity should have ControlLinearUInt8 type in its ControlNode";
		ASSERT_TRUE(!!staticValues) << "VirtualEntity should have valid values in its ControlNode";
		ASSERT_TRUE(!staticValues.areDynamicValues()) << "VirtualEntity should have static values in its ControlNode";

		// Expect to pass ControlValues validation with a value set to minimum
		EXPECT_TRUE(c.validateControlValues(EntityID, ControlIndex, staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::uint8_t>>{ { { 0u } } } }));

		// Expect to pass ControlValues validation with a value set to maximum
		EXPECT_TRUE(c.validateControlValues(EntityID, ControlIndex, staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::uint8_t>>{ { { 255u } } } }));
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
	auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en");
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
		auto const& staticValues = controlNode.staticModel->values;

		ASSERT_EQ(1u, staticValues.size()) << "VirtualEntity should have 1 value in its ControlNode";
		ASSERT_EQ(la::avdecc::entity::model::ControlValueType::Type::ControlLinearUInt8, staticValues.getType()) << "VirtualEntity should have ControlLinearUInt8 type in its ControlNode";
		ASSERT_TRUE(!!staticValues) << "VirtualEntity should have valid values in its ControlNode";
		ASSERT_TRUE(!staticValues.areDynamicValues()) << "VirtualEntity should have static values in its ControlNode";

		// Expect to not pass ControlValues validation with non-valid dynamic values
		EXPECT_FALSE(c.validateControlValues(EntityID, ControlIndex, staticValues, {}));

		// Expect to not pass ControlValues validation with static values instead of dynamic values
		EXPECT_FALSE(c.validateControlValues(EntityID, ControlIndex, staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueStatic<std::uint8_t>>{} }));

		// Expect to not pass ControlValues validation with a different type of dynamic values
		EXPECT_FALSE(c.validateControlValues(EntityID, ControlIndex, staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::int8_t>>{} }));

		// Expect to not pass ControlValues validation with a different count of values
		EXPECT_FALSE(c.validateControlValues(EntityID, ControlIndex, staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::uint8_t>>{} }));

		// Expect to not pass ControlValues validation with a value not multiple of Step for LinearValues
		EXPECT_FALSE(c.validateControlValues(EntityID, ControlIndex, staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::uint8_t>>{ { { 1u } } } }));

		// Expect to not pass ControlValues validation with a value outside bounds // TODO: Cannot test with an IDENTIFY Control, have to create another Control
	}
	catch (la::avdecc::controller::ControlledEntity::Exception const&)
	{
		ASSERT_FALSE(true) << "ControlNode not found";
	}
}
