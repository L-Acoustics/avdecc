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
/** GET_MILAN_INFO Command - Milan-2019 Clause 7.4.1 */
constexpr size_t AecpMvuGetMilanInfoCommandPayloadSize = 2u;

/** GET_MILAN_INFO Response - Milan-2019 Clause 7.4.1 */
constexpr size_t AecpMvuGetMilanInfoResponsePayloadSize = 14u;


} // namespace mvuPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
