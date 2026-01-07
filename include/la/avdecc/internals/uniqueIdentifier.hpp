/*
* Copyright (C) 2016-2026, L-Acoustics and its contributors

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
* @file uniqueIdentifier.hpp
* @author Christophe Calmejane
* @brief UniqueIdentifier definition and helper methods.
* @note http://standards.ieee.org/develop/regauth/tut/eui.pdf
*/

#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include <functional>

namespace la
{
namespace avdecc
{
class UniqueIdentifier final
{
public:
	using value_type = std::uint64_t;

	/** Default constructor. */
	constexpr UniqueIdentifier() noexcept
		: _eui(0xFFFFFFFFFFFFFFFF)
	{
	}

	/** Constructor to create a UniqueIdentifier from the underlying value. */
	explicit constexpr UniqueIdentifier(value_type const eui) noexcept
		: _eui(eui)
	{
	}

	/** Setter to change the underlying value. */
	constexpr void setValue(value_type const eui) noexcept
	{
		_eui = eui;
	}

	/** Getter to retrieve the underlying value. */
	constexpr value_type getValue() const noexcept
	{
		return _eui;
	}

	/** Returns the VendorID as a OUI-24 (by default) or OUI-64 if using std::uint64_t type. It's the caller's responsibility to know if it needs to get the OUI-24 or OUI-36. */
	template<typename Type = std::uint32_t>
	constexpr std::enable_if_t<std::is_same_v<Type, std::uint32_t> | std::is_same_v<Type, std::uint64_t>, Type> getVendorID() const noexcept
	{
		if constexpr (std::is_same_v<Type, std::uint32_t>)
		{
			return static_cast<Type>((_eui >> 40) & 0x0000000000FFFFFF);
		}
		else if constexpr (std::is_same_v<Type, std::uint64_t>)
		{
			return static_cast<Type>((_eui >> 28) & 0x0000000FFFFFFFFF);
		}
	}

	/** Returns the Value for the Vendor. Value being the remaining part after the OUI-24 (by default) or OUI-64 if using std::uint64_t type. It's the caller's responsibility to know if it needs to get the value after OUI-24 or OUI-36. */
	template<typename Type = std::uint64_t>
	constexpr std::enable_if_t<std::is_same_v<Type, std::uint64_t> | std::is_same_v<Type, std::uint32_t>, Type> getVendorValue() const noexcept
	{
		if constexpr (std::is_same_v<Type, std::uint64_t>)
		{
			return static_cast<Type>(_eui) & 0x000000FFFFFFFFFF;
		}
		else if constexpr (std::is_same_v<Type, std::uint32_t>)
		{
			return static_cast<Type>(_eui) & 0x0FFFFFFF;
		}
	}

	/** Returns true if the UniqueIdentifier is Group (aka Multicast/Broadcast). Returns false if the UniqueIdentifier is Individual (aka Unicast), or invalid. */
	constexpr bool isGroupIdentifier() const noexcept
	{
		return isValid() && ((_eui & 0x0100000000000000) == 0x0100000000000000);
	}

	/** Returns true if the UniqueIdentifier is Locally Administrated. Returns false if the UniqueIdentifier is Universally Administered, or invalid. */
	constexpr bool isLocalIdentifier() const noexcept
	{
		return isValid() && ((_eui & 0x0200000000000000) == 0x0200000000000000);
	}

	/** True if the UniqueIdentifier contains a valid underlying value, false otherwise. */
	constexpr bool isValid() const noexcept
	{
		return _eui != NullIdentifierValue && _eui != UninitializedIdentifierValue;
	}

	/** Underlying value operator (equivalent to getValue()). */
	constexpr operator value_type() const noexcept
	{
		return getValue();
	}

	/** Underlying value validity bool operator (equivalent to isValid()). */
	explicit constexpr operator bool() const noexcept
	{
		return isValid();
	}

	/** Equality operator. Returns true if the underlying values are equal (Null and Uninitialized values are considered equal, since they both are invalid). */
	constexpr friend bool operator==(UniqueIdentifier const& lhs, UniqueIdentifier const& rhs) noexcept
	{
		return (!lhs.isValid() && !rhs.isValid()) || lhs._eui == rhs._eui;
	}

	/** Non equality operator. */
	constexpr friend bool operator!=(UniqueIdentifier const& lhs, UniqueIdentifier const& rhs) noexcept
	{
		return !operator==(lhs, rhs);
	}

	/** operator< */
	constexpr friend bool operator<(UniqueIdentifier const& lhs, UniqueIdentifier const& rhs) noexcept
	{
		return lhs._eui < rhs._eui;
	}

	/** Static helper method to create a Null UniqueIdentifier (isValid() returns false). */
	static UniqueIdentifier getNullUniqueIdentifier() noexcept
	{
		return UniqueIdentifier{ NullIdentifierValue };
	}

	/** Static helper method to create an Uninitialized UniqueIdentifier (isValid() returns false). */
	static UniqueIdentifier getUninitializedUniqueIdentifier() noexcept
	{
		return UniqueIdentifier{ UninitializedIdentifierValue };
	}

	/** Hash functor to be used for std::hash */
	struct hash
	{
		std::size_t operator()(UniqueIdentifier const& uid) const
		{
			return std::hash<value_type>()(uid._eui);
		}
	};

	// Defaulted compiler auto-generated methods
	UniqueIdentifier(UniqueIdentifier&&) = default;
	UniqueIdentifier(UniqueIdentifier const&) = default;
	UniqueIdentifier& operator=(UniqueIdentifier const&) = default;
	UniqueIdentifier& operator=(UniqueIdentifier&&) = default;

private:
	static constexpr value_type NullIdentifierValue = 0x0000000000000000;
	static constexpr value_type UninitializedIdentifierValue = 0xFFFFFFFFFFFFFFFF;
	value_type _eui{ UninitializedIdentifierValue };
};

} // namespace avdecc
} // namespace la
