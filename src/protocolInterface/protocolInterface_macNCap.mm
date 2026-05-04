/*
* Copyright (C) 2016-2026, L-Acoustics and its contributors

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
* @file protocolInterface_macNCap.mm
* @author Christophe Calmejane
* @brief macOS native ProtocolInterface using Network.framework (NWEthernetChannel).
*
* @details This implementation mirrors the pcap ProtocolInterface architecture but uses Apple's
*          `nw_ethernet_channel` C API from Network.framework to send and receive raw layer-2
*          Ethernet frames matching the AVTP EtherType (0x22F0).
*
* @note Using `nw_ethernet_channel` requires the host process to hold the
*       `com.apple.developer.networking.custom-protocol` entitlement.
*
* @warning `nw_ethernet_channel` operates on an **exclusive claim model**: once the channel is
*          started, the registered EtherType (0x22F0) is claimed by the kernel and frames are no
*          longer delivered to BPF/pcap or to other processes on the same interface.
*          This means that this ProtocolInterface **cannot coexist** with other applications that
*          listen on the same EtherType on the same interface (e.g. pcap-based or AVB Framework-based
*          applications). This is a fundamental limitation of Apple's Network.framework API and
*          cannot be worked around. Use the pcap ProtocolInterface when multi-process coexistence
*          is required.
*/

#include "la/avdecc/internals/serialization.hpp"
#include "la/avdecc/internals/protocolAemAecpdu.hpp"
#include "la/avdecc/internals/protocolAaAecpdu.hpp"
#include "la/avdecc/watchDog.hpp"
#include "la/avdecc/utils.hpp"
#include "la/avdecc/executor.hpp"

#include "stateMachine/stateMachineManager.hpp"
#include "ethernetPacketDispatch.hpp"
#include "protocolInterface_macNCap.hpp"
#include "logHelper.hpp"

#include <stdexcept>
#include <string>
#include <memory>
#include <chrono>
#include <atomic>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <dlfcn.h>

#import <Network/Network.h>
#import <dispatch/dispatch.h>

