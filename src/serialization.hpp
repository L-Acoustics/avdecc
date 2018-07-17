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
* @file serialization.hpp
* @author Christophe Calmejane
* @brief Simple buffer serializer/deserializer.
*/

#pragma once

#include <cstdint>
#include <type_traits>
#include <exception>
#include <array>
#include <cstring> // memcpy
#include "la/avdecc/internals/endian.hpp"
#include "la/avdecc/utils.hpp"
#include "la/avdecc/internals/entityModel.hpp"
#include "la/avdecc/networkInterfaceHelper.hpp"

namespace la
{
namespace avdecc
{

/* SERIALIZATION */
template<size_t MaximumSize>
class Serializer
{
public:
	static constexpr size_t maximum_size = MaximumSize;

	/** Initializes a serializer with a default initial size */
	Serializer() = default;

	/** Gets raw pointer to serialized buffer */
	std::uint8_t const* data() const
	{
		return _buffer.data();
	}

	/** Gets size of serialized buffer */
	size_t size() const
	{
		return _pos;
	}

	/** Serializes any arithmetic type (including enums) */
	template<typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value || std::is_enum<T>::value>>
	Serializer& operator<<(T const& v)
	{
		// Check enough room in buffer
		if (remaining() < sizeof(v))
		{
			throw std::invalid_argument("Not enough room to serialize");
		}

		// Copy data to buffer
		T* const ptr = reinterpret_cast<T*>(_buffer.data() + _pos);
		*ptr = AVDECC_PACK_TYPE(v, T);

		// Advance data pointer
		_pos += sizeof(v);

		return *this;
	}

	/** Serializes any TypedDefine type */
	template<typename T>
	Serializer& operator<<(TypedDefine<T> const& v)
	{
		// Check enough room in buffer
		if (remaining() < sizeof(v))
		{
			throw std::invalid_argument("Not enough room to serialize");
		}

		// Copy value to buffer
		T* const ptr = reinterpret_cast<T*>(_buffer.data() + _pos);
		*ptr = AVDECC_PACK_TYPE(v.getValue(), T);

		// Advance data pointer
		_pos += sizeof(v);

		return *this;
	}

	/** Serializes a UniqueIdentifier */
	Serializer& operator<<(UniqueIdentifier const& v)
	{
		return operator<<(v.getValue());
	}

	/** Serializes an AvdeccFixedString (without changing endianess) */
	Serializer& operator<<(entity::model::AvdeccFixedString const& v)
	{
		packBuffer(v.data(), v.size());
		return *this;
	}

	/** Serializes a MacAddress (without changing endianess) */
	Serializer& operator<<(networkInterface::MacAddress const& v)
	{
		packBuffer(v.data(), v.size());
		return *this;
	}

	/** Appends a raw buffer to the serialized buffer (without changing endianess) */
	Serializer& packBuffer(void const* const ptr, size_t const size)
	{
		// Check enough room in buffer
		if (remaining() < size)
		{
			throw std::invalid_argument("Not enough room to serialize");
		}

		// Copy data to buffer
		std::memcpy(_buffer.data() + _pos, ptr, size);

		// Advance data pointer
		_pos += size;

		return *this;
	}

	size_t remaining() const
	{
		return MaximumSize - _pos;
	}

	size_t usedBytes() const
	{
		return _pos;
	}

	size_t capacity() const
	{
		return MaximumSize;
	}

private:
	std::array<std::uint8_t, MaximumSize> _buffer{};
	size_t _pos{ 0u };
};


/* DESERIALIZATION */
class Deserializer
{
public:
	Deserializer(void const* const ptr, size_t const size)
		: _ptr(ptr)
		, _size(size)
	{
	}

	/** Unpacks any arithmetic (including enums) */
	template<typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value || std::is_enum<T>::value>>
	Deserializer& operator>>(T& v)
	{
		// Check enough remaining data in buffer
		if (remaining() < sizeof(v))
		{
			throw std::invalid_argument("Not enough data to deserialize");
		}

		// Read value
		auto const* const ptr = static_cast<std::uint8_t const*>(_ptr) + _pos;
		auto const val = *reinterpret_cast<T const*>(ptr);
		v = AVDECC_UNPACK_TYPE(val, T);

		// Advance data pointer
		_pos += sizeof(v);

		return *this;
	}

	/** Unpacks any TypedDefine type */
	template<typename T>
	Deserializer& operator>>(TypedDefine<T>& v)
	{
		// Check enough remaining data in buffer
		if (remaining() < sizeof(v))
		{
			throw std::invalid_argument("Not enough data to deserialize");
		}

		// Read value
		auto const* const ptr = static_cast<std::uint8_t const*>(_ptr) + _pos;
		auto const val = *reinterpret_cast<T const*>(ptr);
		v.setValue(AVDECC_UNPACK_TYPE(val, T));

		// Advance data pointer
		_pos += sizeof(v);

		return *this;
	}

	/** Unpacks a UniqueIdentifier */
	Deserializer& operator>>(UniqueIdentifier& v)
	{
		UniqueIdentifier::value_type value;
		operator>>(value);
		v.setValue(value);
		return *this;
	}

	/** Unpacks an AvdeccFixedString (without changing endianess) */
	Deserializer& operator>>(entity::model::AvdeccFixedString& v)
	{
		unpackBuffer(v.data(), v.size());
		return *this;
	}

	/** Unpacks a MacAddress (without changing endianess) */
	Deserializer& operator>>(networkInterface::MacAddress& v)
	{
		unpackBuffer(v.data(), v.size());
		return *this;
	}

	/** Unpacks data to a raw buffer (without changing endianess) */
	void unpackBuffer(void* const buffer, size_t const size)
	{
		// Check enough remaining data in buffer
		if (remaining() < size)
		{
			throw std::invalid_argument("Not enough data to deserialize");
		}

		std::memcpy(buffer, static_cast<std::uint8_t const*>(_ptr) + _pos, size);

		// Advance data pointer
		_pos += size;
	}

	size_t remaining() const
	{
		return _size - _pos;
	}

	size_t usedBytes() const
	{
		return _pos;
	}

	void setPosition(const size_t position)
	{
		if (position > _size)
			throw std::invalid_argument("Trying to setPosition more bytes than available");
		_pos = position;
	}

	void const* currentData() const noexcept
	{
		return static_cast<std::uint8_t const*>(_ptr) + _pos;
	}

private:
	size_t _pos{ 0 };
	void const* _ptr{ nullptr };
	size_t _size{ 0 };
};

} // namespace avdecc
} // namespace la
