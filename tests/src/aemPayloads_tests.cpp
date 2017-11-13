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
#include <array>
#include <cstdint>
#include "protocol/protocolAemPayloads.hpp"

#define CHECK_PAYLOAD(MessageName, ...) checkPayload<la::avdecc::protocol::aemPayload::AecpAem##MessageName##PayloadSize>(la::avdecc::protocol::aemPayload::serialize##MessageName, la::avdecc::protocol::aemPayload::deserialize##MessageName, __VA_ARGS__);
template<size_t PayloadSize, typename SerializeMethod, typename DeserializeMethod, typename... Parameters>
void checkPayload(SerializeMethod&& serializeMethod, DeserializeMethod&& deserializeMethod, Parameters&&... params)
{
	EXPECT_NO_THROW(
		auto const ser = serializeMethod(std::forward<Parameters>(params)...);
		EXPECT_EQ(PayloadSize, ser.size());

		auto const inputTuple = std::tuple<Parameters...>(std::forward<Parameters>(params)...);
		auto const result = deserializeMethod({ ser.data(), ser.usedBytes() });

		EXPECT_EQ(inputTuple, result);
		) << "Serialization/deserialization should not throw anything";

	{
		auto ser = serializeMethod(std::forward<Parameters>(params)...);
		EXPECT_THROW(
			ser << std::uint8_t(0u);
		, std::invalid_argument) << "Trying to serialize past the buffer should throw";
	}

	{
		std::array<std::uint8_t, PayloadSize - 1> const buf{ 0u };
		EXPECT_THROW(
			deserializeMethod({ buf.data(), buf.size() });
		, la::avdecc::protocol::aemPayload::IncorrectPayloadSizeException) << "Trying to deserialize past the buffer should throw";
	}
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

TEST(AemPayloads, SetConfigurationCommand)
{
	CHECK_PAYLOAD(SetConfigurationCommand, la::avdecc::entity::model::ConfigurationIndex(0));
	CHECK_PAYLOAD(SetConfigurationCommand, la::avdecc::entity::model::ConfigurationIndex(5));
}

TEST(AemPayloads, SetConfigurationResponse)
{
	CHECK_PAYLOAD(SetConfigurationResponse, la::avdecc::entity::model::ConfigurationIndex(0));
	CHECK_PAYLOAD(SetConfigurationResponse, la::avdecc::entity::model::ConfigurationIndex(5));
}

TEST(AemPayloads, GetConfigurationResponse)
{
	CHECK_PAYLOAD(GetConfigurationResponse, la::avdecc::entity::model::ConfigurationIndex(0));
	CHECK_PAYLOAD(GetConfigurationResponse, la::avdecc::entity::model::ConfigurationIndex(5));
}

TEST(AemPayloads, SetStreamFormatCommand)
{
	CHECK_PAYLOAD(SetStreamFormatCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::getNullStreamFormat());
	CHECK_PAYLOAD(SetStreamFormatCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5), la::avdecc::entity::model::StreamFormat(159));
}

TEST(AemPayloads, SetStreamFormatResponse)
{
	CHECK_PAYLOAD(SetStreamFormatResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::getNullStreamFormat());
	CHECK_PAYLOAD(SetStreamFormatResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(50), la::avdecc::entity::model::StreamFormat(501369));
}

TEST(AemPayloads, GetStreamFormatCommand)
{
	CHECK_PAYLOAD(GetStreamFormatCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(GetStreamFormatCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, GetStreamFormatResponse)
{
	CHECK_PAYLOAD(GetStreamFormatResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::getNullStreamFormat());
	CHECK_PAYLOAD(GetStreamFormatResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(50), la::avdecc::entity::model::StreamFormat(501369));
}

TEST(AemPayloads, SetStreamInfoCommand)
{
	la::avdecc::entity::model::StreamInfo streamInfo{ la::avdecc::operator|(la::avdecc::entity::StreamInfoFlags::Connected, la::avdecc::entity::StreamInfoFlags::SavedState), la::avdecc::entity::model::StreamFormat(16132), la::avdecc::UniqueIdentifier(5), std::uint32_t(52), la::avdecc::networkInterface::MacAddress{1,2,3,4,5,6}, std::uint8_t(8), la::avdecc::UniqueIdentifier(99), std::uint16_t(1) };
	CHECK_PAYLOAD(SetStreamInfoCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::StreamInfo{});
	CHECK_PAYLOAD(SetStreamInfoCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5), streamInfo);
}

TEST(AemPayloads, SetStreamInfoResponse)
{
	la::avdecc::entity::model::StreamInfo streamInfo{ la::avdecc::operator|(la::avdecc::entity::StreamInfoFlags::Connected, la::avdecc::entity::StreamInfoFlags::SavedState), la::avdecc::entity::model::StreamFormat(16132), la::avdecc::UniqueIdentifier(5), std::uint32_t(52), la::avdecc::networkInterface::MacAddress{ 1,2,3,4,5,6 }, std::uint8_t(8), la::avdecc::UniqueIdentifier(99), std::uint16_t(1) };
	CHECK_PAYLOAD(SetStreamInfoResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::StreamInfo{});
	CHECK_PAYLOAD(SetStreamInfoResponse, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5), streamInfo);
}