// All Network.framework symbols used here are macOS 10.15+. A runtime check is performed via isSupported(),
// guaranteeing these APIs are never invoked on older systems. Silence the compile-time availability warning.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace
{
/** Maximum time to wait for the nw_ethernet_channel to become ready after start. */
constexpr auto ChannelReadyTimeout = std::chrono::seconds{ 5 };
/** Maximum time to wait for the nw_ethernet_channel to reach the cancelled state after cancel. */
constexpr auto ChannelCancelTimeout = std::chrono::seconds{ 2 };
/** Maximum time to wait for the network interface to be discovered by the path monitor. */
constexpr auto InterfaceDiscoveryTimeout = std::chrono::seconds{ 3 };

/**
* @brief Build a dispatch_data_t from a serialization buffer.
* @details The data object makes a private copy of the contents, so the buffer can be safely discarded right after.
*/
dispatch_data_t makeDispatchData(std::uint8_t const* const bytes, std::size_t const length) noexcept
{
	if (length == 0u)
	{
		return dispatch_data_empty;
	}
	return dispatch_data_create(bytes, length, nullptr, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
}

} // namespace

class ProtocolInterfaceMacNCapImpl final : public ProtocolInterfaceMacNCap, private stateMachine::ProtocolInterfaceDelegate, private stateMachine::AdvertiseStateMachine::Delegate, private stateMachine::DiscoveryStateMachine::Delegate, private stateMachine::CommandStateMachine::Delegate
{
public:
	/* ************************************************************ */
	/* Public APIs                                                  */
	/* ************************************************************ */
	/** Constructor */
	ProtocolInterfaceMacNCapImpl(std::string const& networkInterfaceID, std::string const& executorName)
		: ProtocolInterfaceMacNCap(networkInterfaceID, executorName)
	{
		AVDECC_ASSERT(isSupported(), "Should always be supported. Cannot create a macOS Native Network ProtocolInterface if it's not supported");

		// Resolve the nw_interface_t matching the provided BSD name
		auto resolvedInterface = resolveInterface(networkInterfaceID);
		if (resolvedInterface == nullptr)
		{
			throw Exception(Error::InterfaceNotFound, "No matching network interface found by the Network.framework path monitor for '" + networkInterfaceID + "'");
		}
		_interface = resolvedInterface;

		// Create serial dispatch queue dedicated to channel callbacks
		_channelQueue = dispatch_queue_create("com.l-acoustics.avdecc.macNCap", DISPATCH_QUEUE_SERIAL);
		if (_channelQueue == nullptr)
		{
			throw Exception(Error::TransportError, "Failed to create dispatch queue");
		}

		// Create the Ethernet channel with the AVTP EtherType
		_channel = nw_ethernet_channel_create(AvtpEtherType, _interface);
		if (_channel == nullptr)
		{
			throw Exception(Error::TransportError, "nw_ethernet_channel_create failed (missing entitlement or invalid ethertype?)");
		}

		nw_ethernet_channel_set_queue(_channel, _channelQueue);

		// Install the state change handler to track ready/failed/cancelled transitions
		auto* const selfPtr = this;
		nw_ethernet_channel_set_state_changed_handler(_channel, ^(nw_ethernet_channel_state_t state, nw_error_t _Nullable error) {
			selfPtr->onChannelStateChanged(state, error);
		});

		// Install the receive handler that forwards frames to the AVDECC dispatcher
		nw_ethernet_channel_set_receive_handler(_channel, ^(dispatch_data_t content, std::uint16_t /*vlan_tag*/, nw_ethernet_address_t local_address, nw_ethernet_address_t remote_address) {
			selfPtr->onFrameReceived(content, static_cast<std::uint8_t const*>(local_address), static_cast<std::uint8_t const*>(remote_address));
		});

		// Start the channel and wait (with timeout) for it to reach the ready state
		nw_ethernet_channel_start(_channel);

		auto const readyResult = waitForStateAtLeast(nw_ethernet_channel_state_ready, ChannelReadyTimeout);
		if (readyResult == WaitResult::Timeout)
		{
			cancelAndWait();
			throw Exception(Error::TransportError, "Timed out waiting for nw_ethernet_channel to become ready");
		}
		if (readyResult == WaitResult::Failed)
		{
			cancelAndWait();
			throw Exception(Error::TransportError, "nw_ethernet_channel failed to become ready (missing 'com.apple.developer.networking.custom-protocol' entitlement?)");
		}

		// Start the state machines
		_stateMachineManager.startStateMachines();
	}

	/** Destructor */
	virtual ~ProtocolInterfaceMacNCapImpl() noexcept override
	{
		shutdown();
	}

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept override
	{
		delete this;
	}

	// Deleted compiler auto-generated methods
	ProtocolInterfaceMacNCapImpl(ProtocolInterfaceMacNCapImpl&&) = delete;
	ProtocolInterfaceMacNCapImpl(ProtocolInterfaceMacNCapImpl const&) = delete;
	ProtocolInterfaceMacNCapImpl& operator=(ProtocolInterfaceMacNCapImpl const&) = delete;
	ProtocolInterfaceMacNCapImpl& operator=(ProtocolInterfaceMacNCapImpl&&) = delete;

private:
	/* ************************************************************ */
	/* ProtocolInterface overrides                                  */
	/* ************************************************************ */
	virtual void shutdown() noexcept override
	{
		// Stop the state machines
		_stateMachineManager.stopStateMachines();

		// Cancel the channel and wait for its final state
		cancelAndWait();

		// Flush executor jobs so no dispatch job remains in-flight while we tear down
		la::avdecc::ExecutorManager::getInstance().flush(getExecutorName());

		// Release ObjC objects (under ARC these would be released automatically, but we make it explicit)
		_channel = nullptr;
		_interface = nullptr;
		_channelQueue = nullptr;
	}

	virtual UniqueIdentifier getDynamicEID() const noexcept override
	{
		auto eid = UniqueIdentifier::value_type{ 0u };
		auto const& macAddress = getMacAddress();

		eid += macAddress[0];
		eid <<= 8;
		eid += macAddress[1];
		eid <<= 8;
		eid += macAddress[2];
		eid <<= 8;
		eid += macAddress[3];
		eid <<= 8;
		eid += macAddress[4];
		eid <<= 8;
		eid += macAddress[5];
		eid <<= 16;
		std::srand(static_cast<unsigned int>(std::time(0)));
		eid += static_cast<std::uint16_t>((std::rand() % 0xFFFD) + 1);

		return UniqueIdentifier{ eid };
	}

	virtual void releaseDynamicEID(UniqueIdentifier const /*entityID*/) const noexcept override
	{
		// Nothing to do
	}

	virtual Error registerLocalEntity(entity::LocalEntity& entity) noexcept override
	{
		auto const index = _stateMachineManager.getMatchingInterfaceIndex(entity);

		if (index)
		{
			return _stateMachineManager.registerLocalEntity(entity);
		}

		return Error::InvalidParameters;
	}

	virtual Error unregisterLocalEntity(entity::LocalEntity& entity) noexcept override
	{
		return _stateMachineManager.unregisterLocalEntity(entity);
	}

	virtual Error injectRawPacket(la::avdecc::MemoryBuffer&& packet) const noexcept override
	{
		// The packet is expected to be a full Ethernet frame (including EtherLayer2 header)
		auto const& buffer = packet;
		if (buffer.size() < EtherLayer2::HeaderLength)
		{
			return Error::InvalidParameters;
		}

		auto etherLayer2 = EtherLayer2{};
		auto deserializationBuffer = DeserializationBuffer{ buffer };
		try
		{
			deserialize<EtherLayer2>(&etherLayer2, deserializationBuffer);
		}
		catch (...)
		{
			return Error::InvalidParameters;
		}

		auto const payloadOffset = EtherLayer2::HeaderLength;
		auto const payloadLength = buffer.size() - payloadOffset;
		dispatchAvdeccPayload(buffer.data() + payloadOffset, payloadLength, etherLayer2);
		return Error::NoError;
	}

	virtual Error setEntityNeedsAdvertise(entity::LocalEntity const& entity, entity::LocalEntity::AdvertiseFlags const /*flags*/) noexcept override
	{
		return _stateMachineManager.setEntityNeedsAdvertise(entity);
	}

	virtual Error enableEntityAdvertising(entity::LocalEntity& entity) noexcept override
	{
		return _stateMachineManager.enableEntityAdvertising(entity);
	}

	virtual Error disableEntityAdvertising(entity::LocalEntity const& entity) noexcept override
	{
		return _stateMachineManager.disableEntityAdvertising(entity);
	}

	virtual Error discoverRemoteEntities() const noexcept override
	{
		return discoverRemoteEntity(UniqueIdentifier::getNullUniqueIdentifier());
	}

	virtual Error discoverRemoteEntity(UniqueIdentifier const entityID) const noexcept override
	{
		auto const frame = stateMachine::Manager::makeDiscoveryMessage(getMacAddress(), entityID);
		auto const err = sendMessage(frame);
		if (!err)
		{
			_stateMachineManager.discoverMessageSent();
		}
		return err;
	}

	virtual Error forgetRemoteEntity(UniqueIdentifier const entityID) const noexcept override
	{
		return _stateMachineManager.forgetRemoteEntity(entityID);
	}

	virtual Error setAutomaticDiscoveryDelay(std::chrono::milliseconds const delay) const noexcept override
	{
		return _stateMachineManager.setAutomaticDiscoveryDelay(delay);
	}

	virtual bool isDirectMessageSupported() const noexcept override
	{
		return true;
	}

	virtual Error sendAdpMessage(Adpdu const& adpdu) const noexcept override
	{
		return sendMessage(adpdu);
	}

	virtual Error sendAecpMessage(Aecpdu const& aecpdu) const noexcept override
	{
		return sendMessage(aecpdu);
	}

	virtual Error sendAcmpMessage(Acmpdu const& acmpdu) const noexcept override
	{
		return sendMessage(acmpdu);
	}

	virtual Error sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, AecpCommandResultHandler const& onResult) const noexcept override
	{
		auto const messageType = aecpdu->getMessageType();

		if (!AVDECC_ASSERT_WITH_RET(!isAecpResponseMessageType(messageType), "Calling sendAecpCommand with a Response MessageType"))
		{
			return Error::MessageNotSupported;
		}

		if (messageType == AecpMessageType::VendorUniqueCommand)
		{
			auto& vuAecp = static_cast<VuAecpdu&>(*aecpdu);

			auto const vuProtocolID = vuAecp.getProtocolIdentifier();
			auto* vuDelegate = getVendorUniqueDelegate(vuProtocolID);

			if (!vuDelegate || !vuDelegate->areHandledByControllerStateMachine(vuProtocolID))
			{
				return Error::MessageNotSupported;
			}
		}

		return _stateMachineManager.sendAecpCommand(std::move(aecpdu), onResult);
	}

	virtual Error sendAecpResponse(Aecpdu::UniquePointer&& aecpdu) const noexcept override
	{
		auto const messageType = aecpdu->getMessageType();

		if (!AVDECC_ASSERT_WITH_RET(isAecpResponseMessageType(messageType), "Calling sendAecpResponse with a Command MessageType"))
		{
			return Error::MessageNotSupported;
		}

		if (messageType == AecpMessageType::VendorUniqueResponse)
		{
			auto& vuAecp = static_cast<VuAecpdu&>(*aecpdu);

			auto const vuProtocolID = vuAecp.getProtocolIdentifier();
			auto* vuDelegate = getVendorUniqueDelegate(vuProtocolID);

			if (!vuDelegate || !vuDelegate->areHandledByControllerStateMachine(vuProtocolID))
			{
				return Error::MessageNotSupported;
			}
		}

		return sendMessage(static_cast<Aecpdu const&>(*aecpdu));
	}

	virtual Error sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, AcmpCommandResultHandler const& onResult) const noexcept override
	{
		return _stateMachineManager.sendAcmpCommand(std::move(acmpdu), onResult);
	}

	virtual Error sendAcmpResponse(Acmpdu::UniquePointer&& acmpdu) const noexcept override
	{
		return sendMessage(static_cast<Acmpdu const&>(*acmpdu));
	}

	virtual void lock() const noexcept override
	{
		_stateMachineManager.lock();
	}

	virtual void unlock() const noexcept override
	{
		_stateMachineManager.unlock();
	}

	virtual bool isSelfLocked() const noexcept override
	{
		return _stateMachineManager.isSelfLocked();
	}

	/* ************************************************************ */
	/* stateMachine::ProtocolInterfaceDelegate overrides            */
	/* ************************************************************ */
	/* **** AECP notifications **** */
	virtual void onAecpCommand(Aecpdu const& aecpdu) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpCommand, this, aecpdu);
	}

	virtual void onVuAecpUnsolicitedResponse(VuAecpdu::ProtocolIdentifier const& protocolIdentifier, VuAecpdu const& aecpdu) noexcept override
	{
		handleVendorUniqueUnsolicitedResponse(protocolIdentifier, aecpdu);
	}

	/* **** ACMP notifications **** */
	virtual void onAcmpCommand(Acmpdu const& acmpdu) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAcmpCommand, this, acmpdu);
	}

	virtual void onAcmpResponse(Acmpdu const& acmpdu) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAcmpResponse, this, acmpdu);
	}

	/* **** Sending methods **** */
	virtual Error sendMessage(Adpdu const& adpdu) const noexcept override
	{
		try
		{
			// Serialize only the AVTP layers (the Ethernet L2 header is produced by nw_ethernet_channel itself)
			SerializationBuffer buffer;
			serialize<AvtpduControl>(adpdu, buffer);
			serialize<Adpdu>(adpdu, buffer);
			return sendPayload(adpdu.getDestAddress(), buffer);
		}
		catch ([[maybe_unused]] std::exception const& e)
		{
			LOG_PROTOCOL_INTERFACE_DEBUG(adpdu.getSrcAddress(), adpdu.getDestAddress(), std::string("Failed to serialize ADPDU: ") + e.what());
			return Error::InternalError;
		}
	}

	virtual Error sendMessage(Aecpdu const& aecpdu) const noexcept override
	{
		try
		{
			SerializationBuffer buffer;
			serialize<AvtpduControl>(aecpdu, buffer);
			serialize<Aecpdu>(aecpdu, buffer);
			return sendPayload(aecpdu.getDestAddress(), buffer);
		}
		catch ([[maybe_unused]] std::exception const& e)
		{
			LOG_PROTOCOL_INTERFACE_DEBUG(aecpdu.getSrcAddress(), aecpdu.getDestAddress(), std::string("Failed to serialize AECPDU: ") + e.what());
			return Error::InternalError;
		}
	}

	virtual Error sendMessage(Acmpdu const& acmpdu) const noexcept override
	{
		try
		{
			SerializationBuffer buffer;
			serialize<AvtpduControl>(acmpdu, buffer);
			serialize<Acmpdu>(acmpdu, buffer);
			return sendPayload(acmpdu.getDestAddress(), buffer);
		}
		catch ([[maybe_unused]] std::exception const& e)
		{
			LOG_PROTOCOL_INTERFACE_DEBUG(acmpdu.getSrcAddress(), Acmpdu::Multicast_Mac_Address, "Failed to serialize ACMPDU: {}", e.what());
			return Error::InternalError;
		}
	}

	/* *** Other methods **** */
	virtual std::uint32_t getVuAecpCommandTimeoutMsec(VuAecpdu::ProtocolIdentifier const& protocolIdentifier, VuAecpdu const& aecpdu) const noexcept override
	{
		return getVendorUniqueCommandTimeout(protocolIdentifier, aecpdu);
	}

	virtual bool isVuAecpUnsolicitedResponse(VuAecpdu::ProtocolIdentifier const& protocolIdentifier, VuAecpdu const& aecpdu) const noexcept override
	{
		return isVendorUniqueUnsolicitedResponse(protocolIdentifier, aecpdu);
	}

	/* ************************************************************ */
	/* stateMachine::DiscoveryStateMachine::Delegate overrides      */
	/* ************************************************************ */
	virtual void onLocalEntityOnline(entity::Entity const& entity) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityOnline, this, entity);
	}

	virtual void onLocalEntityOffline(UniqueIdentifier const entityID) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityOffline, this, entityID);
	}

	virtual void onLocalEntityUpdated(entity::Entity const& entity) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityUpdated, this, entity);
	}

	virtual void onRemoteEntityOnline(entity::Entity const& entity) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityOnline, this, entity);
	}

	virtual void onRemoteEntityOffline(UniqueIdentifier const entityID) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityOffline, this, entityID);
		_stateMachineManager.onRemoteEntityOffline(entityID);
	}

	virtual void onRemoteEntityUpdated(entity::Entity const& entity) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityUpdated, this, entity);
	}

	/* ************************************************************ */
	/* stateMachine::CommandStateMachine::Delegate overrides        */
	/* ************************************************************ */
	virtual void onAecpAemUnsolicitedResponse(AemAecpdu const& aecpdu) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpAemUnsolicitedResponse, this, aecpdu);
	}
	virtual void onAecpAemIdentifyNotification(AemAecpdu const& aecpdu) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpAemIdentifyNotification, this, aecpdu);
	}
	virtual void onAecpRetry(UniqueIdentifier const& entityID) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpRetry, this, entityID);
	}
	virtual void onAecpTimeout(UniqueIdentifier const& entityID) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpTimeout, this, entityID);
	}
	virtual void onAecpUnexpectedResponse(UniqueIdentifier const& entityID) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpUnexpectedResponse, this, entityID);
	}
	virtual void onAecpResponseTime(UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept override
	{
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpResponseTime, this, entityID, responseTime);
	}

	/* ************************************************************ */
	/* la::avdecc::utils::Subject overrides                         */
	/* ************************************************************ */
	virtual void onObserverRegistered(observer_type* const observer) noexcept override
	{
		if (observer)
		{
			class DiscoveryDelegate final : public stateMachine::DiscoveryStateMachine::Delegate
			{
			public:
				DiscoveryDelegate(ProtocolInterface& pi, ProtocolInterface::Observer& obs)
					: _pi{ pi }
					, _obs{ obs }
				{
				}

			private:
				virtual void onLocalEntityOnline(la::avdecc::entity::Entity const& entity) noexcept override
				{
					utils::invokeProtectedMethod(&ProtocolInterface::Observer::onLocalEntityOnline, &_obs, &_pi, entity);
				}
				virtual void onLocalEntityOffline(la::avdecc::UniqueIdentifier const /*entityID*/) noexcept override {}
				virtual void onLocalEntityUpdated(la::avdecc::entity::Entity const& /*entity*/) noexcept override {}
				virtual void onRemoteEntityOnline(la::avdecc::entity::Entity const& entity) noexcept override
				{
					utils::invokeProtectedMethod(&ProtocolInterface::Observer::onRemoteEntityOnline, &_obs, &_pi, entity);
				}
				virtual void onRemoteEntityOffline(la::avdecc::UniqueIdentifier const /*entityID*/) noexcept override {}
				virtual void onRemoteEntityUpdated(la::avdecc::entity::Entity const& /*entity*/) noexcept override {}

				ProtocolInterface& _pi;
				ProtocolInterface::Observer& _obs;
			};
			auto discoveryDelegate = DiscoveryDelegate{ *this, static_cast<ProtocolInterface::Observer&>(*observer) };

			_stateMachineManager.notifyDiscoveredEntities(discoveryDelegate);
		}
	}

	/* ************************************************************ */
	/* Private methods                                              */
	/* ************************************************************ */
	/** Possible outcomes for waitForStateAtLeast(). */
	enum class WaitResult
	{
		Reached, /**< Target state (or a later one excluding failed/cancelled) was reached. */
		Timeout, /**< Timeout elapsed without reaching the target state. */
		Failed, /**< Channel entered the failed or cancelled state before reaching the target state. */
	};

	/**
	* @brief Handler invoked by the nw_ethernet_channel on state transitions.
	* @details Updates the internal cached state, signals the state condition variable, and notifies
	*          observers of a transport error when the channel reaches the failed state after having
	*          been ready.
	*/
	void onChannelStateChanged(nw_ethernet_channel_state_t const state, nw_error_t _Nullable error) noexcept
	{
		auto notifyTransportError = false;
		{
			auto const lg = std::lock_guard{ _stateMutex };
			auto const wasReady = _currentState == nw_ethernet_channel_state_ready;
			_currentState = state;
			if ((state == nw_ethernet_channel_state_failed || state == nw_ethernet_channel_state_cancelled) && wasReady && !_shouldTerminate)
			{
				notifyTransportError = true;
			}
		}
		_stateCondition.notify_all();

		if (error != nullptr)
		{
			auto const errorCode = nw_error_get_error_code(error);
			LOG_PROTOCOL_INTERFACE_TRACE(la::networkInterface::MacAddress{}, la::networkInterface::MacAddress{}, "nw_ethernet_channel state {} with error code {}", static_cast<int>(state), errorCode);
		}

		if (notifyTransportError)
		{
			notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onTransportError, this);
		}
	}

	/**
	* @brief Wait until the channel reaches at least the specified state (or a failure/cancellation).
	* @return The outcome of the wait (reached, timed out, or failed).
	*/
	WaitResult waitForStateAtLeast(nw_ethernet_channel_state_t const target, std::chrono::steady_clock::duration const timeout) noexcept
	{
		auto lock = std::unique_lock{ _stateMutex };
		auto const result = _stateCondition.wait_for(lock, timeout,
			[this, target]()
			{
				return _currentState >= target || _currentState == nw_ethernet_channel_state_failed || _currentState == nw_ethernet_channel_state_cancelled;
			});

		if (!result)
		{
			return WaitResult::Timeout;
		}
		if (_currentState == nw_ethernet_channel_state_failed)
		{
			return WaitResult::Failed;
		}
		if (_currentState == nw_ethernet_channel_state_cancelled && target != nw_ethernet_channel_state_cancelled)
		{
			return WaitResult::Failed;
		}
		return WaitResult::Reached;
	}

	/**
	* @brief Cancel the channel and wait (with a short timeout) for it to reach the cancelled state.
	* @details Safe to call multiple times. Must be called before the ProtocolInterface is destroyed so
	*          any in-flight callback has completed before member objects are freed.
	*/
	void cancelAndWait() noexcept
	{
		_shouldTerminate = true;
		if (_channel == nullptr)
		{
			return;
		}

		auto shouldCancel = false;
		{
			auto const lg = std::lock_guard{ _stateMutex };
			if (_currentState != nw_ethernet_channel_state_cancelled && _currentState != nw_ethernet_channel_state_failed)
			{
				shouldCancel = true;
			}
		}
		if (shouldCancel)
		{
			nw_ethernet_channel_cancel(_channel);
			waitForStateAtLeast(nw_ethernet_channel_state_cancelled, ChannelCancelTimeout);
		}
	}

	/**
	* @brief Resolve the provided BSD interface name to a nw_interface_t.
	* @details Preferred path uses the private SPI `nw_interface_create_with_name` (dlsym'd from
	*          libnetwork.dylib) which works reliably for any BSD interface including unconfigured
	*          ones with no default route (e.g. a Thunderbolt Ethernet port directly wired to an
	*          AVB device). If the SPI is not available (unexpected on macOS 10.15+), falls back to
	*          the public path-monitor enumeration which only sees interfaces present in an active
	*          network path.
	* @return The resolved nw_interface_t, or nullptr if the interface cannot be resolved.
	*/
	nw_interface_t _Nullable resolveInterface(std::string const& interfaceName) noexcept
	{
		// Preferred path: private SPI nw_interface_create_with_name. This function reliably
		// resolves any BSD-named interface even if it has no IP configuration or default route.
		using nw_interface_create_with_name_t = nw_interface_t _Nullable (*)(char const*);
		static auto const s_interfaceCreateWithName = reinterpret_cast<nw_interface_create_with_name_t>(dlsym(RTLD_DEFAULT, "nw_interface_create_with_name"));
		if (s_interfaceCreateWithName != nullptr)
		{
			auto* const iface = s_interfaceCreateWithName(interfaceName.c_str());
			if (iface != nullptr)
			{
				return iface;
			}
			// Fall through to the public API fallback below.
		}

		auto const queue = dispatch_queue_create("com.l-acoustics.avdecc.macNCap.pathMonitor", DISPATCH_QUEUE_SERIAL);
		if (queue == nullptr)
		{
			return nullptr;
		}

		__block nw_interface_t foundInterface = nullptr;
		auto const semaphore = dispatch_semaphore_create(0);
		auto const foundMutex = std::make_shared<std::mutex>();
		auto const nameCopy = interfaceName;

		// List of path monitors to create (0 means default path monitor; otherwise type-specific monitor)
		auto const monitorTypes = std::vector<int>{ -1, static_cast<int>(nw_interface_type_wired), static_cast<int>(nw_interface_type_wifi), static_cast<int>(nw_interface_type_cellular), static_cast<int>(nw_interface_type_loopback), static_cast<int>(nw_interface_type_other) };
		auto monitors = std::vector<nw_path_monitor_t>{};
		monitors.reserve(monitorTypes.size());

		for (auto const type : monitorTypes)
		{
			auto const monitor = (type < 0) ? nw_path_monitor_create() : nw_path_monitor_create_with_type(static_cast<nw_interface_type_t>(type));
			if (monitor == nullptr)
			{
				continue;
			}
			nw_path_monitor_set_queue(monitor, queue);
			nw_path_monitor_set_update_handler(monitor, ^(nw_path_t path) {
				nw_path_enumerate_interfaces(path, ^bool(nw_interface_t interface) {
					auto const* const name = nw_interface_get_name(interface);
					if (name != nullptr && nameCopy == name)
					{
						auto const lg = std::lock_guard{ *foundMutex };
						if (foundInterface == nullptr)
						{
							foundInterface = interface;
							dispatch_semaphore_signal(semaphore);
						}
						return false;
					}
					return true;
				});
			});
			nw_path_monitor_start(monitor);
			monitors.push_back(monitor);
		}

		auto const waitTimeout = dispatch_time(DISPATCH_TIME_NOW, std::chrono::duration_cast<std::chrono::nanoseconds>(InterfaceDiscoveryTimeout).count());
		auto const timedOut = dispatch_semaphore_wait(semaphore, waitTimeout) != 0;

		for (auto const& monitor : monitors)
		{
			nw_path_monitor_cancel(monitor);
		}

		if (timedOut)
		{
			return nullptr;
		}
		return foundInterface;
	}

	/**
	* @brief Handler invoked by the nw_ethernet_channel when a frame is received.
	* @details Builds an EtherLayer2 wrapper and defers the actual AVDECC dispatch to the executor thread.
	*/
	void onFrameReceived(dispatch_data_t content, std::uint8_t const* const localAddress, std::uint8_t const* const remoteAddress) noexcept
	{
		if (content == nullptr)
		{
			return;
		}
		auto const length = dispatch_data_get_size(content);
		if (length == 0u)
		{
			return;
		}

		// Copy the received payload (it may be composed of multiple mapped regions)
		auto payload = std::vector<std::uint8_t>(length);
		auto* const payloadData = payload.data();
		__block auto offset = std::size_t{ 0u };
		dispatch_data_apply(content, ^bool(dispatch_data_t /*region*/, size_t /*regionOffset*/, void const* bytes, size_t size) {
			std::memcpy(payloadData + offset, bytes, size);
			offset += size;
			return true;
		});

		// Build EtherLayer2 from the addresses (remote = source, local = destination)
		auto etherLayer2 = EtherLayer2{};
		auto destMac = la::networkInterface::MacAddress{};
		auto srcMac = la::networkInterface::MacAddress{};
		for (auto i = std::size_t{ 0u }; i < 6u; ++i)
		{
			destMac[i] = localAddress[i];
			srcMac[i] = remoteAddress[i];
		}
		etherLayer2.setDestAddress(destMac);
		etherLayer2.setSrcAddress(srcMac);
		etherLayer2.setEtherType(AvtpEtherType);

		dispatchAvdeccPayload(payload.data(), payload.size(), etherLayer2);
	}

	/**
	* @brief Schedule the dispatch of an AVDECC payload on the executor thread.
	* @details The payload is expected to start at the AVTPDU (i.e. without the Ethernet L2 header).
	*/
	void dispatchAvdeccPayload(std::uint8_t const* const payload, std::size_t const length, EtherLayer2 const& etherLayer2) const noexcept
	{
		auto packet = la::avdecc::MemoryBuffer{ payload, length };
		la::avdecc::ExecutorManager::getInstance().pushJob(getExecutorName(),
			[this, packet = std::move(packet), etherLayer2]()
			{
				if (packet.size() == 0u)
				{
					return;
				}
				auto const* const avtpdu = packet.data();
				// Check AVTP control bit (meaning AVDECC packet)
				auto const avtpSubTypeControl = avtpdu[0];
				if ((avtpSubTypeControl & 0xF0) == 0)
				{
					return;
				}

				_watchDog.registerWatch("avdecc::MacNCap::dispatchAvdeccMessage::" + utils::toHexString(reinterpret_cast<std::size_t>(this)), std::chrono::milliseconds{ 1000u }, true);
				_ethernetPacketDispatcher.dispatchAvdeccMessage(avtpdu, packet.size(), etherLayer2);
				_watchDog.unregisterWatch("avdecc::MacNCap::dispatchAvdeccMessage::" + utils::toHexString(reinterpret_cast<std::size_t>(this)), true);
			});
	}

	/**
	* @brief Send the provided AVTPDU payload over the channel to the specified destination MAC.
	*/
	Error sendPayload(la::networkInterface::MacAddress const& destMac, SerializationBuffer const& buffer) const noexcept
	{
		if (_channel == nullptr)
		{
			return Error::TransportError;
		}

		{
			auto const lg = std::lock_guard{ _stateMutex };
			if (_currentState != nw_ethernet_channel_state_ready)
			{
				return Error::TransportError;
			}
		}

		auto const data = makeDispatchData(buffer.data(), buffer.size());
		if (data == nullptr)
		{
			return Error::InternalError;
		}

		nw_ethernet_address_t ethernetAddress{};
		for (auto i = std::size_t{ 0u }; i < 6u; ++i)
		{
			ethernetAddress[i] = destMac[i];
		}

		nw_ethernet_channel_send(_channel, data, 0u, ethernetAddress,
			^(nw_error_t _Nullable /*error*/){
				// Send completion is asynchronous and fire-and-forget here; errors surface indirectly via
				// channel state changes (onChannelStateChanged will emit onTransportError on channel failure).
			});

		// The completion is asynchronous; we don't wait for it to avoid blocking the caller, since errors
		// would only report that the frame failed to leave the host - we trust the caller to monitor
		// reception/timeout at a higher level, just like the pcap implementation does.
		return Error::NoError;
	}

	// Private variables
	watchDog::WatchDog::SharedPointer _watchDogSharedPointer{ watchDog::WatchDog::getInstance() };
	watchDog::WatchDog& _watchDog{ *_watchDogSharedPointer };
	nw_interface_t _Nullable _interface{ nullptr };
	nw_ethernet_channel_t _Nullable _channel{ nullptr };
	dispatch_queue_t _Nullable _channelQueue{ nullptr };
	mutable std::mutex _stateMutex{};
	std::condition_variable _stateCondition{};
	nw_ethernet_channel_state_t _currentState{ nw_ethernet_channel_state_invalid };
	std::atomic<bool> _shouldTerminate{ false };
	mutable stateMachine::Manager _stateMachineManager{ this, this, this, this, this };
	friend class EthernetPacketDispatcher<ProtocolInterfaceMacNCapImpl>;
	EthernetPacketDispatcher<ProtocolInterfaceMacNCapImpl> _ethernetPacketDispatcher{ this, _stateMachineManager };
};

ProtocolInterfaceMacNCap::ProtocolInterfaceMacNCap(std::string const& networkInterfaceID, std::string const& executorName)
	: ProtocolInterface(networkInterfaceID, executorName)
{
}

bool ProtocolInterfaceMacNCap::isSupported() noexcept
{
	// nw_ethernet_channel is available on macOS 10.15+. The project minimum is already macOS 10.13,
	// but the symbol itself is weak-linked; a runtime check for the symbol's availability is enough.
	if (@available(macOS 10.15, *))
	{
		return true;
	}
	return false;
}

ProtocolInterfaceMacNCap* ProtocolInterfaceMacNCap::createRawProtocolInterfaceMacNCap(std::string const& networkInterfaceID, std::string const& executorName)
{
	return new ProtocolInterfaceMacNCapImpl(networkInterfaceID, executorName);
}

} // namespace protocol
} // namespace avdecc
} // namespace la

#pragma clang diagnostic pop
