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
* @file entityModelControlValues.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model control descriptor values.
* @warning All structures have to be exported on unix-like systems (using LA_AVDECC_TYPE_INFO_EXPORT) so type_info is correctly visible outside the shared library (required for std::any_cast to work)
*/

#pragma once

#include "la/avdecc/memoryBuffer.hpp"

#include "entityModelTypes.hpp"
#include "exports.hpp"

#include <optional>
#include <tuple>
#include <array>
#if !defined(__GNUC__) || __GNUC__ >= 10 /* <version> is not present in earier versions of gcc (not sure which version exactly, using 10 here) */
#	include <version>
#endif

#if defined(__cpp_lib_constexpr_vector) && defined(__cpp_lib_array_constexpr)
#	define CONSTEXPR_COMPARISON constexpr
#else
#	define CONSTEXPR_COMPARISON inline
#endif

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
/** Linear Values - Clause 7.3.5.2.1 */
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType>>>
struct LA_AVDECC_TYPE_INFO_EXPORT LinearValueStatic
{
	// Comparison operator
	CONSTEXPR_COMPARISON friend bool operator==(LinearValueStatic const& lhs, LinearValueStatic const& rhs) noexcept
	{
		return lhs.minimum == rhs.minimum && lhs.maximum == rhs.maximum && lhs.step == rhs.step && lhs.defaultValue == rhs.defaultValue && lhs.unit == rhs.unit && lhs.localizedName == rhs.localizedName;
	}

	SizeType minimum{ 0 };
	SizeType maximum{ 0 };
	SizeType step{ 0 };
	SizeType defaultValue{ 0 };
	ControlValueUnit unit{ 0 };
	LocalizedStringReference localizedName{};
};

template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType>>>
struct LA_AVDECC_TYPE_INFO_EXPORT LinearValueDynamic
{
	// Comparison operator
	CONSTEXPR_COMPARISON friend bool operator==(LinearValueDynamic const& lhs, LinearValueDynamic const& rhs) noexcept
	{
		return lhs.currentValue == rhs.currentValue;
	}

	SizeType currentValue{ 0 }; // The actual default value should be the one from LinearValueStatic
};

template<typename ValueType>
class LA_AVDECC_TYPE_INFO_EXPORT LinearValues final
{
public:
	using control_value_details_traits = ControlValues::control_value_details_traits<LinearValues<ValueType>>;
	using value_type = ValueType;
	using Values = std::vector<ValueType>;
	static_assert(control_value_details_traits::is_value_details, "LinearValues, ControlValues::control_value_details_traits::is_value_details trait not defined for requested ValueType. Did you include entityModelControlValuesTraits.hpp?");

	constexpr LinearValues() noexcept {}

	explicit LinearValues(Values const& values) noexcept
		: _values{ values }
	{
	}

	explicit LinearValues(Values&& values) noexcept
		: _values{ std::move(values) }
	{
	}

	void addValue(value_type const& value) noexcept
	{
		_values.push_back(value);
	}

	void addValue(value_type&& value) noexcept
	{
		_values.emplace_back(std::move(value));
	}

	Values const& getValues() const noexcept
	{
		return _values;
	}

	Values& getValues() noexcept
	{
		return _values;
	}

	std::uint16_t countValues() const noexcept
	{
		return static_cast<std::uint16_t>(_values.size());
	}

	bool empty() const noexcept
	{
		return _values.empty();
	}

	// Comparison operator
	CONSTEXPR_COMPARISON friend bool operator==(LinearValues const& lhs, LinearValues const& rhs) noexcept
	{
		return lhs._values == rhs._values;
	}

	// Defaulted compiler auto-generated methods
	LinearValues(LinearValues const&) = default;
	LinearValues(LinearValues&&) = default;
	LinearValues& operator=(LinearValues const&) = default;
	LinearValues& operator=(LinearValues&&) = default;

private:
	Values _values{};
};

/** Selector Value - Clause 7.3.5.2.2 */
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType> || std::is_same_v<SizeType, LocalizedStringReference>>>
struct LA_AVDECC_TYPE_INFO_EXPORT SelectorValueStatic
{
	using control_value_details_traits = ControlValues::control_value_details_traits<SelectorValueStatic<SizeType>>;

	std::uint16_t countValues() const noexcept
	{
		return 1; // There is actually just one value in SELECTOR type, but multiple options
	}

	// Comparison operator
	CONSTEXPR_COMPARISON friend bool operator==(SelectorValueStatic const& lhs, SelectorValueStatic const& rhs) noexcept
	{
		return lhs.defaultValue == rhs.defaultValue && lhs.unit == rhs.unit && lhs.options == rhs.options;
	}

	SizeType defaultValue{ 0 };
	ControlValueUnit unit{ 0 };
	std::vector<SizeType> options{};
};

template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType> || std::is_same_v<SizeType, LocalizedStringReference>>>
struct LA_AVDECC_TYPE_INFO_EXPORT SelectorValueDynamic
{
	using control_value_details_traits = ControlValues::control_value_details_traits<SelectorValueDynamic<SizeType>>;

