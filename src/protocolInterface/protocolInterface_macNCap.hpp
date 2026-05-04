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
* @file protocolInterface_macNCap.hpp
* @author Christophe Calmejane
* @brief macOS native ProtocolInterface based on Network.framework (NWEthernetChannel).
*/

#pragma once

#include "la/avdecc/internals/protocolInterface.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{
/**
* @brief ProtocolInterface implementation based on Apple's Network.framework NWEthernetChannel (macOS 10.15+).
* @details This implementation uses the nw_ethernet_channel C API to send and receive raw layer-2 Ethernet
*          frames matching the AVTP EtherType (0x22F0). It is an alternative to the pcap-based implementation
*          and does not require any third-party library, but requires the binary to hold the
*          `com.apple.developer.networking.custom-protocol` entitlement.
*/
class ProtocolInterfaceMacNCap : public ProtocolInterface
{
public:
	/**
	* @brief Factory method to create a new ProtocolInterfaceMacNCap.
	* @details Creates a new ProtocolInterfaceMacNCap as a raw pointer.
	* @param[in] networkInterfaceID The ID (BSD name, e.g. `en0`) of the network interface to use.
	* @param[in] executorName The name of the executor to use to dispatch incoming messages.
	* @return A new ProtocolInterfaceMacNCap as a raw pointer.
	* @note Throws Exception if the specified interface cannot be opened or the channel cannot be created.
	*/
	static ProtocolInterfaceMacNCap* createRawProtocolInterfaceMacNCap(std::string const& networkInterfaceID, std::string const& executorName);

	/** Returns true if this ProtocolInterface is supported (runtime check). */
	static bool isSupported() noexcept;

	/** Destructor */
	virtual ~ProtocolInterfaceMacNCap() noexcept = default;

	// Deleted compiler auto-generated methods
	ProtocolInterfaceMacNCap(ProtocolInterfaceMacNCap&&) = delete;
	ProtocolInterfaceMacNCap(ProtocolInterfaceMacNCap const&) = delete;
	ProtocolInterfaceMacNCap& operator=(ProtocolInterfaceMacNCap const&) = delete;
	ProtocolInterfaceMacNCap& operator=(ProtocolInterfaceMacNCap&&) = delete;

protected:
	ProtocolInterfaceMacNCap(std::string const& networkInterfaceID, std::string const& executorName);
};

} // namespace protocol
} // namespace avdecc
} // namespace la
