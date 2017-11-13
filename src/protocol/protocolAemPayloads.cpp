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
* @file protocolAemPayloads.cpp
* @author Christophe Calmejane
*/

#include "protocolAemPayloads.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace aemPayload
{

/** ACQUIRE_ENTITY Command - Clause 7.4.1.1 */
Serializer<AecpAemAcquireEntityCommandPayloadSize> serializeAcquireEntityCommand(AemAcquireEntityFlags const flags, UniqueIdentifier const ownerID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemAcquireEntityCommandPayloadSize> ser;

	ser << flags;
	ser << ownerID;
	ser << descriptorType << descriptorIndex;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<AemAcquireEntityFlags, UniqueIdentifier, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeAcquireEntityCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemAcquireEntityCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	AemAcquireEntityFlags flags{ AemAcquireEntityFlags::None };
	UniqueIdentifier ownerID{ getUninitializedIdentifier() };
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> flags;
	des >> ownerID;
	des >> descriptorType >> descriptorIndex;

	assert(des.usedBytes() == AecpAemAcquireEntityCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(flags, ownerID, descriptorType, descriptorIndex);
}

/** ACQUIRE_ENTITY Response - Clause 7.4.1.1 */
Serializer<AecpAemAcquireEntityResponsePayloadSize> serializeAcquireEntityResponse(AemAcquireEntityFlags const flags, UniqueIdentifier ownerID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	// Same as ACQUIRE_ENTITY Command
	static_assert(AecpAemAcquireEntityResponsePayloadSize == AecpAemAcquireEntityCommandPayloadSize, "ACQUIRE_ENTITY Response no longer the same as ACQUIRE_ENTITY Command");
	return serializeAcquireEntityCommand(flags, ownerID, descriptorType, descriptorIndex);
}

std::tuple<AemAcquireEntityFlags, UniqueIdentifier, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeAcquireEntityResponse(AemAecpdu::Payload const& payload)
{
	// Same as ACQUIRE_ENTITY Command
	static_assert(AecpAemAcquireEntityResponsePayloadSize == AecpAemAcquireEntityCommandPayloadSize, "ACQUIRE_ENTITY Response no longer the same as ACQUIRE_ENTITY Command");
	return deserializeAcquireEntityCommand(payload);
}

/** LOCK_ENTITY Command - Clause 7.4.2.1 */
Serializer<AecpAemLockEntityCommandPayloadSize> serializeLockEntityCommand(AemLockEntityFlags flags, UniqueIdentifier lockedID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemLockEntityCommandPayloadSize> ser;

	ser << flags;
	ser << lockedID;
	ser << descriptorType << descriptorIndex;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<AemLockEntityFlags, UniqueIdentifier, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeLockEntityCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemLockEntityCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	AemLockEntityFlags flags{ AemLockEntityFlags::None };
	UniqueIdentifier lockedID{ getUninitializedIdentifier() };
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> flags;
	des >> lockedID;
	des >> descriptorType >> descriptorIndex;

	assert(des.usedBytes() == AecpAemLockEntityCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(flags, lockedID, descriptorType, descriptorIndex);
}

/** LOCK_ENTITY Response - Clause 7.4.2.1 */
Serializer<AecpAemLockEntityResponsePayloadSize> serializeLockEntityResponse(AemLockEntityFlags flags, UniqueIdentifier lockedID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	// Same as LOCK_ENTITY Command
	static_assert(AecpAemLockEntityResponsePayloadSize == AecpAemLockEntityCommandPayloadSize, "LOCK_ENTITY Response no longer the same as LOCK_ENTITY Command");
	return serializeLockEntityCommand(flags, lockedID, descriptorType, descriptorIndex);
}

std::tuple<AemLockEntityFlags, UniqueIdentifier, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeLockEntityResponse(AemAecpdu::Payload const& payload)
{
	// Same as LOCK_ENTITY Command
	static_assert(AecpAemLockEntityResponsePayloadSize == AecpAemLockEntityCommandPayloadSize, "LOCK_ENTITY Response no longer the same as LOCK_ENTITY Command");
	return deserializeLockEntityCommand(payload);
}

/** READ_DESCRIPTOR Command - Clause 7.4.5.1 */

/** READ_DESCRIPTOR Response - Clause 7.4.5.2 */

/** WRITE_DESCRIPTOR Command - Clause 7.4.6.1 */

/** WRITE_DESCRIPTOR Response - Clause 7.4.6.1 */

/** ENTITY_AVAILABLE Command - Clause 7.4.3.1 */
// No payload

/** ENTITY_AVAILABLE Response - Clause 7.4.3.1 */
// No payload

/** CONTROLLER_AVAILABLE Command - Clause 7.4.4.1 */
// No payload

/** CONTROLLER_AVAILABLE Response - Clause 7.4.4.1 */
// No payload

/** SET_CONFIGURATION Command - Clause 7.4.7.1 */
Serializer<AecpAemSetConfigurationCommandPayloadSize> serializeSetConfigurationCommand(entity::model::ConfigurationIndex const configurationIndex)
{
	Serializer<AecpAemSetConfigurationCommandPayloadSize> ser;
	std::uint16_t const reserved{ 0u };

	ser << reserved << configurationIndex;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::ConfigurationIndex> deserializeSetConfigurationCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemSetConfigurationCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	std::uint16_t reserved{ 0u };
	entity::model::ConfigurationIndex configurationIndex{ 0u };

	des >> reserved >> configurationIndex;

	assert(des.usedBytes() == AecpAemSetConfigurationCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(configurationIndex);
}

/** SET_CONFIGURATION Response - Clause 7.4.7.1 */
Serializer<AecpAemSetConfigurationResponsePayloadSize> serializeSetConfigurationResponse(entity::model::ConfigurationIndex const configurationIndex)
{
	// Same as SET_CONFIGURATION Command
	static_assert(AecpAemSetConfigurationResponsePayloadSize == AecpAemSetConfigurationCommandPayloadSize, "SET_CONFIGURATION Response no longer the same as SET_CONFIGURATION Command");
	return serializeSetConfigurationCommand(configurationIndex);
}

std::tuple<entity::model::ConfigurationIndex> deserializeSetConfigurationResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_CONFIGURATION Command
	static_assert(AecpAemSetConfigurationResponsePayloadSize == AecpAemSetConfigurationCommandPayloadSize, "SET_CONFIGURATION Response no longer the same as SET_CONFIGURATION Command");
	return deserializeSetConfigurationCommand(payload);
}

/** GET_CONFIGURATION Command - Clause 7.4.8.1 */
// No payload

/** GET_CONFIGURATION Response - Clause 7.4.8.2 */
Serializer<AecpAemGetConfigurationResponsePayloadSize> serializeGetConfigurationResponse(entity::model::ConfigurationIndex const configurationIndex)
{
	// Same as SET_CONFIGURATION Command
	static_assert(AecpAemGetConfigurationResponsePayloadSize == AecpAemSetConfigurationCommandPayloadSize, "GET_CONFIGURATION Response no longer the same as SET_CONFIGURATION Command");
	return serializeSetConfigurationCommand(configurationIndex);
}

std::tuple<entity::model::ConfigurationIndex> deserializeGetConfigurationResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_CONFIGURATION Command
	static_assert(AecpAemGetConfigurationResponsePayloadSize == AecpAemSetConfigurationCommandPayloadSize, "GET_CONFIGURATION Response no longer the same as SET_CONFIGURATION Command");
	return deserializeSetConfigurationCommand(payload);
}

/** SET_STREAM_FORMAT Command - Clause 7.4.9.1 */
Serializer<AecpAemSetStreamFormatCommandPayloadSize> serializeSetStreamFormatCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamFormat const streamFormat)
{
	Serializer<AecpAemSetStreamFormatCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;
	ser << streamFormat;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamFormat> deserializeSetStreamFormatCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemSetStreamFormatCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::StreamFormat streamFormat{ entity::model::getNullStreamFormat() };

	des >> descriptorType >> descriptorIndex;
	des >> streamFormat;

	assert(des.usedBytes() == AecpAemSetStreamFormatCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, streamFormat);
}

/** SET_STREAM_FORMAT Response - Clause 7.4.9.1 */
Serializer<AecpAemSetStreamFormatResponsePayloadSize> serializeSetStreamFormatResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamFormat const streamFormat)
{
	// Same as SET_STREAM_FORMAT Command
	static_assert(AecpAemSetStreamFormatResponsePayloadSize == AecpAemSetStreamFormatCommandPayloadSize, "SET_STREAM_FORMAT Response no longer the same as SET_STREAM_FORMAT Command");
	return serializeSetStreamFormatCommand(descriptorType, descriptorIndex, streamFormat);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamFormat> deserializeSetStreamFormatResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_STREAM_FORMAT Command
	static_assert(AecpAemSetStreamFormatResponsePayloadSize == AecpAemSetStreamFormatCommandPayloadSize, "SET_STREAM_FORMAT Response no longer the same as SET_STREAM_FORMAT Command");
	return deserializeSetStreamFormatCommand(payload);
}

