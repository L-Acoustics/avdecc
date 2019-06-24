/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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

// Public API
// Internal API

#include <gtest/gtest.h>
#include <la/avdecc/networkInterfaceHelper.hpp>

TEST(IPAddress, ToString)
{
	auto const adrs = la::avdecc::networkInterface::IPAddress{ "10.0.0.0" };

	auto const adrsAsString = static_cast<std::string>(adrs);
	EXPECT_STREQ("10.0.0.0", adrsAsString.c_str());
}

TEST(IPAddress, IncrementOperator)
{
	auto const adrs = la::avdecc::networkInterface::IPAddress{ "10.0.0.0" };

	auto const adrsAsString = static_cast<std::string>(adrs + 1);
	EXPECT_STREQ("10.0.0.1", adrsAsString.c_str());
}

TEST(IPAddressInfo, IsPrivateAddress)
{
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "10.0.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.0.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "10.0.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.255.255" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "10.0.0.1" }, la::avdecc::networkInterface::IPAddress{ "255.0.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "10.0.1.0" }, la::avdecc::networkInterface::IPAddress{ "255.0.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "10.1.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.0.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "10.8.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "10.255.255.255" }, la::avdecc::networkInterface::IPAddress{ "255.0.0.0" } }.isPrivateNetworkAddress()));

	EXPECT_FALSE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "10.0.0.0" }, la::avdecc::networkInterface::IPAddress{ "254.0.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_FALSE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "9.0.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.0.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_FALSE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "11.0.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.0.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_FALSE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "9.0.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.255.255" } }.isPrivateNetworkAddress()));


	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.16.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.240.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.16.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.255.255" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.16.0.1" }, la::avdecc::networkInterface::IPAddress{ "255.240.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.16.1.0" }, la::avdecc::networkInterface::IPAddress{ "255.240.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.17.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.240.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.17.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.31.255.255" }, la::avdecc::networkInterface::IPAddress{ "255.240.0.0" } }.isPrivateNetworkAddress()));

	EXPECT_FALSE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.15.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.240.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_FALSE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.15.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.255.255" } }.isPrivateNetworkAddress()));
	EXPECT_FALSE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.32.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.240.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_FALSE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.32.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.255.255" } }.isPrivateNetworkAddress()));


	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.255.255" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.0.1" }, la::avdecc::networkInterface::IPAddress{ "255.255.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.1.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.1.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.255.0" } }.isPrivateNetworkAddress()));
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.255.255" }, la::avdecc::networkInterface::IPAddress{ "255.255.0.0" } }.isPrivateNetworkAddress()));

	EXPECT_FALSE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.167.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_FALSE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.169.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.0.0" } }.isPrivateNetworkAddress()));
	EXPECT_FALSE((la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.167.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.255.255" } }.isPrivateNetworkAddress()));
}

#pragma message("TODO: Complete NetworkInterfaceHelper unit tests")
