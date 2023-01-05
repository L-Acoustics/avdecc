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
* @file protocolAcmpdu_c.cpp
* @author Christophe Calmejane
* @brief C bindings for la::avdecc::protocol::Acmpdu.
*/

#include <la/avdecc/internals/protocolAcmpdu.hpp>
#include "la/avdecc/avdecc.h"
#include "utils.hpp"
#include <mutex>

/* ************************************************************************** */
/* ProtocolAcmpdu APIs                                                        */
/* ************************************************************************** */
LA_AVDECC_BINDINGS_C_API avdecc_mac_address_t const* LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_Protocol_Acmpdu_getMulticastMacAddress()
{
	static std::once_flag s_once{};
	static avdecc_mac_address_t Acmpdu_Multicast_Mac_Address{ 0u };

	std::call_once(s_once,
		[]()
		{
			la::avdecc::bindings::fromCppToC::set_macAddress(la::avdecc::protocol::Acmpdu::Multicast_Mac_Address, Acmpdu_Multicast_Mac_Address);
		});

	return &Acmpdu_Multicast_Mac_Address;
}
