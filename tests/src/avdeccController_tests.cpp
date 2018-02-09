/*
* Copyright (C) 2016-2017, L-Acoustics and its contributors

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
* @file controller_tests.cpp
* @author Christophe Calmejane
*/

#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <chrono>
#include <future>
#include "controller/avdeccControlledEntityImpl.hpp"
#include "entity/controllerEntityImpl.hpp"
#include "protocolInterface/protocolInterface_virtual.hpp"
#include "protocol/protocolAemAecpdu.hpp"
#include <la/avdecc/controller/avdeccController.hpp>

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
			_serializedModel += "pdt" + std::to_string(la::avdecc::to_integral(node->descriptorType)) + ",";
	}
	void serializeNode(la::avdecc::controller::model::EntityModelNode const& node) noexcept
	{
		_serializedModel += "dt" + std::to_string(la::avdecc::to_integral(node.descriptorType)) + ",";
		_serializedModel += "di" + std::to_string(node.descriptorIndex) + ",";
	}
	void serializeNode(la::avdecc::controller::model::VirtualNode const& node) noexcept
	{
		_serializedModel += "dt" + std::to_string(la::avdecc::to_integral(node.descriptorType)) + ",";
		_serializedModel += "vi" + std::to_string(node.virtualIndex) + ",";
	}

	// la::avdecc::controller::model::EntityModelVisitor overrides
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::EntityNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::ConfigurationNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::AudioUnitNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::StreamNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::AvbInterfaceNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::ClockSourceNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::LocaleNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::StringsNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::StreamPortNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::AudioClusterNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::AudioMapNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::RedundantStreamNode const& node) noexcept override
	{
		serializeNode(parent);
		serializeNode(node);
		_serializedModel += "rsi";
		for (auto const& streamKV : node.redundantStreams)
		{
			auto const streamIndex = streamKV.first;
			_serializedModel += std::to_string(streamIndex) + "+";
		}
	}

	std::string _serializedModel{};
};
};

