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
* @file memoryBuffer_tests.cpp
* @author Christophe Calmejane
*/

#include <gtest/gtest.h>
#include <la/avdecc/memoryBuffer.hpp>

TEST(MemoryBuffer, DefaultConstructor)
{
	la::avdecc::MemoryBuffer b;
	ASSERT_EQ(nullptr, b.data());
	ASSERT_EQ(0u, b.capacity());
	ASSERT_EQ(0u, b.size());
	ASSERT_TRUE(b.empty());
}

TEST(MemoryBuffer, StringConstructor)
{
	auto s = std::string("Hello");
	la::avdecc::MemoryBuffer b(s);
	ASSERT_EQ(5u, b.capacity());
	ASSERT_EQ(5u, b.size());
	ASSERT_FALSE(b.empty());
	ASSERT_EQ(0, std::string(b.data(), b.data() + b.size()).compare(s));
}

TEST(MemoryBuffer, VectorConstructor)
{
	std::vector<std::uint8_t> v{ 0, 5, 255 };
	la::avdecc::MemoryBuffer b(v);
	ASSERT_EQ(v.size() * sizeof(decltype(v)::value_type), b.capacity());
	ASSERT_EQ(3u, b.size());
	ASSERT_FALSE(b.empty());
	for (auto i = 0u; i < v.size(); ++i)
		ASSERT_EQ(v[i], b.data()[i]);
}

TEST(MemoryBuffer, BufferConstructor)
{
	auto s = std::string("Hello");
	la::avdecc::MemoryBuffer b(s.data(), s.size());
	ASSERT_EQ(5u, b.capacity());
	ASSERT_EQ(5u, b.size());
	ASSERT_FALSE(b.empty());
	ASSERT_EQ(0, std::string(b.data(), b.data() + b.size()).compare(s));
}

TEST(MemoryBuffer, CopyConstructor)
{
	la::avdecc::MemoryBuffer b1;
	auto s = std::string("Hello");
	b1.assign(s);
	ASSERT_EQ(5u, b1.capacity());
	ASSERT_EQ(5u, b1.size());
	ASSERT_FALSE(b1.empty());
	ASSERT_EQ(0, std::string(b1.data(), b1.data() + b1.size()).compare(s));

	la::avdecc::MemoryBuffer b2(b1);
	ASSERT_NE(nullptr, b2.data());
	ASSERT_NE(b2.data(), b1.data());
	ASSERT_EQ(b2.size(), b1.size());
	ASSERT_FALSE(b2.empty());
	ASSERT_GE(b2.capacity(), b1.size());

	b1.clear();
	la::avdecc::MemoryBuffer b3(b1);
	ASSERT_EQ(nullptr, b3.data());
	ASSERT_EQ(0u, b3.capacity());
	ASSERT_EQ(0u, b3.size());
	ASSERT_TRUE(b3.empty());
}

TEST(MemoryBuffer, CopyOperator)
{
	la::avdecc::MemoryBuffer b1;
	auto s = std::string("Hello");
	b1.assign(s);
	ASSERT_EQ(5u, b1.capacity());
	ASSERT_EQ(5u, b1.size());
	ASSERT_FALSE(b1.empty());
	ASSERT_EQ(0, std::string(b1.data(), b1.data() + b1.size()).compare(s));

	la::avdecc::MemoryBuffer b2;
	ASSERT_EQ(nullptr, b2.data());
	ASSERT_EQ(0u, b2.capacity());
	ASSERT_EQ(0u, b2.size());
	ASSERT_TRUE(b2.empty());
	b2 = b1;
	ASSERT_NE(nullptr, b2.data());
	ASSERT_NE(b2.data(), b1.data());
	ASSERT_EQ(b2.size(), b1.size());
	ASSERT_FALSE(b2.empty());
	ASSERT_GE(b2.capacity(), b1.size());

	b1.clear();
	la::avdecc::MemoryBuffer b3;
	ASSERT_EQ(nullptr, b3.data());
	ASSERT_EQ(0u, b3.capacity());
	ASSERT_EQ(0u, b3.size());
	ASSERT_TRUE(b3.empty());
	b3 = b1;
	ASSERT_EQ(nullptr, b3.data());
	ASSERT_EQ(0u, b3.capacity());
	ASSERT_EQ(0u, b3.size());
	ASSERT_TRUE(b3.empty());
}

