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
#include <optional>
#include <map>

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
	/** Entity common information, identical for all network interfaces. */
	struct CommonInformation
	{
		UniqueIdentifier entityID{ la::avdecc::UniqueIdentifier::getNullUniqueIdentifier() }; /** The entity's unique identifier. */
		UniqueIdentifier entityModelID{ la::avdecc::UniqueIdentifier::getNullUniqueIdentifier() }; /** The EntityModel unique identifier. */
		EntityCapabilities entityCapabilities{ EntityCapabilities::None }; /** The entity's current capabilities (can change over time). */
		std::uint16_t talkerStreamSources{ 0u }; /** The maximum number of streams the entity is capable of sourcing simultaneously. */
		TalkerCapabilities talkerCapabilities{ TalkerCapabilities::None }; /** The entity's capabilities as a talker. */
		std::uint16_t listenerStreamSinks{ 0u }; /** The maximum number of streams the entity is capable of sinking simultaneously. */
		ListenerCapabilities listenerCapabilities{ ListenerCapabilities::None }; /** The entity's capabilities as a listener. */
		ControllerCapabilities controllerCapabilities{ ControllerCapabilities::None }; /** The entity's capabilities as a controller. */
		std::optional<model::ControlIndex> identifyControlIndex{ std::nullopt }; /** The ControlIndex for the primary IDENTIFY control, if set. Only valid if EntityCapabilities::AemIdentifyControlIndexValid is defined. */
		std::optional<UniqueIdentifier> associationID{ std::nullopt }; /** The unique identifier of the associated entity, if set. Only valid if EntityCapabilities::AssociationIDValid is defined. */
	};

	/** Information specific to each entity's network interfaces plugged on the discovery domain (if entity does not support nor specify an AvbInterfaceIndex, GlobalAvbInterfaceIndex is used instead). */
	struct InterfaceInformation
	{
		networkInterface::MacAddress macAddress{}; /** The mac address this interface is attached to. */
		std::uint8_t validTime{ 31u }; /** The number of 2-seconds periods the entity's announcement is valid on this interface. */
		std::uint32_t availableIndex{ 0u }; /** The current available index for the entity on this interface. */
		std::optional<UniqueIdentifier> gptpGrandmasterID{ std::nullopt }; /** The current gPTP grandmaster unique identifier on this interface. Only valid if EntityCapabilities::GptpSupported is defined. */
		std::optional<std::uint8_t> gptpDomainNumber{ std::nullopt }; /** The current gPTP domain number on this interface. Only valid if EntityCapabilities::GptpSupported is defined. */
	};

	using UniquePointer = std::unique_ptr<Entity>;
	static constexpr model::AvbInterfaceIndex GlobalAvbInterfaceIndex{ 0xffff }; /** The AvbInterfaceIndex value used when EntityCapabilities::AemInterfaceIndexValid is not set */
	using InterfacesInformation = std::map<model::AvbInterfaceIndex, InterfaceInformation>;

	/** Sets (or replaces) the InterfaceInformation for the specified AvbInterfaceIndex. */
	virtual void setInterfaceInformation(InterfaceInformation const& interfaceInformation, model::AvbInterfaceIndex const interfaceIndex) noexcept
	{
		_interfaceInformation[interfaceIndex] = interfaceInformation;
	}

	/** Removes the InterfaceInformation for the specified AvbInterfaceIndex. */
	virtual void removeInterfaceInformation(model::AvbInterfaceIndex const interfaceIndex) noexcept
	{
		_interfaceInformation.erase(interfaceIndex);
	}

	/** Gets non-modifiable CommonInformation */
	CommonInformation const& getCommonInformation() const noexcept
	{
		return _commonInformation;
	}

	/** Gets modifiable CommonInformation */
	CommonInformation& getCommonInformation() noexcept
	{
		return _commonInformation;
	}

	/** Gets non-modifiable InterfaceInformation */
	InterfaceInformation const& getInterfaceInformation(model::AvbInterfaceIndex const interfaceIndex) const
	{
		auto const infoIt = _interfaceInformation.find(interfaceIndex);
		if (infoIt == _interfaceInformation.end())
		{
			throw la::avdecc::Exception("Invalid interfaceIndex");
		}

		return infoIt->second;
	}

	/** Gets modifiable InterfaceInformation */
	InterfaceInformation& getInterfaceInformation(model::AvbInterfaceIndex const interfaceIndex)
	{
		auto const infoIt = _interfaceInformation.find(interfaceIndex);
		if (infoIt == _interfaceInformation.end())
		{
			throw la::avdecc::Exception("Invalid interfaceIndex");
		}

		return infoIt->second;
	}

	/** Gets the non-modifiable InterfaceInformation for all interfaces */
	InterfacesInformation const& getInterfacesInformation() const noexcept
	{
		return _interfaceInformation;
	}

	/** Gets the modifiable InterfaceInformation for all interfaces */
	InterfacesInformation& getInterfacesInformation() noexcept
	{
		return _interfaceInformation;
	}

	/** Gets the unique identifier computed for this entity */
	UniqueIdentifier getEntityID() const noexcept
	{
		return _commonInformation.entityID;
	}

	/** Gets any mac address for the entity */
	la::avdecc::networkInterface::MacAddress getAnyMacAddress() const
	{
		// Get the first InterfaceInformation data (ordered, so we always get the same)
		auto infoIt = _interfaceInformation.begin();
		if (infoIt == _interfaceInformation.end())
		{
			throw la::avdecc::Exception("Invalid interfaceIndex");
		}

		return infoIt->second.macAddress;
	}

	/** Gets the entity model ID */
	UniqueIdentifier getEntityModelID() const noexcept
	{
		return _commonInformation.entityModelID;
	}

	/** Gets the entity capabilities */
	EntityCapabilities getEntityCapabilities() const noexcept
	{
		return _commonInformation.entityCapabilities;
	}

	/** Gets the talker stream sources */
	std::uint16_t getTalkerStreamSources() const noexcept
	{
		return _commonInformation.talkerStreamSources;
	}

	/** Gets the talker capabilities */
	TalkerCapabilities getTalkerCapabilities() const noexcept
	{
		return _commonInformation.talkerCapabilities;
	}

	/** Gets the listener stream sinks */
	std::uint16_t getListenerStreamSinks() const noexcept
	{
		return _commonInformation.listenerStreamSinks;
	}

	/** Gets the listener capabilities */
	ListenerCapabilities getListenerCapabilities() const noexcept
	{
		return _commonInformation.listenerCapabilities;
	}

	/** Gets the controller capabilities */
	ControllerCapabilities getControllerCapabilities() const noexcept
	{
		return _commonInformation.controllerCapabilities;
	}

	/** Gets the identify control index */
	std::optional<model::ControlIndex> getIdentifyControlIndex() const noexcept
	{
		return _commonInformation.identifyControlIndex;
	}

	/** Gets the association unique identifier */
	std::optional<UniqueIdentifier> getAssociationID() const noexcept
	{
		return _commonInformation.associationID;
	}

	/** Generates an EID from a MacAddress (OUI-36) and a ProgID. This method is provided for backward compatibility, ProtocolInterface::getDynamicEID */
	static UniqueIdentifier generateEID(la::avdecc::networkInterface::MacAddress const& macAddress, std::uint16_t const progID)
	{
		UniqueIdentifier::value_type eid{ 0u };
		if (macAddress.size() != 6)
			throw Exception("Invalid MAC address size");
		if (progID == 0 || progID == 0xFFFF || progID == 0xFFFE)
			throw Exception("Reserved value for Entity's progID value: " + std::to_string(progID));
		eid += macAddress[0];
		eid <<= 8;
		eid += macAddress[1];
		eid <<= 8;
		eid += macAddress[2];
		eid <<= 16;
		eid += progID;
		eid <<= 8;
		eid += macAddress[3];
		eid <<= 8;
		eid += macAddress[4];
		eid <<= 8;
		eid += macAddress[5];

		return UniqueIdentifier{ eid };
	}

	/** Constructor */
	Entity(CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation)
		: _commonInformation(commonInformation)
		, _interfaceInformation(interfacesInformation)
	{
		if (interfacesInformation.empty())
		{
			throw Exception("An Entity should at least have one InterfaceInformation");
		}
	}

	/** Sets the entity capabilities */
	virtual void setEntityCapabilities(EntityCapabilities const entityCapabilities) noexcept
	{
		_commonInformation.entityCapabilities = entityCapabilities;
	}

	/** Sets the association unique identifier */
	virtual void setAssociationID(UniqueIdentifier const associationID) noexcept
	{
		_commonInformation.associationID = associationID;
	}

	/** Sets the valid time value */
	virtual void setValidTime(std::uint8_t const validTime, std::optional<model::AvbInterfaceIndex> const interfaceIndex = std::nullopt)
	{
		constexpr std::uint8_t minValidTime = 1;
		constexpr std::uint8_t maxValidTime = 31;
		auto const value = std::min(maxValidTime, std::max(minValidTime, validTime));

		// If interfaceIndex is specified, only set the value for this interface
		if (interfaceIndex)
		{
			getInterfaceInformation(interfaceIndex.value()).validTime = value;
		}
		else
		{
			// Otherwise set the value for all interfaces on the entity
			for (auto& infoKV : getInterfacesInformation())
			{
				auto const avbInterfaceIndex = infoKV.first;
				auto& information = infoKV.second;
				information.validTime = value;
			}
		}
	}

	/** Sets the gptp grandmaster unique identifier */
	virtual void setGptpGrandmasterID(UniqueIdentifier const gptpGrandmasterID, model::AvbInterfaceIndex const interfaceIndex)
	{
		getInterfaceInformation(interfaceIndex).gptpGrandmasterID = gptpGrandmasterID;
	}

	/** Sets the gptp domain number */
	virtual void setGptpDomainNumber(std::uint8_t const gptpDomainNumber, model::AvbInterfaceIndex const interfaceIndex)
	{
		getInterfaceInformation(interfaceIndex).gptpDomainNumber = gptpDomainNumber;
	}

	// Defaulted compiler auto-generated methods
	virtual ~Entity() noexcept = default;
	Entity(Entity&&) = default;
	Entity(Entity const&) = default;
	Entity& operator=(Entity const&) = default;
	Entity& operator=(Entity&&) = default;

private:
	CommonInformation _commonInformation{};
	InterfacesInformation _interfaceInformation{};
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

	/** Enables entity advertising with available duration included between 2-62 seconds on the specified interfaceIndex if set, otherwise on all interfaces. Returns false if EntityID is already in use on the local computer, true otherwise. */
	virtual bool enableEntityAdvertising(std::uint32_t const availableDuration, std::optional<model::AvbInterfaceIndex> const interfaceIndex = std::nullopt) noexcept = 0;

	/** Disables entity advertising on the specified interfaceIndex if set, otherwise on all interfaces. */
	virtual void disableEntityAdvertising(std::optional<model::AvbInterfaceIndex> const interfaceIndex = std::nullopt) noexcept = 0;

	/** Gets the dirty state of the entity. If true, then it should be announced again using ENTITY_AVAILABLE message. The state is reset once retrieved. */
	//virtual bool isDirty() noexcept = 0;

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
	/** Constructor */
	LocalEntity(CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation)
		: Entity(commonInformation, interfacesInformation)
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
