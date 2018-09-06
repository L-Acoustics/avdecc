/*
* Copyright (C) 2016-2018, L-Acoustics and its contributors

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
* @file aemPayloads_tests.cpp
* @author Christophe Calmejane
*/

// Internal API
#include "protocol/protocolAemPayloads.hpp"

#include <gtest/gtest.h>
#include <array>
#include <cstdint>

// Test disable on clang/gcc because of a compilation error in the checkPayload template caused by the UniqueIdentifier class (was fine when it was a simple type). TODO: Fix this
#ifdef _WIN32

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
	CHECK_PAYLOAD(AcquireEntityCommand, la::avdecc::protocol::AemAcquireEntityFlags::None, la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(AcquireEntityCommand, la::avdecc::protocol::AemAcquireEntityFlags::Persistent | la::avdecc::protocol::AemAcquireEntityFlags::Release, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, AcquireEntityResponse)
{
	CHECK_PAYLOAD(AcquireEntityResponse, la::avdecc::protocol::AemAcquireEntityFlags::None, la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(AcquireEntityResponse, la::avdecc::protocol::AemAcquireEntityFlags::Persistent | la::avdecc::protocol::AemAcquireEntityFlags::Release, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, LockEntityCommand)
{
	CHECK_PAYLOAD(LockEntityCommand, la::avdecc::protocol::AemLockEntityFlags::None, la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(LockEntityCommand, la::avdecc::protocol::AemLockEntityFlags::Unlock, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, LockEntityResponse)
{
	CHECK_PAYLOAD(LockEntityResponse, la::avdecc::protocol::AemLockEntityFlags::None, la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(LockEntityResponse, la::avdecc::protocol::AemLockEntityFlags::Unlock, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
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
	la::avdecc::entity::model::StreamInfo streamInfo{ la::avdecc::entity::StreamInfoFlags::Connected | la::avdecc::entity::StreamInfoFlags::SavedState, la::avdecc::entity::model::StreamFormat(16132), la::avdecc::UniqueIdentifier(5), std::uint32_t(52), la::avdecc::networkInterface::MacAddress{1,2,3,4,5,6}, std::uint8_t(8), la::avdecc::UniqueIdentifier(99), std::uint16_t(1) };
	CHECK_PAYLOAD(SetStreamInfoCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::StreamInfo{});
	CHECK_PAYLOAD(SetStreamInfoCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5), streamInfo);
}

TEST(AemPayloads, SetStreamInfoResponse)
{
	la::avdecc::entity::model::StreamInfo streamInfo{ la::avdecc::entity::StreamInfoFlags::Connected | la::avdecc::entity::StreamInfoFlags::SavedState, la::avdecc::entity::model::StreamFormat(16132), la::avdecc::UniqueIdentifier(5), std::uint32_t(52), la::avdecc::networkInterface::MacAddress{ 1,2,3,4,5,6 }, std::uint8_t(8), la::avdecc::UniqueIdentifier(99), std::uint16_t(1) };
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
	la::avdecc::entity::model::StreamInfo streamInfo{ la::avdecc::entity::StreamInfoFlags::Connected | la::avdecc::entity::StreamInfoFlags::SavedState, la::avdecc::entity::model::StreamFormat(16132), la::avdecc::UniqueIdentifier(5), std::uint32_t(52), la::avdecc::networkInterface::MacAddress{ 1,2,3,4,5,6 }, std::uint8_t(8), la::avdecc::UniqueIdentifier(99), std::uint16_t(1) };
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
	CHECK_PAYLOAD(SetClockSourceResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(50), la::avdecc::entity::model::ClockSourceIndex(50369));
}

TEST(AemPayloads, GetClockSourceCommand)
{
	CHECK_PAYLOAD(GetClockSourceCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(GetClockSourceCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, GetClockSourceResponse)
{
	CHECK_PAYLOAD(GetClockSourceResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::ClockSourceIndex(0u));
	CHECK_PAYLOAD(GetClockSourceResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(50), la::avdecc::entity::model::ClockSourceIndex(50369));
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

#pragma message("TODO: ReadDescriptorResponse tests")

TEST(AemPayloads, GetAvbInfoCommand)
{
	CHECK_PAYLOAD(GetAvbInfoCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(GetAvbInfoCommand, la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, GetAvbInfoResponse)
{
	la::avdecc::entity::model::AvbInfo avbInfo{ la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier(), std::uint32_t(5), std::uint8_t(2), la::avdecc::entity::AvbInfoFlags::AsCapable, la::avdecc::entity::model::MsrpMappings{} };
	EXPECT_NO_THROW(
		auto const ser = la::avdecc::protocol::aemPayload::serializeGetAvbInfoResponse(la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), avbInfo);
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemGetAvbInfoResponsePayloadMinSize, ser.size());
		auto result = la::avdecc::protocol::aemPayload::deserializeGetAvbInfoResponse({ ser.data(), ser.usedBytes() });
		auto const& info = std::get<2>(result);
		EXPECT_EQ(avbInfo, info);
	) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, GetAudioMapCommand)
{
	CHECK_PAYLOAD(GetAudioMapCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::MapIndex(0));
	CHECK_PAYLOAD(GetAudioMapCommand, la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5), la::avdecc::entity::model::MapIndex(11));
}

TEST(AemPayloads, GetAudioMapResponse)
{
	EXPECT_NO_THROW(
		auto const ser = la::avdecc::protocol::aemPayload::serializeGetAudioMapResponse(la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::MapIndex(0), la::avdecc::entity::model::MapIndex(0), la::avdecc::entity::model::AudioMappings{});
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemGetAudioMapResponsePayloadMinSize, ser.size());
		la::avdecc::protocol::aemPayload::deserializeGetAudioMapResponse({ ser.data(), ser.usedBytes() });
	) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, AddAudioMappingsCommand)
{
	EXPECT_NO_THROW(
		auto const ser = la::avdecc::protocol::aemPayload::serializeAddAudioMappingsCommand(la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::AudioMappings{});
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemAddAudioMappingsCommandPayloadMinSize, ser.size());
		la::avdecc::protocol::aemPayload::deserializeAddAudioMappingsCommand({ ser.data(), ser.usedBytes() });
	) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, AddAudioMappingsResponse)
{
	EXPECT_NO_THROW(
		auto const ser = la::avdecc::protocol::aemPayload::serializeAddAudioMappingsResponse(la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::AudioMappings{});
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemAddAudioMappingsResponsePayloadMinSize, ser.size());
		la::avdecc::protocol::aemPayload::deserializeAddAudioMappingsResponse({ ser.data(), ser.usedBytes() });
	) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, RemoveAudioMappingsCommand)
{
	EXPECT_NO_THROW(
		auto const ser = la::avdecc::protocol::aemPayload::serializeRemoveAudioMappingsCommand(la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::AudioMappings{});
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemRemoveAudioMappingsCommandPayloadMinSize, ser.size());
		la::avdecc::protocol::aemPayload::deserializeRemoveAudioMappingsCommand({ ser.data(), ser.usedBytes() });
	) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, RemoveAudioMappingsResponse)
{
	EXPECT_NO_THROW(
		auto const ser = la::avdecc::protocol::aemPayload::serializeRemoveAudioMappingsResponse(la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::AudioMappings{});
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemRemoveAudioMappingsResponsePayloadMinSize, ser.size());
		la::avdecc::protocol::aemPayload::deserializeRemoveAudioMappingsResponse({ ser.data(), ser.usedBytes() });
	) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, SetMemoryObjectLengthCommand)
{
	EXPECT_NO_THROW(
		auto const ser = la::avdecc::protocol::aemPayload::serializeSetMemoryObjectLengthCommand(la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::MemoryObjectIndex(0), std::uint64_t(0));
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemSetMemoryObjectLengthCommandPayloadSize, ser.size());
		la::avdecc::protocol::aemPayload::deserializeSetMemoryObjectLengthCommand({ ser.data(), ser.usedBytes() });
	) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, SetMemoryObjectLengthResponse)
{
	EXPECT_NO_THROW(
		auto const ser = la::avdecc::protocol::aemPayload::serializeSetMemoryObjectLengthResponse(la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::MemoryObjectIndex(0), std::uint64_t(0));
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemSetMemoryObjectLengthResponsePayloadSize, ser.size());
		la::avdecc::protocol::aemPayload::deserializeSetMemoryObjectLengthResponse({ ser.data(), ser.usedBytes() });
	) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, GetMemoryObjectLengthCommand)
{
	EXPECT_NO_THROW(
		auto const ser = la::avdecc::protocol::aemPayload::serializeGetMemoryObjectLengthCommand(la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::MemoryObjectIndex(0));
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemGetMemoryObjectLengthCommandPayloadSize, ser.size());
		la::avdecc::protocol::aemPayload::deserializeGetMemoryObjectLengthCommand({ ser.data(), ser.usedBytes() });
	) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, GetMemoryObjectLengthResponse)
{
	EXPECT_NO_THROW(
		auto const ser = la::avdecc::protocol::aemPayload::serializeGetMemoryObjectLengthResponse(la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::MemoryObjectIndex(0), std::uint64_t(0));
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemGetMemoryObjectLengthResponsePayloadSize, ser.size());
		la::avdecc::protocol::aemPayload::deserializeGetMemoryObjectLengthResponse({ ser.data(), ser.usedBytes() });
	) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, StartOperationCommand)
{
	try
	{
		auto const ser = la::avdecc::protocol::aemPayload::serializeStartOperationCommand(la::avdecc::entity::model::DescriptorType::MemoryObject, la::avdecc::entity::model::MemoryObjectIndex(55), 10u, la::avdecc::entity::model::MemoryObjectOperationType::StoreAndReboot, la::avdecc::MemoryBuffer{});
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemStartOperationCommandPayloadMinSize, ser.size());
		auto const[descriptorType, descriptorIndex, operationID, operationType, memoryBuffer] = la::avdecc::protocol::aemPayload::deserializeStartOperationCommand({ ser.data(), ser.usedBytes() });
		EXPECT_EQ(la::avdecc::entity::model::DescriptorType::MemoryObject, descriptorType);
		EXPECT_EQ(la::avdecc::entity::model::MemoryObjectIndex(55), descriptorIndex);
		EXPECT_EQ(10u, operationID);
		EXPECT_EQ(la::avdecc::entity::model::MemoryObjectOperationType::StoreAndReboot, operationType);
		EXPECT_TRUE(memoryBuffer.empty());
	}
	catch (...)
	{
		EXPECT_FALSE(true) << "Should not have thrown";
	}

	try
	{
		la::avdecc::MemoryBuffer const buffer{ std::vector<std::uint8_t>{1, 2, 3, 4} };
		auto const ser = la::avdecc::protocol::aemPayload::serializeStartOperationCommand(la::avdecc::entity::model::DescriptorType::MemoryObject, la::avdecc::entity::model::MemoryObjectIndex(8), 60u, la::avdecc::entity::model::MemoryObjectOperationType::Upload, buffer);
		EXPECT_LT(la::avdecc::protocol::aemPayload::AecpAemStartOperationCommandPayloadMinSize, ser.size());
		auto const[descriptorType, descriptorIndex, operationID, operationType, memoryBuffer] = la::avdecc::protocol::aemPayload::deserializeStartOperationCommand({ ser.data(), ser.usedBytes() });
		EXPECT_EQ(la::avdecc::entity::model::DescriptorType::MemoryObject, descriptorType);
		EXPECT_EQ(la::avdecc::entity::model::MemoryObjectIndex(8), descriptorIndex);
		EXPECT_EQ(60u, operationID);
		EXPECT_EQ(la::avdecc::entity::model::MemoryObjectOperationType::Upload, operationType);
		EXPECT_EQ(buffer, memoryBuffer);
	}
	catch (...)
	{
		EXPECT_FALSE(true) << "Should not have thrown";
	}
}

