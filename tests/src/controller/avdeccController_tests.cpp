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
* @file avdeccController_tests.cpp.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/executor.hpp>
#include <la/avdecc/controller/avdeccController.hpp>
#include <la/avdecc/internals/protocolAemAecpdu.hpp>
#include <la/avdecc/internals/protocolAemPayloadSizes.hpp>
#include <la/avdecc/internals/entityModelControlValuesTraits.hpp>
#include <la/avdecc/internals/streamFormatInfo.hpp>

// Internal API
#include "controller/avdeccControlledEntityImpl.hpp"
#include "controller/avdeccControllerImpl.hpp"
#include "entity/controllerEntityImpl.hpp"
#include "la/avdecc/internals/entityModelTreeCommon.hpp"
#include "la/avdecc/internals/entityModelTypes.hpp"
#include "protocolInterface/protocolInterface_virtual.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <thread>
#include <chrono>
#include <future>
#include <vector>
#include <cstdint>
#include <functional>

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
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::TimingNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::PtpInstanceNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	// Virtual parenting to show PtpInstanceNode which have the specified TimingNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::TimingNode const* const parent, la::avdecc::controller::model::PtpInstanceNode const& node) noexcept override
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::PtpInstanceNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::PtpInstanceNode const* const parent, la::avdecc::controller::model::PtpPortNode const& node) noexcept override
	{
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
	// Virtual parenting to show ControlNode which have the specified TimingNode as grandParent and PtpInstanceNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::TimingNode const* const grandParent, la::avdecc::controller::model::PtpInstanceNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
	{
		serializeNode(grandGrandParent);
		serializeNode(grandParent);
		serializeNode(parent);
		serializeNode(node);
	}
	// Virtual parenting to show PtpPortNode which have the specified TimingNode as grandParent and PtpInstanceNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::TimingNode const* const grandParent, la::avdecc::controller::model::PtpInstanceNode const* const parent, la::avdecc::controller::model::PtpPortNode const& node) noexcept override
	{
		serializeNode(grandGrandParent);
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
		// Create an executor for ProtocolInterface
		auto const executorWrapper = la::avdecc::ExecutorManager::getInstance().registerExecutor(DefaultExecutorName, la::avdecc::ExecutorWithDispatchQueue::create(DefaultExecutorName, la::avdecc::utils::ThreadPriority::Highest));

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
	auto status = commandResultPromise.get_future().wait_for(std::chrono::seconds(2));
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
	//auto status = commandResultPromise.get_future().wait_for(std::chrono::seconds(2));
	//ASSERT_NE(std::future_status::timeout, status);
}

TEST_F(Controller_F, VirtualEntityLoadTalkerFailedLegacyName)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::IgnoreAEMSanityChecks, la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics };

	auto& controller = getController();
	auto const [error, message] = controller.loadVirtualEntityFromJson("data/EntityTalkerFailedLegacyName.json", flags);
	EXPECT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	// Get the entity
	auto const entity = controller.getControlledEntityGuard(la::avdecc::UniqueIdentifier{ 0x001B92FFFF000001 });
	ASSERT_TRUE(!!entity);

	// Check if device is Milan compatible
	EXPECT_TRUE(entity->getCompatibilityFlags().test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::Milan));

	// Get StreamInputNode
	auto const& streamNode = entity->getStreamInputNode(entity->getCurrentConfigurationIndex(), la::avdecc::entity::model::StreamIndex{ 0u });
	ASSERT_TRUE(!!streamNode.dynamicModel.streamDynamicInfo);

	auto const& streamDynamicInfo = *streamNode.dynamicModel.streamDynamicInfo;

	// Check SRP registration failed flag is set (legacy conversion from hasTalkerFailed)
	EXPECT_TRUE(streamDynamicInfo.hasSrpRegistrationFailed);

	// Check the StreamFormatValid flag is set (to make sure the flags are not all zero)
	EXPECT_TRUE(streamDynamicInfo._streamInfoFlags.test(la::avdecc::entity::StreamInfoFlag::StreamFormatValid));

	// Check the flag is also set in the bitfield (also a legacy conversion)
	EXPECT_TRUE(streamDynamicInfo._streamInfoFlags.test(la::avdecc::entity::StreamInfoFlag::SrpRegistrationFailed));
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
	//auto status = commandResultPromise.get_future().wait_for(std::chrono::seconds(2));
	//ASSERT_NE(std::future_status::timeout, status);
}

namespace
{
class Builder : public la::avdecc::controller::model::DefaultedVirtualEntityBuilder
{
public:
	using StreamFormatChooser = std::function<la::avdecc::entity::model::StreamFormat(la::avdecc::entity::model::StreamNodeStaticModel const& staticModel)>;
	Builder(la::avdecc::controller::ControlledEntity::CompatibilityFlags const flags, la::avdecc::entity::model::MilanVersion const& milanCompatibilityVersion, StreamFormatChooser const& streamFormatChooser) noexcept
		: _compatibilityFlags{ flags }
		, _milanCompatibilityVersion{ milanCompatibilityVersion }
		, _streamFormatChooser{ streamFormatChooser }
	{
	}

	virtual void build(la::avdecc::entity::model::EntityTree const& entityTree, la::avdecc::entity::Entity::CommonInformation& commonInformation, la::avdecc::entity::Entity::InterfacesInformation& intfcInformation) noexcept override
	{
		auto const countInputStreams = [](la::avdecc::entity::model::EntityTree const& entityTree)
		{
			// Very crude and shouldn't be considered a good example
			auto count = size_t{ 0u };
			if (entityTree.configurationTrees.empty())
			{
				return count;
			}
			return entityTree.configurationTrees.begin()->second.streamInputModels.size();
		};
		commonInformation.entityID = la::avdecc::UniqueIdentifier{ 0x0102030405060708 };
		//commonInformation.entityModelID = la::avdecc::UniqueIdentifier{ 0x1122334455667788 };
		commonInformation.entityCapabilities = la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported };
		//commonInformation.talkerStreamSources = 0u;
		//commonInformation.talkerCapabilities = {};
		commonInformation.listenerStreamSinks = static_cast<decltype(commonInformation.listenerStreamSinks)>(countInputStreams(entityTree));
		commonInformation.listenerCapabilities = la::avdecc::entity::ListenerCapabilities{ la::avdecc::entity::ListenerCapability::Implemented };
		//commonInformation.controllerCapabilities = {};
		commonInformation.identifyControlIndex = la::avdecc::entity::model::ControlIndex{ 0u };
		//commonInformation.associationID = std::nullopt;

		auto const interfaceInfo = la::avdecc::entity::Entity::InterfaceInformation{ la::networkInterface::MacAddress{ 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 }, 31u, 0u, std::nullopt, std::nullopt };
		intfcInformation[la::avdecc::entity::Entity::GlobalAvbInterfaceIndex] = interfaceInfo;
	}
	virtual void build(la::avdecc::controller::ControlledEntity::CompatibilityFlags& compatibilityFlags, la::avdecc::entity::model::MilanVersion& milanCompatibilityVersion) noexcept override
	{
		for (auto const f : _compatibilityFlags)
		{
			compatibilityFlags.set(f);
		}
		milanCompatibilityVersion = _milanCompatibilityVersion;
	}
	virtual void build(la::avdecc::entity::model::MilanInfo& milanInfo, la::avdecc::entity::model::MilanDynamicState& /*milanDynamicState*/) noexcept override
	{
		milanInfo.protocolVersion = 1;
		milanInfo.specificationVersion = _milanCompatibilityVersion;
	}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::EntityNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::EntityNodeDynamicModel& dynamicModel) noexcept override
	{
		dynamicModel.entityName = la::avdecc::entity::model::AvdeccFixedString{ "Test entity" };
		dynamicModel.currentConfiguration = ActiveConfigurationIndex;
	}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const descriptorIndex, la::avdecc::entity::model::ConfigurationNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::ConfigurationNodeDynamicModel& dynamicModel) noexcept override
	{
		// Set active configuration
		if (descriptorIndex == ActiveConfigurationIndex)
		{
			dynamicModel.isActiveConfiguration = true;
		}
		_isConfigurationActive = dynamicModel.isActiveConfiguration;
	}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AudioUnitIndex const /*descriptorIndex*/, la::avdecc::entity::model::AudioUnitNodeStaticModel const& staticModel, la::avdecc::entity::model::AudioUnitNodeDynamicModel& dynamicModel) noexcept override
	{
		// Only process active configuration
		if (_isConfigurationActive)
		{
			// Choose the first sampling rate
			dynamicModel.currentSamplingRate = staticModel.samplingRates.empty() ? la::avdecc::entity::model::SamplingRate{} : *staticModel.samplingRates.begin();
		}
	}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*descriptorIndex*/, la::avdecc::entity::model::StreamNodeStaticModel const& staticModel, la::avdecc::entity::model::StreamInputNodeDynamicModel& dynamicModel) noexcept override
	{
		// Only process active configuration
		if (_isConfigurationActive)
		{
			// Choose the first stream format
			dynamicModel.streamFormat = _streamFormatChooser(staticModel);
		}
	}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*descriptorIndex*/, la::avdecc::entity::model::StreamNodeStaticModel const& staticModel, la::avdecc::entity::model::StreamOutputNodeDynamicModel& dynamicModel) noexcept override
	{
		// Only process active configuration
		if (_isConfigurationActive)
		{
			// Choose the first stream format
			dynamicModel.streamFormat = _streamFormatChooser(staticModel);
		}
	}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvbInterfaceIndex const /*descriptorIndex*/, la::avdecc::entity::model::AvbInterfaceNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::AvbInterfaceNodeDynamicModel& dynamicModel) noexcept override
	{
		// Only process active configuration
		if (_isConfigurationActive)
		{
			// Set the macAddress
			dynamicModel.macAddress = la::networkInterface::MacAddress{ 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 };
		}
	}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ControlIndex const /*descriptorIndex*/, la::avdecc::entity::model::DescriptorType const /*attachedTo*/, la::avdecc::entity::model::ControlNodeStaticModel const& staticModel, la::avdecc::entity::model::ControlNodeDynamicModel& dynamicModel) noexcept override
	{
		// Only process active configuration
		if (_isConfigurationActive)
		{
			// Identify control
			if (staticModel.controlType == la::avdecc::UniqueIdentifier{ la::avdecc::utils::to_integral(la::avdecc::entity::model::StandardControlType::Identify) })
			{
				auto values = la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::uint8_t>>{};
				values.addValue({ std::uint8_t{ 0x00 } });
				dynamicModel.values = la::avdecc::entity::model::ControlValues{ values };
			}
		}
	}

private:
	static auto constexpr ActiveConfigurationIndex = la::avdecc::entity::model::ConfigurationIndex{ 0u };
	bool _isConfigurationActive{ false };
	la::avdecc::controller::ControlledEntity::CompatibilityFlags _compatibilityFlags{};
	la::avdecc::entity::model::MilanVersion _milanCompatibilityVersion{};
	StreamFormatChooser _streamFormatChooser{};
};
} // namespace

inline std::tuple<la::avdecc::jsonSerializer::DeserializationError, std::string> doCreateVirtualEntityFromEntityModelFile(Controller_F* self, std::string const& fileName, Builder::StreamFormatChooser const& streamFormatChooser)
{
	auto const CompatibilityFlags = la::avdecc::controller::ControlledEntity::CompatibilityFlags{ la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221, la::avdecc::controller::ControlledEntity::CompatibilityFlag::Milan };

	auto builder = Builder{ CompatibilityFlags, la::avdecc::entity::model::MilanVersion{ 1, 0 }, streamFormatChooser };

	auto& controller = self->getController();
	return controller.createVirtualEntityFromEntityModelFile(fileName, &builder, false);
}

inline void doValidateVirtualEntityFromEntityModelFile(Controller_F* self, la::avdecc::controller::ControlledEntity::CompatibilityFlags const compatibilityFlags)
{
	auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x0102030405060708 };

	// Validate entity
	auto& controller = self->getController();
	auto const entity = controller.getControlledEntityGuard(EntityID);
	ASSERT_TRUE(!!entity);

	// Check EntityID
	EXPECT_EQ(EntityID, entity->getEntity().getEntityID());

	// Check compatibility flags
	EXPECT_EQ(compatibilityFlags, entity->getCompatibilityFlags());
}

TEST_F(Controller_F, VirtualEntityFromEntityModelFileV1)
{
	auto const [error, message] = doCreateVirtualEntityFromEntityModelFile(this, "data/SimpleEntityModelV1.json",
		[](la::avdecc::entity::model::StreamNodeStaticModel const& staticModel)
		{
			// Choose the first stream format
			return *staticModel.formats.begin();
		});
	ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	doValidateVirtualEntityFromEntityModelFile(this, la::avdecc::controller::ControlledEntity::CompatibilityFlags{ la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221, la::avdecc::controller::ControlledEntity::CompatibilityFlag::Milan });

	// Serialize the virtual entity
	{
		auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x0102030405060708 };
		auto& controller = getController();
		auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDiagnostics /*, la::avdecc::entity::model::jsonSerializer::Flag::BinaryFormat*/ };
		controller.serializeControlledEntityAsJson(EntityID, "OutputVirtualEntity.json", flags, "Unit Test");
	}
}

TEST_F(Controller_F, VirtualEntityFromEntityModelFileV2)
{
	auto const [error, message] = doCreateVirtualEntityFromEntityModelFile(this, "data/SimpleEntityModelV2.json",
		[](la::avdecc::entity::model::StreamNodeStaticModel const& staticModel)
		{
			// Choose the first stream format
			return *staticModel.formats.begin();
		});
	ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	EXPECT_STREQ("", message.c_str());

	doValidateVirtualEntityFromEntityModelFile(this, la::avdecc::controller::ControlledEntity::CompatibilityFlags{ la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221, la::avdecc::controller::ControlledEntity::CompatibilityFlag::Milan });

	// Serialize the virtual entity
	{
		auto constexpr EntityID = la::avdecc::UniqueIdentifier{ 0x0102030405060708 };
		auto& controller = getController();
		auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDiagnostics /*, la::avdecc::entity::model::jsonSerializer::Flag::BinaryFormat*/ };
		controller.serializeControlledEntityAsJson(EntityID, "OutputVirtualEntity.json", flags, "Unit Test");
	}
}

TEST_F(Controller_F, VirtualEntityFromEntityModelFile_InvalidFormat)
{
	auto const [error, message] = doCreateVirtualEntityFromEntityModelFile(this, "data/SimpleEntityModelV2.json",
		[](la::avdecc::entity::model::StreamNodeStaticModel const& /*staticModel*/)
		{
			// Invalid Format
			return la::avdecc::entity::model::StreamFormat{};
		});
	ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::MissingInformation, error);
}

TEST_F(Controller_F, VirtualEntityFromEntityModelFile_UpToBit_PassUpToBitFormat)
{
	auto const [error, message] = doCreateVirtualEntityFromEntityModelFile(this, "data/EntityModel_UpToBit.json",
		[](la::avdecc::entity::model::StreamNodeStaticModel const& staticModel)
		{
			auto const firstFormat = *staticModel.formats.begin();
			auto const sfi = la::avdecc::entity::model::StreamFormatInfo::create(firstFormat);
			EXPECT_TRUE(sfi->isUpToChannelsCount());
			return firstFormat;
		});
	ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::MissingInformation, error);
}

