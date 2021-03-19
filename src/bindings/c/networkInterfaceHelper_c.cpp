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
* @file networkInterfaceHelper.cpp
* @author Christophe Calmejane
* @brief Network interface helper for C bindings Library.
*/

#include <la/avdecc/networkInterfaceHelper.hpp>
#include "la/avdecc/networkInterfaceHelper.h"
#include "utils.hpp"
#include <cstdlib>

static avdecc_network_interface_p make_avdecc_network_interface(la::avdecc::networkInterface::Interface const& intfc)
{
	auto ifc = new avdecc_network_interface_t();

	ifc->id = strdup(intfc.id.c_str());
	ifc->description = strdup(intfc.description.c_str());
	ifc->alias = strdup(intfc.alias.c_str());
	la::avdecc::bindings::fromCppToC::set_macAddress(intfc.macAddress, ifc->mac_address);
	if (intfc.ipAddressInfos.empty())
	{
		ifc->ip_addresses = nullptr;
	}
	else
	{
		auto const count = intfc.ipAddressInfos.size();
		ifc->ip_addresses = new avdecc_string_t[count + 1];
		for (auto i = 0u; i < count; ++i)
		{
			ifc->ip_addresses[i] = strdup(static_cast<std::string>(intfc.ipAddressInfos[i].address).c_str());
		}
		ifc->ip_addresses[count] = nullptr;
	}
	if (intfc.gateways.empty())
	{
		ifc->gateways = nullptr;
	}
	else
	{
		auto const count = intfc.gateways.size();
		ifc->gateways = new avdecc_string_t[count + 1];
		for (auto i = 0u; i < count; ++i)
		{
			ifc->gateways[i] = strdup(static_cast<std::string>(intfc.gateways[i]).c_str());
		}
		ifc->gateways[count] = nullptr;
	}
	ifc->type = static_cast<avdecc_network_interface_type_t>(intfc.type);
	ifc->is_enabled = static_cast<avdecc_bool_t>(intfc.isEnabled);
	ifc->is_connected = static_cast<avdecc_bool_t>(intfc.isConnected);
	ifc->is_virtual = static_cast<avdecc_bool_t>(intfc.isVirtual);

	return ifc;
}

static void delete_avdecc_network_interface(avdecc_network_interface_p ifc)
{
	if (ifc == nullptr)
		return;
	std::free(ifc->id);
	std::free(ifc->description);
	std::free(ifc->alias);
	if (ifc->ip_addresses != nullptr)
	{
		auto* ptr = ifc->ip_addresses;
		while (*ptr != nullptr)
		{
			std::free(*ptr);
			++ptr;
		}
		delete[] ifc->ip_addresses;
	}
	if (ifc->gateways != nullptr)
	{
		auto* ptr = ifc->gateways;
		while (*ptr != nullptr)
		{
			std::free(*ptr);
			++ptr;
		}
		delete[] ifc->gateways;
	}

	delete ifc;
}

LA_AVDECC_BINDINGS_C_API void LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_enumerateInterfaces(avdecc_enumerate_interfaces_cb const onInterface)
{
	la::avdecc::networkInterface::enumerateInterfaces(
		[onInterface](la::avdecc::networkInterface::Interface const& intfc)
		{
			auto* i = make_avdecc_network_interface(intfc);
			onInterface(i);
		});
}

LA_AVDECC_BINDINGS_C_API avdecc_network_interface_p LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_getInterfaceByName(avdecc_string_t const name)
{
	try
	{
		return make_avdecc_network_interface(la::avdecc::networkInterface::getInterfaceByName(name));
	}
	catch (...)
	{
		return nullptr;
	}
}

LA_AVDECC_BINDINGS_C_API avdecc_string_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_macAddressToString(avdecc_mac_address_cp const macAddress, avdecc_bool_t const upperCase)
{
	return strdup(la::avdecc::networkInterface::macAddressToString(la::avdecc::bindings::fromCToCpp::make_macAddress(macAddress), upperCase).c_str());
}

LA_AVDECC_BINDINGS_C_API avdecc_bool_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_isMacAddressValid(avdecc_mac_address_cp const macAddress)
{
	return static_cast<avdecc_bool_t>(la::avdecc::networkInterface::isMacAddressValid(la::avdecc::bindings::fromCToCpp::make_macAddress(macAddress)));
}

LA_AVDECC_BINDINGS_C_API void LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_freeNetworkInterface(avdecc_network_interface_p intfc)
{
	delete_avdecc_network_interface(intfc);
}
