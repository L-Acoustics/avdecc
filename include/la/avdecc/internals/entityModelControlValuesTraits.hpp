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
/** Linear Values - IEEE1722.1-2013 Clause 7.3.5.2.1 */
template<typename SizeType, bool IsDynamic>
struct LinearValuesBaseTraits
{
	using size_type = SizeType;
	static constexpr bool is_value_details = true;
	static constexpr bool is_dynamic = IsDynamic;
	static constexpr std::optional<bool> static_dynamic_counts_identical = true;
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

/** Selector Value - IEEE1722.1-2013 Clause 7.3.5.2.2 */
template<typename SizeType, bool IsDynamic>
struct SelectorValueBaseTraits
{
	using size_type = SizeType;
	static constexpr bool is_value_details = true;
	static constexpr bool is_dynamic = IsDynamic;
	static constexpr std::optional<bool> static_dynamic_counts_identical = true;
};

template<>
struct ControlValues::control_value_details_traits<SelectorValueStatic<std::int8_t>> : SelectorValueBaseTraits<std::int8_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorInt8;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueStatic<std::uint8_t>> : SelectorValueBaseTraits<std::uint8_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorUInt8;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueStatic<std::int16_t>> : SelectorValueBaseTraits<std::int16_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorInt16;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueStatic<std::uint16_t>> : SelectorValueBaseTraits<std::uint16_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorUInt16;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueStatic<std::int32_t>> : SelectorValueBaseTraits<std::int32_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorInt32;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueStatic<std::uint32_t>> : SelectorValueBaseTraits<std::uint32_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorUInt32;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueStatic<std::int64_t>> : SelectorValueBaseTraits<std::int64_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorInt64;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueStatic<std::uint64_t>> : SelectorValueBaseTraits<std::uint64_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorUInt64;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueStatic<float>> : SelectorValueBaseTraits<float, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorFloat;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueStatic<double>> : SelectorValueBaseTraits<double, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorDouble;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueStatic<LocalizedStringReference>> : SelectorValueBaseTraits<LocalizedStringReference::value_type, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorString;
};

template<>
struct ControlValues::control_value_details_traits<SelectorValueDynamic<std::int8_t>> : SelectorValueBaseTraits<std::int8_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorInt8;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueDynamic<std::uint8_t>> : SelectorValueBaseTraits<std::uint8_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorUInt8;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueDynamic<std::int16_t>> : SelectorValueBaseTraits<std::int16_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorInt16;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueDynamic<std::uint16_t>> : SelectorValueBaseTraits<std::uint16_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorUInt16;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueDynamic<std::int32_t>> : SelectorValueBaseTraits<std::int32_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorInt32;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueDynamic<std::uint32_t>> : SelectorValueBaseTraits<std::uint32_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorUInt32;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueDynamic<std::int64_t>> : SelectorValueBaseTraits<std::int64_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorInt64;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueDynamic<std::uint64_t>> : SelectorValueBaseTraits<std::uint64_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorUInt64;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueDynamic<float>> : SelectorValueBaseTraits<float, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorFloat;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueDynamic<double>> : SelectorValueBaseTraits<double, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorDouble;
};
template<>
struct ControlValues::control_value_details_traits<SelectorValueDynamic<LocalizedStringReference>> : SelectorValueBaseTraits<LocalizedStringReference::value_type, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlSelectorString;
};

/** Array Values - IEEE1722.1-2013 Clause 7.3.5.2.3 */
template<typename SizeType, bool IsDynamic>
struct ArrayValuesBaseTraits
{
	using size_type = SizeType;
	static constexpr bool is_value_details = true;
	static constexpr bool is_dynamic = IsDynamic;
	static constexpr std::optional<bool> static_dynamic_counts_identical = false;
};

template<>
struct ControlValues::control_value_details_traits<ArrayValueStatic<std::int8_t>> : ArrayValuesBaseTraits<std::int8_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayInt8;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueStatic<std::uint8_t>> : ArrayValuesBaseTraits<std::uint8_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayUInt8;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueStatic<std::int16_t>> : ArrayValuesBaseTraits<std::int16_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayInt16;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueStatic<std::uint16_t>> : ArrayValuesBaseTraits<std::uint16_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayUInt16;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueStatic<std::int32_t>> : ArrayValuesBaseTraits<std::int32_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayInt32;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueStatic<std::uint32_t>> : ArrayValuesBaseTraits<std::uint32_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayUInt32;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueStatic<std::int64_t>> : ArrayValuesBaseTraits<std::int64_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayInt64;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueStatic<std::uint64_t>> : ArrayValuesBaseTraits<std::uint64_t, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayUInt64;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueStatic<float>> : ArrayValuesBaseTraits<float, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayFloat;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueStatic<double>> : ArrayValuesBaseTraits<double, false>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayDouble;
};

template<>
struct ControlValues::control_value_details_traits<ArrayValueDynamic<std::int8_t>> : ArrayValuesBaseTraits<std::int8_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayInt8;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueDynamic<std::uint8_t>> : ArrayValuesBaseTraits<std::uint8_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayUInt8;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueDynamic<std::int16_t>> : ArrayValuesBaseTraits<std::int16_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayInt16;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueDynamic<std::uint16_t>> : ArrayValuesBaseTraits<std::uint16_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayUInt16;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueDynamic<std::int32_t>> : ArrayValuesBaseTraits<std::int32_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayInt32;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueDynamic<std::uint32_t>> : ArrayValuesBaseTraits<std::uint32_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayUInt32;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueDynamic<std::int64_t>> : ArrayValuesBaseTraits<std::int64_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayInt64;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueDynamic<std::uint64_t>> : ArrayValuesBaseTraits<std::uint64_t, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayUInt64;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueDynamic<float>> : ArrayValuesBaseTraits<float, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayFloat;
};
template<>
struct ControlValues::control_value_details_traits<ArrayValueDynamic<double>> : ArrayValuesBaseTraits<double, true>
{
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlArrayDouble;
};

/** UTF-8 String Value - IEEE1722.1-2013 Clause 7.3.5.2.4 */
template<>
struct ControlValues::control_value_details_traits<UTF8StringValueStatic>
{
	static constexpr bool is_value_details = true;
	static constexpr bool is_dynamic = false;
	static constexpr std::optional<bool> static_dynamic_counts_identical = true;
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlUtf8;
};
template<>
struct ControlValues::control_value_details_traits<UTF8StringValueDynamic>
{
	static constexpr bool is_value_details = true;
	static constexpr bool is_dynamic = true;
	static constexpr std::optional<bool> static_dynamic_counts_identical = true;
	static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::ControlUtf8;
};

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
