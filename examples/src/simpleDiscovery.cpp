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
* @file simpleDiscovery.cpp
* @author Christophe Calmejane
* @brief Minimal, non-interactive AVDECC discovery sample.
*
* @details Runs an AVDECC controller for a fixed duration, printing entity online/offline
*          notifications (EntityID and entity name when available) to stdout. All parameters
*          are passed via command line; the program never reads from stdin. This makes it
*          suitable for bundling into a macOS .app with an embedded provisioning profile,
*          which is required to exercise the `MacOSNCap` (Network.framework) ProtocolInterface.
*/

#include <la/avdecc/controller/avdeccController.hpp>
#include <la/avdecc/utils.hpp>
#include <la/networkInterfaceHelper/networkInterfaceHelper.hpp>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace
{
constexpr auto DefaultDurationSeconds = 10u;
constexpr auto VendorID = std::uint32_t{ 0x001B92 };
constexpr auto DeviceID = std::uint32_t{ 0x80 };
constexpr auto ModelID = std::uint32_t{ 0x00000001 };

auto const AllowedProtocolInterfaceTypes = la::avdecc::protocol::ProtocolInterface::SupportedProtocolInterfaceTypes{ la::avdecc::protocol::ProtocolInterface::Type::PCap, la::avdecc::protocol::ProtocolInterface::Type::MacOSNative, la::avdecc::protocol::ProtocolInterface::Type::MacOSNCap };

/** @brief Short CLI token for a given ProtocolInterface::Type. */
std::string protocolTypeToToken(la::avdecc::protocol::ProtocolInterface::Type const type) noexcept
{
	switch (type)
	{
		case la::avdecc::protocol::ProtocolInterface::Type::PCap:
			return "pcap";
		case la::avdecc::protocol::ProtocolInterface::Type::MacOSNative:
			return "macos-native";
		case la::avdecc::protocol::ProtocolInterface::Type::MacOSNCap:
			return "macos-ncap";
		default:
			return "unknown";
	}
}

/** @brief Parse a CLI protocol interface token into a ProtocolInterface::Type (limited to Allowed types). */
la::avdecc::protocol::ProtocolInterface::Type tokenToProtocolType(std::string const& token) noexcept
{
	for (auto const type : la::avdecc::protocol::ProtocolInterface::getSupportedProtocolInterfaceTypes() & AllowedProtocolInterfaceTypes)
	{
		if (protocolTypeToToken(type) == token)
		{
			return type;
		}
	}
	return la::avdecc::protocol::ProtocolInterface::Type::None;
}

/** @brief Enumerate all available network interfaces via la::networkInterface. */
std::vector<la::networkInterface::Interface> enumerateAllInterfaces() noexcept
{
	auto interfaces = std::vector<la::networkInterface::Interface>{};
	la::networkInterface::NetworkInterfaceHelper::getInstance().enumerateInterfaces(
		[&interfaces](la::networkInterface::Interface const& intfc)
		{
			interfaces.push_back(intfc);
		});
	return interfaces;
}

/** @brief Print CLI usage. */
void printUsage(std::string const& programName) noexcept
{
	std::cout << "Usage: " << programName << " [options]\n";
	std::cout << "\n";
	std::cout << "Options:\n";
	std::cout << "  --help, -h                   Show this help message and exit\n";
	std::cout << "  --list-protocols             List supported protocol interface types and exit\n";
	std::cout << "  --list-interfaces            List available network interfaces and exit\n";
	std::cout << "  --protocol-interface <name>  Protocol interface to use (use --list-protocols to see available options)\n";
	std::cout << "  --network-interface <id>     Network interface BSD name to use (use --list-interfaces to see available options)\n";
	std::cout << "  --duration <seconds>         Discovery duration in seconds (default: " << DefaultDurationSeconds << ")\n";
	std::cout << "\n";
	std::cout << "Examples:\n";
	std::cout << "  " << programName << " --list-interfaces\n";
	std::cout << "  " << programName << " --protocol-interface pcap --network-interface en0 --duration 5\n";
}

/** @brief Print the list of available network interfaces to stdout. */
void printInterfaces() noexcept
{
	std::cout << "Available network interfaces:\n";
	for (auto const& intfc : enumerateAllInterfaces())
	{
		std::cout << "  " << intfc.id;
		std::cout << " [alias: " << intfc.alias << "]";
		std::cout << " [connected: " << (intfc.isConnected ? "yes" : "no") << "]";
		std::cout << " [virtual: " << (intfc.isVirtual ? "yes" : "no") << "]";
		std::cout << "\n";
	}
}

/** @brief Print the list of supported protocol interface types accepted by this sample. */
void printProtocols() noexcept
{
	std::cout << "Supported protocol interface types:\n";
	for (auto const type : la::avdecc::protocol::ProtocolInterface::getSupportedProtocolInterfaceTypes() & AllowedProtocolInterfaceTypes)
	{
		std::cout << "  " << protocolTypeToToken(type) << "  -  " << la::avdecc::protocol::ProtocolInterface::typeToString(type) << "\n";
	}
}

/** @brief Retrieve the entity name (best-effort). Falls back to empty string if unavailable. */
std::string getEntityName(la::avdecc::controller::ControlledEntity const& entity) noexcept
{
	try
	{
		auto const& dynamicModel = entity.getEntityNode().dynamicModel;
		return dynamicModel.entityName.str();
	}
	catch (...)
	{
		return {};
	}
}

/**
* @brief Minimal Controller observer that prints entity online/offline events.
*/
class SimpleObserver : public la::avdecc::controller::Controller::DefaultedObserver
{
public:
	virtual void onEntityOnline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity) noexcept override
	{
		auto const entityID = entity->getEntity().getEntityID();
		auto const name = getEntityName(*entity);
		std::cout << "[ONLINE ] " << la::avdecc::utils::toHexString(entityID, true);
		if (!name.empty())
		{
			std::cout << "  name=\"" << name << "\"";
		}
		std::cout << "\n";
		std::flush(std::cout);
	}

	virtual void onEntityOffline(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity) noexcept override
	{
		auto const entityID = entity->getEntity().getEntityID();
		auto const name = getEntityName(*entity);
		std::cout << "[OFFLINE] " << la::avdecc::utils::toHexString(entityID, true);
		if (!name.empty())
		{
			std::cout << "  name=\"" << name << "\"";
		}
		std::cout << "\n";
		std::flush(std::cout);
	}

	virtual void onEntityNameChanged(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::AvdeccFixedString const& entityName) noexcept override
	{
		auto const entityID = entity->getEntity().getEntityID();
		std::cout << "[NAME   ] " << la::avdecc::utils::toHexString(entityID, true) << "  name=\"" << entityName.str() << "\"\n";
		std::flush(std::cout);
	}

	virtual void onTransportError(la::avdecc::controller::Controller const* const /*controller*/, la::avdecc::controller::Controller::InterfaceType const interfaceType) noexcept override
	{
		std::cerr << "[ERROR  ] Transport error on " << ((interfaceType == la::avdecc::controller::Controller::InterfaceType::Primary) ? "Primary" : "Secondary") << " interface\n";
	}
};

