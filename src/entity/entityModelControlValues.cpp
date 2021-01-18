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
* @file entityModelControlValues.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/serialization.hpp"

#include "logHelper.hpp"
#include "protocol/protocolAemControlValuesPayloads.hpp"

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
static inline void createUnpackFullControlValuesDispatchTable(std::unordered_map<ControlValueType::Type, std::function<ControlValues(Deserializer&, std::uint16_t)>>& dispatchTable)
{
	dispatchTable[ControlValueType::Type::ControlLinearInt8] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlLinearInt8>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlLinearUInt8] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlLinearUInt8>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlLinearInt16] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlLinearInt16>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlLinearUInt16] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlLinearUInt16>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlLinearInt32] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlLinearInt32>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlLinearUInt32] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlLinearUInt32>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlLinearInt64] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlLinearInt64>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlLinearUInt64] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlLinearUInt64>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlLinearFloat] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlLinearFloat>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlLinearDouble] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlLinearDouble>::unpackDynamicControlValues;
}

std::optional<ControlValues> LA_AVDECC_CALL_CONVENTION unpackDynamicControlValues(MemoryBuffer const& packedControlValues, ControlValueType::Type const valueType, std::uint16_t const numberOfValues) noexcept
{
	static auto s_Dispatch = std::unordered_map<ControlValueType::Type, std::function<ControlValues(Deserializer&, std::uint16_t)>>{};

	if (s_Dispatch.empty())
	{
		// Create the dispatch table
		createUnpackFullControlValuesDispatchTable(s_Dispatch);
	}

	try
	{
		if (auto const& it = s_Dispatch.find(valueType); it != s_Dispatch.end())
		{
			auto des = Deserializer{ packedControlValues };
			return it->second(des, numberOfValues);
		}
		else
		{
			LOG_AEM_PAYLOAD_TRACE("unpackDynamicControlValues warning: Unsupported ControlValueType: {}", valueType);
		}
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_AEM_PAYLOAD_TRACE("unpackDynamicControlValues error: Cannot unpack ControlValueType {}: {}", controlValueTypeToString(valueType), e.what());
	}
	return {};
}

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
