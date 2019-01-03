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
* @file protocolMvuAecpdu.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/protocolMvuAecpdu.hpp"
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
/* MvuAecpdu class definition                              */
/***********************************************************/

VuAecpdu::ProtocolIdentifier MvuAecpdu::ProtocolID{ { 0x00, 0x1b, 0xc5, 0x0a, 0xc1, 0x00 } }; /* Avnu OUI-36 (00-1B-C5-0A-C) + MVU ProtocolUniqueIdentifier (0x100) */

MvuAecpdu::MvuAecpdu() noexcept
{
	Aecpdu::setAecpSpecificDataLength(VuAecpdu::HeaderLength + MvuAecpdu::HeaderLength);
	VuAecpdu::setProtocolIdentifier(ProtocolID);
}

MvuAecpdu::~MvuAecpdu() noexcept {}

void LA_AVDECC_CALL_CONVENTION MvuAecpdu::setCommandType(MvuCommandType const commandType) noexcept
{
	_commandType = commandType;
}

void LA_AVDECC_CALL_CONVENTION MvuAecpdu::setCommandSpecificData(void const* const commandSpecificData, size_t const commandSpecificDataLength)
{
	// Check Aecp do not exceed maximum allowed length
	if (commandSpecificDataLength > MaximumPayloadBufferLength)
	{
		throw std::invalid_argument("MVU payload too big");
	}

	_commandSpecificDataLength = commandSpecificDataLength;
	if (_commandSpecificDataLength > 0)
	{
		AVDECC_ASSERT(commandSpecificData != nullptr, "commandSpecificData is nullptr");
		std::memcpy(_commandSpecificData.data(), commandSpecificData, _commandSpecificDataLength);
	}
	// Don't forget to update parent's specific data length field
	setAecpSpecificDataLength(VuAecpdu::HeaderLength + MvuAecpdu::HeaderLength + commandSpecificDataLength);
}

MvuCommandType LA_AVDECC_CALL_CONVENTION MvuAecpdu::getCommandType() const noexcept
{
	return _commandType;
}

MvuAecpdu::Payload LA_AVDECC_CALL_CONVENTION MvuAecpdu::getPayload() const noexcept
{
	return std::make_pair(_commandSpecificData.data(), _commandSpecificDataLength);
}

void LA_AVDECC_CALL_CONVENTION MvuAecpdu::serialize(SerializationBuffer& buffer) const
{
	// First call parent
	VuAecpdu::serialize(buffer);

	auto const previousSize = buffer.size();

	std::uint8_t reserved{ 0u };
	buffer << static_cast<std::uint16_t>(((reserved << 15) & 0x8000) | (_commandType.getValue() & 0x7fff));

	auto payloadLength = _commandSpecificDataLength;
	// Clamp command specific buffer in case ControlDataLength exceeds maximum allowed value
	if (payloadLength > MaximumSendPayloadBufferLength)
	{
		LOG_SERIALIZATION_WARN(_destAddress, "MvuAecpdu::serialize error: Payload size exceeds maximum protocol value of " + std::to_string(MaximumSendPayloadBufferLength) + " for MvuCommandType " + std::string(_commandType) + " (" + utils::toHexString(_commandType.getValue()) + "),  clamping buffer down from " + std::to_string(payloadLength));
		payloadLength = std::min(payloadLength, MaximumSendPayloadBufferLength); // Clamping
	}

	buffer.packBuffer(_commandSpecificData.data(), payloadLength);

	if (!AVDECC_ASSERT_WITH_RET((buffer.size() - previousSize) == (HeaderLength + payloadLength), "MvuAecpdu::serialize error: Packed buffer length != expected header length"))
	{
		LOG_SERIALIZATION_ERROR(_destAddress, "MvuAecpdu::serialize error: Packed buffer length != expected header length");
	}
}

