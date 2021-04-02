/*
* Copyright (C) 2016-2021, L-Acoustics and its contributors

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
* @file avdeccControlledEntityJsonSerializer.hpp
* @author Christophe Calmejane
* @brief Avdecc Entity JSON Serializer.
*/

#pragma once

#include <la/avdecc/internals/jsonSerialization.hpp>

#include <nlohmann/json.hpp>

#include <string>

using json = nlohmann::json;

namespace la
{
namespace avdecc
{
namespace controller
{
class ControlledEntityImpl;
namespace jsonSerializer
{
// Serialization methods
json createJsonObject(ControlledEntityImpl const& entity, entity::model::jsonSerializer::Flags const flags); // Throws SerializationException

// Deserialization methods
void setEntityModel(ControlledEntityImpl& entity, json const& object, entity::model::jsonSerializer::Flags flags); // Throws DeserializationException
void setEntityState(ControlledEntityImpl& entity, json const& object); // Throws DeserializationException
void setEntityStatistics(ControlledEntityImpl& entity, json const& object); // Throws DeserializationException

} // namespace jsonSerializer
} // namespace controller
} // namespace avdecc
} // namespace la
