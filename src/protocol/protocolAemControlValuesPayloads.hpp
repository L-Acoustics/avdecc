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
* @file protocolAemControlValuesPayloads.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/entityModelControlValues.hpp"
#include "la/avdecc/internals/entityModelControlValuesTraits.hpp"
#include "la/avdecc/internals/protocolAemAecpdu.hpp"
#include "la/avdecc/internals/serialization.hpp"

#include <tuple>

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace aemPayload
{
/** Base ControlValues Payload Traits */
template<entity::model::ControlValueType::Type Type>
struct control_values_payload_traits
{
	static std::tuple<entity::model::ControlValues, entity::model::ControlValues> unpackFullControlValues(Deserializer& /*des*/, std::uint16_t const /*numberOfValues*/)
	{
		throw std::invalid_argument("No template specialization found for this ControlValueType");
	}
	static entity::model::ControlValues unpackDynamicControlValues(Deserializer& /*des*/, std::uint16_t const /*numberOfValues*/)
	{
		throw std::invalid_argument("No template specialization found for this ControlValueType");
	}
	static void packDynamicControlValues(Serializer<AemAecpdu::MaximumSendPayloadBufferLength>& /*ser*/, entity::model::ControlValues const& /*values*/)
	{
		throw std::invalid_argument("No template specialization found for this ControlValueType");
	}
	static constexpr entity::model::ControlValueType::Type control_value_type = Type;
};

template<class StaticValueType, class DynamicValueType>
struct BaseValuesPayloadTraits
{
	using StaticTraits = entity::model::ControlValues::control_value_details_traits<StaticValueType>;
	using DynamicTraits = entity::model::ControlValues::control_value_details_traits<DynamicValueType>;
	static_assert(!StaticTraits::is_dynamic, "LinearValuesPayloadTraits, StaticValueType is not a static type");
	static_assert(DynamicTraits::is_dynamic, "LinearValuesPayloadTraits, DynamicValueType is not a dynamic type");
	static_assert(std::is_same_v<typename StaticTraits::size_type, typename DynamicTraits::size_type>, "LinearValuesPayloadTraits, StaticValueType and DynamicValueType syze_type does not match");
};

/** Linear Values - Clause 7.3.5.2.1 */
template<class StaticValueType, class DynamicValueType>
struct LinearValuesPayloadTraits : BaseValuesPayloadTraits<StaticValueType, DynamicValueType>
{
	static std::tuple<entity::model::ControlValues, entity::model::ControlValues> unpackFullControlValues(Deserializer& des, std::uint16_t const numberOfValues)
	{
		auto valuesStatic = StaticValueType{};
		auto valuesDynamic = DynamicValueType{};

		for (auto i = 0u; i < numberOfValues; ++i)
		{
			auto valueStatic = typename decltype(valuesStatic)::value_type{};
			auto valueDynamic = typename decltype(valuesDynamic)::value_type{};

			des >> valueStatic.minimum >> valueStatic.maximum >> valueStatic.step >> valueStatic.defaultValue >> valueDynamic.currentValue >> valueStatic.unit >> valueStatic.localizedName;

			valuesStatic.addValue(std::move(valueStatic));
			valuesDynamic.addValue(std::move(valueDynamic));
		}

		return std::make_tuple(entity::model::ControlValues{ std::move(valuesStatic) }, entity::model::ControlValues{ std::move(valuesDynamic) });
	}

	static entity::model::ControlValues unpackDynamicControlValues(Deserializer& des, std::uint16_t const numberOfValues)
	{
		auto valuesDynamic = DynamicValueType{};

		for (auto i = 0u; i < numberOfValues; ++i)
		{
			auto valueDynamic = typename decltype(valuesDynamic)::value_type{};

			des >> valueDynamic.currentValue;

			valuesDynamic.addValue(std::move(valueDynamic));
		}

		return entity::model::ControlValues{ std::move(valuesDynamic) };
	}

	static void packDynamicControlValues(Serializer<AemAecpdu::MaximumSendPayloadBufferLength>& ser, entity::model::ControlValues const& values)
	{
		auto const linearValues = values.getValues<DynamicValueType>(); // We have to store the copie or it will go out of scope if using it directly in the range-based loop
		for (auto const& val : linearValues.getValues())
		{
			ser << val.currentValue;
		}
	}
};

template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt8> : LinearValuesPayloadTraits<entity::model::LinearValues<entity::model::LinearValueStatic<std::int8_t>>, entity::model::LinearValues<entity::model::LinearValueDynamic<std::int8_t>>>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt8> : LinearValuesPayloadTraits<entity::model::LinearValues<entity::model::LinearValueStatic<std::uint8_t>>, entity::model::LinearValues<entity::model::LinearValueDynamic<std::uint8_t>>>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt16> : LinearValuesPayloadTraits<entity::model::LinearValues<entity::model::LinearValueStatic<std::int16_t>>, entity::model::LinearValues<entity::model::LinearValueDynamic<std::int16_t>>>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt16> : LinearValuesPayloadTraits<entity::model::LinearValues<entity::model::LinearValueStatic<std::uint16_t>>, entity::model::LinearValues<entity::model::LinearValueDynamic<std::uint16_t>>>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt32> : LinearValuesPayloadTraits<entity::model::LinearValues<entity::model::LinearValueStatic<std::int32_t>>, entity::model::LinearValues<entity::model::LinearValueDynamic<std::int32_t>>>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt32> : LinearValuesPayloadTraits<entity::model::LinearValues<entity::model::LinearValueStatic<std::uint32_t>>, entity::model::LinearValues<entity::model::LinearValueDynamic<std::uint32_t>>>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearInt64> : LinearValuesPayloadTraits<entity::model::LinearValues<entity::model::LinearValueStatic<std::int64_t>>, entity::model::LinearValues<entity::model::LinearValueDynamic<std::int64_t>>>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearUInt64> : LinearValuesPayloadTraits<entity::model::LinearValues<entity::model::LinearValueStatic<std::uint64_t>>, entity::model::LinearValues<entity::model::LinearValueDynamic<std::uint64_t>>>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearFloat> : LinearValuesPayloadTraits<entity::model::LinearValues<entity::model::LinearValueStatic<float>>, entity::model::LinearValues<entity::model::LinearValueDynamic<float>>>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlLinearDouble> : LinearValuesPayloadTraits<entity::model::LinearValues<entity::model::LinearValueStatic<double>>, entity::model::LinearValues<entity::model::LinearValueDynamic<double>>>
{
};


} // namespace aemPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
