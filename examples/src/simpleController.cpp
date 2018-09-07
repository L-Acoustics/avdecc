/*
* Copyright (C) 2016-2018, L-Acoustics and its contributors

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
* @file simpleController.cpp
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

int doJob()
{
	class ControllerDelegate : public la::avdecc::entity::ControllerEntity::Delegate, public la::avdecc::logger::Logger::Observer
	{
	private:
		// la::avdecc::logger::Logger::Observer overrides
		virtual void onLogItem(la::avdecc::logger::Level const level, la::avdecc::logger::LogItem const* const item) noexcept override
		{
			outputText("[" + la::avdecc::logger::Logger::getInstance().levelToString(level) + "] " + item->getMessage() + "\n");
		}
		// la::avdecc::entity::ControllerEntity::Delegate overrides
		/* Discovery Protocol (ADP) */
		virtual void onEntityOnline(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::Entity const& entity) noexcept override
		{
			try
			{
				std::stringstream ss;
				ss << std::hex << "### Unit online (" << la::avdecc::toHexString(entityID, true) << ")";
				if (la::avdecc::hasFlag(entity.getEntityCapabilities(), la::avdecc::entity::EntityCapabilities::AemSupported))
				{
					ss << std::hex << ", querying EntityModel" << std::endl;
					controller->readEntityDescriptor(entityID, std::bind(&ControllerDelegate::onEntityDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
				}
				else
				{
					ss << std::hex << ", but EntityModel not supported" << std::endl;
				}
				outputText(ss.str());
			}
			catch (...)
			{
				outputText("Uncaught exception in onEntityOnline");
			}
		}
		virtual void onEntityOffline(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID) noexcept override
		{
			try
			{
				std::stringstream ss;
				ss << std::hex << "### Unit offline (" << la::avdecc::toHexString(entityID, true) << ")" << std::endl;
				outputText(ss.str());
			}
			catch (...)
			{
				outputText("Uncaught exception in onEntityOffline");
			}
		}
		virtual void onEntityUpdate(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::Entity const& /*entity*/) noexcept override
		{
			try
			{
				std::stringstream ss;
				ss << std::hex << "### Unit updated (" << la::avdecc::toHexString(entityID, true) << ")" << std::endl;
				outputText(ss.str());
			}
			catch (...)
			{
				outputText("Uncaught exception in onEntityUpdate");
			}
		}

		// Result handlers
		/* Enumeration and Control Protocol (AECP) */
		void onEntityAvailableResult(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept
		{
			try
			{
				std::stringstream ss;
				ss << std::hex << "Unit available status (" << la::avdecc::toHexString(entityID, true) << "): " << la::avdecc::entity::ControllerEntity::statusToString(status) << std::endl;
				outputText(ss.str());
			}
			catch (...)
			{
				outputText("Uncaught exception in onEntityAvailableResult");
			}
		}
		void onEntityAcquireResult(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const /*owningEntity*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept
		{
			try
			{
				std::stringstream ss;
				ss << std::hex << "Unit acquire status (" << la::avdecc::toHexString(entityID, true) << "): " << la::avdecc::entity::ControllerEntity::statusToString(status) << std::endl;
				if (!!status)
				{
					if (entityID == _talker)
						_talkerAcquired = true;
					if (entityID == _listener)
						_listenerAcquired = true;
				}
				outputText(ss.str());
			}
			catch (...)
			{
				outputText("Uncaught exception in onEntityAcquireResult");
			}
		}
		void onEntityReleaseResult(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept
		{
			try
			{
				std::stringstream ss;
				ss << std::hex << "Unit release status (" << la::avdecc::toHexString(entityID, true) << "): " << la::avdecc::entity::ControllerEntity::statusToString(status) << std::endl;
				outputText(ss.str());
			}
			catch (...)
			{
				outputText("Uncaught exception in onEntityReleaseResult");
			}
		}
		void onEntityDescriptorResult(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::EntityDescriptor const& descriptor) noexcept
		{
			try
			{
				std::stringstream ss;
				ss << std::hex << "Entity descriptor status (" << la::avdecc::toHexString(entityID, true) << "): " << la::avdecc::entity::ControllerEntity::statusToString(status) << std::endl;
				if (!!status)
				{
					ss << "Unit name: " << descriptor.entityName << std::endl;
				}

				if (descriptor.entityName == std::string("macMini AVB Talker"))
				{
					_talker = entityID;
					controller->acquireEntity(entityID, false, la::avdecc::entity::model::DescriptorType::Entity, 0u, std::bind(&ControllerDelegate::onEntityAcquireResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
					_talkerConfiguration = descriptor.currentConfiguration;
					controller->readConfigurationDescriptor(entityID, _talkerConfiguration, std::bind(&ControllerDelegate::onConfigurationDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
				}
				if (descriptor.entityName == std::string("LA12X"))
				{
					_listener = entityID;
					controller->acquireEntity(entityID, false, la::avdecc::entity::model::DescriptorType::Entity, 0u, std::bind(&ControllerDelegate::onEntityAcquireResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
					_listenerConfiguration = descriptor.currentConfiguration;
					controller->readConfigurationDescriptor(entityID, _listenerConfiguration, std::bind(&ControllerDelegate::onConfigurationDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
				}

				outputText(ss.str());
			}
			catch (...)
			{
				outputText("Uncaught exception in onEntityDescriptorResult");
			}
		}
		void onConfigurationDescriptorResult(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ConfigurationDescriptor const& descriptor) noexcept
		{
			try
			{
				std::stringstream ss;
				ss << std::hex << "Configuration descriptor status (" << la::avdecc::toHexString(entityID, true) << "): " << la::avdecc::entity::ControllerEntity::statusToString(status) << std::endl;
				if (!!status)
				{
					if (entityID == _talker)
					{
						// Read output streams
						la::avdecc::entity::model::StreamIndex count{ 0 };
						auto countIt = descriptor.descriptorCounts.find(la::avdecc::entity::model::DescriptorType::StreamOutput);
						if (countIt != descriptor.descriptorCounts.end())
							count = la::avdecc::entity::model::StreamIndex(countIt->second);
						ss << std::dec << "Talker configuration " << configurationIndex << " has " << count << " OUTPUT STREAMS" << std::endl;
						for (auto index = la::avdecc::entity::model::StreamIndex(0); index < count; ++index)
							controller->readStreamOutputDescriptor(entityID, _talkerConfiguration, index, std::bind(&ControllerDelegate::onStreamOutputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
					}
					if (entityID == _listener)
					{
						// Read locales
						{
							std::uint16_t count{ 0 };
							auto countIt = descriptor.descriptorCounts.find(la::avdecc::entity::model::DescriptorType::Locale);
							if (countIt != descriptor.descriptorCounts.end())
								count = countIt->second;
							ss << std::dec << "Listener configuration '" << descriptor.objectName << "' has " << count << " LOCALES" << std::endl;
							for (auto index = la::avdecc::entity::model::LocaleIndex(0); index < count; ++index)
								controller->readLocaleDescriptor(entityID, _listenerConfiguration, index, [this](la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::LocaleIndex const localeIndex, la::avdecc::entity::model::LocaleDescriptor const& descriptor)
							{
								if (!!status)
								{
									std::stringstream ss;
									ss << "Locales for index " << localeIndex << ": " << descriptor.numberOfStringDescriptors << " string descriptors (start at offset " << descriptor.baseStringDescriptorIndex << ")" << std::endl;
									outputText(ss.str());
									for (auto stringDescriptorIndex = la::avdecc::entity::model::StringsIndex(0); stringDescriptorIndex < descriptor.numberOfStringDescriptors; ++stringDescriptorIndex)
									{
										controller->readStringsDescriptor(entityID, _listenerConfiguration, descriptor.baseStringDescriptorIndex + stringDescriptorIndex, [localeIdentifier = descriptor.localeID](la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StringsIndex const stringsIndex, la::avdecc::entity::model::StringsDescriptor const& descriptor)
										{
											std::stringstream ss;
											if (!!status)
											{
												for (auto strIndex = 0u; strIndex < descriptor.strings.size(); ++strIndex)
												{
													ss << "String " << (stringsIndex * descriptor.strings.size()) + strIndex << " locale " << localeIdentifier << ": " << descriptor.strings[strIndex] << std::endl;
												}
											}
											else
											{
												ss << "Error getting strings descriptor " << stringsIndex << ": " << la::avdecc::to_integral(status) << std::endl;
											}
											outputText(ss.str());
										});
									}
								}
							});
						}
						// Read input streams
						{
							la::avdecc::entity::model::StreamIndex count{ 0 };
							auto countIt = descriptor.descriptorCounts.find(la::avdecc::entity::model::DescriptorType::StreamInput);
							if (countIt != descriptor.descriptorCounts.end())
								count = la::avdecc::entity::model::StreamIndex(countIt->second);
							ss << std::dec << "Listener configuration '" << descriptor.objectName << "' has " << count << " INPUT STREAMS" << std::endl;
							for (auto index = la::avdecc::entity::model::StreamIndex(0); index < count; ++index)
								controller->readStreamInputDescriptor(entityID, _listenerConfiguration, index, std::bind(&ControllerDelegate::onStreamInputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
						}
					}
				}
				outputText(ss.str());
			}
			catch (...)
			{
				outputText("Uncaught exception in onConfigurationDescriptorResult");
			}
		}
		void onStreamInputDescriptorResult(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamDescriptor const& descriptor) noexcept
		{
			try
			{
				if (!!status)
				{
					std::stringstream ss;
					ss << "Stream input for index " << streamIndex << ": " << descriptor.objectName << std::endl;
					outputText(ss.str());
				}
			}
			catch (...)
			{
				outputText("Uncaught exception in onStreamInputDescriptorResult");
			}
		}
		void onStreamOutputDescriptorResult(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamDescriptor const& descriptor) noexcept
		{
			try
			{
				if (!!status)
				{
					std::stringstream ss;
					ss << "Stream output for index " << streamIndex << ": " << descriptor.objectName << std::endl;
					outputText(ss.str());
				}
			}
			catch (...)
			{
				outputText("Uncaught exception in onStreamOutputDescriptorResult");
			}
		}
		/* Connection Management Protocol sniffed messages (ACMP) */
		virtual void onControllerConnectResponseSniffed(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept override
		{
			try
			{
				std::stringstream ss;
				ss << std::hex << "Stream connect status (" << la::avdecc::toHexString(listenerStream.entityID, true) << " -> " << la::avdecc::toHexString(talkerStream.entityID, true) << "): " << la::avdecc::entity::ControllerEntity::statusToString(status) << std::endl;
				outputText(ss.str());
			}
			catch (...)
			{
				outputText("Uncaught exception in onConnectStreamSniffed");
			}
		}
		virtual void onControllerDisconnectResponseSniffed(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept override
		{
			try
			{
				std::stringstream ss;
				ss << std::hex << "Stream disconnect status (" << la::avdecc::toHexString(listenerStream.entityID, true) << " -> " << la::avdecc::toHexString(talkerStream.entityID, true) << "): " << la::avdecc::entity::ControllerEntity::statusToString(status) << std::endl;
				outputText(ss.str());
			}
			catch (...)
			{
				outputText("Uncaught exception in onDisconnectStreamSniffed");
			}
		}
		virtual void onGetListenerStreamStateResponseSniffed(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const /*status*/) noexcept override
		{
			try
			{
			}
			catch (...)
			{
				outputText("Uncaught exception in onGetListenerStreamStateSniffed");
			}
		}



	private:
		la::avdecc::UniqueIdentifier _talker{ 0 };
		la::avdecc::entity::model::ConfigurationIndex _talkerConfiguration{ 0 };
		bool _talkerAcquired{ false };
		la::avdecc::UniqueIdentifier _listener{ 0 };
		la::avdecc::entity::model::ConfigurationIndex _listenerConfiguration{ 0 };
		bool _listenerAcquired{ false };
		//bool _connected{ false };
	};

	auto const protocolInterfaceType = chooseProtocolInterfaceType();
	auto intfc = chooseNetworkInterface();

	if (intfc.type == la::avdecc::networkInterface::Interface::Type::None || protocolInterfaceType == la::avdecc::protocol::ProtocolInterface::Type::None)
	{
		return 1;
	}

	try
	{
		outputText("Selected interface '" + intfc.alias + "' and protocol interface '" + la::avdecc::protocol::ProtocolInterface::typeToString(protocolInterfaceType) + "':\n");
		auto endPoint = la::avdecc::EndStation::create(protocolInterfaceType, intfc.name);
		ControllerDelegate controllerDelegate;

		// Register log observer
		la::avdecc::logger::Logger::getInstance().registerObserver(&controllerDelegate);
		// Set default log level
		la::avdecc::logger::Logger::getInstance().setLevel(la::avdecc::logger::Level::Trace);

		auto* controller = endPoint->addControllerEntity(0x0002, la::avdecc::entity::model::makeEntityModelID(VENDOR_ID, DEVICE_ID, MODEL_ID), &controllerDelegate);

		// Try to start entity advertisement
		if (!controller->enableEntityAdvertising(10))
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
