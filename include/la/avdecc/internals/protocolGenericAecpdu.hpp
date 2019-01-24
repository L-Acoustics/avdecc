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
* @file protocolGenericAecpdu.hpp
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
/** Generic Aecpdu message */
class GenericAecpdu final : public Aecpdu
{
public:
	static constexpr size_t HeaderLength = 0; /* Nothing */
	static constexpr size_t MaximumPayloadLength_17221 = Aecpdu::MaximumLength_1722_1 - Aecpdu::HeaderLength - HeaderLength;
	static constexpr size_t MaximumPayloadBufferLength = Aecpdu::MaximumLength_BigPayloads - Aecpdu::HeaderLength - HeaderLength;
	static constexpr size_t MaximumSendPayloadBufferLength = Aecpdu::MaximumSendLength - Aecpdu::HeaderLength - HeaderLength;
	static constexpr size_t MaximumRecvPayloadBufferLength = Aecpdu::MaximumRecvLength - Aecpdu::HeaderLength - HeaderLength;
	static_assert(MaximumPayloadBufferLength >= MaximumSendPayloadBufferLength && MaximumPayloadBufferLength >= MaximumRecvPayloadBufferLength, "Incoherent constexpr values");
	using Payload = std::pair<void const*, size_t>;

	/**
	* @brief Factory method to create a new GenericAecpdu.
	* @details Creates a new GenericAecpdu as a unique pointer.
	* @return A new GenericAecpdu as a Aecpdu::UniquePointer.
	*/
	static UniquePointer create() noexcept
	{
		auto deleter = [](Aecpdu* self)
		{
			static_cast<GenericAecpdu*>(self)->destroy();
		};
		return UniquePointer(createRawGenericAecpdu(), deleter);
	}

	/** Constructor for heap GenericAecpdu */
	LA_AVDECC_API GenericAecpdu() noexcept;

	/** Destructor (for some reason we have to define it in the cpp file or clang complains about missing vtable, using = default or inline not working) */
	virtual LA_AVDECC_API ~GenericAecpdu() noexcept override;

	// Setters
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION setPayload(void const* const payload, size_t const payloadLength);

	// Getters
	LA_AVDECC_API Payload LA_AVDECC_CALL_CONVENTION getPayload() const noexcept;

	/** Serialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION serialize(SerializationBuffer& buffer) const override;

	/** Deserialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION deserialize(DeserializationBuffer& buffer) override;

	/** Copy method */
	virtual LA_AVDECC_API UniquePointer LA_AVDECC_CALL_CONVENTION copy() const override;

	// Defaulted compiler auto-generated methods
	GenericAecpdu(GenericAecpdu&&) = default;
	GenericAecpdu(GenericAecpdu const&) = default;
	GenericAecpdu& operator=(GenericAecpdu const&) = default;
	GenericAecpdu& operator=(GenericAecpdu&&) = default;

private:
	/** Entry point */
	static LA_AVDECC_API GenericAecpdu* LA_AVDECC_CALL_CONVENTION createRawGenericAecpdu() noexcept;

	/** Destroy method for COM-like interface */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION destroy() noexcept override;

	// Generic header data
	std::array<std::uint8_t, MaximumPayloadBufferLength> _payload{};
	size_t _payloadLength{ 0u };
};

} // namespace protocol
} // namespace avdecc
} // namespace la
