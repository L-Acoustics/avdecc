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

// Public API
#include <la/avdecc/networkInterfaceHelper.hpp>

// Internal API
#include "networkInterfaceHelper/networkInterfaceHelper_common.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

/* ************************************************************ */
/* IPAddress Tests                                              */
/* ************************************************************ */
TEST(IPAddress, DefaultConstruct)
{
	auto const ip = la::avdecc::networkInterface::IPAddress{};

	EXPECT_FALSE(ip.isValid()) << "Default constructed IPAddress should not be valid";
	EXPECT_FALSE(static_cast<bool>(ip)) << "Default constructed IPAddress should not be valid";

	EXPECT_THROW(ip.getIPV4(), std::invalid_argument) << "Trying to get IP value if invalid should throw";
	EXPECT_THROW((void)static_cast<la::avdecc::networkInterface::IPAddress::value_type_v4>(ip), std::invalid_argument) << "Trying to get IP value if invalid should throw";
	EXPECT_THROW(ip.getIPV4Packed(), std::invalid_argument) << "Trying to get IP value if invalid should throw";
	EXPECT_THROW((void)static_cast<la::avdecc::networkInterface::IPAddress::value_type_packed_v4>(ip), std::invalid_argument) << "Trying to get IP value if invalid should throw";
	EXPECT_THROW(ip.getIPV6(), std::invalid_argument) << "Trying to get IP value if invalid should throw";
	EXPECT_THROW((void)static_cast<la::avdecc::networkInterface::IPAddress::value_type_v6>(ip), std::invalid_argument) << "Trying to get IP value if invalid should throw";

	EXPECT_EQ(la::avdecc::networkInterface::IPAddress::Type::None, ip.getType()) << "getType() for invalid IPAddress should be None";
}

TEST(IPAddress, V4Construct)
{
	auto const ip = la::avdecc::networkInterface::IPAddress{ la::avdecc::networkInterface::IPAddress::value_type_v4{ 192u, 168u, 0u, 1u } };

	EXPECT_TRUE(ip.isValid()) << "V4 constructed IPAddress should be valid";
	EXPECT_TRUE(static_cast<bool>(ip)) << "V4 constructed IPAddress should be valid";

	EXPECT_NO_THROW(ip.getIPV4()) << "Trying to get IPV4 value should not throw";
	EXPECT_NO_THROW((void)static_cast<la::avdecc::networkInterface::IPAddress::value_type_v4>(ip)) << "Trying to get IPV4 value should not throw";
	EXPECT_NO_THROW(ip.getIPV4Packed()) << "Trying to get IPV4Packed value should not throw";
	EXPECT_NO_THROW((void)static_cast<la::avdecc::networkInterface::IPAddress::value_type_packed_v4>(ip)) << "Trying to get IPV4Packed value should not throw";
	EXPECT_THROW(ip.getIPV6(), std::invalid_argument) << "Trying to get IPV6 value should throw";
	EXPECT_THROW((void)static_cast<la::avdecc::networkInterface::IPAddress::value_type_v6>(ip), std::invalid_argument) << "Trying to get IPV6 value should throw";

	EXPECT_EQ(la::avdecc::networkInterface::IPAddress::Type::V4, ip.getType()) << "getType() for V4 IPAddress should be V4";

	EXPECT_EQ((la::avdecc::networkInterface::IPAddress::value_type_v4{ 192u, 168u, 0u, 1u }), ip.getIPV4());
	EXPECT_EQ((la::avdecc::networkInterface::IPAddress::value_type_v4{ 192u, 168u, 0u, 1u }), static_cast<la::avdecc::networkInterface::IPAddress::value_type_v4>(ip));
	EXPECT_EQ((la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xC0A80001 }), ip.getIPV4Packed());
	EXPECT_EQ((la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xC0A80001 }), static_cast<la::avdecc::networkInterface::IPAddress::value_type_packed_v4>(ip));
}

