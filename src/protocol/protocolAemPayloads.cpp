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
* @file protocolAemPayloads.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/entityModelControlValuesTraits.hpp"

#include "protocolAemControlValuesPayloads.hpp"
#include "protocolAemPayloads.hpp"
#include "logHelper.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace aemPayload
{
/** Offset to be added/removed from Deserialization/Serialization buffer when computing 'offset' fields in various payload (required because the spec starts counting offset at 'descriptor_type' field, whilst our buffers start at 'configuration_index') */
static constexpr auto PayloadBufferOffset = sizeof(entity::model::ConfigurationIndex) + sizeof(std::uint16_t);

static inline void checkResponsePayload(AemAecpdu::Payload const& payload, entity::LocalEntity::AemCommandStatus const status, size_t const expectedPayloadCommandLength, size_t const expectedPayloadResponseLength)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	// If status is NotImplemented, we expect a reflected message (using Command length)
	if (status == entity::LocalEntity::AemCommandStatus::NotImplemented)
	{
		if (commandPayloadLength != expectedPayloadCommandLength || (expectedPayloadCommandLength > 0 && commandPayload == nullptr)) // Malformed packet
		{
			throw IncorrectPayloadSizeException();
		}
	}
	else
	{
		// Otherwise we expect a valid response with all fields
		if (commandPayloadLength < expectedPayloadResponseLength || (expectedPayloadResponseLength > 0 && commandPayload == nullptr)) // Malformed packet
		{
			throw IncorrectPayloadSizeException();
		}
	}
}

/** ACQUIRE_ENTITY Command - IEEE1722.1-2013 Clause 7.4.1.1 */
Serializer<AecpAemAcquireEntityCommandPayloadSize> serializeAcquireEntityCommand(AemAcquireEntityFlags const flags, UniqueIdentifier const ownerID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemAcquireEntityCommandPayloadSize> ser;

	ser << flags;
	ser << ownerID;
	ser << descriptorType << descriptorIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	UniqueIdentifier ownerID{};
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> flags;
	des >> ownerID;
	des >> descriptorType >> descriptorIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemAcquireEntityCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(flags, ownerID, descriptorType, descriptorIndex);
}

/** ACQUIRE_ENTITY Response - IEEE1722.1-2013 Clause 7.4.1.1 */
Serializer<AecpAemAcquireEntityResponsePayloadSize> serializeAcquireEntityResponse(AemAcquireEntityFlags const flags, UniqueIdentifier ownerID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	// Same as ACQUIRE_ENTITY Command
	static_assert(AecpAemAcquireEntityResponsePayloadSize == AecpAemAcquireEntityCommandPayloadSize, "ACQUIRE_ENTITY Response no longer the same as ACQUIRE_ENTITY Command");
	return serializeAcquireEntityCommand(flags, ownerID, descriptorType, descriptorIndex);
}

std::tuple<AemAcquireEntityFlags, UniqueIdentifier, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeAcquireEntityResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as ACQUIRE_ENTITY Command
	static_assert(AecpAemAcquireEntityResponsePayloadSize == AecpAemAcquireEntityCommandPayloadSize, "ACQUIRE_ENTITY Response no longer the same as ACQUIRE_ENTITY Command");
	checkResponsePayload(payload, status, AecpAemAcquireEntityCommandPayloadSize, AecpAemAcquireEntityResponsePayloadSize);
	return deserializeAcquireEntityCommand(payload);
}

/** LOCK_ENTITY Command - IEEE1722.1-2013 Clause 7.4.2.1 */
Serializer<AecpAemLockEntityCommandPayloadSize> serializeLockEntityCommand(AemLockEntityFlags flags, UniqueIdentifier lockedID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemLockEntityCommandPayloadSize> ser;

	ser << flags;
	ser << lockedID;
	ser << descriptorType << descriptorIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	UniqueIdentifier lockedID{};
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> flags;
	des >> lockedID;
	des >> descriptorType >> descriptorIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemLockEntityCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(flags, lockedID, descriptorType, descriptorIndex);
}

/** LOCK_ENTITY Response - IEEE1722.1-2013 Clause 7.4.2.1 */
Serializer<AecpAemLockEntityResponsePayloadSize> serializeLockEntityResponse(AemLockEntityFlags flags, UniqueIdentifier lockedID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	// Same as LOCK_ENTITY Command
	static_assert(AecpAemLockEntityResponsePayloadSize == AecpAemLockEntityCommandPayloadSize, "LOCK_ENTITY Response no longer the same as LOCK_ENTITY Command");
	return serializeLockEntityCommand(flags, lockedID, descriptorType, descriptorIndex);
}

std::tuple<AemLockEntityFlags, UniqueIdentifier, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeLockEntityResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as LOCK_ENTITY Command
	static_assert(AecpAemLockEntityResponsePayloadSize == AecpAemLockEntityCommandPayloadSize, "LOCK_ENTITY Response no longer the same as LOCK_ENTITY Command");
	checkResponsePayload(payload, status, AecpAemLockEntityCommandPayloadSize, AecpAemLockEntityResponsePayloadSize);
	return deserializeLockEntityCommand(payload);
}

/** READ_DESCRIPTOR Command - IEEE1722.1-2013 Clause 7.4.5.1 */
Serializer<AecpAemReadDescriptorCommandPayloadSize> serializeReadDescriptorCommand(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemReadDescriptorCommandPayloadSize> ser;
	std::uint16_t const reserved{ 0u };

	ser << configurationIndex << reserved;
	ser << descriptorType << descriptorIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> configurationIndex >> reserved;
	des >> descriptorType >> descriptorIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemReadDescriptorCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(configurationIndex, descriptorType, descriptorIndex);
}

