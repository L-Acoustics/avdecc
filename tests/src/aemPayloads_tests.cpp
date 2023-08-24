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
* @file aemPayloads_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/internals/entityModelControlValuesTraits.hpp>
#include <la/avdecc/internals/entityModelControlValues.hpp>
#ifdef ENABLE_AVDECC_FEATURE_JSON
#	include <la/avdecc/internals/jsonTypes.hpp>
#endif // ENABLE_AVDECC_FEATURE_JSON

// Internal API
#include "protocol/protocolAemPayloads.hpp"

#include <gtest/gtest.h>
#include <array>
#include <cstdint>

// Test disable on gcc because of a compilation error in the checkPayload template caused by the UniqueIdentifier class (was fine when it was a simple type). TODO: Fix this
#if defined(_WIN32) || defined(__APPLE__)

#	define CHECK_PAYLOAD(MessageName, ...) checkPayload<true, true, false, la::avdecc::protocol::aemPayload::AecpAem##MessageName##PayloadSize>(la::avdecc::protocol::aemPayload::serialize##MessageName, la::avdecc::protocol::aemPayload::deserialize##MessageName, __VA_ARGS__);
#	define CHECK_PAYLOAD_WITH_STATUS(MessageName, ...) checkPayload<true, true, true, la::avdecc::protocol::aemPayload::AecpAem##MessageName##PayloadSize>(la::avdecc::protocol::aemPayload::serialize##MessageName, la::avdecc::protocol::aemPayload::deserialize##MessageName, __VA_ARGS__);
#	define CHECK_PAYLOAD_SIZED(PayloadSize, checkSerializePastBuffer, checkDeserializePastBuffer, MessageName, ...) checkPayload<checkSerializePastBuffer, checkDeserializePastBuffer, true, PayloadSize>(la::avdecc::protocol::aemPayload::serialize##MessageName, la::avdecc::protocol::aemPayload::deserialize##MessageName, __VA_ARGS__);
template<bool CheckSerializePastBuffer, bool CheckDeserializePastBuffer, bool DeserializeHasCommandStatus, size_t PayloadSize, typename SerializeMethod, typename DeserializeMethod, typename... Parameters>
void checkPayload(SerializeMethod&& serializeMethod, DeserializeMethod&& deserializeMethod, Parameters&&... params)
{
	EXPECT_NO_THROW(auto const ser = serializeMethod(std::forward<Parameters>(params)...); EXPECT_EQ(PayloadSize, ser.size());

									auto const inputTuple = std::tuple<Parameters...>(std::forward<Parameters>(params)...); if constexpr (DeserializeHasCommandStatus) {
										auto const result = deserializeMethod(la::avdecc::entity::LocalEntity::AemCommandStatus::Success, { ser.data(), ser.usedBytes() });
										EXPECT_EQ(inputTuple, result);
									} else {
										auto const result = deserializeMethod({ ser.data(), ser.usedBytes() });
										EXPECT_EQ(inputTuple, result);
									})
		<< "Serialization/deserialization should not throw anything";

	if constexpr (CheckSerializePastBuffer)
	{
		auto ser = serializeMethod(std::forward<Parameters>(params)...);
		EXPECT_THROW(ser << std::uint8_t(0u);, std::invalid_argument) << "Trying to serialize past the buffer should throw";
	}

	if constexpr (CheckDeserializePastBuffer)
	{
		std::array<std::uint8_t, PayloadSize - 1> const buf{ 0u };
		if constexpr (DeserializeHasCommandStatus)
		{
			EXPECT_THROW(deserializeMethod(la::avdecc::entity::LocalEntity::AemCommandStatus::Success, { buf.data(), buf.size() });, la::avdecc::protocol::aemPayload::IncorrectPayloadSizeException) << "Trying to deserialize past the buffer should throw";
		}
		else
		{
			EXPECT_THROW(deserializeMethod({ buf.data(), buf.size() });, la::avdecc::protocol::aemPayload::IncorrectPayloadSizeException) << "Trying to deserialize past the buffer should throw";
		}
	}
}