	std::uint16_t countValues() const noexcept
	{
		return 1;
	}

	// Comparison operator
	CONSTEXPR_COMPARISON friend bool operator==(SelectorValueDynamic const& lhs, SelectorValueDynamic const& rhs) noexcept
	{
		return lhs.currentValue == rhs.currentValue;
	}

	SizeType currentValue{}; // The actual default value should be the one from SelectorValueStatic
};

/** Array Values - Clause 7.3.5.2.3 */
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType>>>
struct LA_AVDECC_TYPE_INFO_EXPORT ArrayValueStatic
{
	using control_value_details_traits = ControlValues::control_value_details_traits<ArrayValueStatic<SizeType>>;

	std::uint16_t countValues() const noexcept
	{
		return 1; // Dynamic ArrayValue Types share the same Static information
	}

	// Comparison operator
	CONSTEXPR_COMPARISON friend bool operator==(ArrayValueStatic const& lhs, ArrayValueStatic const& rhs) noexcept
	{
		return lhs.minimum == rhs.minimum && lhs.maximum == rhs.maximum && lhs.step == rhs.step && lhs.defaultValue == rhs.defaultValue && lhs.unit == rhs.unit && lhs.localizedName == rhs.localizedName;
	}

	SizeType minimum{ 0 };
	SizeType maximum{ 0 };
	SizeType step{ 0 };
	SizeType defaultValue{ 0 };
	ControlValueUnit unit{ 0 };
	LocalizedStringReference localizedName{};
};

template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType>>>
struct LA_AVDECC_TYPE_INFO_EXPORT ArrayValueDynamic
{
	using control_value_details_traits = ControlValues::control_value_details_traits<ArrayValueDynamic<SizeType>>;

	std::uint16_t countValues() const noexcept
	{
		return static_cast<std::uint16_t>(currentValues.size());
	}

	// Comparison operator
	CONSTEXPR_COMPARISON friend bool operator==(ArrayValueDynamic const& lhs, ArrayValueDynamic const& rhs) noexcept
	{
		return lhs.currentValues == rhs.currentValues;
	}

	std::vector<SizeType> currentValues{}; // The actual default value should be the one from ArrayValueStatic
};

/** UTF-8 String Value - Clause 7.3.5.2.4 */
struct LA_AVDECC_TYPE_INFO_EXPORT UTF8StringValueStatic
{
	static constexpr size_t MaxLength = 406;

	std::uint16_t countValues() const noexcept
	{
		return 1;
	}

	// Comparison operator
	CONSTEXPR_COMPARISON friend bool operator==(UTF8StringValueStatic const& /*lhs*/, UTF8StringValueStatic const& /*rhs*/) noexcept
	{
		return true;
	}
};
struct LA_AVDECC_TYPE_INFO_EXPORT UTF8StringValueDynamic
{
	using value_type = std::uint8_t;
	using Values = std::array<std::uint8_t, UTF8StringValueStatic::MaxLength>;

	std::uint16_t countValues() const noexcept
	{
		return 1;
	}

	// Comparison operator
	CONSTEXPR_COMPARISON friend bool operator==(UTF8StringValueDynamic const& lhs, UTF8StringValueDynamic const& rhs) noexcept
	{
		return lhs.currentValue == rhs.currentValue;
	}

	Values currentValue{};
};

enum class ControlValuesValidationResult
{
	Valid,
	NoStaticValues, /**< static values not initialized */
	WrongStaticValuesType, /**< static values of incorrect type (ie. dynamic) */
	NoDynamicValues, /**< dynamic values not initialized */
	WrongDynamicValuesType, /**< dynamic values of incorrect type (ie. static) */
	StaticDynamicTypeMismatch, /**< Type mismatch between static and dynamic values */
	StaticDynamicCountMismatch, /**< Count mismatch between static and dynamic values */
	CurrentValueBelowMinimum, /**< 'currentValue' is below 'minimum' */
	CurrentValueAboveMaximum, /**< 'currentValue' is above 'maximum' */
	CurrentValueNotMultipleOfStep, /**< 'currentValue' is not a multiple of 'step' */
	CurrentValueNotInOptions, /**< 'currentValue' is not in 'options' */
	CurrentValueNotNullTerminated, /**< 'currentValue' is not null terminated */
	InvalidPackedValues = 98, /**< Packed values are invalid */
	NotSupported = 99, /**< Validation not supported for this ControlValueType */
};

LA_AVDECC_API std::optional<ControlValues> LA_AVDECC_CALL_CONVENTION unpackDynamicControlValues(MemoryBuffer const& packedControlValues, ControlValueType::Type const valueType, std::uint16_t const numberOfValues) noexcept;
LA_AVDECC_API std::tuple<ControlValuesValidationResult, std::string> LA_AVDECC_CALL_CONVENTION validateControlValues(ControlValues const& staticValues, ControlValues const& dynamicValues) noexcept;

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la

#undef CONSTEXPR_COMPARISON
