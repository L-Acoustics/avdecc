/*
* Copyright (C) 2016-2020, L-Acoustics and its contributors

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
* @file protocolInterface_virtual.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/serialization.hpp"
#include "la/avdecc/internals/protocolAemAecpdu.hpp"
#include "la/avdecc/internals/protocolAaAecpdu.hpp"
#include "la/avdecc/internals/protocolMvuAecpdu.hpp"
#include "la/avdecc/internals/instrumentationNotifier.hpp"
#include "la/avdecc/watchDog.hpp"
#include "la/avdecc/utils.hpp"

#include "stateMachine/stateMachineManager.hpp"
#include "protocolInterface_virtual.hpp"
#include "logHelper.hpp"

#include <stdexcept>
#include <thread>
#include <condition_variable>
#include <list>
#include <mutex>
#include <memory>
#include <functional>
#include <atomic>

// Only enable instrumentation in static library and in debug (for unit testing mainly)
#if defined(DEBUG) && defined(la_avdecc_cxx_STATICS)
#	include <chrono>
#	define SEND_INSTRUMENTATION_NOTIFICATION(eventName) la::avdecc::InstrumentationNotifier::getInstance().triggerEvent(eventName)
#	define UNIQUE_LOCK(mutex, sleepDelay, retryCount) \
		la::avdecc::InstrumentationNotifier::getInstance().triggerEvent("ProtocolInterfaceVirtual::PushMessage::PreLock"); \
		std::unique_lock<decltype(mutex)> lock(mutex, std::defer_lock); \
		{ \
			std::uint8_t count{ retryCount }; \
			while (count > 0) \
			{ \
				if (lock.try_lock()) \
					break; \
				std::this_thread::sleep_for(sleepDelay); \
				--count; \
			} \
			if (!lock.owns_lock()) \
			{ \
				la::avdecc::InstrumentationNotifier::getInstance().triggerEvent("ProtocolInterfaceVirtual::PushMessage::LockTimeOut"); \
				lock.lock(); \
			} \
		} \
		la::avdecc::InstrumentationNotifier::getInstance().triggerEvent(std::string("ProtocolInterfaceVirtual::PushMessage::PostLock"))
#else // !DEBUG || !la_avdecc_cxx_STATICS
#	define SEND_INSTRUMENTATION_NOTIFICATION(eventName)
#	define UNIQUE_LOCK(mutex, sleepDelay, retryCount) std::unique_lock<decltype(mutex)> lock(mutex)
#endif // DEBUG && la_avdecc_cxx_STATICS

namespace la
{
namespace avdecc
{
namespace protocol
{
class MessageDispatcher final
{
	using Subject = utils::TypedSubject<struct SubjectTag, std::mutex>;
	using MessagesList = std::list<SerializationBuffer>;
	struct Interface
	{
		std::atomic_bool shouldTerminate{ false };
		std::mutex mutex;
		std::thread dispatchThread{};
		Subject observers{};
		MessagesList messages{};
		std::condition_variable cond{};

		~Interface()
		{
			// Notify the thread we shall terminate
			shouldTerminate = true;
			cond.notify_all();

			// Wait for the thread to complete its pending tasks
			if (dispatchThread.joinable())
				dispatchThread.join();
		}
	};

public:
	class Observer : public utils::Observer<Subject>
	{
	public:
		virtual void onMessage(SerializationBuffer const& message) noexcept = 0;
		virtual void onTransportError() noexcept = 0;
	};

	static MessageDispatcher& getInstance() noexcept
	{
		static MessageDispatcher s_dispatcher{};
		return s_dispatcher;
	}

	void registerObserver(std::string const& networkInterfaceName, Observer* const observer) noexcept
	{
		std::lock_guard<decltype(_mutex)> const lg(_mutex);

		auto interfaceIt = _interfaces.find(networkInterfaceName);

		// Virtual interface not created yet
		if (interfaceIt == _interfaces.end())
		{
			auto intfc = std::make_unique<Interface>();
			intfc->dispatchThread = std::thread(
				[networkInterfaceName, intfc = intfc.get()]()
				{
					utils::setCurrentThreadName("avdecc::VirtualInterface." + networkInterfaceName + "::Capture");
					while (!intfc->shouldTerminate)
					{
						MessagesList messagesToSend{};

						// Wait for one (or more) message to be available (while under the lock), or for shouldTerminate to be set
						{
							std::unique_lock<decltype(intfc->mutex)> lock(intfc->mutex);

							// Wait for message in the queue
							intfc->cond.wait(lock,
								[intfc]
								{
									return intfc->messages.size() > 0 || intfc->shouldTerminate;
								});

							// Empty the queue
							while (!intfc->shouldTerminate && intfc->messages.size() > 0)
							{
								// Pop a message from the queue
								messagesToSend.push_back(std::move(intfc->messages.front()));
								intfc->messages.pop_front();

								SEND_INSTRUMENTATION_NOTIFICATION("ProtocolInterfaceVirtual::onMessage::PostLock");
							}
						}

						// Now we can send messages without locking
						while (!intfc->shouldTerminate && messagesToSend.size() > 0)
						{
							auto const& message = messagesToSend.front();

							// Transport error
							if (message.size() == 0)
							{
								intfc->observers.notifyObservers<Observer>(
									[](auto* obs)
									{
										obs->onTransportError();
									});
								intfc->shouldTerminate = true;
								break;
							}

							// Notify registered observers
							intfc->observers.notifyObservers<Observer>(
								[&message](auto* obs)
								{
									obs->onMessage(message);
								});
							messagesToSend.pop_front();
						}
					}
				});
			auto result = _interfaces.emplace(std::make_pair(networkInterfaceName, std::move(intfc)));
			// Insertion failed
			if (!result.second)
				return;
			interfaceIt = result.first;
		}

		// Register observer
		auto& intfc = *interfaceIt->second;
		try
		{
			intfc.observers.registerObserver(observer);
		}
		catch (std::invalid_argument const&)
		{
		}
	}

	void unregisterObserver(std::string const& networkInterfaceName, Observer* const observer) noexcept
	{
		std::lock_guard<decltype(_mutex)> const lg(_mutex);

		auto interfaceIt = _interfaces.find(networkInterfaceName);

		// Interface does not exist, ignore the message
		if (interfaceIt == _interfaces.end())
			return;

		// Unregister observer
		auto& intfc = *interfaceIt->second;
		try
		{
			intfc.observers.unregisterObserver(observer);
		}
		catch (std::invalid_argument const&)
		{
		}

		// If last observer for this interface, remove the interface (effectively waiting for the dispatch thread to shutdown)
		if (intfc.observers.countObservers() == 0)
		{
			_interfaces.erase(interfaceIt);
		}
	}

	void push(std::string const& networkInterfaceName, SerializationBuffer message)
	{
		std::lock_guard<decltype(_mutex)> const lg(_mutex);

		auto interfaceIt = _interfaces.find(networkInterfaceName);

		// Interface does not exist, ignore the message
		if (interfaceIt == _interfaces.end())
			return;

		// Add message to the queue
		auto& intfc = *interfaceIt->second;

		UNIQUE_LOCK(intfc.mutex, std::chrono::milliseconds(10), 100);

		intfc.messages.push_back(std::move(message));

		// Notify the dispatch thread
		intfc.cond.notify_all();
	}

	// Deleted compiler auto-generated methods
	MessageDispatcher(MessageDispatcher&&) = delete;
	MessageDispatcher(MessageDispatcher const&) = delete;
	MessageDispatcher& operator=(MessageDispatcher const&) = delete;
	MessageDispatcher& operator=(MessageDispatcher&&) = delete;

private:
	MessageDispatcher() = default;

	~MessageDispatcher() noexcept {}

	// Private variables
	std::mutex _mutex;
	std::unordered_map<std::string, std::unique_ptr<Interface>> _interfaces{};
};


static networkInterface::MacAddress Multicast_Mac_Address{ { 0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00 } };
static networkInterface::MacAddress Identify_Mac_Address{ { 0x91, 0xe0, 0xf0, 0x01, 0x00, 0x01 } };

class ProtocolInterfaceVirtualImpl final : public ProtocolInterfaceVirtual, private stateMachine::ProtocolInterfaceDelegate, private stateMachine::AdvertiseStateMachine::Delegate, private stateMachine::DiscoveryStateMachine::Delegate, private stateMachine::CommandStateMachine::Delegate, private MessageDispatcher::Observer
{
public:
	/* ************************************************************ */
	/* Public APIs                                                  */
	/* ************************************************************ */
	/** Constructor */
	ProtocolInterfaceVirtualImpl(std::string const& networkInterfaceName, networkInterface::MacAddress const& macAddress);

	/** Destructor */
	virtual ~ProtocolInterfaceVirtualImpl() noexcept;

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept override;

	// Deleted compiler auto-generated methods
	ProtocolInterfaceVirtualImpl(ProtocolInterfaceVirtualImpl&&) = delete;
	ProtocolInterfaceVirtualImpl(ProtocolInterfaceVirtualImpl const&) = delete;
	ProtocolInterfaceVirtualImpl& operator=(ProtocolInterfaceVirtualImpl const&) = delete;
	ProtocolInterfaceVirtualImpl& operator=(ProtocolInterfaceVirtualImpl&&) = delete;

