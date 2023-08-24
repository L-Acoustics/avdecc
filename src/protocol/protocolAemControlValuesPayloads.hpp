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
	static std::tuple<entity::model::ControlValuesValidationResult, std::string> validateControlValues(entity::model::ControlValues const& /*staticValues*/, entity::model::ControlValues const& /*dynamicValues*/) noexcept
	{
		return std::make_tuple(entity::model::ControlValuesValidationResult::NotSupported, "No template specialization found for this ControlValueType");
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

		// Validate there is no more data in the buffer
		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_WARN("Unpack LINEAR value warning: Remaining data in GET_CONTROL response");
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

	static std::tuple<entity::model::ControlValuesValidationResult, std::string> validateControlValues(entity::model::ControlValues const& staticValues, entity::model::ControlValues const& dynamicValues) noexcept
	{
		try
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
					return std::make_tuple(entity::model::ControlValuesValidationResult::CurrentValueBelowMinimum, "DynamicValue " + std::to_string(pos) + " is out of bounds (lower than minimum value of " + std::to_string(utils::forceNumeric(staticValue.minimum)) + "): " + std::to_string(utils::forceNumeric(dynamicValue.currentValue)));
				}
				// Check upper bound
				if (dynamicValue.currentValue > staticValue.maximum)
				{
					return std::make_tuple(entity::model::ControlValuesValidationResult::CurrentValueAboveMaximum, "DynamicValue " + std::to_string(pos) + " is out of bounds (greater than maximum value of " + std::to_string(utils::forceNumeric(staticValue.maximum)) + "): " + std::to_string(utils::forceNumeric(dynamicValue.currentValue)));
				}
				// Check step
				if (staticValue.step != value_size{ 0 })
				{
					if constexpr (std::is_integral_v<value_size>)
					{
						auto const adjustedValue = static_cast<value_size>(dynamicValue.currentValue - staticValue.minimum);
						if ((adjustedValue % staticValue.step) != 0)
						{
							return std::make_tuple(entity::model::ControlValuesValidationResult::CurrentValueNotMultipleOfStep, "DynamicValue " + std::to_string(pos) + " is not a multiple of step: " + std::to_string(utils::forceNumeric(dynamicValue.currentValue)));
						}
					}
				}
				++pos;
			}
			return std::make_tuple(entity::model::ControlValuesValidationResult::Valid, "");
		}
		catch (...)
		{
			return std::make_tuple(entity::model::ControlValuesValidationResult::InvalidPackedValues, "");
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

/** Selector Value - Clause 7.3.5.2.2 */
template<typename SizeType, typename StaticValueType = entity::model::SelectorValueStatic<SizeType>, typename DynamicValueType = entity::model::SelectorValueDynamic<SizeType>>
struct SelectorValuePayloadTraits : BaseValuesPayloadTraits<StaticValueType, DynamicValueType>
{
	static std::tuple<entity::model::ControlValues, entity::model::ControlValues> unpackFullControlValues(Deserializer& des, std::uint16_t const numberOfValues)
	{
		auto valueStatic = StaticValueType{};
		auto valueDynamic = DynamicValueType{};

		des >> valueDynamic.currentValue;
		des >> valueStatic.defaultValue;

		// For Selector Values, the number of options is the number of values
		auto const numberOfOptions = numberOfValues;
		for (auto i = 0u; i < numberOfOptions; ++i)
		{
			auto option = SizeType{};

			des >> option;

			valueStatic.options.push_back(std::move(option));
		}

		des >> valueStatic.unit;

		return std::make_tuple(entity::model::ControlValues{ std::move(valueStatic) }, entity::model::ControlValues{ std::move(valueDynamic) });
	}

	static entity::model::ControlValues unpackDynamicControlValues(Deserializer& des, std::uint16_t const /*numberOfValues*/)
	{
		// For Selector Values, the number of dynamic values is always 1
		// The number of static values is the number of options
		auto valueDynamic = DynamicValueType{};
		des >> valueDynamic.currentValue;

		// Validate there is no more data in the buffer
		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_WARN("Unpack SELECTOR value warning: Remaining data in GET_CONTROL response");
		}

		return entity::model::ControlValues{ std::move(valueDynamic) };
	}

	static void packDynamicControlValues(Serializer<AemAecpdu::MaximumSendPayloadBufferLength>& ser, entity::model::ControlValues const& values)
	{
		// For Selector Values, the number of dynamic values is always 1
		// The number of static values is the number of options
		auto const& selectorValue = values.getValues<DynamicValueType>();
		ser << selectorValue.currentValue;
	}

	static std::tuple<entity::model::ControlValuesValidationResult, std::string> validateControlValues(entity::model::ControlValues const& staticValues, entity::model::ControlValues const& dynamicValues) noexcept
	{
		try
		{
			auto const& staticSelectorValue = staticValues.getValues<StaticValueType>();
			auto const& dynamicSelectorValue = dynamicValues.getValues<DynamicValueType>();
			auto const dynamicValue = dynamicSelectorValue.currentValue;

			// Check that the current dynamic value is in the list of possible options
			if (std::find(staticSelectorValue.options.begin(), staticSelectorValue.options.end(), dynamicValue) == staticSelectorValue.options.end())
			{
				if constexpr (std::is_same_v<SizeType, entity::model::LocalizedStringReference>)
				{
					return std::make_tuple(entity::model::ControlValuesValidationResult::CurrentValueNotInOptions, "DynamicValue " + std::to_string(utils::forceNumeric(dynamicValue.getValue())) + " is not in the list of possible values");
				}
				else
				{
					return std::make_tuple(entity::model::ControlValuesValidationResult::CurrentValueNotInOptions, "DynamicValue " + std::to_string(utils::forceNumeric(dynamicValue)) + " is not in the list of possible values");
				}
			}

			return std::make_tuple(entity::model::ControlValuesValidationResult::Valid, "");
		}
		catch (...)
		{
			return std::make_tuple(entity::model::ControlValuesValidationResult::InvalidPackedValues, "");
		}
	}
};

