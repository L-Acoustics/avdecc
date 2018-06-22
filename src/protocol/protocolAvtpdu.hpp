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
* @file protocolAvtpdu.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/networkInterfaceHelper.hpp"
#include "la/avdecc/internals/entity.hpp"
#include "la/avdecc/internals/protocolDefines.hpp"
#include "serialization.hpp"
#include <cstdint>
#include <utility>
#include <array>
#include <memory>

namespace la
{
namespace avdecc
{
namespace protocol
{

/** Ethernet frame payload minimum size */
static constexpr size_t EthernetPayloadMinimumSize = 46;

/** Serialization buffer */
using SerializationBuffer = la::avdecc::Serializer<EthernetMaxFrameSize>;
static_assert(SerializationBuffer::maximum_size >= EthernetPayloadMinimumSize, "Ethernet serialization buffer must contain at least 46 bytes (minimum ethernet frame payload size)");
/** Deserialization buffer */
using DeserializationBuffer = la::avdecc::Deserializer;

/** Ethernet layer 2 header */
class EtherLayer2
{
public:
	static constexpr size_t HeaderLength = 14; /* DestMacAddress + SrcMacAddress + EtherType */
	using UniquePointer = std::unique_ptr<EtherLayer2, void(*)(EtherLayer2*)>;

	/** Constructor */
	EtherLayer2() noexcept = default;

	/** Destructor */
	virtual ~EtherLayer2() noexcept = default;

	// Setters
	void setDestAddress(networkInterface::MacAddress const& destAddress) noexcept
	{
		_destAddress = destAddress;
	}
	void setSrcAddress(networkInterface::MacAddress const& srcAddress) noexcept
	{
		_srcAddress = srcAddress;
	}
	void setEtherType(std::uint16_t const etherType) noexcept
	{
		_etherType = etherType;
	}

	// Getters
	networkInterface::MacAddress getDestAddress() const noexcept
	{
		return _destAddress;
	}
	networkInterface::MacAddress getSrcAddress() const noexcept
	{
		return _srcAddress;
	}
	std::uint16_t getEtherType() const noexcept
	{
		return _etherType;
	}

	// Serialization method
	void serialize(SerializationBuffer& buffer) const;

	// Deserialization method
	void deserialize(DeserializationBuffer& buffer);

	// Defaulted compiler auto-generated methods
	EtherLayer2(EtherLayer2&&) = default;
	EtherLayer2(EtherLayer2 const&) = default;
	EtherLayer2& operator=(EtherLayer2 const&) = default;
	EtherLayer2& operator=(EtherLayer2&&) = default;

protected:
	// Ethernet Layer 2 header data
	networkInterface::MacAddress _destAddress{};
	networkInterface::MacAddress _srcAddress{};
	std::uint16_t _etherType{};
};

/** Avtpdu common header */
class Avtpdu : public EtherLayer2
{
public:
	/** Constructor */
	Avtpdu() noexcept;

	/** Destructor */
	virtual ~Avtpdu() noexcept override = default;

	// Setters
	void setCD(bool const cd) noexcept
	{
		_cd = cd;
	}
	void setSubType(std::uint8_t const subType) noexcept
	{
		_subType = subType;
	}
	void setHeaderSpecific(bool const headerSpecific) noexcept
	{
		_headerSpecific = headerSpecific;
	}
	void setVersion(std::uint8_t const version) noexcept
	{
		_version = version;
	}

	// Getters
	bool getCD() const noexcept
	{
		return _cd;
	}
	std::uint8_t getSubType() const noexcept
	{
		return _subType;
	}
	bool getHeaderSpecific() const noexcept
	{
		return _headerSpecific;
	}
	std::uint8_t getVersion() const noexcept
	{
		return _version;
	}

	// Defaulted compiler auto-generated methods
	Avtpdu(Avtpdu&&) = default;
	Avtpdu(Avtpdu const&) = default;
	Avtpdu& operator=(Avtpdu const&) = default;
	Avtpdu& operator=(Avtpdu&&) = default;

protected:
	// Avtpdu header data
	bool _cd{};
	std::uint8_t _subType{};
	bool _headerSpecific{};
	std::uint8_t _version{};

private:
	// Hide EtherLayer2 const data
	using EtherLayer2::setEtherType;
	using EtherLayer2::getEtherType;
};

/** Avtpdu common control header */
class AvtpduControl : public Avtpdu
{
public:
	static constexpr size_t HeaderLength = 12; /* CD + SubType + StreamValid + Version + ControlData + Status + ControlDataLength + StreamID */

	/** Constructor */
	AvtpduControl() noexcept;

	/**  Destructor */
	virtual ~AvtpduControl() noexcept override = default;

	// Setters
	void setStreamValid(bool const streamValid) noexcept
	{
		Avtpdu::setHeaderSpecific(streamValid);
	}
	void setControlData(std::uint8_t const controlData) noexcept
	{
		_controlData = controlData;
	}
	void setStatus(std::uint8_t const status) noexcept
	{
		_status = status;
	}
	void setControlDataLength(std::uint16_t controlDataLength) noexcept
	{
		_controlDataLength = controlDataLength;
	}
	void setStreamID(std::uint64_t const streamID) noexcept
	{
		_streamID = streamID;
	}

	// Getters
	bool getStreamValid() const noexcept
	{
		return Avtpdu::getHeaderSpecific();
	}
	std::uint8_t getControlData() const noexcept
	{
		return _controlData;
	}
	std::uint8_t getStatus() const noexcept
	{
		return _status;
	}
	std::uint16_t getControlDataLength() const noexcept
	{
		return _controlDataLength;
	}
	std::uint64_t getStreamID() const noexcept
	{
		return _streamID;
	}

	// Serialization method
	void serialize(SerializationBuffer& buffer) const;

	// Deserialization method
	void deserialize(DeserializationBuffer& buffer);

	// Defaulted compiler auto-generated methods
	AvtpduControl(AvtpduControl&&) = default;
	AvtpduControl(AvtpduControl const&) = default;
	AvtpduControl& operator=(AvtpduControl const&) = default;
	AvtpduControl& operator=(AvtpduControl&&) = default;

protected:
	// AvtpduControl header data
	std::uint8_t _controlData{ 0 };
	std::uint8_t _status{ 0 };
	std::uint16_t _controlDataLength{ 0 };
	std::uint64_t _streamID{};

private:
	// Hide renamed Avtpdu data
	using Avtpdu::setHeaderSpecific;
	using Avtpdu::getHeaderSpecific;
	// Hide Avtpdu const data
	using Avtpdu::setCD;
	using Avtpdu::setVersion;
	using Avtpdu::getCD;
	using Avtpdu::getVersion;
};

/** Serialization helper template */
template<class FrameType, typename... Ts>
void serialize(EtherLayer2 const& frame, Ts&&... params)
{
	static_cast<FrameType const&>(frame).serialize(std::forward<Ts>(params)...);
}

/** Deserialization helper template */
template<class FrameType, typename... Ts>
void deserialize(EtherLayer2* const frame, Ts&&... params)
{
	static_cast<FrameType*>(frame)->deserialize(std::forward<Ts>(params)...);
}

} // namespace protocol
} // namespace avdecc
} // namespace la
