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
* @file protocolVuAecpdu.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "protocolAecpdu.hpp"

#include <utility>
#include <array>
#include <cstring> // memcpy

namespace la
{
namespace avdecc
{
namespace protocol
{
/** Vendor Unique Aecpdu message */
class VuAecpdu : public Aecpdu
{
public:
	class ProtocolIdentifier
	{
	public:
		static constexpr size_t Size = 6;
		using IntegralType = std::uint64_t;
		using ArrayType = std::array<std::uint8_t, Size>;

		ProtocolIdentifier() noexcept {}

		/** Initializes the ProtocolIdentifier from a 64 bits value (only the LSB 48 bits are used) */
		explicit ProtocolIdentifier(IntegralType const identifier) noexcept
		{
			setValue(identifier);
		}

		/** Initializes the ProtocolIdentifier from an array of 6 bytes (Identifier will be represented as 0x00[0][1][2][3][4][5]) */
		explicit ProtocolIdentifier(ArrayType const& identifier) noexcept
		{
			setValue(identifier);
		}

		/** Sets the ProtocolIdentifier from a 64 bits value (only the LSB 48 bits are used) */
		void setValue(IntegralType const identifier) noexcept
		{
			_identifier = identifier & 0x00FFFFFFFFFFFF;
		}

		/** Sets the ProtocolIdentifier from an array of 6 bytes (Identifier will be represented as 0x00[0][1][2][3][4][5]) */
		void setValue(ArrayType const& identifier) noexcept
		{
			_identifier = 0ull;

			for (auto const v : identifier)
			{
				_identifier = (_identifier << 8) | v;
			}
		}

		/** Returns the ProtocolIdentifier as a 64 bits value (only the LSB 48 bits are valid, the 16 MSB are set to 0) */
		explicit constexpr operator IntegralType() const noexcept
		{
			return _identifier;
		}

		/** Returns the ProtocolIdentifier as an array of 6 bytes (Identifier will be represented as 0x00[0][1][2][3][4][5]) */
		explicit operator ArrayType() const noexcept
		{
			ArrayType value{};

			for (auto i = 0u; i < Size; ++i)
			{
				value[i] = static_cast<ArrayType::value_type>((_identifier >> ((Size - 1 - i) * 8)) & 0x00000000000000FF);
			}

			return value;
		}

		/** Comparison operator between 2 ProtocolIdentifiers */
		constexpr friend bool operator==(ProtocolIdentifier const& lhs, ProtocolIdentifier const& rhs) noexcept
		{
			return lhs._identifier == rhs._identifier;
		}

		/** Comparison operator between a ProtocolIdentifier and a 64 bits value */
		constexpr friend bool operator==(ProtocolIdentifier const& lhs, IntegralType const& rhs) noexcept
		{
			return lhs._identifier == rhs;
		}

		/** Comparison operator between a ProtocolIdentifier and an array of 6 bytes */
		friend bool operator==(ProtocolIdentifier const& lhs, ArrayType const& rhs) noexcept
		{
			auto const pi = ProtocolIdentifier{ rhs };
			return lhs._identifier == pi._identifier;
		}

		/** Hash functor to be used for std::hash */
		struct hash
		{
			std::size_t operator()(ProtocolIdentifier const& identifier) const
			{
				return std::hash<IntegralType>()(identifier._identifier);
			}
		};

		// Defaulted compiler auto-generated methods
		~ProtocolIdentifier() noexcept = default;
		ProtocolIdentifier(ProtocolIdentifier const&) = default;
		ProtocolIdentifier(ProtocolIdentifier&&) = default;
		ProtocolIdentifier& operator=(ProtocolIdentifier const&) = default;
		ProtocolIdentifier& operator=(ProtocolIdentifier&&) = default;

	private:
		IntegralType _identifier{ 0ull };
	};

	static constexpr size_t HeaderLength = ProtocolIdentifier::Size; /* ProtocolID */
	using Payload = std::pair<void const*, size_t>;

	// Setters
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION setProtocolIdentifier(ProtocolIdentifier const& protocolIdentifier) noexcept;

	// Getters
	LA_AVDECC_API ProtocolIdentifier LA_AVDECC_CALL_CONVENTION getProtocolIdentifier() const noexcept;

	/** Serialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION serialize(SerializationBuffer& buffer) const override;

	/** Deserialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION deserialize(DeserializationBuffer& buffer) override;

protected:
	/** Constructor */
	VuAecpdu(bool const isResponse) noexcept;

	/** Destructor */
	virtual ~VuAecpdu() noexcept override = default;

	// Defaulted compiler auto-generated methods
	VuAecpdu(VuAecpdu&&) = default;
	VuAecpdu(VuAecpdu const&) = default;
	VuAecpdu& operator=(VuAecpdu const&) = default;
	VuAecpdu& operator=(VuAecpdu&&) = default;

private:
	// VuAecpdu header data
	ProtocolIdentifier _protocolIdentifier{};
};

} // namespace protocol
} // namespace avdecc
} // namespace la