TEST_F(Controller_F, VirtualEntityFromEntityModelFile_UpToBit_PassAdaptedFormat)
{
	auto const [error, message] = doCreateVirtualEntityFromEntityModelFile(this, "data/EntityModel_UpToBit.json",
		[](la::avdecc::entity::model::StreamNodeStaticModel const& staticModel)
		{
			auto const firstFormat = *staticModel.formats.begin();
			auto const sfi = la::avdecc::entity::model::StreamFormatInfo::create(firstFormat);
			EXPECT_TRUE(sfi->isUpToChannelsCount());
			auto const adaptedFormat = sfi->getAdaptedStreamFormat(sfi->getChannelsCount());
			EXPECT_TRUE(adaptedFormat.isValid());
			return adaptedFormat;
		});
	ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
}

TEST_F(Controller_F, VirtualEntityFromEntityModelFile_UpToBit_PassAboveUpToFormat)
{
	auto const [error, message] = doCreateVirtualEntityFromEntityModelFile(this, "data/EntityModel_UpToBit.json",
		[](la::avdecc::entity::model::StreamNodeStaticModel const& staticModel)
		{
			auto const firstFormat = *staticModel.formats.begin();
			auto const sfi = la::avdecc::entity::model::StreamFormatInfo::create(firstFormat);
			EXPECT_TRUE(sfi->isUpToChannelsCount());
			EXPECT_EQ(8u, sfi->getChannelsCount());
			return la::avdecc::entity::model::StreamFormat{ 0x020702200400C000 }; // 16 channels
		});
	ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::MissingInformation, error);
}

TEST_F(Controller_F, VirtualEntityFromEntityModelFile_NotUpToBit_PassAdaptedFormat)
{
	auto const [error, message] = doCreateVirtualEntityFromEntityModelFile(this, "data/EntityModel_NotUpToBit.json",
		[](la::avdecc::entity::model::StreamNodeStaticModel const& staticModel)
		{
			auto const firstFormat = *staticModel.formats.begin();
			auto const sfi = la::avdecc::entity::model::StreamFormatInfo::create(firstFormat);
			EXPECT_FALSE(sfi->isUpToChannelsCount());
			auto const adaptedFormat = sfi->getAdaptedStreamFormat(sfi->getChannelsCount());
			EXPECT_TRUE(adaptedFormat.isValid());
			return adaptedFormat;
		});
	ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
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
		auto status = fut.wait_for(std::chrono::seconds(2));
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
		auto status = fut.wait_for(std::chrono::seconds(2));
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
		auto status = fut.wait_for(std::chrono::seconds(2));
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
		auto status = fut.wait_for(std::chrono::seconds(2));
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
		auto status = fut.wait_for(std::chrono::seconds(2));
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
		auto status = fut.wait_for(std::chrono::seconds(2));
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
		auto status = fut.wait_for(std::chrono::seconds(2));
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
		auto status = fut.wait_for(std::chrono::seconds(2));
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
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResultKind::Valid, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::uint8_t>>{ { { 0u } } } }).kind);

		// Expect to pass ControlValues validation with a value set to maximum
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResultKind::Valid, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::uint8_t>>{ { { 255u } } } }).kind);
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
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResultKind::InvalidValues, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, {}).kind);

		// Expect to have InvalidValues validation result with static values instead of dynamic values
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResultKind::InvalidValues, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueStatic<std::uint8_t>>{} }).kind);

		// Expect to have InvalidValues validation result with a different type of dynamic values
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResultKind::InvalidValues, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::int8_t>>{} }).kind);

		// Expect to have InvalidValues validation result with a different count of values
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResultKind::InvalidValues, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::uint8_t>>{} }).kind);

		// Expect to have InvalidValues validation result with a value not multiple of Step for LinearValues
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResultKind::InvalidValues, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, la::avdecc::entity::model::ControlValues{ la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::uint8_t>>{ { { 1u } } } }).kind);
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
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResultKind::CurrentValueOutOfRange, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, controlNode.dynamicModel.values).kind);
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
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResultKind::CurrentValueOutOfRange, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, controlNode.dynamicModel.values).kind);
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
		EXPECT_EQ(la::avdecc::controller::ControllerImpl::DynamicControlValuesValidationResultKind::CurrentValueOutOfRange, c.validateControlValues(EntityID, ControlIndex, controlNode.staticModel.controlType, staticValues.getType(), staticValues, controlNode.dynamicModel.values).kind);
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
static auto constexpr Entity0A = la::avdecc::UniqueIdentifier{ 0x000000000000000A }; // CD.0 on Internal
static auto constexpr Entity0B = la::avdecc::UniqueIdentifier{ 0x000000000000000B }; // CD.0 on Internal
static auto constexpr Entity0C = la::avdecc::UniqueIdentifier{ 0x000000000000000C }; // CD.0 on SI.MCRF connected to E0A
static auto constexpr Entity1C = la::avdecc::UniqueIdentifier{ 0x000000000000001C }; // CD.0 on SI.MCRF connected to E0B
static auto constexpr Entity0D = la::avdecc::UniqueIdentifier{ 0x000000000000000D }; // SI.0 connected to E0C.0	 // CD.0 on SI.0

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

TEST_F(MediaClockModel_F, StreamInput_ReplaceTalkerForSingleEntity)
{
	loadEntityFile("data/MediaClockModel/Entity_0x0A.json");
	loadEntityFile("data/MediaClockModel/Entity_0x0B.json");
	loadEntityFile("data/MediaClockModel/Entity_0x0C.json");

	try
	{
		auto& c = getControllerImpl();
		// Check initial state
		{
			auto const& e = *c.getControlledEntityGuard(Entity0C);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity0C, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity0A, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
		}

		// Expect Controller::Observer::onMediaClockChainChanged() to be called
		registerMockObserver();
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity0C).get(), la::avdecc::entity::model::ClockDomainIndex{ 0u }, ::testing::_));

		// Replace connection of MCRF to entity 0B stream
		c.handleListenerStreamStateNotification(la::avdecc::entity::model::StreamIdentification{ Entity0B, la::avdecc::entity::model::StreamIndex{ 2u } }, la::avdecc::entity::model::StreamIdentification{ Entity0C, la::avdecc::entity::model::StreamIndex{ 2u } }, true, la::avdecc::entity::ConnectionFlags{}, true);

		// Validate chain has been updated
		{
			auto const& e = *c.getControlledEntityGuard(Entity0C);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity0C, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity0B, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
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

TEST_F(MediaClockModel_F, StreamInput_ReplaceTalkerForMiddleChainEntity)
{
	/*	Initial state:
			Entity C CS:CRF connected to SO:CRF from Entity A
			Entity 1 CS:SI connected to SO:1 from Entity C
		Then Replace Clock Source of Entity C with Entity B SO:CRF
		Both entities C and D should reflect the Media clock change
	*/

	loadEntityFile("data/MediaClockModel/Entity_0x0A.json");
	loadEntityFile("data/MediaClockModel/Entity_0x0B.json");
	loadEntityFile("data/MediaClockModel/Entity_0x0C.json");
	loadEntityFile("data/MediaClockModel/Entity_0x0D.json");

	try
	{
		auto& c = getControllerImpl();
		// Check initial state for Entity 0C
		{
			auto const& e = *c.getControlledEntityGuard(Entity0C);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity0C, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity0A, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
		}

		// Check initial state for Entity 0D
		{
			auto const& e = *c.getControlledEntityGuard(Entity0D);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(3u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity0D, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity0C, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[2];
				EXPECT_EQ(Entity0A, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
		}

		// Expect Controller::Observer::onMediaClockChainChanged() to be called
		registerMockObserver();
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity0D).get(), la::avdecc::entity::model::ClockDomainIndex{ 0u }, ::testing::_));
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity0C).get(), la::avdecc::entity::model::ClockDomainIndex{ 0u }, ::testing::_));

		// Replace connection of MCRF to entity 0B stream
		c.handleListenerStreamStateNotification(la::avdecc::entity::model::StreamIdentification{ Entity0B, la::avdecc::entity::model::StreamIndex{ 2u } }, la::avdecc::entity::model::StreamIdentification{ Entity0C, la::avdecc::entity::model::StreamIndex{ 2u } }, true, la::avdecc::entity::ConnectionFlags{}, true);

		// Validate chain has been updated for Entity 0C
		{
			auto const& e = *c.getControlledEntityGuard(Entity0C);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(2u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity0C, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity0B, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
		}

		// Validate chain has been updated for Entity 0D
		{
			auto const& e = *c.getControlledEntityGuard(Entity0D);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(3u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity0D, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity0C, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[2];
				EXPECT_EQ(Entity0B, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
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

TEST_F(MediaClockModel_F, StreamInput_ReplaceTalkerForLastChainEntity)
{
	/*	Initial state:
			Entity C CS:CRF connected to SO:CRF from Entity A
			Entity 1C CS:CRF connected to SO:CRF from Entity B
			Entity D CS:SI connected to SO:0 from Entity C
		Then Replace Clock Source of Entity D with Entity 1C SO:0 from entity 1C
	*/

	loadEntityFile("data/MediaClockModel/Entity_0x0A.json");
	loadEntityFile("data/MediaClockModel/Entity_0x0B.json");
	loadEntityFile("data/MediaClockModel/Entity_0x0C.json");
	loadEntityFile("data/MediaClockModel/Entity_0x1C.json");
	loadEntityFile("data/MediaClockModel/Entity_0x0D.json");

	try
	{
		auto& c = getControllerImpl();
		// Check initial state for Entity 0D
		{
			auto const& e = *c.getControlledEntityGuard(Entity0D);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(3u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity0D, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity0C, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[2];
				EXPECT_EQ(Entity0A, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(std::nullopt, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamOutputIndex);
			}
		}

		// Expect Controller::Observer::onMediaClockChainChanged() to be called
		registerMockObserver();
		EXPECT_CALL(*this, onMediaClockChainChanged(::testing::_, c.getControlledEntityGuard(Entity0D).get(), la::avdecc::entity::model::ClockDomainIndex{ 0u }, ::testing::_));

		// Replace connection of SI to entity 1C stream
		c.handleListenerStreamStateNotification(la::avdecc::entity::model::StreamIdentification{ Entity1C, la::avdecc::entity::model::StreamIndex{ 0u } }, la::avdecc::entity::model::StreamIdentification{ Entity0D, la::avdecc::entity::model::StreamIndex{ 0u } }, true, la::avdecc::entity::ConnectionFlags{}, true);

		// Validate chain has been updated for Entity 0D
		{
			auto const& e = *c.getControlledEntityGuard(Entity0D);
			auto const& node = e.getClockDomainNode(la::avdecc::entity::model::ConfigurationIndex{ 0u }, la::avdecc::entity::model::ClockDomainIndex{ 0u });
			ASSERT_EQ(3u, node.mediaClockChain.size());
			{
				auto const& n = node.mediaClockChain[0];
				EXPECT_EQ(Entity0D, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamInputIndex);
				EXPECT_EQ(std::nullopt, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[1];
				EXPECT_EQ(Entity1C, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 3u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::StreamInput, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 2u }, n.streamInputIndex);
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, n.streamOutputIndex);
			}
			{
				auto const& n = node.mediaClockChain[2];
				EXPECT_EQ(Entity0B, n.entityID);
				EXPECT_EQ(la::avdecc::entity::model::ClockDomainIndex{ 0u }, n.clockDomainIndex);
				EXPECT_EQ(la::avdecc::entity::model::ClockSourceIndex{ 0u }, n.clockSourceIndex);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Type::Internal, n.type);
				EXPECT_EQ(la::avdecc::controller::model::MediaClockChainNode::Status::Active, n.status);
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
		c.updateClockSource(*c.getControlledEntityImplGuard(Entity01, true, false), la::avdecc::entity::model::ClockDomainIndex{ 0u }, la::avdecc::entity::model::ClockSourceIndex{ 1u }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

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

#ifdef ENABLE_AVDECC_FEATURE_CBR
static auto const Mappings_Identity_One = la::avdecc::entity::model::AudioMappings{ la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 0u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } } };
static auto const Mappings_Identity_Two = la::avdecc::entity::model::AudioMappings{ la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 0u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } }, la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 0u }, std::uint16_t{ 1u }, la::avdecc::entity::model::ClusterIndex{ 1u }, std::uint16_t{ 0u } } };
static auto constexpr ListenerClusterIdentification = la::avdecc::controller::model::ClusterIdentification{ la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } };
static auto constexpr ListenerClusterIdentification2 = la::avdecc::controller::model::ClusterIdentification{ la::avdecc::entity::model::ClusterIndex{ 1u }, std::uint16_t{ 0u } };
static auto constexpr TalkerClusterIdentification = la::avdecc::controller::model::ClusterIdentification{ la::avdecc::entity::model::ClusterIndex{ 80u }, std::uint16_t{ 0u } };
static auto constexpr TalkerClusterIdentification2 = la::avdecc::controller::model::ClusterIdentification{ la::avdecc::entity::model::ClusterIndex{ 81u }, std::uint16_t{ 0u } };
static auto constexpr TalkerStreamIdentification = la::avdecc::entity::model::StreamIdentification{ Entity02, 0u };
static auto constexpr TalkerStreamIdentification4 = la::avdecc::entity::model::StreamIdentification{ Entity04, 0u };
static auto constexpr ListenerStreamIdentification = la::avdecc::entity::model::StreamIdentification{ Entity01, 0u };
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
static auto constexpr Entity05_R = la::avdecc::UniqueIdentifier{ 0x0000000000000005 }; // 12X Redundant
static auto constexpr Entity06_R = la::avdecc::UniqueIdentifier{ 0x0000000000000006 }; // P1 Redundant
static auto constexpr Entity07_R = la::avdecc::UniqueIdentifier{ 0x0000000000000007 }; // 12X Redundant - Connected to Entity08_R - RedundantListenerMappings_Identity_One set
static auto constexpr Entity08_R = la::avdecc::UniqueIdentifier{ 0x0000000000000008 }; // P1 Redundant - RedundantTalkerMappings_Identity_One set
static auto const RedundantListenerMappings_Identity_One = la::avdecc::entity::model::AudioMappings{ la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 0u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } }, la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 1u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } } };
static auto const RedundantTalkerMappings_Identity_One = la::avdecc::entity::model::AudioMappings{ la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 0u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } }, la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 2u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } } };
static auto constexpr RedundantTalkerClusterIdentification = la::avdecc::controller::model::ClusterIdentification{ la::avdecc::entity::model::ClusterIndex{ 8u }, std::uint16_t{ 0u } };
static auto constexpr TalkerPrimaryStreamIdentification6 = la::avdecc::entity::model::StreamIdentification{ Entity06_R, 0u };
static auto constexpr TalkerSecondaryStreamIdentification6 = la::avdecc::entity::model::StreamIdentification{ Entity06_R, 2u };
static auto constexpr TalkerPrimaryStreamIdentification8 = la::avdecc::entity::model::StreamIdentification{ Entity08_R, 0u };
static auto constexpr TalkerSecondaryStreamIdentification8 = la::avdecc::entity::model::StreamIdentification{ Entity08_R, 2u };
static auto constexpr ListenerPrimaryStreamIdentification5 = la::avdecc::entity::model::StreamIdentification{ Entity05_R, 0u };
static auto constexpr ListenerSecondaryStreamIdentification5 = la::avdecc::entity::model::StreamIdentification{ Entity05_R, 1u };
static auto constexpr ListenerPrimaryStreamIdentification7 = la::avdecc::entity::model::StreamIdentification{ Entity07_R, 0u };
static auto constexpr ListenerSecondaryStreamIdentification7 = la::avdecc::entity::model::StreamIdentification{ Entity07_R, 1u };
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
#endif // ENABLE_AVDECC_FEATURE_CBR