TEST(IPAddress, StringConstruct)
{
	// Valid IPV4 string
	{
		auto ip = la::avdecc::networkInterface::IPAddress{};
		EXPECT_NO_THROW(ip = la::avdecc::networkInterface::IPAddress{ "192.168.0.1" }) << "Constructing from a valid string should not throw";

		EXPECT_TRUE(ip.isValid()) << "V4 string constructed IPAddress should be valid";
		EXPECT_TRUE(static_cast<bool>(ip)) << "V4 string constructed IPAddress should be valid";

		EXPECT_NO_THROW(ip.getIPV4()) << "Trying to get IPV4 value should not throw";
		EXPECT_NO_THROW((void)static_cast<la::avdecc::networkInterface::IPAddress::value_type_v4>(ip)) << "Trying to get IPV4 value should not throw";
		EXPECT_NO_THROW(ip.getIPV4Packed()) << "Trying to get IPV4Packed value should not throw";
		EXPECT_NO_THROW((void)static_cast<la::avdecc::networkInterface::IPAddress::value_type_packed_v4>(ip)) << "Trying to get IPV4Packed value should not throw";
		EXPECT_THROW(ip.getIPV6(), std::invalid_argument) << "Trying to get IPV6 value should throw";
		EXPECT_THROW((void)static_cast<la::avdecc::networkInterface::IPAddress::value_type_v6>(ip), std::invalid_argument) << "Trying to get IPV6 value should throw";

		EXPECT_EQ(la::avdecc::networkInterface::IPAddress::Type::V4, ip.getType()) << "getType() for V4 IPAddress should be V4";

		EXPECT_EQ((la::avdecc::networkInterface::IPAddress::value_type_v4{ 192u, 168u, 0u, 1u }), ip.getIPV4());
		EXPECT_EQ((la::avdecc::networkInterface::IPAddress::value_type_v4{ 192u, 168u, 0u, 1u }), static_cast<la::avdecc::networkInterface::IPAddress::value_type_v4>(ip));
		EXPECT_EQ((la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xC0A80001 }), ip.getIPV4Packed());
		EXPECT_EQ((la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xC0A80001 }), static_cast<la::avdecc::networkInterface::IPAddress::value_type_packed_v4>(ip));
	}

	// Valid IPV4 string
	{
		auto ip = la::avdecc::networkInterface::IPAddress{};
		EXPECT_NO_THROW(ip = la::avdecc::networkInterface::IPAddress{ "192 .   168  . 0 .  1" }) << "Constructing from a valid string should not throw";

		EXPECT_TRUE(ip.isValid()) << "V4 string constructed IPAddress should be valid";
		EXPECT_TRUE(static_cast<bool>(ip)) << "V4 string constructed IPAddress should be valid";

		EXPECT_NO_THROW(ip.getIPV4()) << "Trying to get IPV4 value should not throw";
		EXPECT_NO_THROW((void)static_cast<la::avdecc::networkInterface::IPAddress::value_type_v4>(ip)) << "Trying to get IPV4 value should not throw";
		EXPECT_NO_THROW(ip.getIPV4Packed()) << "Trying to get IPV4Packed value should not throw";
		EXPECT_NO_THROW((void)static_cast<la::avdecc::networkInterface::IPAddress::value_type_packed_v4>(ip)) << "Trying to get IPV4Packed value should not throw";
		EXPECT_THROW(ip.getIPV6(), std::invalid_argument) << "Trying to get IPV6 value should throw";
		EXPECT_THROW((void)static_cast<la::avdecc::networkInterface::IPAddress::value_type_v6>(ip), std::invalid_argument) << "Trying to get IPV6 value should throw";

		EXPECT_EQ(la::avdecc::networkInterface::IPAddress::Type::V4, ip.getType()) << "getType() for V4 IPAddress should be V4";

		EXPECT_EQ((la::avdecc::networkInterface::IPAddress::value_type_v4{ 192u, 168u, 0u, 1u }), ip.getIPV4());
		EXPECT_EQ((la::avdecc::networkInterface::IPAddress::value_type_v4{ 192u, 168u, 0u, 1u }), static_cast<la::avdecc::networkInterface::IPAddress::value_type_v4>(ip));
		EXPECT_EQ((la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xC0A80001 }), ip.getIPV4Packed());
		EXPECT_EQ((la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xC0A80001 }), static_cast<la::avdecc::networkInterface::IPAddress::value_type_packed_v4>(ip));
	}

	// Invalid IPV4 string
	{
		auto ip = la::avdecc::networkInterface::IPAddress{};
		EXPECT_THROW(ip = la::avdecc::networkInterface::IPAddress{ "192.168.0" }, std::invalid_argument) << "Constructing from an invalid string should throw";
	}

	// Invalid IPV4 string
	{
		auto ip = la::avdecc::networkInterface::IPAddress{};
		EXPECT_THROW(ip = la::avdecc::networkInterface::IPAddress{ "192+168+0+1" }, std::invalid_argument) << "Constructing from an invalid string should throw";
	}

	// Invalid IPV4 string
	{
		auto ip = la::avdecc::networkInterface::IPAddress{};
		EXPECT_THROW(ip = la::avdecc::networkInterface::IPAddress{ "192.168.0.256" }, std::invalid_argument) << "Constructing from an invalid string should throw";
	}

	// Invalid IPV4 string
	{
		auto ip = la::avdecc::networkInterface::IPAddress{};
		EXPECT_THROW(ip = la::avdecc::networkInterface::IPAddress{ "192.168.0.1.1" }, std::invalid_argument) << "Constructing from an invalid string should throw";
	}
}