private:
	/* ************************************************************ */
	/* ProtocolInterface overrides                                  */
	/* ************************************************************ */
	virtual void shutdown() noexcept override;
	virtual UniqueIdentifier getDynamicEID() const noexcept override;
	virtual void releaseDynamicEID(UniqueIdentifier const entityID) const noexcept override;
	virtual Error registerLocalEntity(entity::LocalEntity& entity) noexcept override;
	virtual Error unregisterLocalEntity(entity::LocalEntity& entity) noexcept override;
	virtual Error setEntityNeedsAdvertise(entity::LocalEntity const& entity, entity::LocalEntity::AdvertiseFlags const flags) noexcept override;
	virtual Error enableEntityAdvertising(entity::LocalEntity& entity) noexcept override;
	virtual Error disableEntityAdvertising(entity::LocalEntity const& entity) noexcept override;
	virtual Error discoverRemoteEntities() const noexcept override;
	virtual Error discoverRemoteEntity(UniqueIdentifier const entityID) const noexcept override;
	virtual Error setAutomaticDiscoveryDelay(std::chrono::milliseconds const delay) const noexcept override;
	virtual bool isDirectMessageSupported() const noexcept override;
	virtual Error sendAdpMessage(Adpdu const& adpdu) const noexcept override;
	virtual Error sendAecpMessage(Aecpdu const& aecpdu) const noexcept override;
	virtual Error sendAcmpMessage(Acmpdu const& acmpdu) const noexcept override;
	virtual Error sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, AecpCommandResultHandler const& onResult) const noexcept override;
	virtual Error sendAecpResponse(Aecpdu::UniquePointer&& aecpdu) const noexcept override;
	virtual Error sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, AcmpCommandResultHandler const& onResult) const noexcept override;
	virtual Error sendAcmpResponse(Acmpdu::UniquePointer&& acmpdu) const noexcept override;
	virtual void lock() const noexcept override;
	virtual void unlock() const noexcept override;
	virtual bool isSelfLocked() const noexcept override;

	/* ************************************************************ */
	/* ProtocolInterfaceVirtual overrides                           */
	/* ************************************************************ */
	virtual void forceTransportError() const noexcept override;

	/* ************************************************************ */
	/* stateMachine::ProtocolInterfaceDelegate overrides            */
	/* ************************************************************ */
	virtual void onAecpCommand(Aecpdu const& aecpdu) noexcept override;
	virtual void onAcmpCommand(Acmpdu const& acmpdu) noexcept override;
	virtual void onAcmpResponse(Acmpdu const& acmpdu) noexcept override;
	virtual Error sendMessage(Adpdu const& adpdu) const noexcept override;
	virtual Error sendMessage(Aecpdu const& aecpdu) const noexcept override;
	virtual Error sendMessage(Acmpdu const& acmpdu) const noexcept override;
	virtual std::uint32_t getVuAecpCommandTimeoutMsec(VuAecpdu::ProtocolIdentifier const& protocolIdentifier, VuAecpdu const& aecpdu) const noexcept override;

	/* ************************************************************ */
	/* stateMachine::AdvertiseStateMachine::Delegate overrides      */
	/* ************************************************************ */

	/* ************************************************************ */
	/* stateMachine::DiscoveryStateMachine::Delegate overrides      */
	/* ************************************************************ */
	virtual void onLocalEntityOnline(entity::Entity const& entity) noexcept override;
	virtual void onLocalEntityOffline(UniqueIdentifier const entityID) noexcept override;
	virtual void onLocalEntityUpdated(entity::Entity const& entity) noexcept override;
	virtual void onRemoteEntityOnline(entity::Entity const& entity) noexcept override;
	virtual void onRemoteEntityOffline(UniqueIdentifier const entityID) noexcept override;
	virtual void onRemoteEntityUpdated(entity::Entity const& entity) noexcept override;

	/* ************************************************************ */
	/* stateMachine::CommandStateMachine::Delegate overrides        */
	/* ************************************************************ */
	virtual void onAecpAemUnsolicitedResponse(AemAecpdu const& aecpdu) noexcept override;
	virtual void onAecpAemIdentifyNotification(AemAecpdu const& aecpdu) noexcept override;
	virtual void onAecpRetry(UniqueIdentifier const& entityID) noexcept override;
	virtual void onAecpTimeout(UniqueIdentifier const& entityID) noexcept override;
	virtual void onAecpUnexpectedResponse(UniqueIdentifier const& entityID) noexcept override;
	virtual void onAecpResponseTime(UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept override;

	/* ************************************************************ */
	/* MessageDispatcher::Observer overrides                        */
	/* ************************************************************ */
	virtual void onMessage(SerializationBuffer const& message) noexcept override;
	virtual void onTransportError() noexcept override;

	/* ************************************************************ */
	/* Private methods                                              */
	/* ************************************************************ */
	Error sendPacket(SerializationBuffer const& buffer) const noexcept;
	void deserializeAecpMessage(EtherLayer2 const& etherLayer2, Deserializer& des, Aecpdu& aecp) const;
	void dispatchAvdeccMessage(std::uint8_t const* const pkt_data, size_t const pkt_len, EtherLayer2 const& etherLayer2) noexcept;

	// Private variables
	mutable stateMachine::Manager _stateMachineManager{ this, this, this, this, this };
};

