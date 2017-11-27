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
* @file protocolInterface_virtual.cpp
* @author Christophe Calmejane
*/

#include "protocolInterface_virtual.hpp"
#include "serialization.hpp"
#include "protocol/protocolAemAecpdu.hpp"
#include "stateMachine/controllerStateMachine.hpp"
#include <stdexcept>
#include <thread>
#include <condition_variable>
#include <list>
#include <mutex>
#include <memory>
#include <functional>
#include <atomic>
#include "la/avdecc/logger.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{

class MessageDispatcher final
{
	using Subject = la::avdecc::TypedSubject<struct SubjectTag, std::mutex>;
	struct Interface
	{
		std::atomic_bool shouldTerminate{ false };
		std::mutex mutex;
		std::thread dispatchThread{};
		Subject observers{};
		std::list<SerializationBuffer> messages{};
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
	class Observer : public la::avdecc::Observer<Subject>
	{
	public:
		virtual void onMessage(SerializationBuffer const& message) noexcept = 0;
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
			auto interface = std::make_unique<Interface>();
			interface->dispatchThread = std::thread([this, networkInterfaceName, interface = interface.get()]()
			{
				la::avdecc::setCurrentThreadName("avdecc::VirtualInterface." + networkInterfaceName);
				while (!interface->shouldTerminate)
				{
					std::unique_lock<decltype(interface->mutex)> lock(interface->mutex);

					// Wait for message in the queue
					interface->cond.wait(lock, [this, interface]
					{
						return interface->messages.size() > 0 || interface->shouldTerminate;
					});

					// Empty the queue
					while (!interface->shouldTerminate && interface->messages.size() > 0)
					{
						// Pop a message from the queue
						auto message = std::move(interface->messages.front());
						interface->messages.pop_front();

						// Notify registered observers
						interface->observers.notifyObservers<Observer>([&message](auto* obs)
						{
							obs->onMessage(message);
						});
					}
				}
			});
			auto result = _interfaces.emplace(std::make_pair(networkInterfaceName, std::move(interface)));
			// Insertion failed
			if (!result.second)
				return;
			interfaceIt = result.first;
		}

		// Register observer
		auto& interface = *interfaceIt->second;
		try
		{
			interface.observers.registerObserver(observer);
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
		auto& interface = *interfaceIt->second;
		try
		{
			interface.observers.unregisterObserver(observer);
		}
		catch (std::invalid_argument const&)
		{
		}

		// If last observer for this interface, remove the interface
		if (interface.observers.countObservers() == 0)
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
		auto& interface = *interfaceIt->second;
		std::unique_lock<decltype(interface.mutex)> lock(interface.mutex);
		interface.messages.push_back(std::move(message));

		// Notify the dispatch thread
		interface.cond.notify_all();
	}

	// Deleted compiler auto-generated methods
	MessageDispatcher(MessageDispatcher&&) = delete;
	MessageDispatcher(MessageDispatcher const&) = delete;
	MessageDispatcher& operator=(MessageDispatcher const&) = delete;
	MessageDispatcher& operator=(MessageDispatcher&&) = delete;

private:
	MessageDispatcher() = default;

	~MessageDispatcher() noexcept
	{
	}

	// Private variables
	std::mutex _mutex;
	std::unordered_map<std::string, std::unique_ptr<Interface>> _interfaces{};
};


static la::avdecc::networkInterface::MacAddress Multicast_Mac_Address{ { 0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00 } };
static la::avdecc::networkInterface::MacAddress Identify_Mac_Address{ { 0x91, 0xe0, 0xf0, 0x01, 0x00, 0x01 } };

class ProtocolInterfaceVirtualImpl final : public ProtocolInterfaceVirtual, private stateMachine::ControllerStateMachine::Delegate, private MessageDispatcher::Observer
{
public:
	/** Constructor */
	ProtocolInterfaceVirtualImpl(std::string const& networkInterfaceName, networkInterface::MacAddress const& macAddress);

	/** Destructor */
	virtual ~ProtocolInterfaceVirtualImpl() noexcept;

	void dispatchAvdeccMessage(std::uint8_t const* const pkt_data, size_t const pkt_len, EtherLayer2 const& etherLayer2) noexcept;

