/*
* Copyright (C) 2016-2021, L-Acoustics and its contributors

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
* @file protocolAcmpdu.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/protocolAcmpdu.hpp"

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
/* Acmpdu class definition                                 */
/***********************************************************/

networkInterface::MacAddress const Acmpdu::Multicast_Mac_Address{ { 0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00 } };

Acmpdu::Acmpdu() noexcept
{
	EtherLayer2::setDestAddress(Multicast_Mac_Address);
	Avtpdu::setSubType(la::avdecc::protocol::AvtpSubType_Acmp);
	AvtpduControl::setStreamValid(0);
	AvtpduControl::setControlDataLength(Length);
}

Acmpdu::~Acmpdu() noexcept {}

void LA_AVDECC_CALL_CONVENTION Acmpdu::serialize(SerializationBuffer& buffer) const
{
	auto const previousSize = buffer.size();
	std::uint16_t reserved{ 0u };

	buffer << _controllerEntityID << _talkerEntityID << _listenerEntityID << _talkerUniqueID << _listenerUniqueID;
	buffer.packBuffer(_streamDestAddress.data(), _streamDestAddress.size());
	buffer << _connectionCount << _sequenceID << _flags << _streamVlanID << reserved;

	if (!AVDECC_ASSERT_WITH_RET((buffer.size() - previousSize) == Length, "Acmpdu::serialize error: Packed buffer length != expected header length"))
	{
		LOG_SERIALIZATION_ERROR(_destAddress, "Acmpdu::serialize error: Packed buffer length != expected header length");
	}
}

void LA_AVDECC_CALL_CONVENTION Acmpdu::deserialize(DeserializationBuffer& buffer)
{
	// Check if there is enough bytes to read the header
	auto const beginRemainingBytes = buffer.remaining();
	if (!AVDECC_ASSERT_WITH_RET(beginRemainingBytes >= Length, "Acmpdu::deserialize error: Not enough data in buffer"))
	{
		LOG_SERIALIZATION_ERROR(_srcAddress, "Acmpdu::deserialize error: Not enough data in buffer");
		throw std::invalid_argument("Not enough data to deserialize");
	}

	// Check is there are less advertised data than the required minimum
	if (_controlDataLength < Length)
	{
#if defined(IGNORE_INVALID_CONTROL_DATA_LENGTH)
		// Allow this packet to go through, the ControlData specific unpacker will trap any error if the message is further ill-formed
		LOG_SERIALIZATION_DEBUG(_srcAddress, "Acmpdu::deserialize error: ControlDataLength field minimum value for ACMPDU is {}. Only {} bytes advertised", Length, _controlDataLength);
#else // !IGNORE_INVALID_CONTROL_DATA_LENGTH
		LOG_SERIALIZATION_WARN(_srcAddress, "Acmpdu::deserialize error: ControlDataLength field minimum value for ACMPDU is {}. Only {} bytes advertised", Length, _controlDataLength);
		throw std::invalid_argument("ControlDataLength field value too small for ACMPDU");
#endif // IGNORE_INVALID_CONTROL_DATA_LENGTH
	}

	// Check if there is more advertised data than actual bytes in the buffer
	if (_controlDataLength > beginRemainingBytes)
	{
#if defined(IGNORE_INVALID_CONTROL_DATA_LENGTH)
		// Allow this packet to go through, the ControlData specific unpacker will trap any error if the message is further ill-formed
		LOG_SERIALIZATION_DEBUG(_srcAddress, "Acmpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer, but trying to unpack the message");
#else // !IGNORE_INVALID_CONTROL_DATA_LENGTH
		LOG_SERIALIZATION_WARN(_srcAddress, "Acmpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer, ignoring the message");
		throw std::invalid_argument("Not enough data to deserialize");
#endif // IGNORE_INVALID_CONTROL_DATA_LENGTH
	}

	std::uint16_t reserved;

	buffer >> _controllerEntityID >> _talkerEntityID >> _listenerEntityID >> _talkerUniqueID >> _listenerUniqueID;
	buffer.unpackBuffer(_streamDestAddress.data(), _streamDestAddress.size());
	buffer >> _connectionCount >> _sequenceID >> _flags >> _streamVlanID >> reserved;

#ifdef DEBUG
	// Do not log this error in release, it might happen too often if an entity is bugged or if the message contains data this version of the library do not unpack
	if (buffer.remaining() != 0 && buffer.usedBytes() >= EthernetPayloadMinimumSize)
	{
		LOG_SERIALIZATION_TRACE(_srcAddress, "Acmpdu::deserialize warning: Remaining bytes in buffer for AcmpMessageType {} ({}): {}", std::string(getMessageType()), utils::toHexString(getMessageType().getValue()), buffer.remaining());
	}
#endif // DEBUG
}

/** Copy method */
Acmpdu::UniquePointer LA_AVDECC_CALL_CONVENTION Acmpdu::copy() const
{
	auto deleter = [](Acmpdu* self)
	{
		self->destroy();
	};
	return UniquePointer(new Acmpdu(*this), deleter);
}

// Defaulted compiler auto-generated methods
Acmpdu::Acmpdu(Acmpdu&&) = default;
Acmpdu::Acmpdu(Acmpdu const&) = default;
Acmpdu& LA_AVDECC_CALL_CONVENTION Acmpdu::operator=(Acmpdu const&) = default;
Acmpdu& LA_AVDECC_CALL_CONVENTION Acmpdu::operator=(Acmpdu&&) = default;

/** Entry point */
Acmpdu* LA_AVDECC_CALL_CONVENTION Acmpdu::createRawAcmpdu() noexcept
{
	return new Acmpdu();
}

/** Destroy method for COM-like interface */
void LA_AVDECC_CALL_CONVENTION Acmpdu::destroy() noexcept
{
	delete this;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