/** READ_DESCRIPTOR Response - IEEE1722.1-2013 Clause 7.4.5.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeReadDescriptorCommonResponse(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AemAecpdu::MaximumSendPayloadBufferLength> ser;
	std::uint16_t const reserved{ 0u };

	ser << configurationIndex << reserved;
	ser << descriptorType << descriptorIndex;

	return ser;
}

void serializeReadEntityDescriptorResponse(Serializer<AemAecpdu::MaximumSendPayloadBufferLength>& ser, entity::model::EntityDescriptor const& entityDescriptor)
{
	ser << entityDescriptor.entityID << entityDescriptor.entityModelID << entityDescriptor.entityCapabilities;
	ser << entityDescriptor.talkerStreamSources << entityDescriptor.talkerCapabilities;
	ser << entityDescriptor.listenerStreamSinks << entityDescriptor.listenerCapabilities;
	ser << entityDescriptor.controllerCapabilities;
	ser << entityDescriptor.availableIndex;
	ser << entityDescriptor.associationID;
	ser << entityDescriptor.entityName;
	ser << entityDescriptor.vendorNameString << entityDescriptor.modelNameString;
	ser << entityDescriptor.firmwareVersion;
	ser << entityDescriptor.groupName;
	ser << entityDescriptor.serialNumber;
	ser << entityDescriptor.configurationsCount << entityDescriptor.currentConfiguration;
}

void serializeReadConfigurationDescriptorResponse(Serializer<AemAecpdu::MaximumSendPayloadBufferLength>& ser, entity::model::ConfigurationDescriptor const& configurationDescriptor)
{
	ser << configurationDescriptor.objectName;
	ser << configurationDescriptor.localizedDescription;
	auto const descriptorCountsCount = static_cast<std::uint16_t>(configurationDescriptor.descriptorCounts.size());
	ser << descriptorCountsCount;
	// Compute serializer offset for descriptor counts (IEEE1722.1-2021 Clause 7.2.2 says the descriptor_counts_offset field is from the base of the descriptor, which is not where our serializer buffer starts). Also have to add the size of this very field since offset starts after it...
	auto const descriptorCountsOffset = static_cast<std::uint16_t>(ser.usedBytes() - PayloadBufferOffset + sizeof(std::uint16_t));
	AVDECC_ASSERT(descriptorCountsOffset == 74, "Descriptor Counts offset should be 74 for IEEE1722.1-2021");
	ser << descriptorCountsOffset;

	for (auto const& [descriptorType, descriptorCount] : configurationDescriptor.descriptorCounts)
	{
		ser << descriptorType << descriptorCount;
	}
}

std::tuple<size_t, entity::model::ConfigurationIndex, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeReadDescriptorCommonResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	checkResponsePayload(payload, status, AecpAemReadDescriptorCommandPayloadSize, AecpAemReadCommonDescriptorResponsePayloadSize);

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::ConfigurationIndex configurationIndex{ 0u };
	std::uint16_t reserved{ 0u };
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	// Read common READ_DESCRIPTOR Response fields
	des >> configurationIndex >> reserved;
	des >> descriptorType >> descriptorIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemReadCommonDescriptorResponsePayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(des.usedBytes(), configurationIndex, descriptorType, descriptorIndex);
}

entity::model::EntityDescriptor deserializeReadEntityDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::EntityDescriptor entityDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadEntityDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check entity descriptor payload - IEEE1722.1-2013 Clause 7.2.1
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> entityDescriptor.entityID >> entityDescriptor.entityModelID >> entityDescriptor.entityCapabilities;
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

		AVDECC_ASSERT(des.usedBytes() == AecpAemReadEntityDescriptorResponsePayloadSize, "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_ENTITY_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return entityDescriptor;
}

entity::model::ConfigurationDescriptor deserializeReadConfigurationDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::ConfigurationDescriptor configurationDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadConfigurationDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check configuration descriptor payload - IEEE1722.1-2013 Clause 7.2.2
		Deserializer des(commandPayload, commandPayloadLength);
		std::uint16_t descriptorCountsCount{ 0u };
		std::uint16_t descriptorCountsOffset{ 0u };
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> configurationDescriptor.objectName;
		des >> configurationDescriptor.localizedDescription;
		des >> descriptorCountsCount >> descriptorCountsOffset;

		// Check descriptor variable size
		constexpr size_t descriptorInfoSize = sizeof(entity::model::DescriptorType) + sizeof(std::uint16_t);
		auto const descriptorCountsSize = descriptorInfoSize * descriptorCountsCount;
		if (des.remaining() < descriptorCountsSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Compute deserializer offset for descriptor counts (IEEE1722.1-2013 Clause 7.2.2 says the descriptor_counts_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
		descriptorCountsOffset += PayloadBufferOffset;

		// Set deserializer position
		if (descriptorCountsOffset < des.usedBytes())
		{
			throw IncorrectPayloadSizeException();
		}
		des.setPosition(descriptorCountsOffset);

		// Unpack descriptor remaining data
		for (auto index = 0u; index < descriptorCountsCount; ++index)
		{
			entity::model::DescriptorType type;
			std::uint16_t count;
			des >> type >> count;
			configurationDescriptor.descriptorCounts[type] = count;
		}
		AVDECC_ASSERT(des.usedBytes() == (protocol::aemPayload::AecpAemReadConfigurationDescriptorResponsePayloadMinSize + descriptorCountsSize), "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_CONFIGURATION_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return configurationDescriptor;
}

entity::model::AudioUnitDescriptor deserializeReadAudioUnitDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::AudioUnitDescriptor audioUnitDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadAudioUnitDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check audio unit descriptor payload - IEEE1722.1-2013 Clause 7.2.3
		Deserializer des(commandPayload, commandPayloadLength);
		std::uint16_t samplingRatesOffset{ 0u };
		std::uint16_t numberOfSamplingRates{ 0u };
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> audioUnitDescriptor.objectName;
		des >> audioUnitDescriptor.localizedDescription >> audioUnitDescriptor.clockDomainIndex;
		des >> audioUnitDescriptor.numberOfStreamInputPorts >> audioUnitDescriptor.baseStreamInputPort;
		des >> audioUnitDescriptor.numberOfStreamOutputPorts >> audioUnitDescriptor.baseStreamOutputPort;
		des >> audioUnitDescriptor.numberOfExternalInputPorts >> audioUnitDescriptor.baseExternalInputPort;
		des >> audioUnitDescriptor.numberOfExternalOutputPorts >> audioUnitDescriptor.baseExternalOutputPort;
		des >> audioUnitDescriptor.numberOfInternalInputPorts >> audioUnitDescriptor.baseInternalInputPort;
		des >> audioUnitDescriptor.numberOfInternalOutputPorts >> audioUnitDescriptor.baseInternalOutputPort;
		des >> audioUnitDescriptor.numberOfControls >> audioUnitDescriptor.baseControl;
		des >> audioUnitDescriptor.numberOfSignalSelectors >> audioUnitDescriptor.baseSignalSelector;
		des >> audioUnitDescriptor.numberOfMixers >> audioUnitDescriptor.baseMixer;
		des >> audioUnitDescriptor.numberOfMatrices >> audioUnitDescriptor.baseMatrix;
		des >> audioUnitDescriptor.numberOfSplitters >> audioUnitDescriptor.baseSplitter;
		des >> audioUnitDescriptor.numberOfCombiners >> audioUnitDescriptor.baseCombiner;
		des >> audioUnitDescriptor.numberOfDemultiplexers >> audioUnitDescriptor.baseDemultiplexer;
		des >> audioUnitDescriptor.numberOfMultiplexers >> audioUnitDescriptor.baseMultiplexer;
		des >> audioUnitDescriptor.numberOfTranscoders >> audioUnitDescriptor.baseTranscoder;
		des >> audioUnitDescriptor.numberOfControlBlocks >> audioUnitDescriptor.baseControlBlock;
		des >> audioUnitDescriptor.currentSamplingRate >> samplingRatesOffset >> numberOfSamplingRates;

		// Check descriptor variable size
		auto const samplingRatesSize = sizeof(entity::model::SamplingRate) * numberOfSamplingRates;
		if (des.remaining() < samplingRatesSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Compute deserializer offset for sampling rates (IEEE1722.1-2013 Clause 7.2.3 says the sampling_rates_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
		samplingRatesOffset += PayloadBufferOffset;

		// Set deserializer position
		if (samplingRatesOffset < des.usedBytes())
			throw IncorrectPayloadSizeException();
		des.setPosition(samplingRatesOffset);

		// Let's loop over the sampling rates
		for (auto index = 0u; index < numberOfSamplingRates; ++index)
		{
			entity::model::SamplingRate rate;
			des >> rate;
			audioUnitDescriptor.samplingRates.insert(rate);
		}

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_AUDIO_UNIT_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return audioUnitDescriptor;
}

entity::model::StreamDescriptor deserializeReadStreamDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::StreamDescriptor streamDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadStreamDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check stream descriptor payload - IEEE1722.1-2013 Clause 7.2.6
		Deserializer des(commandPayload, commandPayloadLength);
		std::uint16_t formatsOffset{ 0u };
		std::uint16_t numberOfFormats{ 0u };
		auto endDescriptorOffset{ commandPayloadLength };
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> streamDescriptor.objectName;
		des >> streamDescriptor.localizedDescription >> streamDescriptor.clockDomainIndex >> streamDescriptor.streamFlags;
		des >> streamDescriptor.currentFormat >> formatsOffset >> numberOfFormats;
		des >> streamDescriptor.backupTalkerEntityID_0 >> streamDescriptor.backupTalkerUniqueID_0;
		des >> streamDescriptor.backupTalkerEntityID_1 >> streamDescriptor.backupTalkerUniqueID_1;
		des >> streamDescriptor.backupTalkerEntityID_2 >> streamDescriptor.backupTalkerUniqueID_2;
		des >> streamDescriptor.backedupTalkerEntityID >> streamDescriptor.backedupTalkerUnique;
		des >> streamDescriptor.avbInterfaceIndex >> streamDescriptor.bufferLength;

		// Compute deserializer offset for formats (IEEE1722.1-2013 Clause 7.2.6 says the formats_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
		formatsOffset += PayloadBufferOffset;

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
		// Check if we have redundant fields (AVnu Alliance 'Network Redundancy' extension)
		std::uint16_t redundantOffset{ 0u };
		std::uint16_t numberOfRedundantStreams{ 0u };
		auto const remainingBytesBeforeFormats = formatsOffset - des.usedBytes();
		if (remainingBytesBeforeFormats >= (sizeof(redundantOffset) + sizeof(numberOfRedundantStreams)))
		{
			des >> redundantOffset >> numberOfRedundantStreams;
			// Compute deserializer offset for redundant streams association (IEEE1722.1-2013 Clause 7.2.6 says the redundant_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
			redundantOffset += PayloadBufferOffset;
			endDescriptorOffset = redundantOffset;
		}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
		auto const staticPartEndOffset = des.usedBytes();

		// Check descriptor variable size
		constexpr size_t formatInfoSize = sizeof(std::uint64_t);
		auto const formatsSize = formatInfoSize * numberOfFormats;
		if (formatsSize > static_cast<decltype(formatsSize)>(endDescriptorOffset - formatsOffset))
			throw IncorrectPayloadSizeException();
		if (formatsOffset < staticPartEndOffset)
			throw IncorrectPayloadSizeException();

		// Read formats
		// Set deserializer position
		des.setPosition(formatsOffset);

		// Let's loop over the formats
		for (auto index = 0u; index < numberOfFormats; ++index)
		{
			entity::model::StreamFormat format{};
			des >> format;
			streamDescriptor.formats.insert(format);
		}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
		// Read redundant streams association
		if (redundantOffset > 0)
		{
			// Set deserializer position
			if (redundantOffset < staticPartEndOffset)
				throw IncorrectPayloadSizeException();
			des.setPosition(redundantOffset);

			// Let's loop over the redundant streams association
			for (auto index = 0u; index < numberOfRedundantStreams; ++index)
			{
				entity::model::StreamIndex redundantStreamIndex;
				des >> redundantStreamIndex;
				streamDescriptor.redundantStreams.insert(redundantStreamIndex);
			}
		}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_STREAM_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return streamDescriptor;
}

entity::model::JackDescriptor deserializeReadJackDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::JackDescriptor jackDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadJackDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check jack descriptor payload - IEEE1722.1-2013 Clause 7.2.7
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> jackDescriptor.objectName;
		des >> jackDescriptor.localizedDescription;
		des >> jackDescriptor.jackFlags >> jackDescriptor.jackType;
		des >> jackDescriptor.numberOfControls >> jackDescriptor.baseControl;

		AVDECC_ASSERT(des.usedBytes() == AecpAemReadJackDescriptorResponsePayloadSize, "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_JACK_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return jackDescriptor;
}

entity::model::AvbInterfaceDescriptor deserializeReadAvbInterfaceDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::AvbInterfaceDescriptor avbInterfaceDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadAvbInterfaceDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check avb interface descriptor payload - IEEE1722.1-2013 Clause 7.2.8
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> avbInterfaceDescriptor.objectName;
		des >> avbInterfaceDescriptor.localizedDescription;
		des >> avbInterfaceDescriptor.macAddress;
		des >> avbInterfaceDescriptor.interfaceFlags;
		des >> avbInterfaceDescriptor.clockIdentity;
		des >> avbInterfaceDescriptor.priority1 >> avbInterfaceDescriptor.clockClass;
		des >> avbInterfaceDescriptor.offsetScaledLogVariance >> avbInterfaceDescriptor.clockAccuracy;
		des >> avbInterfaceDescriptor.priority2 >> avbInterfaceDescriptor.domainNumber;
		des >> avbInterfaceDescriptor.logSyncInterval >> avbInterfaceDescriptor.logAnnounceInterval >> avbInterfaceDescriptor.logPDelayInterval;
		des >> avbInterfaceDescriptor.portNumber;

		AVDECC_ASSERT(des.usedBytes() == AecpAemReadAvbInterfaceDescriptorResponsePayloadSize, "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_AVB_INTERFACE_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return avbInterfaceDescriptor;
}

entity::model::ClockSourceDescriptor deserializeReadClockSourceDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::ClockSourceDescriptor clockSourceDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadClockSourceDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check clock source descriptor payload - IEEE1722.1-2013 Clause 7.2.9
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> clockSourceDescriptor.objectName;
		des >> clockSourceDescriptor.localizedDescription;
		des >> clockSourceDescriptor.clockSourceFlags >> clockSourceDescriptor.clockSourceType;
		des >> clockSourceDescriptor.clockSourceIdentifier;
		des >> clockSourceDescriptor.clockSourceLocationType >> clockSourceDescriptor.clockSourceLocationIndex;

		AVDECC_ASSERT(des.usedBytes() == AecpAemReadClockSourceDescriptorResponsePayloadSize, "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_CLOCK_SOURCE_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return clockSourceDescriptor;
}

entity::model::MemoryObjectDescriptor deserializeReadMemoryObjectDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::MemoryObjectDescriptor memoryObjectDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadMemoryObjectDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check memory object descriptor payload - IEEE1722.1-2013 Clause 7.2.10
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> memoryObjectDescriptor.objectName;
		des >> memoryObjectDescriptor.localizedDescription;
		des >> memoryObjectDescriptor.memoryObjectType;
		des >> memoryObjectDescriptor.targetDescriptorType >> memoryObjectDescriptor.targetDescriptorIndex;
		des >> memoryObjectDescriptor.startAddress >> memoryObjectDescriptor.maximumLength >> memoryObjectDescriptor.length;
#pragma message("TODO: Unpack the new field added in corrigendum document (but preserve compatibility with older devices if the field is not present!!)")

		AVDECC_ASSERT(des.usedBytes() == AecpAemReadMemoryObjectDescriptorResponsePayloadSize, "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_MEMORY_OBJECT_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return memoryObjectDescriptor;
}

entity::model::LocaleDescriptor deserializeReadLocaleDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::LocaleDescriptor localeDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadLocaleDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check locale descriptor payload - IEEE1722.1-2013 Clause 7.2.11
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> localeDescriptor.localeID;
		des >> localeDescriptor.numberOfStringDescriptors >> localeDescriptor.baseStringDescriptorIndex;

		AVDECC_ASSERT(des.usedBytes() == AecpAemReadLocaleDescriptorResponsePayloadSize, "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_LOCALE_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return localeDescriptor;
}

entity::model::StringsDescriptor deserializeReadStringsDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::StringsDescriptor stringsDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadStringsDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check strings descriptor payload - IEEE1722.1-2013 Clause 7.2.12
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		for (auto strIndex = 0u; strIndex < stringsDescriptor.strings.size(); ++strIndex)
		{
			des >> stringsDescriptor.strings[strIndex];
		}

		AVDECC_ASSERT(des.usedBytes() == AecpAemReadStringsDescriptorResponsePayloadSize, "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_STRINGS_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return stringsDescriptor;
}

entity::model::StreamPortDescriptor deserializeReadStreamPortDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::StreamPortDescriptor streamPortDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadStreamPortDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check stream port descriptor payload - IEEE1722.1-2013 Clause 7.2.13
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> streamPortDescriptor.clockDomainIndex >> streamPortDescriptor.portFlags;
		des >> streamPortDescriptor.numberOfControls >> streamPortDescriptor.baseControl;
		des >> streamPortDescriptor.numberOfClusters >> streamPortDescriptor.baseCluster;
		des >> streamPortDescriptor.numberOfMaps >> streamPortDescriptor.baseMap;

		AVDECC_ASSERT(des.usedBytes() == AecpAemReadStreamPortDescriptorResponsePayloadSize, "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_STREAM_PORT_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return streamPortDescriptor;
}

entity::model::ExternalPortDescriptor deserializeReadExternalPortDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::ExternalPortDescriptor externalPortDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadExternalPortDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check external port descriptor payload - IEEE1722.1-2013 Clause 7.2.14
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> externalPortDescriptor.clockDomainIndex >> externalPortDescriptor.portFlags;
		des >> externalPortDescriptor.numberOfControls >> externalPortDescriptor.baseControl;
		des >> externalPortDescriptor.signalType >> externalPortDescriptor.signalIndex >> externalPortDescriptor.signalOutput;
		des >> externalPortDescriptor.blockLatency >> externalPortDescriptor.jackIndex;

		AVDECC_ASSERT(des.usedBytes() == AecpAemReadExternalPortDescriptorResponsePayloadSize, "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_EXTERNAL_PORT_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return externalPortDescriptor;
}

entity::model::InternalPortDescriptor deserializeReadInternalPortDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::InternalPortDescriptor internalPortDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadInternalPortDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check internal port descriptor payload - IEEE1722.1-2013 Clause 7.2.15
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> internalPortDescriptor.clockDomainIndex >> internalPortDescriptor.portFlags;
		des >> internalPortDescriptor.numberOfControls >> internalPortDescriptor.baseControl;
		des >> internalPortDescriptor.signalType >> internalPortDescriptor.signalIndex >> internalPortDescriptor.signalOutput;
		des >> internalPortDescriptor.blockLatency >> internalPortDescriptor.internalIndex;

		AVDECC_ASSERT(des.usedBytes() == AecpAemReadInternalPortDescriptorResponsePayloadSize, "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_INTERNAL_PORT_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return internalPortDescriptor;
}

entity::model::AudioClusterDescriptor deserializeReadAudioClusterDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::AudioClusterDescriptor audioClusterDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadAudioClusterDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check audio cluster descriptor payload - IEEE1722.1-2013 Clause 7.2.16
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> audioClusterDescriptor.objectName;
		des >> audioClusterDescriptor.localizedDescription;
		des >> audioClusterDescriptor.signalType >> audioClusterDescriptor.signalIndex >> audioClusterDescriptor.signalOutput;
		des >> audioClusterDescriptor.pathLatency >> audioClusterDescriptor.blockLatency;
		des >> audioClusterDescriptor.channelCount >> audioClusterDescriptor.format;

		AVDECC_ASSERT(des.usedBytes() == AecpAemReadAudioClusterDescriptorResponsePayloadSize, "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_AUDIO_CLUSTER_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return audioClusterDescriptor;
}

entity::model::AudioMapDescriptor deserializeReadAudioMapDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::AudioMapDescriptor audioMapDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadAudioMapDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check audio map descriptor payload - IEEE1722.1-2013 Clause 7.2.19
		Deserializer des(commandPayload, commandPayloadLength);
		std::uint16_t mappingsOffset{ 0u };
		std::uint16_t numberOfMappings{ 0u };
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> mappingsOffset >> numberOfMappings;

		// Check descriptor variable size
		auto const mappingsSize = entity::model::AudioMapping::size() * numberOfMappings;
		if (des.remaining() < mappingsSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Compute deserializer offset for sampling rates (IEEE1722.1-2013 Clause 7.2.19 says the mappings_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
		mappingsOffset += PayloadBufferOffset;

		// Set deserializer position
		if (mappingsOffset < des.usedBytes())
			throw IncorrectPayloadSizeException();
		des.setPosition(mappingsOffset);

		// Let's loop over the mappings
		for (auto index = 0u; index < numberOfMappings; ++index)
		{
			entity::model::AudioMapping mapping;
			des >> mapping.streamIndex >> mapping.streamChannel >> mapping.clusterOffset >> mapping.clusterChannel;
			audioMapDescriptor.mappings.push_back(mapping);
		}

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_AUDIO_MAP_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return audioMapDescriptor;
}

static inline void createUnpackFullControlValuesDispatchTable(std::unordered_map<entity::model::ControlValueType::Type, std::function<std::tuple<entity::model::ControlValues, entity::model::ControlValues>(Deserializer&, std::uint16_t)>>& dispatchTable)
{
	/** Linear Values - IEEE1722.1-2013 Clause 7.3.5.2.1 */
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearInt8] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt8>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearUInt8] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt8>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearInt16] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt16>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearUInt16] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt16>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearInt32] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt32>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearUInt32] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt32>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearInt64] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt64>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearUInt64] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt64>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearFloat] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearFloat>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearDouble] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearDouble>::unpackFullControlValues;

	/** Selector Value - IEEE1722.1-2013 Clause 7.3.5.2.2 */
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorInt8] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt8>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorUInt8] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt8>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorInt16] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt16>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorUInt16] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt16>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorInt32] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt32>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorUInt32] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt32>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorInt64] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt64>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorUInt64] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt64>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorFloat] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorFloat>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorDouble] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorDouble>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorString] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorString>::unpackFullControlValues;

	/** Array Values - IEEE1722.1-2013 Clause 7.3.5.2.3 */
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayInt8] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt8>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayUInt8] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt8>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayInt16] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt16>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayUInt16] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt16>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayInt32] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt32>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayUInt32] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt32>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayInt64] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt64>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayUInt64] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt64>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayFloat] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayFloat>::unpackFullControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayDouble] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayDouble>::unpackFullControlValues;

	/** UTF-8 String Value - IEEE1722.1-2013 Clause 7.3.5.2.4 */
	dispatchTable[entity::model::ControlValueType::Type::ControlUtf8] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlUtf8>::unpackFullControlValues;
}

