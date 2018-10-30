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
* @file protocolAecpdu.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/protocolAecpdu.hpp"
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
/* Aecpdu class definition                                 */
/***********************************************************/

Aecpdu::Aecpdu() noexcept
{
	AvtpduControl::setSubType(la::avdecc::protocol::AvtpSubType_Aecp);
	AvtpduControl::setStreamValid(0);
}

void LA_AVDECC_CALL_CONVENTION Aecpdu::serialize(SerializationBuffer& buffer) const
{
	auto const previousSize = buffer.size();

	buffer << _controllerEntityID << _sequenceID;

	// ControlDataLength exceeds maximum protocol value
	if (_controlDataLength > Aecpdu::MaximumSendLength)
	{
		LOG_SERIALIZATION_WARN(_destAddress, "Aecpdu::serialize warning: ControlDataLength field exceeds maximum allowed value of {}: {}", Aecpdu::MaximumSendLength, _controlDataLength);
	}

	if (!AVDECC_ASSERT_WITH_RET((buffer.size() - previousSize) == HeaderLength, "Aecpdu::serialize error: Packed buffer length != expected header length"))
	{
		LOG_SERIALIZATION_WARN(_destAddress, "Aecpdu::serialize error: Packed buffer length != expected header length");
	}
}

void LA_AVDECC_CALL_CONVENTION Aecpdu::deserialize(DeserializationBuffer& buffer)
{
	// Check if there is enough bytes to read the header
	if (!AVDECC_ASSERT_WITH_RET(buffer.remaining() >= HeaderLength, "Aecpdu::deserialize error: Not enough data in buffer"))
	{
		LOG_SERIALIZATION_ERROR(_srcAddress, "Aecpdu::deserialize error: Not enough data in buffer");
		throw std::invalid_argument("Not enough data to deserialize");
	}

	// ControlDataLength exceeds maximum protocol value
	if (_controlDataLength > Aecpdu::MaximumRecvLength)
	{
		LOG_SERIALIZATION_WARN(_srcAddress, "Aecpdu::deserialize warning: ControlDataLength field exceeds maximum allowed value of {}: {}", Aecpdu::MaximumRecvLength, _controlDataLength);
	}

	buffer >> _controllerEntityID >> _sequenceID;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
