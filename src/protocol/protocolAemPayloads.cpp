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
#include "la/avdecc/logger.hpp"

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
Serializer<AecpAemReadDescriptorCommandPayloadSize> serializeReadDescriptorCommand(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemReadDescriptorCommandPayloadSize> ser;
	std::uint16_t const reserved{ 0u };

	ser << configurationIndex << reserved;
	ser << descriptorType << descriptorIndex;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::ConfigurationIndex, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeReadDescriptorCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemReadDescriptorCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::ConfigurationIndex configurationIndex{ 0u };
	std::uint16_t reserved{ 0u };
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> configurationIndex >> reserved;
	des >> descriptorType >> descriptorIndex;

	assert(des.usedBytes() == AecpAemReadDescriptorCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(configurationIndex, descriptorType, descriptorIndex);
}

/** READ_DESCRIPTOR Response - Clause 7.4.5.2 */
//Serializer<AemAecpdu::MaximumPayloadLength> serializeReadDescriptorResponse(entity::model::ConfigurationIndex const /*configurationIndex*/, entity::model::DescriptorType const /*descriptorType*/, entity::model::DescriptorIndex const /*descriptorIndex*/)
//{
//	return {};
//}

std::tuple<size_t, entity::model::ConfigurationIndex, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeReadDescriptorCommonResponse(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemReadCommonDescriptorResponsePayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::ConfigurationIndex configurationIndex{ 0u };
	std::uint16_t reserved{ 0u };
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	// Read common READ_DESCRIPTOR Response fields
	des >> configurationIndex >> reserved;
	des >> descriptorType >> descriptorIndex;

	assert(des.usedBytes() == AecpAemReadCommonDescriptorResponsePayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(des.usedBytes(), configurationIndex, descriptorType, descriptorIndex);
}

entity::model::EntityDescriptor deserializeReadEntityDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::EntityDescriptor entityDescriptor{};

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadEntityDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check entity descriptor payload - Clause 7.2.1
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> entityDescriptor.entityID >> entityDescriptor.vendorEntityModelID >> entityDescriptor.entityCapabilities;
		des >> entityDescriptor.talkerStreamSources >> entityDescriptor.talkerCapabilities;
		des >> entityDescriptor.listenerStreamSinks >> entityDescriptor.listenerCapabilities;
		des >> entityDescriptor.controllerCapabilities;
		des >> entityDescriptor.availableIndex;
		des >> entityDescriptor.associationID;
		des >> entityDescriptor.entityName;
		des >> entityDescriptor.vendorNameString >> entityDescriptor.modelNameString;
		des >> entityDescriptor.firmwareVersion;
		des >> entityDescriptor.groupName;
		des >> entityDescriptor.serialNumber;
		des >> entityDescriptor.configurationsCount >> entityDescriptor.currentConfiguration;

		assert(des.usedBytes() == AecpAemReadEntityDescriptorResponsePayloadSize && "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Trace, "ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for ENTITY");
	}

	return entityDescriptor;
}

entity::model::ConfigurationDescriptor deserializeReadConfigurationDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::ConfigurationDescriptor configurationDescriptor{};

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadConfigurationDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check configuration descriptor payload - Clause 7.2.2
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> configurationDescriptor.objectName;
		des >> configurationDescriptor.localizedDescription;
		des >> configurationDescriptor.descriptorCountsCount >> configurationDescriptor.descriptorCountsOffset;

		// Check descriptor variable size
		constexpr size_t descriptorInfoSize = sizeof(entity::model::DescriptorType) + sizeof(std::uint16_t);
		auto const descriptorCountsSize = descriptorInfoSize * configurationDescriptor.descriptorCountsCount;
		if (des.remaining() < descriptorCountsSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Unpack descriptor remaining data
		for (auto index = 0u; index < configurationDescriptor.descriptorCountsCount; ++index)
		{
			entity::model::DescriptorType type;
			std::uint16_t count;
			des >> type >> count;
			configurationDescriptor.descriptorCounts[type] = count;
		}
		assert(des.usedBytes() == (protocol::aemPayload::AecpAemReadConfigurationDescriptorResponsePayloadMinSize + descriptorCountsSize) && "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Trace, "ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for CONFIGURATION");
	}

	return configurationDescriptor;
}

entity::model::StreamDescriptor deserializeReadStreamDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::StreamDescriptor streamDescriptor{};

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadStreamDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check stream descriptor payload - Clause 7.2.6
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> streamDescriptor.objectName;
		des >> streamDescriptor.localizedDescription >> streamDescriptor.clockDomainIndex >> streamDescriptor.streamFlags;
		des >> streamDescriptor.currentFormat >> streamDescriptor.formatsOffset >> streamDescriptor.numberOfFormats;
		des >> streamDescriptor.backupTalkerEntityID_0 >> streamDescriptor.backupTalkerUniqueID_0;
		des >> streamDescriptor.backupTalkerEntityID_1 >> streamDescriptor.backupTalkerUniqueID_1;
		des >> streamDescriptor.backupTalkerEntityID_2 >> streamDescriptor.backupTalkerUniqueID_2;
		des >> streamDescriptor.backedupTalkerEntityID >> streamDescriptor.backedupTalkerUnique;
		des >> streamDescriptor.avbInterfaceIndex >> streamDescriptor.bufferLength;

		// Check descriptor variable size
		constexpr size_t formatInfoSize = sizeof(std::uint64_t);
		auto const formatsSize = formatInfoSize * streamDescriptor.numberOfFormats;
		if (des.remaining() < formatsSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Compute deserializer offset for formats (Clause 7.4.5.2 says the formats_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
		auto const formatsOffset = sizeof(entity::model::ConfigurationIndex) + sizeof(std::uint16_t) + streamDescriptor.formatsOffset;

		// Set deserializer position
		if (formatsOffset < des.usedBytes())
			throw IncorrectPayloadSizeException();
		des.setPosition(formatsOffset);

		// Let's loop over the formats
		for (auto index = 0u; index < streamDescriptor.numberOfFormats; ++index)
		{
			std::uint64_t format;
			des >> format;
			streamDescriptor.formats.push_back(format);
		}

		if (des.remaining() != 0)
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Trace, "ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for STREAM");
	}

	return streamDescriptor;
}

