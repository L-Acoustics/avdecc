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
* @file protocolAvtpdu.cpp
* @author Christophe Calmejane
*/

#include "protocolAvtpdu.hpp"
#include "logHelper.hpp"
#include <cassert>

namespace la
{
namespace avdecc
{
namespace protocol
{

/***********************************************************/
/* Ethernet layer 2 class definition                       */
/***********************************************************/

void EtherLayer2::serialize(SerializationBuffer& buffer) const
{
#ifdef DEBUG
	auto const previousSize = buffer.size();
#endif // DEBUG

	buffer.packBuffer(_destAddress.data(), _destAddress.size());
	buffer.packBuffer(_srcAddress.data(), _srcAddress.size());
	buffer << _etherType;

#ifdef DEBUG
	AVDECC_ASSERT((buffer.size() - previousSize) == HeaderLength, "EtherLayer2::serialize error: Packed buffer length != expected header length");
#endif // DEBUG
}

void EtherLayer2::deserialize(DeserializationBuffer& buffer)
{
	if (!AVDECC_ASSERT_WITH_RET(buffer.remaining() >= HeaderLength, "EtherLayer2::deserialize error: Not enough data in buffer"))
	{
		LOG_SERIALIZATION_ERROR(_srcAddress, "EtherLayer2::deserialize error: Not enough data in buffer");
		throw std::invalid_argument("Not enough data to deserialize");
	}
	buffer.unpackBuffer(_destAddress.data(), _destAddress.size());
	buffer.unpackBuffer(_srcAddress.data(), _srcAddress.size());
	buffer >> _etherType;
}

/***********************************************************/
/* Avtpdu class definition                                 */
/***********************************************************/

Avtpdu::Avtpdu() noexcept
{
	setEtherType(AvtpEtherType);
}

/***********************************************************/
/* AvtpduControl class definition                          */
/***********************************************************/

AvtpduControl::AvtpduControl() noexcept
{
	Avtpdu::setCD(true);
	Avtpdu::setVersion(AvtpVersion);
}

void AvtpduControl::serialize(SerializationBuffer& buffer) const
{
#ifdef DEBUG
	auto const previousSize = buffer.size();
#endif // DEBUG

	buffer << static_cast<std::uint8_t>(((_cd << 7) & 0x80) | (_subType & 0x7f));
	buffer << static_cast<std::uint8_t>(((_headerSpecific << 7) & 0x80) | ((_version << 4) & 0x70) | (_controlData & 0x0f));
	buffer << static_cast<std::uint16_t>(((_status << 11) & 0xf800) | (_controlDataLength & 0x7ff));
	buffer << _streamID;

#ifdef DEBUG
	AVDECC_ASSERT((buffer.size() - previousSize) == HeaderLength, "AvtpduControl::serialize error: Packed buffer length != expected header length");
#endif // DEBUG
}

void AvtpduControl::deserialize(DeserializationBuffer& buffer)
{
	if (!AVDECC_ASSERT_WITH_RET(buffer.remaining() >= HeaderLength, "EtherLayer2::deserialize error: Not enough data in buffer"))
	{
		LOG_SERIALIZATION_ERROR(_srcAddress, "AvtpduControl::deserialize error: Not enough data in buffer");
		throw std::invalid_argument("Not enough data to deserialize");
	}

	std::uint8_t cd_subType;
	std::uint8_t hs_vers_cd;
	std::uint16_t st_cdl;

	buffer >> cd_subType >> hs_vers_cd >> st_cdl >> _streamID;

	_cd = ((cd_subType & 0x80) >> 7) != 0;
	_subType = cd_subType & 0x7f;
	_headerSpecific = ((hs_vers_cd & 0x80) >> 7) != 0;
	_version = (hs_vers_cd & 0x70) >> 4;
	_controlData = hs_vers_cd & 0x0f;
	_status = (st_cdl & 0xf800) >> 11;
	_controlDataLength = st_cdl & 0x7ff;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