namespace
{
class ChannelConnection_F : public ::testing::Test, public la::avdecc::controller::Controller::DefaultedObserver
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

	void loadEntityFile(std::string const& filePath) noexcept
	{
		auto const [error, msg] = _controller->loadVirtualEntityFromJson(filePath, la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessCompatibility, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan, la::avdecc::entity::model::jsonSerializer::Flag::ProcessState, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStatistics, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDiagnostics });
		ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	}

	void checkAllConnectionsDisconnected(la::avdecc::controller::model::ChannelConnections const& connections)
	{
		// Check all connections are fully disconnected (ie. no listener mappings, no connection, no talker mappings)
		for (auto const& [clusterId, channelId] : connections)
		{
			EXPECT_EQ(la::avdecc::controller::model::ChannelIdentification{}, channelId);
			EXPECT_FALSE(channelId.isConnected());
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
			EXPECT_FALSE(channelId.isPartiallyConnected());
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
		}
	}

	// la::avdecc::controller::Controller::Observer Mock overrides
	MOCK_METHOD(void, onStreamInputConnectionChanged, (la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInputConnectionInfo const& /*info*/, bool const /*changedByOther*/), (noexcept, override));
	MOCK_METHOD(void, onChannelInputConnectionChanged, (la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ClusterIdentification const& /*clusterIdentification*/, la::avdecc::controller::model::ChannelIdentification const& /*talkerChannel*/), (noexcept, override));
	MOCK_METHOD(void, onStreamPortInputAudioMappingsChanged, (la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/), (noexcept, override));
	MOCK_METHOD(void, onStreamPortOutputAudioMappingsChanged, (la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/), (noexcept, override));

private:
	la::avdecc::controller::Controller::UniquePointer _controller{ nullptr, nullptr };
};
} // namespace

