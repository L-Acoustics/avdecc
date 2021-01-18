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
* @file controllerCapabilityDelegate_tests.cpp
* @author Christophe Calmejane
*/

// Public API
//#include <la/avdecc/internals/protocolAemAecpdu.hpp>

// Internal API
#include "entity/controllerEntityImpl.hpp"
#include "protocolInterface/protocolInterface_virtual.hpp"

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <future>

namespace
{
class ControllerCapabilityDelegate_F : public ::testing::Test
{
public:
	virtual void SetUp() override
	{
		_pi = std::unique_ptr<la::avdecc::protocol::ProtocolInterfaceVirtual>(la::avdecc::protocol::ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual("VirtualInterface", { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }));
		auto const commonInformation = la::avdecc::entity::Entity::CommonInformation{ la::avdecc::UniqueIdentifier{ 0x0102030405060708 }, la::avdecc::UniqueIdentifier{ 0x1122334455667788 }, la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0u, la::avdecc::entity::TalkerCapabilities{}, 0u, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented }, std::nullopt, std::nullopt };
		auto const interfaceInfo = la::avdecc::entity::Entity::InterfaceInformation{ la::avdecc::networkInterface::MacAddress{ { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, 31u, 0u, std::nullopt, std::nullopt };
		_controllerGuard = std::make_unique<la::avdecc::entity::LocalEntityGuard<la::avdecc::entity::ControllerEntityImpl>>(_pi.get(), commonInformation, la::avdecc::entity::Entity::InterfacesInformation{ { la::avdecc::entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } }, nullptr);
	}

	virtual void TearDown() override {}

	la::avdecc::entity::ControllerEntity& getController() noexcept
	{
		return *_controllerGuard;
	}

private:
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