void LA_AVDECC_CALL_CONVENTION MvuAecpdu::deserialize(DeserializationBuffer& buffer)
{
	// First call parent
	VuAecpdu::deserialize(buffer);

	// Check if there is enough bytes to read the header
	if (!AVDECC_ASSERT_WITH_RET(buffer.remaining() >= HeaderLength, "MvuAecpdu::deserialize error: Not enough data in buffer"))
	{
		LOG_SERIALIZATION_ERROR(_srcAddress, "MvuAecpdu::deserialize error: Not enough data in buffer");
		throw std::invalid_argument("Not enough data to deserialize");
	}

	std::uint16_t u_ct;
	std::uint8_t reserved{ 0u };

	buffer >> u_ct;

	reserved = ((u_ct & 0x8000) >> 15) != 0;
	_commandType = static_cast<MvuCommandType>(u_ct & 0x7fff);

	_commandSpecificDataLength = _controlDataLength - VuAecpdu::HeaderLength - MvuAecpdu::HeaderLength - Aecpdu::HeaderLength;

	// Check reserved bit
	if (reserved != 0)
	{
		LOG_SERIALIZATION_WARN(_srcAddress, "MvuAecpdu::deserialize error: Reserved bit is not set to 0 for MvuCommandType " + std::string(_commandType) + " (" + utils::toHexString(_commandType.getValue()) + ")");
	}

	// Check if there is more advertised data than actual bytes in the buffer (not checking earlier since we want to get as much information as possible from the packet to display a proper log message)
	auto const remainingBytes = buffer.remaining();
	if (_commandSpecificDataLength > remainingBytes)
	{
#if defined(IGNORE_INVALID_CONTROL_DATA_LENGTH)
		// Allow this packet to go through, the ControlData specific unpacker will trap any error if the message is further ill-formed
		_commandSpecificDataLength = remainingBytes;
		LOG_SERIALIZATION_DEBUG(_srcAddress, "MvuAecpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer for MvuCommandType " + std::string(_commandType) + " (" + utils::toHexString(_commandType.getValue()) + ")");
#else // !IGNORE_INVALID_CONTROL_DATA_LENGTH
		LOG_SERIALIZATION_WARN(_srcAddress, "MvuAecpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer for MvuCommandType " + std::string(_commandType) + " (" + utils::toHexString(_commandType.getValue()) + ")");
#endif // IGNORE_INVALID_CONTROL_DATA_LENGTH
	}

	// Clamp command specific buffer in case ControlDataLength exceeds maximum allowed value, the ControlData specific unpacker will trap any error if the message is further ill-formed
	if (_commandSpecificDataLength > MaximumRecvPayloadBufferLength)
	{
		LOG_SERIALIZATION_WARN(_srcAddress, "MvuAecpdu::deserialize error: Payload size exceeds maximum protocol value of " + std::to_string(MaximumRecvPayloadBufferLength) + " for MvuCommandType " + std::string(_commandType) + " (" + utils::toHexString(_commandType.getValue()) + "),  clamping buffer down from " + std::to_string(_commandSpecificDataLength));
		_commandSpecificDataLength = std::min(_commandSpecificDataLength, MaximumRecvPayloadBufferLength); // Clamping
	}

	buffer.unpackBuffer(_commandSpecificData.data(), _commandSpecificDataLength);

#ifdef DEBUG
	// Do not log this error in release, it might happen too often if an entity is bugged or if the message contains data this version of the library do not unpack
	if (buffer.remaining() != 0 && buffer.usedBytes() >= EthernetPayloadMinimumSize)
		LOG_SERIALIZATION_TRACE(_srcAddress, "MvuAecpdu::deserialize warning: Remaining bytes in buffer for MvuCommandType " + std::string(_commandType) + " (" + utils::toHexString(_commandType.getValue()) + ")");
#endif // DEBUG
}

/** Copy method */
Aecpdu::UniquePointer LA_AVDECC_CALL_CONVENTION MvuAecpdu::copy() const
{
	auto deleter = [](Aecpdu* self)
	{
		static_cast<MvuAecpdu*>(self)->destroy();
	};
	return UniquePointer(new MvuAecpdu(*this), deleter);
}

/** Entry point */
MvuAecpdu* LA_AVDECC_CALL_CONVENTION MvuAecpdu::createRawMvuAecpdu()
{
	return new MvuAecpdu();
}

/** Destroy method for COM-like interface */
void LA_AVDECC_CALL_CONVENTION MvuAecpdu::destroy() noexcept
{
	delete this;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
