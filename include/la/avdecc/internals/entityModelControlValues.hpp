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
* @file entityModelControlValues.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model control descriptor values.
*/

#pragma once

#include "la/avdecc/memoryBuffer.hpp"

#include "entityModelTypes.hpp"
#include "exports.hpp"

#include <optional>

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
struct LinearValueStatic
{
	SizeType minimum{ 0 };
	SizeType maximum{ 0 };
	SizeType step{ 0 };
	SizeType defaultValue{ 0 };
	ControlValueUnit unit{ 0 };
	LocalizedStringReference localizedName{};
};

template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType>>>
struct LinearValueDynamic
{
	SizeType currentValue{ 0 }; // The actual default value should be the one from LinearValueStatic
};

template<typename ValueType>
class LinearValues final
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

	std::uint16_t size() const noexcept
	{
		return static_cast<std::uint16_t>(_values.size());
	}

	bool empty() const noexcept
	{
		return _values.empty();
	}

	// Defaulted compiler auto-generated methods
	LinearValues(LinearValues const&) = default;
	LinearValues(LinearValues&&) = default;
	LinearValues& operator=(LinearValues const&) = default;
	LinearValues& operator=(LinearValues&&) = default;

private:
	Values _values{};
};

LA_AVDECC_API std::optional<ControlValues> LA_AVDECC_CALL_CONVENTION unpackDynamicControlValues(MemoryBuffer const& packedControlValues, ControlValueType::Type const valueType, std::uint16_t const numberOfValues) noexcept;

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
