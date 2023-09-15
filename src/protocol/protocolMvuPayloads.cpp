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
* @file protocolMvuPayloads.cpp
* @author Christophe Calmejane
*/

#include "protocolMvuPayloads.hpp"
#include "logHelper.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace mvuPayload
{
/** GET_MILAN_INFO Command - Milan-2019 Clause 7.4.1 */
Serializer<AecpMvuGetMilanInfoCommandPayloadSize> serializeGetMilanInfoCommand()
{
	Serializer<AecpMvuGetMilanInfoCommandPayloadSize> ser;
	std::uint16_t const reserved{ 0u };

	ser << reserved;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

void deserializeGetMilanInfoCommand(MvuAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpMvuGetMilanInfoCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	std::uint16_t reserved{ 0u };

	des >> reserved;

	AVDECC_ASSERT(des.usedBytes() == AecpMvuGetMilanInfoCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return;
}

/** GET_MILAN_INFO Response - Milan-2019 Clause 7.4.1 */
Serializer<AecpMvuGetMilanInfoResponsePayloadSize> serializeGetMilanInfoResponse(entity::model::MilanInfo const& info)
{
	Serializer<AecpMvuGetMilanInfoResponsePayloadSize> ser;
	std::uint16_t const reserved{ 0u };

	ser << reserved;
	ser << info.protocolVersion << info.featuresFlags << info.certificationVersion;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::MilanInfo> deserializeGetMilanInfoResponse(MvuAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpMvuGetMilanInfoResponsePayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	std::uint16_t reserved{ 0u };
	entity::model::MilanInfo info{};

	des >> reserved;
	des >> info.protocolVersion >> info.featuresFlags >> info.certificationVersion;

	AVDECC_ASSERT(des.usedBytes() == AecpMvuGetMilanInfoResponsePayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(info);
}


} // namespace mvuPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
