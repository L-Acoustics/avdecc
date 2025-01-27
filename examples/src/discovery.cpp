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
* @file discovery.cpp
* @author Christophe Calmejane
*/

/** ************************************************************************ **/
/** AVDECC_CONTROLLER DELEGATE EXAMPLE                                       **/
/** ************************************************************************ **/

#include <la/avdecc/controller/avdeccController.hpp>
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

//#define LOAD_TEST_VIRTUAL_ENTITY

#if defined(LOAD_TEST_VIRTUAL_ENTITY)
class Builder : public la::avdecc::controller::model::DefaultedVirtualEntityBuilder
{
public:
	Builder() noexcept = default;

	virtual void build(la::avdecc::entity::model::EntityTree const& entityTree, la::avdecc::entity::Entity::CommonInformation& commonInformation, la::avdecc::entity::Entity::InterfacesInformation& intfcInformation) noexcept override
	{
		auto const countInputStreams = [](la::avdecc::entity::model::EntityTree const& entityTree)
		{
			// Very crude and shouldn't be considered a good example
			auto count = size_t{ 0u };
			if (entityTree.configurationTrees.empty())
			{
				return count;
			}
			return entityTree.configurationTrees.begin()->second.streamInputModels.size();
		};
		commonInformation.entityID = la::avdecc::UniqueIdentifier{ 0x0102030405060708 };
		//commonInformation.entityModelID = la::avdecc::UniqueIdentifier{ 0x1122334455667788 };
		commonInformation.entityCapabilities = la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported };
		//commonInformation.talkerStreamSources = 0u;
		//commonInformation.talkerCapabilities = {};
		commonInformation.listenerStreamSinks = static_cast<decltype(commonInformation.listenerStreamSinks)>(countInputStreams(entityTree));
		commonInformation.listenerCapabilities = la::avdecc::entity::ListenerCapabilities{ la::avdecc::entity::ListenerCapability::Implemented };
		//commonInformation.controllerCapabilities = {};
		commonInformation.identifyControlIndex = la::avdecc::entity::model::ControlIndex{ 0u };
		//commonInformation.associationID = std::nullopt;

		auto const interfaceInfo = la::avdecc::entity::Entity::InterfaceInformation{ la::networkInterface::MacAddress{ 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 }, 31u, 0u, std::nullopt, std::nullopt };
		intfcInformation[la::avdecc::entity::Entity::GlobalAvbInterfaceIndex] = interfaceInfo;
	}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::EntityNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::EntityNodeDynamicModel& dynamicModel) noexcept override
	{
		dynamicModel.entityName = la::avdecc::entity::model::AvdeccFixedString{ "Test entity" };
	}
	virtual void build(la::avdecc::controller::ControlledEntity::CompatibilityFlags& compatibilityFlags) noexcept override
	{
		compatibilityFlags.set(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221);
		compatibilityFlags.set(la::avdecc::controller::ControlledEntity::CompatibilityFlag::Milan);
	}
};
#endif // LOAD_TEST_VIRTUAL_ENTITY

/* ************************************************************************** */
/* Discovery class                                                            */
/* ************************************************************************** */

class Discovery : public la::avdecc::controller::Controller::DefaultedObserver, public la::avdecc::logger::Logger::Observer
{
public:
	/** Constructor/destructor/destroy */
	Discovery(la::avdecc::protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, la::avdecc::UniqueIdentifier const entityModelID, std::string const& preferedLocale);
	~Discovery() noexcept override
	{
		la::avdecc::logger::Logger::getInstance().unregisterObserver(this);
	}

	// Deleted compiler auto-generated methods
	Discovery(Discovery&&) = delete;
	Discovery(Discovery const&) = delete;
	Discovery& operator=(Discovery const&) = delete;
	Discovery& operator=(Discovery&&) = delete;

private:
	// Private methods
	std::string flagsToString(la::avdecc::controller::ControlledEntity::CompatibilityFlags const compatibilityFlags) const noexcept;
	// la::avdecc::Logger::Observer overrides
	virtual void onLogItem(la::avdecc::logger::Level const level, la::avdecc::logger::LogItem const* const item) noexcept override
	{
		outputText("[" + la::avdecc::logger::Logger::getInstance().levelToString(level) + "] " + item->getMessage() + "\n");
	}
	// la::avdecc::controller::Controller::Observer overrides
	// Global notifications
	virtual void onTransportError(la::avdecc::controller::Controller const* const controller) noexcept override;
	virtual void onEntityQueryError(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::Controller::QueryCommandError const error) noexcept override;
	// Discovery notifications (ADP)
	virtual void onEntityOnline(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity) noexcept override;
	virtual void onEntityOffline(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity) noexcept override;
	// Statistics
	virtual void onAecpRetryCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, std::uint64_t const /*value*/) noexcept override;
	virtual void onAecpTimeoutCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, std::uint64_t const /*value*/) noexcept override;
	virtual void onAecpUnexpectedResponseCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, std::uint64_t const /*value*/) noexcept override;
	virtual void onAecpResponseAverageTimeChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, std::chrono::milliseconds const& value) noexcept override;
	virtual void onAemAecpUnsolicitedCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, std::uint64_t const /*value*/) noexcept override;
	virtual void onAemAecpUnsolicitedLossCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const /*entity*/, std::uint64_t const /*value*/) noexcept override;
	virtual void onMaxTransitTimeChanged(la::avdecc::controller::Controller const* const controller, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const streamIndex, std::chrono::nanoseconds const& maxTransitTime) noexcept override;

