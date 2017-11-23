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
* @file entityImpl.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/entity.hpp"
#include "la/avdecc/internals/entityModel.hpp"
#include "protocolInterface/protocolInterface.hpp"
#include <cstdint>
#include <thread>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <mutex>

namespace la
{
namespace avdecc
{
namespace entity
{

template<class SuperClass = LocalEntity>
class LocalEntityImpl : public SuperClass
{
public:
	LocalEntityImpl(protocol::ProtocolInterface* const protocolInterface, std::uint16_t const progID,
									model::VendorEntityModel const vendorEntityModelID, EntityCapabilities const entityCapabilities,
									std::uint16_t const talkerStreamSources, TalkerCapabilities const talkerCapabilities,
									std::uint16_t const listenerStreamSinks, ListenerCapabilities const listenerCapabilities,
									ControllerCapabilities const controllerCapabilities,
									std::uint16_t const identifyControlIndex, std::uint16_t const interfaceIndex, UniqueIdentifier const associationID)
		: SuperClass(generateEID(protocolInterface, progID), protocolInterface->getMacAddress(), vendorEntityModelID, entityCapabilities,
								 talkerStreamSources, talkerCapabilities,
								 listenerStreamSinks, listenerCapabilities,
								 controllerCapabilities,
								 identifyControlIndex, interfaceIndex, associationID)
		, _protocolInterface(protocolInterface)
	{
		if (!!_protocolInterface->registerLocalEntity(*this))
		{
			throw Exception("Failed to register local entity");
		}
	}

	virtual ~LocalEntityImpl() noexcept override
	{
	}

	/* ************************************************************************** */
	/* LocalEntity methods                                                        */
	/* ************************************************************************** */
	virtual bool enableEntityAdvertising(std::uint32_t const availableDuration) noexcept override
	{
		std::uint8_t validTime = static_cast<decltype(validTime)>(availableDuration / 2);
		if (validTime < 1)
			validTime = 1;
		else if (validTime > 31)
			validTime = 31;
		setValidTime(validTime);
		return !_protocolInterface->enableEntityAdvertising(*this);
	}

	virtual void disableEntityAdvertising() noexcept override
	{
		_protocolInterface->disableEntityAdvertising(*this);
	}

	virtual bool isDirty() noexcept override
	{
		std::lock_guard<decltype(_lock)> const lg(_lock);
		auto const dirty = _dirty;
		_dirty = false;
		return dirty;
	}

	/** Sets the valid time value and flag for announcement */
	virtual void setValidTime(std::uint8_t const validTime) noexcept override
	{
		std::lock_guard<decltype(_lock)> const lg(_lock);
		SuperClass::setValidTime(validTime);
		_dirty = true;
	}

	/** Sets the entity capabilities and flag for announcement */
	virtual void setEntityCapabilities(EntityCapabilities const entityCapabilities) noexcept override
	{
		std::lock_guard<decltype(_lock)> const lg(_lock);
		SuperClass::setEntityCapabilities(entityCapabilities);
		_dirty = true;
	}

	/** Sets the gptp grandmaster unique identifier and flag for announcement */
	virtual void setGptpGrandmasterID(UniqueIdentifier const gptpGrandmasterID) noexcept override
	{
		std::lock_guard<decltype(_lock)> const lg(_lock);
		SuperClass::setGptpGrandmasterID(gptpGrandmasterID);
		_dirty = true;
	}

	/** Sets th gptp domain number and flag for announcement */
	virtual void setGptpDomainNumber(std::uint8_t const gptpDomainNumber) noexcept override
	{
		std::lock_guard<decltype(_lock)> const lg(_lock);
		SuperClass::setGptpDomainNumber(gptpDomainNumber);
		_dirty = true;
	}

	protocol::ProtocolInterface* getProtocolInterface() noexcept
	{
		return _protocolInterface;
	}

	protocol::ProtocolInterface const* getProtocolInterface() const noexcept
	{
		return _protocolInterface;
	}

	// BasicLockable concept lock method
	void lock() noexcept override
	{
		_lock.lock();
	}

	// BasicLockable concept unlock method
	void unlock() noexcept override
	{
		_lock.unlock();
	}

protected:
	/** Shutdown method that has to be called by any class inheriting from this one */
	void shutdown() noexcept
	{
		// Disable advertising
		disableEntityAdvertising();

		// Unregister local entity
		_protocolInterface->unregisterLocalEntity(*this); // Ignore errors
	}

private:
	UniqueIdentifier generateEID(protocol::ProtocolInterface* const protocolInterface, std::uint16_t const progID) const
	{
		// Generate eid
		UniqueIdentifier eid{ 0 };
		auto macAddress = protocolInterface->getMacAddress();
		if (macAddress.size() != 6)
			throw Exception("Invalid MAC address size");
		if (progID == 0 || progID == 0xFFFF || progID == 0xFFFE)
			throw Exception("Reserved value for Entity's progID value: " + std::to_string(progID));
		eid += macAddress[0];
		eid <<= 8; eid += macAddress[1];
		eid <<= 8; eid += macAddress[2];
		eid <<= 16; eid += progID;
		eid <<= 8; eid += macAddress[3];
		eid <<= 8; eid += macAddress[4];
		eid <<= 8; eid += macAddress[5];

		return eid;
	}

	// Internal variables
	std::recursive_mutex _lock{}; // Lock to protect writable fields and dirty state
	protocol::ProtocolInterface* const _protocolInterface{ nullptr }; // Weak reference to the protocolInterface
	bool _dirty{ false }; // Is the entity dirty and should send an ENTITY_AVAILABLE message
};

/** Class to be used as final LocalEntityImpl inherited class in order to properly shutdown any inflight messages. */
template<class SuperClass>
class LocalEntityGuard final : public SuperClass
{
public:
	LocalEntityGuard(protocol::ProtocolInterface* const protocolInterface, std::uint16_t const progID, entity::model::VendorEntityModel const vendorEntityModelID, ControllerEntity::Delegate* const delegate)
		: SuperClass(protocolInterface, progID, vendorEntityModelID, delegate)
	{
	}
	~LocalEntityGuard() noexcept
	{
		// Shutdown method shall be called first by any class inheriting from LocalEntityImpl
		shutdown();
	}
};

} // namespace entity
} // namespace avdecc
} // namespace la
