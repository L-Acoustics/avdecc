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
* @file discovery.cpp
* @author Christophe Calmejane
*/

/** ************************************************************************ **/
/** AVDECC_CONTROLLER DELEGATE EXAMPLE                                       **/
/** ************************************************************************ **/

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <unordered_map>
#include <stdexcept>
#include <cassert>
#include <la/avdecc/controller/avdeccController.hpp>
#include <la/avdecc/networkInterfaceHelper.hpp>
#include <la/avdecc/utils.hpp>
#include <la/avdecc/logger.hpp>
#include "utils.hpp"

/* ************************************************************************** */
/* Discovery class                                                            */
/* ************************************************************************** */

class Discovery : public la::avdecc::controller::Controller::Observer, public la::avdecc::Logger::Observer
{
public:
	/** Constructor/destructor/destroy */
	Discovery(la::avdecc::EndStation::ProtocolInterfaceType const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, la::avdecc::entity::model::VendorEntityModel const vendorEntityModelID, std::string const& preferedLocale);
	~Discovery() = default;

	// Deleted compiler auto-generated methods
	Discovery(Discovery&&) = delete;
	Discovery(Discovery const&) = delete;
	Discovery& operator=(Discovery const&) = delete;
	Discovery& operator=(Discovery&&) = delete;

private:
	// la::avdecc::Logger::Observer overrides
	virtual void onLog(la::avdecc::Logger::Layer const layer, la::avdecc::Logger::Level const level, std::string const& message) noexcept override
	{
		outputText("[" + la::avdecc::Logger::getInstance().layerToString(layer) + "," + la::avdecc::Logger::getInstance().levelToString(level) + "] " + message + "\n");
	}
	// la::avdecc::controller::Controller::Observer overrides
	// Global notifications
	virtual void onTransportError(la::avdecc::controller::Controller const* const controller) noexcept override;
	virtual void onEntityQueryError(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::Controller::QueryCommandError const error) noexcept override;
	// Discovery notifications (ADP)
	virtual void onEntityOnline(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity) noexcept override;
	virtual void onEntityOffline(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity) noexcept override;

private:
	la::avdecc::controller::Controller::UniquePointer _controller{ nullptr, nullptr }; // Read/Write from the UI thread (and read only from la::avdecc::controller::Controller::Observer callbacks)
};

Discovery::Discovery(la::avdecc::EndStation::ProtocolInterfaceType const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, la::avdecc::entity::model::VendorEntityModel const vendorEntityModelID, std::string const& preferedLocale)
	: _controller(la::avdecc::controller::Controller::create(protocolInterfaceType, interfaceName, progID, vendorEntityModelID, preferedLocale))
{
	// Register observers
	la::avdecc::Logger::getInstance().registerObserver(this);
	_controller->registerObserver(this);
	// Start controller advertising
	_controller->enableEntityAdvertising(10);
	// Set default log level
	la::avdecc::Logger::getInstance().setLevel(la::avdecc::Logger::Level::Trace);
}

void Discovery::onTransportError(la::avdecc::controller::Controller const* const /*controller*/) noexcept
{
	outputText("Fatal error on transport layer\n");
}

void Discovery::onEntityQueryError(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* entity, la::avdecc::controller::Controller::QueryCommandError const error) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	outputText("Query error on entity " + la::avdecc::toHexString(entityID, true) + ": " + std::to_string(static_cast<std::underlying_type_t<decltype(error)>>(error)) + "\n");
}

void Discovery::onEntityOnline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	if (la::avdecc::hasFlag(entity->getEntity().getEntityCapabilities(), la::avdecc::entity::EntityCapabilities::AemSupported))
	{
		auto const& entityDescriptor = entity->getEntityDescriptor();
		std::uint32_t const vendorID = std::get<0>(la::avdecc::entity::model::splitVendorEntityModel(entityDescriptor.vendorEntityModelID));
		// Filter entities from the same vendor as this controller
		if (vendorID == VENDOR_ID)
		{
			outputText("New LA unit online: " + la::avdecc::toHexString(entityID, true) + "\n");
			_controller->acquireEntity(entity->getEntity().getEntityID(), false, [](la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const /*owningEntity*/) noexcept
			{
				if (!!status)
				{
					outputText("Unit acquired: " + la::avdecc::toHexString(entity->getEntity().getEntityID(), true) + "\n");
				}
			});
		}
		else if (la::avdecc::hasFlag(entity->getEntity().getTalkerCapabilities(), la::avdecc::entity::TalkerCapabilities::Implemented))
		{
			outputText("New talker online: " + la::avdecc::toHexString(entityID, true) + "\n");
		}
		else
		{
			outputText("New unknown entity online: " + la::avdecc::toHexString(entityID, true) + "\n");
		}
	}
	else
	{
		outputText("New NON-AEM entity online: " + la::avdecc::toHexString(entityID, true) + "\n");
	}
}

void Discovery::onEntityOffline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	outputText("Unit going offline: " + la::avdecc::toHexString(entityID, true) + "\n");
}


/* ************************************************************************** */
/* Main code                                                                  */
/* ************************************************************************** */

int doJob()
{
	auto const protocolInterfaceType = chooseProtocolInterfaceType();
	auto intfc = chooseNetworkInterface();

	if (intfc.type == la::avdecc::networkInterface::Interface::Type::None || protocolInterfaceType == la::avdecc::EndStation::ProtocolInterfaceType::None)
	{
		return 1;
	}

	try
	{
		outputText("Selected interface '" + intfc.alias + "' and protocol interface '" + la::avdecc::EndStation::typeToString(protocolInterfaceType) + "', discovery active:\n");

		Discovery discovery(protocolInterfaceType, intfc.name, 0x0003, la::avdecc::entity::model::makeVendorEntityModel(VENDOR_ID, DEVICE_ID, MODEL_ID), "en");

		std::this_thread::sleep_for(std::chrono::seconds(30));
	}
	catch (la::avdecc::controller::Controller::Exception const& e)
	{
		outputText(std::string("Cannot create controller: ") + e.what() + "\n");
		return 1;
	}
	catch (std::invalid_argument const& e)
	{
		assert(false && "Unknown exception (Should not happen anymore)");
		outputText(std::string("Cannot open interface: ") + e.what() + "\n");
		return 1;
	}
	catch (std::exception const& e)
	{
		assert(false && "Unknown exception (Should not happen anymore)");
		outputText(std::string("Unknown exception: ") + e.what() + "\n");
		return 1;
	}
	catch (...)
	{
		assert(false && "Unknown exception");
		outputText(std::string("Unknown exception\n"));
		return 1;
	}

	return 0;
}

int main()
{
	// Check avdecc library interface version (only required when using the shared version of the library, but the code is here as an example)
	if (!la::avdecc::isCompatibleWithInterfaceVersion(la::avdecc::InterfaceVersion))
	{
		outputText(std::string("Avdecc shared library interface version invalid:\nCompiled with interface ") + std::to_string(la::avdecc::InterfaceVersion) + " (v" + la::avdecc::getVersion() + "), but running interface " + std::to_string(la::avdecc::getInterfaceVersion()) + "\n");
		getch();
		return -1;
	}

	initOutput();

	auto ret = doJob();
	if (ret != 0)
	{
		outputText("\nTerminating with an error. Press any key to close\n");
		getch();
	}

	deinitOutput();

	return ret;
}
