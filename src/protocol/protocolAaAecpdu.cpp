/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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
* @file protocolAaAecpdu.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/protocolAaAecpdu.hpp"
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
/* AaAecpdu class definition                              */
/***********************************************************/

AaAecpdu::AaAecpdu() noexcept
{
	Aecpdu::setAecpSpecificDataLength(AaAecpdu::HeaderLength);
}

AaAecpdu::~AaAecpdu() noexcept {}

void LA_AVDECC_CALL_CONVENTION AaAecpdu::serialize(SerializationBuffer& buffer) const
{
	// First call parent
	Aecpdu::serialize(buffer);

	auto const previousSize = buffer.size();

	auto const tlvCount{ static_cast<std::uint16_t>(_tlvData.size()) };
	buffer << tlvCount;

	for (auto const& tlv : _tlvData)
	{
		buffer << static_cast<std::uint16_t>(((tlv.getMode().getValue() << 12) & 0xf000) | (tlv.size() & 0x0fff)) << tlv.getAddress();
		buffer.packBuffer(tlv.data(), tlv.size());
	}

	if (!AVDECC_ASSERT_WITH_RET((buffer.size() - previousSize) == (HeaderLength + _tlvDataLength), "AaAecpdu::serialize error: Packed buffer length != expected header length"))
	{
		LOG_SERIALIZATION_ERROR(_destAddress, "AaAecpdu::serialize error: Packed buffer length != expected header length");
	}
}

void LA_AVDECC_CALL_CONVENTION AaAecpdu::deserialize(DeserializationBuffer& buffer)
{
	auto const beginRemainingBytes = buffer.remaining();

	// First call parent
	Aecpdu::deserialize(buffer);

	// Check if there is enough bytes to read the header
	if (!AVDECC_ASSERT_WITH_RET(buffer.remaining() >= HeaderLength, "AaAecpdu::deserialize error: Not enough data in buffer"))
	{
		LOG_SERIALIZATION_ERROR(_srcAddress, "AaAecpdu::deserialize error: Not enough data in buffer");
		throw std::invalid_argument("Not enough data to deserialize");
	}

	// Check if there is more advertised data than actual bytes in the buffer
	if (_controlDataLength > beginRemainingBytes)
	{
#if defined(IGNORE_INVALID_CONTROL_DATA_LENGTH)
		// Allow this packet to go through, the ControlData specific unpacker will trap any error if the message is further ill-formed
		LOG_SERIALIZATION_DEBUG(_srcAddress, "AaAecpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer, but trying to unpack the message");
#else // !IGNORE_INVALID_CONTROL_DATA_LENGTH
		LOG_SERIALIZATION_WARN(_srcAddress, "AaAecpdu::deserialize error: ControlDataLength field advertises more bytes than remaining bytes in buffer, ignoring the message");
		throw std::invalid_argument("Not enough data to deserialize");
#endif // IGNORE_INVALID_CONTROL_DATA_LENGTH
	}

	std::uint16_t tlvCount;

	buffer >> tlvCount;

	for (auto i = 0u; i < tlvCount; ++i)
	{
		std::uint16_t mode_length;
		std::uint64_t address;

		buffer >> mode_length >> address;

		auto const mode{ static_cast<AaMode>((mode_length & 0xf000) >> 12) };
		auto const length{ static_cast<std::uint16_t>(mode_length & 0x0fff) };
		auto tlv{ entity::addressAccess::Tlv{ mode, address, length } };

		buffer.unpackBuffer(tlv.data(), length);

		_tlvData.push_back(std::move(tlv));
		_tlvDataLength += TlvHeaderLength + length;
	}

#ifdef DEBUG
	// Do not log this error in release, it might happen too often if an entity is bugged or if the message contains data this version of the library do not unpack
	if (buffer.remaining() != 0 && buffer.usedBytes() >= EthernetPayloadMinimumSize)
		LOG_SERIALIZATION_TRACE(_srcAddress, "AaAecpdu::deserialize warning: Remaining bytes in buffer");
#endif // DEBUG
}

/** Copy method */
Aecpdu::UniquePointer LA_AVDECC_CALL_CONVENTION AaAecpdu::copy() const
{
	auto deleter = [](Aecpdu* self)
	{
		static_cast<AaAecpdu*>(self)->destroy();
	};
	return UniquePointer(new AaAecpdu(*this), deleter);
}

/** Entry point */
AaAecpdu* LA_AVDECC_CALL_CONVENTION AaAecpdu::createRawAaAecpdu()
{
	return new AaAecpdu();
}

/** Destroy method for COM-like interface */
void LA_AVDECC_CALL_CONVENTION AaAecpdu::destroy() noexcept
{
	delete this;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
