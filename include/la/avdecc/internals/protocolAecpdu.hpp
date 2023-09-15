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
* @file protocolAecpdu.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "protocolAvtpdu.hpp"

#include <memory>

namespace la
{
namespace avdecc
{
namespace protocol
{
/** Aecpdu common header */
class Aecpdu : public AvtpduControl
{
public:
	static constexpr size_t HeaderLength = 10; /* ControllerEID + SequenceID */
	static constexpr size_t MaximumLength_1722_1 = 524; /* AECPDU maximum size - IEEE1722.1-2013 Clause 9.2.1.1.7 */
#if defined(ALLOW_SEND_BIG_AECP_PAYLOADS) || defined(ALLOW_RECV_BIG_AECP_PAYLOADS) // Memory optimization, only set MaximumLength_BigPayloads to a greater value if either option is enabled
	static constexpr size_t MaximumLength_BigPayloads = EthernetMaxFrameSize - EtherLayer2::HeaderLength - AvtpduControl::HeaderLength; /* Extended size, up to max Ethernet frame size (minus headers) */
#else
	static constexpr size_t MaximumLength_BigPayloads = MaximumLength_1722_1; /* Use same value as 1722.1 to optimize memory footprint */
#endif
#if defined(ALLOW_SEND_BIG_AECP_PAYLOADS)
	static constexpr size_t MaximumSendLength = MaximumLength_BigPayloads;
#else // !ALLOW_SEND_BIG_AECP_PAYLOADS
	static constexpr size_t MaximumSendLength = MaximumLength_1722_1;
#endif // ALLOW_SEND_BIG_AECP_PAYLOADS
#if defined(ALLOW_RECV_BIG_AECP_PAYLOADS)
	static constexpr size_t MaximumRecvLength = MaximumLength_BigPayloads;
#else // !ALLOW_RECV_BIG_AECP_PAYLOADS
	static constexpr size_t MaximumRecvLength = MaximumLength_1722_1;
#endif // ALLOW_RECV_BIG_AECP_PAYLOADS
	using UniquePointer = std::unique_ptr<Aecpdu, void (*)(Aecpdu*)>;

	// Setters
	void LA_AVDECC_CALL_CONVENTION setStatus(AecpStatus const status) noexcept
	{
		AvtpduControl::setStatus(status.getValue());
	}
	void LA_AVDECC_CALL_CONVENTION setTargetEntityID(UniqueIdentifier const targetEntityID) noexcept
	{
		AvtpduControl::setStreamID(targetEntityID.getValue());
	}
	void LA_AVDECC_CALL_CONVENTION setControllerEntityID(UniqueIdentifier const controllerEntityID) noexcept
	{
		_controllerEntityID = controllerEntityID;
	}
	void LA_AVDECC_CALL_CONVENTION setSequenceID(AecpSequenceID const sequenceID) noexcept
	{
		_sequenceID = sequenceID;
	}
	void LA_AVDECC_CALL_CONVENTION setAecpSpecificDataLength(size_t const commandSpecificDataLength) noexcept
	{
		auto controlDataLength = static_cast<std::uint16_t>(Aecpdu::HeaderLength + commandSpecificDataLength);
		AvtpduControl::setControlDataLength(controlDataLength);
	}

	// Getters
	AecpMessageType LA_AVDECC_CALL_CONVENTION getMessageType() const noexcept
	{
		return AecpMessageType(AvtpduControl::getControlData());
	}
	AecpStatus LA_AVDECC_CALL_CONVENTION getStatus() const noexcept
	{
		return AecpStatus(AvtpduControl::getStatus());
	}
	UniqueIdentifier LA_AVDECC_CALL_CONVENTION getTargetEntityID() const noexcept
	{
		return UniqueIdentifier{ AvtpduControl::getStreamID() };
	}
	UniqueIdentifier LA_AVDECC_CALL_CONVENTION getControllerEntityID() const noexcept
	{
		return _controllerEntityID;
	}
	AecpSequenceID LA_AVDECC_CALL_CONVENTION getSequenceID() const noexcept
	{
		return _sequenceID;
	}

	/** Serialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION serialize(SerializationBuffer& buffer) const = 0;

	/** Deserialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION deserialize(DeserializationBuffer& buffer) = 0;

	/** Contruct a Response message to this Command (only changing the messageType to be of Response kind). Returns nullptr if the message is not a Command or if no Response is possible for this messageType */
	virtual LA_AVDECC_API UniquePointer LA_AVDECC_CALL_CONVENTION responseCopy() const = 0;

protected:
	/** Constructor */
	Aecpdu() noexcept;

	/** Destructor */
	virtual ~Aecpdu() noexcept override = default;

	// Defaulted compiler auto-generated methods
	Aecpdu(Aecpdu&&) = default;
	Aecpdu(Aecpdu const&) = default;
	Aecpdu& operator=(Aecpdu const&) = default;
	Aecpdu& operator=(Aecpdu&&) = default;

	void LA_AVDECC_CALL_CONVENTION setMessageType(AecpMessageType const messageType) noexcept
	{
		AvtpduControl::setControlData(messageType.getValue());
	}

private:
	/** Destroy method for COM-like interface */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION destroy() noexcept = 0;

	// Hide renamed AvtpduControl data
	using AvtpduControl::setControlData;
	using AvtpduControl::setControlDataLength;
	using AvtpduControl::setStreamID;
	using AvtpduControl::getControlData;
	using AvtpduControl::getControlDataLength;
	using AvtpduControl::getStreamID;
	// Hide Avtpdu const data
	using Avtpdu::setSubType;
	using Avtpdu::getSubType;
	// Hide AvtpduControl const data
	using AvtpduControl::setStreamValid;
	using AvtpduControl::getStreamValid;

	// Aecpdu header data
	UniqueIdentifier _controllerEntityID{};
	AecpSequenceID _sequenceID{ 0u };
};

} // namespace protocol
} // namespace avdecc
} // namespace la
