/*
* Copyright (C) 2016-2025, L-Acoustics and its contributors

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
* @file utils_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/utils.hpp>

#include <gtest/gtest.h>
#include <cstdint>

class UtilsTest : public ::testing::Test
{
public:
	virtual void SetUp() override {}
	virtual void TearDown() override {}
};

TEST_F(UtilsTest, ReverseBits_ZeroValue)
{
	EXPECT_EQ(std::uint8_t(0x00), la::avdecc::utils::reverseBits(std::uint8_t(0x00)));
	EXPECT_EQ(std::uint16_t(0x0000), la::avdecc::utils::reverseBits(std::uint16_t(0x0000)));
	EXPECT_EQ(std::uint32_t(0x00000000), la::avdecc::utils::reverseBits(std::uint32_t(0x00000000)));
	EXPECT_EQ(std::uint64_t(0x0000000000000000), la::avdecc::utils::reverseBits(std::uint64_t(0x0000000000000000)));
}

TEST_F(UtilsTest, ReverseBits_AllOnes)
{
	EXPECT_EQ(std::uint8_t(0xFF), la::avdecc::utils::reverseBits(std::uint8_t(0xFF)));
	EXPECT_EQ(std::uint16_t(0xFFFF), la::avdecc::utils::reverseBits(std::uint16_t(0xFFFF)));
	EXPECT_EQ(std::uint32_t(0xFFFFFFFF), la::avdecc::utils::reverseBits(std::uint32_t(0xFFFFFFFF)));
	EXPECT_EQ(std::uint64_t(0xFFFFFFFFFFFFFFFF), la::avdecc::utils::reverseBits(std::uint64_t(0xFFFFFFFFFFFFFFFF)));
}

TEST_F(UtilsTest, ReverseBits32_SingleBit)
{
	// Test each individual bit position for 32-bit
	for (int i = 0; i < 32; ++i)
	{
		std::uint32_t input = 1u << i;
		std::uint32_t expected = 1u << (31 - i);
		EXPECT_EQ(expected, la::avdecc::utils::reverseBits(input)) << "Failed for bit position " << i << " (input: 0x" << std::hex << input << ", expected: 0x" << expected << ")";
	}
}

TEST_F(UtilsTest, ReverseBits16_SingleBit)
{
	// Test each individual bit position for 16-bit
	for (int i = 0; i < 16; ++i)
	{
		std::uint16_t input = std::uint16_t(1u << i);
		std::uint16_t expected = std::uint16_t(1u << (15 - i));
		EXPECT_EQ(expected, la::avdecc::utils::reverseBits(input)) << "Failed for bit position " << i << " (input: 0x" << std::hex << input << ", expected: 0x" << expected << ")";
	}
}

TEST_F(UtilsTest, ReverseBits8_SingleBit)
{
	// Test each individual bit position for 8-bit
	for (int i = 0; i < 8; ++i)
	{
		std::uint8_t input = std::uint8_t(1u << i);
		std::uint8_t expected = std::uint8_t(1u << (7 - i));
		EXPECT_EQ(expected, la::avdecc::utils::reverseBits(input)) << "Failed for bit position " << i << " (input: 0x" << std::hex << +input << ", expected: 0x" << +expected << ")";
	}
}

TEST_F(UtilsTest, ReverseBits64_SingleBit)
{
	// Test each individual bit position for 64-bit
	for (int i = 0; i < 64; ++i)
	{
		std::uint64_t input = 1ull << i;
		std::uint64_t expected = 1ull << (63 - i);
		EXPECT_EQ(expected, la::avdecc::utils::reverseBits(input)) << "Failed for bit position " << i << " (input: 0x" << std::hex << input << ", expected: 0x" << expected << ")";
	}
}

TEST_F(UtilsTest, ReverseBits32_KnownValues)
{
	// Test some known values for 32-bit
	EXPECT_EQ(0x80000000u, la::avdecc::utils::reverseBits(0x00000001u));
	EXPECT_EQ(0x40000000u, la::avdecc::utils::reverseBits(0x00000002u));
	EXPECT_EQ(0x20000000u, la::avdecc::utils::reverseBits(0x00000004u));
	EXPECT_EQ(0x10000000u, la::avdecc::utils::reverseBits(0x00000008u));

	EXPECT_EQ(0x00000001u, la::avdecc::utils::reverseBits(0x80000000u));
	EXPECT_EQ(0x00000002u, la::avdecc::utils::reverseBits(0x40000000u));
	EXPECT_EQ(0x00000004u, la::avdecc::utils::reverseBits(0x20000000u));
	EXPECT_EQ(0x00000008u, la::avdecc::utils::reverseBits(0x10000000u));
}

TEST_F(UtilsTest, ReverseBits16_KnownValues)
{
	// Test some known values for 16-bit
	EXPECT_EQ(std::uint16_t(0x8000), la::avdecc::utils::reverseBits(std::uint16_t(0x0001)));
	EXPECT_EQ(std::uint16_t(0x4000), la::avdecc::utils::reverseBits(std::uint16_t(0x0002)));
	EXPECT_EQ(std::uint16_t(0x0001), la::avdecc::utils::reverseBits(std::uint16_t(0x8000)));
	EXPECT_EQ(std::uint16_t(0x0002), la::avdecc::utils::reverseBits(std::uint16_t(0x4000)));
}

TEST_F(UtilsTest, ReverseBits8_KnownValues)
{
	// Test some known values for 8-bit
	EXPECT_EQ(std::uint8_t(0x80), la::avdecc::utils::reverseBits(std::uint8_t(0x01)));
	EXPECT_EQ(std::uint8_t(0x40), la::avdecc::utils::reverseBits(std::uint8_t(0x02)));
	EXPECT_EQ(std::uint8_t(0x01), la::avdecc::utils::reverseBits(std::uint8_t(0x80)));
	EXPECT_EQ(std::uint8_t(0x02), la::avdecc::utils::reverseBits(std::uint8_t(0x40)));
}

TEST_F(UtilsTest, ReverseBits64_KnownValues)
{
	// Test some known values for 64-bit
	EXPECT_EQ(0x8000000000000000ull, la::avdecc::utils::reverseBits(0x0000000000000001ull));
	EXPECT_EQ(0x4000000000000000ull, la::avdecc::utils::reverseBits(0x0000000000000002ull));
	EXPECT_EQ(0x0000000000000001ull, la::avdecc::utils::reverseBits(0x8000000000000000ull));
	EXPECT_EQ(0x0000000000000002ull, la::avdecc::utils::reverseBits(0x4000000000000000ull));
}

TEST_F(UtilsTest, ReverseBits_Patterns)
{
	// Test alternating patterns for different sizes
	EXPECT_EQ(std::uint8_t(0x55), la::avdecc::utils::reverseBits(std::uint8_t(0xAA)));
	EXPECT_EQ(std::uint8_t(0xAA), la::avdecc::utils::reverseBits(std::uint8_t(0x55)));

	EXPECT_EQ(std::uint16_t(0x5555), la::avdecc::utils::reverseBits(std::uint16_t(0xAAAA)));
	EXPECT_EQ(std::uint16_t(0xAAAA), la::avdecc::utils::reverseBits(std::uint16_t(0x5555)));

	EXPECT_EQ(0x55555555u, la::avdecc::utils::reverseBits(0xAAAAAAAAu));
	EXPECT_EQ(0xAAAAAAAAu, la::avdecc::utils::reverseBits(0x55555555u));

	EXPECT_EQ(0x5555555555555555ull, la::avdecc::utils::reverseBits(0xAAAAAAAAAAAAAAAAull));
	EXPECT_EQ(0xAAAAAAAAAAAAAAAAull, la::avdecc::utils::reverseBits(0x5555555555555555ull));

	// Test nibble patterns
	EXPECT_EQ(0x0F0F0F0Fu, la::avdecc::utils::reverseBits(0xF0F0F0F0u));
	EXPECT_EQ(0xF0F0F0F0u, la::avdecc::utils::reverseBits(0x0F0F0F0Fu));

	// Test byte patterns
	EXPECT_EQ(0x00FF00FFu, la::avdecc::utils::reverseBits(0xFF00FF00u));
	EXPECT_EQ(0xFF00FF00u, la::avdecc::utils::reverseBits(0x00FF00FFu));
}

TEST_F(UtilsTest, ReverseBits_DoubleReverse)
{
	// Double reverse should return original value
	std::vector<std::uint8_t> testValues8 = { 0x00, 0xFF, 0x12, 0x87, 0xAA, 0x55, 0xF0, 0x0F };
	std::vector<std::uint16_t> testValues16 = { 0x0000, 0xFFFF, 0x1234, 0x8765, 0xAAAA, 0x5555, 0xF0F0, 0x0F0F };
	std::vector<std::uint32_t> testValues32 = { 0x00000000u, 0xFFFFFFFFu, 0x12345678u, 0x87654321u, 0xAAAAAAAAu, 0x55555555u, 0xF0F0F0F0u, 0x0F0F0F0Fu, 0xFF00FF00u, 0x00FF00FFu, 0xDEADBEEFu, 0xCAFEBABEu };
	std::vector<std::uint64_t> testValues64 = { 0x0000000000000000ull, 0xFFFFFFFFFFFFFFFFull, 0x123456789ABCDEFull, 0xFEDCBA9876543210ull, 0xAAAAAAAAAAAAAAAAull, 0x5555555555555555ull, 0xF0F0F0F0F0F0F0F0ull, 0x0F0F0F0F0F0F0F0Full };

	for (auto value : testValues8)
	{
		auto reversed = la::avdecc::utils::reverseBits(value);
		auto doubleReversed = la::avdecc::utils::reverseBits(reversed);
		EXPECT_EQ(value, doubleReversed) << "Double reverse failed for 8-bit value 0x" << std::hex << +value;
	}

	for (auto value : testValues16)
	{
		auto reversed = la::avdecc::utils::reverseBits(value);
		auto doubleReversed = la::avdecc::utils::reverseBits(reversed);
		EXPECT_EQ(value, doubleReversed) << "Double reverse failed for 16-bit value 0x" << std::hex << value;
	}

	for (auto value : testValues32)
	{
		auto reversed = la::avdecc::utils::reverseBits(value);
		auto doubleReversed = la::avdecc::utils::reverseBits(reversed);
		EXPECT_EQ(value, doubleReversed) << "Double reverse failed for 32-bit value 0x" << std::hex << value;
	}

	for (auto value : testValues64)
	{
		auto reversed = la::avdecc::utils::reverseBits(value);
		auto doubleReversed = la::avdecc::utils::reverseBits(reversed);
		EXPECT_EQ(value, doubleReversed) << "Double reverse failed for 64-bit value 0x" << std::hex << value;
	}
}

TEST_F(UtilsTest, Pow)
{
	// Test compile-time pow with integral_constant
	constexpr auto pow2_8 = std::integral_constant<std::uint32_t, la::avdecc::utils::pow(2, 8)>::value;
	constexpr auto pow2_15 = std::integral_constant<std::uint32_t, la::avdecc::utils::pow(2, 15)>::value;

	EXPECT_EQ(256u, pow2_8);
	EXPECT_EQ(32768u, pow2_15);
}