TEST(AemPayloads, AcquireEntityCommand)
{
	CHECK_PAYLOAD(AcquireEntityCommand, la::avdecc::protocol::AemAcquireEntityFlags::None, la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(AcquireEntityCommand, la::avdecc::protocol::AemAcquireEntityFlags::Persistent | la::avdecc::protocol::AemAcquireEntityFlags::Release, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, AcquireEntityResponse)
{
	CHECK_PAYLOAD_WITH_STATUS(AcquireEntityResponse, la::avdecc::protocol::AemAcquireEntityFlags::None, la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD_WITH_STATUS(AcquireEntityResponse, la::avdecc::protocol::AemAcquireEntityFlags::Persistent | la::avdecc::protocol::AemAcquireEntityFlags::Release, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, LockEntityCommand)
{
	CHECK_PAYLOAD(LockEntityCommand, la::avdecc::protocol::AemLockEntityFlags::None, la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(LockEntityCommand, la::avdecc::protocol::AemLockEntityFlags::Unlock, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, LockEntityResponse)
{
	CHECK_PAYLOAD_WITH_STATUS(LockEntityResponse, la::avdecc::protocol::AemLockEntityFlags::None, la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD_WITH_STATUS(LockEntityResponse, la::avdecc::protocol::AemLockEntityFlags::Unlock, la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, SetConfigurationCommand)
{
	CHECK_PAYLOAD(SetConfigurationCommand, la::avdecc::entity::model::ConfigurationIndex(0));
	CHECK_PAYLOAD(SetConfigurationCommand, la::avdecc::entity::model::ConfigurationIndex(5));
}

TEST(AemPayloads, SetConfigurationResponse)
{
	CHECK_PAYLOAD_WITH_STATUS(SetConfigurationResponse, la::avdecc::entity::model::ConfigurationIndex(0));
	CHECK_PAYLOAD_WITH_STATUS(SetConfigurationResponse, la::avdecc::entity::model::ConfigurationIndex(5));
}

TEST(AemPayloads, GetConfigurationResponse)
{
	CHECK_PAYLOAD_WITH_STATUS(GetConfigurationResponse, la::avdecc::entity::model::ConfigurationIndex(0));
	CHECK_PAYLOAD_WITH_STATUS(GetConfigurationResponse, la::avdecc::entity::model::ConfigurationIndex(5));
}

TEST(AemPayloads, SetStreamFormatCommand)
{
	CHECK_PAYLOAD(SetStreamFormatCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::StreamFormat{});
	CHECK_PAYLOAD(SetStreamFormatCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5), la::avdecc::entity::model::StreamFormat(159));
}

TEST(AemPayloads, SetStreamFormatResponse)
{
	CHECK_PAYLOAD_WITH_STATUS(SetStreamFormatResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::StreamFormat{});
	CHECK_PAYLOAD_WITH_STATUS(SetStreamFormatResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(50), la::avdecc::entity::model::StreamFormat(501369));
}

TEST(AemPayloads, GetStreamFormatCommand)
{
	CHECK_PAYLOAD(GetStreamFormatCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(GetStreamFormatCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, GetStreamFormatResponse)
{
	CHECK_PAYLOAD_WITH_STATUS(GetStreamFormatResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::StreamFormat{});
	CHECK_PAYLOAD_WITH_STATUS(GetStreamFormatResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(50), la::avdecc::entity::model::StreamFormat(501369));
}

TEST(AemPayloads, SetStreamInfoCommand)
{
	la::avdecc::entity::model::StreamInfo streamInfo{ la::avdecc::entity::StreamInfoFlags{ la::avdecc::entity::StreamInfoFlag::Connected } | la::avdecc::entity::StreamInfoFlags{ la::avdecc::entity::StreamInfoFlag::SavedState }, la::avdecc::entity::model::StreamFormat(16132), la::avdecc::UniqueIdentifier(5), std::uint32_t(52), la::networkInterface::MacAddress{ 1, 2, 3, 4, 5, 6 }, la::avdecc::entity::model::MsrpFailureCode{ 8 }, la::avdecc::UniqueIdentifier(99), std::uint16_t(1) };
	CHECK_PAYLOAD(SetStreamInfoCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::StreamInfo{});
	CHECK_PAYLOAD(SetStreamInfoCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5), streamInfo);
}

TEST(AemPayloads, SetStreamInfoResponse)
{
	la::avdecc::entity::model::StreamInfo streamInfo{ la::avdecc::entity::StreamInfoFlags{ la::avdecc::entity::StreamInfoFlag::Connected } | la::avdecc::entity::StreamInfoFlags{ la::avdecc::entity::StreamInfoFlag::SavedState }, la::avdecc::entity::model::StreamFormat(16132), la::avdecc::UniqueIdentifier(5), std::uint32_t(52), la::networkInterface::MacAddress{ 1, 2, 3, 4, 5, 6 }, la::avdecc::entity::model::MsrpFailureCode{ 8 }, la::avdecc::UniqueIdentifier(99), std::uint16_t(1) };
	CHECK_PAYLOAD_WITH_STATUS(SetStreamInfoResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::StreamInfo{});
	CHECK_PAYLOAD_WITH_STATUS(SetStreamInfoResponse, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5), streamInfo);
}

TEST(AemPayloads, GetStreamInfoCommand)
{
	CHECK_PAYLOAD(GetStreamInfoCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(GetStreamInfoCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, GetStreamInfoResponse)
{
	//la::avdecc::entity::model::StreamInfo streamInfo{ la::avdecc::entity::StreamInfoFlags{ la::avdecc::entity::StreamInfoFlag::Connected } | la::avdecc::entity::StreamInfoFlags{ la::avdecc::entity::StreamInfoFlag::SavedState }, la::avdecc::entity::model::StreamFormat(16132), la::avdecc::UniqueIdentifier(5), std::uint32_t(52), la::networkInterface::MacAddress{ 1, 2, 3, 4, 5, 6 }, la::avdecc::entity::model::MsrpFailureCode{ 8 }, la::avdecc::UniqueIdentifier(99), std::uint16_t(1) };
	la::avdecc::entity::model::StreamInfo streamInfoMilan{ la::avdecc::entity::StreamInfoFlags{ la::avdecc::entity::StreamInfoFlag::Connected } | la::avdecc::entity::StreamInfoFlags{ la::avdecc::entity::StreamInfoFlag::SavedState }, la::avdecc::entity::model::StreamFormat(16132), la::avdecc::UniqueIdentifier(5), std::uint32_t(52), la::networkInterface::MacAddress{ 1, 2, 3, 4, 5, 6 }, la::avdecc::entity::model::MsrpFailureCode{ 8 }, la::avdecc::UniqueIdentifier(99), std::uint16_t(1), la::avdecc::entity::StreamInfoFlagsEx{ la::avdecc::entity::StreamInfoFlagEx::Registering }, la::avdecc::entity::model::ProbingStatus::Active, la::avdecc::protocol::AcmpStatus::ListenerMisbehaving };
	//CHECK_PAYLOAD_SIZED(la::avdecc::protocol::aemPayload::AecpAemGetStreamInfoResponsePayloadSize, false, true, GetStreamInfoResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::StreamInfo{});
	//CHECK_PAYLOAD_SIZED(la::avdecc::protocol::aemPayload::AecpAemGetStreamInfoResponsePayloadSize, false, true, GetStreamInfoResponse, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5), streamInfo);
	CHECK_PAYLOAD_SIZED(la::avdecc::protocol::aemPayload::AecpAemMilanGetStreamInfoResponsePayloadSize, true, false, GetStreamInfoResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(66), streamInfoMilan);
}

TEST(AemPayloads, SetNameCommand)
{
	CHECK_PAYLOAD(SetNameCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), std::uint16_t(0u), la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::AvdeccFixedString("Hi"));
	CHECK_PAYLOAD(SetNameCommand, la::avdecc::entity::model::DescriptorType::AudioCluster, la::avdecc::entity::model::DescriptorIndex(5), std::uint16_t(8u), la::avdecc::entity::model::ConfigurationIndex(16), la::avdecc::entity::model::AvdeccFixedString("Hi"));
}

TEST(AemPayloads, SetNameResponse)
{
	CHECK_PAYLOAD_WITH_STATUS(SetNameResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), std::uint16_t(0u), la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::AvdeccFixedString("Hi"));
	CHECK_PAYLOAD_WITH_STATUS(SetNameResponse, la::avdecc::entity::model::DescriptorType::AudioUnit, la::avdecc::entity::model::DescriptorIndex(18), std::uint16_t(22u), la::avdecc::entity::model::ConfigurationIndex(44), la::avdecc::entity::model::AvdeccFixedString("Hi"));
}

TEST(AemPayloads, GetNameCommand)
{
	CHECK_PAYLOAD(GetNameCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), std::uint16_t(0u), la::avdecc::entity::model::ConfigurationIndex(0));
	CHECK_PAYLOAD(GetNameCommand, la::avdecc::entity::model::DescriptorType::SignalTranscoder, la::avdecc::entity::model::DescriptorIndex(100), std::uint16_t(20u), la::avdecc::entity::model::ConfigurationIndex(101));
}

TEST(AemPayloads, GetNameResponse)
{
	CHECK_PAYLOAD_WITH_STATUS(GetNameResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), std::uint16_t(0u), la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::AvdeccFixedString("Hi"));
	CHECK_PAYLOAD_WITH_STATUS(GetNameResponse, la::avdecc::entity::model::DescriptorType::JackInput, la::avdecc::entity::model::DescriptorIndex(0), std::uint16_t(19u), la::avdecc::entity::model::ConfigurationIndex(27), la::avdecc::entity::model::AvdeccFixedString("Hi"));
}

TEST(AemPayloads, SetSamplingRateCommand)
{
	CHECK_PAYLOAD(SetSamplingRateCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::SamplingRate{});
	CHECK_PAYLOAD(SetSamplingRateCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5), la::avdecc::entity::model::SamplingRate(159));
}

TEST(AemPayloads, SetSamplingRateResponse)
{
	CHECK_PAYLOAD_WITH_STATUS(SetSamplingRateResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::SamplingRate{});
	CHECK_PAYLOAD_WITH_STATUS(SetSamplingRateResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(50), la::avdecc::entity::model::SamplingRate(501369));
}

TEST(AemPayloads, GetSamplingRateCommand)
{
	CHECK_PAYLOAD(GetSamplingRateCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(GetSamplingRateCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, GetSamplingRateResponse)
{
	CHECK_PAYLOAD_WITH_STATUS(GetSamplingRateResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::SamplingRate(0u));
	CHECK_PAYLOAD_WITH_STATUS(GetSamplingRateResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(50), la::avdecc::entity::model::SamplingRate(501369));
}

TEST(AemPayloads, SetClockSourceCommand)
{
	CHECK_PAYLOAD(SetClockSourceCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::ClockSourceIndex(0u));
	CHECK_PAYLOAD(SetClockSourceCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5), la::avdecc::entity::model::ClockSourceIndex(159));
}

TEST(AemPayloads, SetClockSourceResponse)
{
	CHECK_PAYLOAD_WITH_STATUS(SetClockSourceResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::ClockSourceIndex(0u));
	CHECK_PAYLOAD_WITH_STATUS(SetClockSourceResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(50), la::avdecc::entity::model::ClockSourceIndex(50369));
}

TEST(AemPayloads, GetClockSourceCommand)
{
	CHECK_PAYLOAD(GetClockSourceCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(GetClockSourceCommand, la::avdecc::entity::model::DescriptorType::StreamInput, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, GetClockSourceResponse)
{
	CHECK_PAYLOAD_WITH_STATUS(GetClockSourceResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::ClockSourceIndex(0u));
	CHECK_PAYLOAD_WITH_STATUS(GetClockSourceResponse, la::avdecc::entity::model::DescriptorType::StreamOutput, la::avdecc::entity::model::DescriptorIndex(50), la::avdecc::entity::model::ClockSourceIndex(50369));
}

TEST(AemPayloads, StartStreamingCommand)
{
	CHECK_PAYLOAD(StartStreamingCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(StartStreamingCommand, la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, StartStreamingResponse)
{
	CHECK_PAYLOAD_WITH_STATUS(StartStreamingResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD_WITH_STATUS(StartStreamingResponse, la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, StopStreamingCommand)
{
	CHECK_PAYLOAD(StopStreamingCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(StopStreamingCommand, la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, StopStreamingResponse)
{
	CHECK_PAYLOAD_WITH_STATUS(StopStreamingResponse, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD_WITH_STATUS(StopStreamingResponse, la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, ReadDescriptorCommand)
{
	CHECK_PAYLOAD(ReadDescriptorCommand, la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(ReadDescriptorCommand, la::avdecc::entity::model::ConfigurationIndex(123), la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

#	pragma message("TODO: ReadDescriptorResponse tests")

TEST(AemPayloads, GetAvbInfoCommand)
{
	CHECK_PAYLOAD(GetAvbInfoCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(GetAvbInfoCommand, la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, GetAvbInfoResponse)
{
	la::avdecc::entity::model::AvbInfo avbInfo{ la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier(), std::uint32_t(5), std::uint8_t(2), la::avdecc::entity::AvbInfoFlags{ la::avdecc::entity::AvbInfoFlag::AsCapable }, la::avdecc::entity::model::MsrpMappings{} };
	EXPECT_NO_THROW(auto const ser = la::avdecc::protocol::aemPayload::serializeGetAvbInfoResponse(la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), avbInfo); EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemGetAvbInfoResponsePayloadMinSize, ser.size()); auto result = la::avdecc::protocol::aemPayload::deserializeGetAvbInfoResponse(la::avdecc::entity::LocalEntity::AemCommandStatus::Success, { ser.data(), ser.usedBytes() }); auto const& info = std::get<2>(result); EXPECT_EQ(avbInfo, info);) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, GetAsPathCommand)
{
	CHECK_PAYLOAD(GetAsPathCommand, la::avdecc::entity::model::DescriptorIndex(0));
	CHECK_PAYLOAD(GetAsPathCommand, la::avdecc::entity::model::DescriptorIndex(5));
}

TEST(AemPayloads, GetAsPathResponse)
{
	la::avdecc::entity::model::AsPath asPath{ la::avdecc::entity::model::PathSequence{} };
	EXPECT_NO_THROW(auto const ser = la::avdecc::protocol::aemPayload::serializeGetAsPathResponse(la::avdecc::entity::model::DescriptorIndex(0), asPath); EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemGetAsPathResponsePayloadMinSize, ser.size()); auto result = la::avdecc::protocol::aemPayload::deserializeGetAsPathResponse(la::avdecc::entity::LocalEntity::AemCommandStatus::Success, { ser.data(), ser.usedBytes() }); auto const& path = std::get<1>(result); EXPECT_EQ(asPath, path);) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, GetAudioMapCommand)
{
	CHECK_PAYLOAD(GetAudioMapCommand, la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::MapIndex(0));
	CHECK_PAYLOAD(GetAudioMapCommand, la::avdecc::entity::model::DescriptorType::Configuration, la::avdecc::entity::model::DescriptorIndex(5), la::avdecc::entity::model::MapIndex(11));
}

TEST(AemPayloads, GetAudioMapResponse)
{
	EXPECT_NO_THROW(auto const ser = la::avdecc::protocol::aemPayload::serializeGetAudioMapResponse(la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::MapIndex(0), la::avdecc::entity::model::MapIndex(0), la::avdecc::entity::model::AudioMappings{}); EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemGetAudioMapResponsePayloadMinSize, ser.size()); la::avdecc::protocol::aemPayload::deserializeGetAudioMapResponse(la::avdecc::entity::LocalEntity::AemCommandStatus::Success, { ser.data(), ser.usedBytes() });) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, AddAudioMappingsCommand)
{
	EXPECT_NO_THROW(auto const ser = la::avdecc::protocol::aemPayload::serializeAddAudioMappingsCommand(la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::AudioMappings{}); EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemAddAudioMappingsCommandPayloadMinSize, ser.size()); la::avdecc::protocol::aemPayload::deserializeAddAudioMappingsCommand({ ser.data(), ser.usedBytes() });) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, AddAudioMappingsResponse)
{
	EXPECT_NO_THROW(auto const ser = la::avdecc::protocol::aemPayload::serializeAddAudioMappingsResponse(la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::AudioMappings{}); EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemAddAudioMappingsResponsePayloadMinSize, ser.size()); la::avdecc::protocol::aemPayload::deserializeAddAudioMappingsResponse(la::avdecc::entity::LocalEntity::AemCommandStatus::Success, { ser.data(), ser.usedBytes() });) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, RemoveAudioMappingsCommand)
{
	EXPECT_NO_THROW(auto const ser = la::avdecc::protocol::aemPayload::serializeRemoveAudioMappingsCommand(la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::AudioMappings{}); EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemRemoveAudioMappingsCommandPayloadMinSize, ser.size()); la::avdecc::protocol::aemPayload::deserializeRemoveAudioMappingsCommand({ ser.data(), ser.usedBytes() });) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, RemoveAudioMappingsResponse)
{
	EXPECT_NO_THROW(auto const ser = la::avdecc::protocol::aemPayload::serializeRemoveAudioMappingsResponse(la::avdecc::entity::model::DescriptorType::Entity, la::avdecc::entity::model::DescriptorIndex(0), la::avdecc::entity::model::AudioMappings{}); EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemRemoveAudioMappingsResponsePayloadMinSize, ser.size()); la::avdecc::protocol::aemPayload::deserializeRemoveAudioMappingsResponse(la::avdecc::entity::LocalEntity::AemCommandStatus::Success, { ser.data(), ser.usedBytes() });) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, SetMemoryObjectLengthCommand)
{
	EXPECT_NO_THROW(auto const ser = la::avdecc::protocol::aemPayload::serializeSetMemoryObjectLengthCommand(la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::MemoryObjectIndex(0), std::uint64_t(0)); EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemSetMemoryObjectLengthCommandPayloadSize, ser.size()); la::avdecc::protocol::aemPayload::deserializeSetMemoryObjectLengthCommand({ ser.data(), ser.usedBytes() });) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, SetMemoryObjectLengthResponse)
{
	EXPECT_NO_THROW(auto const ser = la::avdecc::protocol::aemPayload::serializeSetMemoryObjectLengthResponse(la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::MemoryObjectIndex(0), std::uint64_t(0)); EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemSetMemoryObjectLengthResponsePayloadSize, ser.size()); la::avdecc::protocol::aemPayload::deserializeSetMemoryObjectLengthResponse(la::avdecc::entity::LocalEntity::AemCommandStatus::Success, { ser.data(), ser.usedBytes() });) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, GetMemoryObjectLengthCommand)
{
	EXPECT_NO_THROW(auto const ser = la::avdecc::protocol::aemPayload::serializeGetMemoryObjectLengthCommand(la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::MemoryObjectIndex(0)); EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemGetMemoryObjectLengthCommandPayloadSize, ser.size()); la::avdecc::protocol::aemPayload::deserializeGetMemoryObjectLengthCommand({ ser.data(), ser.usedBytes() });) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, GetMemoryObjectLengthResponse)
{
	EXPECT_NO_THROW(auto const ser = la::avdecc::protocol::aemPayload::serializeGetMemoryObjectLengthResponse(la::avdecc::entity::model::ConfigurationIndex(0), la::avdecc::entity::model::MemoryObjectIndex(0), std::uint64_t(0)); EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemGetMemoryObjectLengthResponsePayloadSize, ser.size()); la::avdecc::protocol::aemPayload::deserializeGetMemoryObjectLengthResponse(la::avdecc::entity::LocalEntity::AemCommandStatus::Success, { ser.data(), ser.usedBytes() });) << "Serialization/deserialization should not throw anything";
}

TEST(AemPayloads, StartOperationCommand)
{
	try
	{
		auto const ser = la::avdecc::protocol::aemPayload::serializeStartOperationCommand(la::avdecc::entity::model::DescriptorType::MemoryObject, la::avdecc::entity::model::MemoryObjectIndex(55), 10u, la::avdecc::entity::model::MemoryObjectOperationType::StoreAndReboot, la::avdecc::MemoryBuffer{});
		EXPECT_EQ(la::avdecc::protocol::aemPayload::AecpAemStartOperationCommandPayloadMinSize, ser.size());
		auto const [descriptorType, descriptorIndex, operationID, operationType, memoryBuffer] = la::avdecc::protocol::aemPayload::deserializeStartOperationCommand({ ser.data(), ser.usedBytes() });
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
		la::avdecc::MemoryBuffer const buffer{ std::vector<std::uint8_t>{ 1, 2, 3, 4 } };
		auto const ser = la::avdecc::protocol::aemPayload::serializeStartOperationCommand(la::avdecc::entity::model::DescriptorType::MemoryObject, la::avdecc::entity::model::MemoryObjectIndex(8), 60u, la::avdecc::entity::model::MemoryObjectOperationType::Upload, buffer);
		EXPECT_LT(la::avdecc::protocol::aemPayload::AecpAemStartOperationCommandPayloadMinSize, ser.size());
		auto const [descriptorType, descriptorIndex, operationID, operationType, memoryBuffer] = la::avdecc::protocol::aemPayload::deserializeStartOperationCommand({ ser.data(), ser.usedBytes() });
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
		auto const [descriptorType, descriptorIndex, operationID, operationType, memoryBuffer] = la::avdecc::protocol::aemPayload::deserializeStartOperationResponse(la::avdecc::entity::LocalEntity::AemCommandStatus::Success, { ser.data(), ser.usedBytes() });
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
		la::avdecc::MemoryBuffer const buffer{ std::vector<std::uint8_t>{ 1, 2, 3, 4 } };
		auto const ser = la::avdecc::protocol::aemPayload::serializeStartOperationResponse(la::avdecc::entity::model::DescriptorType::MemoryObject, la::avdecc::entity::model::MemoryObjectIndex(8), 60u, la::avdecc::entity::model::MemoryObjectOperationType::Upload, buffer);
		EXPECT_LT(la::avdecc::protocol::aemPayload::AecpAemStartOperationResponsePayloadMinSize, ser.size());
		auto const [descriptorType, descriptorIndex, operationID, operationType, memoryBuffer] = la::avdecc::protocol::aemPayload::deserializeStartOperationResponse(la::avdecc::entity::LocalEntity::AemCommandStatus::Success, { ser.data(), ser.usedBytes() });
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
		auto const [descriptorType, descriptorIndex, operationID] = la::avdecc::protocol::aemPayload::deserializeAbortOperationCommand({ ser.data(), ser.usedBytes() });
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
		auto const [descriptorType, descriptorIndex, operationID] = la::avdecc::protocol::aemPayload::deserializeAbortOperationResponse(la::avdecc::entity::LocalEntity::AemCommandStatus::Success, { ser.data(), ser.usedBytes() });
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
		auto const [descriptorType, descriptorIndex, operationID, percentComplete] = la::avdecc::protocol::aemPayload::deserializeOperationStatusResponse({ ser.data(), ser.usedBytes() });
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

TEST(AemPayloads, SendPayloadMaximumSize)
{
	la::avdecc::entity::model::AudioMappings mappings{};

	// More than 62 mappings do not fit in a single non-big aecp payload
	for (auto i = 0u; i < 64; ++i)
	{
		mappings.push_back({});
	}

#	if defined(ALLOW_SEND_BIG_AECP_PAYLOADS)
	EXPECT_NO_THROW(la::avdecc::protocol::aemPayload::serializeGetAudioMapResponse(la::avdecc::entity::model::DescriptorType::StreamPortInput, 0, 0, 0, mappings););
#	else // !ALLOW_SEND_BIG_AECP_PAYLOADS
	EXPECT_THROW(la::avdecc::protocol::aemPayload::serializeGetAudioMapResponse(la::avdecc::entity::model::DescriptorType::StreamPortInput, 0, 0, 0, mappings);, std::invalid_argument);
#	endif // ALLOW_SEND_BIG_AECP_PAYLOADS
}

TEST(AemPayloads, RecvPayloadMaximumSize)
{
	auto const serializeMappings = [](la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::MapIndex const numberOfMaps, auto const& mappings)
	{
		la::avdecc::Serializer<la::avdecc::protocol::AemAecpdu::MaximumRecvPayloadBufferLength> ser;
		std::uint16_t const reserved{ 0u };

		ser << descriptorType << descriptorIndex;
		ser << mapIndex << numberOfMaps << static_cast<std::uint16_t>(mappings.size()) << reserved;

		// Serialize variable data
		for (auto const& mapping : mappings)
		{
			ser << mapping.streamIndex << mapping.streamChannel << mapping.clusterOffset << mapping.clusterChannel;
		}

		return la::avdecc::protocol::AemAecpdu::Payload{ ser.data(), ser.usedBytes() };
	};

	la::avdecc::entity::model::AudioMappings mappings{};

	// More than 62 mappings do not fit in a single non-big aecp payload
	for (auto i = 0u; i < 64; ++i)
	{
		mappings.push_back({});
	}

	auto const payload = serializeMappings(la::avdecc::entity::model::DescriptorType::AudioCluster, 5u, 8u, 1u, mappings);

#	if defined(ALLOW_RECV_BIG_AECP_PAYLOADS)
	try
	{
		auto const [descriptorType, descriptorIndex, mapIndex, numberOfMaps, m] = la::avdecc::protocol::aemPayload::deserializeGetAudioMapResponse(la::avdecc::entity::LocalEntity::AemCommandStatus::Success, payload);
		EXPECT_EQ(la::avdecc::entity::model::DescriptorType::AudioCluster, descriptorType);
		EXPECT_EQ(5u, descriptorIndex);
		EXPECT_EQ(8u, mapIndex);
		EXPECT_EQ(1u, numberOfMaps);
		EXPECT_EQ(mappings.size(), m.size());
	}
	catch (...)
	{
		EXPECT_FALSE(true) << "Should not have thrown";
	}
#	else // !ALLOW_RECV_BIG_AECP_PAYLOADS
	EXPECT_THROW(la::avdecc::protocol::aemPayload::deserializeGetAudioMapResponse(payload);, std::invalid_argument);
#	endif // ALLOW_SEND_BIG_AECP_PAYLOADS
}

static inline std::tuple<la::avdecc::entity::model::ControlNodeStaticModel, la::avdecc::entity::model::ControlNodeDynamicModel> buildControlNodes(la::avdecc::entity::model::ControlDescriptor const& descriptor)
{
	// Copy static model
	auto s = la::avdecc::entity::model::ControlNodeStaticModel{};
	{
		s.localizedDescription = descriptor.localizedDescription;

		s.blockLatency = descriptor.blockLatency;
		s.controlLatency = descriptor.controlLatency;
		s.controlDomain = descriptor.controlDomain;
		s.controlType = descriptor.controlType;
		s.resetTime = descriptor.resetTime;
		s.signalType = descriptor.signalType;
		s.signalIndex = descriptor.signalIndex;
		s.signalOutput = descriptor.signalOutput;
		s.controlValueType = descriptor.controlValueType;
		s.values = descriptor.valuesStatic;
	}

	// Copy dynamic model
	auto d = la::avdecc::entity::model::ControlNodeDynamicModel{};
	{
		d.objectName = descriptor.objectName;
		d.values = descriptor.valuesDynamic;
	}

	return { s, d };
}

#	ifdef ENABLE_AVDECC_FEATURE_JSON
TEST(AemPayloads, DeserializeReadControlDescriptorResponse_LinearUInt8)
{
	auto ser = la::avdecc::Serializer<la::avdecc::protocol::AemAecpdu::MaximumPayloadBufferLength>{};
	ser << la::avdecc::entity::model::ConfigurationIndex{ 0u } << std::uint16_t{ 0u } << la::avdecc::entity::model::DescriptorType::Control << std::uint16_t{ 0u }; // We must put all the header in the buffer as deserializeReadControlDescriptorResponse will check for it
	ser << la::avdecc::entity::model::AvdeccFixedString{ "Test" };
	ser << la::avdecc::entity::model::LocalizedStringReference{};
	ser << std::uint32_t{ 1u } << std::uint32_t{ 2u } << std::uint16_t{ 3u }; // Dummy value
	ser << la::avdecc::entity::model::ControlValueType::Type::ControlLinearUInt8;
	ser << la::avdecc::UniqueIdentifier{ 0x90e0f00000000001 };
	ser << std::uint32_t{ 4u }; // Dummy value
	ser << std::uint16_t{ 104u }; // Values offset
	ser << std::uint16_t{ 1u }; // Number of values
	ser << la::avdecc::entity::model::DescriptorType::Invalid << la::avdecc::entity::model::DescriptorIndex{ 0u } << std::uint16_t{ 0u };
	// Actual Control Values
	ser << la::avdecc::MemoryBuffer{ std::vector<std::uint8_t>{ 0, 255, 255, 0, 0, 0, 0, 255, 255 } };

	auto const payload = la::avdecc::protocol::AemAecpdu::Payload{ ser.data(), ser.usedBytes() };
	auto descriptor = la::avdecc::entity::model::ControlDescriptor{};
	ASSERT_NO_THROW(descriptor = la::avdecc::protocol::aemPayload::deserializeReadControlDescriptorResponse(payload, 8u, static_cast<la::avdecc::protocol::AemAecpStatus>(la::avdecc::protocol::AemAecpStatus::Success)););
	ASSERT_EQ(la::avdecc::entity::model::ControlValuesValidationResult::Valid, std::get<0>(la::avdecc::entity::model::validateControlValues(descriptor.valuesStatic, descriptor.valuesDynamic)));

	try
	{
		auto const [s, d] = buildControlNodes(descriptor);
		auto js = nlohmann::json{};
		js = s;
		auto const ss = js.get<decltype(s)>();
		EXPECT_TRUE(s.values.isEqualTo<la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueStatic<std::uint8_t>>>(ss.values));
		auto jd = nlohmann::json{};
		jd = d;
		auto const dd = jd.get<decltype(d)>();
		EXPECT_TRUE(d.values.isEqualTo<la::avdecc::entity::model::LinearValues<la::avdecc::entity::model::LinearValueDynamic<std::uint8_t>>>(dd.values));
	}
	catch (...)
	{
		ASSERT_TRUE(false) << "Should not throw";
	}
}

TEST(AemPayloads, DeserializeReadControlDescriptorResponse_ArrayInt8)
{
	auto ser = la::avdecc::Serializer<la::avdecc::protocol::AemAecpdu::MaximumPayloadBufferLength>{};
	ser << la::avdecc::entity::model::ConfigurationIndex{ 0u } << std::uint16_t{ 0u } << la::avdecc::entity::model::DescriptorType::Control << std::uint16_t{ 0u }; // We must put all the header in the buffer as deserializeReadControlDescriptorResponse will check for it
	ser << la::avdecc::entity::model::AvdeccFixedString{ "Test" };
	ser << la::avdecc::entity::model::LocalizedStringReference{};
	ser << std::uint32_t{ 1u } << std::uint32_t{ 2u } << std::uint16_t{ 3u }; // Dummy value
	ser << la::avdecc::entity::model::ControlValueType::Type::ControlArrayInt8;
	ser << la::avdecc::UniqueIdentifier{ 0x001cab0000100031 };
	ser << std::uint32_t{ 4u }; // Dummy value
	ser << std::uint16_t{ 104u }; // Values offset
	ser << std::uint16_t{ 2u }; // Number of values
	ser << la::avdecc::entity::model::DescriptorType::Invalid << la::avdecc::entity::model::DescriptorIndex{ 0u } << std::uint16_t{ 0u };
	// Actual Control Values
	ser << la::avdecc::MemoryBuffer{ std::vector<std::uint8_t>{ 0x00, 0x7f, 0x01, 0x00, 0x00, 0x00, 0x1f, 0xff, 0x07, 0x08 } };

	auto const payload = la::avdecc::protocol::AemAecpdu::Payload{ ser.data(), ser.usedBytes() };
	auto descriptor = la::avdecc::entity::model::ControlDescriptor{};
	ASSERT_NO_THROW(descriptor = la::avdecc::protocol::aemPayload::deserializeReadControlDescriptorResponse(payload, 8u, static_cast<la::avdecc::protocol::AemAecpStatus>(la::avdecc::protocol::AemAecpStatus::Success)););
	ASSERT_EQ(la::avdecc::entity::model::ControlValuesValidationResult::Valid, std::get<0>(la::avdecc::entity::model::validateControlValues(descriptor.valuesStatic, descriptor.valuesDynamic)));

	try
	{
		auto const [s, d] = buildControlNodes(descriptor);
		auto js = nlohmann::json{};
		js = s;
		auto const ss = js.get<decltype(s)>();
		EXPECT_TRUE(s.values.isEqualTo<la::avdecc::entity::model::ArrayValueStatic<std::int8_t>>(ss.values));
		auto jd = nlohmann::json{};
		jd = d;
		auto const dd = jd.get<decltype(d)>();
		EXPECT_TRUE(d.values.isEqualTo<la::avdecc::entity::model::ArrayValueDynamic<std::int8_t>>(dd.values));
	}
	catch (...)
	{
		ASSERT_TRUE(false) << "Should not throw";
	}
}

TEST(AemPayloads, DeserializeReadControlDescriptorResponse_ArrayUInt32)
{
	auto ser = la::avdecc::Serializer<la::avdecc::protocol::AemAecpdu::MaximumPayloadBufferLength>{};
	ser << la::avdecc::entity::model::ConfigurationIndex{ 0u } << std::uint16_t{ 0u } << la::avdecc::entity::model::DescriptorType::Control << std::uint16_t{ 0u }; // We must put all the header in the buffer as deserializeReadControlDescriptorResponse will check for it
	ser << la::avdecc::entity::model::AvdeccFixedString{ "Test" };
	ser << la::avdecc::entity::model::LocalizedStringReference{};
	ser << std::uint32_t{ 1u } << std::uint32_t{ 2u } << std::uint16_t{ 3u }; // Dummy value
	ser << la::avdecc::entity::model::ControlValueType::Type::ControlArrayUInt32;
	ser << la::avdecc::UniqueIdentifier{ 0x001cab0000100031 };
	ser << std::uint32_t{ 4u }; // Dummy value
	ser << std::uint16_t{ 104u }; // Values offset
	ser << std::uint16_t{ 2u }; // Number of values
	ser << la::avdecc::entity::model::DescriptorType::Invalid << la::avdecc::entity::model::DescriptorIndex{ 0u } << std::uint16_t{ 0u };
	// Actual Control Values
	ser << la::avdecc::MemoryBuffer{ std::vector<std::uint8_t>{ 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x08 } };

	auto const payload = la::avdecc::protocol::AemAecpdu::Payload{ ser.data(), ser.usedBytes() };
	auto descriptor = la::avdecc::entity::model::ControlDescriptor{};
	ASSERT_NO_THROW(descriptor = la::avdecc::protocol::aemPayload::deserializeReadControlDescriptorResponse(payload, 8u, static_cast<la::avdecc::protocol::AemAecpStatus>(la::avdecc::protocol::AemAecpStatus::Success)););
	ASSERT_EQ(la::avdecc::entity::model::ControlValuesValidationResult::Valid, std::get<0>(la::avdecc::entity::model::validateControlValues(descriptor.valuesStatic, descriptor.valuesDynamic)));

	try
	{
		auto const [s, d] = buildControlNodes(descriptor);
		auto js = nlohmann::json{};
		js = s;
		auto const ss = js.get<decltype(s)>();
		EXPECT_TRUE(s.values.isEqualTo<la::avdecc::entity::model::ArrayValueStatic<std::uint32_t>>(ss.values));
		auto jd = nlohmann::json{};
		jd = d;
		auto const dd = jd.get<decltype(d)>();
		EXPECT_TRUE(d.values.isEqualTo<la::avdecc::entity::model::ArrayValueDynamic<std::uint32_t>>(dd.values));
	}
	catch (...)
	{
		ASSERT_TRUE(false) << "Should not throw";
	}
}

#pragma message("TODO: Test each possible value returned by validateControlValues. Need an easy way to create ControlValues")
TEST(AemPayloads, DeserializeReadControlDescriptorResponse_Utf8)
{
	auto ser = la::avdecc::Serializer<la::avdecc::protocol::AemAecpdu::MaximumPayloadBufferLength>{};
	ser << la::avdecc::entity::model::ConfigurationIndex{ 0u } << std::uint16_t{ 0u } << la::avdecc::entity::model::DescriptorType::Control << std::uint16_t{ 0u }; // We must put all the header in the buffer as deserializeReadControlDescriptorResponse will check for it
	ser << la::avdecc::entity::model::AvdeccFixedString{ "Test" };
	ser << la::avdecc::entity::model::LocalizedStringReference{};
	ser << std::uint32_t{ 1u } << std::uint32_t{ 2u } << std::uint16_t{ 3u }; // Dummy value
	ser << la::avdecc::entity::model::ControlValueType::Type::ControlUtf8;
	ser << la::avdecc::UniqueIdentifier{ 0x001cab0000100031 };
	ser << std::uint32_t{ 4u }; // Dummy value
	ser << std::uint16_t{ 104u }; // Values offset
	ser << std::uint16_t{ 1u }; // Number of values
	ser << la::avdecc::entity::model::DescriptorType::Invalid << la::avdecc::entity::model::DescriptorIndex{ 0u } << std::uint16_t{ 0u };
	// Actual Control Values
	ser << la::avdecc::MemoryBuffer{ std::vector<std::uint8_t>{ 0x54, 0x65, 0x73, 0x74, 0 } };

	auto const payload = la::avdecc::protocol::AemAecpdu::Payload{ ser.data(), ser.usedBytes() };
	auto descriptor = la::avdecc::entity::model::ControlDescriptor{};
	ASSERT_NO_THROW(descriptor = la::avdecc::protocol::aemPayload::deserializeReadControlDescriptorResponse(payload, 8u, static_cast<la::avdecc::protocol::AemAecpStatus>(la::avdecc::protocol::AemAecpStatus::Success)););
	ASSERT_EQ(la::avdecc::entity::model::ControlValuesValidationResult::Valid, std::get<0>(la::avdecc::entity::model::validateControlValues(descriptor.valuesStatic, descriptor.valuesDynamic)));

	try
	{
		auto const [s, d] = buildControlNodes(descriptor);
		auto js = nlohmann::json{};
		js = s;
		auto const ss = js.get<decltype(s)>();
		EXPECT_TRUE(s.values.isEqualTo<la::avdecc::entity::model::UTF8StringValueStatic>(ss.values));
		auto jd = nlohmann::json{};
		jd = d;
		auto const dd = jd.get<decltype(d)>();
		EXPECT_TRUE(d.values.isEqualTo<la::avdecc::entity::model::UTF8StringValueDynamic>(dd.values));
	}
	catch (...)
	{
		ASSERT_TRUE(false) << "Should not throw";
	}
}
#	endif // ENABLE_AVDECC_FEATURE_JSON

#endif // _WIN32 || __APPLE__
