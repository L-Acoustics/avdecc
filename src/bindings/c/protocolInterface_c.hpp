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
* @file protocolInterface_c.hpp
* @author Christophe Calmejane
* @brief Private APIs for ProtocolInterface C bindings.
*/

#pragma once

#include <la/avdecc/internals/protocolInterface.hpp>
#include "la/avdecc/avdecc.h"

namespace la
{
namespace avdecc
{
namespace bindings
{
LA_AVDECC_BINDINGS_C_API la::avdecc::protocol::ProtocolInterface& LA_AVDECC_BINDINGS_C_CALL_CONVENTION getProtocolInterface(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle);
} // namespace bindings
} // namespace avdecc
} // namespace la