/* ************************************************************ */
/* Public APIs                                                  */
/* ************************************************************ */
/** Constructor */
ProtocolInterfaceVirtualImpl::ProtocolInterfaceVirtualImpl(std::string const& networkInterfaceName, networkInterface::MacAddress const& macAddress)
	: ProtocolInterfaceVirtual(networkInterfaceName, macAddress)
{
	// Should always be supported. Cannot create a Virtual ProtocolInterface if it's not supported.
	AVDECC_ASSERT(isSupported(), "Should always be supported. Cannot create a Virtual ProtocolInterface if it's not supported");

	// Register to the message dispatcher
	auto& dispatcher = MessageDispatcher::getInstance();
	dispatcher.registerObserver(networkInterfaceName, this);

	// Start the state machines
	_stateMachineManager.startStateMachines();
}

/** Destructor */
ProtocolInterfaceVirtualImpl::~ProtocolInterfaceVirtualImpl() noexcept
{
	shutdown();
}

void ProtocolInterfaceVirtualImpl::destroy() noexcept
{
	delete this;
}

/* ************************************************************ */
/* ProtocolInterface overrides                                  */
/* ************************************************************ */
void ProtocolInterfaceVirtualImpl::shutdown() noexcept
{
	// Stop the state machines
	_stateMachineManager.stopStateMachines();

	// Unregister from the message dispatcher
	auto& dispatcher = MessageDispatcher::getInstance();
	dispatcher.unregisterObserver(_networkInterfaceName, this);
}

