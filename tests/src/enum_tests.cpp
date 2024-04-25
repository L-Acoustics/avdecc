/*
* Copyright (C) 2016-2024, L-Acoustics and its contributors

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
* @file enum_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/utils.hpp>

#include <gtest/gtest.h>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <set>

namespace
{
enum class TestEnum : std::uint8_t
{
	None = 0,
	First = 1,
	Other = 4,
};

enum class TestBitfieldEnumTrait : std::uint8_t
{
	None = 0u,
	Implemented = 1u << 0, // 1
	Supported = 1u << 1, // 2
};

}; // namespace

// Define bitfield enum traits for TestBitfieldEnum
template<>
struct la::avdecc::utils::enum_traits<::TestBitfieldEnumTrait>
{
	static constexpr bool is_bitfield = true;
};

TEST(Enum, ToIntegral)
{
	EXPECT_EQ(0, la::avdecc::utils::to_integral(TestEnum::None));
	EXPECT_EQ(1, la::avdecc::utils::to_integral(TestEnum::First));
	EXPECT_EQ(4, la::avdecc::utils::to_integral(TestEnum::Other));
}

TEST(Enum, EnumClassHash)
{
	std::unordered_map<TestEnum, char const*, la::avdecc::utils::EnumClassHash> myEnumToStringMap{
		{ TestEnum::None, "None" },
		{ TestEnum::First, "First" },
		{ TestEnum::Other, "Other" },
	};

	EXPECT_NO_THROW(EXPECT_STREQ("First", myEnumToStringMap.at(TestEnum::First)););
	EXPECT_THROW((void)myEnumToStringMap.at(static_cast<TestEnum>(2));, std::out_of_range);
}

TEST(EnumBitfieldTrait, OperatorAnd)
{
	auto const v1 = TestBitfieldEnumTrait::None;
	auto const v2 = TestBitfieldEnumTrait::Implemented;
	auto const v3 = TestBitfieldEnumTrait::Supported;

	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(0), v1 & TestBitfieldEnumTrait::Implemented);
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(1), v2 & TestBitfieldEnumTrait::Implemented);
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(0), v3 & TestBitfieldEnumTrait::Implemented);
}

TEST(EnumBitfieldTrait, OperatorOr)
{
	auto const v1 = TestBitfieldEnumTrait::None;
	auto const v2 = TestBitfieldEnumTrait::Implemented;
	auto const v3 = TestBitfieldEnumTrait::Supported;

	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(1), v1 | TestBitfieldEnumTrait::Implemented);
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(1), v2 | TestBitfieldEnumTrait::Implemented);
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(3), v3 | TestBitfieldEnumTrait::Implemented);
}

TEST(EnumBitfieldTrait, OperatorOrEqual)
{
	auto v = TestBitfieldEnumTrait::None;
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(0), v);

	v |= TestBitfieldEnumTrait::Implemented;
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(1), v);

	v |= TestBitfieldEnumTrait::Supported;
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(3), v);
}

TEST(EnumBitfieldTrait, HasFlag)
{
	auto const v1 = TestBitfieldEnumTrait::None;
	auto const v2 = TestBitfieldEnumTrait::Implemented;
	auto const v3 = TestBitfieldEnumTrait::Implemented | TestBitfieldEnumTrait::Supported;

	EXPECT_FALSE(la::avdecc::utils::hasFlag(v1, TestBitfieldEnumTrait::None)); // Testing no-bit returns false, obviously
	EXPECT_FALSE(la::avdecc::utils::hasFlag(v1, TestBitfieldEnumTrait::Implemented));
	EXPECT_FALSE(la::avdecc::utils::hasFlag(v1, TestBitfieldEnumTrait::Supported));

	EXPECT_FALSE(la::avdecc::utils::hasFlag(v2, TestBitfieldEnumTrait::None)); // Testing no-bit returns false, obviously
	EXPECT_TRUE(la::avdecc::utils::hasFlag(v2, TestBitfieldEnumTrait::Implemented));
	EXPECT_FALSE(la::avdecc::utils::hasFlag(v2, TestBitfieldEnumTrait::Supported));

	EXPECT_FALSE(la::avdecc::utils::hasFlag(v3, TestBitfieldEnumTrait::None)); // Testing no-bit returns false, obviously
	EXPECT_TRUE(la::avdecc::utils::hasFlag(v3, TestBitfieldEnumTrait::Implemented));
	EXPECT_TRUE(la::avdecc::utils::hasFlag(v3, TestBitfieldEnumTrait::Supported));
}

TEST(EnumBitfieldTrait, HasAnyFlag)
{
	auto const v1 = TestBitfieldEnumTrait::None;
	auto const v2 = TestBitfieldEnumTrait::Implemented;
	auto const v3 = TestBitfieldEnumTrait::Implemented | TestBitfieldEnumTrait::Supported;

	EXPECT_FALSE(la::avdecc::utils::hasAnyFlag(v1));
	EXPECT_TRUE(la::avdecc::utils::hasAnyFlag(v2));
	EXPECT_TRUE(la::avdecc::utils::hasAnyFlag(v3));
}

TEST(EnumBitfieldTrait, AddFlag)
{
	auto v1 = TestBitfieldEnumTrait::None;
	auto v2 = TestBitfieldEnumTrait::Implemented;
	auto v3 = TestBitfieldEnumTrait::Supported;
	auto v4 = TestBitfieldEnumTrait::Implemented | TestBitfieldEnumTrait::Supported;

	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(1), la::avdecc::utils::addFlag(v1, TestBitfieldEnumTrait::Implemented));
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(1), la::avdecc::utils::addFlag(v2, TestBitfieldEnumTrait::Implemented));
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(3), la::avdecc::utils::addFlag(v3, TestBitfieldEnumTrait::Implemented));
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(3), la::avdecc::utils::addFlag(v4, TestBitfieldEnumTrait::Implemented));

	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(1), v1);
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(1), v2);
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(3), v3);
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(3), v4);
}

TEST(EnumBitfieldTrait, ClearFlag)
{
	auto v1 = TestBitfieldEnumTrait::None;
	auto v2 = TestBitfieldEnumTrait::Implemented;
	auto v3 = TestBitfieldEnumTrait::Supported;
	auto v4 = TestBitfieldEnumTrait::Implemented | TestBitfieldEnumTrait::Supported;

	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(0), la::avdecc::utils::clearFlag(v1, TestBitfieldEnumTrait::Implemented));
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(0), la::avdecc::utils::clearFlag(v2, TestBitfieldEnumTrait::Implemented));
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(2), la::avdecc::utils::clearFlag(v3, TestBitfieldEnumTrait::Implemented));
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(2), la::avdecc::utils::clearFlag(v4, TestBitfieldEnumTrait::Implemented));

	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(0), v1);
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(0), v2);
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(2), v3);
	EXPECT_EQ(static_cast<TestBitfieldEnumTrait>(2), v4);
}

template<typename EnumBitfield>
class EnumBitfieldTest final
{
private:
	using TestBitfieldClass = EnumBitfield;
	using TestBitfieldClasses = la::avdecc::utils::EnumBitfield<EnumBitfield>;

	void testTypes() const
	{
		EXPECT_EQ(typeid(TestBitfieldClass), typeid(typename TestBitfieldClasses::value_type));
		EXPECT_EQ(typeid(std::underlying_type_t<TestBitfieldClass>), typeid(typename TestBitfieldClasses::underlying_value_type));
		EXPECT_EQ(sizeof(std::underlying_type_t<TestBitfieldClass>) * 8, TestBitfieldClasses::value_size);
	}

	void testHash() const
	{
		std::unordered_map<TestBitfieldClasses, char const*, typename TestBitfieldClasses::Hash> myBitfieldToStringMap{
			{ TestBitfieldClasses{}, "None" },
			{ TestBitfieldClasses{ TestBitfieldClass::Implemented }, "Implemented" },
			{ TestBitfieldClasses{ TestBitfieldClass::Other }, "Other" },
		};

		EXPECT_NO_THROW(EXPECT_STREQ("Implemented", myBitfieldToStringMap.at(TestBitfieldClasses{ TestBitfieldClass::Implemented })););
		EXPECT_THROW((void)myBitfieldToStringMap.at(TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Other });, std::out_of_range);
		EXPECT_THROW((void)myBitfieldToStringMap.at(TestBitfieldClasses{ static_cast<TestBitfieldClass>(2) });, std::out_of_range);
	}

	void testConstructionAndValue() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		constexpr auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		constexpr auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented };
		constexpr auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		constexpr auto v6 = TestBitfieldClasses{};
		constexpr auto v7 = TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::Supported, TestBitfieldClass::Supported };

		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v1.value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v2.value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v3.value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented), v4.value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v5.value());
		EXPECT_EQ(0u, v6.value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v7.value());
	}

	void testAssign() const
	{
		auto v1_1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		auto v1_2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		auto v2_1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		auto v2_2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		auto v3_1 = TestBitfieldClasses{};

		v1_1.assign(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented));
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented), v1_1.value());
		v1_2.assign(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported));
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v1_2.value());

		v2_1.assign(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented));
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented), v2_1.value());
		v2_2.assign(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported));
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v2_2.value());

		v3_1.assign(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented));
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented), v3_1.value());
	}

	void testClear() const
	{
		auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported, TestBitfieldClass::Other };
		auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		auto v4 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		auto v5 = TestBitfieldClasses{ TestBitfieldClass::Other };
		auto v6 = TestBitfieldClasses{};

		v1.clear();
		EXPECT_TRUE(v1.empty());
		v2.clear();
		EXPECT_TRUE(v2.empty());
		v3.clear();
		EXPECT_TRUE(v3.empty());
		v4.clear();
		EXPECT_TRUE(v4.empty());
		v5.clear();
		EXPECT_TRUE(v5.empty());
		v6.clear();
		EXPECT_TRUE(v6.empty());
	}

	void testEqualityOperator() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		constexpr auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		constexpr auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented };
		constexpr auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		constexpr auto v6 = TestBitfieldClasses{};

		EXPECT_TRUE((v1 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v1 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v1 == TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v1 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v1 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v1 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v1 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v1 == TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v1 == TestBitfieldClasses{ TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v1 == TestBitfieldClasses{ TestBitfieldClass::Supported }));
		EXPECT_FALSE((v1 == TestBitfieldClasses{ TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v1 == TestBitfieldClasses{}));

		EXPECT_FALSE((v2 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v2 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v2 == TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v2 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v2 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v2 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v2 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v2 == TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v2 == TestBitfieldClasses{ TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v2 == TestBitfieldClasses{ TestBitfieldClass::Supported }));
		EXPECT_FALSE((v2 == TestBitfieldClasses{ TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v2 == TestBitfieldClasses{}));

		EXPECT_FALSE((v3 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v3 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v3 == TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v3 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v3 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v3 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v3 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v3 == TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v3 == TestBitfieldClasses{ TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v3 == TestBitfieldClasses{ TestBitfieldClass::Supported }));
		EXPECT_FALSE((v3 == TestBitfieldClasses{ TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v3 == TestBitfieldClasses{}));

		EXPECT_FALSE((v4 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v4 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v4 == TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v4 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v4 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v4 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v4 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v4 == TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v4 == TestBitfieldClasses{ TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v4 == TestBitfieldClasses{ TestBitfieldClass::Supported }));
		EXPECT_FALSE((v4 == TestBitfieldClasses{ TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v4 == TestBitfieldClasses{}));

		EXPECT_FALSE((v5 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v5 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v5 == TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v5 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v5 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v5 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v5 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v5 == TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v5 == TestBitfieldClasses{ TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v5 == TestBitfieldClasses{ TestBitfieldClass::Supported }));
		EXPECT_FALSE((v5 == TestBitfieldClasses{ TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v5 == TestBitfieldClasses{}));

		EXPECT_FALSE((v6 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v6 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v6 == TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v6 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v6 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v6 == TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v6 == TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v6 == TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v6 == TestBitfieldClasses{ TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v6 == TestBitfieldClasses{ TestBitfieldClass::Supported }));
		EXPECT_FALSE((v6 == TestBitfieldClasses{ TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v6 == TestBitfieldClasses{}));
	}

	void testDifferenceOperator() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		constexpr auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		constexpr auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented };
		constexpr auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		constexpr auto v6 = TestBitfieldClasses{};

		EXPECT_FALSE((v1 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v1 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v1 != TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v1 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v1 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v1 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v1 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v1 != TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v1 != TestBitfieldClasses{ TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v1 != TestBitfieldClasses{ TestBitfieldClass::Supported }));
		EXPECT_TRUE((v1 != TestBitfieldClasses{ TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v1 != TestBitfieldClasses{}));

		EXPECT_TRUE((v2 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v2 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v2 != TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v2 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v2 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v2 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v2 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v2 != TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v2 != TestBitfieldClasses{ TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v2 != TestBitfieldClasses{ TestBitfieldClass::Supported }));
		EXPECT_TRUE((v2 != TestBitfieldClasses{ TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v2 != TestBitfieldClasses{}));

		EXPECT_TRUE((v3 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v3 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_FALSE((v3 != TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v3 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v3 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v3 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v3 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v3 != TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v3 != TestBitfieldClasses{ TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v3 != TestBitfieldClasses{ TestBitfieldClass::Supported }));
		EXPECT_TRUE((v3 != TestBitfieldClasses{ TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v3 != TestBitfieldClasses{}));

		EXPECT_TRUE((v4 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v4 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v4 != TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v4 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v4 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v4 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v4 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v4 != TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v4 != TestBitfieldClasses{ TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v4 != TestBitfieldClasses{ TestBitfieldClass::Supported }));
		EXPECT_TRUE((v4 != TestBitfieldClasses{ TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v4 != TestBitfieldClasses{}));

		EXPECT_TRUE((v5 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v5 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v5 != TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v5 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v5 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v5 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v5 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v5 != TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v5 != TestBitfieldClasses{ TestBitfieldClass::Implemented }));
		EXPECT_FALSE((v5 != TestBitfieldClasses{ TestBitfieldClass::Supported }));
		EXPECT_TRUE((v5 != TestBitfieldClasses{ TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v5 != TestBitfieldClasses{}));

		EXPECT_TRUE((v6 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v6 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v6 != TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v6 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v6 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::NotSupported }));
		EXPECT_TRUE((v6 != TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v6 != TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported }));
		EXPECT_TRUE((v6 != TestBitfieldClasses{ TestBitfieldClass::Supported, TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v6 != TestBitfieldClasses{ TestBitfieldClass::Implemented }));
		EXPECT_TRUE((v6 != TestBitfieldClasses{ TestBitfieldClass::Supported }));
		EXPECT_TRUE((v6 != TestBitfieldClasses{ TestBitfieldClass::NotSupported }));
		EXPECT_FALSE((v6 != TestBitfieldClasses{}));
	}

	void testOrEqualOperator() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		constexpr auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		constexpr auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented };
		constexpr auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		constexpr auto v6 = TestBitfieldClasses{};

		{
			auto v = v1;
			v |= v2;
			EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v.value());
		}
		{
			auto v = v2;
			v |= v1;
			EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v.value());
		}
		{
			auto v = v2;
			v |= v3;
			EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v.value());
		}
		{
			auto v = v2;
			v |= v4;
			EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v.value());
		}
		{
			auto v = v3;
			v |= v5;
			EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v.value());
		}
		{
			auto v = v6;
			v |= v5;
			EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v.value());
			v |= v4;
			EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v.value());
			v |= v3;
			EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v.value());
		}
	}

	void testAndEqualOperator() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		constexpr auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		constexpr auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented };
		constexpr auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };

		{
			auto v = v1;
			v &= v2;
			EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v.value());
		}
		{
			auto v = v2;
			v &= v1;
			EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v.value());
		}
		{
			auto v = v2;
			v &= v3;
			EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v.value());
		}
		{
			auto v = v2;
			v &= v4;
			EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented), v.value());
		}
		{
			auto v = v3;
			v &= v4;
			EXPECT_TRUE(v.empty());
		}
		{
			auto v = v1;
			v &= v3;
			EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v.value());
			v &= v5;
			EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v.value());
			v &= v4;
			EXPECT_TRUE(v.empty());
		}
	}

	void testOrOperator() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		constexpr auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		constexpr auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented };
		constexpr auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		constexpr auto v6 = TestBitfieldClasses{};

		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), (v1 | v2).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), (v2 | v1).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), (v2 | v3).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), (v2 | v4).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), (v3 | v5).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Supported), (v6 | v5).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), (v6 | v5 | v4).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), (v6 | v5 | v4 | v3).value());
	}

	void testAndOperator() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		constexpr auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		constexpr auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented };
		constexpr auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };

		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), (v1 & v2).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), (v2 & v1).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Supported), (v2 & v3).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented), (v2 & v4).value());
		EXPECT_TRUE((v3 & v4).empty());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), (v1 & v3).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Supported), (v1 & v3 & v5).value());
		EXPECT_TRUE((v1 & v3 & v5 & v4).empty());
	}

	void testTestBit() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		constexpr auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		constexpr auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented };
		constexpr auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		constexpr auto v6 = TestBitfieldClasses{};

		EXPECT_TRUE(v1.test(TestBitfieldClass::Implemented));
		EXPECT_TRUE(v1.test(TestBitfieldClass::Supported));
		EXPECT_TRUE(v1.test(TestBitfieldClass::NotSupported));

		EXPECT_TRUE(v2.test(TestBitfieldClass::Implemented));
		EXPECT_TRUE(v2.test(TestBitfieldClass::Supported));
		EXPECT_FALSE(v2.test(TestBitfieldClass::NotSupported));

		EXPECT_FALSE(v3.test(TestBitfieldClass::Implemented));
		EXPECT_TRUE(v3.test(TestBitfieldClass::Supported));
		EXPECT_TRUE(v3.test(TestBitfieldClass::NotSupported));

		EXPECT_TRUE(v4.test(TestBitfieldClass::Implemented));
		EXPECT_FALSE(v4.test(TestBitfieldClass::Supported));
		EXPECT_FALSE(v4.test(TestBitfieldClass::NotSupported));

		EXPECT_FALSE(v5.test(TestBitfieldClass::Implemented));
		EXPECT_TRUE(v5.test(TestBitfieldClass::Supported));
		EXPECT_FALSE(v5.test(TestBitfieldClass::NotSupported));

		EXPECT_FALSE(v6.test(TestBitfieldClass::Implemented));
		EXPECT_FALSE(v6.test(TestBitfieldClass::Supported));
		EXPECT_FALSE(v6.test(TestBitfieldClass::NotSupported));
	}

	void testSetBit() const
	{
		auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented };
		auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		auto v6 = TestBitfieldClasses{};

		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v1.set(TestBitfieldClass::Implemented).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v1.set(TestBitfieldClass::Supported).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v1.set(TestBitfieldClass::NotSupported).value());

		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v2.set(TestBitfieldClass::Implemented).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v2.set(TestBitfieldClass::Supported).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v2.set(TestBitfieldClass::NotSupported).value());

		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v3.set(TestBitfieldClass::Implemented).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v3.set(TestBitfieldClass::Supported).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v3.set(TestBitfieldClass::NotSupported).value());

		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented), v4.set(TestBitfieldClass::Implemented).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v4.set(TestBitfieldClass::Supported).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v4.set(TestBitfieldClass::NotSupported).value());

		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v5.set(TestBitfieldClass::Implemented).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v5.set(TestBitfieldClass::Supported).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v5.set(TestBitfieldClass::NotSupported).value());

		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented), v6.set(TestBitfieldClass::Implemented).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v6.set(TestBitfieldClass::Supported).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Implemented) | la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v6.set(TestBitfieldClass::NotSupported).value());
	}

	void testCount() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::Other };
		constexpr auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		constexpr auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Other };
		constexpr auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		constexpr auto v6 = TestBitfieldClasses{};

		EXPECT_EQ(3u, v1.count());
		EXPECT_EQ(3u, v2.count());
		EXPECT_EQ(2u, v3.count());
		EXPECT_EQ(2u, v4.count());
		EXPECT_EQ(1u, v5.count());
		EXPECT_EQ(0u, v6.count());
	}

	void testSize() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		constexpr auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		constexpr auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented };
		constexpr auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		constexpr auto v6 = TestBitfieldClasses{};

		EXPECT_EQ(sizeof(std::underlying_type_t<TestBitfieldClass>) * 8u, v1.size());
		EXPECT_EQ(sizeof(std::underlying_type_t<TestBitfieldClass>) * 8u, v2.size());
		EXPECT_EQ(sizeof(std::underlying_type_t<TestBitfieldClass>) * 8u, v3.size());
		EXPECT_EQ(sizeof(std::underlying_type_t<TestBitfieldClass>) * 8u, v4.size());
		EXPECT_EQ(sizeof(std::underlying_type_t<TestBitfieldClass>) * 8u, v5.size());
		EXPECT_EQ(sizeof(std::underlying_type_t<TestBitfieldClass>) * 8u, v6.size());
	}

	void testEmpty() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		constexpr auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		constexpr auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented };
		constexpr auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		constexpr auto v6 = TestBitfieldClasses{};

		EXPECT_FALSE(v1.empty());
		EXPECT_FALSE(v2.empty());
		EXPECT_FALSE(v3.empty());
		EXPECT_FALSE(v4.empty());
		EXPECT_FALSE(v5.empty());
		EXPECT_TRUE(v6.empty());
	}

	void testResetBit() const
	{
		auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented };
		auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		auto v6 = TestBitfieldClasses{};

		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v1.reset(TestBitfieldClass::Implemented).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v1.reset(TestBitfieldClass::Supported).value());
		EXPECT_EQ(0u, v1.reset(TestBitfieldClass::NotSupported).value());

		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v2.reset(TestBitfieldClass::Implemented).value());
		EXPECT_EQ(0u, v2.reset(TestBitfieldClass::Supported).value());
		EXPECT_EQ(0u, v2.reset(TestBitfieldClass::NotSupported).value());

		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Supported) | la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v3.reset(TestBitfieldClass::Implemented).value());
		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::NotSupported), v3.reset(TestBitfieldClass::Supported).value());
		EXPECT_EQ(0u, v3.reset(TestBitfieldClass::NotSupported).value());

		EXPECT_EQ(0u, v4.reset(TestBitfieldClass::Implemented).value());
		EXPECT_EQ(0u, v4.reset(TestBitfieldClass::Supported).value());
		EXPECT_EQ(0u, v4.reset(TestBitfieldClass::NotSupported).value());

		EXPECT_EQ(la::avdecc::utils::to_integral(TestBitfieldClass::Supported), v5.reset(TestBitfieldClass::Implemented).value());
		EXPECT_EQ(0u, v5.reset(TestBitfieldClass::Supported).value());
		EXPECT_EQ(0u, v5.reset(TestBitfieldClass::NotSupported).value());

		EXPECT_EQ(0u, v6.reset(TestBitfieldClass::Implemented).value());
		EXPECT_EQ(0u, v6.reset(TestBitfieldClass::Supported).value());
		EXPECT_EQ(0u, v6.reset(TestBitfieldClass::NotSupported).value());
	}

	void testIterator() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::Other };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Implemented };

		{
			auto it = v1.begin();
			EXPECT_NE(v1.end(), it);
			EXPECT_EQ(TestBitfieldClass::Implemented, *it);

			++it;
			EXPECT_NE(v1.end(), it);
			EXPECT_EQ(TestBitfieldClass::Supported, *it);

			++it;
			EXPECT_NE(v1.end(), it);
			EXPECT_EQ(TestBitfieldClass::NotSupported, *it);

			++it;
			EXPECT_NE(v1.end(), it);
			EXPECT_EQ(TestBitfieldClass::Other, *it);

			++it;
			EXPECT_EQ(v1.end(), it);
		}
		{
			auto it = v2.begin();
			EXPECT_NE(v2.end(), it);
			EXPECT_EQ(TestBitfieldClass::Implemented, *it);

			++it;
			EXPECT_NE(v2.end(), it);
			EXPECT_EQ(TestBitfieldClass::NotSupported, *it);

			++it;
			EXPECT_EQ(v2.end(), it);
		}

		{
			auto const it = v1.begin();
			EXPECT_EQ(TestBitfieldClass::Implemented, *(it + 0));
			EXPECT_EQ(TestBitfieldClass::Supported, *(it + 1));
			EXPECT_EQ(TestBitfieldClass::NotSupported, *(it + 2));
			EXPECT_EQ(TestBitfieldClass::Other, *(it + 3));
			EXPECT_EQ(v1.end(), it + 4);
		}
		{
			auto const it = v2.begin();
			EXPECT_EQ(TestBitfieldClass::Implemented, *(it + 0));
			EXPECT_EQ(TestBitfieldClass::NotSupported, *(it + 1));
			EXPECT_EQ(v2.end(), it + 2);
		}
	}

	void testAt() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::Other };
		constexpr auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		constexpr auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Other };
		constexpr auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		constexpr auto v6 = TestBitfieldClasses{};

		EXPECT_EQ(TestBitfieldClass::Implemented, v1.at(0));
		EXPECT_EQ(TestBitfieldClass::Supported, v1.at(1));
		EXPECT_EQ(TestBitfieldClass::NotSupported, v1.at(2));
		for (auto i = v1.count(); i < v1.size(); ++i)
		{
			EXPECT_THROW(v1.at(i);, std::out_of_range);
		}

		EXPECT_EQ(TestBitfieldClass::Implemented, v2.at(0));
		EXPECT_EQ(TestBitfieldClass::Supported, v2.at(1));
		EXPECT_EQ(TestBitfieldClass::Other, v2.at(2));
		for (auto i = v2.count(); i < v2.size(); ++i)
		{
			EXPECT_THROW(v2.at(i);, std::out_of_range);
		}

		EXPECT_EQ(TestBitfieldClass::Supported, v3.at(0));
		EXPECT_EQ(TestBitfieldClass::NotSupported, v3.at(1));
		for (auto i = v3.count(); i < v3.size(); ++i)
		{
			EXPECT_THROW(v3.at(i);, std::out_of_range);
		}

		EXPECT_EQ(TestBitfieldClass::Implemented, v4.at(0));
		EXPECT_EQ(TestBitfieldClass::Other, v4.at(1));
		for (auto i = v4.count(); i < v4.size(); ++i)
		{
			EXPECT_THROW(v4.at(i);, std::out_of_range);
		}

		EXPECT_EQ(TestBitfieldClass::Supported, v5.at(0));
		for (auto i = v5.count(); i < v5.size(); ++i)
		{
			EXPECT_THROW(v5.at(i);, std::out_of_range);
		}

		for (auto i = v6.count(); i < v6.size(); ++i)
		{
			EXPECT_THROW(v6.at(i);, std::out_of_range);
		}
	}

	void testGetBitSetPosition() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::Other };
		constexpr auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		constexpr auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Other };
		constexpr auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		constexpr auto v6 = TestBitfieldClasses{};

		EXPECT_EQ(0u, v1.getBitSetPosition(TestBitfieldClass::Implemented));
		EXPECT_EQ(1u, v1.getBitSetPosition(TestBitfieldClass::Supported));
		EXPECT_EQ(2u, v1.getBitSetPosition(TestBitfieldClass::NotSupported));
		EXPECT_THROW(v1.getBitSetPosition(TestBitfieldClass::Other);, std::out_of_range);

		EXPECT_EQ(0u, v2.getBitSetPosition(TestBitfieldClass::Implemented));
		EXPECT_EQ(1u, v2.getBitSetPosition(TestBitfieldClass::Supported));
		EXPECT_EQ(2u, v2.getBitSetPosition(TestBitfieldClass::Other));
		EXPECT_THROW(v2.getBitSetPosition(TestBitfieldClass::NotSupported);, std::out_of_range);

		EXPECT_EQ(0u, v3.getBitSetPosition(TestBitfieldClass::Supported));
		EXPECT_EQ(1u, v3.getBitSetPosition(TestBitfieldClass::NotSupported));
		EXPECT_THROW(v3.getBitSetPosition(TestBitfieldClass::Implemented);, std::out_of_range);
		EXPECT_THROW(v3.getBitSetPosition(TestBitfieldClass::Other);, std::out_of_range);

		EXPECT_EQ(0u, v4.getBitSetPosition(TestBitfieldClass::Implemented));
		EXPECT_EQ(1u, v4.getBitSetPosition(TestBitfieldClass::Other));
		EXPECT_THROW(v4.getBitSetPosition(TestBitfieldClass::Supported);, std::out_of_range);
		EXPECT_THROW(v4.getBitSetPosition(TestBitfieldClass::NotSupported);, std::out_of_range);

		EXPECT_EQ(0u, v5.getBitSetPosition(TestBitfieldClass::Supported));
		EXPECT_THROW(v5.getBitSetPosition(TestBitfieldClass::Implemented);, std::out_of_range);
		EXPECT_THROW(v5.getBitSetPosition(TestBitfieldClass::NotSupported);, std::out_of_range);
		EXPECT_THROW(v5.getBitSetPosition(TestBitfieldClass::Other);, std::out_of_range);

		EXPECT_THROW(v6.getBitSetPosition(TestBitfieldClass::Implemented);, std::out_of_range);
		EXPECT_THROW(v6.getBitSetPosition(TestBitfieldClass::Supported);, std::out_of_range);
		EXPECT_THROW(v6.getBitSetPosition(TestBitfieldClass::NotSupported);, std::out_of_range);
		EXPECT_THROW(v6.getBitSetPosition(TestBitfieldClass::Other);, std::out_of_range);
	}

	void testGetPosition() const
	{
		EXPECT_THROW(TestBitfieldClasses::getPosition(static_cast<TestBitfieldClass>(3)), std::logic_error);
		EXPECT_EQ(0u, TestBitfieldClasses::getPosition(TestBitfieldClass::Implemented));
		EXPECT_EQ(3u, TestBitfieldClasses::getPosition(TestBitfieldClass::Supported));
		EXPECT_EQ(5u, TestBitfieldClasses::getPosition(TestBitfieldClass::NotSupported));
		EXPECT_EQ(TestBitfieldClasses::value_size - 2, TestBitfieldClasses::getPosition(TestBitfieldClass::Other));
	}

	void testEnumerateBits() const
	{
		constexpr auto v1 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
		constexpr auto v2 = TestBitfieldClasses{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
		constexpr auto v3 = TestBitfieldClasses{ TestBitfieldClass::NotSupported, TestBitfieldClass::Supported };
		constexpr auto v4 = TestBitfieldClasses{ TestBitfieldClass::Implemented };
		constexpr auto v5 = TestBitfieldClasses{ TestBitfieldClass::Supported };
		constexpr auto v6 = TestBitfieldClasses{};

		// V1
		{
			std::set<TestBitfieldClass> ControlSet{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
			std::set<TestBitfieldClass> enumerated{};
			for (auto v : v1)
			{
				auto const [it, inserted] = enumerated.insert(v);
				EXPECT_TRUE(inserted);
			}
			EXPECT_EQ(ControlSet, enumerated);
		}

		// V2
		{
			std::set<TestBitfieldClass> ControlSet{ TestBitfieldClass::Implemented, TestBitfieldClass::Supported };
			std::set<TestBitfieldClass> enumerated{};
			for (auto v : v2)
			{
				auto const [it, inserted] = enumerated.insert(v);
				EXPECT_TRUE(inserted);
			}
			EXPECT_EQ(ControlSet, enumerated);
		}

		// V3
		{
			std::set<TestBitfieldClass> ControlSet{ TestBitfieldClass::Supported, TestBitfieldClass::NotSupported };
			std::set<TestBitfieldClass> enumerated{};
			for (auto v : v3)
			{
				auto const [it, inserted] = enumerated.insert(v);
				EXPECT_TRUE(inserted);
			}
			EXPECT_EQ(ControlSet, enumerated);
		}

		// V4
		{
			std::set<TestBitfieldClass> ControlSet{ TestBitfieldClass::Implemented };
			std::set<TestBitfieldClass> enumerated{};
			for (auto v : v4)
			{
				auto const [it, inserted] = enumerated.insert(v);
				EXPECT_TRUE(inserted);
			}
			EXPECT_EQ(ControlSet, enumerated);
		}

		// V5
		{
			std::set<TestBitfieldClass> ControlSet{ TestBitfieldClass::Supported };
			std::set<TestBitfieldClass> enumerated{};
			for (auto v : v5)
			{
				auto const [it, inserted] = enumerated.insert(v);
				EXPECT_TRUE(inserted);
			}
			EXPECT_EQ(ControlSet, enumerated);
		}

		// V6
		{
			std::set<TestBitfieldClass> ControlSet{};
			std::set<TestBitfieldClass> enumerated{};
			for (auto v : v6)
			{
				auto const [it, inserted] = enumerated.insert(v);
				EXPECT_TRUE(inserted);
			}
			EXPECT_EQ(ControlSet, enumerated);
		}
	}

	void testMemoryFootprint() const
	{
		EXPECT_EQ(sizeof(std::underlying_type_t<TestBitfieldClass>), sizeof(TestBitfieldClasses));
	}

public:
	void runAllTests() const
	{
		testTypes();
		testHash();
		testConstructionAndValue();
		testAssign();
		testClear();
		testEqualityOperator();
		testDifferenceOperator();
		testOrEqualOperator();
		testAndEqualOperator();
		testOrOperator();
		testAndOperator();
		testTestBit();
		testSetBit();
		testCount();
		testSize();
		testEmpty();
		testResetBit();
		testIterator();
		testAt();
		testGetBitSetPosition();
		testGetPosition();
		testEnumerateBits();
		testMemoryFootprint();
	}
};

TEST(EnumBitfieldClass, uint8)
{
	enum class TestBitfieldClass : std::uint8_t
	{
		Implemented = 1u << 0, // 1
		Supported = 1u << 3, // 8
		NotSupported = 1u << 5, // 32
		Other = 1u << 6, // 64
	};

	EnumBitfieldTest<TestBitfieldClass>{}.runAllTests();
}

TEST(EnumBitfieldClass, uint16)
{
	enum class TestBitfieldClass : std::uint16_t
	{
		Implemented = 1u << 0, // 1
		Supported = 1u << 3, // 8
		NotSupported = 1u << 5, // 32
		Other = 1u << 14, // 16384
	};

	EnumBitfieldTest<TestBitfieldClass>{}.runAllTests();
}

TEST(EnumBitfieldClass, uint32)
{
	enum class TestBitfieldClass : std::uint32_t
	{
		Implemented = 1u << 0, // 1
		Supported = 1u << 3, // 8
		NotSupported = 1u << 5, // 32
		Other = 1u << 30, // 1073741824
	};

	EnumBitfieldTest<TestBitfieldClass>{}.runAllTests();
}

TEST(EnumBitfieldClass, uint64)
{
	enum class TestBitfieldClass : std::uint64_t
	{
		Implemented = 1ull << 0, // 1
		Supported = 1ull << 3, // 8
		NotSupported = 1ull << 5, // 32
		Other = 1ull << 62, // 4611686018427387904
	};

	EnumBitfieldTest<TestBitfieldClass>{}.runAllTests();
}
