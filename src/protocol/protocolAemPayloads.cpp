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
/** ACQUIRE_ENTITY Command - Clause 7.4.1.1 */
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

/** READ_DESCRIPTOR Response - Clause 7.4.5.2 */
//Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeReadDescriptorResponse(entity::model::ConfigurationIndex const /*configurationIndex*/, entity::model::DescriptorType const /*descriptorType*/, entity::model::DescriptorIndex const /*descriptorIndex*/)
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

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadConfigurationDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check configuration descriptor payload - Clause 7.2.2
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

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadAudioUnitDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check audio unit descriptor payload - Clause 7.2.3
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

		// Compute deserializer offset for sampling rates (Clause 7.2.3 says the sampling_rates_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
		samplingRatesOffset += sizeof(entity::model::ConfigurationIndex) + sizeof(std::uint16_t);

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

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadStreamDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check stream descriptor payload - Clause 7.2.6
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

		// Compute deserializer offset for formats (Clause 7.2.6 says the formats_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
		formatsOffset += sizeof(entity::model::ConfigurationIndex) + sizeof(std::uint16_t);

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
		// Check if we have redundant fields (AVnu Alliance 'Network Redundancy' extension)
		std::uint16_t redundantOffset{ 0u };
		std::uint16_t numberOfRedundantStreams{ 0u };
		auto const remainingBytesBeforeFormats = formatsOffset - des.usedBytes();
		if (remainingBytesBeforeFormats >= (sizeof(redundantOffset) + sizeof(numberOfRedundantStreams)))
		{
			des >> redundantOffset >> numberOfRedundantStreams;
			// Compute deserializer offset for redundant streams association (Clause 7.2.6 says the redundant_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
			redundantOffset += sizeof(entity::model::ConfigurationIndex) + sizeof(std::uint16_t);
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

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadJackDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check jack descriptor payload - Clause 7.2.7
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

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadAvbInterfaceDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check avb interface descriptor payload - Clause 7.2.8
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

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadClockSourceDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check clock source descriptor payload - Clause 7.2.9
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

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadMemoryObjectDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check memory object descriptor payload - Clause 7.2.10
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

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadStreamPortDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check stream port descriptor payload - Clause 7.2.13
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

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadExternalPortDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check external port descriptor payload - Clause 7.2.14
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

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadInternalPortDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check internal port descriptor payload - Clause 7.2.15
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

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadAudioClusterDescriptorResponsePayloadSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check audio cluster descriptor payload - Clause 7.2.16
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

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadAudioMapDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check audio map descriptor payload - Clause 7.2.19
		Deserializer des(commandPayload, commandPayloadLength);
		std::uint16_t mappingsOffset{ 0u };
		std::uint16_t numberOfMappings{ 0u };
		des.setPosition(commonSize); // Skip already unpacked common header
		des >> mappingsOffset >> numberOfMappings;

		// Check descriptor variable size
		auto const mappingsSize = entity::model::AudioMapping::size() * numberOfMappings;
		if (des.remaining() < mappingsSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Compute deserializer offset for sampling rates (Clause 7.2.19 says the mappings_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
		mappingsOffset += sizeof(entity::model::ConfigurationIndex) + sizeof(std::uint16_t);

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

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadControlDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check control descriptor payload - Clause 7.2.22
		auto des = Deserializer{ commandPayload, commandPayloadLength };
		auto numberOfValues = std::uint16_t{ 0u };
		auto valuesOffset = std::uint16_t{ 0u };

		des.setPosition(commonSize); // Skip already unpacked common header
		des >> controlDescriptor.objectName >> controlDescriptor.localizedDescription;
		des >> controlDescriptor.blockLatency >> controlDescriptor.controlLatency >> controlDescriptor.controlDomain;
		des >> controlDescriptor.controlValueType >> controlDescriptor.controlType >> controlDescriptor.resetTime;
		des >> valuesOffset >> numberOfValues;
		des >> controlDescriptor.signalType >> controlDescriptor.signalIndex >> controlDescriptor.signalOutput;

		// No need to check descriptor variable size, we'll let the ControlValueType specific unpacker throw if needed

		// Compute deserializer offset for values (Clause 7.2.22 says the values_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
		valuesOffset += sizeof(entity::model::ConfigurationIndex) + sizeof(std::uint16_t);

		// Set deserializer position
		if (valuesOffset < des.usedBytes())
			throw IncorrectPayloadSizeException();
		des.setPosition(valuesOffset);

		// Unpack Control Values based on ControlValueType
		auto const valueType = controlDescriptor.controlValueType.getType();
		if (auto const& it = s_Dispatch.find(valueType); it != s_Dispatch.end())
		{
			auto [valuesStatic, valuesDynamic] = it->second(des, numberOfValues);
			controlDescriptor.valuesStatic = std::move(valuesStatic);
			controlDescriptor.valuesDynamic = std::move(valuesDynamic);
		}
		else
		{
			LOG_AEM_PAYLOAD_TRACE("ReadDescriptorResponse deserialize warning: Unsupported ControlValueType for READ_CONTROL_DESCRIPTOR RESPONSE: {}", valueType);
			throw UnsupportedValueException();
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

	// Clause 7.4.5.2 says we should only unpack common descriptor fields in case status is not Success
	if (status == AecpStatus::Success)
	{
		auto* const commandPayload = payload.first;
		auto const commandPayloadLength = payload.second;

		if (commandPayload == nullptr || commandPayloadLength < AecpAemReadClockDomainDescriptorResponsePayloadMinSize) // Malformed packet
			throw IncorrectPayloadSizeException();

		// Check clock domain descriptor payload - Clause 7.2.32
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

		// Compute deserializer offset for sampling rates (Clause 7.2.32 says the clock_sources_offset field is from the base of the descriptor, which is not where our deserializer buffer starts)
		clockSourcesOffset += sizeof(entity::model::ConfigurationIndex) + sizeof(std::uint16_t);

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


/** GET_STREAM_INFO Response - Clause 7.4.16.2 */
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

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamInfo> deserializeGetStreamInfoResponse(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetStreamInfoResponsePayloadSize) // Malformed packet
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

	return std::make_tuple(descriptorType, descriptorIndex, streamInfo);
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

/** SET_CLOCK_SOURCE Response - Clause 7.4.23.1 */
Serializer<AecpAemSetClockSourceResponsePayloadSize> serializeSetClockSourceResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ClockSourceIndex const clockSourceIndex)
{
	// Same as SET_CLOCK_SOURCE Command
	static_assert(AecpAemSetClockSourceResponsePayloadSize == AecpAemSetClockSourceCommandPayloadSize, "SET_CLOCK_SOURCE Response no longer the same as SET_CLOCK_SOURCE Command");
	return serializeSetClockSourceCommand(descriptorType, descriptorIndex, clockSourceIndex);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::ClockSourceIndex> deserializeSetClockSourceResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_CLOCK_SOURCE Command
	static_assert(AecpAemSetClockSourceResponsePayloadSize == AecpAemSetClockSourceCommandPayloadSize, "SET_CLOCK_SOURCE Response no longer the same as SET_CLOCK_SOURCE Command");
	return deserializeSetClockSourceCommand(payload);
}

/** GET_CLOCK_SOURCE Command - Clause 7.4.24.1 */
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

/** SET_CONTROL Command - Clause 7.4.25.1 */
static inline void createPackDynamicControlValuesDispatchTable(std::unordered_map<entity::model::ControlValueType::Type, std::function<void(Serializer<AemAecpdu::MaximumSendPayloadBufferLength>&, entity::model::ControlValues const&)>>& dispatchTable)
{
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

/** SET_CONTROL Response - Clause 7.4.25.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeSetControlResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ControlValues const& controlValues)
{
	// Same as SET_CONTROL Command
	static_assert(AecpAemSetControlResponsePayloadMinSize == AecpAemSetControlCommandPayloadMinSize, "SET_CONTROL Response no longer the same as SET_CONTROL Command");
	return serializeSetControlCommand(descriptorType, descriptorIndex, controlValues);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, MemoryBuffer> deserializeSetControlResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_CONTROL Command
	static_assert(AecpAemSetControlResponsePayloadMinSize == AecpAemSetControlCommandPayloadMinSize, "SET_CONTROL Response no longer the same as SET_CONTROL Command");
	return deserializeSetControlCommand(payload);
}

/** GET_CONTROL Command - Clause 7.4.26.1 */
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

/** GET_CONTROL Response - Clause 7.4.26.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeGetControlResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ControlValues const& controlValues)
{
	// Same as SET_CONTROL Command
	static_assert(AecpAemGetControlResponsePayloadMinSize == AecpAemSetControlCommandPayloadMinSize, "GET_CONTROL Response no longer the same as SET_CONTROL Command");
	return serializeSetControlCommand(descriptorType, descriptorIndex, controlValues);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, MemoryBuffer> deserializeGetControlResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_CONTROL Command
	static_assert(AecpAemGetControlResponsePayloadMinSize == AecpAemSetControlCommandPayloadMinSize, "GET_CONTROL Response no longer the same as SET_CONTROL Command");
	return deserializeSetControlCommand(payload);
}

/** START_STREAMING Command - Clause 7.4.35.1 */
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

/** GET_AVB_INFO Response - Clause 7.4.40.2 */
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

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::AvbInfo> deserializeGetAvbInfoResponse(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetAvbInfoResponsePayloadMinSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::AvbInfo avbInfo{};
	std::uint16_t numberOfMappings{ 0u };

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

	return std::make_tuple(descriptorType, descriptorIndex, avbInfo);
}

/** GET_AS_PATH Command - Clause 7.4.41.1 */
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

/** GET_AS_PATH Response - Clause 7.4.41.2 */
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

std::tuple<entity::model::DescriptorIndex, entity::model::AsPath> deserializeGetAsPathResponse(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetAsPathResponsePayloadMinSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::AsPath asPath{};
	std::uint16_t count{ 0u };

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

	return std::make_tuple(descriptorIndex, asPath);
}

/** GET_COUNTERS Command - Clause 7.4.42.1 */
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

/** GET_COUNTERS Response - Clause 7.4.42.2 */
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

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::DescriptorCounterValidFlag, entity::model::DescriptorCounters> deserializeGetCountersResponse(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetCountersResponsePayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::DescriptorCounterValidFlag validCounters{ 0u };
	entity::model::DescriptorCounters counters{};

	des >> descriptorType >> descriptorIndex;
	des >> validCounters;

	// Deserialize the counters
	for (auto& counter : counters)
	{
		des >> counter;
	}

	AVDECC_ASSERT(des.usedBytes() == AecpAemGetCountersResponsePayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, validCounters, counters);
}

/** GET_AUDIO_MAP Command - Clause 7.4.44.1 */
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

/** GET_AUDIO_MAP Response - Clause 7.4.44.2 */
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

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::MapIndex, entity::model::MapIndex, entity::model::AudioMappings> deserializeGetAudioMapResponse(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpAemGetAudioMapResponsePayloadMinSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	entity::model::MapIndex mapIndex{ 0u };
	entity::model::MapIndex numberOfMaps{ 0u };
	std::uint16_t numberOfMappings{ 0u };
	std::uint16_t reserved{ 0u };

	des >> descriptorType >> descriptorIndex;
	des >> mapIndex >> numberOfMaps >> numberOfMappings >> reserved;

	// Check variable size
	auto const mappingsSize = entity::model::AudioMapping::size() * numberOfMappings;
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
	AVDECC_ASSERT(des.usedBytes() == (protocol::aemPayload::AecpAemGetAudioMapResponsePayloadMinSize + mappingsSize), "Used more bytes than specified in protocol constant");

	if (des.remaining() != 0)
	{
		LOG_AEM_PAYLOAD_TRACE("GetAudioMap Response deserialize warning: Remaining bytes in buffer");
	}

	return std::make_tuple(descriptorType, descriptorIndex, mapIndex, numberOfMaps, mappings);
}

/** ADD_AUDIO_MAPPINGS Command - Clause 7.4.45.1 */
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

/** ADD_AUDIO_MAPPINGS Response - Clause 7.4.45.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeAddAudioMappingsResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AudioMappings const& mappings)
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

/** REMOVE_AUDIO_MAPPINGS Response - Clause 7.4.46.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeRemoveAudioMappingsResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AudioMappings const& mappings)
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

/** START_OPERATION Command - Clause 7.4.53.1 */
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

/** START_OPERATION Response - Clause 7.4.53.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeStartOperationResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, entity::model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer)
{
	// Same as START_OPERATION Command
	static_assert(AecpAemStartOperationResponsePayloadMinSize == AecpAemStartOperationCommandPayloadMinSize, "START_OPERATION Response no longer the same as START_OPERATION Command");
	return serializeStartOperationCommand(descriptorType, descriptorIndex, operationID, operationType, memoryBuffer);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::OperationID, entity::model::MemoryObjectOperationType, MemoryBuffer> deserializeStartOperationResponse(AemAecpdu::Payload const& payload)
{
	// Same as START_OPERATION Command
	static_assert(AecpAemStartOperationResponsePayloadMinSize == AecpAemStartOperationCommandPayloadMinSize, "START_OPERATION Response no longer the same as START_OPERATION Command");
	return deserializeStartOperationCommand(payload);
}

/** ABORT_OPERATION Command - Clause 7.4.55.1 */
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

/** ABORT_OPERATION Response - Clause 7.4.55.1 */
Serializer<AecpAemAbortOperationResponsePayloadSize> serializeAbortOperationResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID)
{
	// Same as ABORT_OPERATION Command
	static_assert(AecpAemAbortOperationResponsePayloadSize == AecpAemAbortOperationCommandPayloadSize, "ABORT_OPERATION Response no longer the same as ABORT_OPERATION Command");
	return serializeAbortOperationCommand(descriptorType, descriptorIndex, operationID);
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::OperationID> deserializeAbortOperationResponse(AemAecpdu::Payload const& payload)
{
	// Same as ABORT_OPERATION Command
	static_assert(AecpAemAbortOperationResponsePayloadSize == AecpAemAbortOperationCommandPayloadSize, "ABORT_OPERATION Response no longer the same as ABORT_OPERATION Command");
	return deserializeAbortOperationCommand(payload);
}

/** OPERATION_STATUS Unsolicited Response - Clause 7.4.55.1 */
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
		throw IncorrectPayloadSizeException();

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

/** SET_MEMORY_OBJECT_LENGTH Command - Clause 7.4.72.1 */
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

/** SET_MEMORY_OBJECT_LENGTH Response - Clause 7.4.72.1 */
Serializer<AecpAemSetMemoryObjectLengthResponsePayloadSize> serializeSetMemoryObjectLengthResponse(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length)
{
	// Same as SET_MEMORY_OBJECT_LENGTH Command
	static_assert(AecpAemSetMemoryObjectLengthResponsePayloadSize == AecpAemSetMemoryObjectLengthCommandPayloadSize, "SET_MEMORY_OBJECT_LENGTH Response no longer the same as SET_MEMORY_OBJECT_LENGTH Command");
	return serializeSetMemoryObjectLengthCommand(configurationIndex, memoryObjectIndex, length);
}

std::tuple<entity::model::ConfigurationIndex, entity::model::MemoryObjectIndex, std::uint64_t> deserializeSetMemoryObjectLengthResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_MEMORY_OBJECT_LENGTH Command
	static_assert(AecpAemSetMemoryObjectLengthResponsePayloadSize == AecpAemSetMemoryObjectLengthCommandPayloadSize, "SET_MEMORY_OBJECT_LENGTH Response no longer the same as SET_MEMORY_OBJECT_LENGTH Command");
	return deserializeSetMemoryObjectLengthCommand(payload);
}

/** GET_MEMORY_OBJECT_LENGTH Command - Clause 7.4.73.1 */
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

/** GET_MEMORY_OBJECT_LENGTH Response - Clause 7.4.73.2 */
Serializer<AecpAemGetMemoryObjectLengthResponsePayloadSize> serializeGetMemoryObjectLengthResponse(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length)
{
	// Same as SET_MEMORY_OBJECT_LENGTH Command
	static_assert(AecpAemGetMemoryObjectLengthResponsePayloadSize == AecpAemSetMemoryObjectLengthCommandPayloadSize, "GET_MEMORY_OBJECT_LENGTH Response no longer the same as SET_MEMORY_OBJECT_LENGTH Command");
	return serializeSetMemoryObjectLengthCommand(configurationIndex, memoryObjectIndex, length);
}

std::tuple<entity::model::ConfigurationIndex, entity::model::MemoryObjectIndex, std::uint64_t> deserializeGetMemoryObjectLengthResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_MEMORY_OBJECT_LENGTH Command
	static_assert(AecpAemGetMemoryObjectLengthResponsePayloadSize == AecpAemSetMemoryObjectLengthCommandPayloadSize, "GET_MEMORY_OBJECT_LENGTH Response no longer the same as SET_MEMORY_OBJECT_LENGTH Command");
	return deserializeSetMemoryObjectLengthCommand(payload);
}

} // namespace aemPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