entity::model::ControlDescriptor deserializeReadControlDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	static auto s_Dispatch = std::unordered_map<entity::model::ControlValueType::Type, std::function<std::tuple<entity::model::ControlValues, entity::model::ControlValues>(Deserializer&, std::uint16_t)>>{};

	if (s_Dispatch.empty())
	{
		// Create the dispatch table
		createUnpackFullControlValuesDispatchTable(s_Dispatch);
	}

	auto controlDescriptor = entity::model::ControlDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadControlDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check control descriptor payload - IEEE1722.1-2013 Clause 7.2.22
		auto des = Deserializer{ commandPayload, commandPayloadLength };
		auto valuesOffset = std::uint16_t{ 0u };

		des.setPosition(commonSize); // Skip already unpacked common header
		des >> controlDescriptor.objectName >> controlDescriptor.localizedDescription;
		des >> controlDescriptor.blockLatency >> controlDescriptor.controlLatency >> controlDescriptor.controlDomain;
		des >> controlDescriptor.controlValueType >> controlDescriptor.controlType >> controlDescriptor.resetTime;
		des >> valuesOffset >> controlDescriptor.numberOfValues;
		des >> controlDescriptor.signalType >> controlDescriptor.signalIndex >> controlDescriptor.signalOutput;

		// No need to check descriptor variable size, we'll let the ControlValueType specific unpacker throw if needed

		// Compute deserializer offset for values (IEEE1722.1-2013 Clause 7.2.22 says the values_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
		valuesOffset += PayloadBufferOffset;

		// Set deserializer position
		if (valuesOffset < des.usedBytes())
			throw IncorrectPayloadSizeException();
		des.setPosition(valuesOffset);

		// Unpack Control Values based on ControlValueType
		auto const valueType = controlDescriptor.controlValueType.getType();
		if (auto const& it = s_Dispatch.find(valueType); it != s_Dispatch.end())
		{
			try
			{
				auto [valuesStatic, valuesDynamic] = it->second(des, controlDescriptor.numberOfValues);
				controlDescriptor.valuesStatic = std::move(valuesStatic);
				controlDescriptor.valuesDynamic = std::move(valuesDynamic);
			}
			catch ([[maybe_unused]] std::invalid_argument const& e)
			{
				LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Unsupported ControlValueType for READ_CONTROL_DESCRIPTOR RESPONSE: {} ({})", valueType, e.what());
			}
		}
		else
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Unsupported ControlValueType for READ_CONTROL_DESCRIPTOR RESPONSE: {}", valueType);
		}

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_CONTROL_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return controlDescriptor;
}

entity::model::ClockDomainDescriptor deserializeReadClockDomainDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::ClockDomainDescriptor clockDomainDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadClockDomainDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check clock domain descriptor payload - IEEE1722.1-2013 Clause 7.2.32
		Deserializer des(commandPayload, commandPayloadLength);
		std::uint16_t clockSourcesOffset{ 0u };
		std::uint16_t numberOfClockSources{ 0u };
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> clockDomainDescriptor.objectName;
		des >> clockDomainDescriptor.localizedDescription;
		des >> clockDomainDescriptor.clockSourceIndex;
		des >> clockSourcesOffset >> numberOfClockSources;

		// Check descriptor variable size
		auto const clockSourcesSize = sizeof(entity::model::ClockSourceIndex) * numberOfClockSources;
		if (des.remaining() < clockSourcesSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Compute deserializer offset for sampling rates (IEEE1722.1-2013 Clause 7.2.32 says the clock_sources_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
		clockSourcesOffset += PayloadBufferOffset;

		// Set deserializer position
		if (clockSourcesOffset < des.usedBytes())
			throw IncorrectPayloadSizeException();
		des.setPosition(clockSourcesOffset);

		// Let's loop over the clock sources
		for (auto index = 0u; index < numberOfClockSources; ++index)
		{
			entity::model::ClockSourceIndex clockSourceIndex;
			des >> clockSourceIndex;
			clockDomainDescriptor.clockSources.push_back(clockSourceIndex);
		}

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_CLOCK_DOMAIN_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return clockDomainDescriptor;
}

entity::model::TimingDescriptor deserializeReadTimingDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::TimingDescriptor timingDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadTimingDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check timing descriptor payload - IEEE1722.1-2021 Clause 7.2.34
		Deserializer des(commandPayload, commandPayloadLength);
		std::uint16_t ptpInstancesOffset{ 0u };
		std::uint16_t numberOfPtpInstances{ 0u };
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> timingDescriptor.objectName;
		des >> timingDescriptor.localizedDescription >> timingDescriptor.algorithm;
		des >> ptpInstancesOffset >> numberOfPtpInstances;

		// Check descriptor variable size
		auto const ptpInstancesSize = sizeof(entity::model::PtpInstanceIndex) * numberOfPtpInstances;
		if (des.remaining() < ptpInstancesSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Compute deserializer offset for sampling rates (IEEE1722.1-2032 Clause 7.2.34 says the ptp_instances_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
		ptpInstancesOffset += PayloadBufferOffset;

		// Set deserializer position
		if (ptpInstancesOffset < des.usedBytes())
			throw IncorrectPayloadSizeException();
		des.setPosition(ptpInstancesOffset);

		// Let's loop over the ptp instances
		for (auto index = 0u; index < numberOfPtpInstances; ++index)
		{
			entity::model::PtpInstanceIndex ptpInstanceIndex;
			des >> ptpInstanceIndex;
			timingDescriptor.ptpInstances.push_back(ptpInstanceIndex);
		}

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_TIMING_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return timingDescriptor;
}

entity::model::PtpInstanceDescriptor deserializeReadPtpInstanceDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::PtpInstanceDescriptor ptpInstanceDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadPtpInstanceDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check ptp instance descriptor payload - IEEE1722.1-2021 Clause 7.2.35
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> ptpInstanceDescriptor.objectName;
		des >> ptpInstanceDescriptor.localizedDescription;
		des >> ptpInstanceDescriptor.clockIdentity >> ptpInstanceDescriptor.flags;
		des >> ptpInstanceDescriptor.numberOfControls >> ptpInstanceDescriptor.baseControl;
		des >> ptpInstanceDescriptor.numberOfPtpPorts >> ptpInstanceDescriptor.basePtpPort;

		AVDECC_ASSERT(des.usedBytes() == AecpAemReadPtpInstanceDescriptorResponsePayloadSize, "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_PTP_INSTANCE_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return ptpInstanceDescriptor;
}

entity::model::PtpPortDescriptor deserializeReadPtpPortDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status)
{
	entity::model::PtpPortDescriptor ptpPortDescriptor{};

	// IEEE1722.1-2013 Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadPtpPortDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check ptp port descriptor payload - IEEE1722.1-2021 Clause 7.2.36
		Deserializer des(commandPayload, commandPayloadLength);
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> ptpPortDescriptor.objectName;
		des >> ptpPortDescriptor.localizedDescription;
		des >> ptpPortDescriptor.portNumber >> ptpPortDescriptor.portType >> ptpPortDescriptor.flags >> ptpPortDescriptor.avbInterfaceIndex;
		des.unpackBuffer(ptpPortDescriptor.profileIdentifier.data(), ptpPortDescriptor.profileIdentifier.size());

		AVDECC_ASSERT(des.usedBytes() == AecpAemReadPtpPortDescriptorResponsePayloadSize, "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Remaining bytes in buffer for READ_PTP_PORT_DESCRIPTOR RESPONSE: {}", des.remaining());
		}
	}

	return ptpPortDescriptor;
}

