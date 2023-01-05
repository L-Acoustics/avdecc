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
* @file protocolInterface_macNative.mm
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/protocolAemAecpdu.hpp"
#include "la/avdecc/internals/protocolAaAecpdu.hpp"
#include "la/avdecc/internals/protocolVuAecpdu.hpp"
#include "la/avdecc/internals/protocolMvuAecpdu.hpp"

#include "stateMachine/stateMachineManager.hpp"
#include "protocolInterface_macNative.hpp"
#include "logHelper.hpp"

#include <stdexcept>
#include <functional>
#include <memory>
#include <string>
#include <optional>
#include <sstream>
#include <thread>
#include <chrono>
#include <list>
#include <unordered_map>
#include <unordered_set>

#import <AudioVideoBridging/AudioVideoBridging.h>

#pragma mark Forward declaration of ProtocolInterfaceMacNativeImpl
namespace la
{
namespace avdecc
{
namespace protocol
{
/* Default state machine parameters */
static constexpr size_t DefaultMaxAecpInflightCommands = 1;
//static constexpr std::chrono::milliseconds DefaultAecpSendInterval{ 1u };
//static constexpr size_t DefaultMaxAcmpMulticastInflightCommands = 10;
//static constexpr size_t DefaultMaxAcmpUnicastInflightCommands = 10;
//static constexpr std::chrono::milliseconds DefaultAcmpMulticastSendInterval{ 1u };
//static constexpr std::chrono::milliseconds DefaultAcmpUnicastSendInterval{ 1u };

class ProtocolInterfaceMacNativeImpl;
} // namespace protocol
} // namespace avdecc
} // namespace la

#pragma mark - BridgeInterface Declaration
struct EntityQueues
{
	dispatch_queue_t aecpQueue;
	dispatch_semaphore_t aecpLimiter;
};

struct LockInformation
{
	std::recursive_mutex _lock{};
	std::uint32_t _lockedCount{ 0u };
	std::thread::id _lockingThreadID{};

	void lock() noexcept
	{
		_lock.lock();
		if (_lockedCount == 0)
		{
			_lockingThreadID = std::this_thread::get_id();
		}
		++_lockedCount;
	}

	void unlock() noexcept
	{
		--_lockedCount;
		if (_lockedCount == 0)
		{
			_lockingThreadID = {};
		}
		_lock.unlock();
	}

	bool isSelfLocked() const noexcept
	{
		return _lockingThreadID == std::this_thread::get_id();
	}
};

@interface BridgeInterface : NSObject <AVB17221EntityDiscoveryDelegate, AVB17221AECPClient, AVB17221ACMPClient>
// Private variables
{
	BOOL _primedDiscovery;
	la::avdecc::protocol::ProtocolInterfaceMacNativeImpl* _protocolInterface;

	LockInformation _lock; /** Lock to protect the ProtocolInterface */
	std::unordered_map<la::avdecc::UniqueIdentifier, std::uint32_t, la::avdecc::UniqueIdentifier::hash> _lastAvailableIndex; /** Last received AvailableIndex for each entity */
	std::unordered_map<la::avdecc::UniqueIdentifier, la::avdecc::entity::LocalEntity&, la::avdecc::UniqueIdentifier::hash> _localProcessEntities; /** Local entities declared by the running process */
	std::unordered_map<la::avdecc::UniqueIdentifier, la::avdecc::entity::Entity, la::avdecc::UniqueIdentifier::hash> _localMachineEntities; /** Local entities declared by the local machine */
	std::unordered_map<la::avdecc::UniqueIdentifier, la::avdecc::entity::Entity, la::avdecc::UniqueIdentifier::hash> _remoteEntities; /** Remove entities discovered */
	std::unordered_set<la::avdecc::UniqueIdentifier, la::avdecc::UniqueIdentifier::hash> _registeredAcmpHandlers; /** List of ACMP handlers that have been registered (that must be removed upon destruction, since there is no removeAllHandlers method) */

	std::mutex _lockQueues; /** Lock to protect _entityQueues */
	std::unordered_map<la::avdecc::UniqueIdentifier, EntityQueues, la::avdecc::UniqueIdentifier::hash> _entityQueues;
	dispatch_queue_t _deleterQueue; // Queue to postpone deletion of handlers

	std::mutex _lockPending; /** Lock to protect _pendingCommands and _pendingCondVar */
	std::uint32_t _pendingCommands; /** Count of pending (inflight) commands, since there is no way to cancel a command upon destruction (and result block might be called while we already destroyed our objects) */
	std::condition_variable _pendingCondVar;
}

+ (BOOL)isSupported;
/** std::string to NSString conversion */
+ (NSString*)getNSString:(std::string const&)cString;
/** NSString to std::string conversion */
+ (std::string)getStdString:(NSString*)nsString;
+ (NSString*)getEntityCapabilities:(AVB17221Entity*)entity;

- (std::optional<la::avdecc::entity::model::AvbInterfaceIndex>)getMatchingInterfaceIndex:(la::avdecc::entity::LocalEntity const&)entity;

/** Initializer */
- (id)initWithInterfaceName:(NSString*)interfaceName andProtocolInterface:(la::avdecc::protocol::ProtocolInterfaceMacNativeImpl*)protocolInterface;
/** Deinit method to shutdown every pending operations */
- (void)deinit;
/** Destructor */
- (void)dealloc;

// la::avdecc::protocol::ProtocolInterface bridge methods
- (void)notifyDiscoveredEntities:(la::avdecc::protocol::ProtocolInterface::Observer&)obs;
- (la::avdecc::UniqueIdentifier)getDynamicEID;
- (void)releaseDynamicEID:(la::avdecc::UniqueIdentifier)entityID;
// Registration of a local process entity (an entity declared inside this process, not all local computer entities)
- (la::avdecc::protocol::ProtocolInterface::Error)registerLocalEntity:(la::avdecc::entity::LocalEntity&)entity;
// Remove handlers for a local process entity
- (void)removeLocalProcessEntityHandlers:(la::avdecc::entity::LocalEntity const&)entity;
// Unregistration of a local process entity
- (la::avdecc::protocol::ProtocolInterface::Error)unregisterLocalEntity:(la::avdecc::entity::LocalEntity const&)entity;
- (la::avdecc::protocol::ProtocolInterface::Error)injectRawPacket:(la::avdecc::MemoryBuffer&&)packet;
- (la::avdecc::protocol::ProtocolInterface::Error)setEntityNeedsAdvertise:(la::avdecc::entity::LocalEntity const&)entity flags:(la::avdecc::entity::LocalEntity::AdvertiseFlags)flags;
- (la::avdecc::protocol::ProtocolInterface::Error)enableEntityAdvertising:(la::avdecc::entity::LocalEntity const&)entity;
- (la::avdecc::protocol::ProtocolInterface::Error)disableEntityAdvertising:(la::avdecc::entity::LocalEntity const&)entity;
- (BOOL)discoverRemoteEntities;
- (BOOL)discoverRemoteEntity:(la::avdecc::UniqueIdentifier)entityID;
- (la::avdecc::protocol::ProtocolInterface::Error)sendAecpCommand:(la::avdecc::protocol::Aecpdu::UniquePointer&&)aecpdu handler:(la::avdecc::protocol::ProtocolInterface::AecpCommandResultHandler const&)onResult;
- (la::avdecc::protocol::ProtocolInterface::Error)sendAecpResponse:(la::avdecc::protocol::Aecpdu::UniquePointer&&)aecpdu;
- (la::avdecc::protocol::ProtocolInterface::Error)sendAcmpCommand:(la::avdecc::protocol::Acmpdu::UniquePointer&&)acmpdu handler:(la::avdecc::protocol::ProtocolInterface::AcmpCommandResultHandler const&)onResult;
- (void)lock;
- (void)unlock;
- (bool)isSelfLocked;

// Variables
@property (retain) AVBInterface* interface;

@end

#pragma mark - ProtocolInterfaceMacNativeImpl Declaration/Implementation
namespace la
{
namespace avdecc
{
namespace protocol
{
class ProtocolInterfaceMacNativeImpl final : public ProtocolInterfaceMacNative, private stateMachine::ProtocolInterfaceDelegate
{
public:
	// Publicly expose methods so the objC code can use it directly
	using ProtocolInterfaceMacNative::notifyObservers;
	using ProtocolInterfaceMacNative::notifyObserversMethod;
	using ProtocolInterfaceMacNative::getVendorUniqueDelegate;

	/** Constructor */
	ProtocolInterfaceMacNativeImpl(std::string const& networkInterfaceName)
		: ProtocolInterfaceMacNative(networkInterfaceName)
	{
		// Should not be there if the interface is not supported
		AVDECC_ASSERT(isSupported(), "Should not be there if the interface is not supported");

		auto* intName = [BridgeInterface getNSString:networkInterfaceName];

#if 0 // We don't need to check for AVB capability/enable on the interface, AVDECC do not require an AVB compatible interface \
	// Check the interface is AVB enabled
					if(![AVBInterface isAVBEnabledOnInterfaceNamed:intName])
					{
						throw std::invalid_argument("Interface is not AVB enabled");
					}
					// Check the interface is AVB capable
					if(![AVBInterface isAVBCapableInterfaceNamed:intName])
					{
						throw std::invalid_argument("Interface is not AVB capable");
					}
#endif // 0

		// We can now create an AVBInterface from this network interface
		_bridge = [[BridgeInterface alloc] initWithInterfaceName:intName andProtocolInterface:this];

		// Start the state machines
		_stateMachineManager.startStateMachines();

		// Should no longer terminate
		_shouldTerminate = false;

		// Create the state machine thread
		_stateMachineThread = std::thread(
			[this]
			{
				utils::setCurrentThreadName("avdecc::StateMachine::macNative");

				while (!_shouldTerminate)
				{
					// Check for VendorUnique inflight commands expiracy
					checkVendorUniqueInflightCommandsTimeoutExpiracy();

					// Wait a little bit so we don't burn the CPU
					std::this_thread::sleep_for(std::chrono::milliseconds(5));
				}
			});
	}

	void handleVendorUniqueCommandSent(Aecpdu::UniquePointer&& aecpdu, la::avdecc::protocol::ProtocolInterface::AecpCommandResultHandler const& resultHandler) noexcept
	{
		if (AVDECC_ASSERT_WITH_RET(aecpdu->getMessageType() == AecpMessageType::VendorUniqueCommand, "Should be a VendorUnique Command"))
		{
			auto const& vuAecp = static_cast<VuAecpdu const&>(*aecpdu);

			// Lock
			auto const lg = std::lock_guard{ *this };

			// Get CommandEntityInfo matching ControllerEntityID
			auto const& commandEntityIt = _commandEntities.find(aecpdu->getControllerEntityID());
			if (commandEntityIt == _commandEntities.end())
			{
				utils::invokeProtectedHandler(resultHandler, nullptr, Error::UnknownLocalEntity);
				return;
			}
			auto const vuProtocolID = vuAecp.getProtocolIdentifier();
			auto* vuDelegate = getVendorUniqueDelegate(vuProtocolID);
			if (AVDECC_ASSERT_WITH_RET(vuDelegate, "Should have a VendorUnique delegate"))
			{
				// Are the messages handled by our state machine
				if (vuDelegate->areHandledByControllerStateMachine(vuProtocolID))
				{
					auto& commandEntityInfo = commandEntityIt->second;
					auto const targetEntityID = vuAecp.getTargetEntityID();
					auto const sequenceID = vuAecp.getSequenceID();

					// Record the query for when we get a response (so we can send it again if it timed out)
					auto command = VendorUniqueCommandInfo{ sequenceID, std::move(aecpdu), resultHandler };

					// Set times
					command.sendTime = std::chrono::steady_clock::now();
					command.timeoutTime = command.sendTime + std::chrono::milliseconds(getVuAecpCommandTimeoutMsec(vuProtocolID, vuAecp));

					// Add the command to the inflight queue
					commandEntityInfo.inflightVendorUniqueCommands[targetEntityID].push_back(std::move(command));
				}
			}
			else
			{
				utils::invokeProtectedHandler(resultHandler, nullptr, Error::InternalError);
			}
		}
		else
		{
			utils::invokeProtectedHandler(resultHandler, nullptr, Error::InternalError);
		}
	}

