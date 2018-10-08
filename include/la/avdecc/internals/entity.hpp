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
* @file entity.hpp
* @author Christophe Calmejane
* @brief Avdecc entities.
*/

#pragma once

#include "la/avdecc/networkInterfaceHelper.hpp"
#include "entityEnums.hpp"
#include <cstdint>
#include <thread>
#include <algorithm>

namespace la
{
namespace avdecc
{
namespace entity
{
/** An entity */
class Entity
{
public:
	using UniquePointer = std::unique_ptr<Entity>;

	/** Gets the unique identifier computed for this entity */
	UniqueIdentifier getEntityID() const noexcept
	{
		return _entityID;
	}

	/** Gets the entity's mac address */
	la::avdecc::networkInterface::MacAddress getMacAddress() const noexcept
	{
		return _macAddress;
	}

	/** Gets the valid time value */
	std::uint8_t getValidTime() const noexcept
	{
		return _validTime;
	}

	/** Gets the entity model ID */
	UniqueIdentifier getEntityModelID() const noexcept
	{
		return _entityModelID;
	}

	/** Gets the entity capabilities */
	EntityCapabilities getEntityCapabilities() const noexcept
	{
		return _entityCapabilities;
	}

	/** Gets the talker stream sources */
	std::uint16_t getTalkerStreamSources() const noexcept
	{
		return _talkerStreamSources;
	}

	/** Gets the talker capabilities */
	TalkerCapabilities getTalkerCapabilities() const noexcept
	{
		return _talkerCapabilities;
	}

	/** Gets the listener stream sinks */
	std::uint16_t getListenerStreamSinks() const noexcept
	{
		return _listenerStreamSinks;
	}

	/** Gets the listener capabilities */
	ListenerCapabilities getListenerCapabilities() const noexcept
	{
		return _listenerCapabilities;
	}

	/** Gets the controller capabilities */
	ControllerCapabilities getControllerCapabilities() const noexcept
	{
		return _controllerCapabilities;
	}

	/** Gets the available index value */
	std::uint32_t getAvailableIndex() const noexcept
	{
		return _availableIndex;
	}

	/** Gets the gptp grandmaster unique identifier */
	UniqueIdentifier getGptpGrandmasterID() const noexcept
	{
		return _gptpGrandmasterID;
	}

	/** Gets the gptp domain number */
	std::uint8_t getGptpDomainNumber() const noexcept
	{
		return _gptpDomainNumber;
	}

	/** Gets the identify control index */
	std::uint16_t getIdentifyControlIndex() const noexcept
	{
		return _identifyControlIndex;
	}

	/** Gets the interface index */
	std::uint16_t getInterfaceIndex() const noexcept
	{
		return _interfaceIndex;
	}

	/** Gets the association unique identifier */
	UniqueIdentifier getAssociationID() const noexcept
	{
		return _associationID;
	}

	/** Gets next available index value */
	virtual std::uint32_t getNextAvailableIndex() noexcept
	{
		return ++_availableIndex;
	}

	/** Constructor using fields that are not allowed to change after creation */
	Entity(UniqueIdentifier const entityID, networkInterface::MacAddress const& macAddress, UniqueIdentifier const entityModelID, EntityCapabilities const entityCapabilities, std::uint16_t const talkerStreamSources, TalkerCapabilities const talkerCapabilities, std::uint16_t const listenerStreamSinks, ListenerCapabilities const listenerCapabilities, ControllerCapabilities const controllerCapabilities, std::uint16_t const identifyControlIndex, std::uint16_t const interfaceIndex, UniqueIdentifier const associationID) noexcept
		: _entityID(entityID)
		, _macAddress(macAddress)
		, _entityModelID(entityModelID)
		, _entityCapabilities(entityCapabilities)
		, _talkerStreamSources(talkerStreamSources)
		, _talkerCapabilities(talkerCapabilities)
		, _listenerStreamSinks(listenerStreamSinks)
		, _listenerCapabilities(listenerCapabilities)
		, _controllerCapabilities(controllerCapabilities)
		, _identifyControlIndex(identifyControlIndex)
		, _interfaceIndex(interfaceIndex)
		, _associationID(associationID)
	{
	}

