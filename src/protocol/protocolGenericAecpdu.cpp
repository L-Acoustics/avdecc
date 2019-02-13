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
* @file protocolGenericAecpdu.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/protocolGenericAecpdu.hpp"
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
/* GenericAecpdu class definition                          */
/***********************************************************/

GenericAecpdu::GenericAecpdu() noexcept
{
	Aecpdu::setAecpSpecificDataLength(GenericAecpdu::HeaderLength);
}

GenericAecpdu::~GenericAecpdu() noexcept {}

void LA_AVDECC_CALL_CONVENTION GenericAecpdu::setPayload(void const* const payload, size_t const payloadLength)
{
	// Check Aecp do not exceed maximum allowed length
	if (payloadLength > MaximumPayloadBufferLength)
	{
		throw std::invalid_argument("Generic payload too big");
	}

	_payloadLength = payloadLength;
	if (_payloadLength > 0)
	{
		AVDECC_ASSERT(payload != nullptr, "payload is nullptr");
		std::memcpy(_payload.data(), payload, _payloadLength);
	}
	// Don't forget to update parent's specific data length field
	setAecpSpecificDataLength(GenericAecpdu::HeaderLength + payloadLength);
}

GenericAecpdu::Payload LA_AVDECC_CALL_CONVENTION GenericAecpdu::getPayload() const noexcept
{
	return std::make_pair(_payload.data(), _payloadLength);
}

void LA_AVDECC_CALL_CONVENTION GenericAecpdu::serialize(SerializationBuffer& buffer) const
{
	// First call parent
	Aecpdu::serialize(buffer);

	auto const previousSize = buffer.size();

	auto payloadLength = _payloadLength;
	// Clamp payload in case ControlDataLength exceeds maximum allowed value
	if (payloadLength > MaximumSendPayloadBufferLength)
	{
		LOG_SERIALIZATION_WARN(_destAddress, "GenericAecpdu::serialize error: Payload size exceeds maximum protocol value of " + std::to_string(MaximumSendPayloadBufferLength) + ",  clamping buffer down from " + std::to_string(_payloadLength));
		payloadLength = std::min(payloadLength, MaximumSendPayloadBufferLength); // Clamping
	}

	buffer.packBuffer(_payload.data(), payloadLength);

	if (!AVDECC_ASSERT_WITH_RET((buffer.size() - previousSize) == (HeaderLength + payloadLength), "GenericAecpdu::serialize error: Packed buffer length != expected header length"))
	{
		LOG_SERIALIZATION_ERROR(_destAddress, "GenericAecpdu::serialize error: Packed buffer length != expected header length");
	}
}

void LA_AVDECC_CALL_CONVENTION GenericAecpdu::deserialize(DeserializationBuffer& buffer)
{
	// First call parent
	Aecpdu::deserialize(buffer);

	// Check if there is enough bytes to read the header
	if (!AVDECC_ASSERT_WITH_RET(buffer.remaining() >= HeaderLength, "GenericAecpdu::deserialize error: Not enough data in buffer"))
	{
		LOG_SERIALIZATION_ERROR(_srcAddress, "GenericAecpdu::deserialize error: Not enough data in buffer");
		throw std::invalid_argument("Not enough data to deserialize");
	}

	_payloadLength = _controlDataLength - GenericAecpdu::HeaderLength - Aecpdu::HeaderLength;

	// Check if there is more advertised data than actual bytes in the buffer (not checking earlier since we want to get as much information as possible from the packet to display a proper log message)
	auto const remainingBytes = buffer.remaining();
	if (_payloadLength > remainingBytes)
	{
#if defined(IGNORE_INVALID_CONTROL_DATA_LENGTH)
		// Allow this packet to go through, the ControlData specific unpacker will trap any error if the message is further ill-formed
		_payloadLength = remainingBytes;
		LOG_SERIALIZATION_DEBUG(_srcAddress, "GenericAecpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer");
#else // !IGNORE_INVALID_CONTROL_DATA_LENGTH
		LOG_SERIALIZATION_WARN(_srcAddress, "GenericAecpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer");
#endif // IGNORE_INVALID_CONTROL_DATA_LENGTH
	}

	// Clamp command specific buffer in case ControlDataLength exceeds maximum allowed value
	if (_payloadLength > MaximumRecvPayloadBufferLength)
	{
		LOG_SERIALIZATION_WARN(_srcAddress, "GenericAecpdu::deserialize error: Payload size exceeds maximum protocol value of " + std::to_string(MaximumRecvPayloadBufferLength) + ",  clamping buffer down from " + std::to_string(_payloadLength));
		_payloadLength = std::min(_payloadLength, MaximumRecvPayloadBufferLength); // Clamping
	}

	buffer.unpackBuffer(_payload.data(), _payloadLength);

#ifdef DEBUG
	// Do not log this error in release, it might happen too often if an entity is bugged or if the message contains data this version of the library do not unpack
	if (buffer.remaining() != 0 && buffer.usedBytes() >= EthernetPayloadMinimumSize)
		LOG_SERIALIZATION_TRACE(_srcAddress, "GenericAecpdu::deserialize warning: Remaining bytes in buffer");
#endif // DEBUG
}

/** Copy method */
Aecpdu::UniquePointer LA_AVDECC_CALL_CONVENTION GenericAecpdu::copy() const
{
	auto deleter = [](Aecpdu* self)
	{
		static_cast<GenericAecpdu*>(self)->destroy();
	};
	return UniquePointer(new GenericAecpdu(*this), deleter);
}

/** Entry point */
GenericAecpdu* LA_AVDECC_CALL_CONVENTION GenericAecpdu::createRawGenericAecpdu() noexcept
{
	return new GenericAecpdu();
}

/** Destroy method for COM-like interface */
void LA_AVDECC_CALL_CONVENTION GenericAecpdu::destroy() noexcept
{
	delete this;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
