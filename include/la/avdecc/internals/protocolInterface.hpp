/*
* Copyright (C) 2016-2023, L-Acoustics and its contributors

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
* @file protocolInterface.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/utils.hpp"
#include "la/avdecc/memoryBuffer.hpp"

#include "exception.hpp"
#include "entity.hpp"
#include "protocolAdpdu.hpp"
#include "protocolAecpdu.hpp"
#include "protocolAemAecpdu.hpp"
#include "protocolAcmpdu.hpp"
#include "protocolVuAecpdu.hpp"

#include <la/networkInterfaceHelper/networkInterfaceHelper.hpp>

#include <unordered_map>
#include <memory>
#include <string>
#include <cstdint>
#include <list>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <functional>
#include <optional>
#include <chrono>

namespace la
{
namespace avdecc
{
namespace protocol
{
class ProtocolInterface : public la::avdecc::utils::Subject<ProtocolInterface, std::recursive_mutex>
{
public:
	/** The existing types of ProtocolInterface */
	enum class Type
	{
		None = 0u, /**< No protocol interface (not a valid protocol interface type, should only be used to initialize variables). */
		PCap = 1u << 0, /**< Packet Capture protocol interface. */
		MacOSNative = 1u << 1, /**< macOS native API protocol interface - Only usable on macOS. */
		Proxy = 1u << 2, /**< IEEE Std 1722.1 Proxy protocol interface. */
		Virtual = 1u << 3, /**< Virtual protocol interface. */
	};

	/** Possible Error status returned (or thrown) by a ProtocolInterface */
	enum class Error
	{
		NoError = 0,
		TransportError = 1, /**< Transport interface error. This is critical and the interface is no longer usable. */
		Timeout = 2, /**< A timeout occured during the operation. */
		UnknownRemoteEntity = 3, /**< Unknown remote entity. */
		UnknownLocalEntity = 4, /**< Unknown local entity. */
		InvalidEntityType = 5, /**< Invalid entity type for the operation. */
		DuplicateLocalEntityID = 6, /**< The EntityID specified in a LocalEntity is already in use by another local entity. */
		InterfaceNotFound = 7, /**< Specified interfaceName not found. */
		InvalidParameters = 8, /**< Specified parameters are invalid. */
		InterfaceNotSupported = 9, /**< This protocol interface is not in the list of supported protocol interfaces. */
		MessageNotSupported = 10, /**< This type of message is not supported by this protocol interface. */
		ExecutorNotInitialized = 11, /**< The executor is not initialized. */
		InternalError = 99, /**< Internal error, please report the issue. */
	};

	/** The kind of exception thrown by a ProtocolInterface */
	class LA_AVDECC_API Exception final : public la::avdecc::Exception
	{
	public:
		template<class TextType>
		Exception(Error const error, TextType&& text) noexcept
			: la::avdecc::Exception(std::forward<TextType>(text))
			, _error(error)
		{
		}
		Error getError() const noexcept
		{
			return _error;
		}

	private:
		Error const _error{ Error::NoError };
	};

	using UniquePointer = std::unique_ptr<ProtocolInterface, void (*)(ProtocolInterface*)>;
	using SupportedProtocolInterfaceTypes = la::avdecc::utils::EnumBitfield<Type>;
	using AecpCommandResultHandler = std::function<void(la::avdecc::protocol::Aecpdu const* const response, la::avdecc::protocol::ProtocolInterface::Error const error)>;
	using AcmpCommandResultHandler = std::function<void(la::avdecc::protocol::Acmpdu const* const response, la::avdecc::protocol::ProtocolInterface::Error const error)>;

	/** Interface definition for ProtocolInterface events observation */
	class Observer : public la::avdecc::utils::Observer<ProtocolInterface>
	{
	public:
		virtual ~Observer() noexcept = default;

		/* **** Global notifications **** */
		virtual void onTransportError(la::avdecc::protocol::ProtocolInterface* const /*pi*/) noexcept {}

		/* **** Discovery notifications **** */
		virtual void onLocalEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& /*entity*/) noexcept {}
		virtual void onLocalEntityOffline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept {}
		virtual void onLocalEntityUpdated(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& /*entity*/) noexcept {}
		virtual void onRemoteEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& /*entity*/) noexcept {}
		virtual void onRemoteEntityOffline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept {}
		virtual void onRemoteEntityUpdated(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& /*entity*/) noexcept {}

		/* **** AECP notifications **** */
		/** Notification for when an AECP Command is received (for a locally registered entity). */
		virtual void onAecpCommand(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::Aecpdu const& /*aecpdu*/) noexcept {}
		/** Notification for when an unsolicited AECP-AEM Response is received (for a locally registered entity). */
		virtual void onAecpAemUnsolicitedResponse(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::AemAecpdu const& /*aecpdu*/) noexcept {}
		/** Notification for when an identify notification is received (the notification being a multicast message, the notification is triggered even if there are no locally registered entities). */
		virtual void onAecpAemIdentifyNotification(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::AemAecpdu const& /*aecpdu*/) noexcept {}

		/* **** ACMP notifications **** */
		/** Notification for when an ACMP Command is received, even for none of the locally registered entities. */
		virtual void onAcmpCommand(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::Acmpdu const& /*acmpdu*/) noexcept {}
		/** Notification for when an ACMP Response is received, even for none of the locally registered entities and for responses already processed by the CommandStateMachine (meaning the sendAcmpCommand result handler have already been called). */
		virtual void onAcmpResponse(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::Acmpdu const& /*acmpdu*/) noexcept {}

		/* **** Statistics **** */
		/** Notification for when an AECP Command was resent due to a timeout (ControllerStateMachine only). If the retry time out again, then onAecpTimeout will be called. */
		virtual void onAecpRetry(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const& /*entityID*/) noexcept {}
		/** Notification for when an AECP Command timed out (not called when onAecpRetry is called). */
		virtual void onAecpTimeout(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const& /*entityID*/) noexcept {}
		/** Notification for when an AECP Response is received but is not expected (might have already timed out). */
		virtual void onAecpUnexpectedResponse(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const& /*entityID*/) noexcept {}
		/** Notification for when an AECP Response is received (not an Unsolicited one) along with the time elapsed between the send and the receive. */
		virtual void onAecpResponseTime(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const& /*entityID*/, std::chrono::milliseconds const& /*responseTime*/) noexcept {}

		/* **** Low level notifications (not supported by all kinds of ProtocolInterface), triggered before processing the pdu **** */
		/** Notification for when an ADPDU is received (might be a message that was sent by self as this event might be triggered for outgoing messages). */
		virtual void onAdpduReceived(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::Adpdu const& /*adpdu*/) noexcept {}
		/** Notification for when an AECPDU is received (might be a message that was sent by self as this event might be triggered for outgoing messages). Note: Only AECP sub types known by the library are notified by this event. */
		virtual void onAecpduReceived(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::Aecpdu const& /*aecpdu*/) noexcept {}
		/** Notification for when an ACMPDU is received (might be a message that was sent by self as this event might be triggered for outgoing messages). */
		virtual void onAcmpduReceived(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::Acmpdu const& /*acmpdu*/) noexcept {}
	};

	/** Interface definition for AECP VendorUnique message delegation */
	class VendorUniqueDelegate
	{
	public:
		virtual ~VendorUniqueDelegate() noexcept = default;

		/** Method to create an Aecpdu inherited UniquePointer for this VendorUnique. */
		virtual la::avdecc::protocol::Aecpdu::UniquePointer createAecpdu(la::avdecc::protocol::VuAecpdu::ProtocolIdentifier const& protocolIdentifier, bool const isResponse) noexcept = 0;

		/**
		* @brief Query whether the messages are to be handled by the ControllerStateMachine or the VendorUniqueDelegate itself.
		* @details VendorUniqueDelegates should decide if they fully handle VendorUnique messages, or if they let the ControllerStateMachine handle them like any other AECP messages.
		*          If they are to be handled by the ControllerStateMachine:
		*           - onVuAecpResponse will never be called
		*           - getCommandTimeout will be called for VendorUnique Commands by the ControllerStateMachine so it knows when a command timed out and can be retried (or return Timeout error status)
		*           - AecpCommandResultHandler handler passed to sendAecpCommand will be called at some point
		*          If they are to be handled by the VendorUniqueDelegate:
		*           - sendAecpCommand and sendAecpResponse shall not be called (use sendAecpMessage instead)
		*           - onVuAecpResponse will be called when a VendorUnique Reponse for the registered VuAecpdu::ProtocolIdentifier is received and must be processed
		*           - AecpCommandResultHandler handler passed to sendAecpCommand will never be called
		*          In any case:
		*           - onVuAecpCommand will be called whenever a VendorUnique Command for the registered VuAecpdu::ProtocolIdentifier is received
		*           - onAecpCommand and onAecpAemUnsolicitedResponse from Observer won't be called
		* @return True if the messages are to be handled by the ControllerStateMachine, false by the VendorUniqueDelegate itself.
		* @note When messages are handled by the VendorUniqueDelegate itself, no AECP throttling nor retry mechanisms are active, they have to be handled by the delegate if required.
		*/
		virtual bool areHandledByControllerStateMachine(la::avdecc::protocol::VuAecpdu::ProtocolIdentifier const& /*protocolIdentifier*/) const noexcept
		{
			return false;
		}

		/** Gets the timeout value (in milliseconds) for the provided VuAecpdu. Called only if areHandledByControllerStateMachine() returned true. */
		virtual std::uint32_t getVuAecpCommandTimeoutMsec(la::avdecc::protocol::VuAecpdu::ProtocolIdentifier const& /*protocolIdentifier*/, la::avdecc::protocol::VuAecpdu const& /*aecpdu*/) noexcept
		{
			return 250u;
		}

		/** Notification for when an AECP VendorUnique Command is received (for a locally registered entity), for a ProtocolIdentifier this Delegate registered for. */
		virtual void onVuAecpCommand(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::VuAecpdu::ProtocolIdentifier const& /*protocolIdentifier*/, la::avdecc::protocol::VuAecpdu const& /*aecpdu*/) noexcept {}

		/** Notification for when an AECP VendorUnique Response is received (for a locally registered entity), for a ProtocolIdentifier this Delegate registered for. Called only if areHandledByControllerStateMachine() returned false. */
		virtual void onVuAecpResponse(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::protocol::VuAecpdu::ProtocolIdentifier const& /*protocolIdentifier*/, la::avdecc::protocol::VuAecpdu const& /*aecpdu*/) noexcept {}
	};

	/**
	* @brief Factory method to create a new ProtocolInterface.
	* @details Creates a new ProtocolInterface as a unique pointer.
	* @param[in] protocolInterfaceType The protocol interface type to use.
	* @param[in] networkInterfaceName The name of the network interface to use. Use #la::networkInterface::NetworkInterfaceHelper::enumerateInterfaces to get a valid interface name.
	* @param[in] executorName The name of the executor to use to dispatch incoming messages.
	* @return A new ProtocolInterface as a ProtocolInterface::UniquePointer.
	* @note Might throw an Exception.
	*/
	static UniquePointer create(Type const protocolInterfaceType, std::string const& networkInterfaceName, std::string const& executorName)
	{
		auto deleter = [](ProtocolInterface* self)
		{
			self->destroy();
		};
		return UniquePointer(createRawProtocolInterface(protocolInterfaceType, networkInterfaceName, executorName), deleter);
	}

	/* ************************************************************ */
	/* General entry points                                         */
	/* ************************************************************ */
	/** Returns the name of the executor used by this ProtocolInterface. */
	LA_AVDECC_API std::string const& LA_AVDECC_CALL_CONVENTION getExecutorName() const noexcept;
	/** Returns the Mac Address associated with this ProtocolInterface. */
	LA_AVDECC_API networkInterface::MacAddress const& LA_AVDECC_CALL_CONVENTION getMacAddress() const noexcept;
	/** Shuts down the interface, stopping all active communications. This method blocks the current thread until all pending messages are processed. This is automatically called during destructor. */
	virtual void shutdown() noexcept = 0;
	/** Gets an available Entity UniqueIdentifier that is valid for this ProtocolInterface (not supported by all kinds of ProtocolInterface). Call releaseDynamicEID when the returned entityID is no longer used. */
	virtual UniqueIdentifier getDynamicEID() const noexcept = 0;
	/** Releases a dynamic Entity UniqueIdentifier previously returned by getDynamicEID. */
	virtual void releaseDynamicEID(UniqueIdentifier const entityID) const noexcept = 0;
	/** Registers a local entity to the interface, allowing it to send and receive messages (Error::InvalidParameters is returned if the entity has no InterfaceInformation matching this ProtocolInterface). */
	virtual Error registerLocalEntity(entity::LocalEntity& entity) noexcept = 0;
	/** Unregisters a local entity from the interface. It won't be able to send or receive messages anymore. */
	virtual Error unregisterLocalEntity(entity::LocalEntity& entity) noexcept = 0;
	/** Injects a raw packet into the receiving message loop */
	virtual Error injectRawPacket(la::avdecc::MemoryBuffer&& packet) const noexcept = 0;
	/** Registers a VendorUniqueDelegate for the specified protocolIdentifier. */
	LA_AVDECC_API Error LA_AVDECC_CALL_CONVENTION registerVendorUniqueDelegate(VuAecpdu::ProtocolIdentifier const& protocolIdentifier, VendorUniqueDelegate* const delegate) noexcept;
	/** Unregisters the VendorUniqueDelegate previously associated to the specified protocolIdentifier. */
	LA_AVDECC_API Error LA_AVDECC_CALL_CONVENTION unregisterVendorUniqueDelegate(VuAecpdu::ProtocolIdentifier const& protocolIdentifier) noexcept;
	/** Unregisters all VendorUniqueDelegate. */
	LA_AVDECC_API Error LA_AVDECC_CALL_CONVENTION unregisterAllVendorUniqueDelegates() noexcept;

	/* ************************************************************ */
	/* Advertising entry points                                     */
	/* ************************************************************ */
	/** Enables entity advertising on the network. */
	virtual Error enableEntityAdvertising(entity::LocalEntity& entity) noexcept = 0;
	/** Disables entity advertising on the network. */
	virtual Error disableEntityAdvertising(entity::LocalEntity const& entity) noexcept = 0;
	/** Flags the entity for re-announcement on this ProtocolInterface. */
	virtual Error setEntityNeedsAdvertise(entity::LocalEntity const& entity, entity::LocalEntity::AdvertiseFlags const flags) noexcept = 0;

	/* ************************************************************ */
	/* Discovery entry points                                       */
	/* ************************************************************ */
	/** Requests a remote entities discovery. */
	virtual Error discoverRemoteEntities() const noexcept = 0;
	/** Requests a targetted remote entity discovery. */
	virtual Error discoverRemoteEntity(UniqueIdentifier const entityID) const noexcept = 0;
	/** Sets automatic discovery delay. 0 (default) for no automatic discovery. */
	virtual Error setAutomaticDiscoveryDelay(std::chrono::milliseconds const delay) const noexcept = 0;

	/* ************************************************************ */
	/* Sending entry points                                         */
	/* ************************************************************ */
	/** Returns true if the ProtocolInterface supports sending direct messages (send*Message APIs) */
	virtual bool isDirectMessageSupported() const noexcept = 0;
	/** Sends an ADP message directly on the network (not supported by all kinds of ProtocolInterface). */
	virtual Error sendAdpMessage(Adpdu const& adpdu) const noexcept = 0;
	/** Sends an AECP message directly on the network (not supported by all kinds of ProtocolInterface). */
	virtual Error sendAecpMessage(Aecpdu const& aecpdu) const noexcept = 0;
	/** Sends an ACMP message directly on the network (not supported by all kinds of ProtocolInterface). */
	virtual Error sendAcmpMessage(Acmpdu const& acmpdu) const noexcept = 0;
	/** Sends an AECP command message. Only registered LocalEntities are allowed to call this method. VuAecpdu that are not handled by the ControllerStateMachine are not allowed to call this method (use sendAecpMessage for those cases). */
	virtual Error sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, AecpCommandResultHandler const& onResult) const noexcept = 0;
	/** Sends an AECP response message. Only registered LocalEntities are allowed to call this method. */
	virtual Error sendAecpResponse(Aecpdu::UniquePointer&& aecpdu) const noexcept = 0;
	/** Sends an ACMP command message. Only registered LocalEntities are allowed to call this method. */
	virtual Error sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, AcmpCommandResultHandler const& onResult) const noexcept = 0;
	/** Sends an ACMP response message. Only registered LocalEntities are allowed to call this method. */
	virtual Error sendAcmpResponse(Acmpdu::UniquePointer&& acmpdu) const noexcept = 0;

	/* ************************************************************ */
	/* Misc entry points                                            */
	/* ************************************************************ */
	/** BasicLockable concept 'lock' method for the whole ProtocolInterface */
	virtual void lock() const noexcept = 0;
	/** BasicLockable concept 'unlock' method for the whole ProtocolInterface */
	virtual void unlock() const noexcept = 0;
	/** Debug method: Returns true if the whole ProtocolInterface is locked by the calling thread */
	virtual bool isSelfLocked() const noexcept = 0;

	/** Returns true if the specified protocol interface type is supported on the local computer. */
	static LA_AVDECC_API bool LA_AVDECC_CALL_CONVENTION isSupportedProtocolInterfaceType(Type const protocolInterfaceType) noexcept;

	/** Returns the name of the specified protocol interface type. */
	static LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION typeToString(Type const protocolInterfaceType) noexcept;

	/** Returns the list of supported protocol interface types on the local computer. */
	static LA_AVDECC_API SupportedProtocolInterfaceTypes LA_AVDECC_CALL_CONVENTION getSupportedProtocolInterfaceTypes() noexcept;

	// Deleted compiler auto-generated methods
	ProtocolInterface(ProtocolInterface&&) = delete;
	ProtocolInterface(ProtocolInterface const&) = delete;
	ProtocolInterface& operator=(ProtocolInterface const&) = delete;
	ProtocolInterface& operator=(ProtocolInterface&&) = delete;