	/** Constructor using all fields */
	Entity(UniqueIdentifier const entityID, networkInterface::MacAddress const& macAddress, std::uint8_t const validTime, UniqueIdentifier const entityModelID, EntityCapabilities const entityCapabilities, std::uint16_t const talkerStreamSources, TalkerCapabilities const talkerCapabilities, std::uint16_t const listenerStreamSinks, ListenerCapabilities const listenerCapabilities, ControllerCapabilities const controllerCapabilities, std::uint32_t const availableIndex, UniqueIdentifier const gptpGrandmasterID, std::uint8_t const gptpDomainNumber, std::uint16_t const identifyControlIndex, std::uint16_t const interfaceIndex, UniqueIdentifier const associationID) noexcept
		: _entityID(entityID)
		, _macAddress(macAddress)
		, _validTime(validTime)
		, _entityModelID(entityModelID)
		, _entityCapabilities(entityCapabilities)
		, _talkerStreamSources(talkerStreamSources)
		, _talkerCapabilities(talkerCapabilities)
		, _listenerStreamSinks(listenerStreamSinks)
		, _listenerCapabilities(listenerCapabilities)
		, _controllerCapabilities(controllerCapabilities)
		, _availableIndex(availableIndex)
		, _gptpGrandmasterID(gptpGrandmasterID)
		, _gptpDomainNumber(gptpDomainNumber)
		, _identifyControlIndex(identifyControlIndex)
		, _interfaceIndex(interfaceIndex)
		, _associationID(associationID)
	{
	}

	// Defaulted compiler auto-generated methods
	virtual ~Entity() noexcept = default;
	Entity(Entity&&) = default;
	Entity(Entity const&) = default;
	Entity& operator=(Entity const&) = default;
	Entity& operator=(Entity&&) = default;

protected:
	/** Sets the valid time value */
	virtual void setValidTime(std::uint8_t const validTime) noexcept
	{
		constexpr std::uint8_t minValidTime = 1;
		constexpr std::uint8_t maxValidTime = 31;

		AVDECC_ASSERT(validTime >= 1 && validTime <= 31, "setValidTime: Invalid validTime value (must be comprised btw 1 and 31 inclusive)");
		_validTime = std::min(maxValidTime, std::max(minValidTime, validTime));
	}

	/** Sets the entity capabilities */
	virtual void setEntityCapabilities(EntityCapabilities const entityCapabilities) noexcept
	{
		_entityCapabilities = entityCapabilities;
	}

	/** Sets the gptp grandmaster unique identifier */
	virtual void setGptpGrandmasterID(UniqueIdentifier const gptpGrandmasterID) noexcept
	{
		_gptpGrandmasterID = gptpGrandmasterID;
	}

	/** Sets the gptp domain number */
	virtual void setGptpDomainNumber(std::uint8_t const gptpDomainNumber) noexcept
	{
		_gptpDomainNumber = gptpDomainNumber;
	}

	/** Sets the association unique identifier */
	virtual void setAssociationID(UniqueIdentifier const associationID) noexcept
	{
		_associationID = associationID;
	}

private:
	UniqueIdentifier _entityID{};
	networkInterface::MacAddress _macAddress{};
	std::uint8_t _validTime{ 31 }; // Default protocol value is 31 (meaning 62 seconds)
	UniqueIdentifier _entityModelID{};
	EntityCapabilities _entityCapabilities{ EntityCapabilities::None };
	std::uint16_t _talkerStreamSources{ 0u };
	TalkerCapabilities _talkerCapabilities{ TalkerCapabilities::None };
	std::uint16_t _listenerStreamSinks{ 0u };
	ListenerCapabilities _listenerCapabilities{ ListenerCapabilities::None };
	ControllerCapabilities _controllerCapabilities{ ControllerCapabilities::None };
	std::uint32_t _availableIndex{ 0 };
	UniqueIdentifier _gptpGrandmasterID{};
	std::uint8_t _gptpDomainNumber{ 0 };
	std::uint16_t _identifyControlIndex{ 0 };
	std::uint16_t _interfaceIndex{ 0 };
	UniqueIdentifier _associationID{};
};

/** Virtual interface for a local entity (on the same computer) */
class LocalEntity : public Entity
{
public:
	/** Status code returned by all AEM (AECP) command methods. */
	enum class AemCommandStatus : std::uint16_t
	{
		// AVDECC Protocol Error Codes
		Success = 0,
		NotImplemented = 1,
		NoSuchDescriptor = 2,
		LockedByOther = 3,
		AcquiredByOther = 4,
		NotAuthenticated = 5,
		AuthenticationDisabled = 6,
		BadArguments = 7,
		NoResources = 8,
		InProgress = 9,
		EntityMisbehaving = 10,
		NotSupported = 11,
		StreamIsRunning = 12,
		// Library Error Codes
		NetworkError = 995,
		ProtocolError = 996,
		TimedOut = 997,
		UnknownEntity = 998,
		InternalError = 999,
	};

