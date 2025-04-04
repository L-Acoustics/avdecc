/*
* Copyright (C) 2016-2025, L-Acoustics and its contributors

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
* @file protocolMvuPayloads.cpp
* @author Christophe Calmejane
*/

#include "protocolMvuPayloads.hpp"
#include "logHelper.hpp"

namespace la::avdecc::protocol::mvuPayload
{
static inline void checkResponsePayload(MvuAecpdu::Payload const& payload, entity::LocalEntity::MvuCommandStatus const status, size_t const expectedPayloadCommandLength, size_t const expectedPayloadResponseLength)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	// If status is NotImplemented, we expect a reflected message (using Command length)
	if (status == entity::LocalEntity::MvuCommandStatus::NotImplemented)
	{
		if (commandPayloadLength != expectedPayloadCommandLength || (expectedPayloadCommandLength > 0 && commandPayload == nullptr)) // Malformed packet
		{
			throw IncorrectPayloadSizeException();
		}
		throw NotImplementedException();
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

/** GET_MILAN_INFO Command - Milan 1.2 Clause 5.4.4.1 */
Serializer<AecpMvuGetMilanInfoCommandPayloadSize> serializeGetMilanInfoCommand()
{
	auto ser = Serializer<AecpMvuGetMilanInfoCommandPayloadSize>{};
	auto const reserved16 = std::uint16_t{ 0u };

	ser << reserved16;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

void deserializeGetMilanInfoCommand(MvuAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpMvuGetMilanInfoCommandPayloadSize) // Malformed packet
	{
		throw IncorrectPayloadSizeException();
	}

	// Check payload
	auto des = Deserializer{ commandPayload, commandPayloadLength };
	auto reserved16 = std::uint16_t{ 0u };

	des >> reserved16;

	AVDECC_ASSERT(des.usedBytes() == AecpMvuGetMilanInfoCommandPayloadSize, "Unpacked bytes doesn't match protocol constant");

	return;
}

/** GET_MILAN_INFO Response - Milan 1.2 Clause 5.4.4.1 */
Serializer<AecpMvuGetMilanInfoResponsePayloadMaxSize> serializeGetMilanInfoResponse(entity::model::MilanInfo const& info)
{
	auto ser = Serializer<AecpMvuGetMilanInfoResponsePayloadMaxSize>{};
	auto const reserved16 = std::uint16_t{ 0u };

	ser << reserved16;
	ser << info.protocolVersion << info.featuresFlags << info.certificationVersion;

	AVDECC_ASSERT(ser.usedBytes() == AecpMvuGetMilanInfo12ResponsePayloadSize, "Used bytes do not match the protocol constant");

	// Pack Milan 1.3 fields
	ser << info.specificationVersion;
	AVDECC_ASSERT(ser.usedBytes() == AecpMvuGetMilanInfo13ResponsePayloadSize, "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::MilanInfo> deserializeGetMilanInfoResponse(entity::LocalEntity::MvuCommandStatus const status, MvuAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	checkResponsePayload(payload, status, AecpMvuGetMilanInfoCommandPayloadSize, AecpMvuGetMilanInfoResponsePayloadMinSize);

	// Check payload
	auto des = Deserializer{ commandPayload, commandPayloadLength };
	auto reserved16 = std::uint16_t{ 0u };
	auto info = entity::model::MilanInfo{};

	des >> reserved16;
	des >> info.protocolVersion >> info.featuresFlags >> info.certificationVersion;

	AVDECC_ASSERT(des.usedBytes() == AecpMvuGetMilanInfo12ResponsePayloadSize, "Unpacked bytes doesn't match protocol constant");

	// Unpack Milan 1.3 fields if present
	if (commandPayloadLength >= AecpMvuGetMilanInfo13ResponsePayloadSize)
	{
		des >> info.specificationVersion;
		AVDECC_ASSERT(des.usedBytes() == AecpMvuGetMilanInfo13ResponsePayloadSize, "Unpacked bytes doesn't match protocol constant");
	}
	// Fallback to Milan 1.2 specification if protocol version is 1
	else if (info.protocolVersion == 1u)
	{
		info.specificationVersion = entity::model::MilanVersion{ 1, 2 };
	}

	return std::make_tuple(info);
}

/** SET_SYSTEM_UNIQUE_ID Command - Milan 1.2 Clause 5.4.4.2 */
Serializer<AecpMvuSetSystemUniqueIDCommandPayloadSize> serializeSetSystemUniqueIDCommand(entity::model::SystemUniqueIdentifier const systemUniqueID)
{
	auto ser = Serializer<AecpMvuSetSystemUniqueIDCommandPayloadSize>{};
	auto const reserved16 = std::uint16_t{ 0u };

	ser << reserved16;
	ser << systemUniqueID;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::SystemUniqueIdentifier> deserializeSetSystemUniqueIDCommand(MvuAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpMvuSetSystemUniqueIDCommandPayloadSize) // Malformed packet
	{
		throw IncorrectPayloadSizeException();
	}

	// Check payload
	auto des = Deserializer{ commandPayload, commandPayloadLength };
	auto reserved16 = std::uint16_t{ 0u };
	auto systemUniqueID = entity::model::SystemUniqueIdentifier{};

	des >> reserved16;
	des >> systemUniqueID;

	AVDECC_ASSERT(des.usedBytes() == AecpMvuSetSystemUniqueIDCommandPayloadSize, "Unpacked bytes doesn't match protocol constant");

	return std::make_tuple(systemUniqueID);
}

/** SET_SYSTEM_UNIQUE_ID Response - Milan 1.2 Clause 5.4.4.2 */
Serializer<AecpMvuSetSystemUniqueIDResponsePayloadSize> serializeSetSystemUniqueIDResponse(entity::model::SystemUniqueIdentifier const systemUniqueID)
{
	// Same as SET_SYSTEM_UNIQUE_ID Command
	static_assert(AecpMvuSetSystemUniqueIDResponsePayloadSize == AecpMvuSetSystemUniqueIDCommandPayloadSize, "SET_SYSTEM_UNIQUE_ID Response no longer the same as SET_SYSTEM_UNIQUE_ID Command");
	return serializeSetSystemUniqueIDCommand(systemUniqueID);
}

std::tuple<entity::model::SystemUniqueIdentifier> deserializeSetSystemUniqueIDResponse(entity::LocalEntity::MvuCommandStatus const status, MvuAecpdu::Payload const& payload)
{
	// Same as SET_SYSTEM_UNIQUE_ID Command
	static_assert(AecpMvuSetSystemUniqueIDResponsePayloadSize == AecpMvuSetSystemUniqueIDCommandPayloadSize, "SET_SYSTEM_UNIQUE_ID Response no longer the same as SET_SYSTEM_UNIQUE_ID Command");
	checkResponsePayload(payload, status, AecpMvuSetSystemUniqueIDCommandPayloadSize, AecpMvuSetSystemUniqueIDResponsePayloadSize);
	return deserializeSetSystemUniqueIDCommand(payload);
}

/** GET_SYSTEM_UNIQUE_ID Command - Milan 1.2 Clause 5.4.4.3 */
Serializer<AecpMvuGetSystemUniqueIDCommandPayloadSize> serializeGetSystemUniqueIDCommand()
{
	auto ser = Serializer<AecpMvuGetSystemUniqueIDCommandPayloadSize>{};
	auto const reserved16 = std::uint16_t{ 0u };

	ser << reserved16;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

// No payload to deserialize

/** GET_SYSTEM_UNIQUE_ID Response - Milan 1.2 Clause 5.4.4.3 */
Serializer<AecpMvuGetSystemUniqueIDResponsePayloadSize> serializeGetSystemUniqueIDResponse(entity::model::SystemUniqueIdentifier const systemUniqueID)
{
	// Same as SET_SYSTEM_UNIQUE_ID Command
	static_assert(AecpMvuGetSystemUniqueIDResponsePayloadSize == AecpMvuSetSystemUniqueIDCommandPayloadSize, "GET_SYSTEM_UNIQUE_ID Response no longer the same as SET_SYSTEM_UNIQUE_ID Command");
	return serializeSetSystemUniqueIDCommand(systemUniqueID);
}

std::tuple<entity::model::SystemUniqueIdentifier> deserializeGetSystemUniqueIDResponse(entity::LocalEntity::MvuCommandStatus const status, MvuAecpdu::Payload const& payload)
{
	// Same as SET_SYSTEM_UNIQUE_ID Command
	static_assert(AecpMvuGetSystemUniqueIDResponsePayloadSize == AecpMvuSetSystemUniqueIDCommandPayloadSize, "GET_SYSTEM_UNIQUE_ID Response no longer the same as SET_SYSTEM_UNIQUE_ID Command");
	checkResponsePayload(payload, status, AecpMvuGetSystemUniqueIDCommandPayloadSize, AecpMvuGetSystemUniqueIDResponsePayloadSize);
	return deserializeSetSystemUniqueIDCommand(payload);
}

/** SET_MEDIA_CLOCK_REFERENCE_INFO Command - Milan 1.2 Clause 5.4.4.4 */
Serializer<AecpMvuSetMediaClockReferenceInfoCommandPayloadSize> serializeSetMediaClockReferenceInfoCommand(entity::model::ClockDomainIndex const clockDomainIndex, entity::MediaClockReferenceInfoFlags const flags, entity::model::DefaultMediaClockReferencePriority const defaultMcrPrio, entity::model::MediaClockReferencePriority const userMcrPrio, entity::model::AvdeccFixedString const& domainName)
{
	auto ser = Serializer<AecpMvuSetMediaClockReferenceInfoCommandPayloadSize>{};
	auto const reserved8 = std::uint8_t{ 0u };
	auto const reserved32 = std::uint32_t{ 0u };

	ser << clockDomainIndex;
	ser << flags << reserved8 << defaultMcrPrio << userMcrPrio;
	ser << reserved32;
	ser << domainName;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::ClockDomainIndex, entity::MediaClockReferenceInfoFlags, entity::model::DefaultMediaClockReferencePriority, entity::model::MediaClockReferencePriority, entity::model::AvdeccFixedString> deserializeSetMediaClockReferenceInfoCommand(MvuAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpMvuSetMediaClockReferenceInfoCommandPayloadSize) // Malformed packet
	{
		throw IncorrectPayloadSizeException();
	}

	// Check payload
	auto des = Deserializer{ commandPayload, commandPayloadLength };
	auto reserved8 = std::uint8_t{ 0u };
	auto reserved32 = std::uint32_t{ 0u };
	auto clockDomainIndex = entity::model::ClockDomainIndex{};
	auto flags = entity::MediaClockReferenceInfoFlags{};
	auto defaultMcrPrio = entity::model::DefaultMediaClockReferencePriority{};
	auto userMcrPrio = entity::model::MediaClockReferencePriority{};
	auto domainName = entity::model::AvdeccFixedString{};

	des >> clockDomainIndex;
	des >> flags >> reserved8 >> defaultMcrPrio >> userMcrPrio;
	des >> reserved32;
	des >> domainName;

	AVDECC_ASSERT(des.usedBytes() == AecpMvuSetMediaClockReferenceInfoCommandPayloadSize, "Unpacked bytes doesn't match protocol constant");

	return std::make_tuple(clockDomainIndex, flags, defaultMcrPrio, userMcrPrio, domainName);
}

/** SET_MEDIA_CLOCK_REFERENCE_INFO Response - Milan 1.2 Clause 5.4.4.4 */
Serializer<AecpMvuSetMediaClockReferenceInfoResponsePayloadSize> serializeSetMediaClockReferenceInfoResponse(entity::model::ClockDomainIndex const clockDomainIndex, entity::MediaClockReferenceInfoFlags const flags, entity::model::DefaultMediaClockReferencePriority const defaultMcrPrio, entity::model::MediaClockReferencePriority const userMcrPrio, entity::model::AvdeccFixedString const& domainName)
{
	// Same as SET_MEDIA_CLOCK_REFERENCE_INFO Command
	static_assert(AecpMvuSetMediaClockReferenceInfoResponsePayloadSize == AecpMvuSetMediaClockReferenceInfoCommandPayloadSize, "SET_MEDIA_CLOCK_REFERENCE_INFO Response no longer the same as SET_MEDIA_CLOCK_REFERENCE_INFO Command");
	return serializeSetMediaClockReferenceInfoCommand(clockDomainIndex, flags, defaultMcrPrio, userMcrPrio, domainName);
}

std::tuple<entity::model::ClockDomainIndex, entity::MediaClockReferenceInfoFlags, entity::model::DefaultMediaClockReferencePriority, entity::model::MediaClockReferencePriority, entity::model::AvdeccFixedString> deserializeSetMediaClockReferenceInfoResponse(entity::LocalEntity::MvuCommandStatus const status, MvuAecpdu::Payload const& payload)
{
	// Same as SET_MEDIA_CLOCK_REFERENCE_INFO Command
	static_assert(AecpMvuSetMediaClockReferenceInfoResponsePayloadSize == AecpMvuSetMediaClockReferenceInfoCommandPayloadSize, "SET_MEDIA_CLOCK_REFERENCE_INFO Response no longer the same as SET_MEDIA_CLOCK_REFERENCE_INFO Command");
	checkResponsePayload(payload, status, AecpMvuSetMediaClockReferenceInfoCommandPayloadSize, AecpMvuSetMediaClockReferenceInfoResponsePayloadSize);
	return deserializeSetMediaClockReferenceInfoCommand(payload);
}

/** GET_MEDIA_CLOCK_REFERENCE_INFO Command - Milan 1.2 Clause 5.4.4.5 */
Serializer<AecpMvuGetMediaClockReferenceInfoCommandPayloadSize> serializeGetMediaClockReferenceInfoCommand(entity::model::ClockDomainIndex const clockDomainIndex)
{
	auto ser = Serializer<AecpMvuGetMediaClockReferenceInfoCommandPayloadSize>{};

	ser << clockDomainIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::ClockDomainIndex> deserializeGetMediaClockReferenceInfoCommand(MvuAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpMvuGetMediaClockReferenceInfoCommandPayloadSize) // Malformed packet
	{
		throw IncorrectPayloadSizeException();
	}

	// Check payload
	auto des = Deserializer{ commandPayload, commandPayloadLength };
	auto clockDomainIndex = entity::model::ClockDomainIndex{};

	des >> clockDomainIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpMvuGetMediaClockReferenceInfoCommandPayloadSize, "Unpacked bytes doesn't match protocol constant");

	return std::make_tuple(clockDomainIndex);
}

/** GET_MEDIA_CLOCK_REFERENCE_INFO Response - Milan 1.2 Clause 5.4.4.5 */
Serializer<AecpMvuGetMediaClockReferenceInfoResponsePayloadSize> serializeGetMediaClockReferenceInfoResponse(entity::model::ClockDomainIndex const clockDomainIndex, entity::MediaClockReferenceInfoFlags const flags, entity::model::DefaultMediaClockReferencePriority const defaultMcrPrio, entity::model::MediaClockReferencePriority const userMcrPrio, entity::model::AvdeccFixedString const& domainName)
{
	// Same as SET_MEDIA_CLOCK_REFERENCE_INFO Command
	static_assert(AecpMvuGetMediaClockReferenceInfoResponsePayloadSize == AecpMvuSetMediaClockReferenceInfoCommandPayloadSize, "GET_MEDIA_CLOCK_REFERENCE_INFO Response no longer the same as SET_MEDIA_CLOCK_REFERENCE_INFO Command");
	return serializeSetMediaClockReferenceInfoCommand(clockDomainIndex, flags, defaultMcrPrio, userMcrPrio, domainName);
}

std::tuple<entity::model::ClockDomainIndex, entity::MediaClockReferenceInfoFlags, entity::model::DefaultMediaClockReferencePriority, entity::model::MediaClockReferencePriority, entity::model::AvdeccFixedString> deserializeGetMediaClockReferenceInfoResponse(entity::LocalEntity::MvuCommandStatus const status, MvuAecpdu::Payload const& payload)
{
	// Same as SET_MEDIA_CLOCK_REFERENCE_INFO Command
	static_assert(AecpMvuGetMediaClockReferenceInfoResponsePayloadSize == AecpMvuSetMediaClockReferenceInfoCommandPayloadSize, "GET_MEDIA_CLOCK_REFERENCE_INFO Response no longer the same as SET_MEDIA_CLOCK_REFERENCE_INFO Command");
	checkResponsePayload(payload, status, AecpMvuGetMediaClockReferenceInfoCommandPayloadSize, AecpMvuGetMediaClockReferenceInfoResponsePayloadSize);
	return deserializeSetMediaClockReferenceInfoCommand(payload);
}

/** BIND_STREAM Command - Milan 1.3 Clause 5.4.4.6 */
Serializer<AecpMvuBindStreamCommandPayloadSize> serializeBindStreamCommand(entity::BindStreamFlags const flags, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, UniqueIdentifier const talkerEntityID, entity::model::DescriptorIndex talkerStreamIndex)
{
	auto ser = Serializer<AecpMvuBindStreamCommandPayloadSize>{};
	auto const reserved16 = std::uint16_t{ 0u };

	ser << flags << descriptorType << descriptorIndex;
	ser << talkerEntityID << talkerStreamIndex;
	ser << reserved16;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::BindStreamFlags, entity::model::DescriptorType, entity::model::DescriptorIndex, UniqueIdentifier, entity::model::DescriptorIndex> deserializeBindStreamCommand(MvuAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpMvuBindStreamCommandPayloadSize) // Malformed packet
	{
		throw IncorrectPayloadSizeException();
	}

	// Check payload
	auto des = Deserializer{ commandPayload, commandPayloadLength };
	auto reserved16 = std::uint16_t{ 0u };
	auto flags = entity::BindStreamFlags{};
	auto descriptorType = entity::model::DescriptorType{};
	auto descriptorIndex = entity::model::DescriptorIndex{};
	auto talkerEntityID = UniqueIdentifier{};
	auto talkerStreamIndex = entity::model::DescriptorIndex{};

	des >> flags >> descriptorType >> descriptorIndex;
	des >> talkerEntityID >> talkerStreamIndex;
	des >> reserved16;

	AVDECC_ASSERT(des.usedBytes() == AecpMvuBindStreamCommandPayloadSize, "Unpacked bytes doesn't match protocol constant");

	return std::make_tuple(flags, descriptorType, descriptorIndex, talkerEntityID, talkerStreamIndex);
}

/** BIND_STREAM Response - Milan 1.3 Clause 5.4.4.6 */
Serializer<AecpMvuBindStreamResponsePayloadSize> serializeBindStreamResponse(entity::BindStreamFlags const flags, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, UniqueIdentifier const talkerEntityID, entity::model::DescriptorIndex talkerStreamIndex)
{
	// Same as BIND_STREAM Command
	static_assert(AecpMvuBindStreamResponsePayloadSize == AecpMvuBindStreamCommandPayloadSize, "BIND_STREAM Response no longer the same as BIND_STREAM Command");
	return serializeBindStreamCommand(flags, descriptorType, descriptorIndex, talkerEntityID, talkerStreamIndex);
}

std::tuple<entity::BindStreamFlags, entity::model::DescriptorType, entity::model::DescriptorIndex, UniqueIdentifier, entity::model::DescriptorIndex> deserializeBindStreamResponse(entity::LocalEntity::MvuCommandStatus const status, MvuAecpdu::Payload const& payload)
{
	// Same as BIND_STREAM Command
	static_assert(AecpMvuBindStreamResponsePayloadSize == AecpMvuBindStreamCommandPayloadSize, "BIND_STREAM Response no longer the same as BIND_STREAM Command");
	checkResponsePayload(payload, status, AecpMvuBindStreamCommandPayloadSize, AecpMvuBindStreamResponsePayloadSize);
	return deserializeBindStreamCommand(payload);
}

/** UNBIND_STREAM Command - Milan 1.3 Clause 5.4.4.7 */
Serializer<AecpMvuUnbindStreamCommandPayloadSize> serializeUnbindStreamCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	auto ser = Serializer<AecpMvuUnbindStreamCommandPayloadSize>{};
	auto const reserved16 = std::uint16_t{ 0u };

	ser << reserved16;
	ser << descriptorType << descriptorIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeUnbindStreamCommand(MvuAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpMvuUnbindStreamCommandPayloadSize) // Malformed packet
	{
		throw IncorrectPayloadSizeException();
	}

	// Check payload
	auto des = Deserializer{ commandPayload, commandPayloadLength };
	auto reserved16 = std::uint16_t{ 0u };
	auto descriptorType = entity::model::DescriptorType{};
	auto descriptorIndex = entity::model::DescriptorIndex{};

	des >> reserved16;
	des >> descriptorType >> descriptorIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpMvuUnbindStreamCommandPayloadSize, "Unpacked bytes doesn't match protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex);
}

/** UNBIND_STREAM Response - Milan 1.3 Clause 5.4.4.7 */
Serializer<AecpMvuUnbindStreamResponsePayloadSize> serializeUnbindStreamResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
{
	// Same as UNBIND_STREAM Command
	static_assert(AecpMvuUnbindStreamResponsePayloadSize == AecpMvuUnbindStreamCommandPayloadSize, "UNBIND_STREAM Response no longer the same as UNBIND_STREAM Command");
	return serializeUnbindStreamCommand(descriptorType, descriptorIndex);
}
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeUnbindStreamResponse(entity::LocalEntity::MvuCommandStatus const status, MvuAecpdu::Payload const& payload)
{
	// Same as UNBIND_STREAM Command
	static_assert(AecpMvuUnbindStreamResponsePayloadSize == AecpMvuUnbindStreamCommandPayloadSize, "UNBIND_STREAM Response no longer the same as UNBIND_STREAM Command");
	checkResponsePayload(payload, status, AecpMvuUnbindStreamCommandPayloadSize, AecpMvuUnbindStreamResponsePayloadSize);
	return deserializeUnbindStreamCommand(payload);
}

} // namespace la::avdecc::protocol::mvuPayload
