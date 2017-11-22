/*
* Copyright (C) 2016-2017, L-Acoustics and its contributors

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

#include <gtest/gtest.h>
#include "la/avdecc/internals/entityModelTypes.hpp"

TEST(AvdeccFixedString, DefaultConstructor)
{
	la::avdecc::entity::model::AvdeccFixedString const afs;

	ASSERT_EQ(64u, afs.size()) << "Should be 64 bytes long";
	ASSERT_EQ(typeid(char), typeid(la::avdecc::entity::model::AvdeccFixedString::value_type)) << "Underlying type should be 'char'";

	EXPECT_TRUE(afs.empty());
	EXPECT_STREQ("", afs.str().c_str()) << "String should be empty";

	auto const* const ptr = afs.data();
	for (auto i = 0u; i < afs.size(); ++i)
	{
		EXPECT_EQ(0u, ptr[i]) << "Each value should be '\0' initialized";
	}
}

TEST(AvdeccFixedString, StdStringConstructor)
{
	// Small string
	{
		std::string const str{ "Hi" };
		la::avdecc::entity::model::AvdeccFixedString const afs{ str };

		EXPECT_FALSE(afs.empty());
		EXPECT_STREQ(str.c_str(), afs.str().c_str());

		auto const* const ptr = afs.data();
		for (auto i = str.size(); i < afs.size(); ++i)
		{
			EXPECT_EQ(0u, ptr[i]) << "Other values should be '\0' initialized";
		}
	}
	// Oversized string
	{
		std::string const str{ "This is a string that should be contain more than 64 bytes in it!" };
		ASSERT_LT(64u, str.size());
		la::avdecc::entity::model::AvdeccFixedString const afs{ str };

		EXPECT_FALSE(afs.empty());
		EXPECT_STRNE(str.c_str(), afs.str().c_str());
		{
			auto const subStr = str.substr(0, 64);
			EXPECT_STREQ(subStr.c_str(), afs.str().c_str()) << "Should be equal to truncated string";
		}
	}
}

TEST(AvdeccFixedString, RawBufferConstructor)
{
	char const* const str{ "Hi" };
	auto const size = strlen(str);
	la::avdecc::entity::model::AvdeccFixedString const afs{ str, size };

	EXPECT_FALSE(afs.empty());
	EXPECT_STREQ(str, afs.str().c_str());

	auto const* const ptr = afs.data();
	for (auto i = size; i < afs.size(); ++i)
	{
		EXPECT_EQ(0u, ptr[i]) << "Other values should be '\0' initialized";
	}
}

TEST(AvdeccFixedString, AssignStdString)
{
	la::avdecc::entity::model::AvdeccFixedString afs;

	// Assign a long buffer
	{
		std::string const str{ "Hello" };
		afs.assign(str);
		EXPECT_FALSE(afs.empty());
		EXPECT_STREQ(str.c_str(), afs.str().c_str());

		auto const* const ptr = afs.data();
		for (auto i = str.size(); i < afs.size(); ++i)
		{
			EXPECT_EQ(0u, ptr[i]) << "Other values should be '\0' initialized";
		}
	}

	// Assign a shorter buffer (old value should be zero-ed)
	{
		std::string const str{ "Hi" };
		afs.assign(str);
		EXPECT_FALSE(afs.empty());
		EXPECT_STREQ(str.c_str(), afs.str().c_str());

		auto const* const ptr = afs.data();
		for (auto i = str.size(); i < afs.size(); ++i)
		{
			EXPECT_EQ(0u, ptr[i]) << "Other values should be '\0' initialized";
		}
	}
}

TEST(AvdeccFixedString, AssignRawBuffer)
{
	la::avdecc::entity::model::AvdeccFixedString afs;

	// Assign a long buffer
	{
		char const* const str{ "Hello" };
		auto const size = strlen(str);
		afs.assign(str);
		EXPECT_FALSE(afs.empty());
		EXPECT_STREQ(str, afs.str().c_str());

		auto const* const ptr = afs.data();
		for (auto i = size; i < afs.size(); ++i)
		{
			EXPECT_EQ(0u, ptr[i]) << "Other values should be '\0' initialized";
		}
	}

	// Assign a shorter buffer (old value should be zero-ed)
	{
		char const* const str{ "Hi" };
		auto const size = strlen(str);
		afs.assign(str);
		EXPECT_FALSE(afs.empty());
		EXPECT_STREQ(str, afs.str().c_str());

		auto const* const ptr = afs.data();
		for (auto i = size; i < afs.size(); ++i)
		{
			EXPECT_EQ(0u, ptr[i]) << "Other values should be '\0' initialized";
		}
	}
}

TEST(AvdeccFixedString, ComparisonOperator)
{
	std::string const str{ "Hi" };
	la::avdecc::entity::model::AvdeccFixedString const afs{ str };
	la::avdecc::entity::model::AvdeccFixedString const afs2{ "Hi" };
	la::avdecc::entity::model::AvdeccFixedString const afs3{ "Hi!" };

	EXPECT_FALSE(afs.empty());
	EXPECT_STREQ(afs.str().c_str(), afs2.str().c_str());
	EXPECT_TRUE(afs == afs2);

	EXPECT_FALSE(afs2.empty());
	EXPECT_FALSE(afs3.empty());
	EXPECT_STRNE(afs.str().c_str(), afs3.str().c_str());
	EXPECT_STRNE(afs2.str().c_str(), afs3.str().c_str());
	EXPECT_FALSE(afs == afs3);
	EXPECT_FALSE(afs2 == afs3);
}

TEST(AvdeccFixedString, CopyConstructor)
{
	std::string const str{ "Hi" };
	la::avdecc::entity::model::AvdeccFixedString const afs{ str };
	la::avdecc::entity::model::AvdeccFixedString const afs2{ afs };

	EXPECT_TRUE(afs == afs2);
}

TEST(AvdeccFixedString, EqualOperator)
{
	std::string const str{ "Hi" };
	la::avdecc::entity::model::AvdeccFixedString const afs{ str };
	la::avdecc::entity::model::AvdeccFixedString afs2;

	afs2 = afs;
	EXPECT_TRUE(afs == afs2);
}