#ifdef ENABLE_AVDECC_FEATURE_CBR
TEST_F(ChannelConnection_F, NoConnection)
{
	auto& c = getControllerImpl();
	// Expect Controller::Observer::onChannelInputConnectionChanged() NOT to be called
	registerMockObserver();
	EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, c.getControlledEntityGuard(Entity01).get(), ::testing::_, ::testing::_)).Times(0);

	loadEntityFile("data/ChannelConnection/Entity_0x01.json");
	loadEntityFile("data/ChannelConnection/Entity_0x02.json");

	try
	{
		{
			auto const& e = *c.getControlledEntityGuard(Entity01);
			auto const& connections = e.getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& e = *c.getControlledEntityGuard(Entity02);
			auto const& connections = e.getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, ReplaceMappings)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x01.json");
	loadEntityFile("data/ChannelConnection/Entity_0x02.json");

	try
	{
		auto e1 = c.getControlledEntityImplGuard(Entity01, true, false);
		auto e2 = c.getControlledEntityImplGuard(Entity02, true, false);
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when adding listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e2.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(0);

			// Add listener mappings
			c.updateStreamPortInputAudioMappingsAdded(*e1, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when replacing listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e2.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(0);

			// Replace listener mappings
			c.updateStreamPortInputAudioMappingsAdded(*e1, la::avdecc::entity::model::StreamPortIndex{ 0u }, la::avdecc::entity::model::AudioMappings{ la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 1u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } } }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterAddMappings)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x01.json");
	loadEntityFile("data/ChannelConnection/Entity_0x02.json");

	try
	{
		auto e1 = c.getControlledEntityImplGuard(Entity01, true, false);
		auto e2 = c.getControlledEntityImplGuard(Entity02, true, false);
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when adding listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e2.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(0);

			// Add listener mappings
			c.updateStreamPortInputAudioMappingsAdded(*e1, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}

		// Expect onChannelInputConnectionChanged() NOT to be called when just adding mappings, only onStreamPortOutputAudioMappingsChanged() - Because no stream connection
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ::testing::_, ::testing::_)).Times(0);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(0);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e2.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Add Talker mappings
			c.updateStreamPortOutputAudioMappingsAdded(*e2, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings, the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterAddListenerMappingsAndConnectStream)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x01.json");
	loadEntityFile("data/ChannelConnection/Entity_0x02.json");

	try
	{
		auto e1 = c.getControlledEntityImplGuard(Entity01, true, false);
		auto e2 = c.getControlledEntityImplGuard(Entity02, true, false);
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when adding listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e2.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(0);

			// Add listener mappings
			c.updateStreamPortInputAudioMappingsAdded(*e1, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}

		// Expect onChannelInputConnectionChanged() to be called when connecting (even without talker mappings)
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamIndex{ 0u }, la::avdecc::entity::model::StreamInputConnectionInfo{ TalkerStreamIdentification, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected }, false)).Times(1);

			// Connect stream
			c.handleListenerStreamStateNotification(TalkerStreamIdentification, ListenerStreamIdentification, true, {}, false);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerStreamIdentification, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterAddListenerMappingsAndTalkerMappingsAndConnectStream)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x01.json");
	loadEntityFile("data/ChannelConnection/Entity_0x02.json");

	try
	{
		auto e1 = c.getControlledEntityImplGuard(Entity01, true, false);
		auto e2 = c.getControlledEntityImplGuard(Entity02, true, false);
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when adding listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e2.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(0);

			// Add listener mappings
			c.updateStreamPortInputAudioMappingsAdded(*e1, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}

		// Expect onChannelInputConnectionChanged() NOT to be called when just adding mappings, only onStreamPortOutputAudioMappingsChanged()
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ::testing::_, ::testing::_)).Times(0);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(0);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e2.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Add Talker mappings
			c.updateStreamPortOutputAudioMappingsAdded(*e2, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}

		// Expect onChannelInputConnectionChanged() to be called when connecting
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamIndex{ 0u }, la::avdecc::entity::model::StreamInputConnectionInfo{ TalkerStreamIdentification, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected }, false)).Times(1);

			// Connect stream
			c.handleListenerStreamStateNotification(TalkerStreamIdentification, ListenerStreamIdentification, true, {}, false);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerStreamIdentification, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_EQ(TalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_TRUE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterConnectStreamAndAddListenerMappingsAndTalkerMappings)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x01.json");
	loadEntityFile("data/ChannelConnection/Entity_0x02.json");

	try
	{
		auto e1 = c.getControlledEntityImplGuard(Entity01, true, false);
		auto e2 = c.getControlledEntityImplGuard(Entity02, true, false);
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Connect stream first (without mappings)
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ::testing::_, ::testing::_)).Times(0);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamIndex{ 0u }, la::avdecc::entity::model::StreamInputConnectionInfo{ TalkerStreamIdentification, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected }, false)).Times(1);

			c.handleListenerStreamStateNotification(TalkerStreamIdentification, ListenerStreamIdentification, true, {}, false);

			unregisterMockObserver();

			// Listener should still be fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}

		// Add listener mappings - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortInputAudioMappingsAdded(*e1, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerStreamIdentification, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}

		// Add talker mappings - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e2.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortOutputAudioMappingsAdded(*e2, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerStreamIdentification, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_EQ(TalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_TRUE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterAddTalkerMappingsAndListenerMappingsAndConnectStream)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x01.json");
	loadEntityFile("data/ChannelConnection/Entity_0x02.json");

	try
	{
		auto e1 = c.getControlledEntityImplGuard(Entity01, true, false);
		auto e2 = c.getControlledEntityImplGuard(Entity02, true, false);
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Add talker mappings first
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ::testing::_, ::testing::_)).Times(0);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e2.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortOutputAudioMappingsAdded(*e2, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should still be fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}

		// Add listener mappings - should trigger onStreamPortInputAudioMappingsChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortInputAudioMappingsAdded(*e1, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}

		// Connect stream - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamIndex{ 0u }, la::avdecc::entity::model::StreamInputConnectionInfo{ TalkerStreamIdentification, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected }, false)).Times(1);

			c.handleListenerStreamStateNotification(TalkerStreamIdentification, ListenerStreamIdentification, true, {}, false);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerStreamIdentification, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_EQ(TalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_TRUE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, MultipleChannelConnections)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x01.json");
	loadEntityFile("data/ChannelConnection/Entity_0x02.json");

	try
	{
		auto e1 = c.getControlledEntityImplGuard(Entity01, true, false);
		auto e2 = c.getControlledEntityImplGuard(Entity02, true, false);
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Add listener mappings - should trigger onChannelInputConnectionChanged twice (one for each channel)
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification2, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortInputAudioMappingsAdded(*e1, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_Two, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				{
					auto const& channelId = connections.at(ListenerClusterIdentification2);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(std::uint16_t{ 1u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification || clusterId == ListenerClusterIdentification2)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}

		// Add talker mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ::testing::_, ::testing::_)).Times(0);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e2.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortOutputAudioMappingsAdded(*e2, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_Two, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				{
					auto const& channelId = connections.at(ListenerClusterIdentification2);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(std::uint16_t{ 1u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification || clusterId == ListenerClusterIdentification2)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}

		// Connect stream - should trigger onChannelInputConnectionChanged twice (one for each channel)
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification2, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamIndex{ 0u }, la::avdecc::entity::model::StreamInputConnectionInfo{ TalkerStreamIdentification, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected }, false)).Times(1);

			c.handleListenerStreamStateNotification(TalkerStreamIdentification, ListenerStreamIdentification, true, {}, false);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerStreamIdentification, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_EQ(TalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_TRUE(channelId.isConnected());
				}
				{
					auto const& channelId = connections.at(ListenerClusterIdentification2);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(std::uint16_t{ 1u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerStreamIdentification, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_EQ(TalkerClusterIdentification2, channelId.channelConnectionIdentification.clusterIdentification);
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_TRUE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification || clusterId == ListenerClusterIdentification2)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, DisconnectStreamRemovesChannelConnections)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x01.json");
	loadEntityFile("data/ChannelConnection/Entity_0x02.json");

	try
	{
		auto e1 = c.getControlledEntityImplGuard(Entity01, true, false);
		auto e2 = c.getControlledEntityImplGuard(Entity02, true, false);
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Setup: Add mappings and connect
		c.updateStreamPortInputAudioMappingsAdded(*e1, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
		c.updateStreamPortOutputAudioMappingsAdded(*e2, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
		c.handleListenerStreamStateNotification(TalkerStreamIdentification, ListenerStreamIdentification, true, {}, false);
		// Listener should only have connection with new listener mappings (fully connected), the other connections fully disconnected
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			{
				auto const& channelId = connections.at(ListenerClusterIdentification);
				EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
				EXPECT_EQ(TalkerStreamIdentification, channelId.channelConnectionIdentification.streamIdentification);
				EXPECT_EQ(TalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_TRUE(channelId.isConnected());
			}
			for (auto const& [clusterId, channelId] : connections)
			{
				if (clusterId == ListenerClusterIdentification)
				{
					continue;
				}
				EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
				EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.isConnected());
			}
		}
		// Talker should still be fully disconnected
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Disconnect stream - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamIndex{ 0u }, la::avdecc::entity::model::StreamInputConnectionInfo{ TalkerStreamIdentification, la::avdecc::entity::model::StreamInputConnectionInfo::State::NotConnected }, false)).Times(1);

			c.handleListenerStreamStateNotification(TalkerStreamIdentification, ListenerStreamIdentification, false, {}, false);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, RemoveListenerMappingsRemovesChannelConnections)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x01.json");
	loadEntityFile("data/ChannelConnection/Entity_0x02.json");

	try
	{
		auto e1 = c.getControlledEntityImplGuard(Entity01, true, false);
		auto e2 = c.getControlledEntityImplGuard(Entity02, true, false);
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Setup: Add mappings and connect
		c.updateStreamPortInputAudioMappingsAdded(*e1, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
		c.updateStreamPortOutputAudioMappingsAdded(*e2, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
		c.handleListenerStreamStateNotification(la::avdecc::entity::model::StreamIdentification{ Entity02, 0u }, la::avdecc::entity::model::StreamIdentification{ Entity01, 0u }, true, {}, false);
		// Listener should only have connection with new listener mappings (fully connected), the other connections fully disconnected
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			{
				auto const& channelId = connections.at(ListenerClusterIdentification);
				EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
				EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
				EXPECT_EQ(TalkerStreamIdentification, channelId.channelConnectionIdentification.streamIdentification);
				EXPECT_EQ(TalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_TRUE(channelId.isConnected());
			}
			for (auto const& [clusterId, channelId] : connections)
			{
				if (clusterId == ListenerClusterIdentification)
				{
					continue;
				}
				EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
				EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.isConnected());
			}
		}
		// Talker should still be fully disconnected
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Remove listener mappings - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortInputAudioMappingsRemoved(*e1, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should be fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
			// Talker should be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, ReplaceListenerMappingsRemovesChannelConnections)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x01.json");
	loadEntityFile("data/ChannelConnection/Entity_0x02.json");

	try
	{
		auto e1 = c.getControlledEntityImplGuard(Entity01, true, false);
		auto e2 = c.getControlledEntityImplGuard(Entity02, true, false);
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Setup: Add mappings and connect
		c.updateStreamPortInputAudioMappingsAdded(*e1, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
		c.updateStreamPortOutputAudioMappingsAdded(*e2, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
		c.handleListenerStreamStateNotification(la::avdecc::entity::model::StreamIdentification{ Entity02, 0u }, la::avdecc::entity::model::StreamIdentification{ Entity01, 0u }, true, {}, false);
		// Listener should only have connection with new listener mappings (fully connected), the other connections fully disconnected
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			{
				auto const& channelId = connections.at(ListenerClusterIdentification);
				EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
				EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
				EXPECT_EQ(TalkerStreamIdentification, channelId.channelConnectionIdentification.streamIdentification);
				EXPECT_EQ(TalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_TRUE(channelId.isConnected());
			}
			for (auto const& [clusterId, channelId] : connections)
			{
				if (clusterId == ListenerClusterIdentification)
				{
					continue;
				}
				EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
				EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.isConnected());
			}
		}
		// Talker should still be fully disconnected
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Replace listener mappings - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e1.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Replace listener mappings
			c.updateStreamPortInputAudioMappingsAdded(*e1, la::avdecc::entity::model::StreamPortIndex{ 0u }, la::avdecc::entity::model::AudioMappings{ la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 1u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } } }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, RemoveTalkerMappingsRemovesChannelConnections)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x01.json");
	loadEntityFile("data/ChannelConnection/Entity_0x02.json");

	try
	{
		auto e1 = c.getControlledEntityImplGuard(Entity01, true, false);
		auto e2 = c.getControlledEntityImplGuard(Entity02, true, false);
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Setup: Add mappings and connect
		c.updateStreamPortInputAudioMappingsAdded(*e1, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
		c.updateStreamPortOutputAudioMappingsAdded(*e2, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
		c.handleListenerStreamStateNotification(la::avdecc::entity::model::StreamIdentification{ Entity02, 0u }, la::avdecc::entity::model::StreamIdentification{ Entity01, 0u }, true, {}, false);
		// Listener should only have connection with new listener mappings (fully connected), the other connections fully disconnected
		{
			auto const& connections = e1->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			{
				auto const& channelId = connections.at(ListenerClusterIdentification);
				EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
				EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
				EXPECT_EQ(TalkerStreamIdentification, channelId.channelConnectionIdentification.streamIdentification);
				EXPECT_EQ(TalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_TRUE(channelId.isConnected());
			}
			for (auto const& [clusterId, channelId] : connections)
			{
				if (clusterId == ListenerClusterIdentification)
				{
					continue;
				}
				EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
				EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.isConnected());
			}
		}
		// Talker should still be fully disconnected
		{
			auto const& connections = e2->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Remove talker mappings - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e1.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e2.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortOutputAudioMappingsRemoved(*e2, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should have connection with listener mapping but no talker mapping (not fully connected)
			{
				auto const& connections = e1->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerStreamIdentification, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
			// Talker should be fully disconnected
			{
				auto const& connections = e2->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, LoadWithExistingConnectionListenerFirst)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x03.json");
	loadEntityFile("data/ChannelConnection/Entity_0x04.json");

	try
	{
		auto e3 = c.getControlledEntityImplGuard(Entity03, true, false);
		auto e4 = c.getControlledEntityImplGuard(Entity04, true, false);
		// Listener should only have connection (fully connected), the other connections fully disconnected
		{
			auto const& connections = e3->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			{
				auto const& channelId = connections.at(ListenerClusterIdentification);
				EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
				EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
				EXPECT_EQ(TalkerStreamIdentification4, channelId.channelConnectionIdentification.streamIdentification);
				EXPECT_EQ(TalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_TRUE(channelId.isConnected());
			}
			for (auto const& [clusterId, channelId] : connections)
			{
				if (clusterId == ListenerClusterIdentification)
				{
					continue;
				}
				EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
				EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.isConnected());
			}
		}
		// Talker should be fully disconnected
		{
			auto const& connections = e4->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Remove listener mappings - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e3.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e3.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortInputAudioMappingsRemoved(*e3, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should be fully disconnected
			{
				auto const& connections = e3->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
			// Talker should be fully disconnected
			{
				auto const& connections = e4->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, LoadWithExistingConnectionTalkerFirst)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x04.json");
	loadEntityFile("data/ChannelConnection/Entity_0x03.json");

	try
	{
		auto e3 = c.getControlledEntityImplGuard(Entity03, true, false);
		auto e4 = c.getControlledEntityImplGuard(Entity04, true, false);
		// Listener should only have connection (fully connected), the other connections fully disconnected
		{
			auto const& connections = e3->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			{
				auto const& channelId = connections.at(ListenerClusterIdentification);
				EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
				EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
				EXPECT_EQ(TalkerStreamIdentification4, channelId.channelConnectionIdentification.streamIdentification);
				EXPECT_EQ(TalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_TRUE(channelId.isConnected());
			}
			for (auto const& [clusterId, channelId] : connections)
			{
				if (clusterId == ListenerClusterIdentification)
				{
					continue;
				}
				EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
				EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.isConnected());
			}
		}
		// Talker should be fully disconnected
		{
			auto const& connections = e4->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Remove listener mappings - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e3.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e3.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortInputAudioMappingsRemoved(*e3, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should be fully disconnected
			{
				auto const& connections = e3->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
			// Talker should be fully disconnected
			{
				auto const& connections = e4->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, ListenerConnectedToOfflineTalker)
{
	auto& c = getControllerImpl();

	// Load only the listener entity (Entity03) - talker (Entity04) is offline
	loadEntityFile("data/ChannelConnection/Entity_0x03.json");

	try
	{
		auto e3 = c.getControlledEntityImplGuard(Entity03, true, false);
		// Listener should have connection with listener mapping and stream connection, but NO talker mapping (not fully connected) because talker is offline
		{
			auto const& connections = e3->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			{
				auto const& channelId = connections.at(ListenerClusterIdentification);
				EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
				EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
				EXPECT_EQ(TalkerStreamIdentification4, channelId.channelConnectionIdentification.streamIdentification);
				EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{})); // No talker mapping because talker is offline
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.isConnected()); // Not fully connected
			}
			for (auto const& [clusterId, channelId] : connections)
			{
				if (clusterId == ListenerClusterIdentification)
				{
					continue;
				}
				EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
				EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.isConnected());
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, EntityDepartingRemovesChannelConnection)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x03.json");
	loadEntityFile("data/ChannelConnection/Entity_0x04.json");

	try
	{
		auto e3 = c.getControlledEntityImplGuard(Entity03, true, false);
		auto e4 = c.getControlledEntityImplGuard(Entity04, true, false);
		// Listener should have full connection (fully connected), the other connections fully disconnected
		{
			auto const& connections = e3->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			{
				auto const& channelId = connections.at(ListenerClusterIdentification);
				EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
				EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
				EXPECT_EQ(TalkerStreamIdentification4, channelId.channelConnectionIdentification.streamIdentification);
				EXPECT_EQ(TalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_TRUE(channelId.isConnected());
			}
			for (auto const& [clusterId, channelId] : connections)
			{
				if (clusterId == ListenerClusterIdentification)
				{
					continue;
				}
				EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
				EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				EXPECT_FALSE(channelId.isConnected());
			}
		}
		// Talker should be fully disconnected
		{
			auto const& connections = e4->getChannelConnections();
			EXPECT_EQ(80u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Unload talker entity (simulating device going offline) - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e3.get(), ListenerClusterIdentification, ::testing::_)).Times(1);

			c.unloadVirtualEntity(Entity04);

			unregisterMockObserver();

			// Listener should have connection with listener mapping and stream connection, but NO talker mapping (not fully connected) because talker is offline
			{
				auto const& connections = e3->getChannelConnections();
				EXPECT_EQ(80u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerStreamIdentification4, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{})); // No talker mapping because talker is offline
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected()); // Not fully connected
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isPartiallyConnected());
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					EXPECT_FALSE(channelId.isConnected());
				}
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
TEST_F(ChannelConnection_F, AfterAddListenerRedundantMappings)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x05.json");

	try
	{
		auto e5 = c.getControlledEntityImplGuard(Entity05_R, true, false);
		{
			auto const& connections = e5->getChannelConnections();
			EXPECT_EQ(4u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when adding listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Add redundant listener mappings
			c.updateStreamPortInputAudioMappingsAdded(*e5, la::avdecc::entity::model::StreamPortIndex{ 0u }, RedundantListenerMappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e5->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					ASSERT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamChannel);
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterAddListenerRedundantMappingsInSequencePrimaryFirst) // Simulate Controller sending primary mappings only and getting notification from Entity with secondary mappings later
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x05.json");

	try
	{
		auto e5 = c.getControlledEntityImplGuard(Entity05_R, true, false);
		{
			auto const& connections = e5->getChannelConnections();
			EXPECT_EQ(4u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when adding primary listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Add primary listener mappings
			c.updateStreamPortInputAudioMappingsAdded(*e5, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e5->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					ASSERT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_TRUE((*channelId.secondaryChannelConnectionIdentification == la::avdecc::controller::model::ChannelConnectionIdentification{}));
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when adding secondary listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Add secondary listener mappings
			c.updateStreamPortInputAudioMappingsAdded(*e5, la::avdecc::entity::model::StreamPortIndex{ 0u }, la::avdecc::entity::model::AudioMappings{ la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 1u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } } }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e5->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					ASSERT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamChannel);
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterAddListenerRedundantMappingsInSequenceSecondaryFirst) // Simulate Controller sending secondary mappings only and getting notification from Entity with primary mappings later
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x05.json");

	try
	{
		auto e5 = c.getControlledEntityImplGuard(Entity05_R, true, false);
		{
			auto const& connections = e5->getChannelConnections();
			EXPECT_EQ(4u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when adding secondary listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Add secondary listener mappings
			c.updateStreamPortInputAudioMappingsAdded(*e5, la::avdecc::entity::model::StreamPortIndex{ 0u }, la::avdecc::entity::model::AudioMappings{ la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 1u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } } }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e5->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE((channelId.channelConnectionIdentification == la::avdecc::controller::model::ChannelConnectionIdentification{}));
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					ASSERT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamChannel);
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when adding primary listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Add primary listener mappings
			c.updateStreamPortInputAudioMappingsAdded(*e5, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e5->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					ASSERT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamChannel);
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterAddListenerRedundantMappingsAndConnectPrimaryStream)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x05.json");
	loadEntityFile("data/ChannelConnection/Entity_0x06.json");

	try
	{
		auto e5 = c.getControlledEntityImplGuard(Entity05_R, true, false);
		auto e6 = c.getControlledEntityImplGuard(Entity06_R, true, false);
		{
			auto const& connections = e5->getChannelConnections();
			EXPECT_EQ(4u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& connections = e6->getChannelConnections();
			EXPECT_EQ(8u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when adding listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e6.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(0);

			// Add redundant listener mappings
			c.updateStreamPortInputAudioMappingsAdded(*e5, la::avdecc::entity::model::StreamPortIndex{ 0u }, RedundantListenerMappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e5->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					ASSERT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamChannel);
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e6->getChannelConnections();
				EXPECT_EQ(8u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}

		// Expect onChannelInputConnectionChanged() to be called when connecting (even without talker mappings)
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamIndex{ 0u }, la::avdecc::entity::model::StreamInputConnectionInfo{ TalkerPrimaryStreamIdentification6, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected }, false)).Times(1);

			// Connect stream
			c.handleListenerStreamStateNotification(TalkerPrimaryStreamIdentification6, ListenerPrimaryStreamIdentification5, true, {}, false);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e5->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerPrimaryStreamIdentification6, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamChannel);
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e6->getChannelConnections();
				EXPECT_EQ(8u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterAddListenerRedundantMappingsAndConnectSecondaryStream)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x05.json");
	loadEntityFile("data/ChannelConnection/Entity_0x06.json");

	try
	{
		auto e5 = c.getControlledEntityImplGuard(Entity05_R, true, false);
		auto e6 = c.getControlledEntityImplGuard(Entity06_R, true, false);
		{
			auto const& connections = e5->getChannelConnections();
			EXPECT_EQ(4u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& connections = e6->getChannelConnections();
			EXPECT_EQ(8u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when adding listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e6.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(0);

			// Add redundant listener mappings
			c.updateStreamPortInputAudioMappingsAdded(*e5, la::avdecc::entity::model::StreamPortIndex{ 0u }, RedundantListenerMappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e5->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					ASSERT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamChannel);
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e6->getChannelConnections();
				EXPECT_EQ(8u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}

		// Expect onChannelInputConnectionChanged() to be called when connecting secondary stream
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamIndex{ 1u }, la::avdecc::entity::model::StreamInputConnectionInfo{ TalkerSecondaryStreamIdentification6, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected }, false)).Times(1);

			// Connect secondary stream
			c.handleListenerStreamStateNotification(TalkerSecondaryStreamIdentification6, ListenerSecondaryStreamIdentification5, true, {}, false);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e5->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerSecondaryStreamIdentification6, channelId.secondaryChannelConnectionIdentification->streamIdentification);
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e6->getChannelConnections();
				EXPECT_EQ(8u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterAddListenerRedundantMappingsAndConnectRedundantStreamPair)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x05.json");
	loadEntityFile("data/ChannelConnection/Entity_0x06.json");

	try
	{
		auto e5 = c.getControlledEntityImplGuard(Entity05_R, true, false);
		auto e6 = c.getControlledEntityImplGuard(Entity06_R, true, false);
		{
			auto const& connections = e5->getChannelConnections();
			EXPECT_EQ(4u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}
		{
			auto const& connections = e6->getChannelConnections();
			EXPECT_EQ(8u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when adding listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e6.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(0);

			// Add redundant listener mappings
			c.updateStreamPortInputAudioMappingsAdded(*e5, la::avdecc::entity::model::StreamPortIndex{ 0u }, RedundantListenerMappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e5->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					ASSERT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamChannel);
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e6->getChannelConnections();
				EXPECT_EQ(8u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}

		// Expect onChannelInputConnectionChanged() to be called when connecting both streams
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(2); // Called twice, once for each stream
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamIndex{ 0u }, la::avdecc::entity::model::StreamInputConnectionInfo{ TalkerPrimaryStreamIdentification6, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected }, false)).Times(1);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamIndex{ 1u }, la::avdecc::entity::model::StreamInputConnectionInfo{ TalkerSecondaryStreamIdentification6, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected }, false)).Times(1);

			// Connect primary stream
			c.handleListenerStreamStateNotification(TalkerPrimaryStreamIdentification6, ListenerPrimaryStreamIdentification5, true, {}, false);
			// Connect secondary stream
			c.handleListenerStreamStateNotification(TalkerSecondaryStreamIdentification6, ListenerSecondaryStreamIdentification5, true, {}, false);

			unregisterMockObserver();

			// Listener should only have connection with new listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e5->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerPrimaryStreamIdentification6, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerSecondaryStreamIdentification6, channelId.secondaryChannelConnectionIdentification->streamIdentification);
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
			// Talker should still be fully disconnected
			{
				auto const& connections = e6->getChannelConnections();
				EXPECT_EQ(8u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterAddListenerRedundantMappingsAndConnectPrimaryStreamAndAddTalkerRedundantMappings)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x05.json");
	loadEntityFile("data/ChannelConnection/Entity_0x06.json");

	try
	{
		auto e5 = c.getControlledEntityImplGuard(Entity05_R, true, false);
		auto e6 = c.getControlledEntityImplGuard(Entity06_R, true, false);

		// Add listener mappings and connect primary stream (similar to previous test)
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortInputAudioMappingsAdded(*e5, la::avdecc::entity::model::StreamPortIndex{ 0u }, RedundantListenerMappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();
		}

		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamIndex{ 0u }, la::avdecc::entity::model::StreamInputConnectionInfo{ TalkerPrimaryStreamIdentification6, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected }, false)).Times(1);

			c.handleListenerStreamStateNotification(TalkerPrimaryStreamIdentification6, ListenerPrimaryStreamIdentification5, true, {}, false);

			unregisterMockObserver();
		}

		// Add talker mappings - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e6.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Add redundant talker mappings (primary + secondary streams)
			c.updateStreamPortOutputAudioMappingsAdded(*e6, la::avdecc::entity::model::StreamPortIndex{ 0u }, la::avdecc::entity::model::AudioMappings{ la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 0u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } }, la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 2u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } } }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should now be partially connected (has listener mappings, stream connection, and talker mapping for primary)
			{
				auto const& connections = e5->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerPrimaryStreamIdentification6, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_EQ(RedundantTalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
					EXPECT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.secondaryChannelConnectionIdentification->streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.secondaryChannelConnectionIdentification->clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.isConnected()); // Not fully connected - secondary stream not connected
					EXPECT_TRUE(channelId.isPartiallyConnected()); // Partially connected - primary connected
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterAddListenerRedundantMappingsAndConnectSecondaryStreamAndAddTalkerRedundantMappings)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x05.json");
	loadEntityFile("data/ChannelConnection/Entity_0x06.json");

	try
	{
		auto e5 = c.getControlledEntityImplGuard(Entity05_R, true, false);
		auto e6 = c.getControlledEntityImplGuard(Entity06_R, true, false);

		// Add listener mappings and connect secondary stream
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortInputAudioMappingsAdded(*e5, la::avdecc::entity::model::StreamPortIndex{ 0u }, RedundantListenerMappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();
		}

		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamIndex{ 1u }, la::avdecc::entity::model::StreamInputConnectionInfo{ TalkerSecondaryStreamIdentification6, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected }, false)).Times(1);

			c.handleListenerStreamStateNotification(TalkerSecondaryStreamIdentification6, ListenerSecondaryStreamIdentification5, true, {}, false);

			unregisterMockObserver();
		}

		// Add talker mappings - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e6.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Add redundant talker mappings
			c.updateStreamPortOutputAudioMappingsAdded(*e6, la::avdecc::entity::model::StreamPortIndex{ 0u }, la::avdecc::entity::model::AudioMappings{ la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 0u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } }, la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 2u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } } }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should now be partially connected (has listener mappings, secondary stream connection, and talker mapping for secondary)
			{
				auto const& connections = e5->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerSecondaryStreamIdentification6, channelId.secondaryChannelConnectionIdentification->streamIdentification);
					EXPECT_EQ(RedundantTalkerClusterIdentification, channelId.secondaryChannelConnectionIdentification->clusterIdentification);
					EXPECT_FALSE(channelId.isConnected()); // Not fully connected - primary stream not connected
					EXPECT_TRUE(channelId.isPartiallyConnected()); // Partially connected - secondary connected
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterAddListenerRedundantMappingsAndConnectRedundantStreamPairAndAddTalkerRedundantMappings)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x05.json");
	loadEntityFile("data/ChannelConnection/Entity_0x06.json");

	try
	{
		auto e5 = c.getControlledEntityImplGuard(Entity05_R, true, false);
		auto e6 = c.getControlledEntityImplGuard(Entity06_R, true, false);

		// Add listener mappings and connect both streams
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortInputAudioMappingsAdded(*e5, la::avdecc::entity::model::StreamPortIndex{ 0u }, RedundantListenerMappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();
		}

		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(2);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamIndex{ 0u }, la::avdecc::entity::model::StreamInputConnectionInfo{ TalkerPrimaryStreamIdentification6, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected }, false)).Times(1);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e5.get(), la::avdecc::entity::model::StreamIndex{ 1u }, la::avdecc::entity::model::StreamInputConnectionInfo{ TalkerSecondaryStreamIdentification6, la::avdecc::entity::model::StreamInputConnectionInfo::State::Connected }, false)).Times(1);

			// Connect both streams
			c.handleListenerStreamStateNotification(TalkerPrimaryStreamIdentification6, ListenerPrimaryStreamIdentification5, true, {}, false);
			c.handleListenerStreamStateNotification(TalkerSecondaryStreamIdentification6, ListenerSecondaryStreamIdentification5, true, {}, false);

			unregisterMockObserver();
		}

		// Add talker mappings - should trigger onChannelInputConnectionChanged for full connection
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e5.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortOutputAudioMappingsChanged(::testing::_, e6.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Add redundant talker mappings
			c.updateStreamPortOutputAudioMappingsAdded(*e6, la::avdecc::entity::model::StreamPortIndex{ 0u }, la::avdecc::entity::model::AudioMappings{ la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 0u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } }, la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 2u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } } }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should now be fully connected
			{
				auto const& connections = e5->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerPrimaryStreamIdentification6, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_EQ(RedundantTalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
					EXPECT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerSecondaryStreamIdentification6, channelId.secondaryChannelConnectionIdentification->streamIdentification);
					EXPECT_EQ(RedundantTalkerClusterIdentification, channelId.secondaryChannelConnectionIdentification->clusterIdentification);
					EXPECT_TRUE(channelId.isConnected()); // Fully connected
					EXPECT_FALSE(channelId.isPartiallyConnected()); // Not partially connected when fully connected
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, DisconnectSecondaryStreamRemovesChannelConnections)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x07.json");
	loadEntityFile("data/ChannelConnection/Entity_0x08.json");

	try
	{
		auto e7 = c.getControlledEntityImplGuard(Entity07_R, true, false);
		auto e8 = c.getControlledEntityImplGuard(Entity08_R, true, false);

		// Entities should start fully connected (based on Entity_0x07.json and Entity_0x08.json)
		{
			auto const& connections = e7->getChannelConnections();
			EXPECT_EQ(4u, connections.size());
			{
				auto const& channelId = connections.at(ListenerClusterIdentification);
				EXPECT_TRUE(channelId.isConnected());
			}
		}

		// Disconnect secondary stream - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e7.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e7.get(), la::avdecc::entity::model::StreamIndex{ 1u }, la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamInputConnectionInfo::State::NotConnected }, false)).Times(1);

			// Disconnect secondary stream
			c.handleListenerStreamStateNotification(TalkerSecondaryStreamIdentification8, ListenerSecondaryStreamIdentification7, false, {}, false);

			unregisterMockObserver();

			// Listener should now be partially connected (only primary connected)
			{
				auto const& connections = e7->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(TalkerPrimaryStreamIdentification8, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_EQ(RedundantTalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
					EXPECT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_TRUE((channelId.secondaryChannelConnectionIdentification->streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.secondaryChannelConnectionIdentification->clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.isConnected()); // Not fully connected
					EXPECT_TRUE(channelId.isPartiallyConnected()); // Partially connected - primary only
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, DisconnectPrimaryStreamRemovesChannelConnections)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x07.json");
	loadEntityFile("data/ChannelConnection/Entity_0x08.json");

	try
	{
		auto e7 = c.getControlledEntityImplGuard(Entity07_R, true, false);
		auto e8 = c.getControlledEntityImplGuard(Entity08_R, true, false);

		// Entities should start fully connected
		{
			auto const& connections = e7->getChannelConnections();
			EXPECT_EQ(4u, connections.size());
			{
				auto const& channelId = connections.at(ListenerClusterIdentification);
				EXPECT_TRUE(channelId.isConnected());
			}
		}

		// Disconnect primary stream - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e7.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e7.get(), la::avdecc::entity::model::StreamIndex{ 0u }, la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamInputConnectionInfo::State::NotConnected }, false)).Times(1);

			// Disconnect primary stream
			c.handleListenerStreamStateNotification(TalkerPrimaryStreamIdentification8, ListenerPrimaryStreamIdentification7, false, {}, false);

			unregisterMockObserver();

			// Listener should now be partially connected (only secondary connected)
			{
				auto const& connections = e7->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(TalkerSecondaryStreamIdentification8, channelId.secondaryChannelConnectionIdentification->streamIdentification);
					EXPECT_EQ(RedundantTalkerClusterIdentification, channelId.secondaryChannelConnectionIdentification->clusterIdentification);
					EXPECT_FALSE(channelId.isConnected()); // Not fully connected
					EXPECT_TRUE(channelId.isPartiallyConnected()); // Partially connected - secondary only
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, DisconnectRedundantStreamPairRemovesChannelConnections)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x07.json");
	loadEntityFile("data/ChannelConnection/Entity_0x08.json");

	try
	{
		auto e7 = c.getControlledEntityImplGuard(Entity07_R, true, false);
		auto e8 = c.getControlledEntityImplGuard(Entity08_R, true, false);

		// Entities should start fully connected
		{
			auto const& connections = e7->getChannelConnections();
			EXPECT_EQ(4u, connections.size());
			{
				auto const& channelId = connections.at(ListenerClusterIdentification);
				EXPECT_TRUE(channelId.isConnected());
			}
		}

		// Disconnect both streams - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e7.get(), ListenerClusterIdentification, ::testing::_)).Times(2); // Called twice, once for each stream
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e7.get(), la::avdecc::entity::model::StreamIndex{ 0u }, la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamInputConnectionInfo::State::NotConnected }, false)).Times(1);
			EXPECT_CALL(*this, onStreamInputConnectionChanged(::testing::_, e7.get(), la::avdecc::entity::model::StreamIndex{ 1u }, la::avdecc::entity::model::StreamInputConnectionInfo{ la::avdecc::entity::model::StreamIdentification{}, la::avdecc::entity::model::StreamInputConnectionInfo::State::NotConnected }, false)).Times(1);

			// Disconnect both streams
			c.handleListenerStreamStateNotification(TalkerPrimaryStreamIdentification8, ListenerPrimaryStreamIdentification7, false, {}, false);
			c.handleListenerStreamStateNotification(TalkerSecondaryStreamIdentification8, ListenerSecondaryStreamIdentification7, false, {}, false);

			unregisterMockObserver();

			// Listener should have connections with listener mappings but no stream connections (not connected)
			{
				auto const& connections = e7->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_TRUE((channelId.secondaryChannelConnectionIdentification->streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.secondaryChannelConnectionIdentification->clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.isConnected()); // Not connected
					EXPECT_FALSE(channelId.isPartiallyConnected()); // Not partially connected either
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, LoadWithExistingConnectionRedundantListenerFirst)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x07.json");
	loadEntityFile("data/ChannelConnection/Entity_0x08.json");

	try
	{
		auto e7 = c.getControlledEntityImplGuard(Entity07_R, true, false);
		auto e8 = c.getControlledEntityImplGuard(Entity08_R, true, false);
		// Listener should only have connection (fully connected), the other connections fully disconnected
		{
			auto const& connections = e7->getChannelConnections();
			EXPECT_EQ(4u, connections.size());
			{
				auto const& channelId = connections.at(ListenerClusterIdentification);
				EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
				EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
				EXPECT_EQ(TalkerPrimaryStreamIdentification8, channelId.channelConnectionIdentification.streamIdentification);
				EXPECT_EQ(RedundantTalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
				ASSERT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_EQ(TalkerSecondaryStreamIdentification8, channelId.secondaryChannelConnectionIdentification->streamIdentification);
				EXPECT_EQ(RedundantTalkerClusterIdentification, channelId.secondaryChannelConnectionIdentification->clusterIdentification);
				EXPECT_TRUE(channelId.isConnected());
				EXPECT_FALSE(channelId.isPartiallyConnected());
			}
			for (auto const& [clusterId, channelId] : connections)
			{
				if (clusterId == ListenerClusterIdentification)
				{
					continue;
				}
				EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
				EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isConnected());
				EXPECT_FALSE(channelId.isPartiallyConnected());
			}
		}
		// Talker should be fully disconnected
		{
			auto const& connections = e8->getChannelConnections();
			EXPECT_EQ(8u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Remove listener mappings - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e7.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e7.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortInputAudioMappingsRemoved(*e7, la::avdecc::entity::model::StreamPortIndex{ 0u }, RedundantListenerMappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should be fully disconnected
			{
				auto const& connections = e7->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
			// Talker should be fully disconnected
			{
				auto const& connections = e8->getChannelConnections();
				EXPECT_EQ(8u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, LoadWithExistingConnectionRedundantTalkerFirst)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x08.json");
	loadEntityFile("data/ChannelConnection/Entity_0x07.json");

	try
	{
		auto e7 = c.getControlledEntityImplGuard(Entity07_R, true, false);
		auto e8 = c.getControlledEntityImplGuard(Entity08_R, true, false);
		// Listener should only have connection (fully connected), the other connections fully disconnected
		{
			auto const& connections = e7->getChannelConnections();
			EXPECT_EQ(4u, connections.size());
			{
				auto const& channelId = connections.at(ListenerClusterIdentification);
				EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
				EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
				EXPECT_EQ(TalkerPrimaryStreamIdentification8, channelId.channelConnectionIdentification.streamIdentification);
				EXPECT_EQ(RedundantTalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
				ASSERT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_EQ(TalkerSecondaryStreamIdentification8, channelId.secondaryChannelConnectionIdentification->streamIdentification);
				EXPECT_EQ(RedundantTalkerClusterIdentification, channelId.secondaryChannelConnectionIdentification->clusterIdentification);
				EXPECT_TRUE(channelId.isConnected());
				EXPECT_FALSE(channelId.isPartiallyConnected());
			}
			for (auto const& [clusterId, channelId] : connections)
			{
				if (clusterId == ListenerClusterIdentification)
				{
					continue;
				}
				EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
				EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isConnected());
				EXPECT_FALSE(channelId.isPartiallyConnected());
			}
		}
		// Talker should be fully disconnected
		{
			auto const& connections = e8->getChannelConnections();
			EXPECT_EQ(8u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Remove listener mappings - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e7.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e7.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			c.updateStreamPortInputAudioMappingsRemoved(*e7, la::avdecc::entity::model::StreamPortIndex{ 0u }, RedundantListenerMappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should be fully disconnected
			{
				auto const& connections = e7->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
			// Talker should be fully disconnected
			{
				auto const& connections = e8->getChannelConnections();
				EXPECT_EQ(8u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterDelListenerRedundantMappingsInSequencePrimaryFirst) // Simulate Controller sending primary mappings only and getting notification from Entity with secondary mappings later
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x07.json");

	try
	{
		auto e7 = c.getControlledEntityImplGuard(Entity07_R, true, false);

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when removing primary listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e7.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e7.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Remove primary listener mappings
			c.updateStreamPortInputAudioMappingsRemoved(*e7, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e7->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE((channelId.channelConnectionIdentification == la::avdecc::controller::model::ChannelConnectionIdentification{}));
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					ASSERT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 1u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.secondaryChannelConnectionIdentification->streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerSecondaryStreamIdentification8, channelId.secondaryChannelConnectionIdentification->streamIdentification);
					EXPECT_TRUE((channelId.secondaryChannelConnectionIdentification->clusterIdentification == la::avdecc::controller::model::ClusterIdentification{})); // No talker mapping because talker is offline
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when removing secondary listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e7.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e7.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Remove secondary listener mappings
			c.updateStreamPortInputAudioMappingsRemoved(*e7, la::avdecc::entity::model::StreamPortIndex{ 0u }, la::avdecc::entity::model::AudioMappings{ la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 1u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } } }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should be fully disconnected
			{
				auto const& connections = e7->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

TEST_F(ChannelConnection_F, AfterDelListenerRedundantMappingsInSequenceSecondaryFirst) // Simulate Controller sending secondary mappings only and getting notification from Entity with primary mappings later
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x07.json");

	try
	{
		auto e7 = c.getControlledEntityImplGuard(Entity07_R, true, false);

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when removing secondary listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e7.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e7.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Remove secondary listener mappings
			c.updateStreamPortInputAudioMappingsRemoved(*e7, la::avdecc::entity::model::StreamPortIndex{ 0u }, la::avdecc::entity::model::AudioMappings{ la::avdecc::entity::model::AudioMapping{ la::avdecc::entity::model::StreamIndex{ 1u }, std::uint16_t{ 0u }, la::avdecc::entity::model::ClusterIndex{ 0u }, std::uint16_t{ 0u } } }, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should only have connection with listener mappings (but not fully connected), the other connections fully disconnected
			{
				auto const& connections = e7->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerPrimaryStreamIdentification8, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{})); // No talker mapping because talker is offline
					ASSERT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_TRUE((*channelId.secondaryChannelConnectionIdentification == la::avdecc::controller::model::ChannelConnectionIdentification{}));
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
		}

		// Expect onChannelInputConnectionChanged() and onStreamPortInputAudioMappingsChanged() to be called when removing primary listener mappings
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e7.get(), ListenerClusterIdentification, ::testing::_)).Times(1);
			EXPECT_CALL(*this, onStreamPortInputAudioMappingsChanged(::testing::_, e7.get(), la::avdecc::entity::model::StreamPortIndex{ 0u })).Times(1);

			// Remove primary listener mappings
			c.updateStreamPortInputAudioMappingsRemoved(*e7, la::avdecc::entity::model::StreamPortIndex{ 0u }, Mappings_Identity_One, la::avdecc::controller::TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			unregisterMockObserver();

			// Listener should be fully disconnected
			{
				auto const& connections = e7->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				checkAllConnectionsDisconnected(connections);
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}


TEST_F(ChannelConnection_F, RedundantEntityDepartingRemovesChannelConnection)
{
	auto& c = getControllerImpl();

	loadEntityFile("data/ChannelConnection/Entity_0x07.json");
	loadEntityFile("data/ChannelConnection/Entity_0x08.json");

	try
	{
		auto e7 = c.getControlledEntityImplGuard(Entity07_R, true, false);
		auto e8 = c.getControlledEntityImplGuard(Entity08_R, true, false);
		// Listener should only have connection (fully connected), the other connections fully disconnected
		{
			auto const& connections = e7->getChannelConnections();
			EXPECT_EQ(4u, connections.size());
			{
				auto const& channelId = connections.at(ListenerClusterIdentification);
				EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
				EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
				EXPECT_EQ(TalkerPrimaryStreamIdentification8, channelId.channelConnectionIdentification.streamIdentification);
				EXPECT_EQ(RedundantTalkerClusterIdentification, channelId.channelConnectionIdentification.clusterIdentification);
				ASSERT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_EQ(TalkerSecondaryStreamIdentification8, channelId.secondaryChannelConnectionIdentification->streamIdentification);
				EXPECT_EQ(RedundantTalkerClusterIdentification, channelId.secondaryChannelConnectionIdentification->clusterIdentification);
				EXPECT_TRUE(channelId.isConnected());
				EXPECT_FALSE(channelId.isPartiallyConnected());
			}
			for (auto const& [clusterId, channelId] : connections)
			{
				if (clusterId == ListenerClusterIdentification)
				{
					continue;
				}
				EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
				EXPECT_TRUE((channelId.channelConnectionIdentification.streamIdentification == la::avdecc::entity::model::StreamIdentification{}));
				EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{}));
				EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
				EXPECT_FALSE(channelId.isConnected());
				EXPECT_FALSE(channelId.isPartiallyConnected());
			}
		}
		// Talker should be fully disconnected
		{
			auto const& connections = e8->getChannelConnections();
			EXPECT_EQ(8u, connections.size());
			checkAllConnectionsDisconnected(connections);
		}

		// Unload talker entity (simulating device going offline) - should trigger onChannelInputConnectionChanged
		{
			registerMockObserver();
			EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, e7.get(), ListenerClusterIdentification, ::testing::_)).Times(1);

			c.unloadVirtualEntity(Entity08_R);

			unregisterMockObserver();

			// Listener should have connection with listener mapping and stream connection, but NO talker mapping (not fully connected) because talker is offline
			{
				auto const& connections = e7->getChannelConnections();
				EXPECT_EQ(4u, connections.size());
				{
					auto const& channelId = connections.at(ListenerClusterIdentification);
					EXPECT_TRUE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_EQ(la::avdecc::entity::model::StreamIndex{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamIndex);
					EXPECT_EQ(std::uint16_t{ 0u }, channelId.channelConnectionIdentification.streamChannelIdentification.streamChannel);
					EXPECT_EQ(TalkerPrimaryStreamIdentification8, channelId.channelConnectionIdentification.streamIdentification);
					EXPECT_TRUE((channelId.channelConnectionIdentification.clusterIdentification == la::avdecc::controller::model::ClusterIdentification{})); // No talker mapping because talker is offline
					ASSERT_TRUE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_EQ(TalkerSecondaryStreamIdentification8, channelId.secondaryChannelConnectionIdentification->streamIdentification);
					EXPECT_TRUE((channelId.secondaryChannelConnectionIdentification->clusterIdentification == la::avdecc::controller::model::ClusterIdentification{})); // No talker mapping because talker is offline
					EXPECT_FALSE(channelId.isConnected()); // Not fully connected
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
				for (auto const& [clusterId, channelId] : connections)
				{
					if (clusterId == ListenerClusterIdentification)
					{
						continue;
					}
					EXPECT_FALSE(channelId.channelConnectionIdentification.streamChannelIdentification.isValid());
					EXPECT_TRUE((channelId.channelConnectionIdentification == la::avdecc::controller::model::ChannelConnectionIdentification{}));
					EXPECT_FALSE(channelId.secondaryChannelConnectionIdentification.has_value());
					EXPECT_FALSE(channelId.isConnected());
					EXPECT_FALSE(channelId.isPartiallyConnected());
				}
			}
		}
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}

#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

#else // !ENABLE_AVDECC_FEATURE_CBR
TEST_F(ChannelConnection_F, Disabled)
{
	auto& c = getControllerImpl();
	// Expect Controller::Observer::onChannelInputConnectionChanged() NOT to be called
	registerMockObserver();
	EXPECT_CALL(*this, onChannelInputConnectionChanged(::testing::_, c.getControlledEntityGuard(Entity01).get(), ::testing::_, ::testing::_)).Times(0);

	loadEntityFile("data/ChannelConnection/Entity_0x01.json");

	try
	{
		auto const& e = *c.getControlledEntityGuard(Entity01);
		EXPECT_THROW(e.getChannelConnections(), la::avdecc::controller::ControlledEntity::Exception);
	}
	catch (...)
	{
		ASSERT_FALSE(true) << "Should not throw";
	}
}
#endif // ENABLE_AVDECC_FEATURE_CBR

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

TEST(Controller, HashEntityModelV1)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan };
	auto const& [error, msg, controlledEntity] = la::avdecc::controller::Controller::deserializeControlledEntityFromJson("data/SimpleEntity.json", flags);
	ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	auto const checksum = la::avdecc::controller::Controller::computeEntityModelChecksum(*controlledEntity, std::uint32_t{ 1u });
	EXPECT_TRUE(checksum.has_value());
	EXPECT_EQ(64u, checksum.value().size());
	EXPECT_STREQ("8A02AF8AF382B7D443F351786E1CC54B54B70AC60F29B92BA2B1F3074B4980BF", checksum.value().c_str());
}

TEST(Controller, HashEntityModelV2)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan };
	auto const& [error, msg, controlledEntity] = la::avdecc::controller::Controller::deserializeControlledEntityFromJson("data/SimpleEntity.json", flags);
	ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	auto const checksum = la::avdecc::controller::Controller::computeEntityModelChecksum(*controlledEntity, std::uint32_t{ 2u });
	EXPECT_TRUE(checksum.has_value());
	EXPECT_EQ(64u, checksum.value().size());
	EXPECT_STREQ("FE85643511A1F0E41C4AAAAC907DEFEDFA2B911F3BF62284D0952C3E43E7F69F", checksum.value().c_str());
}

TEST(Controller, HashEntityModelV3)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan };
	auto const& [error, msg, controlledEntity] = la::avdecc::controller::Controller::deserializeControlledEntityFromJson("data/SimpleEntity.json", flags);
	ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	auto const checksum = la::avdecc::controller::Controller::computeEntityModelChecksum(*controlledEntity, std::uint32_t{ 3u });
	EXPECT_TRUE(checksum.has_value());
	EXPECT_EQ(64u, checksum.value().size());
	EXPECT_STREQ("33C17AFF5D59BEC76AA3A6B0A6FE6C91F8E09E46DA111B5975858E326D02C4C4", checksum.value().c_str());
}

TEST(Controller, HashEntityModelV4)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan };
	auto const& [error, msg, controlledEntity] = la::avdecc::controller::Controller::deserializeControlledEntityFromJson("data/SimpleEntity.json", flags);
	ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	auto const checksum = la::avdecc::controller::Controller::computeEntityModelChecksum(*controlledEntity, std::uint32_t{ 4u });
	EXPECT_TRUE(checksum.has_value());
	EXPECT_EQ(64u, checksum.value().size());
	EXPECT_STREQ("98343B6A0540080461F83F6EE99FA973C552E98C4FA9AFE4F047F733C858B7F5", checksum.value().c_str());
}

TEST(Controller, HashEntityModelV5)
{
	auto const flags = la::avdecc::entity::model::jsonSerializer::Flags{ la::avdecc::entity::model::jsonSerializer::Flag::ProcessADP, la::avdecc::entity::model::jsonSerializer::Flag::ProcessDynamicModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessStaticModel, la::avdecc::entity::model::jsonSerializer::Flag::ProcessMilan };
	auto const& [error, msg, controlledEntity] = la::avdecc::controller::Controller::deserializeControlledEntityFromJson("data/SimpleEntity.json", flags);
	ASSERT_EQ(la::avdecc::jsonSerializer::DeserializationError::NoError, error);
	auto const checksum = la::avdecc::controller::Controller::computeEntityModelChecksum(*controlledEntity, std::uint32_t{ 5u });
	EXPECT_TRUE(checksum.has_value());
	EXPECT_EQ(64u, checksum.value().size());
	EXPECT_STREQ("068D4565E93A67323C3D83A23ABC407FBCF7ED2FE7CFF6D29766938A3264F30D", checksum.value().c_str());
}

TEST(Controller, GetMappingForInputClusterIdentification_ValidMappingInStaticAudioMaps)
{
	// Setup StreamPortNode with static audio mappings
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ 0u };

	// Configure static model with base cluster 10, 4 clusters
	streamPortNode.staticModel.baseCluster = 10u;
	streamPortNode.staticModel.numberOfClusters = 4u;

	// Create AudioMapNode with a mapping
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	auto& mappings = audioMapNode.staticModel.mappings;

	// Add a mapping: streamIndex=0, streamChannel=1, clusterOffset=2, clusterChannel=3
	// This corresponds to global clusterIndex=12 (baseCluster 10 + clusterOffset 2)
	mappings.push_back({ 0u, 1u, 2u, 3u });

	streamPortNode.audioMaps.emplace(0u, std::move(audioMapNode));

	// Mock redundancy functions - stream 0 is not redundant
	auto isRedundantPrimary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};
	auto isRedundantSecondary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};

	// Test: Look for cluster 12 (baseCluster 10 + clusterOffset 2), channel 3
	auto clusterIdent = la::avdecc::controller::model::ClusterIdentification{ 12u, 3u };
	auto const mappingTuple = la::avdecc::controller::ControllerImpl::getMappingForInputClusterIdentification(streamPortNode, clusterIdent, isRedundantPrimary, isRedundantSecondary);

	// Verify mapping was found as primary (non-redundant)
	EXPECT_FALSE(std::get<0>(mappingTuple)); // Not redundant
	ASSERT_TRUE(std::get<1>(mappingTuple).has_value());
	EXPECT_FALSE(std::get<2>(mappingTuple).has_value());
	EXPECT_EQ(0u, std::get<1>(mappingTuple)->streamIndex);
	EXPECT_EQ(1u, std::get<1>(mappingTuple)->streamChannel);
	EXPECT_EQ(2u, std::get<1>(mappingTuple)->clusterOffset);
	EXPECT_EQ(3u, std::get<1>(mappingTuple)->clusterChannel);
}

TEST(Controller, GetMappingForInputClusterIdentification_ValidMappingInDynamicAudioMap)
{
	// Setup StreamPortNode with dynamic audio mappings
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ 0u };

	// Configure static model
	streamPortNode.staticModel.baseCluster = 5u;
	streamPortNode.staticModel.numberOfClusters = 10u;

	// Add dynamic mapping: streamIndex=2, streamChannel=4, clusterOffset=3, clusterChannel=1
	// This corresponds to global clusterIndex=8 (baseCluster 5 + clusterOffset 3)
	auto& dynamicMappings = streamPortNode.dynamicModel.dynamicAudioMap;
	dynamicMappings.push_back({ 2u, 4u, 3u, 1u });

	// Mock redundancy functions - stream 2 is not redundant
	auto isRedundantPrimary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};
	auto isRedundantSecondary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};

	// Test: Look for cluster 8 (baseCluster 5 + clusterOffset 3), channel 1
	auto clusterIdent = la::avdecc::controller::model::ClusterIdentification{ 8u, 1u };
	auto const mappingTuple = la::avdecc::controller::ControllerImpl::getMappingForInputClusterIdentification(streamPortNode, clusterIdent, isRedundantPrimary, isRedundantSecondary);

	// Verify non-redundant mapping was found in dynamic mappings (returned as primary)
	EXPECT_FALSE(std::get<0>(mappingTuple)); // Not redundant
	ASSERT_TRUE(std::get<1>(mappingTuple).has_value());
	EXPECT_FALSE(std::get<2>(mappingTuple).has_value());
	EXPECT_EQ(2u, std::get<1>(mappingTuple)->streamIndex);
	EXPECT_EQ(4u, std::get<1>(mappingTuple)->streamChannel);
	EXPECT_EQ(3u, std::get<1>(mappingTuple)->clusterOffset);
	EXPECT_EQ(1u, std::get<1>(mappingTuple)->clusterChannel);
}

TEST(Controller, GetMappingForInputClusterIdentification_OutOfRangeLow)
{
	// Setup StreamPortNode
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ 0u };

	// Configure static model: baseCluster 10, 5 clusters (valid range: 10-14)
	streamPortNode.staticModel.baseCluster = 10u;
	streamPortNode.staticModel.numberOfClusters = 5u;

	// Add a mapping
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	audioMapNode.staticModel.mappings.push_back({ 0u, 0u, 0u, 0u });
	streamPortNode.audioMaps.emplace(0u, std::move(audioMapNode));

	// Mock redundancy functions
	auto isRedundantPrimary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};
	auto isRedundantSecondary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};

	// Test: ClusterIndex 9 is below baseCluster (10)
	auto clusterIdent = la::avdecc::controller::model::ClusterIdentification{ 9u, 0u };
	auto const mappingTuple = la::avdecc::controller::ControllerImpl::getMappingForInputClusterIdentification(streamPortNode, clusterIdent, isRedundantPrimary, isRedundantSecondary);

	// Should return nullopt for out of range
	EXPECT_FALSE(std::get<0>(mappingTuple)); // Not redundant (no mapping found)
	EXPECT_FALSE(std::get<1>(mappingTuple).has_value());
	EXPECT_FALSE(std::get<2>(mappingTuple).has_value());
}

TEST(Controller, GetMappingForInputClusterIdentification_OutOfRangeHigh)
{
	// Setup StreamPortNode
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ 0u };

	// Configure static model: baseCluster 10, 5 clusters (valid range: 10-14)
	streamPortNode.staticModel.baseCluster = 10u;
	streamPortNode.staticModel.numberOfClusters = 5u;

	// Add a mapping
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	audioMapNode.staticModel.mappings.push_back({ 0u, 0u, 0u, 0u });
	streamPortNode.audioMaps.emplace(0u, std::move(audioMapNode));

	// Mock redundancy functions
	auto isRedundantPrimary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};
	auto isRedundantSecondary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};

	// Test: ClusterIndex 15 is >= baseCluster + numberOfClusters (10 + 5 = 15)
	auto clusterIdent = la::avdecc::controller::model::ClusterIdentification{ 15u, 0u };
	auto const mappingTuple = la::avdecc::controller::ControllerImpl::getMappingForInputClusterIdentification(streamPortNode, clusterIdent, isRedundantPrimary, isRedundantSecondary);

	// Should return nullopt for out of range
	EXPECT_FALSE(std::get<0>(mappingTuple)); // Not redundant (no mapping found)
	EXPECT_FALSE(std::get<1>(mappingTuple).has_value());
	EXPECT_FALSE(std::get<2>(mappingTuple).has_value());
}

