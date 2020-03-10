/*
* Copyright (C) 2016-2020, L-Acoustics and its contributors

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
* @file networkInterfaceEnumerator.c
* @author Christophe Calmejane
* @brief Example enumerating all detected network interfaces on the local computer (using C Wrapper Library).
*/

#include <la/avdecc/networkInterfaceHelper.h>
#include <stdio.h>

static avdecc_string_t getMacAddress(avdecc_mac_address_cp const macAddress)
{
	return LA_AVDECC_macAddressToString(macAddress, avdecc_bool_true);
	// The returned avdecc_string_t should be freed, but for this example we don't really care for non freed memory
}

static avdecc_const_string_t getInterfaceType(avdecc_network_interface_type_t const type)
{
	switch (type)
	{
		case avdecc_network_interface_type_Loopback:
			return "Loopback";
			break;
		case avdecc_network_interface_type_Ethernet:
			return "Ethernet";
			break;
		case avdecc_network_interface_type_WiFi:
			return "WiFi";
			break;
		case avdecc_network_interface_type_AWDL:
			return "AWDL";
			break;
		default:
			return "Unknown type";
	}
}

static void LA_AVDECC_BINDINGS_C_CALL_CONVENTION on_avdecc_enumerate_interfaces_cb(avdecc_network_interface_p intfc)
{
	unsigned int intNum = 1;

	printf("%d: %s\n", intNum, intfc->id);
	printf("  Description:  %s\n", intfc->description);
	printf("  Alias:        %s\n", intfc->alias);
	printf("  MacAddress:   %s\n", getMacAddress(&intfc->mac_address));
	printf("  Type:         %s\n", getInterfaceType(intfc->type));
	printf("  Enabled:      %s\n", intfc->is_enabled ? "YES" : "NO");
	printf("  Connected:    %s\n", intfc->is_connected ? "YES" : "NO");
	printf("  Virtual:      %s\n", intfc->is_virtual ? "YES" : "NO");
	if (intfc->ip_addresses != NULL)
	{
		printf("  IP Addresses: \n");
		avdecc_string_t* ptr = intfc->ip_addresses;
		while (*ptr != NULL)
		{
			printf("    %s\n", *ptr);
			++ptr;
		}
	}
	if (intfc->gateways != NULL)
	{
		printf("  Gateways: \n");
		avdecc_string_t* ptr = intfc->gateways;
		while (*ptr != NULL)
		{
			printf("    %s\n", *ptr);
			++ptr;
		}
	}

	printf("\n");
	++intNum;

	LA_AVDECC_freeNetworkInterface(intfc);
}

static int displayInterfaces()
{
	printf("Available interfaces:\n\n");

	// Enumerate available interfaces
	LA_AVDECC_enumerateInterfaces(on_avdecc_enumerate_interfaces_cb);

	return 0;
}

int main()
{
	return displayInterfaces();
}
