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
* @file simpleEndpoint.cpp
* @author Christophe Calmejane
*/

/** ************************************************************************ **/
/** AVDECC CONTROLLED ENTITY EXAMPLE                                         **/
/** ************************************************************************ **/

#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/utils.hpp>
#include <la/avdecc/networkInterfaceHelper.hpp>
#include <la/avdecc/logger.hpp>
#include "utils.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <future>

template<class Object>
class AsyncExecutor final
{
public:
	using value_type = std::decay_t<Object>;

	AsyncExecutor()
	{
		_thread = std::thread(
			[this]
			{
				la::avdecc::utils::setCurrentThreadName("AsyncExecutor::Runner");
				while (!_shouldTerminate)
				{
					{
						// Lock
						auto const lg = std::lock_guard{ _lock };

						// Clear all objects
						_objects.clear();
					}
					// Wait a little bit so we don't burn the CPU
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
				}
			});
	}

	void deferred_object(value_type&& object)
	{
		// Lock
		auto const lg = std::lock_guard{ _lock };

		// Move to events queue
		_objects.emplace_back(std::make_shared<value_type>(std::move(object)));
	}

	~AsyncExecutor()
	{
		_shouldTerminate = true;
		if (_thread.joinable())
		{
			_thread.join();
		}
	}

	// Compiler auto-generated methods (move-only object)
	AsyncExecutor(AsyncExecutor const&) = delete;
	AsyncExecutor(AsyncExecutor&&) = default;
	AsyncExecutor& operator=(AsyncExecutor const&) = delete;
	AsyncExecutor& operator=(AsyncExecutor&&) = default;

private:
	using shared_object = std::shared_ptr<value_type>;

	std::mutex _lock{};
	std::vector<shared_object> _objects{};
	bool _shouldTerminate{ false };
	std::thread _thread{};
};