TEST(IPAddress, ToString)
{
	auto const adrs = la::avdecc::networkInterface::IPAddress{ "10.0.0.0" };

	auto const adrsAsString = static_cast<std::string>(adrs);
	EXPECT_STREQ("10.0.0.0", adrsAsString.c_str());
}

TEST(IPAddress, MakePackedMaskV4)
{
	EXPECT_EQ(la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xFFFFFFFF }, la::avdecc::networkInterface::makePackedMaskV4(40));
	EXPECT_EQ(la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xFFFFFFFF }, la::avdecc::networkInterface::makePackedMaskV4(32));
	EXPECT_EQ(la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xFFFFFFF0 }, la::avdecc::networkInterface::makePackedMaskV4(28));
	EXPECT_EQ(la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xFFFFFF00 }, la::avdecc::networkInterface::makePackedMaskV4(24));
	EXPECT_EQ(la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xFFFFF000 }, la::avdecc::networkInterface::makePackedMaskV4(20));
	EXPECT_EQ(la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xFFFF0000 }, la::avdecc::networkInterface::makePackedMaskV4(16));
	EXPECT_EQ(la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xFF000000 }, la::avdecc::networkInterface::makePackedMaskV4(8));
	EXPECT_EQ(la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0x80000000 }, la::avdecc::networkInterface::makePackedMaskV4(1));
	EXPECT_EQ(la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0x00000000 }, la::avdecc::networkInterface::makePackedMaskV4(0));
}

TEST(IPAddress, ValidateNetmaskV4)
{
	EXPECT_NO_THROW(la::avdecc::networkInterface::validateNetmaskV4(la::avdecc::networkInterface::IPAddress{ la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0x80000000 } }));
	EXPECT_NO_THROW(la::avdecc::networkInterface::validateNetmaskV4(la::avdecc::networkInterface::IPAddress{ la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xC0000000 } }));
	EXPECT_NO_THROW(la::avdecc::networkInterface::validateNetmaskV4(la::avdecc::networkInterface::IPAddress{ la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xF8000000 } }));
	EXPECT_NO_THROW(la::avdecc::networkInterface::validateNetmaskV4(la::avdecc::networkInterface::IPAddress{ la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xFFF00000 } }));
	EXPECT_NO_THROW(la::avdecc::networkInterface::validateNetmaskV4(la::avdecc::networkInterface::IPAddress{ la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xFFFFFFFF } }));

	EXPECT_THROW(la::avdecc::networkInterface::validateNetmaskV4(la::avdecc::networkInterface::IPAddress{ la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0x00000000 } }), std::invalid_argument) << "Empty mask should throw";
	EXPECT_THROW(la::avdecc::networkInterface::validateNetmaskV4(la::avdecc::networkInterface::IPAddress{ la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0x00000000 } }), std::invalid_argument) << "Empty mask should throw";
	EXPECT_THROW(la::avdecc::networkInterface::validateNetmaskV4(la::avdecc::networkInterface::IPAddress{ la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0x40000000 } }), std::invalid_argument) << "Mask doesn't has MSB set";
	EXPECT_THROW(la::avdecc::networkInterface::validateNetmaskV4(la::avdecc::networkInterface::IPAddress{ la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xF4000000 } }), std::invalid_argument) << "Mask starts then stops (not contiguous)";
}

