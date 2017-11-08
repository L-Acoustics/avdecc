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

// SET_NAME Command - Clause 7.4.17.1
Serializer<protocol::AecpAemSetNameCommandPayloadSize> serializeSetNameCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString name)
{
	Serializer<protocol::AecpAemSetNameCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;
	ser << nameIndex << configurationIndex;
	ser << name;

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex, entity::model::AvdeccFixedString> deserializeSetNameCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < protocol::AecpAemSetNameCommandPayloadSize) // Malformed packet
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

	assert(des.usedBytes() == protocol::AecpAemSetNameCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, nameIndex, configurationIndex, name);
}

// SET_NAME Response - Clause 7.4.17.1
Serializer<protocol::AecpAemSetNameResponsePayloadSize> serializeSetNameResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString name)
{
	// Same as SET_NAME Command
	static_assert(AecpAemSetNameResponsePayloadSize == AecpAemSetNameCommandPayloadSize, "SET_NAME Response no longer the same as SET_NAME Command");
	return serializeSetNameCommand(std::move(descriptorType), std::move(descriptorIndex), std::move(nameIndex), std::move(configurationIndex), std::move(name));
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex, entity::model::AvdeccFixedString> deserializeSetNameResponse(AemAecpdu::Payload const& payload)
{
	// Same as SET_NAME Command
	static_assert(AecpAemSetNameResponsePayloadSize == AecpAemSetNameCommandPayloadSize, "SET_NAME Response no longer the same as SET_NAME Command");
	return deserializeSetNameCommand(payload);
}

// GET_NAME Command - Clause 7.4.18.1
Serializer<protocol::AecpAemGetNameCommandPayloadSize> serializeGetNameCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex)
{
	Serializer<protocol::AecpAemGetNameCommandPayloadSize> ser;

	ser << descriptorType << descriptorIndex;
	ser << nameIndex << configurationIndex;

	return ser;
}

std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex> deserializeGetNameCommand(AemAecpdu::Payload const& payload)
{
	auto* const commandPayload = payload.first;
	auto const commandPayloadLength = payload.second;

	if (commandPayload == nullptr || commandPayloadLength < protocol::AecpAemGetNameCommandPayloadSize) // Malformed packet
		throw IncorrectPayloadSizeException();

	// Check payload
	Deserializer des(commandPayload, commandPayloadLength);
	entity::model::DescriptorType descriptorType;
	entity::model::DescriptorIndex descriptorIndex;
	std::uint16_t nameIndex;
	entity::model::ConfigurationIndex configurationIndex;

	des >> descriptorType >> descriptorIndex;
	des >> nameIndex >> configurationIndex;

	assert(des.usedBytes() == protocol::AecpAemGetNameCommandPayloadSize && "Used more bytes than specified in protocol constant");

	return std::make_tuple(descriptorType, descriptorIndex, nameIndex, configurationIndex);
}

// GET_NAME Response - Clause 7.4.17.1
Serializer<protocol::AecpAemGetNameResponsePayloadSize> serializeGetNameResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString name)
{
	// Same as SET_NAME Command
	static_assert(AecpAemGetNameResponsePayloadSize == AecpAemSetNameCommandPayloadSize, "GET_NAME Response no longer the same as SET_NAME Command");
	return serializeSetNameCommand(std::move(descriptorType), std::move(descriptorIndex), std::move(nameIndex), std::move(configurationIndex), std::move(name));
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
