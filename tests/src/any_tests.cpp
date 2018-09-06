/*
* Copyright (C) 2016-2018, L-Acoustics and its contributors

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
* @file any_tests.cpp
* @author Christophe Calmejane
*/

#if ENABLE_AVDECC_CUSTOM_ANY
#include <la/avdecc/internals/any.hpp>
#include <gtest/gtest.h>

// Some tests for our std::any implementation, until it's available on all c++17 compilers
TEST(Any, ConstructorLiteral)
{
	std::any v = std::make_any<std::uint32_t>(123u);
	EXPECT_EQ(typeid(std::uint32_t), v.type());
	EXPECT_NE(typeid(float), v.type());

	EXPECT_THROW(
		std::any_cast<float>(v);
	, std::bad_any_cast);

	std::uint32_t value{ 0u };
	EXPECT_NO_THROW(
		value = std::any_cast<std::uint32_t>(v);
	);
	EXPECT_EQ(123u, value);
}

TEST(Any, ConstructorStruct)
{
	struct Test
	{
		int a{ 0 };
		int b{ 1 };
	};

	std::any v{ Test{} };
	EXPECT_EQ(typeid(Test), v.type());

	Test value{};
	EXPECT_NO_THROW(
		value = std::any_cast<Test>(v);
	);
	EXPECT_EQ(0, value.a);
	EXPECT_EQ(1, value.b);

	v = Test{ 5, 6 };
	EXPECT_EQ(typeid(Test), v.type());
	EXPECT_NO_THROW(
		value = std::any_cast<Test>(v);
	);
	EXPECT_EQ(5, value.a);
	EXPECT_EQ(6, value.b);
}

TEST(Any, CopyOperator)
{
	std::any v = std::make_any<std::uint32_t>(123u);
	EXPECT_EQ(typeid(std::uint32_t), v.type());

	// Assign to another type
	float value{ 0.0f };
	v = 1.0f;
	EXPECT_EQ(typeid(float), v.type());
	EXPECT_NO_THROW(
		value = std::any_cast<float>(v);
	);
	EXPECT_EQ(1.0f, value);
}

#endif // ENABLE_AVDECC_CUSTOM_ANY
