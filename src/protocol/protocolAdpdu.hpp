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
* @file protocolAdpdu.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "protocolAvtpdu.hpp"
#include <memory>

namespace la
{
namespace avdecc
{
namespace protocol
{

/** Adpdu common header */
class Adpdu final : public AvtpduControl
{
public:
	static constexpr size_t Length = 56; /* ADPDU size - Clause 6.2.1.7 */
	using UniquePointer = std::unique_ptr<Adpdu, void(*)(Adpdu*)>;
	static la::avdecc::networkInterface::MacAddress Multicast_Mac_Address;

	/**
	* @brief Factory method to create a new Adpdu.
	* @details Creates a new Adpdu as a unique pointer.
	* @return A new Adpdu as a Adpdu::UniquePointer.
	*/
	static UniquePointer create()
	{
		auto deleter = [](Adpdu* self)
		{
			self->destroy();
		};
		return UniquePointer(createRawAdpdu(), deleter);
	}

	/** Constructor */
	Adpdu() noexcept;

	/** Destructor */
	virtual ~Adpdu() noexcept override = default;

	// Setters
	void setMessageType(AdpMessageType const messageType) noexcept
	{
		AvtpduControl::setControlData(messageType.getValue());
	}
	void setValidTime(std::uint8_t const validTime) noexcept
	{
		AvtpduControl::setStatus(validTime);
	}
	void setEntityID(UniqueIdentifier const entityID) noexcept
	{
		AvtpduControl::setStreamID(entityID);
	}
	void setEntityModelID(UniqueIdentifier const entityModelID) noexcept
	{
		_entityModelID = entityModelID;
	}
	void setEntityCapabilities(entity::EntityCapabilities const entityCapabilities)
	{
		_entityCapabilities = entityCapabilities;
	}
	void setTalkerStreamSources(std::uint16_t const talkerStreamSources)
	{
		_talkerStreamSources = talkerStreamSources;
	}
	void setTalkerCapabilities(entity::TalkerCapabilities const talkerCapabilities)
	{
		_talkerCapabilities = talkerCapabilities;
	}
	void setListenerStreamSinks(std::uint16_t const listenerStreamSinks)
	{
		_listenerStreamSinks = listenerStreamSinks;
	}
	void setListenerCapabilities(entity::ListenerCapabilities const listenerCapabilities)
	{
		_listenerCapabilities = listenerCapabilities;
	}
	void setControllerCapabilities(entity::ControllerCapabilities const controllerCapabilities)
	{
		_controllerCapabilities = controllerCapabilities;
	}
	void setAvailableIndex(std::uint32_t const availableIndex)
	{
		_availableIndex = availableIndex;
	}
	void setGptpGrandmasterID(UniqueIdentifier const gptpGrandmasterID)
	{
		_gptpGrandmasterID = gptpGrandmasterID;
	}
	void setGptpDomainNumber(std::uint8_t const gptpDomainNumber)
	{
		_gptpDomainNumber = gptpDomainNumber;
	}
	void setIdentifyControlIndex(std::uint16_t const identifyControlIndex)
	{
		_identifyControlIndex = identifyControlIndex;
	}
	void setInterfaceIndex(std::uint16_t const interfaceIndex)
	{
		_interfaceIndex = interfaceIndex;
	}
	void setAssociationID(UniqueIdentifier const associationID)
	{
		_associationID = associationID;
	}

	// Getters
	AdpMessageType getMessageType() const noexcept
	{
		return AdpMessageType(AvtpduControl::getControlData());
	}
	std::uint8_t getValidTime() const noexcept
	{
		return AvtpduControl::getStatus();
	}
	UniqueIdentifier getEntityID() const noexcept
	{
		return AvtpduControl::getStreamID();
	}
	UniqueIdentifier getEntityModelID() const noexcept
	{
		return _entityModelID;
	}
	entity::EntityCapabilities getEntityCapabilities() const noexcept
	{
		return _entityCapabilities;
	}
	std::uint16_t getTalkerStreamSources() const noexcept
	{
		return _talkerStreamSources;
	}
	entity::TalkerCapabilities getTalkerCapabilities() const noexcept
	{
		return _talkerCapabilities;
	}
	std::uint16_t getListenerStreamSinks() const noexcept
	{
		return _listenerStreamSinks;
	}
	entity::ListenerCapabilities getListenerCapabilities() const noexcept
	{
		return _listenerCapabilities;
	}
	entity::ControllerCapabilities getControllerCapabilities() const noexcept
	{
		return _controllerCapabilities;
	}
	std::uint32_t getAvailableIndex() const noexcept
	{
		return _availableIndex;
	}
	UniqueIdentifier getGptpGrandmasterID() const noexcept
	{
		return _gptpGrandmasterID;
	}
	std::uint8_t getGptpDomainNumber() const noexcept
	{
		return _gptpDomainNumber;
	}
	std::uint16_t getIdentifyControlIndex() const noexcept
	{
		return _identifyControlIndex;
	}
	std::uint16_t getInterfaceIndex() const noexcept
	{
		return _interfaceIndex;
	}
	UniqueIdentifier getAssociationID() const noexcept
	{
		return _associationID;
	}

	/** Serialization method */
	void serialize(SerializationBuffer& buffer) const;

	// Deserialization method
	void deserialize(DeserializationBuffer& buffer);

	// Defaulted compiler auto-generated methods
	Adpdu(Adpdu&&) = default;
	Adpdu(Adpdu const&) = default;
	Adpdu& operator=(Adpdu const&) = default;
	Adpdu& operator=(Adpdu&&) = default;

private:
	/** Entry point */
	static Adpdu* createRawAdpdu();

	/** Destroy method for COM-like interface */
	void destroy() noexcept;

	// Adpdu header data
	UniqueIdentifier _entityModelID{ getNullIdentifier() };
	entity::EntityCapabilities _entityCapabilities{ entity::EntityCapabilities::None };
	std::uint16_t _talkerStreamSources{};
	entity::TalkerCapabilities _talkerCapabilities{ entity::TalkerCapabilities::None };
	std::uint16_t _listenerStreamSinks{};
	entity::ListenerCapabilities _listenerCapabilities{ entity::ListenerCapabilities::None };
	entity::ControllerCapabilities _controllerCapabilities{ entity::ControllerCapabilities::None };
	std::uint32_t _availableIndex{};
	UniqueIdentifier _gptpGrandmasterID{ getNullIdentifier() };
	std::uint8_t _gptpDomainNumber{0};
	// Reserved 24bits
	std::uint16_t _identifyControlIndex{ 0 };
	std::uint16_t _interfaceIndex{ 0 };
	UniqueIdentifier _associationID{ getNullIdentifier() };
	// Reserved 32bits

private:
	// Hide renamed AvtpduControl data
	using AvtpduControl::setControlData;
	using AvtpduControl::setControlDataLength;
	using AvtpduControl::setStatus;
	using AvtpduControl::setStreamID;
	using AvtpduControl::getControlData;
	using AvtpduControl::getControlDataLength;
	using AvtpduControl::getStatus;
	using AvtpduControl::getStreamID;
	// Hide Avtpdu const data
	using Avtpdu::setSubType;
	using Avtpdu::getSubType;
	// Hide AvtpduControl const data
	using AvtpduControl::setStreamValid;
	using AvtpduControl::getStreamValid;
};

} // namespace protocol
} // namespace avdecc
} // namespace la
