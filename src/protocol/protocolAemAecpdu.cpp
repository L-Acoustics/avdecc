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
* @file protocolAemAecpdu.cpp
* @author Christophe Calmejane
*/

#include "protocolAemAecpdu.hpp"
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

la::avdecc::networkInterface::MacAddress AemAecpdu::Identify_Mac_Address{ { 0x91, 0xe0, 0xf0, 0x01, 0x00, 0x01 } };

AemAecpdu::AemAecpdu() noexcept
{
	// Initialize parent's specific data length field
	try
	{
		Aecpdu::setAecpSpecificDataLength(AemAecpdu::HeaderLength); // This method might throw, but it's not possible from here
	}
	catch (std::exception const&)
	{
	}
}

void AemAecpdu::serialize(SerializationBuffer& buffer) const
{
	// First call parent
	Aecpdu::serialize(buffer);

	auto const previousSize = buffer.size();

	buffer << static_cast<std::uint16_t>(((_unsolicited << 15) & 0x8000) | (_commandType.getValue() & 0x7fff));
	buffer.packBuffer(_commandSpecificData.data(), _commandSpecificDataLength);

	if (!AVDECC_ASSERT_WITH_RET((buffer.size() - previousSize) == (HeaderLength + _commandSpecificDataLength), "AemAecpdu::serialize error: Packed buffer length != expected header length"))
	{
		LOG_SERIALIZATION_ERROR(_srcAddress, "AemAecpdu::serialize error: Packed buffer length != expected header length");
	}
}

void AemAecpdu::deserialize(DeserializationBuffer& buffer)
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

	_commandSpecificDataLength = _controlDataLength - AemAecpdu::HeaderLength - Aecpdu::HeaderLength;

	// Check if there is more advertised data than actual bytes in the buffer (not checking earlier since we want to get as much information as possible from the packet to display a proper log message)
	auto const remainingBytes = buffer.remaining();
	if (_commandSpecificDataLength > remainingBytes)
	{
#if defined(IGNORE_INVALID_CONTROL_DATA_LENGTH)
		// Allow this packet to go through, the ControlData specific unpacker will trap any error if the message is further ill-formed
		_commandSpecificDataLength = remainingBytes;
		LOG_SERIALIZATION_DEBUG(_srcAddress, "AemAecpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer for AemCommandType " + std::string(_commandType) + " (" + la::avdecc::toHexString(_commandType.getValue()) + ")");
#else // !IGNORE_INVALID_CONTROL_DATA_LENGTH
		LOG_SERIALIZATION_WARN(_srcAddress, "AemAecpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer for AemCommandType " + std::string(_commandType) + " (" + la::avdecc::toHexString(_commandType.getValue()) + ")");
#endif // IGNORE_INVALID_CONTROL_DATA_LENGTH
	}

	// Clamp command specific buffer in case ControlDataLength exceeds maximum protocol value, the ControlData specific unpacker will trap any error if the message is further ill-formed
	if (_commandSpecificDataLength > MaximumPayloadLength)
	{
#if defined(ALLOW_BIG_AEM_PAYLOADS)
		LOG_SERIALIZATION_INFO(_srcAddress, "AemAecpdu::deserialize error: Payload size exceeds maximum protocol value of " + std::to_string(MaximumPayloadLength) + " for AemCommandType " + std::string(_commandType) + " (" + la::avdecc::toHexString(_commandType.getValue()) + "),  but still processing it because of compilation option ALLOW_BIG_AEM_PAYLOADS");
#else // !ALLOW_BIG_AEM_PAYLOADS
		LOG_SERIALIZATION_WARN(_srcAddress, "AemAecpdu::deserialize error: Payload size exceeds maximum protocol value of " + std::to_string(MaximumPayloadLength) + " for AemCommandType " + std::string(_commandType) + " (" + la::avdecc::toHexString(_commandType.getValue()) + "),  clamping buffer down from " + std::to_string(_commandSpecificDataLength));
#endif // ALLOW_BIG_AEM_PAYLOADS
		_commandSpecificDataLength = std::min(_commandSpecificDataLength, _commandSpecificData.size());
	}

	buffer.unpackBuffer(_commandSpecificData.data(), _commandSpecificDataLength);

#ifdef DEBUG
	// Do not log this error in release, it might happen too often if an entity is bugged or if the message contains data this version of the library do not unpack
	if (buffer.remaining() != 0 && buffer.usedBytes() >= EthernetPayloadMinimumSize)
		LOG_SERIALIZATION_TRACE(_srcAddress, "AemAecpdu::deserialize warning: Remaining bytes in buffer for AemCommandType " + std::string(_commandType) + " (" + la::avdecc::toHexString(_commandType.getValue()) + ")");
#endif // DEBUG
}

/** Copy method */
Aecpdu::UniquePointer AemAecpdu::copy() const
{
	auto deleter = [](Aecpdu* self)
	{
		static_cast<AemAecpdu*>(self)->destroy();
	};
	return UniquePointer(new AemAecpdu(*this), deleter);
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