TEST(IPAddress, EqualityOperator)
{
	auto const ip1 = la::avdecc::networkInterface::IPAddress{ "192.168.0.1" };
	auto const ip2 = la::avdecc::networkInterface::IPAddress{ "192.168.0.2" };
	auto const ipSame = la::avdecc::networkInterface::IPAddress{ "192.168.0.1" };

	EXPECT_FALSE(ip1 == ip2);
	EXPECT_FALSE(ipSame == ip2);
	EXPECT_TRUE(ip1 == ipSame);
}

TEST(IPAddress, DifferenceOperator)
{
	auto const ip1 = la::avdecc::networkInterface::IPAddress{ "192.168.0.1" };
	auto const ip2 = la::avdecc::networkInterface::IPAddress{ "192.168.0.2" };
	auto const ipSame = la::avdecc::networkInterface::IPAddress{ "192.168.0.1" };

	EXPECT_TRUE(ip1 != ip2);
	EXPECT_TRUE(ipSame != ip2);
	EXPECT_FALSE(ip1 != ipSame);
}

TEST(IPAddress, InferiorOperator)
{
	auto const ip1 = la::avdecc::networkInterface::IPAddress{ "192.168.0.1" };
	auto const ip2 = la::avdecc::networkInterface::IPAddress{ "192.168.0.2" };
	auto const ipSame = la::avdecc::networkInterface::IPAddress{ "192.168.0.1" };
	auto const ip3 = la::avdecc::networkInterface::IPAddress{ "192.167.0.3" };
	auto const ip4 = la::avdecc::networkInterface::IPAddress{ "192.169.0.1" };

	EXPECT_TRUE(ip1 < ip2);
	EXPECT_FALSE(ip1 < ipSame);
	EXPECT_TRUE(ip3 < ip1);
	EXPECT_TRUE(ip2 < ip4);
}

TEST(IPAddress, InferiorEqualityOperator)
{
	auto const ip1 = la::avdecc::networkInterface::IPAddress{ "192.168.0.1" };
	auto const ip2 = la::avdecc::networkInterface::IPAddress{ "192.168.0.2" };
	auto const ipSame = la::avdecc::networkInterface::IPAddress{ "192.168.0.1" };

	EXPECT_TRUE(ip1 <= ip2);
	EXPECT_TRUE(ip1 <= ipSame);
}

TEST(IPAddress, AdditionOperator)
{
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddress{ "192.168.0.1" } + 1) == la::avdecc::networkInterface::IPAddress{ "192.168.0.2" });
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddress{ "192.168.0.255" } + 1) == la::avdecc::networkInterface::IPAddress{ "192.168.1.0" });
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddress{ "192.168.0.1" } + 0x10000) == la::avdecc::networkInterface::IPAddress{ "192.169.0.1" });
}

TEST(IPAddress, SubstrationOperator)
{
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddress{ "192.168.0.2" } - 1) == la::avdecc::networkInterface::IPAddress{ "192.168.0.1" });
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddress{ "192.168.1.0" } - 1) == la::avdecc::networkInterface::IPAddress{ "192.168.0.255" });
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddress{ "192.168.0.1" } - 0x10000) == la::avdecc::networkInterface::IPAddress{ "192.167.0.1" });
}

TEST(IPAddress, IncrementOperator)
{
	auto ip1 = la::avdecc::networkInterface::IPAddress{ "192.168.0.1" };
	EXPECT_TRUE(++ip1 == la::avdecc::networkInterface::IPAddress{ "192.168.0.2" });

	auto ip2 = la::avdecc::networkInterface::IPAddress{ "192.168.0.255" };
	EXPECT_TRUE(++ip2 == la::avdecc::networkInterface::IPAddress{ "192.168.1.0" });
}

TEST(IPAddress, DecrementOperator)
{
	auto ip1 = la::avdecc::networkInterface::IPAddress{ "192.168.0.2" };
	EXPECT_TRUE(--ip1 == la::avdecc::networkInterface::IPAddress{ "192.168.0.1" });

	auto ip2 = la::avdecc::networkInterface::IPAddress{ "192.168.1.0" };
	EXPECT_TRUE(--ip2 == la::avdecc::networkInterface::IPAddress{ "192.168.0.255" });
}

TEST(IPAddress, AndOperator)
{
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddress{ "192.168.1.1" } & la::avdecc::networkInterface::IPAddress{ "255.255.0.0" }) == la::avdecc::networkInterface::IPAddress{ "192.168.0.0" });
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddress{ "192.168.20.100" } & la::avdecc::networkInterface::IPAddress{ "255.255.240.0" }) == la::avdecc::networkInterface::IPAddress{ "192.168.16.0" });
}

