/*
* Copyright (C) 2016-2022, L-Acoustics and its contributors

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
* @file serialization.hpp
* @author Christophe Calmejane
* @brief Simple buffer serializer/deserializer.
*/

#pragma once

#include "la/avdecc/memoryBuffer.hpp"
#include "la/avdecc/utils.hpp"

#include "endian.hpp"
#include "entityModel.hpp"

#include <la/networkInterfaceHelper/networkInterfaceHelper.hpp>

#include <cstdint>
#include <type_traits>
#include <stdexcept> // invalid_argument
#include <array>
#include <cstring> // memcpy

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
	template<class Typed, typename T = typename Typed::value_type>
	Serializer& operator<<(utils::TypedDefine<Typed, T> const& v)
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

	/** Serializes any EnumBitfield type */
	template<class Bitfield, typename T = typename std::underlying_type_t<Bitfield>>
	Serializer& operator<<(utils::EnumBitfield<Bitfield> const& v)
	{
		// Check enough room in buffer
		if (remaining() < sizeof(T))
		{
			throw std::invalid_argument("Not enough room to serialize");
		}

		// Copy value to buffer
		T* const ptr = reinterpret_cast<T*>(_buffer.data() + _pos);
		*ptr = AVDECC_PACK_TYPE(v.value(), T);

		// Advance data pointer
		_pos += sizeof(v);

		return *this;
	}

	/** Serializes a UniqueIdentifier */
	Serializer& operator<<(UniqueIdentifier const& v)
	{
		return operator<<(v.getValue());
	}

	/** Serializes a SamplingRate */
	Serializer& operator<<(entity::model::SamplingRate const& v)
	{
		return operator<<(v.getValue());
	}

	/** Serializes a StreamFormat */
	Serializer& operator<<(entity::model::StreamFormat const& v)
	{
		return operator<<(v.getValue());
	}

	/** Serializes a ControlValueUnit */
	Serializer& operator<<(entity::model::ControlValueUnit const& v)
	{
		return operator<<(v.getValue());
	}

	/** Serializes a ControlValueType */
	Serializer& operator<<(entity::model::ControlValueType const& v)
	{
		return operator<<(v.getValue());
	}

	/** Serializes a LocalizedStringReference */
	Serializer& operator<<(entity::model::LocalizedStringReference const& v)
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

	/** Serializes a MemoryBuffer (without changing endianess) */
	Serializer& operator<<(MemoryBuffer const& v)
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
	Deserializer(void const* const ptr, size_t const size) noexcept
		: _ptr{ ptr }
		, _size{ size }
	{
	}

	Deserializer(MemoryBuffer const& buffer) noexcept
		: _ptr{ buffer.data() }
		, _size{ buffer.size() }
	{
	}

	/** Gets raw pointer to deserialization buffer */
	void const* data() const
	{
		return _ptr;
	}

	/** Gets size of deserialization buffer */
	size_t size() const
	{
		return _size;
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
	template<class Typed, typename T = typename Typed::value_type>
	Deserializer& operator>>(utils::TypedDefine<Typed, T>& v)
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

	/** Unpacks any EnumBitfield type */
	template<class Bitfield, typename T = typename std::underlying_type_t<Bitfield>>
	Deserializer& operator>>(utils::EnumBitfield<Bitfield>& v)
	{
		// Check enough remaining data in buffer
		if (remaining() < sizeof(T))
		{
			throw std::invalid_argument("Not enough data to deserialize");
		}

		// Read value
		auto const* const ptr = static_cast<std::uint8_t const*>(_ptr) + _pos;
		auto const val = *reinterpret_cast<T const*>(ptr);
		v.assign(AVDECC_UNPACK_TYPE(val, T));

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

	/** Unpacks a SamplingRate */
	Deserializer& operator>>(entity::model::SamplingRate& v)
	{
		entity::model::SamplingRate::value_type value;
		operator>>(value);
		v.setValue(value);
		return *this;
	}

	/** Unpacks a StreamFormat */
	Deserializer& operator>>(entity::model::StreamFormat& v)
	{
		entity::model::StreamFormat::value_type value;
		operator>>(value);
		v.setValue(value);
		return *this;
	}

	/** Unpacks a ControlValueUnit */
	Deserializer& operator>>(entity::model::ControlValueUnit& v)
	{
		entity::model::ControlValueUnit::value_type value;
		operator>>(value);
		v.setValue(value);
		return *this;
	}

	/** Unpacks a ControlValueType */
	Deserializer& operator>>(entity::model::ControlValueType& v)
	{
		entity::model::ControlValueType::value_type value;
		operator>>(value);
		v.setValue(value);
		return *this;
	}

	/** Unpacks a LocalizedStringReference */
	Deserializer& operator>>(entity::model::LocalizedStringReference& v)
	{
		entity::model::LocalizedStringReference::value_type value;
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

	/** Unpacks a MemoryBuffer (without changing endianess) */
	Deserializer& operator>>(MemoryBuffer& v)
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

	bool operator==(Deserializer const& other) const
	{
		return _ptr == other._ptr && _pos == other._pos && _size == other._size;
	}
	bool operator!=(Deserializer const& other) const
	{
		return !operator==(other);
	}

private:
	size_t _pos{ 0 };
	void const* _ptr{ nullptr };
	size_t _size{ 0 };
};

} // namespace avdecc
} // namespace la
