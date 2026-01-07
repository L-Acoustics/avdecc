/*
* Copyright (C) 2016-2026, L-Acoustics and its contributors

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

#include <algorithm>

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace mvuPayload
{
/** GET_MILAN_INFO Command - Milan 1.3 Clause 5.4.4.1 */
constexpr size_t AecpMvuGetMilanInfoCommandPayloadSize = 2u;

/** GET_MILAN_INFO Response - Milan 1.2 Clause 5.4.4.1 */
constexpr size_t AecpMvuGetMilanInfo12ResponsePayloadSize = 14u;

/** GET_MILAN_INFO Response - Milan 1.3 Clause 5.4.4.1 */
constexpr size_t AecpMvuGetMilanInfo13ResponsePayloadSize = 18u;

constexpr size_t AecpMvuGetMilanInfoResponsePayloadMinSize = std::min(AecpMvuGetMilanInfo12ResponsePayloadSize, AecpMvuGetMilanInfo13ResponsePayloadSize);
constexpr size_t AecpMvuGetMilanInfoResponsePayloadMaxSize = std::max(AecpMvuGetMilanInfo12ResponsePayloadSize, AecpMvuGetMilanInfo13ResponsePayloadSize);

/** SET_SYSTEM_UNIQUE_ID Command - Milan 1.2 Clause 5.4.4.2 */
constexpr size_t AecpMvuSetSystemUniqueID12CommandPayloadSize = 6u;

/** SET_SYSTEM_UNIQUE_ID Command - Milan 1.3 Clause 5.4.4.2 */
constexpr size_t AecpMvuSetSystemUniqueID13CommandPayloadSize = 74u;

/** SET_SYSTEM_UNIQUE_ID Response - Milan 1.2 Clause 5.4.4.2 */
constexpr size_t AecpMvuSetSystemUniqueID12ResponsePayloadSize = 6u;

/** SET_SYSTEM_UNIQUE_ID Response - Milan 1.3 Clause 5.4.4.2 */
constexpr size_t AecpMvuSetSystemUniqueID13ResponsePayloadSize = 74u;

/** GET_SYSTEM_UNIQUE_ID Command - Milan 1.3 Clause 5.4.4.3 */
constexpr size_t AecpMvuGetSystemUniqueIDCommandPayloadSize = 2u;

/** GET_SYSTEM_UNIQUE_ID Response - Milan 1.2 Clause 5.4.4.3 */
constexpr size_t AecpMvuGetSystemUniqueID12ResponsePayloadSize = 6u;

/** GET_SYSTEM_UNIQUE_ID Response - Milan 1.3 Clause 5.4.4.3 */
constexpr size_t AecpMvuGetSystemUniqueID13ResponsePayloadSize = 74u;

constexpr size_t AecpMvuSetSystemUniqueIDCommandPayloadMinSize = std::min(AecpMvuSetSystemUniqueID12CommandPayloadSize, AecpMvuSetSystemUniqueID13CommandPayloadSize);
constexpr size_t AecpMvuSetSystemUniqueIDCommandPayloadMaxSize = std::max(AecpMvuSetSystemUniqueID12CommandPayloadSize, AecpMvuSetSystemUniqueID13CommandPayloadSize);
constexpr size_t AecpMvuSetSystemUniqueIDResponsePayloadMinSize = std::min(AecpMvuSetSystemUniqueID12ResponsePayloadSize, AecpMvuSetSystemUniqueID13ResponsePayloadSize);
constexpr size_t AecpMvuSetSystemUniqueIDResponsePayloadMaxSize = std::max(AecpMvuSetSystemUniqueID12ResponsePayloadSize, AecpMvuSetSystemUniqueID13ResponsePayloadSize);
constexpr size_t AecpMvuGetSystemUniqueIDResponsePayloadMinSize = std::min(AecpMvuGetSystemUniqueID12ResponsePayloadSize, AecpMvuGetSystemUniqueID13ResponsePayloadSize);
constexpr size_t AecpMvuGetSystemUniqueIDResponsePayloadMaxSize = std::max(AecpMvuGetSystemUniqueID12ResponsePayloadSize, AecpMvuGetSystemUniqueID13ResponsePayloadSize);

/** SET_MEDIA_CLOCK_REFERENCE_INFO Command - Milan 1.3 Clause 5.4.4.4 */
constexpr size_t AecpMvuSetMediaClockReferenceInfoCommandPayloadSize = 74u;

/** SET_MEDIA_CLOCK_REFERENCE_INFO Response - Milan 1.3 Clause 5.4.4.4 */
constexpr size_t AecpMvuSetMediaClockReferenceInfoResponsePayloadSize = 74u;

/** GET_MEDIA_CLOCK_REFERENCE_INFO Command - Milan 1.3 Clause 5.4.4.5 */
constexpr size_t AecpMvuGetMediaClockReferenceInfoCommandPayloadSize = 2u;

/** GET_MEDIA_CLOCK_REFERENCE_INFO Response - Milan 1.3 Clause 5.4.4.5 */
constexpr size_t AecpMvuGetMediaClockReferenceInfoResponsePayloadSize = 74u;

/** BIND_STREAM Command - Milan 1.3 Clause 5.4.4.6 */
constexpr size_t AecpMvuBindStreamCommandPayloadSize = 18u;

/** BIND_STREAM Response - Milan 1.3 Clause 5.4.4.6 */
constexpr size_t AecpMvuBindStreamResponsePayloadSize = 18u;

/** UNBIND_STREAM Command - Milan 1.3 Clause 5.4.4.7 */
constexpr size_t AecpMvuUnbindStreamCommandPayloadSize = 6u;

/** UNBIND_STREAM Response - Milan 1.3 Clause 5.4.4.7 */
constexpr size_t AecpMvuUnbindStreamResponsePayloadSize = 6u;

/** GET_STREAM_INPUT_INFO_EX Command - Milan 1.3 Clause 5.4.4.8 */
constexpr size_t AecpMvuGetStreamInputInfoExCommandPayloadSize = 6u;

/** GET_STREAM_INPUT_INFO_EX Response - Milan 1.3 Clause 5.4.4.8 */
constexpr size_t AecpMvuGetStreamInputInfoExResponsePayloadSize = 18u;

} // namespace mvuPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