	// Deleted compiler auto-generated methods
	ProtocolInterfaceVirtualImpl(ProtocolInterfaceVirtualImpl&&) = delete;
	ProtocolInterfaceVirtualImpl(ProtocolInterfaceVirtualImpl const&) = delete;
	ProtocolInterfaceVirtualImpl& operator=(ProtocolInterfaceVirtualImpl const&) = delete;
	ProtocolInterfaceVirtualImpl& operator=(ProtocolInterfaceVirtualImpl&&) = delete;

private:
	Error sendPacket(SerializationBuffer const& buffer) const noexcept;

	// ProtocolInterface overrides
	virtual void shutdown() noexcept override;
	virtual Error registerLocalEntity(entity::LocalEntity& entity) noexcept override;
	virtual Error unregisterLocalEntity(entity::LocalEntity& entity) noexcept override;
	virtual Error enableEntityAdvertising(entity::LocalEntity const& entity) noexcept override;
	virtual Error disableEntityAdvertising(entity::LocalEntity& entity) noexcept override;
	virtual Error discoverRemoteEntities() const noexcept override;
	virtual Error discoverRemoteEntity(UniqueIdentifier const entityID) const noexcept override;
	virtual Error sendAdpMessage(Adpdu::UniquePointer&& adpdu) const noexcept override;
	virtual Error sendAecpMessage(Aecpdu::UniquePointer&& aecpdu) const noexcept override;
	virtual Error sendAcmpMessage(Acmpdu::UniquePointer&& acmpdu) const noexcept override;
	virtual Error sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, networkInterface::MacAddress const& macAddress, AecpCommandResultHandler const& onResult) const noexcept override;
	virtual Error sendAecpResponse(Aecpdu::UniquePointer&& aecpdu, networkInterface::MacAddress const& macAddress) const noexcept override;
	virtual Error sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, AcmpCommandResultHandler const& onResult) const noexcept override;
	virtual Error sendAcmpResponse(Acmpdu::UniquePointer&& acmpdu) const noexcept override;

	// stateMachine::ControllerStateMachine::Delegate overrides
	virtual void onLocalEntityOnline(la::avdecc::entity::DiscoveredEntity const& entity) noexcept override;
	virtual void onLocalEntityOffline(la::avdecc::UniqueIdentifier const entityID) noexcept override;
	virtual void onLocalEntityUpdated(la::avdecc::entity::DiscoveredEntity const& entity) noexcept override;
	virtual void onRemoteEntityOnline(la::avdecc::entity::DiscoveredEntity const& entity) noexcept override;
	virtual void onRemoteEntityOffline(la::avdecc::UniqueIdentifier const entityID) noexcept override;
	virtual void onRemoteEntityUpdated(la::avdecc::entity::DiscoveredEntity const& entity) noexcept override;
	virtual void onAecpCommand(la::avdecc::entity::LocalEntity const& entity, la::avdecc::protocol::Aecpdu const& aecpdu) noexcept override;
	virtual void onAecpUnsolicitedResponse(la::avdecc::entity::LocalEntity const& entity, la::avdecc::protocol::Aecpdu const& aecpdu) noexcept override;
	virtual void onAcmpSniffedCommand(la::avdecc::entity::LocalEntity const& entity, la::avdecc::protocol::Acmpdu const& acmpdu) noexcept override;
	virtual void onAcmpSniffedResponse(la::avdecc::entity::LocalEntity const& entity, la::avdecc::protocol::Acmpdu const& acmpdu) noexcept override;
	virtual ProtocolInterface::Error sendMessage(la::avdecc::protocol::Adpdu const& adpdu) const noexcept override;
	virtual ProtocolInterface::Error sendMessage(la::avdecc::protocol::Aecpdu const& aecpdu) const noexcept override;
	virtual ProtocolInterface::Error sendMessage(la::avdecc::protocol::Acmpdu const& acmpdu) const noexcept override;

	// MessageDispatcher::Observer overrides
	virtual void onMessage(SerializationBuffer const& message) noexcept override;

	// Private variables
	mutable stateMachine::ControllerStateMachine _controllerStateMachine{ this, this };
};

/** Constructor */
ProtocolInterfaceVirtualImpl::ProtocolInterfaceVirtualImpl(std::string const& networkInterfaceName, networkInterface::MacAddress const& macAddress)
	: ProtocolInterfaceVirtual(networkInterfaceName, macAddress)
{
	// Should always be supported. Cannot create a Virtual ProtocolInterface if it's not supported.
	assert(isSupported());

	// Register to the message dispatcher
	auto& dispatcher = MessageDispatcher::getInstance();
	dispatcher.registerObserver(networkInterfaceName, this);
}