TEST(Controller, GetMappingForInputClusterIdentification_NoMatchingMapping)
{
	// Setup StreamPortNode
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ 0u };

	// Configure static model
	streamPortNode.staticModel.baseCluster = 0u;
	streamPortNode.staticModel.numberOfClusters = 10u;

	// Add mappings that don't match what we're looking for
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	audioMapNode.staticModel.mappings.push_back({ 0u, 0u, 2u, 1u }); // clusterOffset=2, channel=1
	audioMapNode.staticModel.mappings.push_back({ 0u, 1u, 5u, 3u }); // clusterOffset=5, channel=3
	streamPortNode.audioMaps.emplace(0u, std::move(audioMapNode));

	// Mock redundancy functions
	auto isRedundantPrimary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};
	auto isRedundantSecondary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};

	// Test: Look for clusterIndex=2, channel=2 (no mapping exists for this channel)
	auto clusterIdent = la::avdecc::controller::model::ClusterIdentification{ 2u, 2u };
	auto const mappingTuple = la::avdecc::controller::ControllerImpl::getMappingForInputClusterIdentification(streamPortNode, clusterIdent, isRedundantPrimary, isRedundantSecondary);

	// Should return nullopt when no matching mapping exists
	EXPECT_FALSE(std::get<0>(mappingTuple)); // Not redundant (no mapping found)
	EXPECT_FALSE(std::get<1>(mappingTuple).has_value());
	EXPECT_FALSE(std::get<2>(mappingTuple).has_value());
}

