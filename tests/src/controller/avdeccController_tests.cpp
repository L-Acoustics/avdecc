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
* @file avdeccController_tests.cpp.cpp
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

namespace
{
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
		auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ la::avdecc::networkInterface::MacAddress{}, 31u, 0u, std::nullopt, std::nullopt } };
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
	//	auto const e{ la::avdecc::entity::Entity{ 0x0102030405060708, la::avdecc::networkInterface::MacAddress{}, 0x1122334455667788, la::avdecc::entity::EntityCapabilities::AemSupported, 0, la::avdecc::entity::TalkerCapabilities{}, 0, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{}, 0, 0, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier() } };
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
		auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ la::avdecc::networkInterface::MacAddress{}, 31u, 0u, std::nullopt, std::nullopt } };
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
		auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ la::avdecc::networkInterface::MacAddress{}, 31u, 0u, std::nullopt, std::nullopt } };
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
		auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ la::avdecc::networkInterface::MacAddress{}, 31u, 0u, std::nullopt, std::nullopt } };
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
		auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ la::avdecc::networkInterface::MacAddress{}, 31u, 0u, std::nullopt, std::nullopt } };
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
		auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ la::avdecc::networkInterface::MacAddress{ { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, 31u, 0u, std::nullopt, std::nullopt } };
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
		auto const s1{ la::avdecc::entity::model::StreamConnectionState{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamConnectionState::State::NotConnected } };
		auto const s2{ la::avdecc::entity::model::StreamConnectionState{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamIdentification{ la::avdecc::UniqueIdentifier{ 0x1 }, 1 }, la::avdecc::entity::model::StreamConnectionState::State::NotConnected } };
		auto const s3{ la::avdecc::entity::model::StreamConnectionState{ la::avdecc::entity::model::StreamIdentification{ la::avdecc::UniqueIdentifier{ 0x1 }, 1 }, la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamConnectionState::State::NotConnected } };
		EXPECT_EQ(s2, s1) << "Talker StreamIdentification ignored when not connected";
		EXPECT_NE(s3, s1);
		EXPECT_NE(s3, s2);
	}

	// FastConnecting
	{
		auto const s1{ la::avdecc::entity::model::StreamConnectionState{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamConnectionState::State::FastConnecting } };
		auto const s2{ la::avdecc::entity::model::StreamConnectionState{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamIdentification{ la::avdecc::UniqueIdentifier{ 0x1 }, 1 }, la::avdecc::entity::model::StreamConnectionState::State::FastConnecting } };
		auto const s3{ la::avdecc::entity::model::StreamConnectionState{ la::avdecc::entity::model::StreamIdentification{ la::avdecc::UniqueIdentifier{ 0x1 }, 1 }, la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamConnectionState::State::FastConnecting } };
		EXPECT_NE(s2, s1) << "Talker StreamIdentification not ignored when fast connecting";
		EXPECT_NE(s3, s1);
		EXPECT_NE(s3, s2);
	}

	// Connected
	{
		auto const s1{ la::avdecc::entity::model::StreamConnectionState{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamConnectionState::State::Connected } };
		auto const s2{ la::avdecc::entity::model::StreamConnectionState{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamIdentification{ la::avdecc::UniqueIdentifier{ 0x1 }, 1 }, la::avdecc::entity::model::StreamConnectionState::State::Connected } };
		auto const s3{ la::avdecc::entity::model::StreamConnectionState{ la::avdecc::entity::model::StreamIdentification{ la::avdecc::UniqueIdentifier{ 0x1 }, 1 }, la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamConnectionState::State::Connected } };
		EXPECT_NE(s2, s1) << "Talker StreamIdentification not ignored when connected";
		EXPECT_NE(s3, s1);
		EXPECT_NE(s3, s2);
	}
}

TEST(Controller, VirtualEntityLoad)
{
	//static std::promise<void> commandResultPromise{};
	{
		auto controller = la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::Virtual, "VirtualInterface", 0x0001, la::avdecc::UniqueIdentifier{}, "en");
		auto const [error, message] = controller->loadVirtualEntityFromReadableJson("data/SimpleEntity.json", false);
		EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
		EXPECT_STREQ("", message.c_str());
	}

	// Wait for the handler to complete
	//auto status = commandResultPromise.get_future().wait_for(std::chrono::seconds(1));
	//ASSERT_NE(std::future_status::timeout, status);
}