private:
	la::avdecc::controller::Controller::UniquePointer _controller{ nullptr, nullptr }; // Read/Write from the UI thread (and read only from la::avdecc::controller::Controller::Observer callbacks)
	DECLARE_AVDECC_OBSERVER_GUARD(Discovery); // Not really needed because the _controller field will be destroyed before parent class destruction
};

Discovery::Discovery(la::avdecc::protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, la::avdecc::UniqueIdentifier const entityModelID, std::string const& preferedLocale)
	: _controller(la::avdecc::controller::Controller::create(protocolInterfaceType, interfaceName, progID, entityModelID, preferedLocale, nullptr, std::nullopt, nullptr))
{
	// Register observers
	la::avdecc::logger::Logger::getInstance().registerObserver(this);
	_controller->registerObserver(this);
	// Start controller advertising
	_controller->enableEntityAdvertising(10);
	// Enable aem caching and fast enum
	_controller->enableEntityModelCache();
	_controller->enableFastEnumeration();
	// Set default log level
	la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Trace);

#if defined(LOAD_TEST_VIRTUAL_ENTITY)
	auto builder = Builder{};
	auto const [error, message] = _controller->createVirtualEntityFromEntityModelFile("SimpleEntityModel.json", &builder, false);
	if (error != la::avdecc::jsonSerializer::DeserializationError::NoError)
	{
		outputText("Error creating virtual entity: " + std::to_string(static_cast<std::underlying_type_t<decltype(error)>>(error)) + "\n");
	}
	else
	{
		outputText("Virtual entity created\n");
	}
#endif // LOAD_TEST_VIRTUAL_ENTITY
}

std::string Discovery::flagsToString(la::avdecc::controller::ControlledEntity::CompatibilityFlags const compatibilityFlags) const noexcept
{
	auto str = std::string{};
	if (compatibilityFlags.test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221))
	{
		str += "IEEE17221 ";
	}
	if (compatibilityFlags.test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::Milan))
	{
		str += "Milan ";
	}
	if (compatibilityFlags.test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::IEEE17221Warning))
	{
		str += "IEEE17221Warning ";
	}
	if (compatibilityFlags.test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::MilanWarning))
	{
		str += "MilanWarning ";
	}
	if (compatibilityFlags.test(la::avdecc::controller::ControlledEntity::CompatibilityFlag::Misbehaving))
	{
		str += "Misbehaving ";
	}
	return str;
}

void Discovery::onTransportError(la::avdecc::controller::Controller const* const /*controller*/) noexcept
{
	outputText("Fatal error on transport layer\n");
}

void Discovery::onEntityQueryError(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* entity, la::avdecc::controller::Controller::QueryCommandError const error) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	outputText("Query error on entity " + la::avdecc::utils::toHexString(entityID, true) + ": " + std::to_string(static_cast<std::underlying_type_t<decltype(error)>>(error)) + "\n");
}

void Discovery::onEntityOnline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	if (entity->getEntity().getEntityCapabilities().test(la::avdecc::entity::EntityCapability::AemSupported))
	{
		std::uint32_t const vendorID = std::get<0>(la::avdecc::entity::model::splitEntityModelID(entity->getEntity().getEntityModelID()));
		// Filter entities from the same vendor as this controller
		if (vendorID == VENDOR_ID)
		{
			outputText("New LA unit online: " + la::avdecc::utils::toHexString(entityID, true) + " (Compatibility: " + flagsToString(entity->getCompatibilityFlags()) + ")\n");
			_controller->acquireEntity(entity->getEntity().getEntityID(), false,
				[](la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const /*owningEntity*/) noexcept
				{
					if (!!status)
					{
						outputText("Unit acquired: " + la::avdecc::utils::toHexString(entity->getEntity().getEntityID(), true) + "\n");
					}
				});
		}
		else if (entity->getEntity().getTalkerCapabilities().test(la::avdecc::entity::TalkerCapability::Implemented))
		{
			outputText("New talker online: " + la::avdecc::utils::toHexString(entityID, true) + "\n");
		}
		else
		{
			outputText("New unknown entity online: " + la::avdecc::utils::toHexString(entityID, true) + "\n");
		}

		// Get PNG Manufacturer image
		auto const& configNode = entity->getCurrentConfigurationNode();
		for (auto const& objIt : configNode.memoryObjects)
		{
			auto const& obj = objIt.second;
			if (obj.staticModel.memoryObjectType == la::avdecc::entity::model::MemoryObjectType::PngEntity)
			{
				_controller->readDeviceMemory(entity->getEntity().getEntityID(), obj.staticModel.startAddress, obj.staticModel.maximumLength,
					[](la::avdecc::controller::ControlledEntity const* const /*entity*/, float const percentComplete)
					{
						outputText("Memory Object progress: " + std::to_string(percentComplete) + "\n");
						return false;
					},
					[](la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AaCommandStatus const status, la::avdecc::controller::Controller::DeviceMemoryBuffer const& memoryBuffer)
					{
						if (!!status && entity)
						{
							auto const fileName{ std::to_string(entity->getEntity().getEntityID()) + ".png" };
							std::ofstream file(fileName, std::ios::binary);
							if (file.is_open())
							{
								file.write(reinterpret_cast<char const*>(memoryBuffer.data()), memoryBuffer.size());
								file.close();
								outputText("Memory Object saved to file: " + fileName + "\n");
							}
						}
					});
			}
		}
	}
	else
	{
		outputText("New NON-AEM entity online: " + la::avdecc::utils::toHexString(entityID, true) + "\n");
	}
}

