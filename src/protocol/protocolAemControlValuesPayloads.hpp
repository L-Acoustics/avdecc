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

#include "logHelper.hpp"

#include "la/avdecc/internals/entityModelControlValues.hpp"
#include "la/avdecc/internals/entityModelControlValuesTraits.hpp"
#include "la/avdecc/internals/protocolAemAecpdu.hpp"
#include "la/avdecc/internals/serialization.hpp"

#include <tuple>
#include <optional>
#include <string>

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
	static std::optional<std::string> validateControlValues(entity::model::ControlValues const& /*staticValues*/, entity::model::ControlValues const& /*dynamicValues*/) noexcept
	{
		return "No template specialization found for this ControlValueType";
	}
	static constexpr entity::model::ControlValueType::Type control_value_type = Type;
};

template<class StaticValueType, class DynamicValueType>
struct BaseValuesPayloadTraits
{
	using StaticTraits = entity::model::ControlValues::control_value_details_traits<StaticValueType>;
	using DynamicTraits = entity::model::ControlValues::control_value_details_traits<DynamicValueType>;
	static_assert(!StaticTraits::is_dynamic, "StaticValueType is not a static type");
	static_assert(DynamicTraits::is_dynamic, "DynamicValueType is not a dynamic type");
	static_assert(std::is_same_v<typename StaticTraits::size_type, typename DynamicTraits::size_type>, "StaticValueType and DynamicValueType syze_type does not match");
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
		auto const linearValues = values.getValues<DynamicValueType>(); // We have to store the copy or it will go out of scope if using it directly in the range-based loop
		for (auto const& val : linearValues.getValues())
		{
			ser << val.currentValue;
		}
	}

	static std::optional<std::string> validateControlValues(entity::model::ControlValues const& staticValues, entity::model::ControlValues const& dynamicValues) noexcept
	{
		using value_size = typename DynamicValueType::control_value_details_traits::size_type;

		auto pos = decltype(std::declval<decltype(staticValues)>().size()){ 0u };

		auto const staticLinearValues = staticValues.getValues<StaticValueType>(); // We have to store the copy or it will go out of scope if using it directly in the range-based loop
		auto const dynamicLinearValues = dynamicValues.getValues<DynamicValueType>(); // We have to store the copy or it will go out of scope

		for (auto const& staticValue : staticLinearValues.getValues())
		{
			auto const& dynamicValue = dynamicLinearValues.getValues()[pos];

			// Check lower bound
			if (dynamicValue.currentValue < staticValue.minimum)
			{
				return "DynamicValue " + std::to_string(pos) + " is out of bounds (lower than minimum value of " + std::to_string(utils::forceNumeric(staticValue.minimum)) + "): " + std::to_string(utils::forceNumeric(dynamicValue.currentValue));
			}
			// Check upper bound
			if (dynamicValue.currentValue > staticValue.maximum)
			{
				return "DynamicValue " + std::to_string(pos) + " is out of bounds (greater than maximum value of " + std::to_string(utils::forceNumeric(staticValue.maximum)) + "): " + std::to_string(utils::forceNumeric(dynamicValue.currentValue));
			}
			// Check step
			if (staticValue.step != value_size{ 0 })
			{
				if constexpr (std::is_integral_v<value_size>)
				{
					auto const adjustedValue = static_cast<value_size>(dynamicValue.currentValue - staticValue.minimum);
					if ((adjustedValue % staticValue.step) != 0)
					{
						return "DynamicValue " + std::to_string(pos) + " is not a multiple of step: " + std::to_string(utils::forceNumeric(dynamicValue.currentValue));
					}
				}
			}
			++pos;
		}
		return std::nullopt;
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


/** UTF-8 String Value - Clause 7.3.5.2.4 */
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlUtf8>
{
	static std::tuple<entity::model::ControlValues, entity::model::ControlValues> unpackFullControlValues(Deserializer& des, std::uint16_t const numberOfValues)
	{
		return std::make_tuple(entity::model::ControlValues{ entity::model::UTF8StringValueStatic{} }, unpackDynamicControlValues(des, numberOfValues));
	}

	static entity::model::ControlValues unpackDynamicControlValues(Deserializer& des, std::uint16_t const numberOfValues)
	{
		if (numberOfValues != 1)
		{
			throw std::invalid_argument("CONTROL_UTF8 should only have 1 value");
		}
		auto valuesDynamic = entity::model::UTF8StringValueDynamic{};

		auto const length = des.remaining();
		if (length == 0u)
		{
			throw std::invalid_argument("CONTROL_UTF8 should have at least one byte (NULL terminated)");
		}

		if (length >= valuesDynamic.currentValue.size())
		{
			throw std::invalid_argument("CONTROL_UTF8 should not exceed " + std::to_string(valuesDynamic.currentValue.size()) + " bytes");
		}
		des.unpackBuffer(valuesDynamic.currentValue.data(), length);

		// Validate NULL terminated string
		if (valuesDynamic.currentValue[length - 1] != decltype(valuesDynamic)::value_type{ 0u })
		{
			LOG_AEM_PAYLOAD_WARN("Unpack CONTROL value warning: UTF-8 string is not NULL terminated (Clause 7.3.5.2.4)");
			valuesDynamic.currentValue[length - 1] = decltype(valuesDynamic)::value_type{ 0u };
		}

		return entity::model::ControlValues{ std::move(valuesDynamic) };
	}

	static void packDynamicControlValues(Serializer<AemAecpdu::MaximumSendPayloadBufferLength>& ser, entity::model::ControlValues const& values)
	{
		if (values.size() != 1)
		{
			throw std::invalid_argument("CONTROL_UTF8 should only have 1 value");
		}

		auto linearValues = values.getValues<entity::model::UTF8StringValueDynamic>();
		auto constexpr maxLength = linearValues.currentValue.size();
		auto constexpr nullCharacter = decltype(linearValues)::value_type{ 0u };

		// Count the number of bytes to copy (including the trailing NULL)
		auto length = size_t{ 0 };
		for (auto const c : linearValues.currentValue)
		{
			++length;
			if (c == nullCharacter)
			{
				break;
			}
		}

		// Validate NULL terminated string
		if (length == maxLength && linearValues.currentValue[maxLength - 1] != nullCharacter)
		{
			LOG_AEM_PAYLOAD_WARN("pack CONTROL value warning: UTF-8 string is not NULL terminated (Clause 7.3.5.2.4)");
			linearValues.currentValue[maxLength - 1] = nullCharacter;
		}

		ser.packBuffer(linearValues.currentValue.data(), length);
	}

	static std::optional<std::string> validateControlValues(entity::model::ControlValues const& /*staticValues*/, entity::model::ControlValues const& /*dynamicValues*/) noexcept
	{
		return std::nullopt;
	}
};

} // namespace aemPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