	bool handleVendorUniqueResponseReceived(VuAecpdu const& vuAecp) noexcept
	{
		auto const vuProtocolID = vuAecp.getProtocolIdentifier();
		auto* vuDelegate = getVendorUniqueDelegate(vuProtocolID);
		if (vuDelegate)
		{
			// Are the messages handled by the VendorUniqueDelegate itself
			if (!vuDelegate->areHandledByControllerStateMachine(vuProtocolID))
			{
				// Low level notification
				notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpduReceived, this, vuAecp);

				// Forward to the delegate
				vuDelegate->onVuAecpResponse(this, vuProtocolID, vuAecp);
			}
			else
			{
				// Messages are handled by our state machine
				auto const controllerID = vuAecp.getControllerEntityID();

				// Lock
				auto const lg = std::lock_guard{ *this };

				// Only process it if it's targeted to a registered local command entity (which is set in the ControllerID field)
				if (auto const commandEntityIt = _commandEntities.find(controllerID); commandEntityIt != _commandEntities.end())
				{
					auto& commandEntityInfo = commandEntityIt->second;
					auto const targetID = vuAecp.getTargetEntityID();

					if (auto inflightIt = commandEntityInfo.inflightVendorUniqueCommands.find(targetID); inflightIt != commandEntityInfo.inflightVendorUniqueCommands.end())
					{
						auto& inflightCommands = inflightIt->second;
						auto const sequenceID = vuAecp.getSequenceID();
						auto commandIt = std::find_if(inflightCommands.begin(), inflightCommands.end(),
							[sequenceID](VendorUniqueCommandInfo const& command)
							{
								return command.sequenceID == sequenceID;
							});
						// If the sequenceID is not found, it means the response already timed out (arriving too late)
						if (commandIt != inflightCommands.end())
						{
							auto& info = *commandIt;

							// Move the query (it will be deleted)
							auto vuAecpQuery = std::move(info);

							// Remove the command from inflight list
							commandEntityInfo.inflightVendorUniqueCommands.erase(inflightIt);

							// Call completion handler
							utils::invokeProtectedHandler(vuAecpQuery.resultHandler, &vuAecp, ProtocolInterface::Error::NoError);
						}
						else
						{
							LOG_CONTROLLER_STATE_MACHINE_DEBUG(targetID, std::string("VendorUnique command with sequenceID ") + std::to_string(sequenceID) + " unexpected (timed out already?)");
						}
					}
				}
			}

			return true;
		}
		else
		{
			LOG_PROTOCOL_INTERFACE_DEBUG(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "Unhandled VendorUnique Response for ProtocolIdentifier {}", utils::toHexString(static_cast<VuAecpdu::ProtocolIdentifier::IntegralType>(vuProtocolID), true));
		}

		return false;
	}

	void addLocalEntity(entity::LocalEntity& entity)
	{
		// Lock
		auto const lg = std::lock_guard{ *this };

		_commandEntities.emplace(std::make_pair(entity.getEntityID(), CommandEntityInfo{ entity }));
	}

	void removeLocalEntity(UniqueIdentifier const& entityID)
	{
		// Lock
		auto const lg = std::lock_guard{ *this };

		_commandEntities.erase(entityID);
	}

	virtual void lock() const noexcept override
	{
		[_bridge lock];
	}

	virtual void unlock() const noexcept override
	{
		[_bridge unlock];
	}

	virtual bool isSelfLocked() const noexcept override
	{
		return [_bridge isSelfLocked];
	}

	/** Destructor */
	virtual ~ProtocolInterfaceMacNativeImpl() noexcept
	{
		shutdown();
	}

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept override
	{
		delete this;
	}

	// Deleted compiler auto-generated methods
	ProtocolInterfaceMacNativeImpl(ProtocolInterfaceMacNativeImpl&&) = delete;
	ProtocolInterfaceMacNativeImpl(ProtocolInterfaceMacNativeImpl const&) = delete;
	ProtocolInterfaceMacNativeImpl& operator=(ProtocolInterfaceMacNativeImpl const&) = delete;
	ProtocolInterfaceMacNativeImpl& operator=(ProtocolInterfaceMacNativeImpl&&) = delete;

private:
#pragma mark la::avdecc::protocol::ProtocolInterface overrides
	virtual void shutdown() noexcept override
	{
		// Stop the state machines
		_stateMachineManager.stopStateMachines();

		// StateMachines are started
		if (_stateMachineThread.joinable())
		{
			// Notify the thread we are shutting down
			_shouldTerminate = true;

			// Wait for the thread to complete its pending tasks
			_stateMachineThread.join();
		}

		// Destroy the bridge
		if (_bridge != nullptr)
		{
			[_bridge deinit];
#if !__has_feature(objc_arc)
			[_bridge release];
#endif
			_bridge = nullptr;
		}
	}

	virtual UniqueIdentifier getDynamicEID() const noexcept override
	{
		return [_bridge getDynamicEID];
	}

	virtual void releaseDynamicEID(UniqueIdentifier const entityID) const noexcept override
	{
		[_bridge releaseDynamicEID:entityID];
	}

	virtual Error registerLocalEntity(entity::LocalEntity& entity) noexcept override
	{
		return [_bridge registerLocalEntity:entity];
	}

	virtual Error unregisterLocalEntity(entity::LocalEntity& entity) noexcept override
	{
		return [_bridge unregisterLocalEntity:entity];
	}

	virtual Error injectRawPacket(la::avdecc::MemoryBuffer&& packet) const noexcept override
	{
		return [_bridge injectRawPacket:std::move(packet)];
	}

	virtual Error setEntityNeedsAdvertise(entity::LocalEntity const& entity, entity::LocalEntity::AdvertiseFlags const flags) noexcept override
	{
		return [_bridge setEntityNeedsAdvertise:entity flags:flags];
	}

	virtual Error enableEntityAdvertising(entity::LocalEntity& entity) noexcept override
	{
		return [_bridge enableEntityAdvertising:entity];
	}

	virtual Error disableEntityAdvertising(entity::LocalEntity const& entity) noexcept override
	{
		return [_bridge disableEntityAdvertising:entity];
	}

	virtual Error discoverRemoteEntities() const noexcept override
	{
		if ([_bridge discoverRemoteEntities])
		{
			_stateMachineManager.discoverMessageSent();
			return ProtocolInterface::Error::NoError;
		}
		return ProtocolInterface::Error::TransportError;
	}

	virtual Error discoverRemoteEntity(UniqueIdentifier const entityID) const noexcept override
	{
		if ([_bridge discoverRemoteEntity:entityID])
		{
			_stateMachineManager.discoverMessageSent();
			return ProtocolInterface::Error::NoError;
		}
		return ProtocolInterface::Error::TransportError;
	}

	virtual Error setAutomaticDiscoveryDelay(std::chrono::milliseconds const delay) const noexcept override
	{
		return _stateMachineManager.setAutomaticDiscoveryDelay(delay);
	}

	virtual bool isDirectMessageSupported() const noexcept override
	{
		return false;
	}

	virtual Error sendAdpMessage(Adpdu const& adpdu) const noexcept override
	{
		return Error::MessageNotSupported;
	}

	virtual Error sendAecpMessage(Aecpdu const& aecpdu) const noexcept override
	{
		// TODO: Check the kind of message so we can at least process some of them
		// All Responses can be sent using the sendAecpResponse method
		// VendorUniqueCommand that are not processed by the ControllerStateMachine can be sent using a duplicate of the sendAecpCommand code (removing the code related to the ResultHandler)
		return Error::MessageNotSupported;
	}

	virtual Error sendAcmpMessage(Acmpdu const& acmpdu) const noexcept override
	{
		return Error::MessageNotSupported;
	}

	virtual Error sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, AecpCommandResultHandler const& onResult) const noexcept override
	{
		auto const messageType = aecpdu->getMessageType();

		if (!AVDECC_ASSERT_WITH_RET(!isAecpResponseMessageType(messageType), "Calling sendAecpCommand with a Response MessageType"))
		{
			return Error::MessageNotSupported;
		}

		// Special check for VendorUnique messages
		if (messageType == AecpMessageType::VendorUniqueCommand)
		{
			auto& vuAecp = static_cast<VuAecpdu&>(*aecpdu);

			auto const vuProtocolID = vuAecp.getProtocolIdentifier();
			auto* vuDelegate = getVendorUniqueDelegate(vuProtocolID);

			// No delegate, or the messages are not handled by the ControllerStateMachine
			if (!vuDelegate || !vuDelegate->areHandledByControllerStateMachine(vuProtocolID))
			{
				return Error::MessageNotSupported;
			}
		}

		return [_bridge sendAecpCommand:std::move(aecpdu) handler:onResult];
	}

	virtual Error sendAecpResponse(Aecpdu::UniquePointer&& aecpdu) const noexcept override
	{
		auto const messageType = aecpdu->getMessageType();

		if (!AVDECC_ASSERT_WITH_RET(isAecpResponseMessageType(messageType), "Calling sendAecpResponse with a Command MessageType"))
		{
			return Error::MessageNotSupported;
		}

		// Special check for VendorUnique messages
		if (messageType == AecpMessageType::VendorUniqueResponse)
		{
			auto& vuAecp = static_cast<VuAecpdu&>(*aecpdu);

			auto const vuProtocolID = vuAecp.getProtocolIdentifier();
			auto* vuDelegate = getVendorUniqueDelegate(vuProtocolID);

			// No delegate, or the messages are not handled by the ControllerStateMachine
			if (!vuDelegate || !vuDelegate->areHandledByControllerStateMachine(vuProtocolID))
			{
				return Error::MessageNotSupported;
			}
		}

		return [_bridge sendAecpResponse:std::move(aecpdu)];
	}

	virtual Error sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, AcmpCommandResultHandler const& onResult) const noexcept override
	{
		return [_bridge sendAcmpCommand:std::move(acmpdu) handler:onResult];
	}

	virtual Error sendAcmpResponse(Acmpdu::UniquePointer&& acmpdu) const noexcept override
	{
		AVDECC_ASSERT(false, "TBD: To be implemented");
		return ProtocolInterface::Error::InternalError;
		//return [_bridge sendAcmpResponse:std::move(acmpdu)];
	}

#pragma mark stateMachine::ProtocolInterfaceDelegate overrides
	/* **** AECP notifications **** */
	virtual void onAecpCommand(la::avdecc::protocol::Aecpdu const& aecpdu) noexcept override
	{
		AVDECC_ASSERT(false, "Should never be called");
	}
	/* **** ACMP notifications **** */
	virtual void onAcmpCommand(la::avdecc::protocol::Acmpdu const& acmpdu) noexcept override
	{
		AVDECC_ASSERT(false, "Should never be called");
	}
	virtual void onAcmpResponse(la::avdecc::protocol::Acmpdu const& acmpdu) noexcept override
	{
		AVDECC_ASSERT(false, "Should never be called");
	}
	/* **** Sending methods **** */
	virtual ProtocolInterface::Error sendMessage(la::avdecc::protocol::Adpdu const& adpdu) const noexcept override
	{
		AVDECC_ASSERT(false, "Should never be called (if needed someday, just forward to _bridge");
		return ProtocolInterface::Error::InternalError;
	}
	virtual ProtocolInterface::Error sendMessage(la::avdecc::protocol::Aecpdu const& aecpdu) const noexcept override
	{
		AVDECC_ASSERT(false, "Should never be called (if needed someday, just forward to _bridge");
		return ProtocolInterface::Error::InternalError;
	}
	virtual ProtocolInterface::Error sendMessage(la::avdecc::protocol::Acmpdu const& acmpdu) const noexcept override
	{
		AVDECC_ASSERT(false, "Should never be called (if needed someday, just forward to _bridge");
		return ProtocolInterface::Error::InternalError;
	}
	/* *** Other methods **** */
	virtual std::uint32_t getVuAecpCommandTimeoutMsec(VuAecpdu::ProtocolIdentifier const& protocolIdentifier, la::avdecc::protocol::VuAecpdu const& aecpdu) const noexcept override
	{
		return getVuAecpCommandTimeout(protocolIdentifier, aecpdu);
	}

#pragma mark la::avdecc::utils::Subject overrides
	virtual void onObserverRegistered(observer_type* const observer) noexcept override
	{
		if (observer)
		{
			[_bridge notifyDiscoveredEntities:static_cast<ProtocolInterface::Observer&>(*observer)];
		}
	}

private:
#pragma mark Private Types
	struct VendorUniqueCommandInfo
	{
		AecpSequenceID sequenceID{ 0 };
		std::chrono::time_point<std::chrono::steady_clock> sendTime{};
		std::chrono::time_point<std::chrono::steady_clock> timeoutTime{};
		bool retried{ false };
		Aecpdu::UniquePointer command{ nullptr, nullptr };
		ProtocolInterface::AecpCommandResultHandler resultHandler{};