TEST(AemPayloads, StartOperationResponse)
{
	try
	{
		auto const ser = la::avdecc::protocol::aemPayload::serializeStartOperationResponse(la::avdecc::entity::model::DescriptorType::MemoryObject, la::avdecc::entity::model::MemoryObjectIndex(55), 10u, la::avdecc::entity::model::MemoryObjectOperationType::StoreAndReboot, la::avdecc::MemoryBuffer{});
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemStartOperationResponsePayloadMinSize, ser.size());
		auto const[descriptorType, descriptorIndex, operationID, operationType, memoryBuffer] = la::avdecc::protocol::aemPayload::deserializeStartOperationResponse({ ser.data(), ser.usedBytes() });
		EXPECT_EQ(la::avdecc::entity::model::DescriptorType::MemoryObject, descriptorType);
		EXPECT_EQ(la::avdecc::entity::model::MemoryObjectIndex(55), descriptorIndex);
		EXPECT_EQ(10u, operationID);
		EXPECT_EQ(la::avdecc::entity::model::MemoryObjectOperationType::StoreAndReboot, operationType);
		EXPECT_TRUE(memoryBuffer.empty());
	}
	catch (...)
	{
		EXPECT_FALSE(true) << "Should not have thrown";
	}

	try
	{
		la::avdecc::MemoryBuffer const buffer{ std::vector<std::uint8_t>{1, 2, 3, 4} };
		auto const ser = la::avdecc::protocol::aemPayload::serializeStartOperationResponse(la::avdecc::entity::model::DescriptorType::MemoryObject, la::avdecc::entity::model::MemoryObjectIndex(8), 60u, la::avdecc::entity::model::MemoryObjectOperationType::Upload, buffer);
		EXPECT_LT(la::avdecc::protocol::aemPayload::AecpAemStartOperationResponsePayloadMinSize, ser.size());
		auto const[descriptorType, descriptorIndex, operationID, operationType, memoryBuffer] = la::avdecc::protocol::aemPayload::deserializeStartOperationResponse({ ser.data(), ser.usedBytes() });
		EXPECT_EQ(la::avdecc::entity::model::DescriptorType::MemoryObject, descriptorType);
		EXPECT_EQ(la::avdecc::entity::model::MemoryObjectIndex(8), descriptorIndex);
		EXPECT_EQ(60u, operationID);
		EXPECT_EQ(la::avdecc::entity::model::MemoryObjectOperationType::Upload, operationType);
		EXPECT_EQ(buffer, memoryBuffer);
	}
	catch (...)
	{
		EXPECT_FALSE(true) << "Should not have thrown";
	}
}

