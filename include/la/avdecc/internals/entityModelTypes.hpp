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

/**
* @file entityModelTypes.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model types.
*/

#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <iostream>
#include <cstring> // std::memcpy
#include <algorithm> // std::min

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{

using VendorEntityModel = std::uint64_t;
using ConfigurationIndex = std::uint16_t;
using LocaleIndex = std::uint16_t;
using StringsIndex = std::uint16_t;
using LocalizedStringReference = std::uint16_t;
using StreamIndex = std::uint16_t;
using StreamFormat = std::uint64_t;
using DescriptorIndex = std::uint16_t;
using MapIndex = std::uint16_t;

/** UTF-8 String */
class AvdeccFixedString final
{
	static constexpr size_t MaxLength = 64;
public:
	using value_type = char;

	/** Default constructor */
	AvdeccFixedString() noexcept = default;

	/** Constructor from a std::string */
	AvdeccFixedString(std::string const& str) noexcept
	{
		assign(str);
	}

	/** Constructor from a raw buffer */
	AvdeccFixedString(void const* const ptr, size_t const size) noexcept
	{
		assign(ptr, size);
	}

	/** Default destructor */
	~AvdeccFixedString() noexcept = default;

	/** Assign from a std::string */
	void assign(std::string const& str) noexcept
	{
		assign(str.c_str(), str.size());
	}

	/** Assign from a raw buffer */
	void assign(void const* const ptr, size_t const size) noexcept
	{
		auto const copySize = std::min(MaxLength, size);

		// Copy std::string to internal std::array
		auto* dstPtr = _buffer.data();
		std::memcpy(dstPtr, ptr, copySize);

		// Fill remaining bytes with \0
		for (auto i = copySize; i < MaxLength; ++i)
		{
			dstPtr[i] = '\0';
		}
	}

	/** Returns the raw buffer */
	value_type* data() noexcept
	{
		return _buffer.data();
	}

	/** Returns the raw buffer (const) (might not be NULL terminated) */
	value_type const* data() const noexcept
	{
		return _buffer.data();
	}

	/** Returns the (fixed) size of the buffer */
	size_t size() const noexcept
	{
		return MaxLength;
	}

	/** operator== */
	bool operator==(AvdeccFixedString const& afs) const noexcept
	{
		return _buffer == afs._buffer;
	}

	/** operator== overload for a std::string */
	bool operator==(std::string const& str) const noexcept
	{
		return operator std::string() == str;
	}

	/** Returns this AvdeccFixedString as a std::string */
	std::string str() const noexcept
	{
		return operator std::string();
	}

	/** Returns this AvdeccFixedString as a std::string */
	operator std::string() const noexcept
	{
		std::string str{};

		// If all bytes in an AvdeccFixedString are used, the buffer is not NULL-terminated. We cannot use strlen or directly copy the buffer into an std::string or we might overflow
		for (auto const c : _buffer)
		{
			if (c == 0)
				break;
			str.push_back(c);
		}

		return str;
	}

	// Defaulted compiler auto-generated methods
	AvdeccFixedString(AvdeccFixedString&&) = default;
	AvdeccFixedString(AvdeccFixedString const&) = default;
	AvdeccFixedString& operator=(AvdeccFixedString const&) = default;
	AvdeccFixedString& operator=(AvdeccFixedString&&) = default;

private:
	std::array<value_type, MaxLength> _buffer{};
};

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la

/** ostream overload for an AvdeccFixedString */
inline std::ostream& operator<<(std::ostream& os, la::avdecc::entity::model::AvdeccFixedString const& rhs)
{
	os << rhs.str();
	return os;
}