/** Destructor */
ProtocolInterfaceVirtualImpl::~ProtocolInterfaceVirtualImpl() noexcept
{
	shutdown();
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

				// Forward to our state machine
				_controllerStateMachine.processAdpdu(adp);
				break;
			}

			/* AECP Message */
			case AvtpSubType_Aecp:
			{
				Aecpdu::UniquePointer aecpdu{ nullptr, nullptr };
				auto const messageType = static_cast<AecpMessageType>(controlData);

#pragma message("TBD: Handle other AECP message types")
				static std::unordered_map<AecpMessageType, std::function<Aecpdu::UniquePointer()>, AecpMessageType::Hash> s_Dispatch{
					{
						AecpMessageType::AemCommand, []()
						{
							return AemAecpdu::create();
						}
					},
					{
						AecpMessageType::AemResponse, []()
						{
							return AemAecpdu::create();
						}
					},
				};

				auto const& it = s_Dispatch.find(messageType);
				if (it == s_Dispatch.end())
					return; // Unsupported AECP message type

				// Create aecpdu frame based on message type
				aecpdu = it->second();

				if (aecpdu != nullptr)
				{
					auto& aecp = static_cast<Aecpdu&>(*aecpdu);

					// Fill EtherLayer2
					aecp.setSrcAddress(etherLayer2.getSrcAddress());
					aecp.setDestAddress(etherLayer2.getDestAddress());
					// Then deserialize Avtp control
					deserialize<AvtpduControl>(&aecp, des);
					// Then deserialize Aecp
					deserialize<Aecpdu>(&aecp, des);

					// Forward to our state machine
#pragma message("TBD: Should not forward to *controllerStateMachine* specifically, but to all possible state machines for the local EndStation")
					_controllerStateMachine.processAecpdu(aecp);
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

				// Forward to our state machine
#pragma message("TBD: Should not forward to *controllerStateMachine* specifically, but to all possible state machines for the local EndStation")
				_controllerStateMachine.processAcmpdu(acmp);
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
	catch (std::invalid_argument const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Warn, std::string("ProtocolInterfaceVirtual: Packet dropped: ") + e.what());
	}
	catch (...)
	{
		assert(false && "Unknown exception");
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Warn, "ProtocolInterfaceVirtual: Packet dropped due to unknown exception");
	}
}

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