/** GET_STREAM_FORMAT Command - Clause 7.4.10.1 */
Serializer<AecpAemGetStreamFormatCommandPayloadSize> serializeGetStreamFormatCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemGetStreamFormatCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeGetStreamFormatCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetStreamFormatCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> descriptorType >> descriptorIndex;

	assert(des.usedBytes() == AecpAemGetStreamFormatCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}

/** GET_STREAM_FORMAT Response - Clause 7.4.10.2 */
Serializer<AecpAemGetStreamFormatResponsePayloadSize> serializeGetStreamFormatResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamFormat const streamFormat)
{
	// Same as SET_STREAM_FORMAT Command
	static_assert(AecpAemGetStreamFormatResponsePayloadSize == AecpAemSetStreamFormatCommandPayloadSize, "GET_STREAM_FORMAT Response no longer the same as SET_STREAM_FORMAT Command");
	return serializeSetStreamFormatCommand(descriptorType, descriptorIndex, streamFormat);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamFormat> deserializeGetStreamFormatResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_STREAM_FORMAT Command
	static_assert(AecpAemGetStreamFormatResponsePayloadSize == AecpAemSetStreamFormatCommandPayloadSize, "GET_STREAM_FORMAT Response no longer the same as SET_STREAM_FORMAT Command");
	return deserializeSetStreamFormatCommand(payload);
}

