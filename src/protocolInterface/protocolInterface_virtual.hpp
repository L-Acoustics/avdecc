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
* @file protocolInterface_virtual.hpp
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
class ProtocolInterfaceVirtual : public ProtocolInterface
{
public:
	/**
	* @brief Factory method to create a ProtocolInterfaceVirtual.
	* @details Factory method to create a ProtocolInterfaceVirtual as a raw pointer.
	* @param[in] networkInterfaceID The ID of the virtual interface to use.
	* @param[in] macAddress The MAC address associated with the network interface. Cannot be all 0.
	* @param[in] executorName The name of the executor to use to dispatch incoming messages.
	* @return A new ProtocolInterfaceVirtual as a raw pointer
	* @note Throws Exception if #interfaceName is invalid or inaccessible.
	*/
	static ProtocolInterfaceVirtual* createRawProtocolInterfaceVirtual(std::string const& networkInterfaceID, networkInterface::MacAddress const& macAddress, std::string const& executorName);

	/** Returns true if this ProtocolInterface is supported (runtime check) */
	static bool isSupported() noexcept;

	/** Destructor */
	virtual ~ProtocolInterfaceVirtual() noexcept = default;

	/** Force a transport error on the interface */
	virtual void forceTransportError() const noexcept = 0;

	// Deleted compiler auto-generated methods
	ProtocolInterfaceVirtual(ProtocolInterfaceVirtual&&) = delete;
	ProtocolInterfaceVirtual(ProtocolInterfaceVirtual const&) = delete;
	ProtocolInterfaceVirtual& operator=(ProtocolInterfaceVirtual const&) = delete;
	ProtocolInterfaceVirtual& operator=(ProtocolInterfaceVirtual&&) = delete;

protected:
	ProtocolInterfaceVirtual(std::string const& networkInterfaceID, networkInterface::MacAddress const& macAddress, std::string const& executorName);
};

} // namespace protocol
} // namespace avdecc
} // namespace la