TEST(IPAddress, OrOperator)
{
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddress{ "192.168.1.0" } | la::avdecc::networkInterface::IPAddress{ "0.0.1.1" }) == la::avdecc::networkInterface::IPAddress{ "192.168.1.1" });
	EXPECT_TRUE((la::avdecc::networkInterface::IPAddress{ "192.168.1.0" } | la::avdecc::networkInterface::IPAddress{ "0.0.2.0" }) == la::avdecc::networkInterface::IPAddress{ "192.168.3.0" });
}

TEST(IPAddress, Pack)
{
	EXPECT_EQ((la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xC0A80001 }), la::avdecc::networkInterface::IPAddress::pack(la::avdecc::networkInterface::IPAddress::value_type_v4{ 192u, 168u, 0u, 1u }));
}

TEST(IPAddress, Unpack)
{
	EXPECT_EQ((la::avdecc::networkInterface::IPAddress::value_type_v4{ 192u, 168u, 0u, 1u }), la::avdecc::networkInterface::IPAddress::unpack(la::avdecc::networkInterface::IPAddress::value_type_packed_v4{ 0xC0A80001 }));
}

/* ************************************************************ */
/* IPAddressInfo Tests                                          */
/* ************************************************************ */
TEST(IPAddressInfo, NetworkBaseAddress)
{
	// Valid IPAddressInfo
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.1.1" }, la::avdecc::networkInterface::IPAddress{ "255.255.255.0" } };

		ASSERT_NO_THROW(info.getNetworkBaseAddress());
		EXPECT_STREQ("192.168.1.0", static_cast<std::string>(info.getNetworkBaseAddress()).c_str());
	}

	// Valid IPAddressInfo
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.1.1" }, la::avdecc::networkInterface::IPAddress{ la::avdecc::networkInterface::makePackedMaskV4(24) } }; // "255.255.255.0" is 24 bits

		ASSERT_NO_THROW(info.getNetworkBaseAddress());
		EXPECT_STREQ("192.168.1.0", static_cast<std::string>(info.getNetworkBaseAddress()).c_str());
	}

	// Valid IPAddressInfo
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.20.1" }, la::avdecc::networkInterface::IPAddress{ "255.255.240.0" } };

		ASSERT_NO_THROW(info.getNetworkBaseAddress());
		EXPECT_STREQ("192.168.16.0", static_cast<std::string>(info.getNetworkBaseAddress()).c_str());
	}

	// Valid IPAddressInfo
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.20.1" }, la::avdecc::networkInterface::IPAddress{ la::avdecc::networkInterface::makePackedMaskV4(20) } }; // "255.255.240.0" is 20 bits

		ASSERT_NO_THROW(info.getNetworkBaseAddress());
		EXPECT_STREQ("192.168.16.0", static_cast<std::string>(info.getNetworkBaseAddress()).c_str());
	}

	// TODO: Complete with invalid values
}

TEST(IPAddressInfo, BroadcastAddress)
{
	// Valid IPAddressInfo
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.1.1" }, la::avdecc::networkInterface::IPAddress{ "255.255.255.0" } };

		ASSERT_NO_THROW(info.getBroadcastAddress());
		EXPECT_STREQ("192.168.1.255", static_cast<std::string>(info.getBroadcastAddress()).c_str());
	}

	// Valid IPAddressInfo
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.1.1" }, la::avdecc::networkInterface::IPAddress{ la::avdecc::networkInterface::makePackedMaskV4(24) } }; // "255.255.255.0" is 24 bits

		ASSERT_NO_THROW(info.getBroadcastAddress());
		EXPECT_STREQ("192.168.1.255", static_cast<std::string>(info.getBroadcastAddress()).c_str());
	}

	// Valid IPAddressInfo
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.20.1" }, la::avdecc::networkInterface::IPAddress{ "255.255.240.0" } };

		ASSERT_NO_THROW(info.getBroadcastAddress());
		EXPECT_STREQ("192.168.31.255", static_cast<std::string>(info.getBroadcastAddress()).c_str());
	}

	// Valid IPAddressInfo
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.20.1" }, la::avdecc::networkInterface::IPAddress{ la::avdecc::networkInterface::makePackedMaskV4(20) } }; // "255.255.240.0" is 20 bits

		ASSERT_NO_THROW(info.getBroadcastAddress());
		EXPECT_STREQ("192.168.31.255", static_cast<std::string>(info.getBroadcastAddress()).c_str());
	}

	// TODO: Complete with invalid values
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