TEST(Controller, RedundantStreams)
{
	std::string singleLinkedModel{};
	std::string doubleLinkedModel{};

	// Single linked model
	{
		auto const e{ la::avdecc::entity::Entity{ 0x0102030405060708, la::avdecc::networkInterface::MacAddress{}, 0x1122334455667788, la::avdecc::entity::EntityCapabilities::AemSupported, 0, la::avdecc::entity::TalkerCapabilities::None, 0, la::avdecc::entity::ListenerCapabilities::None, la::avdecc::entity::ControllerCapabilities::None, 0, 0, la::avdecc::getNullIdentifier() } };
		la::avdecc::controller::ControlledEntityImpl entity{ e };

		entity.setEntityDescriptor(la::avdecc::entity::model::EntityDescriptor{ 0x0102030405060708 , 0x1122334455667788 , la::avdecc::entity::EntityCapabilities::AemSupported, 0, la::avdecc::entity::TalkerCapabilities::None, 0, la::avdecc::entity::ListenerCapabilities::None, la::avdecc::entity::ControllerCapabilities::None, 0, la::avdecc::getNullIdentifier() , std::string("Test entity"), la::avdecc::entity::model::getNullLocalizedStringReference(), la::avdecc::entity::model::getNullLocalizedStringReference(), std::string("Test firmware"), std::string("Test group"), std::string("Test serial number"), 1, 0 });
		entity.setConfigurationDescriptor(la::avdecc::entity::model::ConfigurationDescriptor{ std::string("Test configuration"), la::avdecc::entity::model::getNullLocalizedStringReference(),{ { la::avdecc::entity::model::DescriptorType::StreamInput, 3 } } }, 0);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Test stream 1"), la::avdecc::entity::model::getNullLocalizedStringReference() , 0, la::avdecc::entity::StreamFlags::None, la::avdecc::entity::model::getNullStreamFormat(), la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, 0, 0,{},{} }, 0, 0);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Secondary stream 2"), la::avdecc::entity::model::getNullLocalizedStringReference() , 0, la::avdecc::entity::StreamFlags::None, la::avdecc::entity::model::getNullStreamFormat(), la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, 0, 0,{},{} }, 0, 1);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Primary stream 2"), la::avdecc::entity::model::getNullLocalizedStringReference() , 0, la::avdecc::entity::StreamFlags::None, la::avdecc::entity::model::getNullStreamFormat(), la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, 0, 0,{},{ 1 } }, 0, 2);

		EntityModelVisitor serializer{};
		entity.accept(&serializer);
		singleLinkedModel = serializer.getSerializedModel();
	}
	// Double linked model
	{
		auto const e{ la::avdecc::entity::Entity{ 0x0102030405060708, la::avdecc::networkInterface::MacAddress{}, 0x1122334455667788, la::avdecc::entity::EntityCapabilities::AemSupported, 0, la::avdecc::entity::TalkerCapabilities::None, 0, la::avdecc::entity::ListenerCapabilities::None, la::avdecc::entity::ControllerCapabilities::None, 0, 0, la::avdecc::getNullIdentifier() } };
		la::avdecc::controller::ControlledEntityImpl entity{ e };

		entity.setEntityDescriptor(la::avdecc::entity::model::EntityDescriptor{ 0x0102030405060708 , 0x1122334455667788 , la::avdecc::entity::EntityCapabilities::AemSupported, 0, la::avdecc::entity::TalkerCapabilities::None, 0, la::avdecc::entity::ListenerCapabilities::None, la::avdecc::entity::ControllerCapabilities::None, 0, la::avdecc::getNullIdentifier() , std::string("Test entity"), la::avdecc::entity::model::getNullLocalizedStringReference(), la::avdecc::entity::model::getNullLocalizedStringReference(), std::string("Test firmware"), std::string("Test group"), std::string("Test serial number"), 1, 0 });
		entity.setConfigurationDescriptor(la::avdecc::entity::model::ConfigurationDescriptor{ std::string("Test configuration"), la::avdecc::entity::model::getNullLocalizedStringReference(),{ { la::avdecc::entity::model::DescriptorType::StreamInput, 3 } } }, 0);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Test stream 1"), la::avdecc::entity::model::getNullLocalizedStringReference() , 0, la::avdecc::entity::StreamFlags::None, la::avdecc::entity::model::getNullStreamFormat(), la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, 0, 0,{},{} }, 0, 0);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Secondary stream 2"), la::avdecc::entity::model::getNullLocalizedStringReference() , 0, la::avdecc::entity::StreamFlags::None, la::avdecc::entity::model::getNullStreamFormat(), la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, 0, 0,{},{ 2 } }, 0, 1);
		entity.setStreamInputDescriptor(la::avdecc::entity::model::StreamDescriptor{ std::string("Primary stream 2"), la::avdecc::entity::model::getNullLocalizedStringReference() , 0, la::avdecc::entity::StreamFlags::None, la::avdecc::entity::model::getNullStreamFormat(), la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, la::avdecc::getNullIdentifier(), 0, 0, 0,{},{ 1 } }, 0, 2);

		EntityModelVisitor serializer{};
		entity.accept(&serializer);
		doubleLinkedModel = serializer.getSerializedModel();
	}

	EXPECT_STREQ(singleLinkedModel.c_str(), doubleLinkedModel.c_str());
}

TEST(Controller, DestroyWhileSending)
{
	static std::promise<void> commandResultPromise{};
	{
		auto pi = la::avdecc::protocol::ProtocolInterfaceVirtual::create("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } });
		auto controllerGuard = std::make_unique<la::avdecc::entity::LocalEntityGuard<la::avdecc::entity::ControllerEntityImpl>>(pi.get(), 1, 0, nullptr);
		auto* const controller = static_cast<la::avdecc::entity::ControllerEntity*>(controllerGuard.get());

		controller->getListenerStreamState(0x000102FFFE030405, 0, [](la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const listenerEntityID, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, la::avdecc::UniqueIdentifier const talkerEntityID, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status)
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