/** WRITE_DESCRIPTOR Command - IEEE1722.1-2013 Clause 7.4.6.1 */

/** WRITE_DESCRIPTOR Response - IEEE1722.1-2013 Clause 7.4.6.1 */

/** ENTITY_AVAILABLE Command - IEEE1722.1-2013 Clause 7.4.3.1 */
// No payload

/** ENTITY_AVAILABLE Response - IEEE1722.1-2013 Clause 7.4.3.1 */
// No payload

/** CONTROLLER_AVAILABLE Command - IEEE1722.1-2013 Clause 7.4.4.1 */
// No payload

/** CONTROLLER_AVAILABLE Response - IEEE1722.1-2013 Clause 7.4.4.1 */
// No payload

/** SET_CONFIGURATION Command - IEEE1722.1-2013 Clause 7.4.7.1 */
Serializer<AecpAemSetConfigurationCommandPayloadSize> serializeSetConfigurationCommand(entity::model::ConfigurationIndex const configurationIndex)
{
	Serializer<AecpAemSetConfigurationCommandPayloadSize> ser;
	std::uint16_t const reserved{ 0u };

	ser << reserved << configurationIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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

	AVDECC_ASSERT(des.usedBytes() == AecpAemSetConfigurationCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(configurationIndex);
}

/** SET_CONFIGURATION Response - IEEE1722.1-2013 Clause 7.4.7.1 */
Serializer<AecpAemSetConfigurationResponsePayloadSize> serializeSetConfigurationResponse(entity::model::ConfigurationIndex const configurationIndex)
{
	// Same as SET_CONFIGURATION Command
	static_assert(AecpAemSetConfigurationResponsePayloadSize == AecpAemSetConfigurationCommandPayloadSize, "SET_CONFIGURATION Response no longer the same as SET_CONFIGURATION Command");
	return serializeSetConfigurationCommand(configurationIndex);
}

std::tuple<entity::model::ConfigurationIndex> deserializeSetConfigurationResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_CONFIGURATION Command
	static_assert(AecpAemSetConfigurationResponsePayloadSize == AecpAemSetConfigurationCommandPayloadSize, "SET_CONFIGURATION Response no longer the same as SET_CONFIGURATION Command");
	checkResponsePayload(payload, status, AecpAemSetConfigurationCommandPayloadSize, AecpAemSetConfigurationResponsePayloadSize);
	return deserializeSetConfigurationCommand(payload);
}

/** GET_CONFIGURATION Command - IEEE1722.1-2013 Clause 7.4.8.1 */
// No payload

/** GET_CONFIGURATION Response - IEEE1722.1-2013 Clause 7.4.8.2 */
Serializer<AecpAemGetConfigurationResponsePayloadSize> serializeGetConfigurationResponse(entity::model::ConfigurationIndex const configurationIndex)
{
	// Same as SET_CONFIGURATION Command
	static_assert(AecpAemGetConfigurationResponsePayloadSize == AecpAemSetConfigurationCommandPayloadSize, "GET_CONFIGURATION Response no longer the same as SET_CONFIGURATION Command");
	return serializeSetConfigurationCommand(configurationIndex);
}

std::tuple<entity::model::ConfigurationIndex> deserializeGetConfigurationResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_CONFIGURATION Command
	static_assert(AecpAemGetConfigurationResponsePayloadSize == AecpAemSetConfigurationCommandPayloadSize, "GET_CONFIGURATION Response no longer the same as SET_CONFIGURATION Command");
	checkResponsePayload(payload, status, AecpAemSetConfigurationCommandPayloadSize, AecpAemGetConfigurationResponsePayloadSize);
	return deserializeSetConfigurationCommand(payload);
}

/** SET_STREAM_FORMAT Command - IEEE1722.1-2013 Clause 7.4.9.1 */
Serializer<AecpAemSetStreamFormatCommandPayloadSize> serializeSetStreamFormatCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamFormat const streamFormat)
{
	Serializer<AecpAemSetStreamFormatCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;
	ser << streamFormat;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::StreamFormat streamFormat{};

	des >> descriptorType >> descriptorIndex;
	des >> streamFormat;

	AVDECC_ASSERT(des.usedBytes() == AecpAemSetStreamFormatCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, streamFormat);
}

/** SET_STREAM_FORMAT Response - IEEE1722.1-2013 Clause 7.4.9.1 */
Serializer<AecpAemSetStreamFormatResponsePayloadSize> serializeSetStreamFormatResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamFormat const streamFormat)
{
	// Same as SET_STREAM_FORMAT Command
	static_assert(AecpAemSetStreamFormatResponsePayloadSize == AecpAemSetStreamFormatCommandPayloadSize, "SET_STREAM_FORMAT Response no longer the same as SET_STREAM_FORMAT Command");
	return serializeSetStreamFormatCommand(descriptorType, descriptorIndex, streamFormat);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamFormat> deserializeSetStreamFormatResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_STREAM_FORMAT Command
	static_assert(AecpAemSetStreamFormatResponsePayloadSize == AecpAemSetStreamFormatCommandPayloadSize, "SET_STREAM_FORMAT Response no longer the same as SET_STREAM_FORMAT Command");
	checkResponsePayload(payload, status, AecpAemSetStreamFormatCommandPayloadSize, AecpAemSetStreamFormatResponsePayloadSize);
	return deserializeSetStreamFormatCommand(payload);
}

/** GET_STREAM_FORMAT Command - IEEE1722.1-2013 Clause 7.4.10.1 */
Serializer<AecpAemGetStreamFormatCommandPayloadSize> serializeGetStreamFormatCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemGetStreamFormatCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> descriptorType >> descriptorIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemGetStreamFormatCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}

/** GET_STREAM_FORMAT Response - IEEE1722.1-2013 Clause 7.4.10.2 */
Serializer<AecpAemGetStreamFormatResponsePayloadSize> serializeGetStreamFormatResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamFormat const streamFormat)
{
	// Same as SET_STREAM_FORMAT Command
	static_assert(AecpAemGetStreamFormatResponsePayloadSize == AecpAemSetStreamFormatCommandPayloadSize, "GET_STREAM_FORMAT Response no longer the same as SET_STREAM_FORMAT Command");
	return serializeSetStreamFormatCommand(descriptorType, descriptorIndex, streamFormat);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamFormat> deserializeGetStreamFormatResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_STREAM_FORMAT Command
	static_assert(AecpAemGetStreamFormatResponsePayloadSize == AecpAemSetStreamFormatCommandPayloadSize, "GET_STREAM_FORMAT Response no longer the same as SET_STREAM_FORMAT Command");
	checkResponsePayload(payload, status, AecpAemSetStreamFormatCommandPayloadSize, AecpAemGetStreamFormatResponsePayloadSize);
	return deserializeSetStreamFormatCommand(payload);
}

/** SET_STREAM_INFO Command - IEEE1722.1-2013 Clause 7.4.15.1 */
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

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
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

	AVDECC_ASSERT(des.usedBytes() == AecpAemSetStreamInfoCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, streamInfo);
}

/** SET_STREAM_INFO Response - IEEE1722.1-2013 Clause 7.4.15.1 */
Serializer<AecpAemSetStreamInfoResponsePayloadSize> serializeSetStreamInfoResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamInfo const& streamInfo)
{
	// Same as SET_STREAM_INFO Command
	static_assert(AecpAemSetStreamInfoResponsePayloadSize == AecpAemSetStreamInfoCommandPayloadSize, "SET_STREAM_INFO Response no longer the same as SET_STREAM_INFO Command");
	return serializeSetStreamInfoCommand(descriptorType, descriptorIndex, streamInfo);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamInfo> deserializeSetStreamInfoResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_STREAM_INFO Command
	static_assert(AecpAemSetStreamInfoResponsePayloadSize == AecpAemSetStreamInfoCommandPayloadSize, "SET_STREAM_INFO Response no longer the same as SET_STREAM_INFO Command");
	checkResponsePayload(payload, status, AecpAemSetStreamInfoCommandPayloadSize, AecpAemSetStreamInfoResponsePayloadSize);
	return deserializeSetStreamInfoCommand(payload);
}

/** GET_STREAM_INFO Command - IEEE1722.1-2013 Clause 7.4.16.1 */
Serializer<AecpAemGetStreamInfoCommandPayloadSize> serializeGetStreamInfoCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemGetStreamInfoCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> descriptorType >> descriptorIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemGetStreamInfoCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}


/** GET_STREAM_INFO Response - IEEE1722.1-2013 Clause 7.4.16.2 */
Serializer<AecpAemMilanGetStreamInfoResponsePayloadSize> serializeGetStreamInfoResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamInfo const& streamInfo)
{
	Serializer<AecpAemMilanGetStreamInfoResponsePayloadSize> ser;
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

	if (streamInfo.streamInfoFlagsEx && streamInfo.probingStatus && streamInfo.acmpStatus)
	{
		auto reserved3 = std::uint8_t{ 0u };
		auto reserved4 = std::uint16_t{ 0u };

		ser << *streamInfo.streamInfoFlagsEx << static_cast<std::uint8_t>(((utils::to_integral(*streamInfo.probingStatus) << 5) & 0xe0) | ((*streamInfo.acmpStatus).getValue() & 0x1f)) << reserved3 << reserved4;

		AVDECC_ASSERT(ser.usedBytes() == AecpAemMilanGetStreamInfoResponsePayloadSize, "Used bytes do not match the protocol constant");
	}
	else
	{
		AVDECC_ASSERT(ser.usedBytes() == AecpAemGetStreamInfoResponsePayloadSize, "Used bytes do not match the protocol constant");
	}

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamInfo> deserializeGetStreamInfoResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	checkResponsePayload(payload, status, AecpAemGetStreamInfoCommandPayloadSize, AecpAemGetStreamInfoResponsePayloadSize);

	// Check payload
	auto des = Deserializer{ commandPayload, commandPayloadLength };
	auto descriptorType = entity::model::DescriptorType{ entity::model::DescriptorType::Invalid };
	auto descriptorIndex = entity::model::DescriptorIndex{ 0u };
	auto streamInfo = entity::model::StreamInfo{};
	auto reserved = std::uint8_t{ 0u };
	auto reserved2 = std::uint16_t{ 0u };

	try
	{
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

		if (commandPayloadLength >= AecpAemMilanGetStreamInfoResponsePayloadSize)
		{
			auto streamInfoFlagsEx = entity::StreamInfoFlagsEx{};
			auto probing_acmp_status = std::uint8_t{ 0u };
			auto reserved3 = std::uint8_t{ 0u };
			auto reserved4 = std::uint16_t{ 0u };

			des >> streamInfoFlagsEx >> probing_acmp_status >> reserved3 >> reserved4;

			streamInfo.streamInfoFlagsEx = streamInfoFlagsEx;
			streamInfo.probingStatus = static_cast<entity::model::ProbingStatus>((probing_acmp_status & 0xe0) >> 5);
			streamInfo.acmpStatus = static_cast<AcmpStatus>(probing_acmp_status & 0x1f);

			AVDECC_ASSERT(des.usedBytes() == AecpAemMilanGetStreamInfoResponsePayloadSize, "Used more bytes than specified in protocol constant");
		}
		else
		{
			AVDECC_ASSERT(des.usedBytes() == AecpAemGetStreamInfoResponsePayloadSize, "Used more bytes than specified in protocol constant");
		}
	}
	catch (std::invalid_argument const&)
	{
		// Only NotImplemented status is allowed as we expect a reflected message (using Command length, so we cannot unpack everything)
		if (status != entity::LocalEntity::AemCommandStatus::NotImplemented)
		{
			throw; // Rethrow if we really have another status
		}
	}

	return std::make_tuple(descriptorType, descriptorIndex, streamInfo);
}