/** @brief Run the discovery session for the specified duration. */
int runDiscovery(la::avdecc::protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& interfaceID, std::string const& interfaceAlias, std::chrono::seconds const duration) noexcept
{
	try
	{
		std::cout << "Selected interface '" << interfaceAlias << "' (" << interfaceID << ") with protocol '" << la::avdecc::protocol::ProtocolInterface::typeToString(protocolInterfaceType) << "'. Running discovery for " << duration.count() << "s...\n";
		std::flush(std::cout);

		auto controller = la::avdecc::controller::Controller::create(protocolInterfaceType, interfaceID, 0x0001, la::avdecc::entity::model::makeEntityModelID(VendorID, DeviceID, ModelID), "en", nullptr, std::nullopt, nullptr);

		auto observer = SimpleObserver{};
		controller->registerObserver(&observer);
		controller->enableEntityAdvertising(10);

		std::this_thread::sleep_for(duration);

		controller->unregisterObserver(&observer);
		std::cout << "Discovery session ended.\n";
	}
	catch (la::avdecc::controller::Controller::Exception const& e)
	{
		std::cerr << "Cannot create controller: " << e.what() << "\n";
		return 1;
	}
	catch (std::exception const& e)
	{
		std::cerr << "Unhandled exception: " << e.what() << "\n";
		return 1;
	}
	catch (...)
	{
		std::cerr << "Unhandled unknown exception\n";
		return 1;
	}
	return 0;
}

