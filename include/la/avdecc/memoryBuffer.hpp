/*
* Copyright (C) 2016-2020, L-Acoustics and its contributors

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
* @file memoryBuffer.hpp
* @author Christophe Calmejane
* @brief Lightweight, efficient vector-like container.
*/

#pragma once

#include <cstdint> // uint8_t
#include <vector> // vector
#include <string> // string
#include <new> // bad_alloc
#include <cstring> // memmove / memcpy
#include <cstdlib> // free / realloc
#include <algorithm> // min

namespace la
{
namespace avdecc
{
/**
* @brief Lightweight and efficient vector-like container.
* @details A vector-like container that handles a "byte" resizable array
*          which can be resized without forcing data initialization.
*          All the methods of this class have the same meaning and
*          specification than std:vector, with the addition of a
*          @ref set_size "set_size" method to change the "used bytes" size
*          of the array without default initializing it.
*/
class MemoryBuffer
{
public:
	using value_type = std::uint8_t;

	static_assert(sizeof(value_type) == sizeof(std::uint8_t), "MemoryBuffer::value_type should be of 'char' size, or all the 'assign' method don't make sense");

	/* ************************************************************************** */
	/* Life cycle                                                                 */

	/** Default constructor */
	MemoryBuffer() noexcept {}

	/** Contructor from a std::vector */
	template<typename T>
	explicit MemoryBuffer(std::vector<T> const& vec)
		: MemoryBuffer(vec.data(), vec.size() * sizeof(T))
	{
		static_assert(sizeof(T) == sizeof(value_type), "vector::value_type should have the same size than MemoryBuffer::value_type (or it doesn't make sense of what to copy)");
	}

	/** Constructor from a std::string */
	MemoryBuffer(std::string const& str)
		: MemoryBuffer(str.data(), str.size() * sizeof(std::string::value_type))
	{
	}

	/** Constructor from a raw buffer */
	MemoryBuffer(void const* const ptr, size_t const bytes)
	{
		assign(ptr, bytes);
	}

	/** Copy constructor. Note: Resulting capacity might be different than passed buffer */
	MemoryBuffer(MemoryBuffer const& buffer)
	{
		*this = buffer;
	}

	/** Move constructor */
	MemoryBuffer(MemoryBuffer&& buffer) noexcept
		: _data(buffer._data)
		, _capacity(buffer._capacity)
		, _size(buffer._size)
	{
		buffer._data = nullptr;
		buffer._capacity = 0;
		buffer._size = 0;
	}

	/** Copy operator=. Note: Resulting capacity might be different than passed buffer */
	MemoryBuffer& operator=(MemoryBuffer const& buffer)
	{
		if (this != &buffer)
		{
			if (!buffer.empty())
			{
				reserve(buffer.size());
				std::memcpy(_data, buffer.data(), buffer.size());
			}
			set_size(buffer.size()); // Set same size
		}
		return *this;
	}

	/** Move operator= */
	MemoryBuffer& operator=(MemoryBuffer&& buffer) noexcept
	{
		if (this != &buffer)
		{
			// Free previous buffer
			if (_data != nullptr)
			{
				std::free(_data);
			}
			// Move elements to new container
			_data = buffer._data;
			_capacity = buffer._capacity;
			_size = buffer._size;
			// Reset elements from old container
			buffer._data = nullptr;
			buffer._capacity = 0;
			buffer._size = 0;
		}
		return *this;
	}

	/** Desctructor */
	~MemoryBuffer() noexcept
	{
		clear();
		shrink_to_fit(); // We know it can't throw
	}

	/* ************************************************************************** */
	/* Comparison operators                                                       */
	bool operator==(MemoryBuffer const& buffer) const noexcept
	{
		// First check for size, so we don't have to process the whole data
		if (_size != buffer._size)
		{
			return false;
		}

		// Now we can compare the data
		return std::memcmp(_data, buffer._data, _size) == 0;
	}

	bool operator!=(MemoryBuffer const& buffer) const noexcept
	{
		return !operator==(buffer);
	}

	/* ************************************************************************** */
	/* Writters                                                                   */

	/** Replaces the MemoryBuffer with the content of the specified std::string */
	void assign(std::string const& str)
	{
		assign(str.data(), str.size() * sizeof(std::string::value_type));
	}

	/** Replaces the MemoryBuffer with the content of the specified std::vector */
	template<typename T>
	void assign(std::vector<T> const& vec)
	{
		static_assert(sizeof(T) == sizeof(value_type), "vector::value_type should have the same size than MemoryBuffer::value_type (or it doesn't make sense of what to copy)");
		assign(vec.data(), vec.size() * sizeof(T));
	}

	/** Replaces the MemoryBuffer with the content of the specified raw buffer */
	void assign(void const* const ptr, size_t const bytes)
	{
		set_size(bytes);
		std::memcpy(_data, ptr, bytes);
	}