/** GET_AUDIO_MAP Command  - IEEE1722.1-2013 Clause 7.4.44.1 */

/** GET_AUDIO_MAP Response  - IEEE1722.1-2013 Clause 7.4.44.2 */

/** ADD_AUDIO_MAPPINGS Command  - IEEE1722.1-2013 Clause 7.4.45.1 */

/** ADD_AUDIO_MAPPINGS Response  - IEEE1722.1-2013 Clause 7.4.45.1 */

/** REMOVE_AUDIO_MAPPINGS Command  - IEEE1722.1-2013 Clause 7.4.46.1 */

/** REMOVE_AUDIO_MAPPINGS Response  - IEEE1722.1-2013 Clause 7.4.46.1 */

/** GET_STREAM_INFO Command - IEEE1722.1-2013 Clause 7.4.16.1 */

/** GET_STREAM_INFO Response - IEEE1722.1-2013 Clause 7.4.16.2 */

/** SET_NAME Command - IEEE1722.1-2013 Clause 7.4.17.1 */
Serializer<AecpAemSetNameCommandPayloadSize> serializeSetNameCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name)
{
	Serializer<AecpAemSetNameCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;
	ser << nameIndex << configurationIndex;
	ser << name;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	std::uint16_t nameIndex{ 0u };
	entity::model::ConfigurationIndex configurationIndex{ 0u };
	entity::model::AvdeccFixedString name{};

	des >> descriptorType >> descriptorIndex;
	des >> nameIndex >> configurationIndex >> name;

	AVDECC_ASSERT(des.usedBytes() == AecpAemSetNameCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, nameIndex, configurationIndex, name);
}

/** SET_NAME Response - IEEE1722.1-2013 Clause 7.4.17.1 */
Serializer<AecpAemSetNameResponsePayloadSize> serializeSetNameResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name)
{
	// Same as SET_NAME Command
	static_assert(AecpAemSetNameResponsePayloadSize == AecpAemSetNameCommandPayloadSize, "SET_NAME Response no longer the same as SET_NAME Command");
	return serializeSetNameCommand(descriptorType, descriptorIndex, nameIndex, configurationIndex, name);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex, entity::model::AvdeccFixedString> deserializeSetNameResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_NAME Command
	static_assert(AecpAemSetNameResponsePayloadSize == AecpAemSetNameCommandPayloadSize, "SET_NAME Response no longer the same as SET_NAME Command");
	checkResponsePayload(payload, status, AecpAemSetNameCommandPayloadSize, AecpAemSetNameResponsePayloadSize);
	return deserializeSetNameCommand(payload);
}

/** GET_NAME Command - IEEE1722.1-2013 Clause 7.4.18.1 */
Serializer<AecpAemGetNameCommandPayloadSize> serializeGetNameCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex)
{
	Serializer<AecpAemGetNameCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;
	ser << nameIndex << configurationIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	std::uint16_t nameIndex{ 0u };
	entity::model::ConfigurationIndex configurationIndex{ 0u };

	des >> descriptorType >> descriptorIndex;
	des >> nameIndex >> configurationIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemGetNameCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, nameIndex, configurationIndex);
}

/** GET_NAME Response - IEEE1722.1-2013 Clause 7.4.18.2 */
Serializer<AecpAemGetNameResponsePayloadSize> serializeGetNameResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name)
{
	// Same as SET_NAME Command
	static_assert(AecpAemGetNameResponsePayloadSize == AecpAemSetNameCommandPayloadSize, "GET_NAME Response no longer the same as SET_NAME Command");
	return serializeSetNameCommand(descriptorType, descriptorIndex, nameIndex, configurationIndex, name);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex, entity::model::AvdeccFixedString> deserializeGetNameResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_NAME Command
	static_assert(AecpAemGetNameResponsePayloadSize == AecpAemSetNameCommandPayloadSize, "GET_NAME Response no longer the same as SET_NAME Command");
	checkResponsePayload(payload, status, AecpAemSetNameCommandPayloadSize, AecpAemGetNameResponsePayloadSize);
	return deserializeSetNameCommand(payload);
}

/** SET_ASSOCIATION_ID Command - IEEE1722.1-2013 Clause 7.4.19.1 */
Serializer<AecpAemSetAssociationIDCommandPayloadSize> serializeSetAssociationIDCommand(UniqueIdentifier const associationID)
{
	Serializer<AecpAemSetAssociationIDCommandPayloadSize> ser;

	ser << associationID;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<UniqueIdentifier> deserializeSetAssociationIDCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemSetAssociationIDCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	UniqueIdentifier associationID{};

	des >> associationID;

	AVDECC_ASSERT(des.usedBytes() == AecpAemSetAssociationIDCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(associationID);
}

/** SET_ASSOCIATION_ID Response - IEEE1722.1-2013 Clause 7.4.19.1 */
Serializer<AecpAemSetAssociationIDResponsePayloadSize> serializeSetAssociationIDResponse(UniqueIdentifier const associationID)
{
	// Same as SET_ASSOCIATION_ID Command
	static_assert(AecpAemSetAssociationIDResponsePayloadSize == AecpAemSetAssociationIDCommandPayloadSize, "SET_ASSOCIATION_ID Response no longer the same as SET_ASSOCIATION_ID Command");
	return serializeSetAssociationIDCommand(associationID);
}

std::tuple<UniqueIdentifier> deserializeSetAssociationIDResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_ASSOCIATION_ID Command
	static_assert(AecpAemSetAssociationIDResponsePayloadSize == AecpAemSetAssociationIDCommandPayloadSize, "SET_ASSOCIATION_ID Response no longer the same as SET_ASSOCIATION_ID Command");
	checkResponsePayload(payload, status, AecpAemSetAssociationIDCommandPayloadSize, AecpAemSetAssociationIDResponsePayloadSize);
	return deserializeSetAssociationIDCommand(payload);
}

/** GET_ASSOCIATION_ID Command - IEEE1722.1-2013 Clause 7.4.20.1 */
// No payload

/** GET_ASSOCIATION_ID Response - IEEE1722.1-2013 Clause 7.4.20.2 */
Serializer<AecpAemGetAssociationIDResponsePayloadSize> serializeGetAssociationIDResponse(UniqueIdentifier const associationID)
{
	// Same as GET_ASSOCIATION_ID Command
	static_assert(AecpAemGetAssociationIDResponsePayloadSize == AecpAemSetAssociationIDCommandPayloadSize, "GET_ASSOCIATION_ID Response no longer the same as SET_ASSOCIATION_ID Command");
	return serializeSetAssociationIDCommand(associationID);
}

std::tuple<UniqueIdentifier> deserializeGetAssociationIDResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as GET_ASSOCIATION_ID Command
	static_assert(AecpAemGetAssociationIDResponsePayloadSize == AecpAemSetAssociationIDCommandPayloadSize, "GET_ASSOCIATION_ID Response no longer the same as SET_ASSOCIATION_ID Command");
	checkResponsePayload(payload, status, AecpAemSetAssociationIDCommandPayloadSize, AecpAemGetAssociationIDResponsePayloadSize);
	return deserializeSetAssociationIDCommand(payload);
}

/** SET_SAMPLING_RATE Command - IEEE1722.1-2013 Clause 7.4.21.1 */
Serializer<AecpAemSetSamplingRateCommandPayloadSize> serializeSetSamplingRateCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::SamplingRate const samplingRate)
{
	Serializer<AecpAemSetSamplingRateCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;
	ser << samplingRate;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::SamplingRate samplingRate{};

	des >> descriptorType >> descriptorIndex;
	des >> samplingRate;

	AVDECC_ASSERT(des.usedBytes() == AecpAemSetSamplingRateCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, samplingRate);
}

/** SET_SAMPLING_RATE Response - IEEE1722.1-2013 Clause 7.4.21.1 */
Serializer<AecpAemSetSamplingRateResponsePayloadSize> serializeSetSamplingRateResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::SamplingRate const samplingRate)
{
	// Same as SET_SAMPLING_RATE Command
	static_assert(AecpAemSetSamplingRateResponsePayloadSize == AecpAemSetSamplingRateCommandPayloadSize, "SET_SAMPLING_RATE Response no longer the same as SET_SAMPLING_RATE Command");
	return serializeSetSamplingRateCommand(descriptorType, descriptorIndex, samplingRate);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::SamplingRate> deserializeSetSamplingRateResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_SAMPLING_RATE Command
	static_assert(AecpAemSetSamplingRateResponsePayloadSize == AecpAemSetSamplingRateCommandPayloadSize, "SET_SAMPLING_RATE Response no longer the same as SET_SAMPLING_RATE Command");
	checkResponsePayload(payload, status, AecpAemSetSamplingRateCommandPayloadSize, AecpAemSetSamplingRateResponsePayloadSize);
	return deserializeSetSamplingRateCommand(payload);
}

/** GET_SAMPLING_RATE Command - IEEE1722.1-2013 Clause 7.4.22.1 */
Serializer<AecpAemGetSamplingRateCommandPayloadSize> serializeGetSamplingRateCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemGetSamplingRateCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> descriptorType >> descriptorIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemGetSamplingRateCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}

/** GET_SAMPLING_RATE Response - IEEE1722.1-2013 Clause 7.4.22.2 */
Serializer<AecpAemGetSamplingRateResponsePayloadSize> serializeGetSamplingRateResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::SamplingRate const samplingRate)
{
	// Same as SET_SAMPLING_RATE Command
	static_assert(AecpAemGetSamplingRateResponsePayloadSize == AecpAemSetSamplingRateCommandPayloadSize, "GET_SAMPLING_RATE Response no longer the same as SET_SAMPLING_RATE Command");
	return serializeSetSamplingRateCommand(descriptorType, descriptorIndex, samplingRate);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::SamplingRate> deserializeGetSamplingRateResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_SAMPLING_RATE Command
	static_assert(AecpAemGetSamplingRateResponsePayloadSize == AecpAemSetSamplingRateCommandPayloadSize, "GET_SAMPLING_RATE Response no longer the same as SET_SAMPLING_RATE Command");
	checkResponsePayload(payload, status, AecpAemSetSamplingRateCommandPayloadSize, AecpAemGetSamplingRateResponsePayloadSize);
	return deserializeSetSamplingRateCommand(payload);
}

/** SET_CLOCK_SOURCE Command - IEEE1722.1-2013 Clause 7.4.23.1 */
Serializer<AecpAemSetClockSourceCommandPayloadSize> serializeSetClockSourceCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ClockSourceIndex const clockSourceIndex)
{
	Serializer<AecpAemSetClockSourceCommandPayloadSize> ser;
	std::uint16_t const reserved{ 0u };

	ser << descriptorType << descriptorIndex;
	ser << clockSourceIndex << reserved;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::ClockSourceIndex clockSourceIndex{ 0u };
	std::uint16_t reserved{ 0u };

	des >> descriptorType >> descriptorIndex;
	des >> clockSourceIndex >> reserved;

	AVDECC_ASSERT(des.usedBytes() == AecpAemSetClockSourceCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, clockSourceIndex);
}

