/*
* Copyright (C) 2016-2023, L-Acoustics and its contributors

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
* @file protocolAemAecpdu.hpp
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
/** AEM Aecpdu message */
class AemAecpdu final : public Aecpdu
{
public:
	static constexpr size_t HeaderLength = 2; /* Unsolicited + CommandType */
	static constexpr size_t MaximumPayloadLength_17221 = Aecpdu::MaximumLength_1722_1 - Aecpdu::HeaderLength - HeaderLength;
	static constexpr size_t MaximumPayloadBufferLength = Aecpdu::MaximumLength_BigPayloads - Aecpdu::HeaderLength - HeaderLength;
	static constexpr size_t MaximumSendPayloadBufferLength = Aecpdu::MaximumSendLength - Aecpdu::HeaderLength - HeaderLength;
	static constexpr size_t MaximumRecvPayloadBufferLength = Aecpdu::MaximumRecvLength - Aecpdu::HeaderLength - HeaderLength;
	static_assert(MaximumPayloadBufferLength >= MaximumSendPayloadBufferLength && MaximumPayloadBufferLength >= MaximumRecvPayloadBufferLength, "Incoherent constexpr values");
	static LA_AVDECC_API networkInterface::MacAddress const Identify_Mac_Address; /* IEEE1722.1-2013 Annex B */
	static LA_AVDECC_API UniqueIdentifier const Identify_ControllerEntityID; /* IEEE1722.1-2013 Clause 7.5.1 */
	using Payload = std::pair<void const*, size_t>;

	/**
	* @brief Factory method to create a new AemAecpdu.
	* @details Creates a new AemAecpdu as a unique pointer.
	* @param[in] isResponse True if the AEM message is a response, false if it's a command.
	* @return A new AemAecpdu as a Aecpdu::UniquePointer.
	*/
	static UniquePointer create(bool const isResponse) noexcept
	{
		auto deleter = [](Aecpdu* self)
		{
			static_cast<AemAecpdu*>(self)->destroy();
		};
		return UniquePointer(createRawAemAecpdu(isResponse), deleter);
	}

	/** Constructor for heap AemAecpdu */
	LA_AVDECC_API AemAecpdu(bool const isResponse) noexcept;

	/** Destructor (for some reason we have to define it in the cpp file or clang complains about missing vtable, using = default or inline not working) */
	virtual LA_AVDECC_API ~AemAecpdu() noexcept override;

	// Setters
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION setUnsolicited(bool const unsolicited) noexcept;
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION setCommandType(AemCommandType const commandType) noexcept;
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION setCommandSpecificData(void const* const commandSpecificData, size_t const commandSpecificDataLength);

	// Getters
	LA_AVDECC_API bool LA_AVDECC_CALL_CONVENTION getUnsolicited() const noexcept;
	LA_AVDECC_API AemCommandType LA_AVDECC_CALL_CONVENTION getCommandType() const noexcept;
	LA_AVDECC_API Payload LA_AVDECC_CALL_CONVENTION getPayload() const noexcept;

	/** Serialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION serialize(SerializationBuffer& buffer) const override;

	/** Deserialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION deserialize(DeserializationBuffer& buffer) override;

	/** Contruct a Response message to this Command (only changing the messageType to be of Response kind). Returns nullptr if the message is not a Command or if no Response is possible for this messageType */
	virtual LA_AVDECC_API UniquePointer LA_AVDECC_CALL_CONVENTION responseCopy() const override;

	// Defaulted compiler auto-generated methods
	AemAecpdu(AemAecpdu&&) = default;
	AemAecpdu(AemAecpdu const&) = default;
	AemAecpdu& operator=(AemAecpdu const&) = default;
	AemAecpdu& operator=(AemAecpdu&&) = default;

private:
	/** Entry point */
	static LA_AVDECC_API AemAecpdu* LA_AVDECC_CALL_CONVENTION createRawAemAecpdu(bool const isResponse) noexcept;

	/** Destroy method for COM-like interface */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION destroy() noexcept override;

	// Aem header data
	bool _unsolicited{ false };
	AemCommandType _commandType{ AemCommandType::InvalidCommandType };
	std::array<std::uint8_t, MaximumPayloadBufferLength> _commandSpecificData{};
	size_t _commandSpecificDataLength{ 0u };
};

} // namespace protocol
} // namespace avdecc
} // namespace la
