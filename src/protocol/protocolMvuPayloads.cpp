/*
* Copyright (C) 2016-2018, L-Acoustics and its contributors

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

/** GET_MILAN_INFO Command - Milan Clause 7.4.1 */
Serializer<AecpMvuGetMilanInfoCommandPayloadSize> serializeGetMilanInfoCommand(entity::model::ConfigurationIndex const configurationIndex)
{
	Serializer<AecpMvuGetMilanInfoCommandPayloadSize> ser;

	ser << configurationIndex;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::ConfigurationIndex> deserializeGetMilanInfoCommand(MvuAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpMvuGetMilanInfoCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::ConfigurationIndex configurationIndex{ 0u };

	des >> configurationIndex;

	AVDECC_ASSERT(des.usedBytes() == AecpMvuGetMilanInfoCommandPayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(configurationIndex);
}

/** GET_MILAN_INFO Response - Milan Clause 7.4.1 */
Serializer<AecpMvuGetMilanInfoResponsePayloadSize> serializeGetMilanInfoResponse(entity::model::ConfigurationIndex const configurationIndex, std::uint32_t const protocolVersion, MvuFeaturesFlags const featuresFlags, std::uint32_t const certificationVersion)
{
	Serializer<AecpMvuGetMilanInfoResponsePayloadSize> ser;

	ser << configurationIndex;
	ser << protocolVersion << featuresFlags << certificationVersion;

	AVDECC_ASSERT(ser.usedBytes() == ser.capacity(), "Used bytes do not match the protocol constant");

	return ser;
}

std::tuple<entity::model::ConfigurationIndex, std::uint32_t, MvuFeaturesFlags, std::uint32_t> deserializeGetMilanInfoResponse(MvuAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < AecpMvuGetMilanInfoResponsePayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::ConfigurationIndex configurationIndex{ 0u };
	std::uint32_t protocolVersion{ 0u };
	MvuFeaturesFlags featuresFlags{ MvuFeaturesFlags::None };
	std::uint32_t certificationVersion{ 0u };

	des >> configurationIndex;
	des >> protocolVersion >> featuresFlags >> certificationVersion;

	AVDECC_ASSERT(des.usedBytes() == AecpMvuGetMilanInfoResponsePayloadSize, "Used more bytes than specified in protocol constant");

	return std::make_tuple(configurationIndex, protocolVersion, featuresFlags, certificationVersion);
}


} // namespace mvuPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
