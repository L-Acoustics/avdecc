/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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
	static constexpr size_t ProtocolIdentifierSize = 6;
	static constexpr size_t HeaderLength = ProtocolIdentifierSize; /* ProtocolID */
	using ProtocolIdentifier = std::array<std::uint8_t, ProtocolIdentifierSize>;
	using Payload = std::pair<void const*, size_t>;

	// Setters
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION setProtocolIdentifier(ProtocolIdentifier const& protocolIdentifier) noexcept;

	// Getters
	LA_AVDECC_API ProtocolIdentifier LA_AVDECC_CALL_CONVENTION getProtocolIdentifier() const noexcept;

	/** Serialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION serialize(SerializationBuffer& buffer) const override;

	/** Deserialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION deserialize(DeserializationBuffer& buffer) override;

	// Defaulted compiler auto-generated methods
	VuAecpdu(VuAecpdu&&) = default;
	VuAecpdu(VuAecpdu const&) = default;
	VuAecpdu& operator=(VuAecpdu const&) = default;
	VuAecpdu& operator=(VuAecpdu&&) = default;

protected:
	/** Constructor */
	LA_AVDECC_API VuAecpdu() noexcept;

	/** Destructor */
	virtual ~VuAecpdu() noexcept override = default;

private:
	// VuAecpdu header data
	ProtocolIdentifier _protocolIdentifier{};
};

} // namespace protocol
} // namespace avdecc
} // namespace la
