/*
* Copyright (C) 2016-2025, L-Acoustics and its contributors

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
* @file protocolMvuAecpdu.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "protocolVuAecpdu.hpp"

#include <utility>
#include <array>
#include <cstring> // memcpy

namespace la
{
namespace avdecc
{
namespace protocol
{
/** MVU Aecpdu message */
class MvuAecpdu final : public VuAecpdu
{
public:
	static constexpr size_t HeaderLength = 2; /* Unsolicited + CommandType */
	static constexpr size_t MaximumPayloadLength_17221 = Aecpdu::MaximumLength_1722_1 - Aecpdu::HeaderLength - VuAecpdu::HeaderLength - HeaderLength;
	static constexpr size_t MaximumPayloadBufferLength = Aecpdu::MaximumLength_BigPayloads - Aecpdu::HeaderLength - VuAecpdu::HeaderLength - HeaderLength;
	static constexpr size_t MaximumSendPayloadBufferLength = Aecpdu::MaximumSendLength - Aecpdu::HeaderLength - VuAecpdu::HeaderLength - HeaderLength;
	static constexpr size_t MaximumRecvPayloadBufferLength = Aecpdu::MaximumRecvLength - Aecpdu::HeaderLength - VuAecpdu::HeaderLength - HeaderLength;
	static_assert(MaximumPayloadBufferLength >= MaximumSendPayloadBufferLength && MaximumPayloadBufferLength >= MaximumRecvPayloadBufferLength, "Incoherent constexpr values");
	static LA_AVDECC_API ProtocolIdentifier ProtocolID;

	/**
	* @brief Factory method to create a new MvuAecpdu.
	* @details Creates a new MvuAecpdu as a unique pointer.
	* @param[in] isResponse True if the MVU message is a response, false if it's a command.
	* @return A new MvuAecpdu as a Aecpdu::UniquePointer.
	*/
	static UniquePointer create(bool const isResponse) noexcept
	{
		auto deleter = [](Aecpdu* self)
		{
			static_cast<MvuAecpdu*>(self)->destroy();
		};
		return UniquePointer(createRawMvuAecpdu(isResponse), deleter);
	}

	/** Constructor for heap MvuAecpdu */
	LA_AVDECC_API MvuAecpdu(bool const isResponse) noexcept;

	/** Destructor (for some reason we have to define it in the cpp file or clang complains about missing vtable, using = default or inline not working) */
	virtual LA_AVDECC_API ~MvuAecpdu() noexcept override;

	// Setters
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION setUnsolicited(bool const unsolicited) noexcept;
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION setCommandType(MvuCommandType const commandType) noexcept;
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION setCommandSpecificData(void const* const commandSpecificData, size_t const commandSpecificDataLength);

	// Getters
	LA_AVDECC_API bool LA_AVDECC_CALL_CONVENTION getUnsolicited() const noexcept;
	LA_AVDECC_API MvuCommandType LA_AVDECC_CALL_CONVENTION getCommandType() const noexcept;
	LA_AVDECC_API Payload LA_AVDECC_CALL_CONVENTION getPayload() const noexcept;

	/** Serialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION serialize(SerializationBuffer& buffer) const override;

	/** Deserialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION deserialize(DeserializationBuffer& buffer) override;

	/** Contruct a Response message to this Command (only changing the messageType to be of Response kind). Returns nullptr if the message is not a Command or if no Response is possible for this messageType */
	virtual LA_AVDECC_API UniquePointer LA_AVDECC_CALL_CONVENTION responseCopy() const override;

	// Defaulted compiler auto-generated methods
	LA_AVDECC_API MvuAecpdu(MvuAecpdu&&);
	LA_AVDECC_API MvuAecpdu(MvuAecpdu const&);
	LA_AVDECC_API MvuAecpdu& LA_AVDECC_CALL_CONVENTION operator=(MvuAecpdu const&);
	LA_AVDECC_API MvuAecpdu& LA_AVDECC_CALL_CONVENTION operator=(MvuAecpdu&&);

private:
	/** Entry point */
	static LA_AVDECC_API MvuAecpdu* LA_AVDECC_CALL_CONVENTION createRawMvuAecpdu(bool const isResponse) noexcept;

	/** Destroy method for COM-like interface */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION destroy() noexcept override;

	// Mvu header data
	bool _unsolicited{ false };
	MvuCommandType _commandType{ MvuCommandType::InvalidCommandType };
	std::array<std::uint8_t, MaximumPayloadBufferLength> _commandSpecificData{};
	size_t _commandSpecificDataLength{ 0u };
};

} // namespace protocol
} // namespace avdecc
} // namespace la