/** SET_CLOCK_SOURCE Response - IEEE1722.1-2013 Clause 7.4.23.1 */
Serializer<AecpAemSetClockSourceResponsePayloadSize> serializeSetClockSourceResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ClockSourceIndex const clockSourceIndex)
{
	// Same as SET_CLOCK_SOURCE Command
	static_assert(AecpAemSetClockSourceResponsePayloadSize == AecpAemSetClockSourceCommandPayloadSize, "SET_CLOCK_SOURCE Response no longer the same as SET_CLOCK_SOURCE Command");
	return serializeSetClockSourceCommand(descriptorType, descriptorIndex, clockSourceIndex);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::ClockSourceIndex> deserializeSetClockSourceResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_CLOCK_SOURCE Command
	static_assert(AecpAemSetClockSourceResponsePayloadSize == AecpAemSetClockSourceCommandPayloadSize, "SET_CLOCK_SOURCE Response no longer the same as SET_CLOCK_SOURCE Command");
	checkResponsePayload(payload, status, AecpAemSetClockSourceCommandPayloadSize, AecpAemSetClockSourceResponsePayloadSize);
	return deserializeSetClockSourceCommand(payload);
}

/** GET_CLOCK_SOURCE Command - IEEE1722.1-2013 Clause 7.4.24.1 */
Serializer<AecpAemGetClockSourceCommandPayloadSize> serializeGetClockSourceCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemGetClockSourceCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> descriptorType >> descriptorIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemGetClockSourceCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}

/** GET_CLOCK_SOURCE Response - IEEE1722.1-2013 Clause 7.4.24.2 */
Serializer<AecpAemGetClockSourceResponsePayloadSize> serializeGetClockSourceResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ClockSourceIndex const clockSourceIndex)
{
	// Same as SET_CLOCK_SOURCE Command
	static_assert(AecpAemGetClockSourceResponsePayloadSize == AecpAemSetClockSourceCommandPayloadSize, "GET_CLOCK_SOURCE Response no longer the same as SET_CLOCK_SOURCE Command");
	return serializeSetClockSourceCommand(descriptorType, descriptorIndex, clockSourceIndex);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::ClockSourceIndex> deserializeGetClockSourceResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_CLOCK_SOURCE Command
	static_assert(AecpAemGetClockSourceResponsePayloadSize == AecpAemSetClockSourceCommandPayloadSize, "GET_CLOCK_SOURCE Response no longer the same as SET_CLOCK_SOURCE Command");
	checkResponsePayload(payload, status, AecpAemSetClockSourceCommandPayloadSize, AecpAemGetClockSourceResponsePayloadSize);
	return deserializeSetClockSourceCommand(payload);
}

/** SET_CONTROL Command - IEEE1722.1-2013 Clause 7.4.25.1 */
static inline void createPackDynamicControlValuesDispatchTable(std::unordered_map<entity::model::ControlValueType::Type, std::function<void(Serializer<AemAecpdu::MaximumSendPayloadBufferLength>&, entity::model::ControlValues const&)>>& dispatchTable)
{
	/** Linear Values - IEEE1722.1-2013 Clause 7.3.5.2.1 */
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearInt8] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt8>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearUInt8] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt8>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearInt16] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt16>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearUInt16] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt16>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearInt32] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt32>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearUInt32] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt32>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearInt64] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt64>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearUInt64] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt64>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearFloat] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearFloat>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearDouble] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearDouble>::packDynamicControlValues;

	/** Selector Value - IEEE1722.1-2013 Clause 7.3.5.2.2 */
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorInt8] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt8>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorUInt8] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt8>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorInt16] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt16>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorUInt16] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt16>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorInt32] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt32>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorUInt32] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt32>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorInt64] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt64>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorUInt64] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt64>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorFloat] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorFloat>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorDouble] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorDouble>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorString] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorString>::packDynamicControlValues;

	/** Array Values - IEEE1722.1-2013 Clause 7.3.5.2.3 */
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayInt8] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt8>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayUInt8] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt8>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayInt16] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt16>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayUInt16] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt16>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayInt32] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt32>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayUInt32] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt32>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayInt64] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt64>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayUInt64] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt64>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayFloat] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayFloat>::packDynamicControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayDouble] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayDouble>::packDynamicControlValues;

	/** UTF-8 String Value - IEEE1722.1-2013 Clause 7.3.5.2.4 */
	dispatchTable[entity::model::ControlValueType::Type::ControlUtf8] = control_values_payload_traits<entity::model::ControlValueType::Type::ControlUtf8>::packDynamicControlValues;
}

Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeSetControlCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ControlValues const& controlValues)
{
	static auto s_Dispatch = std::unordered_map<entity::model::ControlValueType::Type, std::function<void(Serializer<AemAecpdu::MaximumSendPayloadBufferLength>&, entity::model::ControlValues const&)>>{};

	if (s_Dispatch.empty())
	{
		// Create the dispatch table
		createPackDynamicControlValuesDispatchTable(s_Dispatch);
	}

	Serializer<AemAecpdu::MaximumSendPayloadBufferLength> ser;
	ser << descriptorType << descriptorIndex;

	// Serialize variable data
	if (!controlValues.empty())
	{
		auto const valueType = controlValues.getType();
		if (auto const& it = s_Dispatch.find(valueType); it != s_Dispatch.end())
		{
			it->second(ser, controlValues);
		}
		else
		{
			LOG_AEM_PAYLOAD_TRACE("serializeSetControlCommand warning: Unsupported ControlValueType: {}", valueType);
			throw UnsupportedValueException();
		}
	}

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, MemoryBuffer> deserializeSetControlCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemSetControlCommandPayloadMinSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	MemoryBuffer memoryBuffer{};

	des >> descriptorType >> descriptorIndex;

	// Unpack remaining data
	auto const remaining = des.remaining();
	if (remaining != 0)
	{
		memoryBuffer.set_size(remaining);
		des >> memoryBuffer;
	}

	return std::make_tuple(descriptorType, descriptorIndex, memoryBuffer);
}

/** SET_CONTROL Response - IEEE1722.1-2013 Clause 7.4.25.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeSetControlResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ControlValues const& controlValues)
{
	// Same as SET_CONTROL Command
	static_assert(AecpAemSetControlResponsePayloadMinSize == AecpAemSetControlCommandPayloadMinSize, "SET_CONTROL Response no longer the same as SET_CONTROL Command");
	return serializeSetControlCommand(descriptorType, descriptorIndex, controlValues);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, MemoryBuffer> deserializeSetControlResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_CONTROL Command
	static_assert(AecpAemSetControlResponsePayloadMinSize == AecpAemSetControlCommandPayloadMinSize, "SET_CONTROL Response no longer the same as SET_CONTROL Command");
	checkResponsePayload(payload, status, AecpAemSetControlCommandPayloadMinSize, AecpAemSetControlResponsePayloadMinSize);
	return deserializeSetControlCommand(payload);
}

/** GET_CONTROL Command - IEEE1722.1-2013 Clause 7.4.26.1 */
Serializer<AecpAemGetControlCommandPayloadSize> serializeGetControlCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemGetControlCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeGetControlCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetControlCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> descriptorType >> descriptorIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemGetControlCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}

/** GET_CONTROL Response - IEEE1722.1-2013 Clause 7.4.26.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeGetControlResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ControlValues const& controlValues)
{
	// Same as SET_CONTROL Command
	static_assert(AecpAemGetControlResponsePayloadMinSize == AecpAemSetControlCommandPayloadMinSize, "GET_CONTROL Response no longer the same as SET_CONTROL Command");
	return serializeSetControlCommand(descriptorType, descriptorIndex, controlValues);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, MemoryBuffer> deserializeGetControlResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_CONTROL Command
	static_assert(AecpAemGetControlResponsePayloadMinSize == AecpAemSetControlCommandPayloadMinSize, "GET_CONTROL Response no longer the same as SET_CONTROL Command");
	checkResponsePayload(payload, status, AecpAemSetControlCommandPayloadMinSize, AecpAemGetControlResponsePayloadMinSize);
	return deserializeSetControlCommand(payload);
}

/** START_STREAMING Command - IEEE1722.1-2013 Clause 7.4.35.1 */
Serializer<AecpAemStartStreamingCommandPayloadSize> serializeStartStreamingCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemStartStreamingCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> descriptorType >> descriptorIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemStartStreamingCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}

/** START_STREAMING Response - IEEE1722.1-2013 Clause 7.4.35.1 */
Serializer<AecpAemStartStreamingResponsePayloadSize> serializeStartStreamingResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	// Same as START_STREAMING Command
	static_assert(AecpAemStartStreamingResponsePayloadSize == AecpAemStartStreamingCommandPayloadSize, "START_STREAMING Response no longer the same as START_STREAMING Command");
	return serializeStartStreamingCommand(descriptorType, descriptorIndex);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeStartStreamingResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as START_STREAMING Command
	static_assert(AecpAemStartStreamingResponsePayloadSize == AecpAemStartStreamingCommandPayloadSize, "START_STREAMING Response no longer the same as START_STREAMING Command");
	checkResponsePayload(payload, status, AecpAemStartStreamingCommandPayloadSize, AecpAemStartStreamingResponsePayloadSize);
	return deserializeStartStreamingCommand(payload);
}

/** STOP_STREAMING Command - IEEE1722.1-2013 Clause 7.4.36.1 */
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

/** STOP_STREAMING Response - IEEE1722.1-2013 Clause 7.4.36.1 */
Serializer<AecpAemStopStreamingResponsePayloadSize> serializeStopStreamingResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	// Same as START_STREAMING Command
	static_assert(AecpAemStopStreamingResponsePayloadSize == AecpAemStartStreamingCommandPayloadSize, "STOP_STREAMING Response no longer the same as START_STREAMING Command");
	return serializeStartStreamingCommand(descriptorType, descriptorIndex);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeStopStreamingResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as START_STREAMING Command
	static_assert(AecpAemStopStreamingResponsePayloadSize == AecpAemStartStreamingCommandPayloadSize, "STOP_STREAMING Response no longer the same as START_STREAMING Command");
	checkResponsePayload(payload, status, AecpAemStartStreamingCommandPayloadSize, AecpAemStopStreamingResponsePayloadSize);
	return deserializeStartStreamingCommand(payload);
}

/** GET_AVB_INFO Command - IEEE1722.1-2013 Clause 7.4.40.1 */
Serializer<AecpAemGetAvbInfoCommandPayloadSize> serializeGetAvbInfoCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemGetAvbInfoCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeGetAvbInfoCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetAvbInfoCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> descriptorType >> descriptorIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemGetAvbInfoCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}