UniqueIdentifier ProtocolInterfaceVirtualImpl::getDynamicEID() const noexcept
{
	UniqueIdentifier::value_type eid{ 0u };
	auto const& macAddress = getMacAddress();
	static auto s_CurrentProgID = std::uint16_t{ 0u };

	eid += macAddress[0];
	eid <<= 8;
	eid += macAddress[1];
	eid <<= 8;
	eid += macAddress[2];
	eid <<= 16;
	eid += ++s_CurrentProgID;
	eid <<= 8;
	eid += macAddress[3];
	eid <<= 8;
	eid += macAddress[4];
	eid <<= 8;
	eid += macAddress[5];

	return UniqueIdentifier{ eid };
}

void ProtocolInterfaceVirtualImpl::releaseDynamicEID(UniqueIdentifier const /*entityID*/) const noexcept
{
	// Nothing to do
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::registerLocalEntity(entity::LocalEntity& entity) noexcept
{
	// Checks if entity has declared an InterfaceInformation matching this ProtocolInterface
	auto const index = _stateMachineManager.getMatchingInterfaceIndex(entity);

	if (index)
	{
		return _stateMachineManager.registerLocalEntity(entity);
	}

	return Error::InvalidParameters;
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::unregisterLocalEntity(entity::LocalEntity& entity) noexcept
{
	return _stateMachineManager.unregisterLocalEntity(entity);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::setEntityNeedsAdvertise(entity::LocalEntity const& entity, entity::LocalEntity::AdvertiseFlags const /*flags*/) noexcept
{
	return _stateMachineManager.setEntityNeedsAdvertise(entity);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::enableEntityAdvertising(entity::LocalEntity& entity) noexcept
{
	return _stateMachineManager.enableEntityAdvertising(entity);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::disableEntityAdvertising(entity::LocalEntity const& entity) noexcept
{
	return _stateMachineManager.disableEntityAdvertising(entity);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::discoverRemoteEntities() const noexcept
{
	return _stateMachineManager.discoverRemoteEntities();
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::discoverRemoteEntity(UniqueIdentifier const entityID) const noexcept
{
	return _stateMachineManager.discoverRemoteEntity(entityID);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::setAutomaticDiscoveryDelay(std::chrono::milliseconds const delay) const noexcept
{
	return _stateMachineManager.setAutomaticDiscoveryDelay(delay);
}

bool ProtocolInterfaceVirtualImpl::isDirectMessageSupported() const noexcept
{
	return true;
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendAdpMessage(Adpdu const& adpdu) const noexcept
{
	// Directly send the message on the network
	return sendMessage(adpdu);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendAecpMessage(Aecpdu const& aecpdu) const noexcept
{
	// Directly send the message on the network
	return sendMessage(aecpdu);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendAcmpMessage(Acmpdu const& acmpdu) const noexcept
{
	// Directly send the message on the network
	return sendMessage(acmpdu);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, AecpCommandResultHandler const& onResult) const noexcept
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

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendAecpResponse(Aecpdu::UniquePointer&& aecpdu) const noexcept
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

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, AcmpCommandResultHandler const& onResult) const noexcept
{
	// Command goes through the state machine to handle timeout, retry and response
	return _stateMachineManager.sendAcmpCommand(std::move(acmpdu), onResult);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendAcmpResponse(Acmpdu::UniquePointer&& acmpdu) const noexcept
{
	// Response can be directly sent
	return sendMessage(static_cast<Acmpdu const&>(*acmpdu));
}

void ProtocolInterfaceVirtualImpl::lock() const noexcept
{
	_stateMachineManager.lock();
}

void ProtocolInterfaceVirtualImpl::unlock() const noexcept
{
	_stateMachineManager.unlock();
}

bool ProtocolInterfaceVirtualImpl::isSelfLocked() const noexcept
{
	return _stateMachineManager.isSelfLocked();
}

/* ************************************************************ */
/* ProtocolInterfaceVirtual overrides                           */
/* ************************************************************ */
void ProtocolInterfaceVirtualImpl::forceTransportError() const noexcept
{
	sendPacket({});
}

/* ************************************************************ */
/* stateMachine::ProtocolInterfaceDelegate overrides            */
/* ************************************************************ */
void ProtocolInterfaceVirtualImpl::onAecpCommand(Aecpdu const& aecpdu) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpCommand, this, aecpdu);
}

void ProtocolInterfaceVirtualImpl::onAcmpCommand(Acmpdu const& acmpdu) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAcmpCommand, this, acmpdu);
}

void ProtocolInterfaceVirtualImpl::onAcmpResponse(Acmpdu const& acmpdu) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAcmpResponse, this, acmpdu);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendMessage(Adpdu const& adpdu) const noexcept
{
	try
	{
		// Virtual transport requires the full frame to be built
		SerializationBuffer buffer;

		// Start with EtherLayer2
		serialize<EtherLayer2>(adpdu, buffer);
		// Then Avtp control
		serialize<AvtpduControl>(adpdu, buffer);
		// Then with Adp
		serialize<Adpdu>(adpdu, buffer);

		// Send the message
		return sendPacket(buffer);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_GENERIC_DEBUG(std::string("Failed to serialize ADPDU: ") + e.what());
		return Error::InternalError;
	}
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendMessage(Aecpdu const& aecpdu) const noexcept
{
	try
	{
		// Virtual transport requires the full frame to be built
		SerializationBuffer buffer;

		// Start with EtherLayer2
		serialize<EtherLayer2>(aecpdu, buffer);
		// Then Avtp control
		serialize<AvtpduControl>(aecpdu, buffer);
		// Then with Aecp
		serialize<Aecpdu>(aecpdu, buffer);

		// Send the message
		return sendPacket(buffer);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_GENERIC_DEBUG(std::string("Failed to serialize AECPDU: ") + e.what());
		return Error::InternalError;
	}
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendMessage(Acmpdu const& acmpdu) const noexcept
{
	try
	{
		// Virtual transport requires the full frame to be built
		SerializationBuffer buffer;

		// Start with EtherLayer2
		serialize<EtherLayer2>(acmpdu, buffer);
		// Then Avtp control
		serialize<AvtpduControl>(acmpdu, buffer);
		// Then with Acmp
		serialize<Acmpdu>(acmpdu, buffer);

		// Send the message
		return sendPacket(buffer);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_GENERIC_DEBUG(std::string("Failed to serialize ACMPDU: ") + e.what());
		return Error::InternalError;
	}
}

/* *** Other methods **** */
std::uint32_t ProtocolInterfaceVirtualImpl::getVuAecpCommandTimeoutMsec(VuAecpdu::ProtocolIdentifier const& protocolIdentifier, VuAecpdu const& aecpdu) const noexcept
{
	return getVuAecpCommandTimeout(protocolIdentifier, aecpdu);
}

/* ************************************************************ */
/* stateMachine::AdvertiseStateMachine::Delegate overrides      */
/* ************************************************************ */

/* ************************************************************ */
/* stateMachine::DiscoveryStateMachine::Delegate overrides      */
/* ************************************************************ */
void ProtocolInterfaceVirtualImpl::onLocalEntityOnline(entity::Entity const& entity) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityOnline, this, entity);
}

void ProtocolInterfaceVirtualImpl::onLocalEntityOffline(UniqueIdentifier const entityID) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityOffline, this, entityID);
}

void ProtocolInterfaceVirtualImpl::onLocalEntityUpdated(entity::Entity const& entity) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityUpdated, this, entity);
}

void ProtocolInterfaceVirtualImpl::onRemoteEntityOnline(entity::Entity const& entity) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityOnline, this, entity);
}

