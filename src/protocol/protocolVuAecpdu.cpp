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
* @file protocolVuAecpdu.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/protocolVuAecpdu.hpp"

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
/* VuAecpdu class definition                               */
/***********************************************************/

VuAecpdu::VuAecpdu(bool const isResponse) noexcept
{
	Aecpdu::setMessageType(isResponse ? AecpMessageType::VendorUniqueResponse : AecpMessageType::VendorUniqueCommand);
	Aecpdu::setAecpSpecificDataLength(VuAecpdu::HeaderLength);
}

void LA_AVDECC_CALL_CONVENTION VuAecpdu::setProtocolIdentifier(ProtocolIdentifier const& protocolIdentifier) noexcept
{
	_protocolIdentifier = protocolIdentifier;
}

VuAecpdu::ProtocolIdentifier LA_AVDECC_CALL_CONVENTION VuAecpdu::getProtocolIdentifier() const noexcept
{
	return _protocolIdentifier;
}

void LA_AVDECC_CALL_CONVENTION VuAecpdu::serialize(SerializationBuffer& buffer) const
{
	// First call parent
	Aecpdu::serialize(buffer);

	auto const previousSize = buffer.size();

	buffer << static_cast<ProtocolIdentifier::ArrayType>(_protocolIdentifier);

	if (!AVDECC_ASSERT_WITH_RET((buffer.size() - previousSize) == HeaderLength, "VuAecpdu::serialize error: Packed buffer length != expected header length"))
	{
		LOG_SERIALIZATION_ERROR(_destAddress, "VuAecpdu::serialize error: Packed buffer length != expected header length");
	}
}

void LA_AVDECC_CALL_CONVENTION VuAecpdu::deserialize(DeserializationBuffer& buffer)
{
	// First call parent
	Aecpdu::deserialize(buffer);

	// Check if there is enough bytes to read the header
	if (!AVDECC_ASSERT_WITH_RET(buffer.remaining() >= HeaderLength, "VuAecpdu::deserialize error: Not enough data in buffer"))
	{
		LOG_SERIALIZATION_ERROR(_srcAddress, "VuAecpdu::deserialize error: Not enough data in buffer");
		throw std::invalid_argument("Not enough data to deserialize");
	}

	ProtocolIdentifier::ArrayType protocolIdentifier{};

	buffer >> protocolIdentifier;

	_protocolIdentifier.setValue(protocolIdentifier);
}

} // namespace protocol
} // namespace avdecc
} // namespace la