/** GET_AVB_INFO Response - IEEE1722.1-2013 Clause 7.4.40.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeGetAvbInfoResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AvbInfo const& avbInfo)
{
	Serializer<AemAecpdu::MaximumSendPayloadBufferLength> ser;

	ser << descriptorType << descriptorIndex;
	ser << avbInfo.gptpGrandmasterID << avbInfo.propagationDelay << avbInfo.gptpDomainNumber << avbInfo.flags << static_cast<std::uint16_t>(avbInfo.mappings.size());

	// Serialize variable data
	for (auto const& mapping : avbInfo.mappings)
	{
		ser << mapping.trafficClass << mapping.priority << mapping.vlanID;
	}

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::AvbInfo> deserializeGetAvbInfoResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	checkResponsePayload(payload, status, AecpAemGetAvbInfoCommandPayloadSize, AecpAemGetAvbInfoResponsePayloadMinSize);

	// Check payload
	auto des = Deserializer{ commandPayload, commandPayloadLength };
	auto descriptorType = entity::model::DescriptorType{ entity::model::DescriptorType::Invalid };
	auto descriptorIndex = entity::model::DescriptorIndex{ 0u };
	auto avbInfo = entity::model::AvbInfo{};
	auto numberOfMappings = std::uint16_t{ 0u };

	try
	{
		des >> descriptorType >> descriptorIndex;
		des >> avbInfo.gptpGrandmasterID >> avbInfo.propagationDelay >> avbInfo.gptpDomainNumber >> avbInfo.flags >> numberOfMappings;

		// Check variable size
		auto const mappingsSize = entity::model::MsrpMapping::size() * numberOfMappings;
		if (des.remaining() < mappingsSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Unpack remaining data
		entity::model::MsrpMappings mappings;
		for (auto index = 0u; index < numberOfMappings; ++index)
		{
			entity::model::MsrpMapping mapping;
			des >> mapping.trafficClass >> mapping.priority >> mapping.vlanID;
			mappings.push_back(mapping);
		}
		AVDECC_ASSERT(des.usedBytes() == (protocol::aemPayload::AecpAemGetAvbInfoResponsePayloadMinSize + mappingsSize), "Used more bytes than specified in protocol constant");
		avbInfo.mappings = std::move(mappings);

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("GetAvbInfo Response deserialize warning: Remaining bytes in buffer");
		}
	}
	catch (std::invalid_argument const&)
	{
		// Only NotImplemented status is allowed as we expect a reflected message (using Command length, so we cannot unpack everything)
		if (status != entity::LocalEntity::AemCommandStatus::NotImplemented)
		{
			throw; // Rethrow if we really have another status
		}
	}

	return std::make_tuple(descriptorType, descriptorIndex, avbInfo);
}

/** GET_AS_PATH Command - IEEE1722.1-2013 Clause 7.4.41.1 */
Serializer<AecpAemGetAsPathCommandPayloadSize> serializeGetAsPathCommand(entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemGetAsPathCommandPayloadSize> ser;
	std::uint16_t const reserved{ 0u };

	ser << descriptorIndex << reserved;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorIndex> deserializeGetAsPathCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetAsPathCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	std::uint16_t reserved{ 0u };

	des >> descriptorIndex >> reserved;

	AVDECC_ASSERT(des.usedBytes() == AecpAemGetAsPathCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorIndex);
}

/** GET_AS_PATH Response - IEEE1722.1-2013 Clause 7.4.41.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeGetAsPathResponse(entity::model::DescriptorIndex const descriptorIndex, entity::model::AsPath const& asPath)
{
	Serializer<AemAecpdu::MaximumSendPayloadBufferLength> ser;

	ser << descriptorIndex << static_cast<std::uint16_t>(asPath.sequence.size());

	// Serialize variable data
	for (auto const& clockIdentity : asPath.sequence)
	{
		ser << clockIdentity;
	}

	return ser;
}

std::tuple<entity::model::DescriptorIndex, entity::model::AsPath> deserializeGetAsPathResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	checkResponsePayload(payload, status, AecpAemGetAsPathCommandPayloadSize, AecpAemGetAsPathResponsePayloadMinSize);

	// Check payload
	auto des = Deserializer{ commandPayload, commandPayloadLength };
	auto descriptorIndex = entity::model::DescriptorIndex{ 0u };
	auto asPath = entity::model::AsPath{};
	auto count = std::uint16_t{ 0u };

	try
	{
		des >> descriptorIndex >> count;

		// Check variable size
		auto const sequenceSize = sizeof(UniqueIdentifier::value_type) * count;
		if (des.remaining() < sequenceSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Unpack remaining data
		for (auto index = 0u; index < count; ++index)
		{
			auto clockIdentity = decltype(asPath.sequence)::value_type{};
			des >> clockIdentity;
			asPath.sequence.push_back(clockIdentity);
		}
		AVDECC_ASSERT(des.usedBytes() == (protocol::aemPayload::AecpAemGetAsPathResponsePayloadMinSize + sequenceSize), "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("GetAsPath Response deserialize warning: Remaining bytes in buffer");
		}
	}
	catch (std::invalid_argument const&)
	{
		// Only NotImplemented status is allowed as we expect a reflected message (using Command length, so we cannot unpack everything)
		if (status != entity::LocalEntity::AemCommandStatus::NotImplemented)
		{
			throw; // Rethrow if we really have another status
		}
	}

	return std::make_tuple(descriptorIndex, asPath);
}

/** GET_COUNTERS Command - IEEE1722.1-2013 Clause 7.4.42.1 */
Serializer<AecpAemGetCountersCommandPayloadSize> serializeGetCountersCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemGetCountersCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeGetCountersCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetCountersCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> descriptorType >> descriptorIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemGetCountersCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}

/** GET_COUNTERS Response - IEEE1722.1-2013 Clause 7.4.42.2 */
Serializer<AecpAemGetCountersResponsePayloadSize> serializeGetCountersResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::DescriptorCounterValidFlag const validCounters, entity::model::DescriptorCounters const& counters)
{
	Serializer<AecpAemGetCountersResponsePayloadSize> ser;

	ser << descriptorType << descriptorIndex;
	ser << validCounters;

	// Serialize the counters
	for (auto const counter : counters)
	{
		ser << counter;
	}

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::DescriptorCounterValidFlag, entity::model::DescriptorCounters> deserializeGetCountersResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	checkResponsePayload(payload, status, AecpAemGetCountersCommandPayloadSize, AecpAemGetCountersResponsePayloadSize);

	// Check payload
	auto des = Deserializer{ commandPayload, commandPayloadLength };
	auto descriptorType = entity::model::DescriptorType{ entity::model::DescriptorType::Invalid };
	auto descriptorIndex = entity::model::DescriptorIndex{ 0u };
	auto validCounters = entity::model::DescriptorCounterValidFlag{ 0u };
	auto counters = entity::model::DescriptorCounters{};
	try
	{
		des >> descriptorType >> descriptorIndex;
		des >> validCounters;

		// Deserialize the counters
		for (auto& counter : counters)
		{
			des >> counter;
		}

		AVDECC_ASSERT(des.usedBytes() == AecpAemGetCountersResponsePayloadSize, "Used more bytes than specified in protocol constant");
	}
	catch (std::invalid_argument const&)
	{
		// Only NotImplemented status is allowed as we expect a reflected message (using Command length, so we cannot unpack everything)
		if (status != entity::LocalEntity::AemCommandStatus::NotImplemented)
		{
			throw; // Rethrow if we really have another status
		}
	}

	return std::make_tuple(descriptorType, descriptorIndex, validCounters, counters);
}

/** REBOOT Command - IEEE1722.1-2013 Clause 7.4.43.1 */
Serializer<AecpAemRebootCommandPayloadSize> serializeRebootCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	Serializer<AecpAemRebootCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeRebootCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemRebootCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	des >> descriptorType >> descriptorIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemRebootCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}

/** REBOOT Response - IEEE1722.1-2013 Clause 7.4.43.1 */
Serializer<AecpAemRebootResponsePayloadSize> serializeRebootResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	// Same as REBOOT Command
	static_assert(AecpAemRebootResponsePayloadSize == AecpAemRebootCommandPayloadSize, "REBOOT Response no longer the same as REBOOT Command");
	return serializeRebootCommand(descriptorType, descriptorIndex);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeRebootResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as REBOOT Command
	static_assert(AecpAemRebootResponsePayloadSize == AecpAemRebootCommandPayloadSize, "REBOOT Response no longer the same as REBOOT Command");
	checkResponsePayload(payload, status, AecpAemRebootCommandPayloadSize, AecpAemRebootResponsePayloadSize);
	return deserializeRebootCommand(payload);
}

/** GET_AUDIO_MAP Command - IEEE1722.1-2013 Clause 7.4.44.1 */
Serializer<AecpAemGetAudioMapCommandPayloadSize> serializeGetAudioMapCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::MapIndex const mapIndex)
{
	Serializer<AecpAemGetAudioMapCommandPayloadSize> ser;
	std::uint16_t const reserved{ 0u };

	ser << descriptorType << descriptorIndex;
	ser << mapIndex << reserved;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

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
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::MapIndex mapIndex{ 0u };
	std::uint16_t reserved{ 0u };

	des >> descriptorType >> descriptorIndex;
	des >> mapIndex >> reserved;

	AVDECC_ASSERT(des.usedBytes() == AecpAemGetAudioMapCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, mapIndex);
}

