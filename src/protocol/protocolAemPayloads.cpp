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

Serializer<protocol::AecpAemSetNameCommandPayloadSize> serializeSetNameCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString name)
{
	Serializer<protocol::AecpAemSetNameCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;
	ser << nameIndex << configurationIndex;
	ser << name;

	return ser;
}

Serializer<protocol::AecpAemGetNameCommandPayloadSize> serializeGetNameCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex)
{
	Serializer<protocol::AecpAemGetNameCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;
	ser << nameIndex << configurationIndex;

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex, entity::model::AvdeccFixedString> deserializeSetNameResponse(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < protocol::AecpAemSetNameResponsePayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException("Invalid payload size for SET_NAME aem response");

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType;
	entity::model::DescriptorIndex descriptorIndex;
	std::uint16_t nameIndex;
	entity::model::ConfigurationIndex configurationIndex;
	entity::model::AvdeccFixedString name;

	des >> descriptorType >> descriptorIndex;
	des >> nameIndex >> configurationIndex >> name;

	assert(des.usedBytes() == protocol::AecpAemSetNameResponsePayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, nameIndex, configurationIndex, name);
}

} // namespace aemPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
