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

#include <gtest/gtest.h>
#include "protocol/protocolAemPayloads.hpp"

#define CHECK_PAYLOAD(MessageName, ...) checkPayload(la::avdecc::protocol::aemPayload::serialize##MessageName, la::avdecc::protocol::aemPayload::deserialize##MessageName, la::avdecc::protocol::aemPayload::AecpAem##MessageName##PayloadSize, __VA_ARGS__);
template<typename SerializeMethod, typename DeserializeMethod, typename... Parameters>
void checkPayload(SerializeMethod&& serializeMethod, DeserializeMethod&& deserializeMethod, size_t const payloadSize, Parameters&&... params)
{
	EXPECT_NO_THROW(
		auto const ser = serializeMethod(std::forward<Parameters>(params)...);
		EXPECT_EQ(payloadSize, ser.size());

		auto const inputTuple = std::tuple<Parameters...>(std::forward<Parameters>(params)...);
		auto const result = deserializeMethod({ ser.data(), ser.usedBytes() });

		EXPECT_EQ(inputTuple, result);
		);
}

TEST(AemPayloads, AcquireEntityCommand)
{
	CHECK_PAYLOAD(AcquireEntityCommand, la::avdecc::protocol::AemAcquireEntityFlags::None, la::avdecc::getUninitializedIdentifier(), la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(AcquireEntityCommand, la::avdecc::protocol::AemAcquireEntityFlags::Persistent | la::avdecc::protocol::AemAcquireEntityFlags::Release, la::avdecc::getNullIdentifier(), la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, AcquireEntityResponse)
{
	CHECK_PAYLOAD(AcquireEntityResponse, la::avdecc::protocol::AemAcquireEntityFlags::None, la::avdecc::getUninitializedIdentifier(), la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(AcquireEntityResponse, la::avdecc::protocol::AemAcquireEntityFlags::Persistent | la::avdecc::protocol::AemAcquireEntityFlags::Release, la::avdecc::getNullIdentifier(), la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, LockEntityCommand)
{
	CHECK_PAYLOAD(LockEntityCommand, la::avdecc::protocol::AemLockEntityFlags::None, la::avdecc::getUninitializedIdentifier(), la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(LockEntityCommand, la::avdecc::protocol::AemLockEntityFlags::Unlock, la::avdecc::getNullIdentifier(), la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, LockEntityResponse)
{
	CHECK_PAYLOAD(LockEntityResponse, la::avdecc::protocol::AemLockEntityFlags::None, la::avdecc::getUninitializedIdentifier(), la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(LockEntityResponse, la::avdecc::protocol::AemLockEntityFlags::Unlock, la::avdecc::getNullIdentifier(), la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, SetNameCommand)
{
	CHECK_PAYLOAD(SetNameCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), std::uint16_t(0u), la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::AvdeccFixedString("Hi"));
	CHECK_PAYLOAD(SetNameCommand, la::avdecc::entity::model::DescriptorType::AudioCluster, la::avdecc::entity::model::DescriptorIndex(5), std::uint16_t(8u), la::avdecc::entity::model::ConfigurationIndex(16), la::avdecc::entity::model::AvdeccFixedString("Hi"));
}

TEST(AemPayloads, SetNameResponse)
{
	CHECK_PAYLOAD(SetNameResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), std::uint16_t(0u), la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::AvdeccFixedString("Hi"));
	CHECK_PAYLOAD(SetNameResponse, la::avdecc::entity::model::DescriptorType::AudioUnit, la::avdecc::entity::model::DescriptorIndex(18), std::uint16_t(22u), la::avdecc::entity::model::ConfigurationIndex(44), la::avdecc::entity::model::AvdeccFixedString("Hi"));
}

TEST(AemPayloads, GetNameCommand)
{
	CHECK_PAYLOAD(GetNameCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), std::uint16_t(0u), la::avdecc::entity::model::ConfigurationIndex(0));
	CHECK_PAYLOAD(GetNameCommand, la::avdecc::entity::model::DescriptorType::SignalTranscoder, la::avdecc::entity::model::DescriptorIndex(100), std::uint16_t(20u), la::avdecc::entity::model::ConfigurationIndex(101));
}

TEST(AemPayloads, GetNameResponse)
{
	CHECK_PAYLOAD(GetNameResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), std::uint16_t(0u), la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::AvdeccFixedString("Hi"));
	CHECK_PAYLOAD(GetNameResponse, la::avdecc::entity::model::DescriptorType::JackInput, la::avdecc::entity::model::DescriptorIndex(0), std::uint16_t(19u), la::avdecc::entity::model::ConfigurationIndex(27), la::avdecc::entity::model::AvdeccFixedString("Hi"));
}