entity::model::LocaleDescriptor deserializeReadLocaleDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::LocaleDescriptor localeDescriptor{};

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadLocaleDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check locale descriptor payload - Clause 7.2.11
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> localeDescriptor.localeID;
		des >> localeDescriptor.numberOfStringDescriptors >> localeDescriptor.baseStringDescriptorIndex;

		assert(des.usedBytes() == AecpAemReadLocaleDescriptorResponsePayloadSize && "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Trace, "ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for LOCALE");
	}

	return localeDescriptor;
}

entity::model::StringsDescriptor deserializeReadStringsDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::StringsDescriptor stringsDescriptor{};

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadStringsDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check strings descriptor payload - Clause 7.2.12
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		for (auto strIndex = 0u; strIndex < stringsDescriptor.strings.size(); ++strIndex)
		{
			des >> stringsDescriptor.strings[strIndex];
		}

		assert(des.usedBytes() == AecpAemReadStringsDescriptorResponsePayloadSize && "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Trace, "ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for STRINGS");
	}

	return stringsDescriptor;
}

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

/** SET_CLOCK_SOURCE Command - Clause 7.4.23.1 */
Serializer<AecpAemSetClockSourceCommandPayloadSize> serializeSetClockSourceCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ClockSourceIndex const clockSourceIndex)
{
	Serializer<AecpAemSetClockSourceCommandPayloadSize> ser;
	std::uint16_t const reserved{ 0u };

	ser << descriptorType << descriptorIndex;
	ser << clockSourceIndex << reserved;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::ClockSourceIndex> deserializeSetClockSourceCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemSetClockSourceCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::ClockSourceIndex clockSourceIndex{ 0u };
	std::uint16_t reserved{ 0u };

	des >> descriptorType >> descriptorIndex;
	des >> clockSourceIndex >> reserved;

	assert(des.usedBytes() == AecpAemSetClockSourceCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, clockSourceIndex);
}

