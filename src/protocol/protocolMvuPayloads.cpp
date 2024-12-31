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

	AVDECC_ASSERT(des.usedBytes() == AecpMvuGetMilanInfoCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return;
}

/** GET_MILAN_INFO Response - Milan 1.2 Clause 5.4.4.1 */
Serializer<AecpMvuGetMilanInfoResponsePayloadSize> serializeGetMilanInfoResponse(entity::model::MilanInfo const& info)
{
	auto ser = Serializer<AecpMvuGetMilanInfoResponsePayloadSize>{};
	auto const reserved16 = std::uint16_t{ 0u };

	ser << reserved16;
	ser << info.protocolVersion << info.featuresFlags << info.certificationVersion;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::MilanInfo> deserializeGetMilanInfoResponse(entity::LocalEntity::MvuCommandStatus const status, MvuAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	checkResponsePayload(payload, status, AecpMvuGetMilanInfoCommandPayloadSize, AecpMvuGetMilanInfoResponsePayloadSize);

	// Check payload
	auto des = Deserializer{ commandPayload, commandPayloadLength };
	auto reserved16 = std::uint16_t{ 0u };
	auto info = entity::model::MilanInfo{};

	des >> reserved16;
	des >> info.protocolVersion >> info.featuresFlags >> info.certificationVersion;

	AVDECC_ASSERT(des.usedBytes() == AecpMvuGetMilanInfoResponsePayloadSize, "Used more bytes than specified in protocol constant");

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

	AVDECC_ASSERT(des.usedBytes() == AecpMvuSetSystemUniqueIDCommandPayloadSize, "Used more bytes than specified in protocol constant");

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
	auto const reserved32 = std::uint8_t{ 0u };

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
	auto reserved32 = std::uint8_t{ 0u };
	auto clockDomainIndex = entity::model::ClockDomainIndex{};
	auto flags = entity::MediaClockReferenceInfoFlags{};
	auto defaultMcrPrio = entity::model::DefaultMediaClockReferencePriority{};
	auto userMcrPrio = entity::model::MediaClockReferencePriority{};
	auto domainName = entity::model::AvdeccFixedString{};

	des >> clockDomainIndex;
	des >> flags >> reserved8 >> defaultMcrPrio >> userMcrPrio;
	des >> reserved32;
	des >> domainName;

	AVDECC_ASSERT(des.usedBytes() == AecpMvuSetMediaClockReferenceInfoCommandPayloadSize, "Used more bytes than specified in protocol constant");

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

	AVDECC_ASSERT(des.usedBytes() == AecpMvuGetMediaClockReferenceInfoCommandPayloadSize, "Used more bytes than specified in protocol constant");

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

} // namespace la::avdecc::protocol::mvuPayload
