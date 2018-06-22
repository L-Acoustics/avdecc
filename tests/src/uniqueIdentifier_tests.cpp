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

#include <gtest/gtest.h>
#include <la/avdecc/internals/uniqueIdentifier.hpp>
#include <unordered_map>
#include <set>

TEST(UniqueIdentifier, DefaultConstructor)
{
	la::avdecc::UniqueIdentifier eid;

	EXPECT_FALSE(eid);
	EXPECT_FALSE(eid.isValid());
	EXPECT_EQ(0xFFFFFFFFFFFFFFFF, eid.getValue()) << "Default constructor value should be 0xFFFFFFFFFFFFFFFF, although this is an implementation detail";
}

TEST(UniqueIdentifier, IsValid)
{
	auto const nullEID = la::avdecc::UniqueIdentifier::getNullUniqueIdentifier();
	auto const uninitEID = la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier();
	auto const eid{ la::avdecc::UniqueIdentifier{ 0x123 } };

	EXPECT_FALSE(nullEID);
	EXPECT_FALSE(nullEID.isValid());
	EXPECT_EQ(0x0000000000000000, nullEID.getValue()) << "getNullUniqueIdentifier value should be 0xFFFFFFFFFFFFFFFF, although this is an implementation detail";

	EXPECT_FALSE(uninitEID);
	EXPECT_FALSE(uninitEID.isValid());
	EXPECT_EQ(0xFFFFFFFFFFFFFFFF, uninitEID.getValue()) << "getUninitializedUniqueIdentifier value should be 0xFFFFFFFFFFFFFFFF, although this is an implementation detail";

	EXPECT_TRUE(eid);
	EXPECT_TRUE(eid.isValid());
	EXPECT_EQ(0x123, eid.getValue());
}

TEST(UniqueIdentifier, Hash)
{
	// Check if compilation works for a std::unordered_map that requires a std::hash
	std::unordered_map<la::avdecc::UniqueIdentifier, int, la::avdecc::UniqueIdentifier::hash> unorderedMap;
}

TEST(UniqueIdentifier, Set)
{
	// Check if compilation works for a std::set that requires operator<
	std::set<la::avdecc::UniqueIdentifier> set;

	set.emplace(la::avdecc::UniqueIdentifier{});
}

TEST(UniqueIdentifier, EqualityOperator)
{
	auto const nullEID = la::avdecc::UniqueIdentifier::getNullUniqueIdentifier();
	auto const uninitEID = la::avdecc::UniqueIdentifier::getUninitializedUniqueIdentifier();
	auto const eid1{ la::avdecc::UniqueIdentifier{ 0x123 } };
	auto const eid2{ la::avdecc::UniqueIdentifier{ 0x321 } };
	auto const eid3{ la::avdecc::UniqueIdentifier{0x123} };

	EXPECT_TRUE((nullEID == uninitEID));
	EXPECT_TRUE((uninitEID == nullEID));
	
	EXPECT_FALSE((nullEID == eid1));
	EXPECT_FALSE((eid1 == nullEID));
	
	EXPECT_FALSE((uninitEID == eid1));
	EXPECT_FALSE((eid1 == uninitEID));
	
	EXPECT_FALSE((eid1 == eid2));
	EXPECT_FALSE((eid2 == eid1));
	
	EXPECT_TRUE((eid1 == eid3));
	EXPECT_TRUE((eid3 == eid1));
}