template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt8> : SelectorValuePayloadTraits<std::int8_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt8> : SelectorValuePayloadTraits<std::uint8_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt16> : SelectorValuePayloadTraits<std::int16_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt16> : SelectorValuePayloadTraits<std::uint16_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt32> : SelectorValuePayloadTraits<std::int32_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt32> : SelectorValuePayloadTraits<std::uint32_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorInt64> : SelectorValuePayloadTraits<std::int64_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorUInt64> : SelectorValuePayloadTraits<std::uint64_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorFloat> : SelectorValuePayloadTraits<float>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorDouble> : SelectorValuePayloadTraits<double>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlSelectorString> : SelectorValuePayloadTraits<entity::model::LocalizedStringReference>
{
};

/** Array Values - Clause 7.3.5.2.3 */
template<typename SizeType, typename StaticValueType = entity::model::ArrayValueStatic<SizeType>, typename DynamicValueType = entity::model::ArrayValueDynamic<SizeType>>
struct ArrayValuesPayloadTraits : BaseValuesPayloadTraits<StaticValueType, DynamicValueType>
{
	static std::tuple<entity::model::ControlValues, entity::model::ControlValues> unpackFullControlValues(Deserializer& des, std::uint16_t const numberOfValues)
	{
		auto valueStatic = StaticValueType{};
		auto valuesDynamic = DynamicValueType{};

		des >> valueStatic.minimum >> valueStatic.maximum >> valueStatic.step >> valueStatic.defaultValue >> valueStatic.unit >> valueStatic.localizedName;

		for (auto i = 0u; i < numberOfValues; ++i)
		{
			auto valueDynamic = SizeType{};

			des >> valueDynamic;

			valuesDynamic.currentValues.push_back(std::move(valueDynamic));
		}

		return std::make_tuple(entity::model::ControlValues{ std::move(valueStatic) }, entity::model::ControlValues{ std::move(valuesDynamic) });
	}

	static entity::model::ControlValues unpackDynamicControlValues(Deserializer& des, std::uint16_t const numberOfValues)
	{
		auto valuesDynamic = DynamicValueType{};

		for (auto i = 0u; i < numberOfValues; ++i)
		{
			auto valueDynamic = SizeType{};

			des >> valueDynamic;

			valuesDynamic.currentValues.push_back(std::move(valueDynamic));
		}

		// Validate there is no more data in the buffer
		if (des.remaining() != 0)
		{
			LOG_AEM_PAYLOAD_WARN("Unpack ARRAY value warning: Remaining data in GET_CONTROL response");
		}

		return entity::model::ControlValues{ std::move(valuesDynamic) };
	}

	static void packDynamicControlValues(Serializer<AemAecpdu::MaximumSendPayloadBufferLength>& ser, entity::model::ControlValues const& values)
	{
		auto const arrayValues = values.getValues<DynamicValueType>(); // We have to store the copy or it will go out of scope if using it directly in the range-based loop
		for (auto const& val : arrayValues.currentValues)
		{
			ser << val;
		}
	}

