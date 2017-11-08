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
* @file protocolAemPayloads.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "protocolAemPayloadSizes.hpp"
#include "serialization.hpp"
#include "la/avdecc/internals/entityModel.hpp"
#include <cstdint>
#include <tuple>

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace aemPayload
{

class IncorrectPayloadSizeException final : public Exception
{
public:
	IncorrectPayloadSizeException(char const* const message) : Exception(message) {}
};


// All serialization methods might throw a std::exception if serialization goes wrong

Serializer<protocol::AecpAemSetNameCommandPayloadSize> serializeSetNameCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString name);
//serializeSetNameResponse
Serializer<protocol::AecpAemGetNameCommandPayloadSize> serializeGetNameCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex);
//serializeGetNameResponse

//deserializeSetNameCommand
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex, entity::model::AvdeccFixedString> deserializeSetNameResponse(AemAecpdu::Payload const& payload);

} // namespace aemPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
