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
* @file protocolAemAecpdu.cpp
* @author Christophe Calmejane
*/

#include "protocolAemAecpdu.hpp"
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
/* AemAecpdu class definition                              */
/***********************************************************/

la::avdecc::networkInterface::MacAddress AemAecpdu::Identify_Mac_Address{ { 0x91, 0xe0, 0xf0, 0x01, 0x00, 0x01 } };

AemAecpdu::AemAecpdu() noexcept
{
	// Initialize parent's specific data length field
	Aecpdu::setAecpSpecificDataLength(AemAecpdu::HeaderLength); // Might throw, but it's not possible from here (if a compiler complains about this constructor declared as 'noexcept' but calling a method not tagged as 'noexcept', add a try-catch)
}

void AemAecpdu::serialize(SerializationBuffer& buffer) const
{
	// First call parent
	Aecpdu::serialize(buffer);

	auto const previousSize = buffer.size();

	buffer << static_cast<std::uint16_t>(((_unsolicited << 15) & 0x8000) | (_commandType.getValue() & 0x7fff));
	buffer.packBuffer(_commandSpecificData.data(), _commandSpecificDataLength);

	if ((buffer.size() - previousSize) != (HeaderLength + _commandSpecificDataLength))
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Error, "AemAecpdu::serialize error: Packed buffer length != expected header length");
		assert((buffer.size() - previousSize) == (HeaderLength + _commandSpecificDataLength) && "AemAecpdu::serialize error: Packed buffer length != expected header length");
	}
}

void AemAecpdu::deserialize(DeserializationBuffer& buffer)
{
	// First call parent
	Aecpdu::deserialize(buffer);

	if (buffer.remaining() < HeaderLength)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Error, "AemAecpdu::deserialize error: Not enough data in buffer");
		assert(buffer.remaining() >= HeaderLength && "AemAecpdu::deserialize error: Not enough data in buffer");
		throw std::invalid_argument("Not enough data to deserialize");
	}

	std::uint16_t u_ct;

	buffer >> u_ct;

	_unsolicited = ((u_ct & 0x8000) >> 15) != 0;
	_commandType = static_cast<AemCommandType>(u_ct & 0x7fff);

	_commandSpecificDataLength = _controlDataLength - AemAecpdu::HeaderLength - Aecpdu::HeaderLength;
	auto const remainingBytes = buffer.remaining();
	if (_commandSpecificDataLength > remainingBytes)
	{
		Logger::Level logLevel{ Logger::Level::Warn };
#if defined(IGNORE_INVALID_CONTROL_DATA_LENGTH)
		// Allow this packet to go through, the ControlData specific unpacker will trap the error anyway
		_commandSpecificDataLength = remainingBytes;
		logLevel = Logger::Level::Debug;
#endif // IGNORE_INVALID_CONTROL_DATA_LENGTH
		Logger::getInstance().log(Logger::Layer::Protocol, logLevel, "AemAecpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer for AemCommandType: " + la::avdecc::toHexString(_commandType.getValue()));
	}
	buffer.unpackBuffer(_commandSpecificData.data(), _commandSpecificDataLength);

#ifdef DEBUG
	// Do not log this error in release, it might happen too often if an entity is bugged
	if (buffer.remaining() != 0 && buffer.usedBytes() >= EthernetPayloadMinimumSize)
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Trace, "AemAecpdu::deserialize warning: Remaining bytes in buffer for AemCommandType: " + la::avdecc::toHexString(_commandType.getValue()));
#endif // DEBUG
}

/** Entry point */
AemAecpdu* AemAecpdu::createRawAemAecpdu()
{
	return new AemAecpdu();
}

/** Destroy method for COM-like interface */
void AemAecpdu::destroy() noexcept
{
	delete this;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
