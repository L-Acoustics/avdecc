/*
* Copyright (C) 2016-2022, L-Acoustics and its contributors

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
* @file protocolInterface_macNative.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/protocolInterface.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{
class ProtocolInterfaceMacNative : public ProtocolInterface
{
public:
	/**
	* @brief Factory to create a ProtocolInterfaceMacNative.
	* @details Factory to create a ProtocolInterfaceMacNative as a raw pointer.
	* @param[in] networkInterfaceName The name of the network interface to use.
	* @return A new ProtocolInterfaceMacNative as a raw pointerr
	* @note Throw #std::invalid_argument if interface is not recognized.
	*/
	static ProtocolInterfaceMacNative* createRawProtocolInterfaceMacNative(std::string const& networkInterfaceName);

	/** Returns true if this ProtocolInterface is supported (runtime check) */
	static bool isSupported() noexcept;

	/** Destructor */
	virtual ~ProtocolInterfaceMacNative() noexcept = default;

	// Deleted compiler auto-generated methods
	ProtocolInterfaceMacNative(ProtocolInterfaceMacNative&&) = delete;
	ProtocolInterfaceMacNative(ProtocolInterfaceMacNative const&) = delete;
	ProtocolInterfaceMacNative& operator=(ProtocolInterfaceMacNative const&) = delete;
	ProtocolInterfaceMacNative& operator=(ProtocolInterfaceMacNative&&) = delete;

protected:
	ProtocolInterfaceMacNative(std::string const& networkInterfaceName);
};

} // namespace protocol
} // namespace avdecc
} // namespace la
