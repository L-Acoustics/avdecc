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
static inline void createUnpackDynamicControlValuesDispatchTable(std::unordered_map<ControlValueType::Type, std::function<ControlValues(Deserializer&, std::uint16_t)>>& dispatchTable)
{
	/** Linear Values - IEEE1722.1-2013 Clause 7.3.5.2.1 */
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

	/** Selector Value - IEEE1722.1-2013 Clause 7.3.5.2.2 */
	dispatchTable[ControlValueType::Type::ControlSelectorInt8] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlSelectorInt8>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlSelectorUInt8] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlSelectorUInt8>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlSelectorInt16] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlSelectorInt16>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlSelectorUInt16] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlSelectorUInt16>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlSelectorInt32] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlSelectorInt32>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlSelectorUInt32] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlSelectorUInt32>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlSelectorInt64] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlSelectorInt64>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlSelectorUInt64] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlSelectorUInt64>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlSelectorFloat] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlSelectorFloat>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlSelectorDouble] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlSelectorDouble>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlSelectorString] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlSelectorString>::unpackDynamicControlValues;

	/** Array Values - IEEE1722.1-2013 Clause 7.3.5.2.3 */
	dispatchTable[ControlValueType::Type::ControlArrayInt8] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlArrayInt8>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlArrayUInt8] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlArrayUInt8>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlArrayInt16] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlArrayInt16>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlArrayUInt16] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlArrayUInt16>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlArrayInt32] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlArrayInt32>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlArrayUInt32] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlArrayUInt32>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlArrayInt64] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlArrayInt64>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlArrayUInt64] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlArrayUInt64>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlArrayFloat] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlArrayFloat>::unpackDynamicControlValues;
	dispatchTable[ControlValueType::Type::ControlArrayDouble] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlArrayDouble>::unpackDynamicControlValues;

	/** UTF-8 String Value - IEEE1722.1-2013 Clause 7.3.5.2.4 */
	dispatchTable[ControlValueType::Type::ControlUtf8] = protocol::aemPayload::control_values_payload_traits<ControlValueType::Type::ControlUtf8>::unpackDynamicControlValues;
}

std::optional<ControlValues> LA_AVDECC_CALL_CONVENTION unpackDynamicControlValues(MemoryBuffer const& packedControlValues, ControlValueType::Type const valueType, std::uint16_t const numberOfValues) noexcept
{
	static auto s_Dispatch = std::unordered_map<ControlValueType::Type, std::function<ControlValues(Deserializer&, std::uint16_t)>>{};

	if (s_Dispatch.empty())
	{
		// Create the dispatch table
		createUnpackDynamicControlValuesDispatchTable(s_Dispatch);
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
			// We still want to return a ControlValues object, but it will be invalid (it's not the device's fault if the ControlValueType is not supported by the library)
			return ControlValues{};
		}
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_AEM_PAYLOAD_TRACE("unpackDynamicControlValues error: Cannot unpack ControlValueType {}: {}", controlValueTypeToString(valueType), e.what());
	}
	return {};
}

static inline void createValidateControlValuesDispatchTable(std::unordered_map<entity::model::ControlValueType::Type, std::function<std::tuple<ControlValuesValidationResult, std::string>(entity::model::ControlValues const&, entity::model::ControlValues const&)>>& dispatchTable)
{
	/** Linear Values - IEEE1722.1-2013 Clause 7.3.5.2.1 */
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearInt8] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt8>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearUInt8] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt8>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearInt16] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt16>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearUInt16] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt16>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearInt32] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt32>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearUInt32] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt32>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearInt64] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt64>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearUInt64] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt64>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearFloat] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearFloat>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlLinearDouble] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearDouble>::validateControlValues;

	/** Selector Value - IEEE1722.1-2013 Clause 7.3.5.2.2 */
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorInt8] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt8>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorUInt8] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt8>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorInt16] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt16>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorUInt16] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt16>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorInt32] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt32>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorUInt32] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt32>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorInt64] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt64>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorUInt64] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt64>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorFloat] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorFloat>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorDouble] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorDouble>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlSelectorString] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorString>::validateControlValues;

	/** Array Values - IEEE1722.1-2013 Clause 7.3.5.2.3 */
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayInt8] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt8>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayUInt8] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt8>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayInt16] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt16>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayUInt16] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt16>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayInt32] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt32>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayUInt32] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt32>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayInt64] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt64>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayUInt64] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt64>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayFloat] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayFloat>::validateControlValues;
	dispatchTable[entity::model::ControlValueType::Type::ControlArrayDouble] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayDouble>::validateControlValues;

	/** UTF-8 String Value - IEEE1722.1-2013 Clause 7.3.5.2.4 */
	dispatchTable[entity::model::ControlValueType::Type::ControlUtf8] = protocol::aemPayload::control_values_payload_traits<entity::model::ControlValueType::Type::ControlUtf8>::validateControlValues;
}

std::tuple<ControlValuesValidationResult, std::string> LA_AVDECC_CALL_CONVENTION validateControlValues(ControlValues const& staticValues, ControlValues const& dynamicValues) noexcept
{
	static auto s_Dispatch = std::unordered_map<entity::model::ControlValueType::Type, std::function<std::tuple<ControlValuesValidationResult, std::string>(entity::model::ControlValues const&, entity::model::ControlValues const&)>>{};

	if (s_Dispatch.empty())
	{
		// Create the dispatch table
		createValidateControlValuesDispatchTable(s_Dispatch);
	}

	if (!staticValues)
	{
		return std::make_tuple(ControlValuesValidationResult::NoStaticValues, "StaticValues are not initialized");
	}

	if (staticValues.areDynamicValues())
	{
		return std::make_tuple(ControlValuesValidationResult::WrongStaticValuesType, "StaticValues are dynamic instead of static");
	}

	if (!dynamicValues)
	{
		return std::make_tuple(ControlValuesValidationResult::NoDynamicValues, "DynamicValues are not initialized");
	}

	if (!dynamicValues.areDynamicValues())
	{
		return std::make_tuple(ControlValuesValidationResult::WrongDynamicValuesType, "DynamicValues are static instead of dynamic");
	}

	auto const valueType = staticValues.getType();
	if (valueType != dynamicValues.getType())
	{
		return std::make_tuple(ControlValuesValidationResult::StaticDynamicTypeMismatch, "DynamicValues type does not match StaticValues type");
	}

	if (staticValues.countMustBeIdentical() != dynamicValues.countMustBeIdentical())
	{
		return std::make_tuple(ControlValuesValidationResult::StaticDynamicCountMismatch, "Values countMustBeIdentical() does not match (" + std::to_string(staticValues.countMustBeIdentical()) + " for static values, " + std::to_string(dynamicValues.countMustBeIdentical()) + " for dynamic ones)");
	}

	if (staticValues.countMustBeIdentical() && staticValues.size() != dynamicValues.size())
	{
		return std::make_tuple(ControlValuesValidationResult::StaticDynamicCountMismatch, "Values count does not match (" + std::to_string(staticValues.size()) + " static values, " + std::to_string(dynamicValues.size()) + " dynamic ones)");
	}

	auto const& it = s_Dispatch.find(valueType);
	if (AVDECC_ASSERT_WITH_RET(it != s_Dispatch.end(), "validateControlValues not handled for this values type"))
	{
		return it->second(staticValues, dynamicValues);
	}

	// In case we don't handle this kind of ControlType, just validate the values
	return std::make_tuple(ControlValuesValidationResult::Valid, "");
}

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