		VendorUniqueCommandInfo() {}
		VendorUniqueCommandInfo(AecpSequenceID const sequenceID, Aecpdu::UniquePointer&& command, ProtocolInterface::AecpCommandResultHandler const& resultHandler)
			: sequenceID(sequenceID)
			, command(std::move(command))
			, resultHandler(resultHandler)
		{
		}
	};
	using InflightVendorUniqueCommands = std::unordered_map<UniqueIdentifier, std::list<VendorUniqueCommandInfo>, UniqueIdentifier::hash>;
	struct CommandEntityInfo
	{
		entity::LocalEntity& entity;

		// VendorUnique variables
		InflightVendorUniqueCommands inflightVendorUniqueCommands{};

		/** Constructor */
		CommandEntityInfo(entity::LocalEntity& entity) noexcept
			: entity(entity)
		{
		}
	};
	using CommandEntities = std::unordered_map<UniqueIdentifier, CommandEntityInfo, UniqueIdentifier::hash>;

#pragma mark Private methods
	/*
	 * When using MacOS Native, it's up to the implementation to keep track of the message, the response, the timeout, the retry.
	 * This method will check for VendorUnique commands timeout expiracy and should be call regulary
	 */
	void checkVendorUniqueInflightCommandsTimeoutExpiracy() noexcept
	{
		// Lock
		auto const lg = std::lock_guard{ *this };

		// Get current time
		auto const now = std::chrono::steady_clock::now();

		// Iterate over all locally registered command entities
		for (auto& localEntityInfoKV : _commandEntities)
		{
			auto& localEntityInfo = localEntityInfoKV.second;

			// Check AECP commands
			for (auto& [targetEntityID, inflight] : localEntityInfo.inflightVendorUniqueCommands)
			{
				// Check all inflight timeouts
				for (auto it = inflight.begin(); it != inflight.end(); /* Iterate inside the loop */)
				{
					auto& command = *it;
					if (now > command.timeoutTime)
					{
						auto error = ProtocolInterface::Error::NoError;
						// Timeout expired, check if we retried yet
						//						if (!command.retried)
						//						{
						//							// Let's retry
						//							command.retried = true;

						//							// Update last send time
						//							inflight.lastSendTime = now;

						//							// Ask the transport layer to send the packet
						//							error = protocolInterface->sendMessage(static_cast<Aecpdu const&>(*command.command));

						//							// Reset command timeout
						//							resetAecpCommandTimeoutValue(command);

						//							// Statistics
						//							utils::invokeProtectedMethod(&Delegate::onAecpRetry, _delegate, targetEntityID);
						//							LOG_CONTROLLER_STATE_MACHINE_DEBUG(targetEntityID, std::string("AECP command with sequenceID ") + std::to_string(command.sequenceID) + " timed out, trying again");
						//						}
						//						else
						{
							error = ProtocolInterface::Error::Timeout;
						}

						if (!!error)
						{
							// Already retried, the command has been lost
							utils::invokeProtectedHandler(command.resultHandler, nullptr, error);
							it = inflight.erase(it);
						}
					}
					else
					{
						++it;
					}
				}
			}
		}
	}

	size_t getMaxInflightVendorUniqueMessages(UniqueIdentifier const& /*entityID*/) const noexcept
	{
		return DefaultMaxAecpInflightCommands;
	}
	std::chrono::milliseconds getVendorUniqueSendInterval(UniqueIdentifier const& entityID) const noexcept
	{
		return {};
	}

#pragma mark Private variables
	BridgeInterface* _bridge{ nullptr };
	mutable stateMachine::Manager _stateMachineManager{ this, this, nullptr, nullptr, nullptr }; // stateMachineManager only required to create the discovery thread (which will callback 'this')
	bool _shouldTerminate{ false };
	std::thread _stateMachineThread{}; // Can safely be declared here, will be joined during destruction
	CommandEntities _commandEntities{};
};

ProtocolInterfaceMacNative::ProtocolInterfaceMacNative(std::string const& networkInterfaceName)
	: ProtocolInterface(networkInterfaceName)
{
}

bool ProtocolInterfaceMacNative::isSupported() noexcept
{
	return [BridgeInterface isSupported];
}

ProtocolInterfaceMacNative* ProtocolInterfaceMacNative::createRawProtocolInterfaceMacNative(std::string const& networkInterfaceName)
{
	return new ProtocolInterfaceMacNativeImpl(networkInterfaceName);
}

} // namespace protocol
} // namespace avdecc
} // namespace la

#pragma mark - FromNative Declaration

@interface FromNative : NSObject
+ (la::avdecc::entity::Entity)makeEntity:(AVB17221Entity*)entity;
+ (la::avdecc::protocol::Aecpdu::UniquePointer)makeAecpdu:(AVB17221AECPMessage*)message toDestAddress:(la::networkInterface::MacAddress const&)destAddress withProtocolInterface:(la::avdecc::protocol::ProtocolInterfaceMacNativeImpl const&)pi;
+ (la::avdecc::protocol::Acmpdu::UniquePointer)makeAcmpdu:(AVB17221ACMPMessage*)message;
+ (la::networkInterface::MacAddress)makeMacAddress:(AVBMACAddress*)macAddress;
+ (la::avdecc::protocol::ProtocolInterface::Error)getProtocolError:(NSError*)error;
@end

#pragma mark - FromNative Implementation

@implementation FromNative
+ (la::networkInterface::MacAddress)getFirstMacAddress:(NSArray*)array {
	la::networkInterface::MacAddress mac;

	if (array.count > 0)
	{
		auto data = [(AVBMACAddress*)[array objectAtIndex:0] dataRepresentation];
		if (data.length == mac.size())
		{
			auto const* dataPtr = static_cast<decltype(mac)::value_type const*>(data.bytes);
			for (auto& v : mac)
			{
				v = *dataPtr;
				++dataPtr;
			}
		}
	}
	return mac;
}

+ (la::avdecc::entity::Entity)makeEntity:(AVB17221Entity*)entity {
	auto entityCaps = la::avdecc::entity::EntityCapabilities{};
	entityCaps.assign(static_cast<la::avdecc::entity::EntityCapabilities::underlying_value_type>(entity.entityCapabilities));
	auto talkerCaps = la::avdecc::entity::TalkerCapabilities{};
	talkerCaps.assign(static_cast<la::avdecc::entity::TalkerCapabilities::underlying_value_type>(entity.talkerCapabilities));
	auto listenerCaps = la::avdecc::entity::ListenerCapabilities{};
	listenerCaps.assign(static_cast<la::avdecc::entity::ListenerCapabilities::underlying_value_type>(entity.listenerCapabilities));
	auto controllerCaps = la::avdecc::entity::ControllerCapabilities{};
	controllerCaps.assign(static_cast<la::avdecc::entity::ControllerCapabilities::underlying_value_type>(entity.controllerCapabilities));
	auto controlIndex{ std::optional<la::avdecc::entity::model::ControlIndex>{} };
	auto associationID{ std::optional<la::avdecc::UniqueIdentifier>{} };
	auto avbInterfaceIndex{ la::avdecc::entity::Entity::GlobalAvbInterfaceIndex };
	auto gptpGrandmasterID{ std::optional<la::avdecc::UniqueIdentifier>{} };
	auto gptpDomainNumber{ std::optional<std::uint8_t>{} };

	if (entityCaps.test(la::avdecc::entity::EntityCapability::AemIdentifyControlIndexValid))
	{
		controlIndex = entity.identifyControlIndex;
	}
	if (entityCaps.test(la::avdecc::entity::EntityCapability::AssociationIDValid))
	{
		associationID = la::avdecc::UniqueIdentifier{ entity.associationID };
	}
	if (entityCaps.test(la::avdecc::entity::EntityCapability::AemInterfaceIndexValid))
	{
		avbInterfaceIndex = entity.interfaceIndex;
	}
	if (entityCaps.test(la::avdecc::entity::EntityCapability::GptpSupported))
	{
		gptpGrandmasterID = la::avdecc::UniqueIdentifier{ entity.gPTPGrandmasterID };
		gptpDomainNumber = entity.gPTPDomainNumber;
	}

	auto const commonInfo{ la::avdecc::entity::Entity::CommonInformation{ la::avdecc::UniqueIdentifier{ entity.entityID }, la::avdecc::UniqueIdentifier{ entity.entityModelID }, entityCaps, entity.talkerStreamSources, talkerCaps, entity.listenerStreamSinks, listenerCaps, controllerCaps, controlIndex, associationID } };
	auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ [FromNative getFirstMacAddress:entity.macAddresses], static_cast<std::uint8_t>(entity.timeToLive / 2u), entity.availableIndex, gptpGrandmasterID, gptpDomainNumber } };

	return la::avdecc::entity::Entity{ commonInfo, { { avbInterfaceIndex, interfaceInfo } } };
}

+ (la::avdecc::protocol::AemAecpdu::UniquePointer)makeAemAecpdu:(AVB17221AECPAEMMessage*)message toDestAddress:(la::networkInterface::MacAddress const&)destAddress isResponse:(bool)isResponse {
	auto aemAecpdu = la::avdecc::protocol::AemAecpdu::create(isResponse);
	auto& aem = static_cast<la::avdecc::protocol::AemAecpdu&>(*aemAecpdu);

	// Set Ether2 fields
	aem.setSrcAddress([FromNative makeMacAddress:message.sourceMAC]);
	aem.setDestAddress(destAddress);

	// Set AECP fields
	aem.setStatus(la::avdecc::protocol::AecpStatus{ message.status });
	aem.setTargetEntityID(la::avdecc::UniqueIdentifier{ message.targetEntityID });
	aem.setControllerEntityID(la::avdecc::UniqueIdentifier{ message.controllerEntityID });
	aem.setSequenceID(message.sequenceID);

	// Set AEM fields
	aem.setUnsolicited(message.isUnsolicited);
	aem.setCommandType(la::avdecc::protocol::AemCommandType{ message.commandType });
	if (message.commandSpecificData.length != 0)
	{
		aem.setCommandSpecificData(message.commandSpecificData.bytes, message.commandSpecificData.length);
	}

	return aemAecpdu;
}

