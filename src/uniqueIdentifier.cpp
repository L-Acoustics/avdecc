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
* @file uniqueIdentifier.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/uniqueIdentifier.hpp"

namespace la
{
namespace avdecc
{

static constexpr UniqueIdentifier s_NullIdentifier = 0x0000000000000000;
static constexpr UniqueIdentifier s_UninitializedIdentifier = 0xFFFFFFFFFFFFFFFF;

UniqueIdentifier LA_AVDECC_CALL_CONVENTION getNullIdentifier() noexcept
{
	return s_NullIdentifier;
}

UniqueIdentifier LA_AVDECC_CALL_CONVENTION getUninitializedIdentifier() noexcept
{
	return s_UninitializedIdentifier;
}

bool LA_AVDECC_CALL_CONVENTION isValidUniqueIdentifier(UniqueIdentifier const eid) noexcept
{
	return eid != s_NullIdentifier && eid != s_UninitializedIdentifier;
}

} // namespace avdecc
} // namespace la
