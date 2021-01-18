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
* @file entityModelControlValuesTraits.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model control descriptor values traits.
*/

#pragma once

#include "entityModelTypes.hpp"
#include "entityModelControlValues.hpp"

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
/** Linear Values - Clause 7.3.5.2.1 */
template<typename SizeType, bool IsDynamic>
struct LinearValuesBaseTraits
{
	using size_type = SizeType;
	static constexpr bool is_value_details = true;
	static constexpr bool is_dynamic = IsDynamic;
};

template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueStatic<std::int8_t>>> : LinearValuesBaseTraits<std::int8_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearInt8;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueStatic<std::uint8_t>>> : LinearValuesBaseTraits<std::uint8_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearUInt8;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueStatic<std::int16_t>>> : LinearValuesBaseTraits<std::int16_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearInt16;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueStatic<std::uint16_t>>> : LinearValuesBaseTraits<std::uint16_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearUInt16;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueStatic<std::int32_t>>> : LinearValuesBaseTraits<std::int32_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearInt32;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueStatic<std::uint32_t>>> : LinearValuesBaseTraits<std::uint32_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearUInt32;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueStatic<std::int64_t>>> : LinearValuesBaseTraits<std::int64_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearInt64;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueStatic<std::uint64_t>>> : LinearValuesBaseTraits<std::uint64_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearUInt64;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueStatic<float>>> : LinearValuesBaseTraits<float, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearFloat;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueStatic<double>>> : LinearValuesBaseTraits<double, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearDouble;
};

template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueDynamic<std::int8_t>>> : LinearValuesBaseTraits<std::int8_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearInt8;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueDynamic<std::uint8_t>>> : LinearValuesBaseTraits<std::uint8_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearUInt8;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueDynamic<std::int16_t>>> : LinearValuesBaseTraits<std::int16_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearInt16;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueDynamic<std::uint16_t>>> : LinearValuesBaseTraits<std::uint16_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearUInt16;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueDynamic<std::int32_t>>> : LinearValuesBaseTraits<std::int32_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearInt32;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueDynamic<std::uint32_t>>> : LinearValuesBaseTraits<std::uint32_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearUInt32;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueDynamic<std::int64_t>>> : LinearValuesBaseTraits<std::int64_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearInt64;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueDynamic<std::uint64_t>>> : LinearValuesBaseTraits<std::uint64_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearUInt64;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueDynamic<float>>> : LinearValuesBaseTraits<float, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearFloat;
};
template<>
struct ControlValues::control_value_details_traits<LinearValues<LinearValueDynamic<double>>> : LinearValuesBaseTraits<double, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlLinearDouble;
};

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
