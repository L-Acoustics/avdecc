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
* @file protocolMvuPayloads.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/memoryBuffer.hpp"
#include "la/avdecc/internals/serialization.hpp"
#include "la/avdecc/internals/entityModel.hpp"
#include "la/avdecc/internals/protocolMvuPayloadSizes.hpp"


#include <cstdint>
#include <tuple>

namespace la::avdecc::protocol::mvuPayload
{
/** Exception when receiving a payload that has an invalid size */
class IncorrectPayloadSizeException final : public Exception
{
public:
	IncorrectPayloadSizeException()
		: Exception("Incorrect payload size")
	{
	}
};

/** Exception when receiving NOT_IMPLEMENTED reponse status */
class NotImplementedException final : public Exception
{
public:
	NotImplementedException()
		: Exception("Incorrect payload size")
	{
	}
};

// All serialization methods might throw a std::invalid_argument if serialization goes wrong
// All deserialization methods might throw a la::avdecc:Exception if deserialization goes wrong

/** GET_MILAN_INFO Command - Milan 1.2 Clause 5.4.4.1 */
Serializer<AecpMvuGetMilanInfoCommandPayloadSize> serializeGetMilanInfoCommand();
void deserializeGetMilanInfoCommand(MvuAecpdu::Payload const& payload);

/** GET_MILAN_INFO Response - Milan 1.2 Clause 5.4.4.1 */
Serializer<AecpMvuGetMilanInfoResponsePayloadMaxSize> serializeGetMilanInfoResponse(entity::model::MilanInfo const& info);
std::tuple<entity::model::MilanInfo> deserializeGetMilanInfoResponse(entity::LocalEntity::MvuCommandStatus const status, MvuAecpdu::Payload const& payload);

/** SET_SYSTEM_UNIQUE_ID Command - Milan 1.2 Clause 5.4.4.2 */
Serializer<AecpMvuSetSystemUniqueIDCommandPayloadSize> serializeSetSystemUniqueIDCommand(entity::model::SystemUniqueIdentifier const systemUniqueID);
std::tuple<entity::model::SystemUniqueIdentifier> deserializeSetSystemUniqueIDCommand(MvuAecpdu::Payload const& payload);

/** SET_SYSTEM_UNIQUE_ID Response - Milan 1.2 Clause 5.4.4.2 */
Serializer<AecpMvuSetSystemUniqueIDResponsePayloadSize> serializeSetSystemUniqueIDResponse(entity::model::SystemUniqueIdentifier const systemUniqueID);
std::tuple<entity::model::SystemUniqueIdentifier> deserializeSetSystemUniqueIDResponse(entity::LocalEntity::MvuCommandStatus const status, MvuAecpdu::Payload const& payload);

/** GET_SYSTEM_UNIQUE_ID Command - Milan 1.2 Clause 5.4.4.3 */
Serializer<AecpMvuGetSystemUniqueIDCommandPayloadSize> serializeGetSystemUniqueIDCommand();
//void deserializeGetSystemUniqueIDCommand(MvuAecpdu::Payload const& payload); // No payload

/** GET_SYSTEM_UNIQUE_ID Response - Milan 1.2 Clause 5.4.4.3 */
Serializer<AecpMvuGetSystemUniqueIDResponsePayloadSize> serializeGetSystemUniqueIDResponse(entity::model::SystemUniqueIdentifier const systemUniqueID);
std::tuple<entity::model::SystemUniqueIdentifier> deserializeGetSystemUniqueIDResponse(entity::LocalEntity::MvuCommandStatus const status, MvuAecpdu::Payload const& payload);

/** SET_MEDIA_CLOCK_REFERENCE_INFO Command - Milan 1.2 Clause 5.4.4.4 */
Serializer<AecpMvuSetMediaClockReferenceInfoCommandPayloadSize> serializeSetMediaClockReferenceInfoCommand(entity::model::ClockDomainIndex const clockDomainIndex, entity::MediaClockReferenceInfoFlags const flags, entity::model::DefaultMediaClockReferencePriority const defaultMcrPrio, entity::model::MediaClockReferencePriority const userMcrPrio, entity::model::AvdeccFixedString const& domainName);
std::tuple<entity::model::ClockDomainIndex, entity::MediaClockReferenceInfoFlags, entity::model::DefaultMediaClockReferencePriority, entity::model::MediaClockReferencePriority, entity::model::AvdeccFixedString> deserializeSetMediaClockReferenceInfoCommand(MvuAecpdu::Payload const& payload);

/** SET_MEDIA_CLOCK_REFERENCE_INFO Response - Milan 1.2 Clause 5.4.4.4 */
Serializer<AecpMvuSetMediaClockReferenceInfoResponsePayloadSize> serializeSetMediaClockReferenceInfoResponse(entity::model::ClockDomainIndex const clockDomainIndex, entity::MediaClockReferenceInfoFlags const flags, entity::model::DefaultMediaClockReferencePriority const defaultMcrPrio, entity::model::MediaClockReferencePriority const userMcrPrio, entity::model::AvdeccFixedString const& domainName);
std::tuple<entity::model::ClockDomainIndex, entity::MediaClockReferenceInfoFlags, entity::model::DefaultMediaClockReferencePriority, entity::model::MediaClockReferencePriority, entity::model::AvdeccFixedString> deserializeSetMediaClockReferenceInfoResponse(entity::LocalEntity::MvuCommandStatus const status, MvuAecpdu::Payload const& payload);

/** GET_MEDIA_CLOCK_REFERENCE_INFO Command - Milan 1.2 Clause 5.4.4.5 */
Serializer<AecpMvuGetMediaClockReferenceInfoCommandPayloadSize> serializeGetMediaClockReferenceInfoCommand(entity::model::ClockDomainIndex const clockDomainIndex);
std::tuple<entity::model::ClockDomainIndex> deserializeGetMediaClockReferenceInfoCommand(MvuAecpdu::Payload const& payload);

/** GET_MEDIA_CLOCK_REFERENCE_INFO Response - Milan 1.2 Clause 5.4.4.5 */
Serializer<AecpMvuGetMediaClockReferenceInfoResponsePayloadSize> serializeGetMediaClockReferenceInfoResponse(entity::model::ClockDomainIndex const clockDomainIndex, entity::MediaClockReferenceInfoFlags const flags, entity::model::DefaultMediaClockReferencePriority const defaultMcrPrio, entity::model::MediaClockReferencePriority const userMcrPrio, entity::model::AvdeccFixedString const& domainName);
std::tuple<entity::model::ClockDomainIndex, entity::MediaClockReferenceInfoFlags, entity::model::DefaultMediaClockReferencePriority, entity::model::MediaClockReferencePriority, entity::model::AvdeccFixedString> deserializeGetMediaClockReferenceInfoResponse(entity::LocalEntity::MvuCommandStatus const status, MvuAecpdu::Payload const& payload);

} // namespace la::avdecc::protocol::mvuPayload
