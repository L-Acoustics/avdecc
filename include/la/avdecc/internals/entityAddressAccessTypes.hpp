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
* @file entityAddressAccessTypes.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model types.
*/

#pragma once

#include <cstdint>
#include <vector>
#include "la/avdecc/utils.hpp"
#include "protocolDefines.hpp"

namespace la
{
namespace avdecc
{
namespace entity
{
namespace addressAccess
{

/** Type-Length-Value for AddressAccess */
class Tlv final
{
public:
	static constexpr size_t MaxLength = pow(2, 12); // Actually a lot less since the TLV must be embedded inside an AECP message
	using value_type = std::uint8_t;
	using memory_data_type = std::vector<value_type>;

	/** Default constructor. */
	//Tlv() noexcept
	//{
	//}

	/** Constructor from a length for a Read mode. */
	Tlv(std::uint64_t const address, size_t const length)
		: _mode(protocol::AaMode::Read), _address(address)
	{
		if (length > MaxLength)
		{
			throw std::invalid_argument("Length too big");
		}
		_memoryData.assign(length, 0u);
	}

	/** Constructor from a length and a mode, allocating the memory data. */
	Tlv(protocol::AaMode const mode, std::uint64_t const address, size_t const length)
		: _mode(mode), _address(address)
	{
		if (length > MaxLength)
		{
			throw std::invalid_argument("Length too big");
		}
		_memoryData.assign(length, 0u);
	}

	/** Constructor from a memory data for a Write or Execute mode. */
	Tlv(std::uint64_t const address, protocol::AaMode const mode, memory_data_type const& memoryData)
		: _mode(mode), _address(address)
	{
		if (memoryData.size() > MaxLength)
		{
			throw std::invalid_argument("Length too big");
		}
		_memoryData = memoryData;
	}

	/** Constructor from a memory data for a Write or Execute mode. */
	Tlv(std::uint64_t const address, protocol::AaMode const mode, memory_data_type&& memoryData)
		: _mode(mode), _address(address)
	{
		if (memoryData.size() > MaxLength)
		{
			throw std::invalid_argument("Length too big");
		}
		_memoryData = std::move(memoryData);
	}

	/** Constructor from a raw buffer for a Write or Execute mode. */
	Tlv(std::uint64_t const address, protocol::AaMode const mode, void const* const ptr, size_t const size)
		: _mode(mode), _address(address)
	{
		if (size > MaxLength)
		{
			throw std::invalid_argument("Length too big");
		}
		value_type const* const copyPtr = reinterpret_cast<value_type const*>(ptr);
		_memoryData.assign(copyPtr, copyPtr + size);
	}

	/** Default destructor. */
	~Tlv() noexcept = default;

	/** Returns the TLV mode. */
	protocol::AaMode getMode() const noexcept
	{
		return _mode;
	}

	/** Returns the memory address. */
	std::uint64_t getAddress() const noexcept
	{
		return _address;
	}

	/** Returns the memory data. */
	memory_data_type const& getMemoryData() const noexcept
	{
		return _memoryData;
	}

	/** Returns the memory data. */
	memory_data_type& getMemoryData() noexcept
	{
		return _memoryData;
	}

	/** Returns the raw memory data. */
	void* data() noexcept
	{
		return _memoryData.data();
	}

	/** Returns the raw memory data (const). */
	void const* data() const noexcept
	{
		return _memoryData.data();
	}

	/** Returns the size of the memory data. */
	size_t size() const noexcept
	{
		return _memoryData.size();
	}

	/** True if the Tlv is valid. */
	constexpr bool isValid() const noexcept
	{
		return _address != 0u && !_memoryData.empty();
	}

	/** operator== */
	bool operator==(Tlv const& lhs) const noexcept
	{
		return _mode == lhs._mode && _address == lhs._address && _memoryData == lhs._memoryData;
	}

	/** operator!= */
	bool operator!=(Tlv const& lhs) const noexcept
	{
		return !operator==(lhs);
	}

	/** Validity operator. */
	explicit constexpr operator bool() const noexcept
	{
		return isValid();
	}

	// Defaulted compiler auto-generated methods
	Tlv(Tlv&&) = default;
	Tlv(Tlv const&) = default;
	Tlv& operator=(Tlv const&) = default;
	Tlv& operator=(Tlv&&) = default;

private:
	protocol::AaMode _mode{ protocol::AaMode::Read };
	std::uint64_t _address{ 0u };
	memory_data_type _memoryData{};
};

using Tlvs = std::vector<Tlv>;

} // namespace addressAccess
} // namespace entity
} // namespace avdecc
} // namespace la
