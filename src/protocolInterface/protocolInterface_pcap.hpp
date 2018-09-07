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
* @file protocolInterface_pcap.hpp
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

class ProtocolInterfacePcap : public ProtocolInterface
{
public:
	/**
	* @brief Factory method to create a new ProtocolInterfacePcap.
	* @details Creates a new ProtocolInterfacePcap as a raw pointer.
	* @param[in] networkInterfaceName The name of the network interface to use.
	* @return A new ProtocolInterfacePcap as a raw pointer.
	* @note Throws Exception if #interfaceName is invalid or inaccessible.
	*/
	static ProtocolInterfacePcap* createRawProtocolInterfacePcap(std::string const& networkInterfaceName);

	/** Returns true if this ProtocolInterface is supported (runtime check) */
	static bool isSupported() noexcept;

	/** Destructor */
	virtual ~ProtocolInterfacePcap() noexcept = default;

	// Deleted compiler auto-generated methods
	ProtocolInterfacePcap(ProtocolInterfacePcap&&) = delete;
	ProtocolInterfacePcap(ProtocolInterfacePcap const&) = delete;
	ProtocolInterfacePcap& operator=(ProtocolInterfacePcap const&) = delete;
	ProtocolInterfacePcap& operator=(ProtocolInterfacePcap&&) = delete;

protected:
	ProtocolInterfacePcap(std::string const& networkInterfaceName);
};

} // namespace protocol
} // namespace avdecc
} // namespace la