/** SET_CLOCK_SOURCE Response - Clause 7.4.23.1 */
Serializer<AecpAemSetClockSourceResponsePayloadSize> serializeSetClockSourceResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ClockSourceIndex const clockSourceIndex)
{
	// Same as PROTONAME Command
	static_assert(AecpAemSetClockSourceResponsePayloadSize == AecpAemSetClockSourceCommandPayloadSize, "PROTONAME Response no longer the same as PROTONAME Command");
	return serializeSetClockSourceCommand(descriptorType, descriptorIndex, clockSourceIndex);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::ClockSourceIndex> deserializeSetClockSourceResponse(AemAecpdu::Payload const& payload)
{
	// Same as PROTONAME Command
	static_assert(AecpAemSetClockSourceResponsePayloadSize == AecpAemSetClockSourceCommandPayloadSize, "PROTONAME Response no longer the same as PROTONAME Command");
	return deserializeSetClockSourceCommand(payload);
}

/** GET_CLOCK_SOURCE Command - Clause 7.4.24.1 */
Serializer<AecpAemGetClockSourceCommandPayloadSize> serializeGetClockSourceCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemGetClockSourceCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeGetClockSourceCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetClockSourceCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> descriptorType >> descriptorIndex;

	assert(des.usedBytes() == AecpAemGetClockSourceCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}

/** GET_CLOCK_SOURCE Response - Clause 7.4.24.2 */
Serializer<AecpAemGetClockSourceResponsePayloadSize> serializeGetClockSourceResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ClockSourceIndex const clockSourceIndex)
{
	// Same as SET_CLOCK_SOURCE Command
	static_assert(AecpAemGetClockSourceResponsePayloadSize == AecpAemSetClockSourceCommandPayloadSize, "GET_CLOCK_SOURCE Response no longer the same as SET_CLOCK_SOURCE Command");
	return serializeSetClockSourceCommand(descriptorType, descriptorIndex, clockSourceIndex);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::ClockSourceIndex> deserializeGetClockSourceResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_CLOCK_SOURCE Command
	static_assert(AecpAemGetClockSourceResponsePayloadSize == AecpAemSetClockSourceCommandPayloadSize, "GET_CLOCK_SOURCE Response no longer the same as SET_CLOCK_SOURCE Command");
	return deserializeSetClockSourceCommand(payload);
}

/** START_STREAMING Command - Clause 7.4.35.1 */
Serializer<AecpAemStartStreamingCommandPayloadSize> serializeStartStreamingCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemStartStreamingCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeStartStreamingCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemStartStreamingCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> descriptorType >> descriptorIndex;

	assert(des.usedBytes() == AecpAemStartStreamingCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}

/** START_STREAMING Response - Clause 7.4.35.1 */
Serializer<AecpAemStartStreamingResponsePayloadSize> serializeStartStreamingResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	// Same as START_STREAMING Command
	static_assert(AecpAemStartStreamingResponsePayloadSize == AecpAemStartStreamingCommandPayloadSize, "START_STREAMING Response no longer the same as START_STREAMING Command");
	return serializeStartStreamingCommand(descriptorType, descriptorIndex);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeStartStreamingResponse(AemAecpdu::Payload const& payload)
{
	// Same as START_STREAMING Command
	static_assert(AecpAemStartStreamingResponsePayloadSize == AecpAemStartStreamingCommandPayloadSize, "START_STREAMING Response no longer the same as START_STREAMING Command");
	return deserializeStartStreamingCommand(payload);
}

/** STOP_STREAMING Command - Clause 7.4.36.1 */
Serializer<AecpAemStopStreamingCommandPayloadSize> serializeStopStreamingCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	// Same as START_STREAMING Command
	static_assert(AecpAemStopStreamingCommandPayloadSize == AecpAemStartStreamingCommandPayloadSize, "STOP_STREAMING Command no longer the same as START_STREAMING Command");
	return serializeStartStreamingCommand(descriptorType, descriptorIndex);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeStopStreamingCommand(AemAecpdu::Payload const& payload)
{
	// Same as START_STREAMING Command
	static_assert(AecpAemStopStreamingCommandPayloadSize == AecpAemStartStreamingCommandPayloadSize, "STOP_STREAMING Command no longer the same as START_STREAMING Command");
	return deserializeStartStreamingCommand(payload);
}

/** STOP_STREAMING Response - Clause 7.4.36.1 */
Serializer<AecpAemStopStreamingResponsePayloadSize> serializeStopStreamingResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	// Same as START_STREAMING Command
	static_assert(AecpAemStopStreamingResponsePayloadSize == AecpAemStartStreamingCommandPayloadSize, "STOP_STREAMING Response no longer the same as START_STREAMING Command");
	return serializeStartStreamingCommand(descriptorType, descriptorIndex);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeStopStreamingResponse(AemAecpdu::Payload const& payload)
{
	// Same as START_STREAMING Command
	static_assert(AecpAemStopStreamingResponsePayloadSize == AecpAemStartStreamingCommandPayloadSize, "STOP_STREAMING Response no longer the same as START_STREAMING Command");
	return deserializeStartStreamingCommand(payload);
}

/** GET_AVB_INFO Command - Clause 7.4.40.1 */
/** GET_AVB_INFO Response - Clause 7.4.40.2 */
/** GET_AS_PATH Command - Clause 7.4.41.1 */
/** GET_AS_PATH Response - Clause 7.4.41.2 */
/** GET_COUNTERS Command - Clause 7.4.42.1 */
/** GET_COUNTERS Response - Clause 7.4.42.2 */