	/** Status code returned by all AA (AECP) command methods. */
	enum class AaCommandStatus : std::uint16_t
	{
		// AVDECC Protocol Error Codes
		Success = 0,
		NotImplemented = 1,
		AddressTooLow = 2,
		AddressTooHigh = 3,
		AddressInvalid = 4,
		TlvInvalid = 5,
		DataInvalid = 6,
		Unsupported = 7,
		// Library Error Codes
		Aborted = 994,
		NetworkError = 995,
		ProtocolError = 996,
		TimedOut = 997,
		UnknownEntity = 998,
		InternalError = 999,
	};

	/** Status code returned by all MVU (AECP) command methods. */
	enum class MvuCommandStatus : std::uint16_t
	{
		// Milan Vendor Unique Protocol Error Codes
		Success = 0,
		NotImplemented = 1,
		BadArguments = 2,
		// Library Error Codes
		NetworkError = 995,
		ProtocolError = 996,
		TimedOut = 997,
		UnknownEntity = 998,
		InternalError = 999,
	};

	/** Status code returned by all ACMP control methods. */
	enum class ControlStatus : std::uint16_t
	{
		// AVDECC Protocol Error Codes
		Success = 0,
		ListenerUnknownID = 1, /**< Listener does not have the specified unique identifier. */
		TalkerUnknownID = 2, /**< Talker does not have the specified unique identifier. */
		TalkerDestMacFail = 3, /**< Talker could not allocate a destination MAC for the Stream. */
		TalkerNoStreamIndex = 4, /**< Talker does not have an available Stream index for the Stream. */
		TalkerNoBandwidth = 5, /**< Talker could not allocate bandwidth for the Stream. */
		TalkerExclusive = 6, /**< Talker already has an established Stream and only supports one Listener. */
		ListenerTalkerTimeout = 7, /**< Listener had timeout for all retries when trying to send command to Talker. */
		ListenerExclusive = 8, /**< The AVDECC Listener already has an established connection to a Stream. */
		StateUnavailable = 9, /**< Could not get the state from the AVDECC Entity. */
		NotConnected = 10, /**< Trying to disconnect when not connected or not connected to the AVDECC Talker specified. */
		NoSuchConnection = 11, /**< Trying to obtain connection info for an AVDECC Talker connection which does not exist. */
		CouldNotSendMessage = 12, /**< The AVDECC Listener failed to send the message to the AVDECC Talker. */
		TalkerMisbehaving = 13, /**< Talker was unable to complete the command because an internal error occurred. */
		ListenerMisbehaving = 14, /**< Listener was unable to complete the command because an internal error occurred. */
		// Reserved
		ControllerNotAuthorized = 16, /**< The AVDECC Controller with the specified Entity ID is not authorized to change Stream connections. */
		IncompatibleRequest = 17, /**< The AVDECC Listener is trying to connect to an AVDECC Talker that is already streaming with a different traffic class, etc. or does not support the requested traffic class. */
		// Reserved
		NotSupported = 31, /**< The command is not supported. */
		// Library Error Codes
		NetworkError = 995, /**< A network error occured. */
		ProtocolError = 996, /**< A protocol error occured. */
		TimedOut = 997, /**< Command timed out. */
		UnknownEntity = 998, /**< Entity is unknown. */
		InternalError = 999, /**< Internal library error. */
	};

	/** Enables entity advertising with available duration included between 2-62 seconds, defaulting to 62. Returns false if EntityID is already in use on the local computer, true otherwise. */
	virtual bool enableEntityAdvertising(std::uint32_t const availableDuration) noexcept = 0;