+ (la::avdecc::protocol::AaAecpdu::UniquePointer)makeAaAecpdu:(AVB17221AECPAddressAccessMessage*)message toDestAddress:(la::networkInterface::MacAddress const&)destAddress isResponse:(bool)isResponse {
	auto aaAecpdu = la::avdecc::protocol::AaAecpdu::create(isResponse);
	auto& aa = static_cast<la::avdecc::protocol::AaAecpdu&>(*aaAecpdu);

	// Set Ether2 fields
	aa.setSrcAddress([FromNative makeMacAddress:message.sourceMAC]);
	aa.setDestAddress(destAddress);

	// Set AECP fields
	aa.setStatus(la::avdecc::protocol::AecpStatus(message.status));
	aa.setTargetEntityID(la::avdecc::UniqueIdentifier{ message.targetEntityID });
	aa.setControllerEntityID(la::avdecc::UniqueIdentifier{ message.controllerEntityID });
	aa.setSequenceID(message.sequenceID);

	// Set Address Access fields
	for (AVB17221AECPAddressAccessTLV* tlv in message.tlvs)
	{
		aa.addTlv(la::avdecc::entity::addressAccess::Tlv{ tlv.address, static_cast<la::avdecc::protocol::AaMode>(tlv.mode), tlv.memoryData.bytes, tlv.memoryData.length });
	}

	return aaAecpdu;
}

+ (la::avdecc::protocol::VuAecpdu::UniquePointer)makeMvuAecpdu:(AVB17221AECPVendorMessage*)message toDestAddress:(la::networkInterface::MacAddress const&)destAddress isResponse:(bool)isResponse withVendorUniqueDelegate:(la::avdecc::protocol::ProtocolInterface::VendorUniqueDelegate*)vuDelegate {
	auto vuAecpdu = vuDelegate->createAecpdu(la::avdecc::protocol::MvuAecpdu::ProtocolID, isResponse);
	if (vuAecpdu)
	{
		auto& mvu = static_cast<la::avdecc::protocol::MvuAecpdu&>(*vuAecpdu);

		// Set Ether2 fields
		mvu.setSrcAddress([FromNative makeMacAddress:message.sourceMAC]);
		mvu.setDestAddress(destAddress);

		// Set AECP fields
		mvu.setStatus(la::avdecc::protocol::AecpStatus{ message.status });
		mvu.setTargetEntityID(la::avdecc::UniqueIdentifier{ message.targetEntityID });
		mvu.setControllerEntityID(la::avdecc::UniqueIdentifier{ message.controllerEntityID });
		mvu.setSequenceID(message.sequenceID);

		auto const bytesLen = message.protocolSpecificData.length;
		if (bytesLen >= la::avdecc::protocol::MvuAecpdu::HeaderLength)
		{
			// Set MVU fields
			auto const* const bytes = static_cast<std::uint8_t const*>(message.protocolSpecificData.bytes);
			auto pos = decltype(bytesLen){ 0 };

			// Skip reserved field
			++pos;

			// Read CommandType
			auto commandType = *reinterpret_cast<la::avdecc::protocol::MvuCommandType const*>(bytes + pos);
			++pos;
			mvu.setCommandType(commandType);

			// Read CommandSpecificData
			mvu.setCommandSpecificData(reinterpret_cast<void const*>(bytes + pos), bytesLen - pos);

			return vuAecpdu;
		}
	}
	return la::avdecc::protocol::VuAecpdu::UniquePointer{ nullptr, nullptr };
}

+ (la::avdecc::protocol::VuAecpdu::UniquePointer)makeVendorUniqueAecpdu:(AVB17221AECPVendorMessage*)message toDestAddress:(la::networkInterface::MacAddress const&)destAddress isResponse:(bool)isResponse withProtocolInterface:(la::avdecc::protocol::ProtocolInterfaceMacNativeImpl const&)pi {
	auto const vuProtocolID = la::avdecc::protocol::VuAecpdu::ProtocolIdentifier{ message.protocolID };
	auto* vuDelegate = pi.getVendorUniqueDelegate(vuProtocolID);
	if (vuDelegate)
	{
		if (vuProtocolID == la::avdecc::protocol::MvuAecpdu::ProtocolID)
		{
			return [FromNative makeMvuAecpdu:message toDestAddress:destAddress isResponse:isResponse withVendorUniqueDelegate:vuDelegate];
		}
	}
	return la::avdecc::protocol::VuAecpdu::UniquePointer{ nullptr, nullptr };
}

+ (la::avdecc::protocol::Aecpdu::UniquePointer)makeAecpdu:(AVB17221AECPMessage*)message toDestAddress:(la::networkInterface::MacAddress const&)destAddress withProtocolInterface:(la::avdecc::protocol::ProtocolInterfaceMacNativeImpl const&)pi {
	switch ([message messageType])
	{
		case AVB17221AECPMessageTypeAEMCommand:
			return [FromNative makeAemAecpdu:static_cast<AVB17221AECPAEMMessage*>(message) toDestAddress:destAddress isResponse:false];
		case AVB17221AECPMessageTypeAEMResponse:
			return [FromNative makeAemAecpdu:static_cast<AVB17221AECPAEMMessage*>(message) toDestAddress:destAddress isResponse:true];
		case AVB17221AECPMessageTypeAddressAccessCommand:
			return [FromNative makeAaAecpdu:static_cast<AVB17221AECPAddressAccessMessage*>(message) toDestAddress:destAddress isResponse:false];
		case AVB17221AECPMessageTypeAddressAccessResponse:
			return [FromNative makeAaAecpdu:static_cast<AVB17221AECPAddressAccessMessage*>(message) toDestAddress:destAddress isResponse:true];
		case AVB17221AECPMessageTypeVendorUniqueCommand:
			return [FromNative makeVendorUniqueAecpdu:static_cast<AVB17221AECPVendorMessage*>(message) toDestAddress:destAddress isResponse:false withProtocolInterface:pi];
		case AVB17221AECPMessageTypeVendorUniqueResponse:
			return [FromNative makeVendorUniqueAecpdu:static_cast<AVB17221AECPVendorMessage*>(message) toDestAddress:destAddress isResponse:true withProtocolInterface:pi];
		default:
			AVDECC_ASSERT(false, "Unhandled AECP message type");
			break;
	}
	return { nullptr, nullptr };
}

+ (la::avdecc::protocol::Acmpdu::UniquePointer)makeAcmpdu:(AVB17221ACMPMessage*)message {
	auto acmpdu = la::avdecc::protocol::Acmpdu::create();
	auto& acmp = static_cast<la::avdecc::protocol::Acmpdu&>(*acmpdu);

	// Set Ether2 fields
#pragma message("TBD: Find a way to retrieve these information")
	//aem.setSrcAddress();
	//aem.setDestAddress();

	// Set ACMP fields
	auto flags = la::avdecc::entity::ConnectionFlags{};
	flags.assign(static_cast<la::avdecc::entity::ConnectionFlags::underlying_value_type>(message.flags));
	acmp.setMessageType(la::avdecc::protocol::AcmpMessageType(message.messageType));
	acmp.setStatus(la::avdecc::protocol::AcmpStatus(message.status));
	acmp.setStreamID(message.streamID);
	acmp.setControllerEntityID(la::avdecc::UniqueIdentifier{ message.controllerEntityID });
	acmp.setTalkerEntityID(la::avdecc::UniqueIdentifier{ message.talkerEntityID });
	acmp.setListenerEntityID(la::avdecc::UniqueIdentifier{ message.listenerEntityID });
	acmp.setTalkerUniqueID(message.talkerUniqueID);
	acmp.setListenerUniqueID(message.listenerUniqueID);
	acmp.setStreamDestAddress([FromNative makeMacAddress:message.destinationMAC]);
	acmp.setConnectionCount(message.connectionCount);
	acmp.setSequenceID(message.sequenceID);
	acmp.setFlags(flags);
	acmp.setStreamVlanID(message.vlanID);

	return acmpdu;
}

+ (la::networkInterface::MacAddress)makeMacAddress:(AVBMACAddress*)macAddress {
	la::networkInterface::MacAddress mac;
	auto const* data = [macAddress dataRepresentation];
	auto const bufferSize = mac.size() * sizeof(la::networkInterface::MacAddress::value_type);

	if (data.length == bufferSize)
		memcpy(mac.data(), data.bytes, bufferSize);

	return mac;
}

+ (la::avdecc::protocol::ProtocolInterface::Error)getProtocolError:(NSError*)error {
	if ([[error domain] isEqualToString:AVBErrorDomain])
	{
		auto const code = IOReturn(error.code);
		switch (code)
		{
			case kIOReturnTimeout:
				return la::avdecc::protocol::ProtocolInterface::Error::Timeout;
			case kIOReturnExclusiveAccess:
				return la::avdecc::protocol::ProtocolInterface::Error::DuplicateLocalEntityID;
			case kIOReturnNotFound:
				return la::avdecc::protocol::ProtocolInterface::Error::UnknownLocalEntity;
			case kIOReturnOffline:
				return la::avdecc::protocol::ProtocolInterface::Error::TransportError;
			case kIOReturnBadArgument:
				return la::avdecc::protocol::ProtocolInterface::Error::InternalError;
			default:
				NSLog(@"Not handled IOReturn error code: %x\n", code);
				AVDECC_ASSERT(false, "Not handled error code");
				return la::avdecc::protocol::ProtocolInterface::Error::TransportError;
		}
	}

	return la::avdecc::protocol::ProtocolInterface::Error::InternalError;
}

@end

#pragma mark - ToNative Declaration

@interface ToNative : NSObject
+ (AVB17221Entity*)makeAVB17221Entity:(la::avdecc::entity::Entity const&)entity interfaceIndex:(la::avdecc::entity::model::AvbInterfaceIndex)interfaceIndex;
+ (AVB17221AECPMessage*)makeAecpMessage:(la::avdecc::protocol::Aecpdu const&)message;
+ (AVBMACAddress*)makeAVBMacAddress:(la::networkInterface::MacAddress const&)macAddress;
@end

#pragma mark - ToNative Implementation

@implementation ToNative
+ (AVB17221Entity*)makeAVB17221Entity:(la::avdecc::entity::Entity const&)entity interfaceIndex:(la::avdecc::entity::model::AvbInterfaceIndex)interfaceIndex {
	auto& interfaceInfo = entity.getInterfaceInformation(interfaceIndex);
	auto entityCaps{ entity.getEntityCapabilities() };
	auto identifyControlIndex{ la::avdecc::entity::model::ControlIndex{ 0u } };
	auto avbInterfaceIndex{ la::avdecc::entity::model::AvbInterfaceIndex{ 0u } };
	auto associationID{ la::avdecc::UniqueIdentifier::getNullUniqueIdentifier() };
	auto gptpGrandmasterID{ la::avdecc::UniqueIdentifier::getNullUniqueIdentifier() };
	auto gptpDomainNumber{ std::uint8_t{ 0u } };

	if (entity.getIdentifyControlIndex())
	{
		entityCaps.set(la::avdecc::entity::EntityCapability::AemIdentifyControlIndexValid);
		identifyControlIndex = *entity.getIdentifyControlIndex();
	}
	else
	{
		// We don't have a valid IdentifyControlIndex, don't set the flag
		entityCaps.reset(la::avdecc::entity::EntityCapability::AemIdentifyControlIndexValid);
	}

	if (interfaceIndex != la::avdecc::entity::Entity::GlobalAvbInterfaceIndex)
	{
		entityCaps.set(la::avdecc::entity::EntityCapability::AemInterfaceIndexValid);
		avbInterfaceIndex = interfaceIndex;
	}
	else
	{
		// We don't have a valid AvbInterfaceIndex, don't set the flag
		entityCaps.reset(la::avdecc::entity::EntityCapability::AemInterfaceIndexValid);
	}

	if (entity.getAssociationID())
	{
		entityCaps.set(la::avdecc::entity::EntityCapability::AssociationIDValid);
		associationID = *entity.getAssociationID();
	}
	else
	{
		// We don't have a valid AssociationID, don't set the flag
		entityCaps.reset(la::avdecc::entity::EntityCapability::AssociationIDValid);
	}

	if (interfaceInfo.gptpGrandmasterID)
	{
		entityCaps.set(la::avdecc::entity::EntityCapability::GptpSupported);
		gptpGrandmasterID = *interfaceInfo.gptpGrandmasterID;
		if (AVDECC_ASSERT_WITH_RET(interfaceInfo.gptpDomainNumber, "gptpDomainNumber should be set when gptpGrandmasterID is set"))
		{
			gptpDomainNumber = *interfaceInfo.gptpDomainNumber;
		}
	}
	else
	{
		// We don't have a valid gptpGrandmasterID value, don't set the flag
		entityCaps.reset(la::avdecc::entity::EntityCapability::GptpSupported);
	}

	auto e = [[AVB17221Entity alloc] init];

	e.entityID = entity.getEntityID();
	e.entityModelID = entity.getEntityModelID();
	e.entityCapabilities = static_cast<AVB17221ADPEntityCapabilities>(entityCaps.value());
	e.talkerStreamSources = entity.getTalkerStreamSources();
	e.talkerCapabilities = static_cast<AVB17221ADPTalkerCapabilities>(entity.getTalkerCapabilities().value());
	e.listenerStreamSinks = entity.getListenerStreamSinks();
	e.listenerCapabilities = static_cast<AVB17221ADPListenerCapabilities>(entity.getListenerCapabilities().value());
	e.controllerCapabilities = static_cast<AVB17221ADPControllerCapabilities>(entity.getControllerCapabilities().value());
	e.identifyControlIndex = identifyControlIndex;
	e.interfaceIndex = avbInterfaceIndex;
	e.associationID = associationID;
	e.gPTPGrandmasterID = gptpGrandmasterID;
	e.gPTPDomainNumber = gptpDomainNumber;
	e.timeToLive = interfaceInfo.validTime * 2u;

	e.availableIndex = 0; // AvailableIndex is automatically handled by macOS APIs
	return e;
}

