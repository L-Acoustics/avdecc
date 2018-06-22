/*
* Copyright (C) 2016-2018, L-Acoustics and its contributors

* This file is part of LA_avdecc.

* LA_avdecc is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* LA_avdecc is distributed in the hope that it will be usefu_state,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public License
* along with LA_avdecc.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* @file uniqueIdentifier.hpp
* @author Christophe Calmejane
* @brief UniqueIdentifier definition and helper methods.
* @note http://standards.ieee.org/develop/regauth/tut/eui.pdf
*/

#pragma once

#include <cstdint>
#include <string>
#include "exports.hpp"

namespace la
{
namespace avdecc
{

using UniqueIdentifier = std::uint64_t;

/** Returns the 'Null' EID */
LA_AVDECC_API UniqueIdentifier LA_AVDECC_CALL_CONVENTION getNullIdentifier() noexcept;

/** Returns the 'Uninitialized' EID */
LA_AVDECC_API UniqueIdentifier LA_AVDECC_CALL_CONVENTION getUninitializedIdentifier() noexcept;

/** Returns true if eid is neither the Null nor Uninitialized EID */
LA_AVDECC_API bool LA_AVDECC_CALL_CONVENTION isValidUniqueIdentifier(UniqueIdentifier const eid) noexcept;

} // namespace avdecc
} // namespace la