/** GET_AUDIO_MAP Response - IEEE1722.1-2013 Clause 7.4.44.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeGetAudioMapResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::MapIndex const mapIndex, entity::model::MapIndex const numberOfMaps, entity::model::AudioMappings const& mappings)
{
	Serializer<AemAecpdu::MaximumSendPayloadBufferLength> ser;
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

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::MapIndex, entity::model::MapIndex, entity::model::AudioMappings> deserializeGetAudioMapResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	checkResponsePayload(payload, status, AecpAemGetAudioMapCommandPayloadSize, AecpAemGetAudioMapResponsePayloadMinSize);

	// Check payload
	auto des = Deserializer{ commandPayload, commandPayloadLength };
	auto descriptorType = entity::model::DescriptorType{ entity::model::DescriptorType::Invalid };
	auto descriptorIndex = entity::model::DescriptorIndex{ 0u };
	auto mapIndex = entity::model::MapIndex{ 0u };
	auto numberOfMaps = entity::model::MapIndex{ 0u };
	auto numberOfMappings = std::uint16_t{ 0u };
	auto reserved = std::uint16_t{ 0u };
	auto mappings = entity::model::AudioMappings{};

	try
	{
		des >> descriptorType >> descriptorIndex;
		des >> mapIndex >> numberOfMaps >> numberOfMappings >> reserved;

		// Check variable size
		auto const mappingsSize = entity::model::AudioMapping::size() * numberOfMappings;
		if (des.remaining() < mappingsSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Unpack remaining data
		for (auto index = 0u; index < numberOfMappings; ++index)
		{
			entity::model::AudioMapping mapping;
			des >> mapping.streamIndex >> mapping.streamChannel >> mapping.clusterOffset >> mapping.clusterChannel;
			mappings.push_back(mapping);
		}
		AVDECC_ASSERT(des.usedBytes() == (protocol::aemPayload::AecpAemGetAudioMapResponsePayloadMinSize + mappingsSize), "Used more bytes than specified in protocol constant");

		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_TRACE("GetAudioMap Response deserialize warning: Remaining bytes in buffer");
		}
	}
	catch (std::invalid_argument const&)
	{
		// Only NotImplemented status is allowed as we expect a reflected message (using Command length, so we cannot unpack everything)
		if (status != entity::LocalEntity::AemCommandStatus::NotImplemented)
		{
			throw; // Rethrow if we really have another status
		}
	}

	return std::make_tuple(descriptorType, descriptorIndex, mapIndex, numberOfMaps, mappings);
}

/** ADD_AUDIO_MAPPINGS Command - IEEE1722.1-2013 Clause 7.4.45.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeAddAudioMappingsCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AudioMappings const& mappings)
{
	Serializer<AemAecpdu::MaximumSendPayloadBufferLength> ser;
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
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
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
	AVDECC_ASSERT(des.usedBytes() == (protocol::aemPayload::AecpAemAddAudioMappingsCommandPayloadMinSize + mappingsSize), "Used more bytes than specified in protocol constant");

	if (des.remaining() != 0)
	{
		LOG_AEM_PAYLOAD_TRACE("AddAudioMap (or RemoveAudioMap) Command (or Response) deserialize warning: Remaining bytes in buffer");
	}

	return std::make_tuple(descriptorType, descriptorIndex, mappings);
}

/** ADD_AUDIO_MAPPINGS Response - IEEE1722.1-2013 Clause 7.4.45.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeAddAudioMappingsResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AudioMappings const& mappings)
{
	// Same as ADD_AUDIO_MAPPINGS Command
	static_assert(AecpAemAddAudioMappingsResponsePayloadMinSize == AecpAemAddAudioMappingsCommandPayloadMinSize, "ADD_AUDIO_MAPPINGS Response no longer the same as ADD_AUDIO_MAPPINGS Command");
	return serializeAddAudioMappingsCommand(descriptorType, descriptorIndex, mappings);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::AudioMappings> deserializeAddAudioMappingsResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as ADD_AUDIO_MAPPINGS Command
	static_assert(AecpAemAddAudioMappingsResponsePayloadMinSize == AecpAemAddAudioMappingsCommandPayloadMinSize, "ADD_AUDIO_MAPPINGS Response no longer the same as ADD_AUDIO_MAPPINGS Command");
	checkResponsePayload(payload, status, AecpAemAddAudioMappingsCommandPayloadMinSize, AecpAemAddAudioMappingsResponsePayloadMinSize);
	return deserializeAddAudioMappingsCommand(payload);
}

/** REMOVE_AUDIO_MAPPINGS Command - IEEE1722.1-2013 Clause 7.4.46.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeRemoveAudioMappingsCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AudioMappings const& mappings)
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

/** REMOVE_AUDIO_MAPPINGS Response - IEEE1722.1-2013 Clause 7.4.46.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeRemoveAudioMappingsResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AudioMappings const& mappings)
{
	// Same as ADD_AUDIO_MAPPINGS Command
	static_assert(AecpAemRemoveAudioMappingsResponsePayloadMinSize == AecpAemAddAudioMappingsCommandPayloadMinSize, "REMOVE_AUDIO_MAPPINGS Response no longer the same as ADD_AUDIO_MAPPINGS Command");
	return serializeAddAudioMappingsCommand(descriptorType, descriptorIndex, mappings);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::AudioMappings> deserializeRemoveAudioMappingsResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as ADD_AUDIO_MAPPINGS Command
	static_assert(AecpAemRemoveAudioMappingsResponsePayloadMinSize == AecpAemAddAudioMappingsCommandPayloadMinSize, "REMOVE_AUDIO_MAPPINGS Response no longer the same as ADD_AUDIO_MAPPINGS Command");
	checkResponsePayload(payload, status, AecpAemAddAudioMappingsCommandPayloadMinSize, AecpAemRemoveAudioMappingsResponsePayloadMinSize);
	return deserializeAddAudioMappingsCommand(payload);
}

/** START_OPERATION Command - IEEE1722.1-2013 Clause 7.4.53.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeStartOperationCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, entity::model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer)
{
	Serializer<AemAecpdu::MaximumSendPayloadBufferLength> ser;

	ser << descriptorType << descriptorIndex;
	ser << operationID << operationType;

	AVDECC_ASSERT(ser.usedBytes() == AecpAemStartOperationCommandPayloadMinSize, "Used bytes do not match the protocol constant");

	// Serialize variable data
	if (!memoryBuffer.empty())
	{
		ser << memoryBuffer;
	}

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::OperationID, entity::model::MemoryObjectOperationType, MemoryBuffer> deserializeStartOperationCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemStartOperationCommandPayloadMinSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::OperationID operationID{ 0u };
	entity::model::MemoryObjectOperationType operationType{ 0u };
	MemoryBuffer memoryBuffer{};

	des >> descriptorType >> descriptorIndex;
	des >> operationID >> operationType;

	// Unpack remaining data
	auto const remaining = des.remaining();
	if (remaining != 0)
	{
		memoryBuffer.set_size(remaining);
		des >> memoryBuffer;
	}

	return std::make_tuple(descriptorType, descriptorIndex, operationID, operationType, memoryBuffer);
}

/** START_OPERATION Response - IEEE1722.1-2013 Clause 7.4.53.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeStartOperationResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, entity::model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer)
{
	// Same as START_OPERATION Command
	static_assert(AecpAemStartOperationResponsePayloadMinSize == AecpAemStartOperationCommandPayloadMinSize, "START_OPERATION Response no longer the same as START_OPERATION Command");
	return serializeStartOperationCommand(descriptorType, descriptorIndex, operationID, operationType, memoryBuffer);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::OperationID, entity::model::MemoryObjectOperationType, MemoryBuffer> deserializeStartOperationResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as START_OPERATION Command
	static_assert(AecpAemStartOperationResponsePayloadMinSize == AecpAemStartOperationCommandPayloadMinSize, "START_OPERATION Response no longer the same as START_OPERATION Command");
	checkResponsePayload(payload, status, AecpAemStartOperationCommandPayloadMinSize, AecpAemStartOperationResponsePayloadMinSize);
	return deserializeStartOperationCommand(payload);
}

/** ABORT_OPERATION Command - IEEE1722.1-2013 Clause 7.4.55.1 */
Serializer<AecpAemAbortOperationCommandPayloadSize> serializeAbortOperationCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID)
{
	Serializer<AecpAemAbortOperationCommandPayloadSize> ser;
	std::uint16_t const reserved{ 0u };

	ser << descriptorType << descriptorIndex;
	ser << operationID << reserved;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::OperationID> deserializeAbortOperationCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemAbortOperationResponsePayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::OperationID operationID{ 0u };
	std::uint16_t reserved{ 0u };

	des >> descriptorType >> descriptorIndex;
	des >> operationID >> reserved;

	AVDECC_ASSERT(des.usedBytes() == AecpAemAbortOperationResponsePayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, operationID);
}

/** ABORT_OPERATION Response - IEEE1722.1-2013 Clause 7.4.55.1 */
Serializer<AecpAemAbortOperationResponsePayloadSize> serializeAbortOperationResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID)
{
	// Same as ABORT_OPERATION Command
	static_assert(AecpAemAbortOperationResponsePayloadSize == AecpAemAbortOperationCommandPayloadSize, "ABORT_OPERATION Response no longer the same as ABORT_OPERATION Command");
	return serializeAbortOperationCommand(descriptorType, descriptorIndex, operationID);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::OperationID> deserializeAbortOperationResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as ABORT_OPERATION Command
	static_assert(AecpAemAbortOperationResponsePayloadSize == AecpAemAbortOperationCommandPayloadSize, "ABORT_OPERATION Response no longer the same as ABORT_OPERATION Command");
	checkResponsePayload(payload, status, AecpAemAbortOperationCommandPayloadSize, AecpAemAbortOperationResponsePayloadSize);
	return deserializeAbortOperationCommand(payload);
}

/** OPERATION_STATUS Unsolicited Response - IEEE1722.1-2013 Clause 7.4.55.1 */
Serializer<AecpAemOperationStatusResponsePayloadSize> serializeOperationStatusResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, std::uint16_t const percentComplete)
{
	Serializer<AecpAemOperationStatusResponsePayloadSize> ser;

	ser << descriptorType << descriptorIndex;
	ser << operationID << percentComplete;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::OperationID, std::uint16_t> deserializeOperationStatusResponse(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemOperationStatusResponsePayloadSize) // Malformed packet
	{
		throw IncorrectPayloadSizeException();
	}

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::OperationID operationID{ 0u };
	std::uint16_t percentComplete{ 0u };

	des >> descriptorType >> descriptorIndex;
	des >> operationID >> percentComplete;

	AVDECC_ASSERT(des.usedBytes() == AecpAemOperationStatusResponsePayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, operationID, percentComplete);
}

/** SET_MEMORY_OBJECT_LENGTH Command - IEEE1722.1-2013 Clause 7.4.72.1 */
Serializer<AecpAemSetMemoryObjectLengthCommandPayloadSize> serializeSetMemoryObjectLengthCommand(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length)
{
	Serializer<AecpAemSetMemoryObjectLengthCommandPayloadSize> ser;

	ser << memoryObjectIndex << configurationIndex;
	ser << length;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::ConfigurationIndex, entity::model::MemoryObjectIndex, std::uint64_t> deserializeSetMemoryObjectLengthCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemSetMemoryObjectLengthCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::MemoryObjectIndex memoryObjectIndex{ 0u };
	entity::model::ConfigurationIndex configurationIndex{ 0u };
	std::uint64_t length{ 0u };

	des >> memoryObjectIndex >> configurationIndex;
	des >> length;

	AVDECC_ASSERT(des.usedBytes() == AecpAemSetMemoryObjectLengthCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(configurationIndex, memoryObjectIndex, length);
}

/** SET_MEMORY_OBJECT_LENGTH Response - IEEE1722.1-2013 Clause 7.4.72.1 */
Serializer<AecpAemSetMemoryObjectLengthResponsePayloadSize> serializeSetMemoryObjectLengthResponse(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length)
{
	// Same as SET_MEMORY_OBJECT_LENGTH Command
	static_assert(AecpAemSetMemoryObjectLengthResponsePayloadSize == AecpAemSetMemoryObjectLengthCommandPayloadSize, "SET_MEMORY_OBJECT_LENGTH Response no longer the same as SET_MEMORY_OBJECT_LENGTH Command");
	return serializeSetMemoryObjectLengthCommand(configurationIndex, memoryObjectIndex, length);
}

std::tuple<entity::model::ConfigurationIndex, entity::model::MemoryObjectIndex, std::uint64_t> deserializeSetMemoryObjectLengthResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_MEMORY_OBJECT_LENGTH Command
	static_assert(AecpAemSetMemoryObjectLengthResponsePayloadSize == AecpAemSetMemoryObjectLengthCommandPayloadSize, "SET_MEMORY_OBJECT_LENGTH Response no longer the same as SET_MEMORY_OBJECT_LENGTH Command");
	checkResponsePayload(payload, status, AecpAemSetMemoryObjectLengthCommandPayloadSize, AecpAemSetMemoryObjectLengthResponsePayloadSize);
	return deserializeSetMemoryObjectLengthCommand(payload);
}

/** GET_MEMORY_OBJECT_LENGTH Command - IEEE1722.1-2013 Clause 7.4.73.1 */
Serializer<AecpAemGetMemoryObjectLengthCommandPayloadSize> serializeGetMemoryObjectLengthCommand(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex)
{
	Serializer<AecpAemGetClockSourceCommandPayloadSize> ser;

	ser << memoryObjectIndex << configurationIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::ConfigurationIndex, entity::model::MemoryObjectIndex> deserializeGetMemoryObjectLengthCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetMemoryObjectLengthCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::MemoryObjectIndex memoryObjectIndex{ 0u };
	entity::model::ConfigurationIndex configurationIndex{ 0u };

	des >> memoryObjectIndex >> configurationIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpAemGetMemoryObjectLengthCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(configurationIndex, memoryObjectIndex);
}

/** GET_MEMORY_OBJECT_LENGTH Response - IEEE1722.1-2013 Clause 7.4.73.2 */
Serializer<AecpAemGetMemoryObjectLengthResponsePayloadSize> serializeGetMemoryObjectLengthResponse(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length)
{
	// Same as SET_MEMORY_OBJECT_LENGTH Command
	static_assert(AecpAemGetMemoryObjectLengthResponsePayloadSize == AecpAemSetMemoryObjectLengthCommandPayloadSize, "GET_MEMORY_OBJECT_LENGTH Response no longer the same as SET_MEMORY_OBJECT_LENGTH Command");
	return serializeSetMemoryObjectLengthCommand(configurationIndex, memoryObjectIndex, length);
}

std::tuple<entity::model::ConfigurationIndex, entity::model::MemoryObjectIndex, std::uint64_t> deserializeGetMemoryObjectLengthResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload)
{
	// Same as SET_MEMORY_OBJECT_LENGTH Command
	static_assert(AecpAemGetMemoryObjectLengthResponsePayloadSize == AecpAemSetMemoryObjectLengthCommandPayloadSize, "GET_MEMORY_OBJECT_LENGTH Response no longer the same as SET_MEMORY_OBJECT_LENGTH Command");
	checkResponsePayload(payload, status, AecpAemSetMemoryObjectLengthCommandPayloadSize, AecpAemGetMemoryObjectLengthResponsePayloadSize);
	return deserializeSetMemoryObjectLengthCommand(payload);
}

} // namespace aemPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