+ (AVB17221AECPAEMMessage*)makeAemMessage:(la::avdecc::protocol::AemAecpdu const&)aecpdu isResponse:(bool)isResponse {
	auto* message = static_cast<AVB17221AECPAEMMessage*>(nullptr);

	if (isResponse)
		message = [AVB17221AECPAEMMessage responseMessage];
	else
		message = [AVB17221AECPAEMMessage commandMessage];

	// Set Aem specific fields
	message.unsolicited = FALSE;
	message.controllerRequest = FALSE;
	message.commandType = static_cast<AVB17221AEMCommandType>(aecpdu.getCommandType().getValue());
	auto const payloadInfo = aecpdu.getPayload();
	auto const* const payload = payloadInfo.first;
	if (payload != nullptr)
	{
		message.commandSpecificData = [NSData dataWithBytes:payload length:payloadInfo.second];
	}

	// Set common fields
	message.status = AVB17221AECPStatusSuccess;
	message.targetEntityID = aecpdu.getTargetEntityID();
	message.controllerEntityID = aecpdu.getControllerEntityID();
	message.sequenceID = aecpdu.getSequenceID();

	return message;
}

+ (AVB17221AECPAddressAccessMessage*)makeAaMessage:(la::avdecc::protocol::AaAecpdu const&)aecpdu isResponse:(bool)isResponse {
	auto* message = static_cast<AVB17221AECPAddressAccessMessage*>(nullptr);

	if (isResponse)
		message = [AVB17221AECPAddressAccessMessage responseMessage];
	else
		message = [AVB17221AECPAddressAccessMessage commandMessage];

	// Set AA specific fields
	auto* tlvs = [[NSMutableArray alloc] init];
	for (auto const& tlv : aecpdu.getTlvData())
	{
		auto* t = [[AVB17221AECPAddressAccessTLV alloc] init];
		t.mode = static_cast<AVB17221AECPAddressAccessTLVMode>(tlv.getMode().getValue());
		t.address = tlv.getAddress();
		t.memoryData = [NSData dataWithBytes:tlv.getMemoryData().data() length:tlv.getMemoryData().size()];
		[tlvs addObject:t];
	}
	message.tlvs = tlvs;

	// Set common fields
	message.status = AVB17221AECPStatusSuccess;
	message.targetEntityID = aecpdu.getTargetEntityID();
	message.controllerEntityID = aecpdu.getControllerEntityID();
	message.sequenceID = aecpdu.getSequenceID();

	return message;
}

+ (AVB17221AECPVendorMessage*)makeVendorUniqueMessage:(la::avdecc::protocol::VuAecpdu const&)aecpdu isResponse:(bool)isResponse {
	auto const message = [[AVB17221AECPVendorMessage alloc] init];
#if !__has_feature(objc_arc)
	[message autorelease];
#endif

	// Set Vendor Unique specific fields
	{
		message.protocolID = static_cast<la::avdecc::protocol::VuAecpdu::ProtocolIdentifier::IntegralType>(aecpdu.getProtocolIdentifier());
	}
	{
		// Use VuAecpdu serialization to construct the vendor specific payload
		la::avdecc::protocol::SerializationBuffer buffer;
		la::avdecc::protocol::serialize<la::avdecc::protocol::VuAecpdu>(aecpdu, buffer);

		// Copy the payload to commandSpecificData, minus the header
		auto constexpr HeaderLength = la::avdecc::protocol::Aecpdu::HeaderLength + la::avdecc::protocol::VuAecpdu::HeaderLength;
		auto const* const payloadData = buffer.data();
		auto const payloadLength = buffer.size();
		if (payloadLength > HeaderLength)
		{
			message.protocolSpecificData = [NSData dataWithBytes:(payloadData + HeaderLength) length:payloadLength - HeaderLength];
		}
	}

	// Set common fields
	message.messageType = isResponse ? AVB17221AECPMessageTypeVendorUniqueResponse : AVB17221AECPMessageTypeVendorUniqueCommand;
	message.status = AVB17221AECPStatusSuccess;
	message.targetEntityID = aecpdu.getTargetEntityID();
	message.controllerEntityID = aecpdu.getControllerEntityID();
	message.sequenceID = aecpdu.getSequenceID();

	return message;
}

+ (AVB17221AECPMessage*)makeAecpMessage:(la::avdecc::protocol::Aecpdu const&)message {
	switch (static_cast<AVB17221AECPMessageType>(message.getMessageType().getValue()))
	{
		case AVB17221AECPMessageTypeAEMCommand:
			return [ToNative makeAemMessage:static_cast<la::avdecc::protocol::AemAecpdu const&>(message) isResponse:false];
		case AVB17221AECPMessageTypeAEMResponse:
			return [ToNative makeAemMessage:static_cast<la::avdecc::protocol::AemAecpdu const&>(message) isResponse:true];
		case AVB17221AECPMessageTypeAddressAccessCommand:
			return [ToNative makeAaMessage:static_cast<la::avdecc::protocol::AaAecpdu const&>(message) isResponse:false];
		case AVB17221AECPMessageTypeAddressAccessResponse:
			return [ToNative makeAaMessage:static_cast<la::avdecc::protocol::AaAecpdu const&>(message) isResponse:true];
		case AVB17221AECPMessageTypeVendorUniqueCommand:
			return [ToNative makeVendorUniqueMessage:static_cast<la::avdecc::protocol::VuAecpdu const&>(message) isResponse:false];
		case AVB17221AECPMessageTypeVendorUniqueResponse:
			return [ToNative makeVendorUniqueMessage:static_cast<la::avdecc::protocol::VuAecpdu const&>(message) isResponse:true];
		default:
			AVDECC_ASSERT(false, "Unhandled AECP message type");
			break;
	}
	return NULL;
}

+ (AVBMACAddress*)makeAVBMacAddress:(la::networkInterface::MacAddress const&)macAddress {
	auto* mac = [[AVBMACAddress alloc] initWithBytes:macAddress.data()];
#if !__has_feature(objc_arc)
	[mac autorelease];
#endif
	return mac;
}

@end

#pragma mark - BridgeInterface Implementation
@implementation BridgeInterface

static constexpr auto AVB17221EntityPropertyImmutableMask = AVB17221EntityPropertyChangedModelID | AVB17221EntityPropertyChangedTalkerCapabilities | AVB17221EntityPropertyChangedTalkerStreamSources | AVB17221EntityPropertyChangedListenerCapabilities | AVB17221EntityPropertyChangedListenerStreamSinks | AVB17221EntityPropertyChangedControllerCapabilities | AVB17221EntityPropertyChangedIdentifyControlIndex;

- (EntityQueues const&)createQueuesForRemoteEntity:(la::avdecc::UniqueIdentifier)entityID {
	EntityQueues eq;
	eq.aecpQueue = dispatch_queue_create([[NSString stringWithFormat:@"%@.0x%016llx.aecp", [self className], entityID.getValue()] UTF8String], 0);
	eq.aecpLimiter = dispatch_semaphore_create(la::avdecc::protocol::DefaultMaxAecpInflightCommands);
	return _entityQueues[entityID] = std::move(eq);
}

+ (BOOL)isSupported {
	if ([NSProcessInfo instancesRespondToSelector:@selector(isOperatingSystemAtLeastVersion:)])
	{
		// Minimum required version is macOS 10.15.0 (Catalina)
		return [[NSProcessInfo processInfo] isOperatingSystemAtLeastVersion:{ 10, 15, 0 }];
	}

	return FALSE;
}

/** std::string to NSString conversion */
+ (NSString*)getNSString:(std::string const&)cString {
	return [NSString stringWithCString:cString.c_str() encoding:NSUTF8StringEncoding];
}

/** NSString to std::string conversion */
+ (std::string)getStdString:(NSString*)nsString {
	return std::string{ [nsString UTF8String] };
}

+ (NSString*)getEntityCapabilities:(AVB17221Entity*)entity {
	return [NSString stringWithFormat:@"%@ %@ %@", (entity.talkerCapabilities & AVB17221ADPTalkerCapabilitiesImplemented) ? @"Talker" : @"", (entity.listenerCapabilities & AVB17221ADPListenerCapabilitiesImplemented) ? @"Listener" : @"", (entity.controllerCapabilities & AVB17221ADPControllerCapabilitiesImplemented) ? @"Controller" : @""];
}

- (void)startAsyncOperation {
	std::lock_guard<decltype(_lockPending)> const lg(_lockPending);
	_pendingCommands++;
}

- (void)stopAsyncOperation {
	{
		std::lock_guard<decltype(_lockPending)> const lg(_lockPending);
		AVDECC_ASSERT(_pendingCommands > 0, "Trying to stop async operation, but there is no pending operation");
		_pendingCommands--;
	}
	_pendingCondVar.notify_all();
}

- (void)waitAsyncOperations {
	// Wait for all remaining async operations to complete
	std::unique_lock<decltype(_lockPending)> sync_lg(_lockPending);
	_pendingCondVar.wait(sync_lg,
		[self]
		{
			return _pendingCommands == 0;
		});
	AVDECC_ASSERT(_pendingCommands == 0, "Waited for pending operations to complete, but there is some remaining one!");
}

- (std::optional<la::avdecc::entity::model::AvbInterfaceIndex>)getMatchingInterfaceIndex:(la::avdecc::entity::LocalEntity const&)entity {
	auto avbInterfaceIndex = std::optional<la::avdecc::entity::model::AvbInterfaceIndex>{ std::nullopt };
	auto const& macAddress = _protocolInterface->getMacAddress();

	for (auto const& [interfaceIndex, interfaceInfo] : entity.getInterfacesInformation())
	{
		if (interfaceInfo.macAddress == macAddress)
		{
			avbInterfaceIndex = interfaceIndex;
			break;
		}
	}

	return avbInterfaceIndex;
}

/** Initializer */
- (id)initWithInterfaceName:(NSString*)interfaceName andProtocolInterface:(la::avdecc::protocol::ProtocolInterfaceMacNativeImpl*)protocolInterface {
	self = [super init];
	if (self)
	{
		_primedDiscovery = FALSE;
		_protocolInterface = protocolInterface;
		_pendingCommands = 0u;
		_deleterQueue = dispatch_queue_create([[NSString stringWithFormat:@"%@.deleterQueue", [self className]] UTF8String], 0);
		self.interface = [[AVBEthernetInterface alloc] initWithInterfaceName:interfaceName];
		self.interface.entityDiscovery.discoveryDelegate = self;
	}
	return self;
}

