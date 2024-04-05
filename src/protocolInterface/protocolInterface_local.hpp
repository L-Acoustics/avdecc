/*
* Copyright (C) 2016-2024, L-Acoustics and its contributors

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
* @file protocolInterface_local.hpp
* @author Luke Howard
*/

#pragma once

#include "la/avdecc/internals/protocolInterface.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{
class ProtocolInterfaceLocal : public ProtocolInterface
{
public:
	/**
	* @brief Factory method to create a new ProtocolInterfaceLocal.
	* @details Creates a new ProtocolInterfaceLocal as a raw pointer.
	* @param[in] networkInterfaceName A path to the local domain socket.
	* @param[in] executorName The name of the executor to use to dispatch incoming messages.
	* @return A new ProtocolInterfaceLocal as a raw pointer.
	* @note Throws Exception if #interfaceName is invalid or inaccessible.
	*/
	static ProtocolInterfaceLocal* createRawProtocolInterfaceLocal(std::string const& networkInterfaceName, std::string const& executorName);

	/** Returns true if this ProtocolInterface is supported (runtime check) */
	static bool isSupported() noexcept;

	/** Destructor */
	virtual ~ProtocolInterfaceLocal() noexcept = default;

	// Deleted compiler auto-generated methods
	ProtocolInterfaceLocal(ProtocolInterfaceLocal&&) = delete;
	ProtocolInterfaceLocal(ProtocolInterfaceLocal const&) = delete;
	ProtocolInterfaceLocal& operator=(ProtocolInterfaceLocal const&) = delete;
	ProtocolInterfaceLocal& operator=(ProtocolInterfaceLocal&&) = delete;

protected:
	ProtocolInterfaceLocal(std::string const& networkInterfaceName, networkInterface::MacAddress const& macAddress, std::string const& executorName);
};

} // namespace protocol
} // namespace avdecc
} // namespace la