	/** Disables entity advertising. */
	virtual void disableEntityAdvertising() noexcept = 0;

	/** Gets the dirty state of the entity. If true, then it should be announced again using ENTITY_AVAILABLE message. The state is reset once retrieved. */
	virtual bool isDirty() noexcept = 0;

	/** BasicLockable concept lock method */
	virtual void lock() noexcept = 0;

	/** BasicLockable concept unlock method */
	virtual void unlock() noexcept = 0;

	/* Utility methods */
	static LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION statusToString(AemCommandStatus const status);
	static LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION statusToString(AaCommandStatus const status);
	static LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION statusToString(MvuCommandStatus const status);
	static LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION statusToString(ControlStatus const status);

protected:
	LocalEntity(UniqueIdentifier const entityID, networkInterface::MacAddress const& macAddress, UniqueIdentifier const entityModelID, EntityCapabilities const entityCapabilities, std::uint16_t const talkerStreamSources, TalkerCapabilities const talkerCapabilities, std::uint16_t const listenerStreamSinks, ListenerCapabilities const listenerCapabilities, ControllerCapabilities const controllerCapabilities, std::uint16_t const identifyControlIndex, std::uint16_t const interfaceIndex, UniqueIdentifier const associationID) noexcept
		: Entity(entityID, macAddress, entityModelID, entityCapabilities, talkerStreamSources, talkerCapabilities, listenerStreamSinks, listenerCapabilities, controllerCapabilities, identifyControlIndex, interfaceIndex, associationID)
	{
	}
	virtual ~LocalEntity() noexcept = default;
};

/* Operator overloads */
constexpr bool operator!(LocalEntity::AemCommandStatus const status)
{
	return status != LocalEntity::AemCommandStatus::Success;
}
constexpr LocalEntity::AemCommandStatus operator|(LocalEntity::AemCommandStatus const lhs, LocalEntity::AemCommandStatus const rhs)
{
	if (!!lhs)
		return rhs;
	return lhs;
}
constexpr LocalEntity::AemCommandStatus& operator|=(LocalEntity::AemCommandStatus& lhs, LocalEntity::AemCommandStatus const rhs)
{
	if (!!lhs)
		lhs = rhs;
	return lhs;
}
constexpr bool operator!(LocalEntity::AaCommandStatus const status)
{
	return status != LocalEntity::AaCommandStatus::Success;
}
constexpr LocalEntity::AaCommandStatus operator|(LocalEntity::AaCommandStatus const lhs, LocalEntity::AaCommandStatus const rhs)
{
	if (!!lhs)
		return rhs;
	return lhs;
}
constexpr LocalEntity::AaCommandStatus& operator|=(LocalEntity::AaCommandStatus& lhs, LocalEntity::AaCommandStatus const rhs)
{
	if (!!lhs)
		lhs = rhs;
	return lhs;
}
constexpr bool operator!(LocalEntity::MvuCommandStatus const status)
{
	return status != LocalEntity::MvuCommandStatus::Success;
}
constexpr LocalEntity::MvuCommandStatus operator|(LocalEntity::MvuCommandStatus const lhs, LocalEntity::MvuCommandStatus const rhs)
{
	if (!!lhs)
		return rhs;
	return lhs;
}
constexpr LocalEntity::MvuCommandStatus& operator|=(LocalEntity::MvuCommandStatus& lhs, LocalEntity::MvuCommandStatus const rhs)
{
	if (!!lhs)
		lhs = rhs;
	return lhs;
}
constexpr bool operator!(LocalEntity::ControlStatus const status)
{
	return status != LocalEntity::ControlStatus::Success;
}
constexpr LocalEntity::ControlStatus& operator|=(LocalEntity::ControlStatus& lhs, LocalEntity::ControlStatus const rhs)
{
	if (!!lhs)
		lhs = rhs;
	return lhs;
}
constexpr LocalEntity::ControlStatus operator|=(LocalEntity::ControlStatus const lhs, LocalEntity::ControlStatus const rhs)
{
	if (!!lhs)
		return rhs;
	return lhs;
}

} // namespace entity
} // namespace avdecc
} // namespace la