struct CliOptions
{
	bool listInterfaces{ false };
	bool listProtocols{ false };
	bool showHelp{ false };
	std::string protocolInterfaceToken{};
	std::string networkInterfaceName{};
	std::chrono::seconds discoveryDuration{ DefaultDurationSeconds };
};

bool parseCli(int argc, char* argv[], CliOptions& out, std::string& error) noexcept
{
	for (auto i = 1; i < argc; ++i)
	{
		auto const arg = std::string{ argv[i] };
		auto const nextArg = [&]() -> std::string
		{
			if (i + 1 >= argc)
			{
				return {};
			}
			return std::string{ argv[++i] };
		};

		if (arg == "--help" || arg == "-h")
		{
			out.showHelp = true;
		}
		else if (arg == "--list-interfaces")
		{
			out.listInterfaces = true;
		}
		else if (arg == "--list-protocols")
		{
			out.listProtocols = true;
		}
		else if (arg == "--protocol-interface")
		{
			out.protocolInterfaceToken = nextArg();
			if (out.protocolInterfaceToken.empty())
			{
				error = "--protocol-interface requires a value";
				return false;
			}
		}
		else if (arg == "--network-interface")
		{
			out.networkInterfaceName = nextArg();
			if (out.networkInterfaceName.empty())
			{
				error = "--network-interface requires a value";
				return false;
			}
		}
		else if (arg == "--duration")
		{
			auto const value = nextArg();
			if (value.empty())
			{
				error = "--duration requires a value";
				return false;
			}
			try
			{
				out.discoveryDuration = std::chrono::seconds{ std::stoi(value) };
			}
			catch (...)
			{
				error = "--duration: invalid integer value '" + value + "'";
				return false;
			}
			if (out.discoveryDuration.count() <= 0)
			{
				error = "--duration must be strictly positive";
				return false;
			}
		}
		else
		{
			error = "Unknown argument: '" + arg + "' (use --help for usage)";
			return false;
		}
	}
	return true;
}

} // namespace

int main(int argc, char* argv[])
{
	auto options = CliOptions{};
	auto parseError = std::string{};
	if (!parseCli(argc, argv, options, parseError))
	{
		std::cerr << parseError << "\n";
		return 1;
	}

	if (options.showHelp)
	{
		printUsage(argc > 0 ? argv[0] : "SimpleDiscovery");
		return 0;
	}
	if (options.listInterfaces)
	{
		printInterfaces();
		return 0;
	}
	if (options.listProtocols)
	{
		printProtocols();
		return 0;
	}

	if (!la::avdecc::isCompatibleWithInterfaceVersion(la::avdecc::InterfaceVersion))
	{
		std::cerr << "Avdecc shared library interface version invalid: compiled with " << la::avdecc::InterfaceVersion << " (v" << la::avdecc::getVersion() << "), running with " << la::avdecc::getInterfaceVersion() << "\n";
		return -1;
	}

	if (options.protocolInterfaceToken.empty() || options.networkInterfaceName.empty())
	{
		std::cerr << "Both --protocol-interface and --network-interface are required.\n";
		std::cerr << "Use --help for usage, --list-protocols and --list-interfaces to see valid values.\n";
		return 1;
	}

	auto const protocolInterfaceType = tokenToProtocolType(options.protocolInterfaceToken);
	if (protocolInterfaceType == la::avdecc::protocol::ProtocolInterface::Type::None)
	{
		std::cerr << "Invalid or unsupported --protocol-interface token: '" << options.protocolInterfaceToken << "'. Use --list-protocols to see supported values.\n";
		return 1;
	}

	// Validate the network interface exists via networkInterfaceHelper
	auto found = la::networkInterface::Interface{};
	found.type = la::networkInterface::Interface::Type::None;
	for (auto const& intfc : enumerateAllInterfaces())
	{
		if (intfc.id == options.networkInterfaceName)
		{
			found = intfc;
			break;
		}
	}
	if (found.type == la::networkInterface::Interface::Type::None)
	{
		std::cerr << "Unknown network interface: '" << options.networkInterfaceName << "'. Use --list-interfaces to see available values.\n";
		return 1;
	}

	return runDiscovery(protocolInterfaceType, found.id, found.alias, options.discoveryDuration);
}