TEST(Controller, GetMappingForInputClusterIdentification_ZeroBaseCluster)
{
	// Setup StreamPortNode with baseCluster=0
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ 0u };

	// Configure static model with baseCluster=0, 8 clusters
	streamPortNode.staticModel.baseCluster = 0u;
	streamPortNode.staticModel.numberOfClusters = 8u;

	// Add a mapping at clusterOffset=0 (global cluster 0)
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	audioMapNode.staticModel.mappings.push_back({ 1u, 2u, 0u, 5u });
	streamPortNode.audioMaps.emplace(0u, std::move(audioMapNode));

	// Mock redundancy functions
	auto isRedundantPrimary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};
	auto isRedundantSecondary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};

	// Test: Look for global cluster 0, channel 5
	auto clusterIdent = la::avdecc::controller::model::ClusterIdentification{ 0u, 5u };
	auto const mappingTuple = la::avdecc::controller::ControllerImpl::getMappingForInputClusterIdentification(streamPortNode, clusterIdent, isRedundantPrimary, isRedundantSecondary);

	// Verify mapping was found as primary (non-redundant)
	EXPECT_FALSE(std::get<0>(mappingTuple)); // Not redundant
	ASSERT_TRUE(std::get<1>(mappingTuple).has_value());
	EXPECT_FALSE(std::get<2>(mappingTuple).has_value());
	EXPECT_EQ(1u, std::get<1>(mappingTuple)->streamIndex);
	EXPECT_EQ(2u, std::get<1>(mappingTuple)->streamChannel);
	EXPECT_EQ(0u, std::get<1>(mappingTuple)->clusterOffset);
	EXPECT_EQ(5u, std::get<1>(mappingTuple)->clusterChannel);
}

