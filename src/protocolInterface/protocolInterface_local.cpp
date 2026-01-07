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
* @file protocolInterface_local.cpp
* @author Luke Howard
*/

#include "la/avdecc/internals/serialization.hpp"
#include "la/avdecc/internals/protocolAemAecpdu.hpp"
#include "la/avdecc/internals/protocolAaAecpdu.hpp"
#include "la/avdecc/watchDog.hpp"
#include "la/avdecc/utils.hpp"
#include "la/avdecc/executor.hpp"

#include "stateMachine/stateMachineManager.hpp"
#include "ethernetPacketDispatch.hpp"
#include "protocolInterface_local.hpp"
#include "logHelper.hpp"


#include <stdexcept>
#include <sstream>
#include <array>
#include <thread>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>
#include <cstdlib>
#include <ctime>

#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace la
{
namespace avdecc
{
namespace protocol
{
static constexpr int SocketReceiveLoopTimeout = 250u;

static constexpr networkInterface::MacAddress Local_Mac_Address = { 0x0A, 0xE9, 0x1B, 0x01, 0x01, 0x01 };
static constexpr networkInterface::MacAddress Peer_Mac_Address = { 0x0A, 0xE9, 0x1B, 0xFF, 0xFF, 0xFF };

class ProtocolInterfaceLocalImpl final : public ProtocolInterfaceLocal, private stateMachine::ProtocolInterfaceDelegate, private stateMachine::AdvertiseStateMachine::Delegate, private stateMachine::DiscoveryStateMachine::Delegate, private stateMachine::CommandStateMachine::Delegate
{
public:
	/* ************************************************************ */
	/* Public APIs                                                  */
	/* ************************************************************ */
	/** Constructor */
	ProtocolInterfaceLocalImpl(std::string const& networkInterfaceName, std::string const& executorName)
		: ProtocolInterfaceLocal(networkInterfaceName, Local_Mac_Address, executorName)
	{
		// open socket
		sockaddr_un sun{};

		_fd = socket(AF_LOCAL, SOCK_DGRAM, 0);
		if (_fd < 0)
		{
			throw Exception(Error::TransportError, "Failed to create local domain socket");
		}

		sun.sun_family = AF_LOCAL;
		if (networkInterfaceName.size() >= sizeof(sun.sun_path))
		{
			throw Exception(Error::InvalidParameters, "Local domain socket path too long");
		}
		strncpy(sun.sun_path, networkInterfaceName.c_str(), sizeof(sun.sun_path) - 1);
#if defined(__APPLE__)
		sun.sun_len = SUN_LEN(&sun);
#endif

		auto err = connect(_fd, reinterpret_cast<sockaddr*>(&sun), sizeof(sun));
		if (err < 0)
		{
			throw Exception(Error::TransportError, "Failed to connect local domain socket to peer");
		}

		// Start the capture thread
		_captureThread = std::thread(
			[this]
			{
				utils::setCurrentThreadName("avdecc::LocalInterface::Capture");
				socketReceiveLoop();
				if (!_shouldTerminate)
				{
					notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onTransportError, this);
				}
			});

		// Start the state machines
		_stateMachineManager.startStateMachines();
	}

	/** Destructor */
	virtual ~ProtocolInterfaceLocalImpl() noexcept
	{
		shutdown();
	}

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept override
	{
		delete this;
	}

	// Deleted compiler auto-generated methods
	ProtocolInterfaceLocalImpl(ProtocolInterfaceLocalImpl&&) = delete;
	ProtocolInterfaceLocalImpl(ProtocolInterfaceLocalImpl const&) = delete;
	ProtocolInterfaceLocalImpl& operator=(ProtocolInterfaceLocalImpl const&) = delete;
	ProtocolInterfaceLocalImpl& operator=(ProtocolInterfaceLocalImpl&&) = delete;

private:
	/* ************************************************************ */
	/* ProtocolInterface overrides                                  */
	/* ************************************************************ */
	virtual void shutdown() noexcept override
	{
		// Stop the state machines
		_stateMachineManager.stopStateMachines();

		// Notify the thread we are shutting down
		_shouldTerminate = true;

		// Wait for the thread to complete its pending tasks
		if (_captureThread.joinable())
		{
			_captureThread.join();
		}

		// Flush executor jobs
		la::avdecc::ExecutorManager::getInstance().flush(getExecutorName());

		// Close underlying file descriptor.
		if (_fd != -1)
		{
			close(_fd);
		}
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
		// Checks if entity has declared an InterfaceInformation matching this ProtocolInterface
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
		processRawPacket(std::move(packet));
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
			_stateMachineManager.discoverMessageSent(); // Notify we are sending a discover message
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

		// Command goes through the state machine to handle timeout, retry and response
		return _stateMachineManager.sendAecpCommand(std::move(aecpdu), onResult);
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

		// Response can be directly sent
		return sendMessage(static_cast<Aecpdu const&>(*aecpdu));
	}

	virtual Error sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, AcmpCommandResultHandler const& onResult) const noexcept override
	{
		// Command goes through the state machine to handle timeout, retry and response
		return _stateMachineManager.sendAcmpCommand(std::move(acmpdu), onResult);
	}

	virtual Error sendAcmpResponse(Acmpdu::UniquePointer&& acmpdu) const noexcept override
	{
		// Response can be directly sent
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
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpCommand, this, aecpdu);
	}

	virtual void onVuAecpUnsolicitedResponse(VuAecpdu::ProtocolIdentifier const& protocolIdentifier, VuAecpdu const& aecpdu) noexcept override
	{
		handleVendorUniqueUnsolicitedResponse(protocolIdentifier, aecpdu);
	}

	/* **** ACMP notifications **** */
	virtual void onAcmpCommand(Acmpdu const& acmpdu) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAcmpCommand, this, acmpdu);
	}

	virtual void onAcmpResponse(Acmpdu const& acmpdu) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAcmpResponse, this, acmpdu);
	}

	/* **** Sending methods **** */
	virtual Error sendMessage(Adpdu const& adpdu) const noexcept override
	{
		try
		{
			SerializationBuffer buffer;

			// Then Avtp control
			serialize<AvtpduControl>(adpdu, buffer);
			// Then with Adp
			serialize<Adpdu>(adpdu, buffer);

			// Send the message
			return sendPacket(buffer);
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

			// Then Avtp control
			serialize<AvtpduControl>(aecpdu, buffer);
			// Then with Aecp
			serialize<Aecpdu>(aecpdu, buffer);

			// Send the message
			return sendPacket(buffer);
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

			// Then Avtp control
			serialize<AvtpduControl>(acmpdu, buffer);
			// Then with Acmp
			serialize<Acmpdu>(acmpdu, buffer);

			// Send the message
			return sendPacket(buffer);
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
	/* stateMachine::AdvertiseStateMachine::Delegate overrides      */
	/* ************************************************************ */

	/* ************************************************************ */
	/* stateMachine::DiscoveryStateMachine::Delegate overrides      */
	/* ************************************************************ */
	virtual void onLocalEntityOnline(entity::Entity const& entity) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityOnline, this, entity);
	}

	virtual void onLocalEntityOffline(UniqueIdentifier const entityID) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityOffline, this, entityID);
	}

	virtual void onLocalEntityUpdated(entity::Entity const& entity) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityUpdated, this, entity);
	}

	virtual void onRemoteEntityOnline(entity::Entity const& entity) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityOnline, this, entity);
	}

	virtual void onRemoteEntityOffline(UniqueIdentifier const entityID) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityOffline, this, entityID);

		// Notify the StateMachineManager
		_stateMachineManager.onRemoteEntityOffline(entityID);
	}

	virtual void onRemoteEntityUpdated(entity::Entity const& entity) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityUpdated, this, entity);
	}

	/* ************************************************************ */
	/* stateMachine::CommandStateMachine::Delegate overrides        */
	/* ************************************************************ */
	virtual void onAecpAemUnsolicitedResponse(AemAecpdu const& aecpdu) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpAemUnsolicitedResponse, this, aecpdu);
	}
	virtual void onAecpAemIdentifyNotification(AemAecpdu const& aecpdu) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpAemIdentifyNotification, this, aecpdu);
	}
	virtual void onAecpRetry(UniqueIdentifier const& entityID) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpRetry, this, entityID);
	}
	virtual void onAecpTimeout(UniqueIdentifier const& entityID) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpTimeout, this, entityID);
	}
	virtual void onAecpUnexpectedResponse(UniqueIdentifier const& entityID) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpUnexpectedResponse, this, entityID);
	}
	virtual void onAecpResponseTime(UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept override
	{
		// Notify observers
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
	void processRawPacket(la::avdecc::MemoryBuffer&& packet) const noexcept
	{
		la::avdecc::ExecutorManager::getInstance().pushJob(getExecutorName(),
			[this, msg = std::move(packet)]()
			{
				std::uint8_t const* avtpdu = msg.data(); // Start of AVB Transport Protocol
				auto avtpdu_size = msg.size();

				auto etherLayer2 = EtherLayer2{};
				etherLayer2.setEtherType(AvtpEtherType);
				etherLayer2.setSrcAddress(Peer_Mac_Address);
				etherLayer2.setDestAddress(Local_Mac_Address);

				// Try to detect possible deadlock
				{
					_watchDog.registerWatch("avdecc::LocalInterface::dispatchAvdeccMessage::" + utils::toHexString(reinterpret_cast<size_t>(this)), std::chrono::milliseconds{ 1000u }, true);
					_ethernetPacketDispatcher.dispatchAvdeccMessage(avtpdu, avtpdu_size, etherLayer2);
					_watchDog.unregisterWatch("avdecc::LocalInterface::dispatchAvdeccMessage::" + utils::toHexString(reinterpret_cast<size_t>(this)), true);
				}
			});
	}

	void socketReceiveLoop(void) noexcept
	{
		struct ::pollfd pollfd;
		std::uint8_t payloadBuffer[AvtpMaxPayloadLength];

		pollfd.fd = _fd;

		while (!_shouldTerminate)
		{
			auto iov = iovec{ payloadBuffer, sizeof(payloadBuffer) };
			msghdr msg{};

			pollfd.events = POLLIN;
			pollfd.revents = 0;

			auto const err = poll(&pollfd, 1, SocketReceiveLoopTimeout); // timeout so we can check _shouldTerminate
			if (err < 0)
			{
				break;
			}
			else if (err == 0 || pollfd.events != POLLIN)
			{
				continue; // timed out or no input events
			}

			msg.msg_iov = &iov;
			msg.msg_iovlen = 1;

			auto const bytesReceived = recvmsg(_fd, &msg, 0);
			if (bytesReceived > 0)
			{
				auto message = la::avdecc::MemoryBuffer{ payloadBuffer, static_cast<std::size_t>(bytesReceived) };
				processRawPacket(std::move(message));
			}
		}
	}

	Error sendPacket(SerializationBuffer const& buffer) const noexcept
	{
		auto iov = iovec{ const_cast<void*>(reinterpret_cast<const void*>(buffer.data())), buffer.size() };
		msghdr msg{};

		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		auto const bytesSent = sendmsg(_fd, &msg, 0);
		if (static_cast<size_t>(bytesSent) != buffer.size())
		{
			return Error::TransportError;
		}

		return Error::NoError;
	}

	// Private variables
	watchDog::WatchDog::SharedPointer _watchDogSharedPointer{ watchDog::WatchDog::getInstance() };
	watchDog::WatchDog& _watchDog{ *_watchDogSharedPointer };
	int _fd{ -1 };
	bool _shouldTerminate{ false };
	mutable stateMachine::Manager _stateMachineManager{ this, this, this, this, this };
	std::thread _captureThread{};
	friend class EthernetPacketDispatcher<ProtocolInterfaceLocalImpl>;
	EthernetPacketDispatcher<ProtocolInterfaceLocalImpl> _ethernetPacketDispatcher{ this, _stateMachineManager };
};

ProtocolInterfaceLocal::ProtocolInterfaceLocal(std::string const& networkInterfaceName, networkInterface::MacAddress const& macAddress, std::string const& executorName)
	: ProtocolInterface(networkInterfaceName, macAddress, executorName)
{
}

bool ProtocolInterfaceLocal::isSupported() noexcept
{
	return true;
}

ProtocolInterfaceLocal* ProtocolInterfaceLocal::createRawProtocolInterfaceLocal(std::string const& networkInterfaceName, std::string const& executorName)
{
	return new ProtocolInterfaceLocalImpl(networkInterfaceName, executorName);
}

} // namespace protocol
} // namespace avdecc
} // namespace la