	static std::tuple<entity::model::ControlValuesValidationResult, std::string> validateControlValues(entity::model::ControlValues const& staticValues, entity::model::ControlValues const& dynamicValues) noexcept
	{
		try
		{
			using value_size = typename DynamicValueType::control_value_details_traits::size_type;

			auto pos = decltype(std::declval<decltype(staticValues)>().size()){ 0u };

			auto const& staticArrayValue = staticValues.getValues<StaticValueType>();
			auto const dynamicArrayValues = dynamicValues.getValues<DynamicValueType>(); // We have to store the copy or it will go out of scope

			for (auto const& dynamicValue : dynamicArrayValues.currentValues)
			{
				// Check lower bound
				if (dynamicValue < staticArrayValue.minimum)
				{
					return std::make_tuple(entity::model::ControlValuesValidationResult::CurrentValueBelowMinimum, "DynamicValue " + std::to_string(pos) + " is out of bounds (lower than minimum value of " + std::to_string(utils::forceNumeric(staticArrayValue.minimum)) + "): " + std::to_string(utils::forceNumeric(dynamicValue)));
				}
				// Check upper bound
				if (dynamicValue > staticArrayValue.maximum)
				{
					return std::make_tuple(entity::model::ControlValuesValidationResult::CurrentValueAboveMaximum, "DynamicValue " + std::to_string(pos) + " is out of bounds (greater than maximum value of " + std::to_string(utils::forceNumeric(staticArrayValue.maximum)) + "): " + std::to_string(utils::forceNumeric(dynamicValue)));
				}
				// Check step
				if (staticArrayValue.step != value_size{ 0 })
				{
					if constexpr (std::is_integral_v<value_size>)
					{
						auto const adjustedValue = static_cast<value_size>(dynamicValue - staticArrayValue.minimum);
						if ((adjustedValue % staticArrayValue.step) != 0)
						{
							return std::make_tuple(entity::model::ControlValuesValidationResult::CurrentValueNotMultipleOfStep, "DynamicValue " + std::to_string(pos) + " is not a multiple of step: " + std::to_string(utils::forceNumeric(dynamicValue)));
						}
					}
				}
				++pos;
			}
			return std::make_tuple(entity::model::ControlValuesValidationResult::Valid, "");
		}
		catch (...)
		{
			return std::make_tuple(entity::model::ControlValuesValidationResult::InvalidPackedValues, "");
		}
	}
};

template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt8> : ArrayValuesPayloadTraits<std::int8_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt8> : ArrayValuesPayloadTraits<std::uint8_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt16> : ArrayValuesPayloadTraits<std::int16_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt16> : ArrayValuesPayloadTraits<std::uint16_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt32> : ArrayValuesPayloadTraits<std::int32_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt32> : ArrayValuesPayloadTraits<std::uint32_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayInt64> : ArrayValuesPayloadTraits<std::int64_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayUInt64> : ArrayValuesPayloadTraits<std::uint64_t>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayFloat> : ArrayValuesPayloadTraits<float>
{
};
template<>
struct control_values_payload_traits<entity::model::ControlValueType::Type::ControlArrayDouble> : ArrayValuesPayloadTraits<double>
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

		auto utf8Values = values.getValues<entity::model::UTF8StringValueDynamic>();
		auto constexpr maxLength = utf8Values.currentValue.size();
		auto constexpr nullCharacter = decltype(utf8Values)::value_type{ 0u };

		// Count the number of bytes to copy (including the trailing NULL)
		auto length = size_t{ 0 };
		for (auto const c : utf8Values.currentValue)
		{
			++length;
			if (c == nullCharacter)
			{
				break;
			}
		}

		// Validate NULL terminated string (we processed the whole std::array without encountering a single NULL character)
		if (length == maxLength && utf8Values.currentValue[maxLength - 1] != nullCharacter)
		{
			LOG_AEM_PAYLOAD_WARN("pack CONTROL value warning: UTF-8 string is not NULL terminated (Clause 7.3.5.2.4)");
			utf8Values.currentValue[maxLength - 1] = nullCharacter;
		}

		ser.packBuffer(utf8Values.currentValue.data(), length);
	}

	static std::tuple<entity::model::ControlValuesValidationResult, std::string> validateControlValues(entity::model::ControlValues const& /*staticValues*/, entity::model::ControlValues const& dynamicValues) noexcept
	{
		try
		{
			// Check for trailing NULL character
			auto utf8Values = dynamicValues.getValues<entity::model::UTF8StringValueDynamic>();
			auto constexpr nullCharacter = decltype(utf8Values)::value_type{ 0u };

			auto foundNullChar = false;
			for (auto const c : utf8Values.currentValue)
			{
				if (c == nullCharacter)
				{
					foundNullChar = true;
					break;
				}
			}

			if (!foundNullChar)
			{
				return std::make_tuple(entity::model::ControlValuesValidationResult::CurrentValueNotNullTerminated, "UTF-8 string is not NULL terminated (Clause 7.3.5.2.4)");
			}

			return std::make_tuple(entity::model::ControlValuesValidationResult::Valid, "");
		}
		catch (...)
		{
			return std::make_tuple(entity::model::ControlValuesValidationResult::InvalidPackedValues, "");
		}
	}
};

} // namespace aemPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