/** Deinit method to shutdown every pending operations */
- (void)deinit {
	// Remove discovery delegate
	self.interface.entityDiscovery.discoveryDelegate = nil;

	decltype(_localProcessEntities) localProcessEntities;
	decltype(_registeredAcmpHandlers) registeredAcmpHandlers;
	// Move internal lists to temporary objects while locking, so we can safely cleanup outside of the lock
	{
		// Lock
		auto const lg = std::lock_guard{ _lock };
		localProcessEntities = std::move(_localProcessEntities);
		registeredAcmpHandlers = std::move(_registeredAcmpHandlers);
		_localMachineEntities.clear();
		_remoteEntities.clear();
	}

	// Remove Local Entities that were not removed
	for (auto const& entityKV : localProcessEntities)
	{
		auto const& entity = entityKV.second;
		// Remove remaining handlers
		[self removeLocalProcessEntityHandlers:entity];
		// Disable advertising
		[self disableEntityAdvertising:entity];
	}

	// Remove ACMP handlers that were not removed
	for (auto const& entityID : registeredAcmpHandlers)
	{
		[self deinitEntity:entityID];
	}

	// Wait for remaining pending operations
	[self waitAsyncOperations];

	if (_deleterQueue)
	{
		// Synchronize the queue using an empty block
		dispatch_sync(_deleterQueue, ^{
									});
#if !__has_feature(objc_arc)
		// Release the objects
		dispatch_release(_deleterQueue);
#endif
		_deleterQueue = nil;
	}

	// Release 1722.1 interface
	self.interface = nil;
}

/** Destructor */
- (void)dealloc {
	[self deinit];
#if !__has_feature(objc_arc)
	[super dealloc];
#endif
}

#pragma mark la::avdecc::protocol::ProtocolInterface bridge methods
- (void)notifyDiscoveredEntities:(la::avdecc::protocol::ProtocolInterface::Observer&)obs {
	auto const lg = std::lock_guard{ _lock };

	for (auto const& [entityID, entity] : _localProcessEntities)
	{
		la::avdecc::utils::invokeProtectedMethod(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityOnline, &obs, _protocolInterface, entity);
	}

	for (auto const& [entityID, entity] : _localMachineEntities)
	{
		la::avdecc::utils::invokeProtectedMethod(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityOnline, &obs, _protocolInterface, entity);
	}

	for (auto const& [entityID, entity] : _remoteEntities)
	{
		la::avdecc::utils::invokeProtectedMethod(&la::avdecc::protocol::ProtocolInterface::Observer::onRemoteEntityOnline, &obs, _protocolInterface, entity);
	}
}

- (la::avdecc::UniqueIdentifier)getDynamicEID {
	return la::avdecc::UniqueIdentifier{ [AVBCentralManager nextAvailableDynamicEntityID] };
}

- (void)releaseDynamicEID:(la::avdecc::UniqueIdentifier)entityID {
	[AVBCentralManager releaseDynamicEntityID:entityID];
}

// Registration of a local process entity (an entity declared inside this process, not all local computer entities)
- (la::avdecc::protocol::ProtocolInterface::Error)registerLocalEntity:(la::avdecc::entity::LocalEntity&)entity {
	// Lock entities now, so we don't get interrupted during registration
	auto const lg = std::lock_guard{ _lock };

	auto const entityID = entity.getEntityID();

	// Entity is controller capable
	if (entity.getControllerCapabilities().test(la::avdecc::entity::ControllerCapability::Implemented))
	{
		// Set a handler to monitor AECP Command messages
		if ([self.interface.aecp setCommandHandler:self forEntityID:entityID] == NO)
		{
			return la::avdecc::protocol::ProtocolInterface::Error::DuplicateLocalEntityID;
		}
		// Set a handler to monitor AECP Response messages, we are interested in unsolicited notifications and VendorUnique responses
		if ([self.interface.aecp setResponseHandler:self forControllerEntityID:entityID] == NO)
		{
			return la::avdecc::protocol::ProtocolInterface::Error::DuplicateLocalEntityID;
		}
	}
	// Other types are not supported right now
	else
	{
		return la::avdecc::protocol::ProtocolInterface::Error::InvalidEntityType;
	}

	// Add the entity to our cache of local entities declared by the running program
	_localProcessEntities.insert(decltype(_localProcessEntities)::value_type(entityID, entity));
	_protocolInterface->addLocalEntity(entity);

	// Notify observers
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityOnline, _protocolInterface, entity);

	return la::avdecc::protocol::ProtocolInterface::Error::NoError;
}

// Remove handlers for a local process entity
- (void)removeLocalProcessEntityHandlers:(la::avdecc::entity::LocalEntity const&)entity {
	auto const entityID = entity.getEntityID();

	// Entity is controller capable
	if (entity.getControllerCapabilities().test(la::avdecc::entity::ControllerCapability::Implemented))
	{
		// Remove handlers
		dispatch_async(_deleterQueue, ^{
			[self.interface.aecp removeCommandHandlerForEntityID:entityID];
			[self.interface.aecp removeResponseHandlerForControllerEntityID:entityID];
		});
	}
}

// Unregistration of a local process entity
- (la::avdecc::protocol::ProtocolInterface::Error)unregisterLocalEntity:(la::avdecc::entity::LocalEntity const&)entity {
	auto const entityID = entity.getEntityID();

	// Remove handlers
	[self removeLocalProcessEntityHandlers:entity];

	// Disable advertising
	[self disableEntityAdvertising:entity];

	// Lock entities now that we have removed the handlers
	auto const lg = std::lock_guard{ _lock };

	// Remove the entity from our cache of local entities declared by the running program
	_localProcessEntities.erase(entityID);
	_protocolInterface->removeLocalEntity(entityID);

	// Notify observers
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityOffline, _protocolInterface, entityID);

	return la::avdecc::protocol::ProtocolInterface::Error::NoError;
}

- (la::avdecc::protocol::ProtocolInterface::Error)injectRawPacket:(la::avdecc::MemoryBuffer&&)packet {
#pragma message("TODO: Implement injectRawPacket")
	return la::avdecc::protocol::ProtocolInterface::Error::InternalError;
}

- (la::avdecc::protocol::ProtocolInterface::Error)setEntityNeedsAdvertise:(const la::avdecc::entity::LocalEntity&)entity flags:(la::avdecc::entity::LocalEntity::AdvertiseFlags)flags {
	NSError* error{ nullptr };

	// Change in GrandMaster
	if (flags.test(la::avdecc::entity::LocalEntity::AdvertiseFlag::GptpGrandmasterID))
	{
		auto const interfaceIndex = [self getMatchingInterfaceIndex:entity];
		if (!AVDECC_ASSERT_WITH_RET(interfaceIndex, "Should always have a matching AvbInterfaceIndex when this method is called"))
		{
			return la::avdecc::protocol::ProtocolInterface::Error::InvalidParameters;
		}

		auto const& interfaceInfo = entity.getInterfaceInformation(*interfaceIndex);
		if (interfaceInfo.gptpGrandmasterID)
		{
			[self.interface.entityDiscovery changeEntityWithEntityID:entity.getEntityID() toNewGPTPGrandmasterID:*interfaceInfo.gptpGrandmasterID error:&error];
			if (error != nullptr)
				return [FromNative getProtocolError:error];
		}
	}
	return la::avdecc::protocol::ProtocolInterface::Error::NoError;
}

- (la::avdecc::protocol::ProtocolInterface::Error)enableEntityAdvertising:(la::avdecc::entity::LocalEntity const&)entity {
	NSError* error{ nullptr };

	auto const interfaceIndex = [self getMatchingInterfaceIndex:entity];
	if (!AVDECC_ASSERT_WITH_RET(interfaceIndex, "Should always have a matching AvbInterfaceIndex when this method is called"))
	{
		return la::avdecc::protocol::ProtocolInterface::Error::InvalidParameters;
	}

	[self.interface.entityDiscovery addLocalEntity:[ToNative makeAVB17221Entity:entity interfaceIndex:*interfaceIndex] error:&error];
	if (error != nullptr)
		return [FromNative getProtocolError:error];

	return la::avdecc::protocol::ProtocolInterface::Error::NoError;
}

- (la::avdecc::protocol::ProtocolInterface::Error)disableEntityAdvertising:(la::avdecc::entity::LocalEntity const&)entity {
	NSError* error{ nullptr };

	[self.interface.entityDiscovery removeLocalEntity:entity.getEntityID() error:&error];
	if (error != nullptr)
		return [FromNative getProtocolError:error];

	return la::avdecc::protocol::ProtocolInterface::Error::NoError;
}

- (BOOL)discoverRemoteEntities {
	if (!_primedDiscovery)
	{
		[self.interface.entityDiscovery primeIterators];
		_primedDiscovery = TRUE;
		return TRUE;
	}
	return [self.interface.entityDiscovery discoverEntities];
}

- (BOOL)discoverRemoteEntity:(la::avdecc::UniqueIdentifier)entityID {
	if (!_primedDiscovery)
	{
		[self.interface.entityDiscovery primeIterators];
		_primedDiscovery = TRUE;
		return TRUE;
	}
	return [self.interface.entityDiscovery discoverEntity:entityID];
}