/** SET_STREAM_INFO Command - Clause 7.4.15.1 */
Serializer<AecpAemSetStreamInfoCommandPayloadSize> serializeSetStreamInfoCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamInfo const& streamInfo)
{
	Serializer<AecpAemSetStreamInfoCommandPayloadSize> ser;
	std::uint8_t const reserved{ 0u };
	std::uint16_t const reserved2{ 0u };

	ser << descriptorType << descriptorIndex;
	ser << streamInfo.streamInfoFlags;
	ser << streamInfo.streamFormat;
	ser << streamInfo.streamID;
	ser << streamInfo.msrpAccumulatedLatency;
	ser.packBuffer(streamInfo.streamDestMac.data(), streamInfo.streamDestMac.size());
	ser << streamInfo.msrpFailureCode;
	ser << reserved;
	ser << streamInfo.msrpFailureBridgeID;
	ser << streamInfo.streamVlanID;
	ser << reserved2;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamInfo> deserializeSetStreamInfoCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemSetStreamInfoCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::StreamInfo streamInfo{};
	std::uint8_t reserved{ 0u };
	std::uint16_t reserved2{ 0u };

	des >> descriptorType >> descriptorIndex;
	des >> streamInfo.streamInfoFlags;
	des >> streamInfo.streamFormat;
	des >> streamInfo.streamID;
	des >> streamInfo.msrpAccumulatedLatency;
	des.unpackBuffer(streamInfo.streamDestMac.data(), streamInfo.streamDestMac.size());
	des >> streamInfo.msrpFailureCode;
	des >> reserved;
	des >> streamInfo.msrpFailureBridgeID;
	des >> streamInfo.streamVlanID;
	des >> reserved2;

	assert(des.usedBytes() == AecpAemSetStreamInfoCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, streamInfo);
}

/** SET_STREAM_INFO Response - Clause 7.4.15.1 */
Serializer<AecpAemSetStreamInfoResponsePayloadSize> serializeSetStreamInfoResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamInfo const& streamInfo)
{
	// Same as SET_STREAM_INFO Command
	static_assert(AecpAemSetStreamInfoResponsePayloadSize == AecpAemSetStreamInfoCommandPayloadSize, "SET_STREAM_INFO Response no longer the same as SET_STREAM_INFO Command");
	return serializeSetStreamInfoCommand(descriptorType, descriptorIndex, streamInfo);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamInfo> deserializeSetStreamInfoResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_STREAM_INFO Command
	static_assert(AecpAemSetStreamInfoResponsePayloadSize == AecpAemSetStreamInfoCommandPayloadSize, "SET_STREAM_INFO Response no longer the same as SET_STREAM_INFO Command");
	return deserializeSetStreamInfoCommand(payload);
}