/** GET_AUDIO_MAP Command - Clause 7.4.44.1 */
Serializer<AecpAemGetAudioMapCommandPayloadSize> serializeGetAudioMapCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::MapIndex const mapIndex)
{
	Serializer<AecpAemGetAudioMapCommandPayloadSize> ser;
	std::uint16_t const reserved{ 0u };

	ser << descriptorType << descriptorIndex;
	ser << mapIndex << reserved;

	assert(ser.usedBytes() == ser.capacity() && "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::MapIndex> deserializeGetAudioMapCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetAudioMapCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::MapIndex mapIndex{ 0u };
	std::uint16_t reserved{ 0u };

	des >> descriptorType >> descriptorIndex;
	des >> mapIndex >> reserved;

	assert(des.usedBytes() == AecpAemGetAudioMapCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, mapIndex);
}

/** GET_AUDIO_MAP Response - Clause 7.4.44.2 */
Serializer<AemAecpdu::MaximumPayloadLength> serializeGetAudioMapResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::MapIndex const mapIndex, entity::model::MapIndex const numberOfMaps, entity::model::AudioMappings const& mappings)
{
	Serializer<AemAecpdu::MaximumPayloadLength> ser;
	std::uint16_t const reserved{ 0u };

	ser << descriptorType << descriptorIndex;
	ser << mapIndex << numberOfMaps << static_cast<std::uint16_t>(mappings.size()) << reserved;

	// Serialize variable data
	for (auto const& mapping : mappings)
	{
		ser << mapping.streamIndex << mapping.streamChannel << mapping.clusterOffset << mapping.clusterChannel;
	}

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::MapIndex, entity::model::MapIndex, entity::model::AudioMappings> deserializeGetAudioMapResponse(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetAudioMapResponsePayloadMinSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::MapIndex mapIndex{ 0u };
	entity::model::MapIndex numberOfMaps{ 0u };
	std::uint16_t numberOfMappings{ 0u };
	std::uint16_t reserved{ 0u };

	des >> descriptorType >> descriptorIndex;
	des >> mapIndex >> numberOfMaps >> numberOfMappings >> reserved;

	// Check variable size
	constexpr size_t mapInfoSize = sizeof(entity::model::AudioMapping::streamIndex) + sizeof(entity::model::AudioMapping::streamChannel) + sizeof(entity::model::AudioMapping::clusterOffset) + sizeof(entity::model::AudioMapping::clusterChannel);
	auto const mappingsSize = mapInfoSize * numberOfMappings;
	if (des.remaining() < mappingsSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Unpack remaining data
	entity::model::AudioMappings mappings;
	for (auto index = 0u; index < numberOfMappings; ++index)
	{
		entity::model::AudioMapping mapping;
		des >> mapping.streamIndex >> mapping.streamChannel >> mapping.clusterOffset >> mapping.clusterChannel;
		mappings.push_back(mapping);
	}
	assert(des.usedBytes() == (protocol::aemPayload::AecpAemGetAudioMapResponsePayloadMinSize + mappingsSize) && "Used more bytes than specified in protocol constant");

	if (des.remaining() != 0)
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Trace, "GetAudioMap Response deserialize warning: Remaining bytes in buffer");

	return std::make_tuple(descriptorType, descriptorIndex, mapIndex, numberOfMaps, mappings);
}

/** ADD_AUDIO_MAPPINGS Command - Clause 7.4.45.1 */
Serializer<AemAecpdu::MaximumPayloadLength> serializeAddAudioMappingsCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AudioMappings const& mappings)
{
	Serializer<AemAecpdu::MaximumPayloadLength> ser;
	std::uint16_t const reserved{ 0u };

	ser << descriptorType << descriptorIndex;
	ser << static_cast<std::uint16_t>(mappings.size()) << reserved;

	// Serialize variable data
	for (auto const& mapping : mappings)
	{
		ser << mapping.streamIndex << mapping.streamChannel << mapping.clusterOffset << mapping.clusterChannel;
	}

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::AudioMappings> deserializeAddAudioMappingsCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemAddAudioMappingsCommandPayloadMinSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	std::uint16_t numberOfMappings{ 0u };
	std::uint16_t reserved{ 0u };

	des >> descriptorType >> descriptorIndex;
	des >> numberOfMappings >> reserved;

	// Check variable size
	constexpr size_t mapInfoSize = sizeof(entity::model::AudioMapping::streamIndex) + sizeof(entity::model::AudioMapping::streamChannel) + sizeof(entity::model::AudioMapping::clusterOffset) + sizeof(entity::model::AudioMapping::clusterChannel);
	auto const mappingsSize = mapInfoSize * numberOfMappings;
	if (des.remaining() < mappingsSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Unpack remaining data
	entity::model::AudioMappings mappings;
	for (auto index = 0u; index < numberOfMappings; ++index)
	{
		entity::model::AudioMapping mapping;
		des >> mapping.streamIndex >> mapping.streamChannel >> mapping.clusterOffset >> mapping.clusterChannel;
		mappings.push_back(mapping);
	}
	assert(des.usedBytes() == (protocol::aemPayload::AecpAemAddAudioMappingsCommandPayloadMinSize + mappingsSize) && "Used more bytes than specified in protocol constant");

	if (des.remaining() != 0)
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Trace, "AddAudioMap (or RemoveAudioMap) Command (or Response) deserialize warning: Remaining bytes in buffer");

	return std::make_tuple(descriptorType, descriptorIndex, mappings);
}

