/*
* Copyright (C) 2016-2024, L-Acoustics and its contributors

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
* @file entity.hpp
* @author Christophe Calmejane
* @brief Avdecc entities.
*/

#pragma once

#include "entityEnums.hpp"
#include "exception.hpp"

#include <la/networkInterfaceHelper/networkInterfaceHelper.hpp>

#include <cstdint>
#include <thread>
#include <algorithm>
#include <optional>
#include <map>
#include <chrono>

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
		EntityCapabilities entityCapabilities{}; /** The entity's current capabilities (can change over time). */
		std::uint16_t talkerStreamSources{ 0u }; /** The maximum number of streams the entity is capable of sourcing simultaneously. */
		TalkerCapabilities talkerCapabilities{}; /** The entity's capabilities as a talker. */
		std::uint16_t listenerStreamSinks{ 0u }; /** The maximum number of streams the entity is capable of sinking simultaneously. */
		ListenerCapabilities listenerCapabilities{}; /** The entity's capabilities as a listener. */
		ControllerCapabilities controllerCapabilities{}; /** The entity's capabilities as a controller. */
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

	/** Returns true if the specified interfaceIndex exists, false otherwise */
	bool hasInterfaceIndex(model::AvbInterfaceIndex const interfaceIndex) const noexcept
	{
		return _interfaceInformation.count(interfaceIndex) > 0;
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

	/** Gets the mac address associated with the specified AvbInterfaceIndex. Returns an invalid MacAddress if there is no such interface index. */
	la::networkInterface::MacAddress getMacAddress(model::AvbInterfaceIndex const interfaceIndex) const noexcept
	{
		if (hasInterfaceIndex(interfaceIndex))
		{
			auto const& info = getInterfaceInformation(interfaceIndex);
			return info.macAddress;
		}
		return {};
	}

	/** Gets any mac address for the entity */
	la::networkInterface::MacAddress getAnyMacAddress() const
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

	/** Generates an EID from a MacAddress (OUI-36) and a ProgID. This method is provided for backward compatibility, use ProtocolInterface::getDynamicEID instead. */
	static UniqueIdentifier generateEID(la::networkInterface::MacAddress const& macAddress, std::uint16_t const progID, bool const useDeprecatedAlgorithm)
	{
		auto eid = UniqueIdentifier::value_type{ 0u };
		if (macAddress.size() != 6)
		{
			throw Exception("Invalid MAC address size");
		}
		eid += macAddress[0];
		eid <<= 8;
		eid += macAddress[1];
		eid <<= 8;
		eid += macAddress[2];
		if (useDeprecatedAlgorithm)
		{
			eid <<= 16;
			eid += progID;
		}
		eid <<= 8;
		eid += macAddress[3];
		eid <<= 8;
		eid += macAddress[4];
		eid <<= 8;
		eid += macAddress[5];
		if (!useDeprecatedAlgorithm)
		{
			eid <<= 16;
			eid += progID;
		}

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
	virtual void setAssociationID(std::optional<UniqueIdentifier> const associationID) noexcept
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
			AVDECC_ASSERT(interfaceIndex, "InterfaceIndex should be valid");
			getInterfaceInformation(*interfaceIndex).validTime = value;
		}
		else
		{
			// Otherwise set the value for all interfaces on the entity
			for (auto& infoKV : getInterfacesInformation())
			{
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
		Success = 0, /**< The AVDECC Entity successfully performed the command and has valid results. */
		NotImplemented = 1, /**< The AVDECC Entity does not support the command type. */
		NoSuchDescriptor = 2, /**< A descriptor with the descriptor_type and descriptor_index specified does not exist. */
		LockedByOther = 3, /**< The AVDECC Entity has been locked by another AVDECC Controller. */
		AcquiredByOther = 4, /**< The AVDECC Entity has been acquired by another AVDECC Controller. */
		NotAuthenticated = 5, /**< The AVDECC Controller is not authenticated with the AVDECC Entity. */
		AuthenticationDisabled = 6, /**< The AVDECC Controller is trying to use an authentication command when authentication isn't enable on the AVDECC Entity. */
		BadArguments = 7, /**< One or more of the values in the fields of the frame were deemed to be bad by the AVDECC Entity (unsupported, incorrect combination, etc.). */
		NoResources = 8, /**< The AVDECC Entity cannot complete the command because it does not have the resources to support it. */
		InProgress = 9, /**< The AVDECC Entity is processing the command and will send a second response at a later time with the result of the command. */
		EntityMisbehaving = 10, /**< The AVDECC Entity generated an internal error while trying to process the command. */
		NotSupported = 11, /**< The command is implemented but the target of the command is not supported. For example trying to set the value of a read - only Control. */
		StreamIsRunning = 12, /**< The Stream is currently streaming and the command is one which cannot be executed on an Active Stream. */
		// Library Error Codes
		BaseProtocolViolation = 991, /**< The entity sent a message that violates the base protocol */
		PartialImplementation = 992, /**< The library does not fully implement this command, please report this */
		Busy = 993, /**< The library is busy, try again later */
		NetworkError = 995, /**< Network error */
		ProtocolError = 996, /**< Failed to unpack the message due to an error in the protocol */
		TimedOut = 997, /**< The command timed out */
		UnknownEntity = 998, /**< The entity has not been detected on the network */
		InternalError = 999, /**< Internal library error, please report this */
	};

	/** Status code returned by all AA (AECP) command methods. */
	enum class AaCommandStatus : std::uint16_t
	{
		// AVDECC Protocol Error Codes
		Success = 0, /**< The AVDECC Entity successfully performed the command and has valid results. */
		NotImplemented = 1, /**< The AVDECC Entity does not support the command type. */
		AddressTooLow = 2, /**< The value in the address field is below the start of the memory map. */
		AddressTooHigh = 3, /**< The value in the address field is above the end of the memory map. */
		AddressInvalid = 4, /**< The value in the address field is within the memory map but is part of an invalid region. */
		TlvInvalid = 5, /**< One or more of the TLVs were invalid. No TLVs have been processed. */
		DataInvalid = 6, /**< The data for writing is invalid .*/
		Unsupported = 7, /**< A requested action was unsupported. Typically used when an unknown EXECUTE was encountered or if EXECUTE is not supported. */
		// Library Error Codes
		BaseProtocolViolation = 991, /**< The entity sent a message that violates the base protocol */
		PartialImplementation = 992, /**< The library does not fully implement this command, please report this */
		Busy = 993, /**< The library is busy, try again later */
		Aborted = 994, /**< Request aborted */
		NetworkError = 995, /**< Network error */
		ProtocolError = 996, /**< Failed to unpack the message due to an error in the protocol */
		TimedOut = 997, /**< The command timed out */
		UnknownEntity = 998, /**< The entity has not been detected on the network */
		InternalError = 999, /**< Internal library error, please report this */
	};

	/** Status code returned by all MVU (AECP) command methods. */
	enum class MvuCommandStatus : std::uint16_t
	{
		// Milan Vendor Unique Protocol Error Codes
		Success = 0,
		NotImplemented = 1,
		BadArguments = 2,
		// Library Error Codes
		BaseProtocolViolation = 991, /**< The entity sent a message that violates the base protocol */
		PartialImplementation = 992, /**< The library does not fully implement this command, please report this */
		Busy = 993, /**< The library is busy, try again later */
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
		BaseProtocolViolation = 991, /**< The entity sent a message that violates the base protocol */
		NetworkError = 995, /**< A network error occured. */
		ProtocolError = 996, /**< A protocol error occured. */
		TimedOut = 997, /**< Command timed out. */
		UnknownEntity = 998, /**< Entity is unknown. */
		InternalError = 999, /**< Internal library error. */
	};

	/** EntityAdvertise dirty flag */
	enum class AdvertiseFlag : std::uint8_t
	{
		None = 0,
		EntityCapabilities = 1u << 0, /**< EntityCapabilities field changed */
		AssociationID = 1u << 1, /**< AssociationID field changed */
		ValidTime = 1u << 2, /**< ValidTime field changed */
		GptpGrandmasterID = 1u << 3, /**< gPTP GrandmasterID field changed */
		GptpDomainNumber = 1u << 4, /**< gPTP DomainNumber field changed */
	};

	using AdvertiseFlags = la::avdecc::utils::EnumBitfield<AdvertiseFlag>;

	/** Enables entity advertising with available duration included between 2-62 seconds on the specified interfaceIndex if set, otherwise on all interfaces. Returns false if any of the specified parameter is invalid, true otherwise. */
	virtual bool enableEntityAdvertising(std::uint32_t const availableDuration, std::optional<model::AvbInterfaceIndex> const interfaceIndex = std::nullopt) noexcept = 0;

	/** Disables entity advertising on the specified interfaceIndex if set, otherwise on all interfaces. */
	virtual void disableEntityAdvertising(std::optional<model::AvbInterfaceIndex> const interfaceIndex = std::nullopt) noexcept = 0;

	/** Requests a remote entities discovery. */
	virtual bool discoverRemoteEntities() const noexcept = 0;

	/** Requests a targetted remote entity discovery. */
	virtual bool discoverRemoteEntity(UniqueIdentifier const entityID) const noexcept = 0;

	/** Forgets the specified remote entity. */
	virtual bool forgetRemoteEntity(UniqueIdentifier const entityID) const noexcept = 0;

	/** Sets automatic discovery delay. 0 (default) for no automatic discovery. */
	virtual void setAutomaticDiscoveryDelay(std::chrono::milliseconds const delay) noexcept = 0;

	/** BasicLockable concept lock method */
	virtual void lock() noexcept = 0;

	/** BasicLockable concept unlock method */
	virtual void unlock() noexcept = 0;

	/* Utility methods */
	static LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION statusToString(AemCommandStatus const status);
	static LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION statusToString(AaCommandStatus const status);
	static LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION statusToString(MvuCommandStatus const status);
	static LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION statusToString(ControlStatus const status);

	/* Debug methods */
	/** Returns true if the class is already locked by the calling thread */
	virtual bool isSelfLocked() const noexcept = 0;

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
