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
* @file protocolAdpdu.cpp
* @author Christophe Calmejane
*/

#include "protocolAdpdu.hpp"
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
/* Adpdu class definition                                  */
/***********************************************************/

la::avdecc::networkInterface::MacAddress Adpdu::Multicast_Mac_Address{ { 0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00 } };

Adpdu::Adpdu() noexcept
{
	AvtpduControl::setSubType(la::avdecc::protocol::AvtpSubType_Adp);
	AvtpduControl::setStreamValid(0);
	AvtpduControl::setControlDataLength(Length);
}

void Adpdu::serialize(SerializationBuffer& buffer) const
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

	if ((buffer.size() - previousSize) != Length)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Error, "Adpdu::serialize error: Packed buffer length != expected header length");
		assert((buffer.size() - previousSize) == Length && "Adpdu::serialize error: Packed buffer length != expected header length");
	}
}

void Adpdu::deserialize(DeserializationBuffer& buffer)
{
	if (buffer.remaining() < Length)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Error, "Adpdu::deserialize error: Not enough data in buffer");
		assert(buffer.remaining() >= Length && "Adpdu::deserialize error: Not enough data in buffer");
		throw std::invalid_argument("Not enough data to deserialize");
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
	// Do not log this error in release, it might happen too often if an entity is bugged
	if (buffer.remaining() != 0)
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Trace, "Adpdu::deserialize warning: Remaining bytes in buffer for AdpMessageType " + std::string(getMessageType()) + " (" + la::avdecc::toHexString(getMessageType().getValue()) + ")");
#endif // DEBUG
}

/** Entry point */
Adpdu* Adpdu::createRawAdpdu()
{
	return new Adpdu();
}

/** Destroy method for COM-like interface */
void Adpdu::destroy() noexcept
{
	delete this;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