TEST(Controller, GetMappingForInputClusterIdentification_PriorityStaticOverDynamic)
{
	// Setup StreamPortNode with both static and dynamic mappings for same cluster
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ 0u };

	// Configure static model
	streamPortNode.staticModel.baseCluster = 0u;
	streamPortNode.staticModel.numberOfClusters = 10u;

	// Add static mapping for clusterOffset=3, channel=2
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	audioMapNode.staticModel.mappings.push_back({ 10u, 20u, 3u, 2u });
	streamPortNode.audioMaps.emplace(0u, std::move(audioMapNode));

	// Add dynamic mapping for same clusterOffset=3, channel=2
	streamPortNode.dynamicModel.dynamicAudioMap.push_back({ 30u, 40u, 3u, 2u });

	// Mock redundancy functions
	auto isRedundantPrimary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};
	auto isRedundantSecondary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};

	// Test: Look for cluster 3, channel 2
	auto clusterIdent = la::avdecc::controller::model::ClusterIdentification{ 3u, 2u };
	auto const mappingTuple = la::avdecc::controller::ControllerImpl::getMappingForInputClusterIdentification(streamPortNode, clusterIdent, isRedundantPrimary, isRedundantSecondary);

	// Should return the static mapping (priority over dynamic) as non-redundant (primary)
	EXPECT_FALSE(std::get<0>(mappingTuple)); // Not redundant
	ASSERT_TRUE(std::get<1>(mappingTuple).has_value());
	EXPECT_FALSE(std::get<2>(mappingTuple).has_value());
	EXPECT_EQ(10u, std::get<1>(mappingTuple)->streamIndex);
	EXPECT_EQ(20u, std::get<1>(mappingTuple)->streamChannel);
	EXPECT_EQ(3u, std::get<1>(mappingTuple)->clusterOffset);
	EXPECT_EQ(2u, std::get<1>(mappingTuple)->clusterChannel);
}

// New redundant mapping tests
TEST(Controller, GetMappingForInputClusterIdentification_RedundantPrimaryAndSecondary)
{
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ 0u };
	streamPortNode.staticModel.baseCluster = 0u;
	streamPortNode.staticModel.numberOfClusters = 10u;

	// Add two mappings with same cluster but different streams - one primary, one secondary
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	audioMapNode.staticModel.mappings.push_back({ 1u, 0u, 2u, 3u }); // streamIndex=1 (will be primary)
	audioMapNode.staticModel.mappings.push_back({ 5u, 1u, 2u, 3u }); // streamIndex=5 (will be secondary)
	streamPortNode.audioMaps.emplace(0u, std::move(audioMapNode));

	// Mock redundancy functions: stream 1 is primary, stream 5 is secondary
	auto isRedundantPrimary = [](la::avdecc::entity::model::StreamIndex streamIndex)
	{
		return streamIndex == 1u;
	};
	auto isRedundantSecondary = [](la::avdecc::entity::model::StreamIndex streamIndex)
	{
		return streamIndex == 5u;
	};

	auto clusterIdent = la::avdecc::controller::model::ClusterIdentification{ 2u, 3u };
	auto const mappingTuple = la::avdecc::controller::ControllerImpl::getMappingForInputClusterIdentification(streamPortNode, clusterIdent, isRedundantPrimary, isRedundantSecondary);

	// Should return both primary and secondary mappings
	EXPECT_TRUE(std::get<0>(mappingTuple)); // Is redundant
	ASSERT_TRUE(std::get<1>(mappingTuple).has_value());
	ASSERT_TRUE(std::get<2>(mappingTuple).has_value());

	// Primary mapping (stream 1)
	EXPECT_EQ(1u, std::get<1>(mappingTuple)->streamIndex);
	EXPECT_EQ(0u, std::get<1>(mappingTuple)->streamChannel);
	EXPECT_EQ(2u, std::get<1>(mappingTuple)->clusterOffset);
	EXPECT_EQ(3u, std::get<1>(mappingTuple)->clusterChannel);

	// Secondary mapping (stream 5)
	EXPECT_EQ(5u, std::get<2>(mappingTuple)->streamIndex);
	EXPECT_EQ(1u, std::get<2>(mappingTuple)->streamChannel);
	EXPECT_EQ(2u, std::get<2>(mappingTuple)->clusterOffset);
	EXPECT_EQ(3u, std::get<2>(mappingTuple)->clusterChannel);
}

TEST(Controller, GetMappingForInputClusterIdentification_RedundantPrimaryOnly)
{
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ 0u };
	streamPortNode.staticModel.baseCluster = 0u;
	streamPortNode.staticModel.numberOfClusters = 10u;

	// Add only primary mapping
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	audioMapNode.staticModel.mappings.push_back({ 2u, 4u, 1u, 5u }); // streamIndex=2 (primary)
	streamPortNode.audioMaps.emplace(0u, std::move(audioMapNode));

	// Mock redundancy functions: stream 2 is primary, no secondary
	auto isRedundantPrimary = [](la::avdecc::entity::model::StreamIndex streamIndex)
	{
		return streamIndex == 2u;
	};
	auto isRedundantSecondary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};

	auto clusterIdent = la::avdecc::controller::model::ClusterIdentification{ 1u, 5u };
	auto const mappingTuple = la::avdecc::controller::ControllerImpl::getMappingForInputClusterIdentification(streamPortNode, clusterIdent, isRedundantPrimary, isRedundantSecondary);

	// Should return primary mapping only, secondary should be nullopt
	EXPECT_TRUE(std::get<0>(mappingTuple)); // Is redundant (but only primary found)
	ASSERT_TRUE(std::get<1>(mappingTuple).has_value());
	EXPECT_FALSE(std::get<2>(mappingTuple).has_value());

	EXPECT_EQ(2u, std::get<1>(mappingTuple)->streamIndex);
	EXPECT_EQ(4u, std::get<1>(mappingTuple)->streamChannel);
	EXPECT_EQ(1u, std::get<1>(mappingTuple)->clusterOffset);
	EXPECT_EQ(5u, std::get<1>(mappingTuple)->clusterChannel);
}

TEST(Controller, GetMappingForInputClusterIdentification_RedundantSecondaryOnly)
{
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ 0u };
	streamPortNode.staticModel.baseCluster = 0u;
	streamPortNode.staticModel.numberOfClusters = 10u;

	// Add only secondary mapping
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	audioMapNode.staticModel.mappings.push_back({ 7u, 8u, 3u, 9u }); // streamIndex=7 (secondary)
	streamPortNode.audioMaps.emplace(0u, std::move(audioMapNode));

	// Mock redundancy functions: stream 7 is secondary, no primary
	auto isRedundantPrimary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};
	auto isRedundantSecondary = [](la::avdecc::entity::model::StreamIndex streamIndex)
	{
		return streamIndex == 7u;
	};

	auto clusterIdent = la::avdecc::controller::model::ClusterIdentification{ 3u, 9u };
	auto const mappingTuple = la::avdecc::controller::ControllerImpl::getMappingForInputClusterIdentification(streamPortNode, clusterIdent, isRedundantPrimary, isRedundantSecondary);

	// Should return secondary mapping only, primary should be nullopt
	EXPECT_TRUE(std::get<0>(mappingTuple)); // Is redundant (but only secondary found)
	EXPECT_FALSE(std::get<1>(mappingTuple).has_value());
	ASSERT_TRUE(std::get<2>(mappingTuple).has_value());

	EXPECT_EQ(7u, std::get<2>(mappingTuple)->streamIndex);
	EXPECT_EQ(8u, std::get<2>(mappingTuple)->streamChannel);
	EXPECT_EQ(3u, std::get<2>(mappingTuple)->clusterOffset);
	EXPECT_EQ(9u, std::get<2>(mappingTuple)->clusterChannel);
}

TEST(Controller, GetMappingForInputClusterIdentification_NonRedundantStream)
{
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ 0u };
	streamPortNode.staticModel.baseCluster = 0u;
	streamPortNode.staticModel.numberOfClusters = 10u;

	// Add non-redundant mapping
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	audioMapNode.staticModel.mappings.push_back({ 10u, 11u, 4u, 6u }); // streamIndex=10 (non-redundant)
	streamPortNode.audioMaps.emplace(0u, std::move(audioMapNode));

	// Mock redundancy functions: stream 10 is neither primary nor secondary
	auto isRedundantPrimary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};
	auto isRedundantSecondary = [](la::avdecc::entity::model::StreamIndex)
	{
		return false;
	};

	auto clusterIdent = la::avdecc::controller::model::ClusterIdentification{ 4u, 6u };
	auto const mappingTuple = la::avdecc::controller::ControllerImpl::getMappingForInputClusterIdentification(streamPortNode, clusterIdent, isRedundantPrimary, isRedundantSecondary);

	// Should return non-redundant mapping in first element, std::nullopt in second element
	EXPECT_FALSE(std::get<0>(mappingTuple)); // Not redundant
	ASSERT_TRUE(std::get<1>(mappingTuple).has_value());
	EXPECT_FALSE(std::get<2>(mappingTuple).has_value());

	EXPECT_EQ(10u, std::get<1>(mappingTuple)->streamIndex);
	EXPECT_EQ(11u, std::get<1>(mappingTuple)->streamChannel);
	EXPECT_EQ(4u, std::get<1>(mappingTuple)->clusterOffset);
	EXPECT_EQ(6u, std::get<1>(mappingTuple)->clusterChannel);
}

// Tests for getMappingForStreamChannelIdentification
TEST(Controller, GetMappingForStreamChannelIdentification_ValidMappingInStaticAudioMaps)
{
	// Create a StreamPortNode with static mappings
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	streamPortNode.staticModel.baseCluster = 2u;
	streamPortNode.staticModel.numberOfClusters = 4u;

	// Add a static audio map with a mapping: stream 5, channel 10 -> cluster offset 2, channel 1
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	auto staticMapping = la::avdecc::entity::model::AudioMapping{};
	staticMapping.streamIndex = 5u;
	staticMapping.streamChannel = 10u;
	staticMapping.clusterOffset = 2u;
	staticMapping.clusterChannel = 1u;
	audioMapNode.staticModel.mappings.push_back(staticMapping);
	streamPortNode.audioMaps.emplace(la::avdecc::entity::model::DescriptorIndex{ 0u }, std::move(audioMapNode));

	// Test: Look for stream 5, channel 10
	auto const mapping = la::avdecc::controller::ControllerImpl::getMappingForStreamChannelIdentification(streamPortNode, la::avdecc::entity::model::StreamIndex{ 5u }, 10u);

	ASSERT_TRUE(mapping.has_value());
	EXPECT_EQ(5u, mapping->streamIndex);
	EXPECT_EQ(10u, mapping->streamChannel);
	EXPECT_EQ(2u, mapping->clusterOffset);
	EXPECT_EQ(1u, mapping->clusterChannel);
}

TEST(Controller, GetMappingForStreamChannelIdentification_ValidMappingInDynamicAudioMap)
{
	// Create a StreamPortNode with dynamic mappings only
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	streamPortNode.staticModel.baseCluster = 1u;
	streamPortNode.staticModel.numberOfClusters = 3u;

	// Add a dynamic audio mapping: stream 7, channel 15 -> cluster offset 1, channel 0
	auto dynamicMapping = la::avdecc::entity::model::AudioMapping{};
	dynamicMapping.streamIndex = 7u;
	dynamicMapping.streamChannel = 15u;
	dynamicMapping.clusterOffset = 1u;
	dynamicMapping.clusterChannel = 0u;
	streamPortNode.dynamicModel.dynamicAudioMap.push_back(dynamicMapping);

	// Test: Look for stream 7, channel 15
	auto const mapping = la::avdecc::controller::ControllerImpl::getMappingForStreamChannelIdentification(streamPortNode, la::avdecc::entity::model::StreamIndex{ 7u }, 15u);

	ASSERT_TRUE(mapping.has_value());
	EXPECT_EQ(7u, mapping->streamIndex);
	EXPECT_EQ(15u, mapping->streamChannel);
	EXPECT_EQ(1u, mapping->clusterOffset);
	EXPECT_EQ(0u, mapping->clusterChannel);
}

TEST(Controller, GetMappingForStreamChannelIdentification_NoMatchingMapping_WrongStreamIndex)
{
	// Create a StreamPortNode with mappings
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	streamPortNode.staticModel.baseCluster = 0u;
	streamPortNode.staticModel.numberOfClusters = 2u;

	// Add a static mapping: stream 3, channel 5 -> cluster offset 0, channel 1
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	auto staticMapping = la::avdecc::entity::model::AudioMapping{};
	staticMapping.streamIndex = 3u;
	staticMapping.streamChannel = 5u;
	staticMapping.clusterOffset = 0u;
	staticMapping.clusterChannel = 1u;
	audioMapNode.staticModel.mappings.push_back(staticMapping);
	streamPortNode.audioMaps.emplace(la::avdecc::entity::model::DescriptorIndex{ 0u }, std::move(audioMapNode));

	// Test: Look for a different stream index (4 instead of 3) with same channel
	auto const mapping = la::avdecc::controller::ControllerImpl::getMappingForStreamChannelIdentification(streamPortNode, la::avdecc::entity::model::StreamIndex{ 4u }, 5u);

	EXPECT_FALSE(mapping.has_value());
}