- (la::avdecc::protocol::ProtocolInterface::Error)sendAecpCommand:(la::avdecc::protocol::Aecpdu::UniquePointer&&)aecpdu handler:(la::avdecc::protocol::ProtocolInterface::AecpCommandResultHandler const&)onResult {
	auto const macAddr = aecpdu->getDestAddress(); // Make a copy of the target macAddress so it can safely be used inside the objC block
	__block auto resultHandler = onResult; // Make a copy of the handler so it can safely be used inside the objC block. Declare it as __block so we can modify it from the block (to fix a bug that macOS sometimes call the completionHandler twice)

	auto message = [ToNative makeAecpMessage:*aecpdu];
	if (message != NULL)
	{
		decltype(EntityQueues::aecpQueue) queue;
		// Only take the lock while searching for the queue, we want to release it before invoking dispath_async to prevent a deadlock
		{
			std::lock_guard<decltype(_lockQueues)> const lg(_lockQueues);
			auto eqIt = _entityQueues.find(la::avdecc::UniqueIdentifier{ message.targetEntityID });
			if (eqIt == _entityQueues.end())
			{
				queue = [self createQueuesForRemoteEntity:la::avdecc::UniqueIdentifier{ message.targetEntityID }].aecpQueue;
			}
			else
			{
				queue = eqIt->second.aecpQueue;
			}
		}

		__block auto sentAecpdu = std::move(aecpdu); // Move the aecpdu to a __block variable so it can safely be used inside the objC block.
		dispatch_async(queue, ^{
			dispatch_semaphore_t limiter;
			{
				std::lock_guard<decltype(_lockQueues)> const lg(_lockQueues);
				auto eqIt = _entityQueues.find(la::avdecc::UniqueIdentifier{ message.targetEntityID });
				if (eqIt == _entityQueues.end())
				{
					// Entity no longer registered, ignore this command and return
					return;
				}
				limiter = eqIt->second.aecpLimiter; // We can store the limiter here, we know the queue and semaphore exists until all scheduled blocks on this queue are finished (thanks to the dispatch_sync call)
			}

			// Take a semaphore count to limit the inflight commands
			dispatch_semaphore_wait(limiter, DISPATCH_TIME_FOREVER);

			[self startAsyncOperation];
			[self.interface.aecp sendCommand:message
													toMACAddress:[ToNative makeAVBMacAddress:macAddr]
										 completionHandler:^(NSError* error, AVB17221AECPMessage* message) {
											 if (!resultHandler)
											 {
												 LOG_PROTOCOL_INTERFACE_DEBUG(la::networkInterface::MacAddress{}, la::networkInterface::MacAddress{}, "AECP completionHandler called again with same result message, ignoring this call.");
												 return;
											 }
											 {
												 // Lock Self before calling a handler, we come from a network thread
												 auto const lg = std::lock_guard{ _lock };
												 if (kIOReturnSuccess == (IOReturn)error.code)
												 {
													 // Special case for VendorUnique messages:
													 //  It's up to the implementation to keep track of the message, the response, the timeout, the retry.
													 //  This completion handler is called immediately upon send, NSError being set if there was an error.
													 //  If no error was detected, we passed the actual message being send so we can retrieve the sequenceID to track the response in - (BOOL)AECPDidReceiveResponse:(AVB17221AECPMessage*)message onInterface:(AVB17221AECPInterface*)anInterface;
													 if (message.messageType == AVB17221AECPMessageTypeVendorUniqueCommand)
													 {
														 // Right now as there is no alternative for sending VendorUnique message, we have to alter our Aecpdu::SequenceID with the autogenerated one
														 // If we someday get a new API to send AECP messages without being handled by macOS state machine, we will have to remove this line
														 sentAecpdu->setSequenceID([message sequenceID]);

														 // Forward to our state machine
														 _protocolInterface->handleVendorUniqueCommandSent(std::move(sentAecpdu), resultHandler);
													 }
													 else
													 {
														 auto aecpdu = [FromNative makeAecpdu:message toDestAddress:_protocolInterface->getMacAddress() withProtocolInterface:*_protocolInterface];
														 la::avdecc::utils::invokeProtectedHandler(resultHandler, aecpdu.get(), la::avdecc::protocol::ProtocolInterface::Error::NoError);
													 }
												 }
												 else
												 {
													 la::avdecc::utils::invokeProtectedHandler(resultHandler, nullptr, [FromNative getProtocolError:error]);
												 }
											 }
											 resultHandler = {}; // Clear resultHandler in case this completionHandler is called twice (bug in macOS)
											 [self stopAsyncOperation];
											 // Signal the semaphore so we can process another command
											 dispatch_semaphore_signal(limiter);
										 }]; // We don't care about the method result, the completionHandler will always be called anyway (if for some reason, we detect it's not always the case, simply remove the resultHandler and call stopAsyncOperation and signal the semaphore if the method fails, and return TransportError. Carefull to change the resultHandler under a small lock that has to be shared with the block as well)
		});
	}
	else
	{
		AVDECC_ASSERT(false, "Not supported AECP message type");
		return la::avdecc::protocol::ProtocolInterface::Error::InternalError;
	}
	return la::avdecc::protocol::ProtocolInterface::Error::NoError;
}

- (la::avdecc::protocol::ProtocolInterface::Error)sendAecpResponse:(la::avdecc::protocol::Aecpdu::UniquePointer&&)aecpdu {
	auto const macAddr = aecpdu->getDestAddress(); // Make a copy of the target macAddress so it can safely be used inside the objC block

	auto message = [ToNative makeAecpMessage:*aecpdu];
	if (message != NULL)
	{
		decltype(EntityQueues::aecpQueue) queue;
		// Only take the lock while searching for the queue, we want to release it before invoking dispath_async to prevent a deadlock
		{
			std::lock_guard<decltype(_lockQueues)> const lg(_lockQueues);
			auto eqIt = _entityQueues.find(la::avdecc::UniqueIdentifier{ message.controllerEntityID });
			if (eqIt == _entityQueues.end())
			{
				queue = [self createQueuesForRemoteEntity:la::avdecc::UniqueIdentifier{ message.controllerEntityID }].aecpQueue;
			}
			else
			{
				queue = eqIt->second.aecpQueue;
			}
		}

		dispatch_async(queue, ^{
			dispatch_semaphore_t limiter;
			{
				std::lock_guard<decltype(_lockQueues)> const lg(_lockQueues);
				auto eqIt = _entityQueues.find(la::avdecc::UniqueIdentifier{ message.controllerEntityID });
				if (eqIt == _entityQueues.end())
				{
					// Entity no longer registered, ignore this command and return
					return;
				}
				limiter = eqIt->second.aecpLimiter; // We can store the limiter here, we know the queue and semaphore exists until all scheduled blocks on this queue are finished (thanks to the dispatch_sync call)
			}

			// Take a semaphore count to limit the inflight commands
			dispatch_semaphore_wait(limiter, DISPATCH_TIME_FOREVER);

			// Actually send the message
			[self startAsyncOperation];
			NSError* error{ nullptr };
			[self.interface.aecp sendResponse:message toMACAddress:[ToNative makeAVBMacAddress:macAddr] error:&error];
			[self stopAsyncOperation];

			// Signal the semaphore so we can process another command
			dispatch_semaphore_signal(limiter);
		});
	}
	else
	{
		AVDECC_ASSERT(false, "Not supported AECP message type");
		return la::avdecc::protocol::ProtocolInterface::Error::InternalError;
	}
	return la::avdecc::protocol::ProtocolInterface::Error::NoError;
}

- (la::avdecc::protocol::ProtocolInterface::Error)sendAcmpCommand:(la::avdecc::protocol::Acmpdu::UniquePointer&&)acmpdu handler:(la::avdecc::protocol::ProtocolInterface::AcmpCommandResultHandler const&)onResult {
	__block auto resultHandler = onResult; // Make a copy of the handler so it can safely be used inside the objC block. Declare it as __block so we can modify it from the block (to fix a bug that macOS sometimes call the completionHandler twice)

	auto const& acmp = static_cast<la::avdecc::protocol::Acmpdu const&>(*acmpdu);
	auto const message = [[AVB17221ACMPMessage alloc] init];
#if !__has_feature(objc_arc)
	[message autorelease];
#endif
	// Set Acmp fields
	message.messageType = static_cast<AVB17221ACMPMessageType>(acmp.getMessageType().getValue());
	message.status = AVB17221ACMPStatusSuccess;
	message.streamID = acmp.getStreamID();
	message.controllerEntityID = acmp.getControllerEntityID();
	message.talkerEntityID = acmp.getTalkerEntityID();
	message.talkerUniqueID = acmp.getTalkerUniqueID();
	message.listenerEntityID = acmp.getListenerEntityID();
	message.listenerUniqueID = acmp.getListenerUniqueID();
	message.destinationMAC = [ToNative makeAVBMacAddress:acmp.getStreamDestAddress()];
	message.connectionCount = acmp.getConnectionCount();
	message.sequenceID = acmp.getSequenceID();
	message.flags = static_cast<AVB17221ACMPFlags>(acmp.getFlags().value());
	message.vlanID = acmp.getStreamVlanID();

	[self startAsyncOperation];
	[self.interface.acmp sendACMPCommandMessage:message
														completionHandler:^(NSError* error, AVB17221ACMPMessage* response) {
															if (!resultHandler)
															{
																LOG_PROTOCOL_INTERFACE_DEBUG(la::networkInterface::MacAddress{}, la::networkInterface::MacAddress{}, "ACMP completionHandler called again with same result message, ignoring this call.");
																return;
															}
															// This is a special hack to protect from a macOS bug: If the adapter is not correctly plugged in, the completionHandler is immediately called with the message as response and no error code
															// Bug is at least present in macOS Big Sur 11.6.1
															{
																// Check a few fields, just to be sure response IS message (comparing pointer is not secured enough)
																if (error == nil && message == response && message.messageType == response.messageType && message.sequenceID == response.sequenceID)
																{
																	LOG_PROTOCOL_INTERFACE_DEBUG(la::networkInterface::MacAddress{}, la::networkInterface::MacAddress{}, "ACMP completionHandler called with our command message instead of the response (without error code), consider it has timed out.");
																	error = [NSError errorWithDomain:AVBErrorDomain code:kIOReturnTimeout userInfo:nil];
																}
															}
															{
																// Lock Self before calling a handler, we come from a network thread
																auto const lg = std::lock_guard{ _lock };
																if (kIOReturnSuccess == (IOReturn)error.code)
																{
																	auto acmp = [FromNative makeAcmpdu:response];
																	la::avdecc::utils::invokeProtectedHandler(resultHandler, acmp.get(), la::avdecc::protocol::ProtocolInterface::Error::NoError);
																}
																else
																{
																	la::avdecc::utils::invokeProtectedHandler(resultHandler, nullptr, [FromNative getProtocolError:error]);
																}
															}
															resultHandler = {}; // Clear resultHandler in case this completionHandler is called twice (bug in macOS)
															[self stopAsyncOperation];
														}]; // We don't care about the method result, the completionHandler will always be called anyway (if for some reason, we detect it's not always the case, simply remove the resultHandler and call stopAsyncOperation if the method fails, and return TransportError. Carefull to change the resultHandler under a small lock that has to be shared with the block as well)
	return la::avdecc::protocol::ProtocolInterface::Error::NoError;
}

- (void)lock {
	_lock.lock();
}

- (void)unlock {
	_lock.unlock();
}

- (bool)isSelfLocked {
	return _lock.isSelfLocked();
}

#pragma mark AVB17221EntityDiscoveryDelegate delegate
- (void)initEntity:(la::avdecc::UniqueIdentifier)entityID {
	// Register ACMP sniffing handler for this entity
	if ([self.interface.acmp setHandler:self forEntityID:entityID])
	{
		// Register the entity for handler removal upon shutdown
		auto const lg = std::lock_guard{ _lock };
		_registeredAcmpHandlers.insert(entityID);
	}

	// Create queues and semaphore
	{
		std::lock_guard<decltype(_lockQueues)> const lg(_lockQueues);
		[self createQueuesForRemoteEntity:entityID];
	}
}

- (void)deinitEntity:(la::avdecc::UniqueIdentifier)entityID {
	{
		// Lock
		auto const lg = std::lock_guard{ _lock };
		// Unregister ACMP handler
		_registeredAcmpHandlers.erase(entityID);
	}
	[self.interface.acmp removeHandlerForEntityID:entityID];

	// Remove the EntityQueues structure from the map under the lock (if found), then clean it without the lock (to prevent deadlock)
	EntityQueues eq;
	bool foundEq{ false };
	{
		std::lock_guard<decltype(_lockQueues)> const lg(_lockQueues);
		auto eqIt = _entityQueues.find(entityID);
		if (eqIt != _entityQueues.end())
		{
			eq = eqIt->second; // Copy the structure now, we will remove it from the map next line (if ARC is enabled, a ref count will be taken. If not we still haven't call dispatch_release so the object is still valid)
			//NSLog(@"Queue retain count before sync: %ld", eq.aecpQueue.retainCount);
			//NSLog(@"Limiter retain count before sync: %ld", eq.aecpLimiter.retainCount);

			// Remove from the map to indicate it's being removed
			_entityQueues.erase(entityID);

			// We found it, we can clean it up
			foundEq = true;
		}
	}

	// Found a matching EntityQueues for the entity, time to clean it up
	if (foundEq)
	{
		// Synchronize the queue using an empty block
		dispatch_sync(eq.aecpQueue, ^{
										//NSLog(@"Queue retain count: %ld", eq.aecpQueue.retainCount);
										//NSLog(@"Limiter retain count: %ld", eq.aecpLimiter.retainCount);
									});
#if !__has_feature(objc_arc)
		// Release the objects
		dispatch_release(eq.aecpQueue);
		dispatch_release(eq.aecpLimiter);
#endif
	}
}

// Notification of an arriving local computer entity
- (void)didAddLocalEntity:(AVB17221Entity*)newEntity on17221EntityDiscovery:(AVB17221EntityDiscovery*)entityDiscovery {
	auto const entityID = la::avdecc::UniqueIdentifier{ newEntity.entityID };
	auto e = [FromNative makeEntity:newEntity];

	// Initialize entity
	[self initEntity:entityID];

	// Lock
	auto const lg = std::lock_guard{ _lock };

	// Add the entity to our cache of local entities declared by the local machine
	_localMachineEntities.insert_or_assign(entityID, e);

	// Notify observers
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityOnline, _protocolInterface, e);
}