int doJob()
{
	class EndpointDelegate : public la::avdecc::entity::endpoint::Delegate, public la::avdecc::logger::Logger::Observer
	{
	public:
		~EndpointDelegate() noexcept override
		{
			la::avdecc::logger::Logger::getInstance().unregisterObserver(this);
		}

	private:
		// la::avdecc::logger::Logger::Observer overrides
		virtual void onLogItem(la::avdecc::logger::Level const level, la::avdecc::logger::LogItem const* const item) noexcept override
		{
			outputText("[" + la::avdecc::logger::Logger::getInstance().levelToString(level) + "] " + item->getMessage() + "\n");
		}
		// la::avdecc::entity::EndpointEntity::Delegate overrides
		/* Discovery Protocol (ADP) */
		virtual void onEntityOnline(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::Entity const& /*entity*/) noexcept override
		{
			try
			{
				std::stringstream ss;
				ss << std::hex << "### Unit online (" << la::avdecc::utils::toHexString(entityID, true) << ")";
				outputText(ss.str());
			}
			catch (...)
			{
				outputText("Uncaught exception in onEntityOnline");
			}
		}
		virtual void onEntityOffline(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const entityID) noexcept override
		{
			try
			{
				std::stringstream ss;
				ss << std::hex << "### Unit offline (" << la::avdecc::utils::toHexString(entityID, true) << ")" << std::endl;
				outputText(ss.str());
			}
			catch (...)
			{
				outputText("Uncaught exception in onEntityOffline");
			}
		}
		virtual void onEntityUpdate(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::Entity const& /*entity*/) noexcept override
		{
			try
			{
				std::stringstream ss;
				ss << std::hex << "### Unit updated (" << la::avdecc::utils::toHexString(entityID, true) << ")" << std::endl;
				outputText(ss.str());
			}
			catch (...)
			{
				outputText("Uncaught exception in onEntityUpdate");
			}
		}

		/* Query received from a Controller. */
		/** Called when a controller wants to register to unsolicited notifications. */
		virtual bool onQueryRegisterToUnsolicitedNotifications(la::avdecc::entity::endpoint::Interface const* const endpoint, la::avdecc::UniqueIdentifier const /*controllerID*/, la::avdecc::protocol::AemAecpdu const& command) noexcept override
		{
			// Immediate response
			endpoint->sendAemAecpResponse(command, la::avdecc::protocol::AecpStatus::Success, {});
			return true;
		}
		/** Called when a controller wants to deregister from unsolicited notifications. */
		virtual bool onQueryDeregisteredFromUnsolicitedNotifications(la::avdecc::entity::endpoint::Interface const* const endpoint, la::avdecc::UniqueIdentifier const /*controllerID*/, la::avdecc::protocol::AemAecpdu const& command) noexcept override
		{
			// Immediate response
			endpoint->sendAemAecpResponse(command, la::avdecc::protocol::AecpStatus::Success, {});
			return true;
		}
		/** Called when a controller wants to acquire the endpoint. */
		virtual bool onQueryAcquireEntity(la::avdecc::entity::endpoint::Interface const* const endpoint, la::avdecc::UniqueIdentifier const /*controllerID*/, la::avdecc::protocol::AemAecpdu const& command, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept override
		{
			_deferredQueue.deferred_object(std::async(std::launch::async,
				[endpoint, command]()
				{
					endpoint->sendAemAecpResponse(command, la::avdecc::protocol::AecpStatus::Success, command.getPayload());
				}));
			return true;
		}
		/** Called when a controller wants to release the endpoint. */
		virtual bool onQueryReleaseEntity(la::avdecc::entity::endpoint::Interface const* const endpoint, la::avdecc::UniqueIdentifier const /*controllerID*/, la::avdecc::protocol::AemAecpdu const& command, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept override
		{
			_deferredQueue.deferred_object(std::async(std::launch::async,
				[endpoint, command]()
				{
					endpoint->sendAemAecpResponse(command, la::avdecc::protocol::AecpStatus::Success, command.getPayload());
				}));
			endpoint->sendAemAecpResponse(command, la::avdecc::protocol::AemAecpStatus::InProgress, command.getPayload());
			return true;
		}

		// Result handlers
		/* Enumeration and Control Protocol (AECP) */

		/* Connection Management Protocol sniffed messages (ACMP) */
		//		virtual void onControllerConnectResponseSniffed(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept override
		//		{
		//			try
		//			{
		//				std::stringstream ss;
		//				ss << std::hex << "Stream connect status (" << la::avdecc::utils::toHexString(listenerStream.entityID, true) << " -> " << la::avdecc::utils::toHexString(talkerStream.entityID, true) << "): " << la::avdecc::entity::ControllerEntity::statusToString(status) << std::endl;
		//				outputText(ss.str());
		//			}
		//			catch (...)
		//			{
		//				outputText("Uncaught exception in onConnectStreamSniffed");
		//			}
		//		}
		//		virtual void onControllerDisconnectResponseSniffed(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept override
		//		{
		//			try
		//			{
		//				std::stringstream ss;
		//				ss << std::hex << "Stream disconnect status (" << la::avdecc::utils::toHexString(listenerStream.entityID, true) << " -> " << la::avdecc::utils::toHexString(talkerStream.entityID, true) << "): " << la::avdecc::entity::ControllerEntity::statusToString(status) << std::endl;
		//				outputText(ss.str());
		//			}
		//			catch (...)
		//			{
		//				outputText("Uncaught exception in onDisconnectStreamSniffed");
		//			}
		//		}
		//		virtual void onGetListenerStreamStateResponseSniffed(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const /*status*/) noexcept override
		//		{
		//			try
		//			{
		//			}
		//			catch (...)
		//			{
		//				outputText("Uncaught exception in onGetListenerStreamStateSniffed");
		//			}
		//		}


	private:
		AsyncExecutor<std::future<void>> _deferredQueue{};
	};

	auto const protocolInterfaceType = chooseProtocolInterfaceType(la::avdecc::protocol::ProtocolInterface::SupportedProtocolInterfaceTypes{ la::avdecc::protocol::ProtocolInterface::Type::PCap, la::avdecc::protocol::ProtocolInterface::Type::MacOSNative });
	auto intfc = chooseNetworkInterface();

	if (intfc.type == la::avdecc::networkInterface::Interface::Type::None || protocolInterfaceType == la::avdecc::protocol::ProtocolInterface::Type::None)
	{
		return 1;
	}

	try
	{
		outputText("Selected interface '" + intfc.alias + "' and protocol interface '" + la::avdecc::protocol::ProtocolInterface::typeToString(protocolInterfaceType) + "':\n");
		auto endStation = la::avdecc::EndStation::create(protocolInterfaceType, intfc.id);
		auto& protocolInterface = endStation->getProtocolInterface();
		auto endpointDelegate = EndpointDelegate{};

		// Register log observer
		la::avdecc::logger::Logger::getInstance().registerObserver(&endpointDelegate);
		// Set default log level
		la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Trace);

		auto const eid = la::avdecc::entity::Entity::generateEID(protocolInterface.getMacAddress(), 0x0001);
		auto const commonInformation{ la::avdecc::entity::Entity::CommonInformation{ eid, la::avdecc::entity::model::makeEntityModelID(VENDOR_ID, DEVICE_ID, MODEL_ID), la::avdecc::entity::EntityCapabilities{ la::avdecc::entity::EntityCapability::AemSupported }, 0u, la::avdecc::entity::TalkerCapabilities{}, 0u, la::avdecc::entity::ListenerCapabilities{ la::avdecc::entity::ListenerCapability::Implemented }, la::avdecc::entity::ControllerCapabilities{}, std::nullopt, std::nullopt } };
		auto* endpoint = endStation->addEndpointEntity(commonInformation, &endpointDelegate);
		//auto* endpoint = endStation->addAggregateEntity(0x0001, la::avdecc::entity::model::makeEntityModelID(VENDOR_ID, DEVICE_ID, MODEL_ID), nullptr, &endpointDelegate);

		// Try to start entity advertisement
		if (!endpoint->enableEntityAdvertising(10))
		{
			outputText("EntityID already in use on the local computer\n");
			return 1;
		}

		std::this_thread::sleep_for(std::chrono::seconds(30));
	}
	catch (la::avdecc::EndStation::Exception const& e)
	{
		outputText(std::string("Cannot create EndStation: ") + e.what() + "\n");
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

	outputText("Done!\nPress any key to terminate.\n");
	getch();

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

	auto ret = doJob();
	if (ret != 0)
	{
		outputText("\nTerminating with an error. Press any key to close\n");
		getch();
	}

	deinitOutput();

	return ret;
}