void ProtocolInterfaceVirtualImpl::onRemoteEntityOffline(UniqueIdentifier const entityID) noexcept
{
	// Notify observers
	SEND_INSTRUMENTATION_NOTIFICATION("ProtocolInterfaceVirtual::onRemoteEntityOffline::PreNotify");
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityOffline, this, entityID);
	SEND_INSTRUMENTATION_NOTIFICATION("ProtocolInterfaceVirtual::onRemoteEntityOffline::PostNotify");
}

void ProtocolInterfaceVirtualImpl::onRemoteEntityUpdated(entity::Entity const& entity) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityUpdated, this, entity);
}

/* ************************************************************ */
/* stateMachine::CommandStateMachine::Delegate overrides        */
/* ************************************************************ */
void ProtocolInterfaceVirtualImpl::onAecpAemUnsolicitedResponse(AemAecpdu const& aecpdu) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpAemUnsolicitedResponse, this, aecpdu);
}

void ProtocolInterfaceVirtualImpl::onAecpAemIdentifyNotification(AemAecpdu const& aecpdu) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpAemIdentifyNotification, this, aecpdu);
}

void ProtocolInterfaceVirtualImpl::onAecpRetry(UniqueIdentifier const& entityID) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpRetry, this, entityID);
}

void ProtocolInterfaceVirtualImpl::onAecpTimeout(UniqueIdentifier const& entityID) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpTimeout, this, entityID);
}

