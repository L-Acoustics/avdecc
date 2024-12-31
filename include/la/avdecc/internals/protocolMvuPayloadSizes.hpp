/*
* Copyright (C) 2016-2025, L-Acoustics and its contributors

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
* @file protocolMvuPayloadSizes.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/protocolMvuAecpdu.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace mvuPayload
{
/** GET_MILAN_INFO Command - Milan 1.2 Clause 5.4.4.1 */
constexpr size_t AecpMvuGetMilanInfoCommandPayloadSize = 2u;

/** GET_MILAN_INFO Response - Milan 1.2 Clause 5.4.4.1 */
constexpr size_t AecpMvuGetMilanInfoResponsePayloadSize = 14u;

/** SET_SYSTEM_UNIQUE_ID Command - Milan 1.2 Clause 5.4.4.2 */
constexpr size_t AecpMvuSetSystemUniqueIDCommandPayloadSize = 6u;

/** SET_SYSTEM_UNIQUE_ID Response - Milan 1.2 Clause 5.4.4.2 */
constexpr size_t AecpMvuSetSystemUniqueIDResponsePayloadSize = 6u;

/** GET_SYSTEM_UNIQUE_ID Command - Milan 1.2 Clause 5.4.4.3 */
constexpr size_t AecpMvuGetSystemUniqueIDCommandPayloadSize = 2u;

/** GET_SYSTEM_UNIQUE_ID Response - Milan 1.2 Clause 5.4.4.3 */
constexpr size_t AecpMvuGetSystemUniqueIDResponsePayloadSize = 6u;

/** SET_MEDIA_CLOCK_REFERENCE_INFO Command - Milan 1.2 Clause 5.4.4.4 */
constexpr size_t AecpMvuSetMediaClockReferenceInfoCommandPayloadSize = 74u;

/** SET_MEDIA_CLOCK_REFERENCE_INFO Response - Milan 1.2 Clause 5.4.4.4 */
constexpr size_t AecpMvuSetMediaClockReferenceInfoResponsePayloadSize = 74u;

/** GET_MEDIA_CLOCK_REFERENCE_INFO Command - Milan 1.2 Clause 5.4.4.5 */
constexpr size_t AecpMvuGetMediaClockReferenceInfoCommandPayloadSize = 2u;

/** GET_MEDIA_CLOCK_REFERENCE_INFO Response - Milan 1.2 Clause 5.4.4.5 */
constexpr size_t AecpMvuGetMediaClockReferenceInfoResponsePayloadSize = 74u;

} // namespace mvuPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
