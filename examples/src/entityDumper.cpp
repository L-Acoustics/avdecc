/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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
* @file entityDumper.cpp
* @author Christophe Calmejane
*/

/** ************************************************************************ **/
/** ENTITIES DUMPER EXAMPLE                                                  **/
/** ************************************************************************ **/

#include <la/avdecc/controller/avdeccController.hpp>
#include <la/avdecc/networkInterfaceHelper.hpp>
#include <la/avdecc/utils.hpp>
#include <la/avdecc/logger.hpp>
#include "utils.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <fstream>
#include <chrono>
#include <unordered_map>
#include <stdexcept>
#include <cassert>

/* ************************************************************************** */
/* Dumper class                                                               */
/* ************************************************************************** */

class Dumper : public la::avdecc::controller::Controller::Observer, public la::avdecc::logger::Logger::Observer
{
public:
	/** Constructor/destructor/destroy */
	Dumper(la::avdecc::protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, la::avdecc::UniqueIdentifier const entityModelID, std::string const& preferedLocale);
	~Dumper() noexcept override;

	// Deleted compiler auto-generated methods
	Dumper(Dumper&&) = delete;
	Dumper(Dumper const&) = delete;
	Dumper& operator=(Dumper const&) = delete;
	Dumper& operator=(Dumper&&) = delete;

private:
	// la::avdecc::Logger::Observer overrides
	virtual void onLogItem(la::avdecc::logger::Level const /*level*/, la::avdecc::logger::LogItem const* const /*item*/) noexcept override
	{
		// TODO: Save to an internal map and dump
		//outputText("[" + la::avdecc::logger::Logger::getInstance().levelToString(level) + "] " + item->getMessage() + "\n");
	}
	// la::avdecc::controller::Controller::Observer overrides
	// Global notifications
	virtual void onTransportError(la::avdecc::controller::Controller const* const controller) noexcept override;
	virtual void onEntityQueryError(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::Controller::QueryCommandError const error) noexcept override;
	// Discovery notifications (ADP)
	virtual void onEntityOnline(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity) noexcept override;

private:
	la::avdecc::controller::Controller::UniquePointer _controller{ nullptr, nullptr }; // Read/Write from the UI thread (and read only from la::avdecc::controller::Controller::Observer callbacks)
	DECLARE_AVDECC_OBSERVER_GUARD(Dumper); // Not really needed because the _controller field will be destroyed before parent class destruction
};

Dumper::Dumper(la::avdecc::protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, la::avdecc::UniqueIdentifier const entityModelID, std::string const& preferedLocale)
	: _controller(la::avdecc::controller::Controller::create(protocolInterfaceType, interfaceName, progID, entityModelID, preferedLocale))
{
	// Register observers
	la::avdecc::logger::Logger::getInstance().registerObserver(this);
	_controller->registerObserver(this);
	// Start controller advertising
	_controller->enableEntityAdvertising(10);
	// Set default log level
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Trace);
}

Dumper::~Dumper() noexcept
{
	auto const networkDumpFileName = std::string{ "FullDump.json" };
	auto const [error, message] = _controller->serializeAllControlledEntitiesAsReadableJson(networkDumpFileName, false, false);
	if (!!error)
	{
		outputText("Failed to dump all entities: " + message + "\n");
	}
	else
	{
		outputText("Successfully dumped network state to " + networkDumpFileName + "\n");
	}
	la::avdecc::logger::Logger::getInstance().unregisterObserver(this);
}

void Dumper::onTransportError(la::avdecc::controller::Controller const* const /*controller*/) noexcept
{
	outputText("Fatal error on transport layer\n");
}

void Dumper::onEntityQueryError(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* entity, la::avdecc::controller::Controller::QueryCommandError const error) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	outputText("Query error on entity " + la::avdecc::utils::toHexString(entityID, true) + ": " + std::to_string(static_cast<std::underlying_type_t<decltype(error)>>(error)) + "\n");
}

void Dumper::onEntityOnline(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	auto const entityString = la::avdecc::utils::toHexString(entityID, true, true);
	auto const entityDumpFileName = std::string("EntityDump_") + entityString + ".json";

	// Dump as JSON
	{
		auto const [error, message] = controller->serializeControlledEntityAsReadableJson(entityID, entityDumpFileName, false);
		if (!!error)
		{
			outputText("Failed to dump entity " + entityString + ": " + message + "\n");
		}
		else
		{
			outputText("Successfully dumped entity " + entityString + " to " + entityDumpFileName + "\n");
		}
	}

	// TODO: Dump log file
}


/* ************************************************************************** */
/* Main code                                                                  */
/* ************************************************************************** */

int doJob()
{
	auto const protocolInterfaceType = chooseProtocolInterfaceType(la::avdecc::protocol::ProtocolInterface::SupportedProtocolInterfaceTypes{ la::avdecc::protocol::ProtocolInterface::Type::PCap });
	auto intfc = chooseNetworkInterface();

	if (intfc.type == la::avdecc::networkInterface::Interface::Type::None || protocolInterfaceType == la::avdecc::protocol::ProtocolInterface::Type::None)
	{
		return 1;
	}

	try
	{
		outputText("Selected interface '" + intfc.alias + "' and protocol interface '" + la::avdecc::protocol::ProtocolInterface::typeToString(protocolInterfaceType) + "', waiting for entities for 10 seconds...\n");

		Dumper dumper(protocolInterfaceType, intfc.id, 0x0001, la::avdecc::entity::model::makeEntityModelID(VENDOR_ID, DEVICE_ID, MODEL_ID), "en");

		std::this_thread::sleep_for(std::chrono::seconds(10));

		outputText("Done.\n");
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

	outputText(std::string("Using Avdecc Library v") + la::avdecc::getVersion() + " with compilation options:\n");
	for (auto const& info : la::avdecc::getCompileOptionsInfo())
	{
		outputText(std::string(" - ") + info.longName + " (" + info.shortName + ")\n");
	}
	outputText("\n");

	outputText(std::string("Using Avdecc Controller Library v") + la::avdecc::controller::getVersion() + " with compilation options:\n");
	for (auto const& info : la::avdecc::controller::getCompileOptionsInfo())
	{
		outputText(std::string(" - ") + info.longName + " (" + info.shortName + ")\n");
	}
	outputText("\n");

	auto ret = doJob();
	if (ret != 0)
	{
		outputText("\nTerminating with an error. Press any key to close\n");
		getch();
	}

	deinitOutput();

	return ret;
}