void ProtocolInterfaceVirtualImpl::onAecpUnexpectedResponse(UniqueIdentifier const& entityID) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpUnexpectedResponse, this, entityID);
}

void ProtocolInterfaceVirtualImpl::onAecpResponseTime(UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpResponseTime, this, entityID, responseTime);
}

/* ************************************************************ */
/* MessageDispatcher::Observer overrides                        */
/* ************************************************************ */
void ProtocolInterfaceVirtualImpl::onMessage(SerializationBuffer const& message) noexcept
{
	auto const* const buffer = message.data();
	auto const length = message.size();

	// Packet received, process it
	auto des = DeserializationBuffer(buffer, length);
	EtherLayer2 etherLayer2;
	deserialize<EtherLayer2>(&etherLayer2, des);

	// Only accept message for my MacAddress or the broadcast address
	auto const& destAddress = etherLayer2.getDestAddress();
	if (destAddress == getMacAddress() || destAddress == Multicast_Mac_Address || destAddress == Identify_Mac_Address)
	{
		// Check ether type (shouldn't be needed, pcap filter is active)
		std::uint16_t etherType = AVDECC_UNPACK_TYPE(*((std::uint16_t*)(buffer + 12)), std::uint16_t);
		if (etherType == AvtpEtherType)
		{
			std::uint8_t const* avtpdu = &buffer[14]; // Start of AVB Transport Protocol
			auto avtpdu_size = length - 14;
			// Check AVTP control bit (meaning AVDECC packet)
			std::uint8_t avtp_sub_type_control = avtpdu[0];
			if ((avtp_sub_type_control & 0xF0) != 0)
			{
				dispatchAvdeccMessage(avtpdu, avtpdu_size, etherLayer2);
			}
		}
	}
}
void ProtocolInterfaceVirtualImpl::onTransportError() noexcept
{
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onTransportError, this);
}

/* ************************************************************ */
/* Private methods                                              */
/* ************************************************************ */
ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendPacket(SerializationBuffer const& buffer) const noexcept
{
	auto length = buffer.size();
	constexpr auto minimumSize = EthernetPayloadMinimumSize + EtherLayer2::HeaderLength;

	/* Check the buffer has enough bytes in it */
	if (length < minimumSize)
		length = minimumSize; // No need to resize nor pad the buffer, it has enough capacity and we don't care about the unused bytes. Simply increase the length of the data to send.

	try
	{
		// Push the buffer to the message dispatcher
		auto& dispatcher = MessageDispatcher::getInstance();
		dispatcher.push(_networkInterfaceName, buffer);
		return Error::NoError;
	}
	catch (...)
	{
	}
	return Error::TransportError;
}

void ProtocolInterfaceVirtualImpl::deserializeAecpMessage(EtherLayer2 const& etherLayer2, Deserializer& des, Aecpdu& aecp) const
{
	// Fill EtherLayer2
	aecp.setSrcAddress(etherLayer2.getSrcAddress());
	aecp.setDestAddress(etherLayer2.getDestAddress());
	// Then deserialize Avtp control
	deserialize<AvtpduControl>(&aecp, des);
	// Then deserialize Aecp
	deserialize<Aecpdu>(&aecp, des);
}