TEST(IPAddressInfo, IsPrivateNetworkAddress)
{
	// Class A Start
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "10.0.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.0.0.0" } };

		ASSERT_NO_THROW(info.isPrivateNetworkAddress());
		EXPECT_TRUE(info.isPrivateNetworkAddress());
	}
	// Class A
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "10.1.2.3" }, la::avdecc::networkInterface::IPAddress{ "255.0.0.0" } };

		ASSERT_NO_THROW(info.isPrivateNetworkAddress());
		EXPECT_TRUE(info.isPrivateNetworkAddress());
	}
	// Class A
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "10.1.2.3" }, la::avdecc::networkInterface::IPAddress{ "255.255.128.0" } };

		ASSERT_NO_THROW(info.isPrivateNetworkAddress());
		EXPECT_TRUE(info.isPrivateNetworkAddress());
	}
	// Class A End
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "10.255.255.255" }, la::avdecc::networkInterface::IPAddress{ "255.0.0.0" } };

		ASSERT_NO_THROW(info.isPrivateNetworkAddress());
		EXPECT_TRUE(info.isPrivateNetworkAddress());
	}

	// Class B Start
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.16.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.240.0.0" } };

		ASSERT_NO_THROW(info.isPrivateNetworkAddress());
		EXPECT_TRUE(info.isPrivateNetworkAddress());
	}
	// Class B
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.20.1.2" }, la::avdecc::networkInterface::IPAddress{ "255.240.0.0" } };

		ASSERT_NO_THROW(info.isPrivateNetworkAddress());
		EXPECT_TRUE(info.isPrivateNetworkAddress());
	}
	// Class B
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.20.1.2" }, la::avdecc::networkInterface::IPAddress{ "255.255.128.0" } };

		ASSERT_NO_THROW(info.isPrivateNetworkAddress());
		EXPECT_TRUE(info.isPrivateNetworkAddress());
	}
	// Class B End
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "172.31.255.255" }, la::avdecc::networkInterface::IPAddress{ "255.240.0.0" } };

		ASSERT_NO_THROW(info.isPrivateNetworkAddress());
		EXPECT_TRUE(info.isPrivateNetworkAddress());
	}

	// Class C Start
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.0.0" }, la::avdecc::networkInterface::IPAddress{ "255.255.0.0" } };

		ASSERT_NO_THROW(info.isPrivateNetworkAddress());
		EXPECT_TRUE(info.isPrivateNetworkAddress());
	}
	// Class C
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.1.2" }, la::avdecc::networkInterface::IPAddress{ "255.255.0.0" } };

		ASSERT_NO_THROW(info.isPrivateNetworkAddress());
		EXPECT_TRUE(info.isPrivateNetworkAddress());
	}
	// Class C
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.1.2" }, la::avdecc::networkInterface::IPAddress{ "255.255.255.128" } };

		ASSERT_NO_THROW(info.isPrivateNetworkAddress());
		EXPECT_TRUE(info.isPrivateNetworkAddress());
	}
	// Class C End
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.255.255" }, la::avdecc::networkInterface::IPAddress{ "255.255.0.0" } };

		ASSERT_NO_THROW(info.isPrivateNetworkAddress());
		EXPECT_TRUE(info.isPrivateNetworkAddress());
	}

	// Invalid Class C Netmask -> Throw
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.168.0.1" }, la::avdecc::networkInterface::IPAddress{ "255.255.0.255" } };

		EXPECT_THROW(info.isPrivateNetworkAddress(), std::invalid_argument);
	}

	// Not private
	{
		auto const info = la::avdecc::networkInterface::IPAddressInfo{ la::avdecc::networkInterface::IPAddress{ "192.169.0.1" }, la::avdecc::networkInterface::IPAddress{ "255.255.0.0" } };

		ASSERT_NO_THROW(info.isPrivateNetworkAddress());
		EXPECT_FALSE(info.isPrivateNetworkAddress());
	}

	// TODO: Complete with invalid values
}

#pragma message("TODO: Complete NetworkInterfaceHelper unit tests")
