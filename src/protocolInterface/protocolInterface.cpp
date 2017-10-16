/*
* Copyright (C) 2016-2017, L-Acoustics and its contributors

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
* @file protocolInterface.cpp
* @author Christophe Calmejane
*/

#include "protocolInterface.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{

// Throws an Exception if networkInterfaceName is not usable
ProtocolInterface::ProtocolInterface(std::string const& networkInterfaceName)
	: _networkInterfaceName(networkInterfaceName)
{
	try
	{
		auto const interface = la::avdecc::networkInterface::getInterfaceByName(networkInterfaceName);
		// Check we have a valid mac address
		if (!la::avdecc::networkInterface::isMacAddressValid(interface.macAddress))
			throw Exception(Error::InterfaceInvalid, "Network interface has an invalid mac address");
		_networkInterfaceMacAddress = interface.macAddress;
#pragma message("TBD: Try to get interface number from 'interface' struct")
		_interfaceIndex = 0;
	}
	catch (la::avdecc::Exception const&)
	{
		throw Exception(Error::InterfaceNotFound, "No interface found with specified name");
	}
}

la::avdecc::networkInterface::MacAddress const& ProtocolInterface::getMacAddress() const noexcept
{
	return _networkInterfaceMacAddress;
}

std::uint16_t ProtocolInterface::getInterfaceIndex() const noexcept
{
	return _interfaceIndex;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