TEST(MemoryBuffer, MoveOperator)
{
	la::avdecc::MemoryBuffer b1;
	auto s = std::string("Hello");
	b1.assign(s);
	ASSERT_EQ(5u, b1.capacity());
	ASSERT_EQ(5u, b1.size());
	ASSERT_FALSE(b1.empty());
	ASSERT_EQ(0, std::string(b1.data(), b1.data() + b1.size()).compare(s));

	la::avdecc::MemoryBuffer b2;
	ASSERT_EQ(nullptr, b2.data());
	ASSERT_EQ(0u, b2.capacity());
	ASSERT_EQ(0u, b2.size());
	ASSERT_TRUE(b2.empty());
	auto tb2 = b1;
	b2 = std::move(tb2);
	ASSERT_NE(nullptr, b2.data());
	ASSERT_NE(b2.data(), b1.data());
	ASSERT_EQ(b2.size(), b1.size());
	ASSERT_FALSE(b2.empty());
	ASSERT_GE(b2.capacity(), b1.size());
	ASSERT_EQ(nullptr, tb2.data());
	ASSERT_EQ(0u, tb2.capacity());
	ASSERT_EQ(0u, tb2.size());
	ASSERT_TRUE(tb2.empty());
	auto b2_raw = b2.data();
	ASSERT_EQ('H', *b2_raw);
	// Now move again to b2, which should invalidate (free) previous pointer
	b2 = std::move(tb2);

	b1.clear();
	la::avdecc::MemoryBuffer b3;
	ASSERT_EQ(nullptr, b3.data());
	ASSERT_EQ(0u, b3.capacity());
	ASSERT_EQ(0u, b3.size());
	ASSERT_TRUE(b3.empty());
	auto tb3 = b1;
	b3 = std::move(tb3);
	ASSERT_EQ(nullptr, b3.data());
	ASSERT_EQ(0u, b3.capacity());
	ASSERT_EQ(0u, b3.size());
	ASSERT_TRUE(b3.empty());
	ASSERT_EQ(nullptr, tb3.data());
	ASSERT_EQ(0u, tb3.capacity());
	ASSERT_EQ(0u, tb3.size());
	ASSERT_TRUE(tb3.empty());
}

TEST(MemoryBuffer, MoveConstructor)
{
	la::avdecc::MemoryBuffer b1;
	auto s = std::string("Hello");
	b1.assign(s);
	ASSERT_EQ(5u, b1.capacity());
	ASSERT_EQ(5u, b1.size());
	ASSERT_FALSE(b1.empty());

	la::avdecc::MemoryBuffer b2(std::move(b1));
	ASSERT_NE(nullptr, b2.data());
	ASSERT_GE(b2.capacity(), b2.size());
	ASSERT_EQ(5u, b2.size());
	ASSERT_FALSE(b2.empty());
	ASSERT_EQ(nullptr, b1.data());
	ASSERT_EQ(0u, b1.capacity());
	ASSERT_EQ(0u, b1.size());
	ASSERT_TRUE(b1.empty());
}

TEST(MemoryBuffer, AssignAppendPointer)
{
	la::avdecc::MemoryBuffer b;
	auto s1 = std::string("Hello");
	b.assign(s1.data(), s1.size());
	ASSERT_NE(nullptr, b.data());
	ASSERT_EQ(5u, b.capacity());
	ASSERT_EQ(5u, b.size());
	ASSERT_FALSE(b.empty());
	ASSERT_EQ(0, std::string(b.data(), b.data() + b.size()).compare(s1));

	auto s2 = std::string("World");
	b.append(s2.data(), s2.size());
	ASSERT_NE(nullptr, b.data());
	ASSERT_EQ(5u + 5u, b.capacity());
	ASSERT_EQ(5u + 5u, b.size());
	ASSERT_FALSE(b.empty());

	ASSERT_EQ(0, std::string(b.data(), b.data() + b.size()).compare(s1 + s2));
}

TEST(MemoryBuffer, AssignAppendString)
{
	la::avdecc::MemoryBuffer b;
	auto s1 = std::string("Hello");
	b.assign(s1);
	ASSERT_NE(nullptr, b.data());
	ASSERT_EQ(5u, b.capacity());
	ASSERT_EQ(5u, b.size());
	ASSERT_FALSE(b.empty());
	ASSERT_EQ(0, std::string(b.data(), b.data() + b.size()).compare(s1));

	auto s2 = std::string("World");
	b.append(s2);
	ASSERT_NE(nullptr, b.data());
	ASSERT_EQ(5u + 5u, b.capacity());
	ASSERT_EQ(5u + 5u, b.size());
	ASSERT_FALSE(b.empty());

	ASSERT_EQ(0, std::string(b.data(), b.data() + b.size()).compare(s1 + s2));
}