TEST(Controller, GetMappingForStreamChannelIdentification_NoMatchingMapping_WrongStreamChannel)
{
	// Create a StreamPortNode with mappings
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	streamPortNode.staticModel.baseCluster = 0u;
	streamPortNode.staticModel.numberOfClusters = 2u;

	// Add a static mapping: stream 3, channel 5 -> cluster offset 0, channel 1
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	auto staticMapping = la::avdecc::entity::model::AudioMapping{};
	staticMapping.streamIndex = 3u;
	staticMapping.streamChannel = 5u;
	staticMapping.clusterOffset = 0u;
	staticMapping.clusterChannel = 1u;
	audioMapNode.staticModel.mappings.push_back(staticMapping);
	streamPortNode.audioMaps.emplace(la::avdecc::entity::model::DescriptorIndex{ 0u }, std::move(audioMapNode));

	// Test: Look for same stream index but different channel (6 instead of 5)
	auto const mapping = la::avdecc::controller::ControllerImpl::getMappingForStreamChannelIdentification(streamPortNode, la::avdecc::entity::model::StreamIndex{ 3u }, 6u);

	EXPECT_FALSE(mapping.has_value());
}

TEST(Controller, GetMappingForStreamChannelIdentification_EmptyMappings)
{
	// Create a StreamPortNode with no mappings
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	streamPortNode.staticModel.baseCluster = 0u;
	streamPortNode.staticModel.numberOfClusters = 2u;

	// Test: Look for any stream/channel should return nothing
	auto const mapping = la::avdecc::controller::ControllerImpl::getMappingForStreamChannelIdentification(streamPortNode, la::avdecc::entity::model::StreamIndex{ 0u }, 0u);

	EXPECT_FALSE(mapping.has_value());
}

TEST(Controller, GetMappingForStreamChannelIdentification_PriorityStaticOverDynamic)
{
	// Create a StreamPortNode with both static and dynamic mappings for the same stream/channel
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	streamPortNode.staticModel.baseCluster = 2u;
	streamPortNode.staticModel.numberOfClusters = 4u;

	// Add a static mapping: stream 10, channel 20 -> cluster offset 3, channel 2
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	auto staticMapping = la::avdecc::entity::model::AudioMapping{};
	staticMapping.streamIndex = 10u;
	staticMapping.streamChannel = 20u;
	staticMapping.clusterOffset = 3u;
	staticMapping.clusterChannel = 2u;
	audioMapNode.staticModel.mappings.push_back(staticMapping);
	streamPortNode.audioMaps.emplace(la::avdecc::entity::model::DescriptorIndex{ 0u }, std::move(audioMapNode));

	// Add a dynamic mapping for the SAME stream 10, channel 20 but DIFFERENT cluster
	auto dynamicMapping = la::avdecc::entity::model::AudioMapping{};
	dynamicMapping.streamIndex = 10u;
	dynamicMapping.streamChannel = 20u;
	dynamicMapping.clusterOffset = 1u;
	dynamicMapping.clusterChannel = 0u;
	streamPortNode.dynamicModel.dynamicAudioMap.push_back(dynamicMapping);

	// Test: Look for stream 10, channel 20
	auto const mapping = la::avdecc::controller::ControllerImpl::getMappingForStreamChannelIdentification(streamPortNode, la::avdecc::entity::model::StreamIndex{ 10u }, 20u);

	// Should return the static mapping (priority over dynamic)
	ASSERT_TRUE(mapping.has_value());
	EXPECT_EQ(10u, mapping->streamIndex);
	EXPECT_EQ(20u, mapping->streamChannel);
	EXPECT_EQ(3u, mapping->clusterOffset);
	EXPECT_EQ(2u, mapping->clusterChannel);
}

TEST(Controller, GetMappingForStreamChannelIdentification_MultipleStaticMappings)
{
	// Create a StreamPortNode with multiple static mappings
	auto streamPortNode = la::avdecc::controller::model::StreamPortInputNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };
	streamPortNode.staticModel.baseCluster = 0u;
	streamPortNode.staticModel.numberOfClusters = 5u;

	// Add multiple static mappings
	auto audioMapNode = la::avdecc::controller::model::AudioMapNode{ la::avdecc::entity::model::DescriptorIndex{ 0u } };

	// Mapping 1: stream 1, channel 0 -> cluster offset 0, channel 0
	auto mapping1 = la::avdecc::entity::model::AudioMapping{};
	mapping1.streamIndex = 1u;
	mapping1.streamChannel = 0u;
	mapping1.clusterOffset = 0u;
	mapping1.clusterChannel = 0u;
	audioMapNode.staticModel.mappings.push_back(mapping1);

	// Mapping 2: stream 1, channel 1 -> cluster offset 0, channel 1
	auto mapping2 = la::avdecc::entity::model::AudioMapping{};
	mapping2.streamIndex = 1u;
	mapping2.streamChannel = 1u;
	mapping2.clusterOffset = 0u;
	mapping2.clusterChannel = 1u;
	audioMapNode.staticModel.mappings.push_back(mapping2);

	// Mapping 3: stream 2, channel 0 -> cluster offset 1, channel 0
	auto mapping3 = la::avdecc::entity::model::AudioMapping{};
	mapping3.streamIndex = 2u;
	mapping3.streamChannel = 0u;
	mapping3.clusterOffset = 1u;
	mapping3.clusterChannel = 0u;
	audioMapNode.staticModel.mappings.push_back(mapping3);

	streamPortNode.audioMaps.emplace(la::avdecc::entity::model::DescriptorIndex{ 0u }, std::move(audioMapNode));

	// Test 1: Look for stream 1, channel 0
	auto const mapping1Result = la::avdecc::controller::ControllerImpl::getMappingForStreamChannelIdentification(streamPortNode, la::avdecc::entity::model::StreamIndex{ 1u }, 0u);
	ASSERT_TRUE(mapping1Result.has_value());
	EXPECT_EQ(1u, mapping1Result->streamIndex);
	EXPECT_EQ(0u, mapping1Result->streamChannel);
	EXPECT_EQ(0u, mapping1Result->clusterOffset);
	EXPECT_EQ(0u, mapping1Result->clusterChannel);

	// Test 2: Look for stream 1, channel 1
	auto const mapping2Result = la::avdecc::controller::ControllerImpl::getMappingForStreamChannelIdentification(streamPortNode, la::avdecc::entity::model::StreamIndex{ 1u }, 1u);
	ASSERT_TRUE(mapping2Result.has_value());
	EXPECT_EQ(1u, mapping2Result->streamIndex);
	EXPECT_EQ(1u, mapping2Result->streamChannel);
	EXPECT_EQ(0u, mapping2Result->clusterOffset);
	EXPECT_EQ(1u, mapping2Result->clusterChannel);

	// Test 3: Look for stream 2, channel 0
	auto const mapping3Result = la::avdecc::controller::ControllerImpl::getMappingForStreamChannelIdentification(streamPortNode, la::avdecc::entity::model::StreamIndex{ 2u }, 0u);
	ASSERT_TRUE(mapping3Result.has_value());
	EXPECT_EQ(2u, mapping3Result->streamIndex);
	EXPECT_EQ(0u, mapping3Result->streamChannel);
	EXPECT_EQ(1u, mapping3Result->clusterOffset);
	EXPECT_EQ(0u, mapping3Result->clusterChannel);
}

// Tests for ChannelConnectionIdentification struct
TEST(ChannelConnectionIdentification, DefaultConstructor_IsInvalid)
{
	auto channelConnection = la::avdecc::controller::model::ChannelConnectionIdentification{};

	EXPECT_FALSE(channelConnection.isValid());
	EXPECT_FALSE(static_cast<bool>(channelConnection));
	EXPECT_FALSE(channelConnection.isConnected());
}

TEST(ChannelConnectionIdentification, OnlyListenerMapping_IsInvalid)
{
	auto channelConnection = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection.streamChannelIdentification.streamChannel = 2u;
	// streamIdentification and clusterIdentification remain invalid

	EXPECT_FALSE(channelConnection.isValid());
	EXPECT_FALSE(static_cast<bool>(channelConnection));
	EXPECT_FALSE(channelConnection.isConnected());
}

TEST(ChannelConnectionIdentification, OnlyTalkerConnection_IsInvalid)
{
	auto channelConnection = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 0u };
	// streamChannelIdentification and clusterIdentification remain invalid

	EXPECT_FALSE(channelConnection.isValid());
	EXPECT_FALSE(static_cast<bool>(channelConnection));
	EXPECT_FALSE(channelConnection.isConnected());
}

TEST(ChannelConnectionIdentification, OnlyTalkerMapping_IsInvalid)
{
	auto channelConnection = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 0u };
	channelConnection.clusterIdentification.clusterChannel = 1u;
	// streamChannelIdentification and streamIdentification remain invalid

	EXPECT_FALSE(channelConnection.isValid());
	EXPECT_FALSE(static_cast<bool>(channelConnection));
	EXPECT_FALSE(channelConnection.isConnected());
}

TEST(ChannelConnectionIdentification, ListenerAndTalkerConnection_NoTalkerMapping_IsInvalid)
{
	auto channelConnection = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection.streamChannelIdentification.streamChannel = 2u;
	channelConnection.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 0u };
	// clusterIdentification remains invalid

	EXPECT_FALSE(channelConnection.isValid());
	EXPECT_FALSE(static_cast<bool>(channelConnection));
	EXPECT_FALSE(channelConnection.isConnected());
}

TEST(ChannelConnectionIdentification, FullyConnected_IsValid)
{
	auto channelConnection = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection.streamChannelIdentification.streamChannel = 2u;
	channelConnection.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 3u };
	channelConnection.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 4u };
	channelConnection.clusterIdentification.clusterChannel = 5u;

	EXPECT_TRUE(channelConnection.isValid());
	EXPECT_TRUE(static_cast<bool>(channelConnection));
	EXPECT_TRUE(channelConnection.isConnected());
}

TEST(ChannelConnectionIdentification, EqualityOperator_SameValues)
{
	auto channelConnection1 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection1.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection1.streamChannelIdentification.streamChannel = 2u;
	channelConnection1.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection1.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 3u };
	channelConnection1.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 4u };
	channelConnection1.clusterIdentification.clusterChannel = 5u;

	auto channelConnection2 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection2.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection2.streamChannelIdentification.streamChannel = 2u;
	channelConnection2.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection2.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 3u };
	channelConnection2.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 4u };
	channelConnection2.clusterIdentification.clusterChannel = 5u;

	EXPECT_EQ(channelConnection1, channelConnection2);
	EXPECT_FALSE(channelConnection1 != channelConnection2);
}

TEST(ChannelConnectionIdentification, EqualityOperator_DifferentListenerStreamIndex)
{
	auto channelConnection1 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection1.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection1.streamChannelIdentification.streamChannel = 2u;
	channelConnection1.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection1.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 3u };
	channelConnection1.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 4u };
	channelConnection1.clusterIdentification.clusterChannel = 5u;

	auto channelConnection2 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection2.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 99u }; // Different listener stream index
	channelConnection2.streamChannelIdentification.streamChannel = 2u;
	channelConnection2.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection2.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 3u };
	channelConnection2.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 4u };
	channelConnection2.clusterIdentification.clusterChannel = 5u;

	EXPECT_NE(channelConnection1, channelConnection2);
	EXPECT_TRUE(channelConnection1 != channelConnection2);
}

TEST(ChannelConnectionIdentification, EqualityOperator_DifferentListenerStreamChannel)
{
	auto channelConnection1 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection1.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection1.streamChannelIdentification.streamChannel = 2u;
	channelConnection1.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection1.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 3u };
	channelConnection1.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 4u };
	channelConnection1.clusterIdentification.clusterChannel = 5u;

	auto channelConnection2 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection2.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection2.streamChannelIdentification.streamChannel = 99u; // Different listener stream channel
	channelConnection2.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection2.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 3u };
	channelConnection2.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 4u };
	channelConnection2.clusterIdentification.clusterChannel = 5u;

	EXPECT_NE(channelConnection1, channelConnection2);
	EXPECT_TRUE(channelConnection1 != channelConnection2);
}

TEST(ChannelConnectionIdentification, EqualityOperator_DifferentTalkerEntityID)
{
	auto channelConnection1 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection1.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection1.streamChannelIdentification.streamChannel = 2u;
	channelConnection1.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection1.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 3u };
	channelConnection1.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 4u };
	channelConnection1.clusterIdentification.clusterChannel = 5u;

	auto channelConnection2 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection2.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection2.streamChannelIdentification.streamChannel = 2u;
	channelConnection2.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050608 }; // Different talker entity ID
	channelConnection2.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 3u };
	channelConnection2.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 4u };
	channelConnection2.clusterIdentification.clusterChannel = 5u;

	EXPECT_NE(channelConnection1, channelConnection2);
	EXPECT_TRUE(channelConnection1 != channelConnection2);
}

TEST(ChannelConnectionIdentification, EqualityOperator_DifferentTalkerStreamIndex)
{
	auto channelConnection1 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection1.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection1.streamChannelIdentification.streamChannel = 2u;
	channelConnection1.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection1.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 3u };
	channelConnection1.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 4u };
	channelConnection1.clusterIdentification.clusterChannel = 5u;

	auto channelConnection2 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection2.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection2.streamChannelIdentification.streamChannel = 2u;
	channelConnection2.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection2.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 99u }; // Different talker stream index
	channelConnection2.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 4u };
	channelConnection2.clusterIdentification.clusterChannel = 5u;

	EXPECT_NE(channelConnection1, channelConnection2);
	EXPECT_TRUE(channelConnection1 != channelConnection2);
}

TEST(ChannelConnectionIdentification, EqualityOperator_DifferentClusterIndex)
{
	auto channelConnection1 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection1.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection1.streamChannelIdentification.streamChannel = 2u;
	channelConnection1.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection1.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 3u };
	channelConnection1.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 4u };
	channelConnection1.clusterIdentification.clusterChannel = 5u;

	auto channelConnection2 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection2.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection2.streamChannelIdentification.streamChannel = 2u;
	channelConnection2.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection2.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 3u };
	channelConnection2.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 99u }; // Different cluster index
	channelConnection2.clusterIdentification.clusterChannel = 5u;

	EXPECT_NE(channelConnection1, channelConnection2);
	EXPECT_TRUE(channelConnection1 != channelConnection2);
}

TEST(ChannelConnectionIdentification, EqualityOperator_DifferentClusterChannel)
{
	auto channelConnection1 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection1.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection1.streamChannelIdentification.streamChannel = 2u;
	channelConnection1.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection1.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 3u };
	channelConnection1.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 4u };
	channelConnection1.clusterIdentification.clusterChannel = 5u;

	auto channelConnection2 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	channelConnection2.streamChannelIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 1u };
	channelConnection2.streamChannelIdentification.streamChannel = 2u;
	channelConnection2.streamIdentification.entityID = la::avdecc::UniqueIdentifier{ 0x0001020304050607 };
	channelConnection2.streamIdentification.streamIndex = la::avdecc::entity::model::StreamIndex{ 3u };
	channelConnection2.clusterIdentification.clusterIndex = la::avdecc::entity::model::ClusterIndex{ 4u };
	channelConnection2.clusterIdentification.clusterChannel = 99u; // Different cluster channel

	EXPECT_NE(channelConnection1, channelConnection2);
	EXPECT_TRUE(channelConnection1 != channelConnection2);
}

TEST(ChannelConnectionIdentification, EqualityOperator_BothInvalid)
{
	auto channelConnection1 = la::avdecc::controller::model::ChannelConnectionIdentification{};
	auto channelConnection2 = la::avdecc::controller::model::ChannelConnectionIdentification{};

	EXPECT_EQ(channelConnection1, channelConnection2);
	EXPECT_FALSE(channelConnection1 != channelConnection2);
}
