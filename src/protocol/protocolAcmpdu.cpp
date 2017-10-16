/*
* Copyright (C) 2016-2017, L-Acoustics and its contributors

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
* @file protocolAcmpdu.cpp
* @author Christophe Calmejane
*/

#include "protocolAcmpdu.hpp"
#include "la/avdecc/logger.hpp"
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

static la::avdecc::networkInterface::MacAddress Multicast_Mac_Address{ { 0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00 } };

Acmpdu::Acmpdu() noexcept
{
	EtherLayer2::setDestAddress(Multicast_Mac_Address);
	Avtpdu::setSubType(la::avdecc::protocol::AvtpSubType_Acmp);
	AvtpduControl::setStreamValid(0);
	AvtpduControl::setControlDataLength(Length);
}

void Acmpdu::serialize(SerializationBuffer& buffer) const
{
	auto const previousSize = buffer.size();
	std::uint16_t reserved{ 0u };

	buffer << _controllerEntityID << _talkerEntityID << _listenerEntityID << _talkerUniqueID << _listenerUniqueID;
	buffer.packBuffer(_streamDestAddress.data(), _streamDestAddress.size());
	buffer << _connectionCount << _sequenceID << _flags << _streamVlanID << reserved;

	if ((buffer.size() - previousSize) != Length)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Error, "Acmpdu::serialize error: Packed buffer length != expected header length");
		assert((buffer.size() - previousSize) == Length && "Acmpdu::serialize error: Packed buffer length != expected header length");
	}
}

void Acmpdu::deserialize(DeserializationBuffer& buffer)
{
	if (buffer.remaining() < Length)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Error, "Acmpdu::deserialize error: Not enough data in buffer");
		assert(buffer.remaining() >= Length && "Acmpdu::deserialize error: Not enough data in buffer");
		throw std::invalid_argument("Not enough data to deserialize");
	}

	std::uint16_t reserved;

	buffer >> _controllerEntityID >> _talkerEntityID >> _listenerEntityID >> _talkerUniqueID >> _listenerUniqueID;
	buffer.unpackBuffer(_streamDestAddress.data(), _streamDestAddress.size());
	buffer >> _connectionCount >> _sequenceID >> _flags >> _streamVlanID >> reserved;

#ifdef DEBUG
	// Do not log this error in release, it might happen too often if an entity is bugged
	if (buffer.remaining() != 0)
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Trace, "Acmpdu::deserialize warning: Remaining bytes in buffer for AcmpMessageType: " + la::avdecc::toHexString(getMessageType().getValue()));
#endif // DEBUG
}

/** Entry point */
Acmpdu* Acmpdu::createRawAcmpdu()
{
	return new Acmpdu();
}

/** Destroy method for COM-like interface */
void Acmpdu::destroy() noexcept
{
	delete this;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