	/** Appends the content of the specified std::string at the end of the MemoryBuffer */
	void append(std::string const& str)
	{
		append(str.data(), str.size() * sizeof(std::string::value_type));
	}

	/** Appends the content of the specified std::vector at the end of the MemoryBuffer */
	template<typename T>
	void append(std::vector<T> const& vec)
	{
		append(vec.data(), vec.size() * sizeof(T));
	}

	/** Appends the content of the specified simple type (arithmetic or enum) at the end of the MemoryBuffer */
	template<typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value || std::is_enum<T>::value>>
	void append(T const& v)
	{
		append(&v, sizeof(v));
	}

	/** Appends the content of the specified raw buffer at the end of the MemoryBuffer */
	void append(void const* const ptr, size_t const bytes)
	{
		auto offset = _size;
		set_size(_size + bytes);
		std::memcpy(_data + offset, ptr, bytes);
	}

	/* ************************************************************************** */
	/* Data access                                                                */

	/** Returns the raw data */
	value_type* data() noexcept
	{
		return _data;
	}

	/** Returns the raw const data */
	value_type const* data() const noexcept
	{
		return _data;
	}

	/* ************************************************************************** */
	/* Capacity getters                                                           */

	/** Gets the current count of valid elements in the buffer */
	constexpr size_t size() const noexcept
	{
		return _size;
	}

	/** True if the buffer contains no element */
	constexpr bool empty() const noexcept
	{
		return _size == 0;
	}

	/** Gets the current allocated buffer size (superior or equal to the value returned by size()) */
	constexpr size_t capacity() const noexcept
	{
		return _capacity;
	}


	/* ************************************************************************** */
	/* Capacity modifiers                                                         */

	/** Increase (if needed) the current allocated buffer size, without changing the count of valid elements in the buffer */
	void reserve(size_t const new_cap)
	{
		if (new_cap > _capacity)
		{
			auto new_ptr = std::realloc(_data, new_cap);
			if (new_ptr == nullptr)
			{
				throw std::bad_alloc();
			}
			_data = static_cast<value_type*>(new_ptr);
			_capacity = new_cap;
		}
	}

	/** Shrinks the allocated buffer to best fit the count of valid elements in the buffer so that the value returned by capacity() equals the value returned by size() */
	void shrink_to_fit()
	{
		if (_data != nullptr && _size != _capacity)
		{
			if (_size == 0)
			{
				std::free(_data);
				_data = nullptr;
				_capacity = 0;
			}
			else
			{
				auto new_ptr = std::realloc(_data, _size);
				if (new_ptr == nullptr)
				{
					throw std::bad_alloc();
				}
				_data = static_cast<value_type*>(new_ptr);
				_capacity = _size;
			}
		}
	}

	/** Removes all the valid elements in the buffer, without deallocating the buffer */
	void clear() noexcept
	{
		set_size(0u);
	}

	/**
	* @brief Changes the used size of the buffer, possibly reallocating it.
	* @details Sets the number of used bytes of the buffer to the specified value,
	*          changing its capacity if the new size if greater than the current
	*          buffer capacity. Newly accessible bytes are <b>not initialized</b>.
	* @param[in] used_size The new used bytes value.
	* @note After using this method, the #size() method will return what was
	*       specified during this call, contrary to #capacity() which returns the
	*       allocated buffer size (usable bytes vs allocated bytes).<BR>
	*       One can allocates a buffer using #reserve(size_t const new_cap) then
	*       pass #data() and #capacity() to a method accepting ptr+size parameters.
	*       Then upon return of such a method, if it returns the number of copied
	*       bytes, call #set_size(size_t const used_size) to defined how many valid
	*       usable bytes are in the buffer.
	*/
	void set_size(size_t const used_size)
	{
		reserve(used_size);
		_size = used_size;
	}

	/**
	* @brief Removes bytes from the begining of the buffer, shifting the remaining.
	* @details Removes the specified amount from the begining of the buffer, then
	*          shifts the remaining bytes to the start of it.
	* @param[in] consumed_size The bytes to consume.
	* @note After using this method, the #size() method will return the count of
	*       remaining usable bytes in the buffer.
	*/
	void consume_size(size_t const consumed_size) noexcept
	{
		if (_data != nullptr && _size != 0 && consumed_size != 0)
		{
			auto to_consume = std::min(consumed_size, _size);
			auto remaining = _size - to_consume;
			if (remaining > 0) // Remaining data in the buffer that we need to move
			{
				auto* const src_ptr = static_cast<value_type const*>(static_cast<std::uint8_t*>(_data) + consumed_size);
				std::memmove(_data, src_ptr, remaining);
			}
			_size -= to_consume;
		}
	}

private:
	value_type* _data{ nullptr };
	size_t _capacity{ 0u };
	size_t _size{ 0u };
};

} // namespace avdecc
} // namespace la