/** GET_STREAM_INFO Command - Clause 7.4.16.1 */
Serializer<AecpAemGetStreamInfoCommandPayloadSize> serializeGetStreamInfoCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemGetStreamInfoCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeGetStreamInfoCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetStreamInfoCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> descriptorType >> descriptorIndex;

	assert(des.usedBytes() == AecpAemGetStreamInfoCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}


/** GET_STREAM_INFO Response - Clause 7.4.16.2 */
Serializer<AecpAemGetStreamInfoResponsePayloadSize> serializeGetStreamInfoResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamInfo const& streamInfo)
{
	// Same as SET_STREAM_INFO Command
	static_assert(AecpAemGetStreamInfoResponsePayloadSize == AecpAemSetStreamInfoCommandPayloadSize, "GET_STREAM_INFO Response no longer the same as SET_STREAM_INFO Command");
	return serializeSetStreamInfoCommand(descriptorType, descriptorIndex, streamInfo);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamInfo> deserializeGetStreamInfoResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_STREAM_INFO Command
	static_assert(AecpAemGetStreamInfoResponsePayloadSize == AecpAemSetStreamInfoCommandPayloadSize, "GET_STREAM_INFO Response no longer the same as SET_STREAM_INFO Command");
	return deserializeSetStreamInfoCommand(payload);
}

/** GET_AUDIO_MAP Command  - Clause 7.4.44.1 */

/** GET_AUDIO_MAP Response  - Clause 7.4.44.2 */

/** ADD_AUDIO_MAPPINGS Command  - Clause 7.4.45.1 */

/** ADD_AUDIO_MAPPINGS Response  - Clause 7.4.45.1 */

/** REMOVE_AUDIO_MAPPINGS Command  - Clause 7.4.46.1 */

/** REMOVE_AUDIO_MAPPINGS Response  - Clause 7.4.46.1 */

/** GET_STREAM_INFO Command - Clause 7.4.16.1 */

/** GET_STREAM_INFO Response - Clause 7.4.16.2 */

/** SET_NAME Command - Clause 7.4.17.1 */
Serializer<AecpAemSetNameCommandPayloadSize> serializeSetNameCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name)
{
	Serializer<AecpAemSetNameCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;
	ser << nameIndex << configurationIndex;
	ser << name;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex, entity::model::AvdeccFixedString> deserializeSetNameCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemSetNameCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType;
	entity::model::DescriptorIndex descriptorIndex;
	std::uint16_t nameIndex;
	entity::model::ConfigurationIndex configurationIndex;
	entity::model::AvdeccFixedString name;

	des >> descriptorType >> descriptorIndex;
	des >> nameIndex >> configurationIndex >> name;

	assert(des.usedBytes() == AecpAemSetNameCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, nameIndex, configurationIndex, name);
}

/** SET_NAME Response - Clause 7.4.17.1 */
Serializer<AecpAemSetNameResponsePayloadSize> serializeSetNameResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name)
{
	// Same as SET_NAME Command
	static_assert(AecpAemSetNameResponsePayloadSize == AecpAemSetNameCommandPayloadSize, "SET_NAME Response no longer the same as SET_NAME Command");
	return serializeSetNameCommand(descriptorType, descriptorIndex, nameIndex, configurationIndex, name);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex, entity::model::AvdeccFixedString> deserializeSetNameResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_NAME Command
	static_assert(AecpAemSetNameResponsePayloadSize == AecpAemSetNameCommandPayloadSize, "SET_NAME Response no longer the same as SET_NAME Command");
	return deserializeSetNameCommand(payload);
}