TEST(AemPayloads, GetStreamInfoCommand)
{
	CHECK_PAYLOAD(GetStreamInfoCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(GetStreamInfoCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, GetStreamInfoResponse)
{
	la::avdecc::entity::model::StreamInfo streamInfo{ la::avdecc::operator|(la::avdecc::entity::StreamInfoFlags::Connected, la::avdecc::entity::StreamInfoFlags::SavedState), la::avdecc::entity::model::StreamFormat(16132), la::avdecc::UniqueIdentifier(5), std::uint32_t(52), la::avdecc::networkInterface::MacAddress{ 1,2,3,4,5,6 }, std::uint8_t(8), la::avdecc::UniqueIdentifier(99), std::uint16_t(1) };
	CHECK_PAYLOAD(GetStreamInfoResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::StreamInfo{});
	CHECK_PAYLOAD(GetStreamInfoResponse, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5), streamInfo);
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

TEST(AemPayloads, SetSamplingRateCommand)
{
	CHECK_PAYLOAD(SetSamplingRateCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::getNullSamplingRate());
	CHECK_PAYLOAD(SetSamplingRateCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5), la::avdecc::entity::model::SamplingRate(159));
}

TEST(AemPayloads, SetSamplingRateResponse)
{
	CHECK_PAYLOAD(SetSamplingRateResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::getNullSamplingRate());
	CHECK_PAYLOAD(SetSamplingRateResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(50), la::avdecc::entity::model::SamplingRate(501369));
}

TEST(AemPayloads, GetSamplingRateCommand)
{
	CHECK_PAYLOAD(GetSamplingRateCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(GetSamplingRateCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, GetSamplingRateResponse)
{
	CHECK_PAYLOAD(GetSamplingRateResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::ClockSourceIndex(0u));
	CHECK_PAYLOAD(GetSamplingRateResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(50), la::avdecc::entity::model::SamplingRate(501369));
}

TEST(AemPayloads, SetClockSourceCommand)
{
	CHECK_PAYLOAD(SetClockSourceCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::ClockSourceIndex(0u));
	CHECK_PAYLOAD(SetClockSourceCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5), la::avdecc::entity::model::ClockSourceIndex(159));
}

TEST(AemPayloads, SetClockSourceResponse)
{
	CHECK_PAYLOAD(SetClockSourceResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::ClockSourceIndex(0u));
	CHECK_PAYLOAD(SetClockSourceResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(50), la::avdecc::entity::model::ClockSourceIndex(501369));
}

TEST(AemPayloads, GetClockSourceCommand)
{
	CHECK_PAYLOAD(GetClockSourceCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(GetClockSourceCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, GetClockSourceResponse)
{
	CHECK_PAYLOAD(GetClockSourceResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::ClockSourceIndex(0u));
	CHECK_PAYLOAD(GetClockSourceResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(50), la::avdecc::entity::model::ClockSourceIndex(501369));
}

TEST(AemPayloads, StartStreamingCommand)
{
	CHECK_PAYLOAD(StartStreamingCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(StartStreamingCommand, la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, StartStreamingResponse)
{
	CHECK_PAYLOAD(StartStreamingResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(StartStreamingResponse, la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, StopStreamingCommand)
{
	CHECK_PAYLOAD(StopStreamingCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(StopStreamingCommand, la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, StopStreamingResponse)
{
	CHECK_PAYLOAD(StopStreamingResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(StopStreamingResponse, la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, ReadDescriptorCommand)
{
	CHECK_PAYLOAD(ReadDescriptorCommand, la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(ReadDescriptorCommand, la::avdecc::entity::model::ConfigurationIndex(123), la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}
