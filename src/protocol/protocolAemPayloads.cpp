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
Serializer<AecpAemAcquireEntityCommandPayloadSize> serializeAcquireEntityCommand(protocol::AemAcquireEntityFlags const flags, UniqueIdentifier const ownerID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemAcquireEntityCommandPayloadSize> ser;

	ser << flags;
	ser << ownerID;
	ser << descriptorType << descriptorIndex;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<protocol::AemAcquireEntityFlags, UniqueIdentifier, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeAcquireEntityCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemAcquireEntityCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	protocol::AemAcquireEntityFlags flags{ protocol::AemAcquireEntityFlags::None };
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
Serializer<AecpAemAcquireEntityResponsePayloadSize> serializeAcquireEntityResponse(protocol::AemAcquireEntityFlags const flags, UniqueIdentifier ownerID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	// Same as ACQUIRE_ENTITY Command
	static_assert(AecpAemAcquireEntityResponsePayloadSize == AecpAemAcquireEntityCommandPayloadSize, "ACQUIRE_ENTITY Response no longer the same as ACQUIRE_ENTITY Command");
	return serializeAcquireEntityCommand(flags, ownerID, descriptorType, descriptorIndex);
}

std::tuple<protocol::AemAcquireEntityFlags, UniqueIdentifier, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeAcquireEntityResponse(AemAecpdu::Payload const& payload)
{
	// Same as ACQUIRE_ENTITY Command
	static_assert(AecpAemAcquireEntityResponsePayloadSize == AecpAemAcquireEntityCommandPayloadSize, "ACQUIRE_ENTITY Response no longer the same as ACQUIRE_ENTITY Command");
	return deserializeAcquireEntityCommand(payload);
}

/** LOCK_ENTITY Command - Clause 7.4.2.1 */
Serializer<AecpAemLockEntityCommandPayloadSize> serializeLockEntityCommand(protocol::AemLockEntityFlags flags, UniqueIdentifier lockedID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemLockEntityCommandPayloadSize> ser;

	ser << flags;
	ser << lockedID;
	ser << descriptorType << descriptorIndex;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<protocol::AemLockEntityFlags, UniqueIdentifier, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeLockEntityCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemLockEntityCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	protocol::AemLockEntityFlags flags{ protocol::AemLockEntityFlags::None };
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
Serializer<AecpAemLockEntityResponsePayloadSize> serializeLockEntityResponse(protocol::AemLockEntityFlags flags, UniqueIdentifier lockedID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	// Same as LOCK_ENTITY Command
	static_assert(AecpAemLockEntityResponsePayloadSize == AecpAemLockEntityCommandPayloadSize, "LOCK_ENTITY Response no longer the same as LOCK_ENTITY Command");
	return serializeLockEntityCommand(flags, lockedID, descriptorType, descriptorIndex);
}

std::tuple<protocol::AemLockEntityFlags, UniqueIdentifier, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeLockEntityResponse(AemAecpdu::Payload const& payload)
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
	std::uint16_t reserved{ 0u };
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

/** SET_STREAM_FORMAT Response - Clause 7.4.9.1 */

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

} // namespace aemPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
