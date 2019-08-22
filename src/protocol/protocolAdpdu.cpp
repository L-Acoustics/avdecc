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
* @file protocolAdpdu.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/protocolAdpdu.hpp"

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
/* Adpdu class definition                                  */
/***********************************************************/

la::avdecc::networkInterface::MacAddress const Adpdu::Multicast_Mac_Address{ { 0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00 } };

Adpdu::Adpdu() noexcept
{
	AvtpduControl::setSubType(la::avdecc::protocol::AvtpSubType_Adp);
	AvtpduControl::setStreamValid(0);
	AvtpduControl::setControlDataLength(Length);
}

void LA_AVDECC_CALL_CONVENTION Adpdu::serialize(SerializationBuffer& buffer) const
{
	auto const previousSize = buffer.size();
	std::uint32_t reserved0{ 0u };
	std::uint32_t reserved1{ 0u };

	buffer << _entityModelID << _entityCapabilities;
	buffer << _talkerStreamSources << _talkerCapabilities;
	buffer << _listenerStreamSinks << _listenerCapabilities;
	buffer << _controllerCapabilities;
	buffer << _availableIndex;
	buffer << _gptpGrandmasterID << static_cast<std::uint32_t>(((_gptpDomainNumber << 24) & 0xff000000) | (reserved0 & 0x00ffffff));
	buffer << _identifyControlIndex << _interfaceIndex << _associationID << reserved1;

	if (!AVDECC_ASSERT_WITH_RET((buffer.size() - previousSize) == Length, "Adpdu::serialize error: Packed buffer length != expected header length"))
	{
		LOG_SERIALIZATION_ERROR(_destAddress, "Adpdu::serialize error: Packed buffer length != expected header length");
	}
}

void LA_AVDECC_CALL_CONVENTION Adpdu::deserialize(DeserializationBuffer& buffer)
{
	// Check if there is enough bytes to read the header
	auto const beginRemainingBytes = buffer.remaining();
	if (!AVDECC_ASSERT_WITH_RET(beginRemainingBytes >= Length, "Adpdu::deserialize error: Not enough data in buffer"))
	{
		LOG_SERIALIZATION_ERROR(_srcAddress, "Adpdu::deserialize error: Not enough data in buffer");
		throw std::invalid_argument("Not enough data to deserialize");
	}

	// Check is there are less advertised data than the required minimum
	if (_controlDataLength < Length)
	{
#if defined(IGNORE_INVALID_CONTROL_DATA_LENGTH)
		// Allow this packet to go through, the ControlData specific unpacker will trap any error if the message is further ill-formed
		LOG_SERIALIZATION_DEBUG(_srcAddress, "Adpdu::deserialize error: ControlDataLength field minimum value for ADPDU is {}. Only {} bytes advertised", Length, _controlDataLength);
#else // !IGNORE_INVALID_CONTROL_DATA_LENGTH
		LOG_SERIALIZATION_WARN(_srcAddress, "Adpdu::deserialize error: ControlDataLength field minimum value for ADPDU is {}. Only {} bytes advertised", Length, _controlDataLength);
		throw std::invalid_argument("ControlDataLength field value too small for ADPDU");
#endif // IGNORE_INVALID_CONTROL_DATA_LENGTH
	}

	// Check if there is more advertised data than actual bytes in the buffer
	if (_controlDataLength > beginRemainingBytes)
	{
#if defined(IGNORE_INVALID_CONTROL_DATA_LENGTH)
		// Allow this packet to go through, the ControlData specific unpacker will trap any error if the message is further ill-formed
		LOG_SERIALIZATION_DEBUG(_srcAddress, "Adpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer, but trying to unpack the message");
#else // !IGNORE_INVALID_CONTROL_DATA_LENGTH
		LOG_SERIALIZATION_WARN(_srcAddress, "Adpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer, ignoring the message");
		throw std::invalid_argument("Not enough data to deserialize");
#endif // IGNORE_INVALID_CONTROL_DATA_LENGTH
	}

	std::uint32_t reserved1{ 0u };
	std::uint32_t gdn_res;

	buffer >> _entityModelID >> _entityCapabilities;
	buffer >> _talkerStreamSources >> _talkerCapabilities;
	buffer >> _listenerStreamSinks >> _listenerCapabilities;
	buffer >> _controllerCapabilities;
	buffer >> _availableIndex;
	buffer >> _gptpGrandmasterID >> gdn_res;
	buffer >> _identifyControlIndex >> _interfaceIndex >> _associationID >> reserved1;

	_gptpDomainNumber = ((gdn_res & 0xff000000) >> 24) != 0;

#ifdef DEBUG
	// Do not log this error in release, it might happen too often if an entity is bugged or if the message contains data this version of the library do not unpack
	if (buffer.remaining() != 0 && buffer.usedBytes() >= EthernetPayloadMinimumSize)
		LOG_SERIALIZATION_TRACE(_srcAddress, "Adpdu::deserialize warning: Remaining bytes in buffer for AdpMessageType " + std::string(getMessageType()) + " (" + utils::toHexString(getMessageType().getValue()) + ")");
#endif // DEBUG
}

/** Copy method */
Adpdu::UniquePointer LA_AVDECC_CALL_CONVENTION Adpdu::copy() const
{
	auto deleter = [](Adpdu* self)
	{
		self->destroy();
	};
	return UniquePointer(new Adpdu(*this), deleter);
}

/** Entry point */
Adpdu* LA_AVDECC_CALL_CONVENTION Adpdu::createRawAdpdu() noexcept
{
	return new Adpdu();
}

/** Destroy method for COM-like interface */
void LA_AVDECC_CALL_CONVENTION Adpdu::destroy() noexcept
{
	delete this;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