void ProtocolInterfaceVirtualImpl::dispatchAvdeccMessage(std::uint8_t const* const pkt_data, size_t const pkt_len, EtherLayer2 const& etherLayer2) noexcept
{
	try
	{
		// Read Avtpdu SubType and ControlData (which is remapped to MessageType for all 1722.1 messages)
		std::uint8_t const subType = pkt_data[0] & 0x7f;
		std::uint8_t const controlData = pkt_data[1] & 0x7f;

		// Create a deserialization buffer
		auto des = DeserializationBuffer(pkt_data, pkt_len);

		switch (subType)
		{
			/* ADP Message */
			case AvtpSubType_Adp:
			{
				auto adpdu = Adpdu::create();
				auto& adp = static_cast<Adpdu&>(*adpdu);

				// Fill EtherLayer2
				adp.setSrcAddress(etherLayer2.getSrcAddress());
				adp.setDestAddress(etherLayer2.getDestAddress());
				// Then deserialize Avtp control
				deserialize<AvtpduControl>(&adp, des);
				// Then deserialize Adp
				deserialize<Adpdu>(&adp, des);

				// Low level notification
				notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAdpduReceived, this, adp);

				// Forward to our state machine
				_stateMachineManager.processAdpdu(adp);
				break;
			}

			/* AECP Message */
			case AvtpSubType_Aecp:
			{
				auto const messageType = static_cast<AecpMessageType>(controlData);

				static std::unordered_map<AecpMessageType, std::function<Aecpdu::UniquePointer(ProtocolInterfaceVirtualImpl* const pi, EtherLayer2 const& etherLayer2, Deserializer& des, std::uint8_t const* const pkt_data, size_t const pkt_len)>, AecpMessageType::Hash> s_Dispatch{
					{ AecpMessageType::AemCommand,
						[](ProtocolInterfaceVirtualImpl* const /*pi*/, EtherLayer2 const& /*etherLayer2*/, Deserializer& /*des*/, std::uint8_t const* const /*pkt_data*/, size_t const /*pkt_len*/)
						{
							return AemAecpdu::create(false);
						} },
					{ AecpMessageType::AemResponse,
						[](ProtocolInterfaceVirtualImpl* const /*pi*/, EtherLayer2 const& /*etherLayer2*/, Deserializer& /*des*/, std::uint8_t const* const /*pkt_data*/, size_t const /*pkt_len*/)
						{
							return AemAecpdu::create(true);
						} },
					{ AecpMessageType::AddressAccessCommand,
						[](ProtocolInterfaceVirtualImpl* const /*pi*/, EtherLayer2 const& /*etherLayer2*/, Deserializer& /*des*/, std::uint8_t const* const /*pkt_data*/, size_t const /*pkt_len*/)
						{
							return AaAecpdu::create(false);
						} },
					{ AecpMessageType::AddressAccessResponse,
						[](ProtocolInterfaceVirtualImpl* const /*pi*/, EtherLayer2 const& /*etherLayer2*/, Deserializer& /*des*/, std::uint8_t const* const /*pkt_data*/, size_t const /*pkt_len*/)
						{
							return AaAecpdu::create(true);
						} },
					{ AecpMessageType::VendorUniqueCommand,
						[](ProtocolInterfaceVirtualImpl* const pi, EtherLayer2 const& etherLayer2, Deserializer& des, std::uint8_t const* const pkt_data, size_t const pkt_len)
						{
							// We have to retrieve the ProtocolID to dispatch
							auto const protocolIdentifierOffset = AvtpduControl::HeaderLength + Aecpdu::HeaderLength;
							if (pkt_len >= (protocolIdentifierOffset + VuAecpdu::ProtocolIdentifier::Size))
							{
								auto protocolIdentifier = VuAecpdu::ProtocolIdentifier::ArrayType{};
								std::memcpy(protocolIdentifier.data(), pkt_data + protocolIdentifierOffset, VuAecpdu::ProtocolIdentifier::Size);

								auto const vuProtocolID = VuAecpdu::ProtocolIdentifier{ protocolIdentifier };
								auto* vuDelegate = pi->getVendorUniqueDelegate(vuProtocolID);
								if (vuDelegate)
								{
									// VendorUnique Commands are always handled by the VendorUniqueDelegate
									auto aecpdu = vuDelegate->createAecpdu(vuProtocolID, false);
									auto& vuAecp = static_cast<VuAecpdu&>(*aecpdu);

									// Deserialize the aecp message
									pi->deserializeAecpMessage(etherLayer2, des, vuAecp);

									// Low level notification
									pi->notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpduReceived, pi, vuAecp);

									// Forward to the delegate
									vuDelegate->onVuAecpCommand(pi, vuProtocolID, vuAecp);

									// Return empty Aecpdu so that it's not processed by the StateMachineManager
								}
								else
								{
									LOG_PROTOCOL_INTERFACE_DEBUG(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "Unhandled VendorUnique Command for ProtocolIdentifier {}", utils::toHexString(static_cast<VuAecpdu::ProtocolIdentifier::IntegralType>(vuProtocolID), true));
								}
							}
							else
							{
								LOG_PROTOCOL_INTERFACE_WARN(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "Invalid VendorUnique Command received. Not enough bytes in the message to hold ProtocolIdentifier");
							}

							return Aecpdu::UniquePointer{ nullptr, nullptr };
						} },
					{ AecpMessageType::VendorUniqueResponse,
						[](ProtocolInterfaceVirtualImpl* const pi, EtherLayer2 const& etherLayer2, Deserializer& des, std::uint8_t const* const pkt_data, size_t const pkt_len)
						{
							// We have to retrieve the ProtocolID to dispatch
							auto const protocolIdentifierOffset = AvtpduControl::HeaderLength + Aecpdu::HeaderLength;
							if (pkt_len >= (protocolIdentifierOffset + VuAecpdu::ProtocolIdentifier::Size))
							{
								auto protocolIdentifier = VuAecpdu::ProtocolIdentifier::ArrayType{};
								std::memcpy(protocolIdentifier.data(), pkt_data + protocolIdentifierOffset, VuAecpdu::ProtocolIdentifier::Size);

								auto const vuProtocolID = VuAecpdu::ProtocolIdentifier{ protocolIdentifier };
								auto* vuDelegate = pi->getVendorUniqueDelegate(vuProtocolID);
								if (vuDelegate)
								{
									auto aecpdu = vuDelegate->createAecpdu(vuProtocolID, true);

									// Are the messages handled by the VendorUniqueDelegate itself
									if (!vuDelegate->areHandledByControllerStateMachine(vuProtocolID))
									{
										auto& vuAecp = static_cast<VuAecpdu&>(*aecpdu);

										// Deserialize the aecp message
										pi->deserializeAecpMessage(etherLayer2, des, vuAecp);

										// Low level notification
										pi->notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpduReceived, pi, vuAecp);

										// Forward to the delegate
										vuDelegate->onVuAecpResponse(pi, vuProtocolID, vuAecp);

										// Return empty Aecpdu so that it's not processed by the StateMachineManager
										aecpdu.reset(nullptr);
									}

									return aecpdu;
								}
								else
								{
									LOG_PROTOCOL_INTERFACE_DEBUG(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "Unhandled VendorUnique Response for ProtocolIdentifier {}", utils::toHexString(static_cast<VuAecpdu::ProtocolIdentifier::IntegralType>(vuProtocolID), true));
								}
							}
							else
							{
								LOG_PROTOCOL_INTERFACE_WARN(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "Invalid VendorUnique Command received. Not enough bytes in the message to hold ProtocolIdentifier");
							}

							return Aecpdu::UniquePointer{ nullptr, nullptr };
						} },
				};

				auto const& it = s_Dispatch.find(messageType);
				if (it == s_Dispatch.end())
					return; // Unsupported AECP message type

				// Create aecpdu frame based on message type
				auto aecpdu = it->second(this, etherLayer2, des, pkt_data, pkt_len);

				if (aecpdu != nullptr)
				{
					auto& aecp = static_cast<Aecpdu&>(*aecpdu);

					// Deserialize the aecp message
					deserializeAecpMessage(etherLayer2, des, aecp);

					// Low level notification
					notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpduReceived, this, aecp);

					// Forward to our state machine
					_stateMachineManager.processAecpdu(aecp);
				}
				break;
			}

			/* ACMP Message */
			case AvtpSubType_Acmp:
			{
				auto acmpdu = Acmpdu::create();
				auto& acmp = static_cast<Acmpdu&>(*acmpdu);

				// Fill EtherLayer2
				acmp.setSrcAddress(etherLayer2.getSrcAddress());
				static_cast<EtherLayer2&>(*acmpdu).setDestAddress(etherLayer2.getDestAddress()); // Fill dest address, even if we know it's always the MultiCast address
				// Then deserialize Avtp control
				deserialize<AvtpduControl>(&acmp, des);
				// Then deserialize Acmp
				deserialize<Acmpdu>(&acmp, des);

				// Low level notification
				notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAcmpduReceived, this, acmp);

				// Forward to our state machine
				_stateMachineManager.processAcmpdu(acmp);
				break;
			}

			/* MAAP Message */
			case AvtpSubType_Maap:
			{
				break;
			}
			default:
				return;
		}
	}
	catch ([[maybe_unused]] std::invalid_argument const& e)
	{
		LOG_GENERIC_WARN(std::string("ProtocolInterfaceVirtual: Packet dropped: ") + e.what());
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Unknown exception");
		LOG_GENERIC_WARN("ProtocolInterfaceVirtual: Packet dropped due to unknown exception");
	}
}

ProtocolInterfaceVirtual::ProtocolInterfaceVirtual(std::string const& networkInterfaceName, networkInterface::MacAddress const& macAddress)
	: ProtocolInterface(networkInterfaceName, macAddress)
{
}

bool ProtocolInterfaceVirtual::isSupported() noexcept
{
	return true;
}

ProtocolInterfaceVirtual* ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual(std::string const& networkInterfaceName, networkInterface::MacAddress const& macAddress)
{
	return new ProtocolInterfaceVirtualImpl(networkInterfaceName, macAddress);
}

} // namespace protocol
} // namespace avdecc
} // namespace la