void Discovery::onEntityOffline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	outputText("Unit going offline: " + la::avdecc::utils::toHexString(entityID, true) + "\n");
}

// Statistics
void Discovery::onAecpRetryCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity, std::uint64_t const value) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	outputText("AECP Retry Counter for " + la::avdecc::utils::toHexString(entityID, true) + ": " + std::to_string(value) + "\n");
}

void Discovery::onAecpTimeoutCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity, std::uint64_t const value) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	outputText("Aecp Timeout Counter for " + la::avdecc::utils::toHexString(entityID, true) + ": " + std::to_string(value) + "\n");
}

void Discovery::onAecpUnexpectedResponseCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity, std::uint64_t const value) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	outputText("Aecp Unexpected Response Counter for " + la::avdecc::utils::toHexString(entityID, true) + ": " + std::to_string(value) + "\n");
}

void Discovery::onAecpResponseAverageTimeChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity, std::chrono::milliseconds const& value) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	outputText("Aecp Response Average Time for " + la::avdecc::utils::toHexString(entityID, true) + ": " + std::to_string(value.count()) + " msec\n");
}

void Discovery::onAemAecpUnsolicitedCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity, std::uint64_t const value) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	outputText("Aem Aecp Unsolicited Counter for " + la::avdecc::utils::toHexString(entityID, true) + ": " + std::to_string(value) + "\n");
}

void Discovery::onAemAecpUnsolicitedLossCounterChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity, std::uint64_t const value) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	outputText("Aem Aecp Unsolicited Loss Counter for " + la::avdecc::utils::toHexString(entityID, true) + ": " + std::to_string(value) + "\n");
}

void Discovery::onMaxTransitTimeChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const streamIndex, std::chrono::nanoseconds const& maxTransitTime) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();
	outputText("Max Transit Time for " + la::avdecc::utils::toHexString(entityID, true) + " Stream " + std::to_string(streamIndex) + ": " + std::to_string(maxTransitTime.count()) + " nsec\n");
}


/* ************************************************************************** */
/* Main code                                                                  */
/* ************************************************************************** */

int doJob()
{
	auto const protocolInterfaceType = chooseProtocolInterfaceType(la::avdecc::protocol::ProtocolInterface::SupportedProtocolInterfaceTypes{ la::avdecc::protocol::ProtocolInterface::Type::PCap, la::avdecc::protocol::ProtocolInterface::Type::MacOSNative });
	auto intfc = chooseNetworkInterface();

	if (intfc.type == la::networkInterface::Interface::Type::None || protocolInterfaceType == la::avdecc::protocol::ProtocolInterface::Type::None)
	{
		return 1;
	}

	try
	{
		outputText("Selected interface '" + intfc.alias + "' and protocol interface '" + la::avdecc::protocol::ProtocolInterface::typeToString(protocolInterfaceType) + "', discovery active:\n");

		// Create a discovery object
		{
			auto discovery = Discovery{ protocolInterfaceType, intfc.id, 0x0001, la::avdecc::entity::model::makeEntityModelID(VENDOR_ID, DEVICE_ID, MODEL_ID), "en" };
			std::this_thread::sleep_for(std::chrono::seconds(10));
			outputText("Destroying discovery object\n");
		}

		// Create another one
		{
			auto discovery = Discovery{ protocolInterfaceType, intfc.id, 0x0001, la::avdecc::entity::model::makeEntityModelID(VENDOR_ID, DEVICE_ID, MODEL_ID), "en" };
			std::this_thread::sleep_for(std::chrono::seconds(1500));
			outputText("Destroying discovery object\n");
		}
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
