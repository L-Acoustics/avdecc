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
* @file protocolAemAecpdu.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/protocolAemAecpdu.hpp"

#include "logHelper.hpp"

#include <cassert>
#include <string>

namespace la
{
namespace avdecc
{
namespace protocol
{
/***********************************************************/
/* AemAecpdu class definition                              */
/***********************************************************/

networkInterface::MacAddress const AemAecpdu::Identify_Mac_Address{ { 0x91, 0xe0, 0xf0, 0x01, 0x00, 0x01 } };
UniqueIdentifier const AemAecpdu::Identify_ControllerEntityID{ 0x90E0F0FFFE010001 };

AemAecpdu::AemAecpdu(bool const isResponse) noexcept
{
	Aecpdu::setMessageType(isResponse ? AecpMessageType::AemResponse : AecpMessageType::AemCommand);
	Aecpdu::setAecpSpecificDataLength(HeaderLength);
}

AemAecpdu::~AemAecpdu() noexcept {}

void LA_AVDECC_CALL_CONVENTION AemAecpdu::setUnsolicited(bool const unsolicited) noexcept
{
	_unsolicited = unsolicited;
}

void LA_AVDECC_CALL_CONVENTION AemAecpdu::setCommandType(AemCommandType const commandType) noexcept
{
	_commandType = commandType;
}

void LA_AVDECC_CALL_CONVENTION AemAecpdu::setCommandSpecificData(void const* const commandSpecificData, size_t const commandSpecificDataLength)
{
	// Check Aecp do not exceed maximum allowed length
	if (commandSpecificDataLength > MaximumPayloadBufferLength)
	{
		throw std::invalid_argument("AEM payload too big");
	}

	_commandSpecificDataLength = commandSpecificDataLength;
	if (_commandSpecificDataLength > 0)
	{
		AVDECC_ASSERT(commandSpecificData != nullptr, "commandSpecificData is nullptr");
		std::memcpy(_commandSpecificData.data(), commandSpecificData, _commandSpecificDataLength);
	}
	// Don't forget to update parent's specific data length field
	setAecpSpecificDataLength(HeaderLength + commandSpecificDataLength);
}

bool LA_AVDECC_CALL_CONVENTION AemAecpdu::getUnsolicited() const noexcept
{
	return _unsolicited;
}

AemCommandType LA_AVDECC_CALL_CONVENTION AemAecpdu::getCommandType() const noexcept
{
	return _commandType;
}

AemAecpdu::Payload LA_AVDECC_CALL_CONVENTION AemAecpdu::getPayload() const noexcept
{
	return std::make_pair(_commandSpecificData.data(), _commandSpecificDataLength);
}

void LA_AVDECC_CALL_CONVENTION AemAecpdu::serialize(SerializationBuffer& buffer) const
{
	// First call parent
	Aecpdu::serialize(buffer);

	auto const previousSize = buffer.size();

	buffer << static_cast<std::uint16_t>(((_unsolicited << 15) & 0x8000) | (_commandType.getValue() & 0x7fff));

	auto payloadLength = _commandSpecificDataLength;
	// Clamp command specific buffer in case ControlDataLength exceeds maximum allowed value
	if (payloadLength > MaximumSendPayloadBufferLength)
	{
		LOG_SERIALIZATION_WARN(_destAddress, "AemAecpdu::serialize error: Payload size exceeds maximum protocol value of " + std::to_string(MaximumSendPayloadBufferLength) + " for AemCommandType " + std::string(_commandType) + " (" + utils::toHexString(_commandType.getValue()) + "),  clamping buffer down from " + std::to_string(payloadLength));
		payloadLength = std::min(payloadLength, MaximumSendPayloadBufferLength); // Clamping
	}

	buffer.packBuffer(_commandSpecificData.data(), payloadLength);

	if (!AVDECC_ASSERT_WITH_RET((buffer.size() - previousSize) == (HeaderLength + payloadLength), "AemAecpdu::serialize error: Packed buffer length != expected header length"))
	{
		LOG_SERIALIZATION_ERROR(_destAddress, "AemAecpdu::serialize error: Packed buffer length != expected header length");
	}
}

void LA_AVDECC_CALL_CONVENTION AemAecpdu::deserialize(DeserializationBuffer& buffer)
{
	// First call parent
	Aecpdu::deserialize(buffer);

	// Check if there is enough bytes to read the header
	if (!AVDECC_ASSERT_WITH_RET(buffer.remaining() >= HeaderLength, "AemAecpdu::deserialize error: Not enough data in buffer"))
	{
		LOG_SERIALIZATION_ERROR(_srcAddress, "AemAecpdu::deserialize error: Not enough data in buffer");
		throw std::invalid_argument("Not enough data to deserialize");
	}

	std::uint16_t u_ct;

	buffer >> u_ct;

	_unsolicited = ((u_ct & 0x8000) >> 15) != 0;
	_commandType = static_cast<AemCommandType>(u_ct & 0x7fff);

	// Check is there are less advertised data than the required minimum (we can do it after we (tried) unpacked as it would have thrown in case the buffer was too small)
	auto constexpr minCDL = HeaderLength + Aecpdu::HeaderLength;
	if (_controlDataLength < minCDL)
	{
#if defined(IGNORE_INVALID_CONTROL_DATA_LENGTH)
		// Allow this packet to go through, the ControlData specific unpacker will trap any error if the message is further ill-formed
		LOG_SERIALIZATION_DEBUG(_srcAddress, "AemAecpdu::deserialize error: ControlDataLength field minimum value for AEM-AECPDU is {}. AemCommandType {} ({}) only advertise {} bytes", minCDL, std::string(_commandType), utils::toHexString(_commandType.getValue()), _controlDataLength);
		_commandSpecificDataLength = 0u;
#else // !IGNORE_INVALID_CONTROL_DATA_LENGTH
		LOG_SERIALIZATION_WARN(_srcAddress, "AemAecpdu::deserialize error: ControlDataLength field minimum value for AEM-AECPDU is {}. AemCommandType {} ({}) only advertise {} bytes", minCDL, std::string(_commandType), utils::toHexString(_commandType.getValue()), _controlDataLength);
		throw std::invalid_argument("ControlDataLength field value too small for AEM-AECPDU");
#endif // IGNORE_INVALID_CONTROL_DATA_LENGTH
	}
	else
	{
		_commandSpecificDataLength = _controlDataLength - minCDL;
	}

	// Check if there is more advertised data than actual bytes in the buffer (not checking earlier since we want to get as much information as possible from the packet to display a proper log message)
	auto const remainingBytes = buffer.remaining();
	if (_commandSpecificDataLength > remainingBytes)
	{
#if defined(IGNORE_INVALID_CONTROL_DATA_LENGTH)
		// Allow this packet to go through, the ControlData specific unpacker will trap any error if the message is further ill-formed
		_commandSpecificDataLength = remainingBytes;
		LOG_SERIALIZATION_DEBUG(_srcAddress, "AemAecpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer for AemCommandType " + std::string(_commandType) + " (" + utils::toHexString(_commandType.getValue()) + ")");
#else // !IGNORE_INVALID_CONTROL_DATA_LENGTH
		LOG_SERIALIZATION_WARN(_srcAddress, "AemAecpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer for AemCommandType " + std::string(_commandType) + " (" + utils::toHexString(_commandType.getValue()) + ")");
#endif // IGNORE_INVALID_CONTROL_DATA_LENGTH
	}

	// Clamp command specific buffer in case ControlDataLength exceeds maximum allowed value, the ControlData specific unpacker will trap any error if the message is further ill-formed
	if (_commandSpecificDataLength > MaximumRecvPayloadBufferLength)
	{
		LOG_SERIALIZATION_WARN(_srcAddress, "AemAecpdu::deserialize error: Payload size exceeds maximum protocol value of " + std::to_string(MaximumRecvPayloadBufferLength) + " for AemCommandType " + std::string(_commandType) + " (" + utils::toHexString(_commandType.getValue()) + "),  clamping buffer down from " + std::to_string(_commandSpecificDataLength));
		_commandSpecificDataLength = std::min(_commandSpecificDataLength, MaximumRecvPayloadBufferLength); // Clamping
	}

	buffer.unpackBuffer(_commandSpecificData.data(), _commandSpecificDataLength);

#ifdef DEBUG
	// Do not log this error in release, it might happen too often if an entity is bugged or if the message contains data this version of the library do not unpack
	if (buffer.remaining() != 0 && buffer.usedBytes() >= EthernetPayloadMinimumSize)
	{
		LOG_SERIALIZATION_TRACE(_srcAddress, "AemAecpdu::deserialize warning: Remaining bytes in buffer for AemCommandType {} ({}): {}", std::string(_commandType), utils::toHexString(_commandType.getValue()), buffer.remaining());
	}
#endif // DEBUG
}

/** Contruct a Response message to this Command (only changing the messageType to be of Response kind). Returns nullptr if the message is not a Command or if no Response is possible for this messageType */
Aecpdu::UniquePointer LA_AVDECC_CALL_CONVENTION AemAecpdu::responseCopy() const
{
	if (!AVDECC_ASSERT_WITH_RET(getMessageType() == AecpMessageType::AemCommand, "Calling AemAecpdu::reflectedResponse() on something that is not an AEM_COMMAND"))
	{
		return UniquePointer{ nullptr, nullptr };
	}

	auto deleter = [](Aecpdu* self)
	{
		static_cast<AemAecpdu*>(self)->destroy();
	};

	// Create a response message as a copy of this
	auto response = UniquePointer(new AemAecpdu(*this), deleter);
	auto& aem = static_cast<AemAecpdu&>(*response);

	// Change the message type to be an AEM_RESPONSE
	aem.setMessageType(AecpMessageType::AemResponse);

	// Return the created response
	return response;
}

/** Entry point */
AemAecpdu* LA_AVDECC_CALL_CONVENTION AemAecpdu::createRawAemAecpdu(bool const isResponse) noexcept
{
	return new AemAecpdu(isResponse);
}

/** Destroy method for COM-like interface */
void LA_AVDECC_CALL_CONVENTION AemAecpdu::destroy() noexcept
{
	delete this;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
