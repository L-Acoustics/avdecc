/*
* Copyright (C) 2016-2021, L-Acoustics and its contributors

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
* @file networkInterfaceHelper.h
* @author Christophe Calmejane
* @brief Network interface helper for C bindings Library.
*/

#pragma once

#include "internals/exports.h"
#include "internals/typedefs.h"

typedef enum avdecc_network_interface_type_e
{
	avdecc_network_interface_type_None = 0, /**< Only used for initialization purpose. Never returned as a real Interface::Type */
	avdecc_network_interface_type_Loopback = 1, /**< Loopback interface */
	avdecc_network_interface_type_Ethernet = 2, /**< Ethernet interface */
	avdecc_network_interface_type_WiFi = 3, /**< 802.11 WiFi interface */
	avdecc_network_interface_type_AWDL = 4, /**< Apple Wireless Direct Link */
} avdecc_network_interface_type_t;

typedef struct avdecc_network_interface_s
{
	avdecc_string_t id; /** Identifier of the interface (system chosen, unique) (UTF-8) */
	avdecc_string_t description; /** Description of the interface (system chosen) (UTF-8) */
	avdecc_string_t alias; /** Alias of the interface (often user chosen) (UTF-8) */
	avdecc_mac_address_t mac_address; /** Mac address */
	avdecc_string_t* ip_addresses; /** List of IP addresses attached to this interface, terminated with NULL */
	avdecc_string_t* gateways; /** List of Gateways available for this interface, terminated with NULL */
	avdecc_network_interface_type_t type; /** The type of interface */
	avdecc_bool_t is_enabled; /** True if this interface is enabled */
	avdecc_bool_t is_connected; /** True if this interface is connected to a working network (able to send and receive packets) */
	avdecc_bool_t is_virtual; /** True if this interface is emulating a physical adapter (Like BlueTooth, VirtualMachine, or Software Loopback) */
} avdecc_network_interface_t, *avdecc_network_interface_p;
typedef avdecc_network_interface_t const* avdecc_network_interface_cp;

/** LA_AVDECC_freeNetworkInterface must be called on each returned 'intfc' when no longer needed. */
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_enumerate_interfaces_cb)(avdecc_network_interface_p intfc);

/** Enumerates network interfaces. The specified handler is called for each found interface. */
LA_AVDECC_BINDINGS_C_API void LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_enumerateInterfaces(avdecc_enumerate_interfaces_cb const onInterface);
/** Retrieve a copy of an interface from it's name. Returns NULL if no interface exists with that name. LA_AVDECC_freeNetworkInterface must be called on the returned interface. */
LA_AVDECC_BINDINGS_C_API avdecc_network_interface_p LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_getInterfaceByName(avdecc_string_t const name);
/** Converts the specified MAC address to string (in the form: xx:xx:xx:xx:xx:xx). LA_AVDECC_freeString must be called on the returned string. */
LA_AVDECC_BINDINGS_C_API avdecc_string_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_macAddressToString(avdecc_mac_address_cp const macAddress, avdecc_bool_t const upperCase);
/** Returns true if specified MAC address is valid. */
LA_AVDECC_BINDINGS_C_API avdecc_bool_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_isMacAddressValid(avdecc_mac_address_cp const macAddress);
/** Frees avdecc_network_interface_p. */
LA_AVDECC_BINDINGS_C_API void LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_freeNetworkInterface(avdecc_network_interface_p intfc);