/** GET_NAME Command - Clause 7.4.18.1 */
Serializer<AecpAemGetNameCommandPayloadSize> serializeGetNameCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex)
{
	Serializer<AecpAemGetNameCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;
	ser << nameIndex << configurationIndex;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex> deserializeGetNameCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetNameCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType;
	entity::model::DescriptorIndex descriptorIndex;
	std::uint16_t nameIndex;
	entity::model::ConfigurationIndex configurationIndex;

	des >> descriptorType >> descriptorIndex;
	des >> nameIndex >> configurationIndex;

	assert(des.usedBytes() == AecpAemGetNameCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, nameIndex, configurationIndex);
}

/** GET_NAME Response - Clause 7.4.18.2 */
Serializer<AecpAemGetNameResponsePayloadSize> serializeGetNameResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name)
{
	// Same as SET_NAME Command
	static_assert(AecpAemGetNameResponsePayloadSize == AecpAemSetNameCommandPayloadSize, "GET_NAME Response no longer the same as SET_NAME Command");
	return serializeSetNameCommand(descriptorType, descriptorIndex, nameIndex, configurationIndex, name);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex, entity::model::AvdeccFixedString> deserializeGetNameResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_NAME Command
	static_assert(AecpAemGetNameResponsePayloadSize == AecpAemSetNameCommandPayloadSize, "GET_NAME Response no longer the same as SET_NAME Command");
	return deserializeSetNameCommand(payload);
}

/** SET_SAMPLING_RATE Command - Clause 7.4.21.1 */
Serializer<AecpAemSetSamplingRateCommandPayloadSize> serializeSetSamplingRateCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::SamplingRate const samplingRate)
{
	Serializer<AecpAemSetSamplingRateCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;
	ser << samplingRate;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::SamplingRate> deserializeSetSamplingRateCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemSetSamplingRateCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::SamplingRate samplingRate{ entity::model::getNullSamplingRate() };

	des >> descriptorType >> descriptorIndex;
	des >> samplingRate;

	assert(des.usedBytes() == AecpAemSetSamplingRateCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, samplingRate);
}

/** SET_SAMPLING_RATE Response - Clause 7.4.21.1 */
Serializer<AecpAemSetSamplingRateResponsePayloadSize> serializeSetSamplingRateResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::SamplingRate const samplingRate)
{
	// Same as SET_SAMPLING_RATE Command
	static_assert(AecpAemSetSamplingRateResponsePayloadSize == AecpAemSetSamplingRateCommandPayloadSize, "SET_SAMPLING_RATE Response no longer the same as SET_SAMPLING_RATE Command");
	return serializeSetSamplingRateCommand(descriptorType, descriptorIndex, samplingRate);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::SamplingRate> deserializeSetSamplingRateResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_SAMPLING_RATE Command
	static_assert(AecpAemSetSamplingRateResponsePayloadSize == AecpAemSetSamplingRateCommandPayloadSize, "SET_SAMPLING_RATE Response no longer the same as SET_SAMPLING_RATE Command");
	return deserializeSetSamplingRateCommand(payload);
}

/** GET_SAMPLING_RATE Command - Clause 7.4.22.1 */
Serializer<AecpAemGetSamplingRateCommandPayloadSize> serializeGetSamplingRateCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemGetSamplingRateCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeGetSamplingRateCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetSamplingRateCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> descriptorType >> descriptorIndex;

	assert(des.usedBytes() == AecpAemGetSamplingRateCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}

/** GET_SAMPLING_RATE Response - Clause 7.4.22.2 */
Serializer<AecpAemGetSamplingRateResponsePayloadSize> serializeGetSamplingRateResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::SamplingRate const samplingRate)
{
	// Same as SET_SAMPLING_RATE Command
	static_assert(AecpAemGetSamplingRateResponsePayloadSize == AecpAemSetSamplingRateCommandPayloadSize, "GET_SAMPLING_RATE Response no longer the same as SET_SAMPLING_RATE Command");
	return serializeSetSamplingRateCommand(descriptorType, descriptorIndex, samplingRate);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::SamplingRate> deserializeGetSamplingRateResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_SAMPLING_RATE Command
	static_assert(AecpAemGetSamplingRateResponsePayloadSize == AecpAemSetSamplingRateCommandPayloadSize, "GET_SAMPLING_RATE Response no longer the same as SET_SAMPLING_RATE Command");
	return deserializeSetSamplingRateCommand(payload);
}

} // namespace aemPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
