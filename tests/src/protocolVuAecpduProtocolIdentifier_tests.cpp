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
* @file protocolVuAecpduProtocolIdentifier_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/internals/protocolVuAecpdu.hpp>

#include <gtest/gtest.h>

/***********************************************************/
/* AEM tests                                               */
/***********************************************************/

TEST(VuAecpduProtocolIdentifier, ConstructorIntegral)
{
	auto const pi = la::avdecc::protocol::VuAecpdu::ProtocolIdentifier{ 0xFFFFF3F4F5F6F7F8 };

	// This also tests "operator IntegralType"
	EXPECT_EQ(static_cast<la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::IntegralType>(0x0000F3F4F5F6F7F8), static_cast<la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::IntegralType>(pi));
}

TEST(VuAecpduProtocolIdentifier, ConstructorArray)
{
	auto const pi = la::avdecc::protocol::VuAecpdu::ProtocolIdentifier{ la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::ArrayType{ 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8 } };

	// This also tests "operator ArrayType"
	EXPECT_EQ(static_cast<la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::IntegralType>(0x0000F3F4F5F6F7F8), static_cast<la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::IntegralType>(pi));
}

TEST(VuAecpduProtocolIdentifier, SetIntegralValue)
{
	auto pi = la::avdecc::protocol::VuAecpdu::ProtocolIdentifier{};

	EXPECT_EQ(0ull, static_cast<la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::IntegralType>(pi));
	pi.setValue(0xFFFFF3F4F5F6F7F8);
	EXPECT_EQ(static_cast<la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::IntegralType>(0x0000F3F4F5F6F7F8), static_cast<la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::IntegralType>(pi));
}

TEST(VuAecpduProtocolIdentifier, SetArrayValue)
{
	auto pi = la::avdecc::protocol::VuAecpdu::ProtocolIdentifier{};

	EXPECT_EQ(0ull, static_cast<la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::IntegralType>(pi));
	pi.setValue(la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::ArrayType{ 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8 });
	EXPECT_EQ(static_cast<la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::IntegralType>(0x0000F3F4F5F6F7F8), static_cast<la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::IntegralType>(pi));
}

TEST(VuAecpduProtocolIdentifier, Comparison)
{
	auto const pi1 = la::avdecc::protocol::VuAecpdu::ProtocolIdentifier{ 0xFFFFF3F4F5F6F7F8 };
	auto const pi2 = la::avdecc::protocol::VuAecpdu::ProtocolIdentifier{ 0x0000F3F4F5F6F7F8 };
	auto const pi3 = la::avdecc::protocol::VuAecpdu::ProtocolIdentifier{ 0x000003F4F5F6F7F8 };

	EXPECT_TRUE(pi1 == pi2);
	EXPECT_FALSE(pi1 == pi3);
	EXPECT_FALSE(pi2 == pi3);

	EXPECT_TRUE(pi1 == static_cast<la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::IntegralType>(0x0000F3F4F5F6F7F8));
	EXPECT_TRUE(pi2 == static_cast<la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::IntegralType>(0x0000F3F4F5F6F7F8));
	EXPECT_FALSE(pi3 == static_cast<la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::IntegralType>(0x0000F3F4F5F6F7F8));

	EXPECT_TRUE((pi1 == la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::ArrayType{ 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8 }));
	EXPECT_TRUE((pi2 == la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::ArrayType{ 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8 }));
	EXPECT_FALSE((pi3 == la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::ArrayType{ 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8 }));
}