TEST(AemPayloads, AbortOperationCommand)
{
	try
	{
		auto const ser = la::avdecc::protocol::aemPayload::serializeAbortOperationCommand(la::avdecc::entity::model::DescriptorType::MemoryObject, la::avdecc::entity::model::MemoryObjectIndex(8), 60u);
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemAbortOperationCommandPayloadSize, ser.size());
		auto const[descriptorType, descriptorIndex, operationID] = la::avdecc::protocol::aemPayload::deserializeAbortOperationCommand({ ser.data(), ser.usedBytes() });
		EXPECT_EQ(la::avdecc::entity::model::DescriptorType::MemoryObject, descriptorType);
		EXPECT_EQ(la::avdecc::entity::model::MemoryObjectIndex(8), descriptorIndex);
		EXPECT_EQ(60u, operationID);
	}
	catch (...)
	{
		EXPECT_FALSE(true) << "Should not have thrown";
	}
}

TEST(AemPayloads, AbortOperationResponse)
{
	try
	{
		auto const ser = la::avdecc::protocol::aemPayload::serializeAbortOperationResponse(la::avdecc::entity::model::DescriptorType::MemoryObject, la::avdecc::entity::model::MemoryObjectIndex(8), 60u);
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemAbortOperationResponsePayloadSize, ser.size());
		auto const[descriptorType, descriptorIndex, operationID] = la::avdecc::protocol::aemPayload::deserializeAbortOperationResponse({ ser.data(), ser.usedBytes() });
		EXPECT_EQ(la::avdecc::entity::model::DescriptorType::MemoryObject, descriptorType);
		EXPECT_EQ(la::avdecc::entity::model::MemoryObjectIndex(8), descriptorIndex);
		EXPECT_EQ(60u, operationID);
	}
	catch (...)
	{
		EXPECT_FALSE(true) << "Should not have thrown";
	}
}

TEST(AemPayloads, OperationStatusResponse)
{
	try
	{
		auto const ser = la::avdecc::protocol::aemPayload::serializeOperationStatusResponse(la::avdecc::entity::model::DescriptorType::MemoryObject, la::avdecc::entity::model::MemoryObjectIndex(8), 60u, 99u);
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemOperationStatusResponsePayloadSize, ser.size());
		auto const[descriptorType, descriptorIndex, operationID, percentComplete] = la::avdecc::protocol::aemPayload::deserializeOperationStatusResponse({ ser.data(), ser.usedBytes() });
		EXPECT_EQ(la::avdecc::entity::model::DescriptorType::MemoryObject, descriptorType);
		EXPECT_EQ(la::avdecc::entity::model::MemoryObjectIndex(8), descriptorIndex);
		EXPECT_EQ(60u, operationID);
		EXPECT_EQ(99u, percentComplete);
	}
	catch (...)
	{
		EXPECT_FALSE(true) << "Should not have thrown";
	}
}

#endif // _WIN32