/** ADD_AUDIO_MAPPINGS Response - Clause 7.4.45.2 */
Serializer<AemAecpdu::MaximumPayloadLength> serializeAddAudioMappingsResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AudioMappings const& mappings)
{
	// Same as ADD_AUDIO_MAPPINGS Command
	static_assert(AecpAemAddAudioMappingsResponsePayloadMinSize == AecpAemAddAudioMappingsCommandPayloadMinSize, "ADD_AUDIO_MAPPINGS Response no longer the same as ADD_AUDIO_MAPPINGS Command");
	return serializeAddAudioMappingsCommand(descriptorType, descriptorIndex, mappings);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::AudioMappings> deserializeAddAudioMappingsResponse(AemAecpdu::Payload const& payload)
{
	// Same as ADD_AUDIO_MAPPINGS Command
	static_assert(AecpAemAddAudioMappingsResponsePayloadMinSize == AecpAemAddAudioMappingsCommandPayloadMinSize, "ADD_AUDIO_MAPPINGS Response no longer the same as ADD_AUDIO_MAPPINGS Command");
	return deserializeAddAudioMappingsCommand(payload);
}

/** REMOVE_AUDIO_MAPPINGS Command - Clause 7.4.46.1 */
Serializer<AemAecpdu::MaximumPayloadLength> serializeRemoveAudioMappingsCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AudioMappings const& mappings)
{
	// Same as ADD_AUDIO_MAPPINGS Command
	static_assert(AecpAemRemoveAudioMappingsCommandPayloadMinSize == AecpAemAddAudioMappingsCommandPayloadMinSize, "REMOVE_AUDIO_MAPPINGS Command no longer the same as ADD_AUDIO_MAPPINGS Command");
	return serializeAddAudioMappingsCommand(descriptorType, descriptorIndex, mappings);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::AudioMappings> deserializeRemoveAudioMappingsCommand(AemAecpdu::Payload const& payload)
{
	// Same as ADD_AUDIO_MAPPINGS Command
	static_assert(AecpAemRemoveAudioMappingsCommandPayloadMinSize == AecpAemAddAudioMappingsCommandPayloadMinSize, "REMOVE_AUDIO_MAPPINGS Command no longer the same as ADD_AUDIO_MAPPINGS Command");
	return deserializeAddAudioMappingsCommand(payload);
}

/** REMOVE_AUDIO_MAPPINGS Response - Clause 7.4.46.2 */
Serializer<AemAecpdu::MaximumPayloadLength> serializeRemoveAudioMappingsResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AudioMappings const& mappings)
{
	// Same as ADD_AUDIO_MAPPINGS Command
	static_assert(AecpAemRemoveAudioMappingsResponsePayloadMinSize == AecpAemAddAudioMappingsCommandPayloadMinSize, "REMOVE_AUDIO_MAPPINGS Response no longer the same as ADD_AUDIO_MAPPINGS Command");
	return serializeAddAudioMappingsCommand(descriptorType, descriptorIndex, mappings);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::AudioMappings> deserializeRemoveAudioMappingsResponse(AemAecpdu::Payload const& payload)
{
	// Same as ADD_AUDIO_MAPPINGS Command
	static_assert(AecpAemRemoveAudioMappingsResponsePayloadMinSize == AecpAemAddAudioMappingsCommandPayloadMinSize, "REMOVE_AUDIO_MAPPINGS Response no longer the same as ADD_AUDIO_MAPPINGS Command");
	return deserializeAddAudioMappingsCommand(payload);
}

} // namespace aemPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