protected:
	/**
	* @brief Create a ProtocolInterface associated with specified network interface name.
	* @details Create a ProtocolInterface associated with specified network interface name, checking the interface actually exists.
	* @param[in] networkInterfaceName The name of the network interface.
	* @param[in] executorName The name of the executor to use to dispatch incoming messages.
	* @note Throws Exception if networkInterfaceName is invalid or inaccessible.
	*/
	ProtocolInterface(std::string const& networkInterfaceName, std::string const& executorName);

	/**
	* @brief Create a ProtocolInterface associated with specified network interface name and MAC address.
	* @details Create a ProtocolInterface associated with specified network interface name and MAC address, not checking if the interface exists.
	* @param[in] networkInterfaceName The name of the network interface.
	* @param[in] macAddress The MAC address associated with the network interface. Cannot be all 0.
	* @param[in] executorName The name of the executor to use to dispatch incoming messages.
	* @note Throws Exception if networkInterfaceName or macAddress is invalid or inaccessible.
	*/
	ProtocolInterface(std::string const& networkInterfaceName, networkInterface::MacAddress const& macAddress, std::string const& executorName);

	virtual ~ProtocolInterface() noexcept = default;

	/** Returns true is the specified AecpMessageType is a Response kind, false if it's a Command kind. */
	static bool isAecpResponseMessageType(AecpMessageType const messageType) noexcept;

	/** Returns the Command Timeout (in msec) for the specified VendorUnique ProtocolIdentifier and VuAecpdu. */
	std::uint32_t getVuAecpCommandTimeout(VuAecpdu::ProtocolIdentifier const& protocolIdentifier, VuAecpdu const& aecpdu) const noexcept;

	/** Returns the VendorUniqueDelegate handling the specified protocolIdentifier, or nullptr if none has been registered. WARNING: Once returned, the pointed object is NOT locked. */
	VendorUniqueDelegate* getVendorUniqueDelegate(VuAecpdu::ProtocolIdentifier const& protocolIdentifier) const noexcept;

	std::string const _networkInterfaceName{};

private:
	/** Entry point */
	static LA_AVDECC_API ProtocolInterface* LA_AVDECC_CALL_CONVENTION createRawProtocolInterface(Type const protocolInterfaceType, std::string const& networkInterfaceName, std::string const& executorName);

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept = 0;

	networkInterface::MacAddress _networkInterfaceMacAddress{};
	std::unordered_map<VuAecpdu::ProtocolIdentifier, VendorUniqueDelegate*, VuAecpdu::ProtocolIdentifier::hash> _vendorUniqueDelegates{};
	std::string _executorName{};
};

/* Operator overloads */
constexpr bool operator!(ProtocolInterface::Error const error)
{
	return error == ProtocolInterface::Error::NoError;
}

constexpr ProtocolInterface::Error& operator|=(ProtocolInterface::Error& lhs, ProtocolInterface::Error const rhs)
{
	lhs = static_cast<ProtocolInterface::Error>(static_cast<std::underlying_type_t<ProtocolInterface::Error>>(lhs) | static_cast<std::underlying_type_t<ProtocolInterface::Error>>(rhs));
	return lhs;
}

} // namespace protocol

} // namespace avdecc
} // namespace la