// ProtocolInterface overrides
void ProtocolInterfaceVirtualImpl::shutdown() noexcept
{
	// Unregister from the message dispatcher
	auto& dispatcher = MessageDispatcher::getInstance();
	dispatcher.unregisterObserver(_networkInterfaceName, this);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::registerLocalEntity(entity::LocalEntity& entity) noexcept
{
	auto error{ ProtocolInterface::Error::NoError };

	// Entity is controller capable
	if (la::avdecc::hasFlag(entity.getControllerCapabilities(), entity::ControllerCapabilities::Implemented))
		error |= _controllerStateMachine.registerLocalEntity(entity);

#pragma message("TBD: Handle talker/listener types")
	// Entity is listener capable
	if (la::avdecc::hasFlag(entity.getListenerCapabilities(), entity::ListenerCapabilities::Implemented))
		return ProtocolInterface::Error::InvalidEntityType; // Not supported right now

	// Entity is talker capable
	if (la::avdecc::hasFlag(entity.getTalkerCapabilities(), entity::TalkerCapabilities::Implemented))
		return ProtocolInterface::Error::InvalidEntityType; // Not supported right now

	return error;
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::unregisterLocalEntity(entity::LocalEntity& entity) noexcept
{
	// Remove from all state machines, without checking the type (will be done by the StateMachine)
	_controllerStateMachine.unregisterLocalEntity(entity);
#pragma message("TBD: Remove from talker/listener state machines too")
	return ProtocolInterface::Error::NoError;
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::enableEntityAdvertising(entity::LocalEntity const& entity) noexcept
{
	return _controllerStateMachine.enableEntityAdvertising(entity);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::disableEntityAdvertising(entity::LocalEntity& entity) noexcept
{
	return _controllerStateMachine.disableEntityAdvertising(entity);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::discoverRemoteEntities() const noexcept
{
	return _controllerStateMachine.discoverRemoteEntities();
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::discoverRemoteEntity(UniqueIdentifier const entityID) const noexcept
{
	return _controllerStateMachine.discoverRemoteEntity(entityID);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendAdpMessage(Adpdu::UniquePointer&& adpdu) const noexcept
{
	// Directly send the message on the network
	return sendMessage(static_cast<Adpdu const&>(*adpdu));
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendAecpMessage(Aecpdu::UniquePointer&& aecpdu) const noexcept
{
	// Directly send the message on the network
	return sendMessage(static_cast<Aecpdu const&>(*aecpdu));
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendAcmpMessage(Acmpdu::UniquePointer&& acmpdu) const noexcept
{
	// Directly send the message on the network
	return sendMessage(static_cast<Acmpdu const&>(*acmpdu));
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, networkInterface::MacAddress const& /*macAddress*/, AecpCommandResultHandler const& onResult) const noexcept
{
	// Virtual protocol interface do not need the macAddress parameter, it will be retrieved from the Aecpdu when sending it
	// Command goes through the state machine to handle timeout, retry and response
	return _controllerStateMachine.sendAecpCommand(std::move(aecpdu), onResult);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendAecpResponse(Aecpdu::UniquePointer&& aecpdu, networkInterface::MacAddress const& /*macAddress*/) const noexcept
{
	// Virtual protocol interface do not need the macAddress parameter, it will be retrieved from the Aecpdu when sending it
	// Response can be directly sent
	return sendMessage(static_cast<Aecpdu const&>(*aecpdu));
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, AcmpCommandResultHandler const& onResult) const noexcept
{
	// Command goes through the state machine to handle timeout, retry and response
	return _controllerStateMachine.sendAcmpCommand(std::move(acmpdu), onResult);
}

ProtocolInterface::Error ProtocolInterfaceVirtualImpl::sendAcmpResponse(Acmpdu::UniquePointer&& acmpdu) const noexcept
{
	// Response can be directly sent
	return sendMessage(static_cast<Acmpdu const&>(*acmpdu));
}

// stateMachine::ControllerStateMachine::Delegate overrides
void ProtocolInterfaceVirtualImpl::onLocalEntityOnline(la::avdecc::entity::DiscoveredEntity const& entity) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityOnline, entity);
}

void ProtocolInterfaceVirtualImpl::onLocalEntityOffline(UniqueIdentifier const entityID) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityOffline, entityID);
}

void ProtocolInterfaceVirtualImpl::onLocalEntityUpdated(la::avdecc::entity::DiscoveredEntity const& entity) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityUpdated, entity);
}

void ProtocolInterfaceVirtualImpl::onRemoteEntityOnline(la::avdecc::entity::DiscoveredEntity const& entity) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityOnline, entity);
}

void ProtocolInterfaceVirtualImpl::onRemoteEntityOffline(UniqueIdentifier const entityID) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityOffline, entityID);
}

void ProtocolInterfaceVirtualImpl::onRemoteEntityUpdated(la::avdecc::entity::DiscoveredEntity const& entity) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityUpdated, entity);
}

void ProtocolInterfaceVirtualImpl::onAecpCommand(entity::LocalEntity const& entity, Aecpdu const& aecpdu) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpCommand, entity, aecpdu);
}

void ProtocolInterfaceVirtualImpl::onAecpUnsolicitedResponse(entity::LocalEntity const& entity, Aecpdu const& aecpdu) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpUnsolicitedResponse, entity, aecpdu);
}

void ProtocolInterfaceVirtualImpl::onAcmpSniffedCommand(entity::LocalEntity const& entity, Acmpdu const& acmpdu) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAcmpSniffedCommand, entity, acmpdu);
}

void ProtocolInterfaceVirtualImpl::onAcmpSniffedResponse(entity::LocalEntity const& entity, Acmpdu const& acmpdu) noexcept
{
	// Notify observers
	notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAcmpSniffedResponse, entity, acmpdu);
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize ADPDU: ") + e.what());
		return ProtocolInterface::Error::InternalError;
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize AECPDU: ") + e.what());
		return ProtocolInterface::Error::InternalError;
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize ACMPDU: ") + e.what());
		return ProtocolInterface::Error::InternalError;
	}
}

// MessageDispatcher::Observer overrides
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

ProtocolInterfaceVirtual::ProtocolInterfaceVirtual(std::string const& networkInterfaceName, networkInterface::MacAddress const& macAddress)
	: ProtocolInterface(networkInterfaceName, macAddress)
{
}

ProtocolInterface::UniquePointer ProtocolInterfaceVirtual::create(std::string const& networkInterfaceName, networkInterface::MacAddress const& macAddress)
{
	return std::make_unique<ProtocolInterfaceVirtualImpl>(networkInterfaceName, macAddress);
}

bool ProtocolInterfaceVirtual::isSupported() noexcept
{
	return true;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