TEST(MemoryBuffer, AssignAppendVectorSameType)
{
	la::avdecc::MemoryBuffer b;
	auto s1 = std::string("Hello");
	auto v1 = std::vector<la::avdecc::MemoryBuffer::value_type>(s1.data(), s1.data() + s1.size());
	ASSERT_EQ(0, s1.compare(std::string(v1.data(), v1.data() + v1.size())));
	b.assign(v1);
	ASSERT_NE(nullptr, b.data());
	ASSERT_EQ(5u, b.capacity());
	ASSERT_EQ(5u, b.size());
	ASSERT_FALSE(b.empty());
	ASSERT_EQ(0, std::string(b.data(), b.data() + b.size()).compare(s1));

	auto s2 = std::string("World");
	auto v2 = std::vector<la::avdecc::MemoryBuffer::value_type>(s2.data(), s2.data() + s2.size());
	ASSERT_EQ(0, s2.compare(std::string(v2.data(), v2.data() + v2.size())));
	b.append(v2);
	ASSERT_NE(nullptr, b.data());
	ASSERT_EQ(5u + 5u, b.capacity());
	ASSERT_EQ(5u + 5u, b.size());
	ASSERT_FALSE(b.empty());
	ASSERT_EQ(0, std::string(b.data(), b.data() + b.size()).compare(s1 + s2));
}

TEST(MemoryBuffer, Reserve)
{
	la::avdecc::MemoryBuffer b;

	b.reserve(0);
	ASSERT_EQ(nullptr, b.data());
	ASSERT_EQ(0u, b.capacity());
	ASSERT_EQ(0u, b.size());
	ASSERT_TRUE(b.empty());

	b.reserve(50);
	ASSERT_NE(nullptr, b.data());
	ASSERT_EQ(50u, b.capacity());
	ASSERT_EQ(0u, b.size());
	ASSERT_TRUE(b.empty());

	b.reserve(20);
	ASSERT_NE(nullptr, b.data());
	ASSERT_EQ(50u, b.capacity());
	ASSERT_EQ(0u, b.size());
	ASSERT_TRUE(b.empty());
}

TEST(MemoryBuffer, Shrink)
{
	la::avdecc::MemoryBuffer b;
	b.reserve(50);
	b.assign("Hello");

	// Reduce capacity to actual used size
	b.shrink_to_fit();
	ASSERT_NE(nullptr, b.data());
	ASSERT_EQ(5u, b.capacity());
	ASSERT_EQ(5u, b.size());
	ASSERT_FALSE(b.empty());

	// Free the storage (not the buffer itself)
	b.clear();
	b.shrink_to_fit();
	ASSERT_EQ(nullptr, b.data());
	ASSERT_EQ(0u, b.capacity());
	ASSERT_EQ(0u, b.size());
	ASSERT_TRUE(b.empty());

	// Try to shrink a freed storage
	b.shrink_to_fit();
	ASSERT_EQ(nullptr, b.data());
	ASSERT_EQ(0u, b.capacity());
	ASSERT_EQ(0u, b.size());
	ASSERT_TRUE(b.empty());
}

TEST(MemoryBuffer, SetSize)
{
	la::avdecc::MemoryBuffer b;
	b.reserve(50);

	auto s = std::string("Hello");
	std::memcpy(b.data(), s.data(), s.size());
	ASSERT_NE(nullptr, b.data());
	ASSERT_EQ(50u, b.capacity());
	ASSERT_EQ(0u, b.size());
	ASSERT_TRUE(b.empty());

	b.set_size(s.size());
	ASSERT_EQ(50u, b.capacity());
	ASSERT_EQ(s.size(), b.size());
	ASSERT_FALSE(b.empty());

	b.set_size(500);
	ASSERT_NE(nullptr, b.data());
	ASSERT_EQ(500u, b.capacity());
	ASSERT_EQ(500u, b.size());
	ASSERT_FALSE(b.empty());
}

TEST(MemoryBuffer, ConsumeBytes)
{
	la::avdecc::MemoryBuffer b;
	std::string s = "HELLO MY WORLD";
	b.assign(s);
	b.append((char)0);
	ASSERT_STREQ(s.c_str(), (char const*)b.data());
	ASSERT_EQ(strlen(s.data()) + 1, b.size());

	b.consume_size(strlen("HELLO "));
	ASSERT_STREQ("MY WORLD", (char const*)b.data());
	ASSERT_EQ(strlen("MY WORLD") + 1, b.size());
}