// Notification of a departing local computer entity
- (void)didRemoveLocalEntity:(AVB17221Entity*)oldEntity on17221EntityDiscovery:(AVB17221EntityDiscovery*)entityDiscovery {
	auto const oldEntityID = la::avdecc::UniqueIdentifier{ oldEntity.entityID };
	[self deinitEntity:oldEntityID];

	// Lock
	auto const lg = std::lock_guard{ _lock };

	// Remove from declared entities
	_localMachineEntities.erase(oldEntityID);

	// Notify observers
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityOffline, _protocolInterface, oldEntityID);
}

- (void)didRediscoverLocalEntity:(AVB17221Entity*)entity on17221EntityDiscovery:(AVB17221EntityDiscovery*)entityDiscovery {
	// Check if Entity already in the list
	{
		// Lock
		auto const lg = std::lock_guard{ _lock };
		if (_registeredAcmpHandlers.find(la::avdecc::UniqueIdentifier{ entity.entityID }) == _registeredAcmpHandlers.end())
		{
			AVDECC_ASSERT(false, "didRediscoverLocalEntity: Entity not registered... I thought Rediscover was called when an entity announces itself again without any change in it's ADP info... Maybe simply call didAddLocalEntity");
			return;
		}
	}
	// Nothing to do, entity has already been detected
}

- (void)didUpdateLocalEntity:(AVB17221Entity*)entity changedProperties:(AVB17221EntityPropertyChanged)changedProperties on17221EntityDiscovery:(AVB17221EntityDiscovery*)entityDiscovery {
	constexpr NSUInteger ignoreChangeMask = 0xFFFFFFFF & ~(AVB17221EntityPropertyChangedTimeToLive | AVB17221EntityPropertyChangedAvailableIndex);
	// If changes are only for flags we want to ignore, return
	if ((changedProperties & ignoreChangeMask) == 0)
		return;

	auto e = [FromNative makeEntity:entity];

	// Lock
	auto const lg = std::lock_guard{ _lock };

	// Update the entity
	_localMachineEntities.insert_or_assign(e.getEntityID(), e);

	// If a change occured in a forbidden flag, simulate offline/online for this entity
	if ((changedProperties & AVB17221EntityPropertyImmutableMask) != 0)
	{
		_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityOffline, _protocolInterface, e.getEntityID());
		_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityOnline, _protocolInterface, e);
	}
	else
	{
		_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onLocalEntityUpdated, _protocolInterface, e);
	}
}

- (void)didAddRemoteEntity:(AVB17221Entity*)newEntity on17221EntityDiscovery:(AVB17221EntityDiscovery*)entityDiscovery {
	auto const entityID = la::avdecc::UniqueIdentifier{ newEntity.entityID };
	auto e = [FromNative makeEntity:newEntity];

	// Initialize entity
	[self initEntity:entityID];

	// Lock
	auto const lg = std::lock_guard{ _lock };

	// Add entity to available index list
	auto previousResult = _lastAvailableIndex.insert(std::make_pair(newEntity.entityID, newEntity.availableIndex));
	if (!AVDECC_ASSERT_WITH_RET(previousResult.second, "Adding a new entity but it's already in the available index list"))
	{
		previousResult.first->second = newEntity.availableIndex;
	}

	// Add the entity to our cache of remote entities
	_remoteEntities.insert_or_assign(entityID, e);

	// Notify observers
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onRemoteEntityOnline, _protocolInterface, e);
}

- (void)didRemoveRemoteEntity:(AVB17221Entity*)oldEntity on17221EntityDiscovery:(AVB17221EntityDiscovery*)entityDiscovery {
	auto const oldEntityID = la::avdecc::UniqueIdentifier{ oldEntity.entityID };
	[self deinitEntity:oldEntityID];

	// Lock
	auto const lg = std::lock_guard{ _lock };

	// Remove from declared entities
	_remoteEntities.erase(oldEntityID);

	// Clear entity from available index list
	_lastAvailableIndex.erase(oldEntityID);

	// Notify observers
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onRemoteEntityOffline, _protocolInterface, oldEntityID);
}

- (void)didRediscoverRemoteEntity:(AVB17221Entity*)entity on17221EntityDiscovery:(AVB17221EntityDiscovery*)entityDiscovery {
	// Check if Entity already in the list
	{
		// Lock
		auto const lg = std::lock_guard{ _lock };
		if (_registeredAcmpHandlers.find(la::avdecc::UniqueIdentifier{ entity.entityID }) == _registeredAcmpHandlers.end())
		{
			AVDECC_ASSERT(false, "didRediscoverRemoteEntity: Entity not registered... I thought Rediscover was called when an entity announces itself again without any change in it's ADP info... Maybe simply call didAddRemoteEntity");
			return;
		}
	}
	// Nothing to do, entity has already been detected
}

- (void)didUpdateRemoteEntity:(AVB17221Entity*)entity changedProperties:(AVB17221EntityPropertyChanged)changedProperties on17221EntityDiscovery:(AVB17221EntityDiscovery*)entityDiscovery {
	// Lock
	auto const lg = std::lock_guard{ _lock };

	// Check for an invalid change in AvailableIndex
	if ((changedProperties & AVB17221EntityPropertyChangedAvailableIndex) != 0)
	{
		auto const entityID = la::avdecc::UniqueIdentifier{ entity.entityID };
		auto previousIndexIt = _lastAvailableIndex.find(entityID);
		if (previousIndexIt == _lastAvailableIndex.end())
		{
			AVDECC_ASSERT(previousIndexIt != _lastAvailableIndex.end(), "didUpdateRemoteEntity called but entity is unknown");
			_lastAvailableIndex.insert(std::make_pair(entityID, entity.availableIndex));
		}
		else
		{
			auto const previousIndex = previousIndexIt->second; // Get previous index
			previousIndexIt->second = entity.availableIndex; // Update index value with the latest one

			if (previousIndex >= entity.availableIndex)
			{
				auto e = [FromNative makeEntity:entity];
				_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onRemoteEntityOffline, _protocolInterface, e.getEntityID());
				_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onRemoteEntityOnline, _protocolInterface, e);
				return;
			}
		}
	}

	constexpr NSUInteger ignoreChangeMask = 0xFFFFFFFF & ~(AVB17221EntityPropertyChangedTimeToLive | AVB17221EntityPropertyChangedAvailableIndex);
	// If changes are only for flags we want to ignore, return
	if ((changedProperties & ignoreChangeMask) == 0)
		return;

	auto e = [FromNative makeEntity:entity];

	// Update the entity
	_remoteEntities.insert_or_assign(e.getEntityID(), e);

	// If a change occured in a forbidden flag, simulate offline/online for this entity
	if ((changedProperties & AVB17221EntityPropertyImmutableMask) != 0)
	{
		_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onRemoteEntityOffline, _protocolInterface, e.getEntityID());
		_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onRemoteEntityOnline, _protocolInterface, e);
	}
	else
	{
		_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onRemoteEntityUpdated, _protocolInterface, e);
	}
}

#pragma mark AVB17221AECPClient delegate
- (BOOL)AECPDidReceiveCommand:(AVB17221AECPMessage*)message onInterface:(AVB17221AECPInterface*)anInterface {
	// This handler is called for all AECP commands targeting one of our registered Entities

	// Lock
	auto const lg = std::lock_guard{ _lock };

	// Only process it if it's targeted to a registered LocalEntity
	if (_localProcessEntities.count(la::avdecc::UniqueIdentifier{ message.targetEntityID }) == 0)
		return NO;

	auto const aecpdu = [FromNative makeAecpdu:message toDestAddress:_protocolInterface->getMacAddress() withProtocolInterface:*_protocolInterface];

	// Notify the observers
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onAecpCommand, _protocolInterface, *aecpdu);

	return YES;
}

- (BOOL)AECPDidReceiveResponse:(AVB17221AECPMessage*)message onInterface:(AVB17221AECPInterface*)anInterface {
	// This handler is called for all AECP responses targeting one of our registered Entities, even the messages that are solicited responses and which will be handled by the block of aecp.sendCommand() method

	// Lock
	auto const lg = std::lock_guard{ _lock };

	auto aecpdu = [FromNative makeAecpdu:message toDestAddress:_protocolInterface->getMacAddress() withProtocolInterface:*_protocolInterface];
	auto const controllerID = la::avdecc::UniqueIdentifier{ [message controllerEntityID] };
	auto const isAemUnsolicitedResponse = [message messageType] == AVB17221AECPMessageTypeAEMResponse && [static_cast<AVB17221AECPAEMMessage*>(message) isUnsolicited];

	// First check if we received a multicast IdentifyNotification
	if (controllerID == la::avdecc::protocol::AemAecpdu::Identify_ControllerEntityID)
	{
		// We have to cheat a little bit here since there is currently no way to retrieve the DestMacAddress from macOS native APIs. Assume we correctly received a multicast message from a device, not a targeted one.
		aecpdu->setDestAddress(la::avdecc::protocol::AemAecpdu::Identify_Mac_Address);

		// Check if it's an AEM unsolicited response
		if (isAemUnsolicitedResponse)
		{
			_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onAecpAemIdentifyNotification, _protocolInterface, static_cast<la::avdecc::protocol::AemAecpdu const&>(*aecpdu));
		}
		else
		{
			LOG_PROTOCOL_INTERFACE_WARN(aecpdu->getSrcAddress(), aecpdu->getDestAddress(), std::string("Received an AECP response message with controller_entity_id set to the IDENTIFY ControllerID, but the message is not an unsolicited AEM response"));
		}
	}

	// Search our local entities, which should be found!
	if (_localProcessEntities.count(controllerID) == 0)
		return NO;

	// Special case for Unsolicited Responses
	if (isAemUnsolicitedResponse)
	{
		// Notify the observers
		_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onAecpAemUnsolicitedResponse, _protocolInterface, static_cast<la::avdecc::protocol::AemAecpdu const&>(*aecpdu));
		return YES;
	}

	// Special case for VendorUnique messages:
	//  It's up to the implementation to keep track of the message, the response, the timeout, the retry.
	if (message.messageType == AVB17221AECPMessageTypeVendorUniqueResponse)
	{
		return _protocolInterface->handleVendorUniqueResponseReceived(static_cast<la::avdecc::protocol::VuAecpdu const&>(*aecpdu));
	}


	// Ignore all other messages in this handler, expected responses will be handled by the block of aecp.sendCommand() method
	return NO;
}

#pragma mark AVB17221ACMPClient delegate
- (BOOL)ACMPDidReceiveCommand:(AVB17221ACMPMessage*)message onInterface:(AVB17221ACMPInterface*)anInterface {
	// This handler is called for all ACMP messages, even the messages that are sent by ourself

	// Lock
	auto const lg = std::lock_guard{ _lock };

	auto const acmpdu = [FromNative makeAcmpdu:message];

	// Notify the observers
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onAcmpCommand, _protocolInterface, *acmpdu);

	return YES;
}

- (BOOL)ACMPDidReceiveResponse:(AVB17221ACMPMessage*)message onInterface:(AVB17221ACMPInterface*)anInterface {
	// This handler is called for all ACMP messages, even the messages that are expected responses and which will be handled by the block of acmp.sendACMPCommandMessage() method

	// Lock
	auto const lg = std::lock_guard{ _lock };

	auto const acmpdu = [FromNative makeAcmpdu:message];

	// Notify the observers
	_protocolInterface->notifyObserversMethod<la::avdecc::protocol::ProtocolInterface::Observer>(&la::avdecc::protocol::ProtocolInterface::Observer::onAcmpResponse, _protocolInterface, *acmpdu);

	return YES;
}

@end
