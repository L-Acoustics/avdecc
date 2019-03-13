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
* @file rawMessageSender.cpp
* @author Christophe Calmejane
*/

/** ************************************************************************ **/
/** Example sending raw messages using a ProtocolInterface (very low level)  **/
/** ************************************************************************ **/

#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/utils.hpp>
#include <la/avdecc/networkInterfaceHelper.hpp>
#include <la/avdecc/internals/protocolMvuAecpdu.hpp>

#include "utils.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <future>

static auto const s_TargetEntityID = la::avdecc::UniqueIdentifier{ 0x001b92fffe01b930 };
static auto const s_ListenerEntityID = la::avdecc::UniqueIdentifier{ 0x001b92fffe01b930 };
static auto const s_TalkerEntityID = la::avdecc::UniqueIdentifier{ 0x1b92fffe02233b };
static auto const s_TargetMacAddress = la::avdecc::networkInterface::MacAddress{ 0x00, 0x1b, 0x92, 0x01, 0xb9, 0x30 };

void sendRawMessages(la::avdecc::protocol::ProtocolInterface& pi)
{
	// Send raw ADP message (Entity Available message)
	{
		auto adpdu = la::avdecc::protocol::Adpdu{};

		// Set Ether2 fields
		adpdu.setSrcAddress(pi.getMacAddress());
		adpdu.setDestAddress(la::avdecc::protocol::Adpdu::Multicast_Mac_Address);
		// Set ADP fields
		adpdu.setMessageType(la::avdecc::protocol::AdpMessageType::EntityAvailable);
		adpdu.setValidTime(10);
		adpdu.setEntityID(0x0102030405060708);
		adpdu.setEntityModelID(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier());
		adpdu.setEntityCapabilities({});
		adpdu.setTalkerStreamSources(0u);
		adpdu.setTalkerCapabilities({});
		adpdu.setListenerStreamSinks(0u);
		adpdu.setListenerCapabilities({});
		adpdu.setControllerCapabilities(la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented });
		adpdu.setAvailableIndex(0);
		adpdu.setGptpGrandmasterID(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier());
		adpdu.setGptpDomainNumber(0u);
		adpdu.setIdentifyControlIndex(0u);
		adpdu.setInterfaceIndex(0);
		adpdu.setAssociationID(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier());

		// Send the message
		pi.sendAdpMessage(adpdu);
	}

	// Send raw ACMP message (Connect Stream Command)
	{
		auto acmpdu = la::avdecc::protocol::Acmpdu{};

		// Set Ether2 fields
		acmpdu.setSrcAddress(pi.getMacAddress());
		acmpdu.setDestAddress(la::avdecc::protocol::Acmpdu::Multicast_Mac_Address);
		// Set ACMP fields
		acmpdu.setMessageType(la::avdecc::protocol::AcmpMessageType::ConnectRxCommand);
		acmpdu.setStatus(la::avdecc::protocol::AcmpStatus::Success);
		acmpdu.setControllerEntityID(0x0af700048902f1);
		acmpdu.setTalkerEntityID(s_TalkerEntityID);
		acmpdu.setListenerEntityID(s_ListenerEntityID);
		acmpdu.setTalkerUniqueID(0u);
		acmpdu.setListenerUniqueID(0u);
		acmpdu.setStreamDestAddress(la::avdecc::networkInterface::MacAddress{});
		acmpdu.setConnectionCount(0u);
		acmpdu.setSequenceID(0u);
		acmpdu.setFlags(la::avdecc::entity::ConnectionFlags{ la::avdecc::entity::ConnectionFlag::StreamingWait });
		acmpdu.setStreamVlanID(0u);

		// Send the message
		pi.sendAcmpMessage(acmpdu);
	}

	// Send raw AECP message (Acquire Command)
	{
		auto aecpdu = la::avdecc::protocol::AemAecpdu{ false };
		la::avdecc::protocol::SerializationBuffer buffer;

		// Manually fill the AEM payload
		buffer << static_cast<std::uint32_t>(0u /* Acquire Flags */) << static_cast<std::uint64_t>(0u /* Owner */) << static_cast<std::uint16_t>(0u /* DescriptorType */) << static_cast<std::uint16_t>(0u /* DescriptorIndex */); // Acquire payload

		// Set Ether2 fields
		aecpdu.setSrcAddress(pi.getMacAddress());
		aecpdu.setDestAddress(s_TargetMacAddress);
		// Set AECP fields
		aecpdu.setStatus(la::avdecc::protocol::AemAecpStatus::Success);
		aecpdu.setTargetEntityID(s_TargetEntityID);
		aecpdu.setControllerEntityID(0x0af700048902f1);
		aecpdu.setSequenceID(0u);
		// Set AEM fields
		aecpdu.setUnsolicited(false);
		aecpdu.setCommandType(la::avdecc::protocol::AemCommandType::AcquireEntity);
		aecpdu.setCommandSpecificData(buffer.data(), buffer.size());

		// Send the message
		pi.sendAecpMessage(aecpdu);
	}
}

void sendControllerCommands(la::avdecc::protocol::ProtocolInterface& pi)
{
	// In order to be allowed to send Commands, we have to declare ourself as a LocalEntity
	auto const commonInformation{ la::avdecc::entity::Entity::CommonInformation{ la::avdecc::entity::Entity::generateEID(pi.getMacAddress(), 0x0005), la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), la::avdecc::entity::EntityCapabilities{}, 0u, la::avdecc::entity::TalkerCapabilities{}, 0u, la::avdecc::entity::ListenerCapabilities{}, la::avdecc::entity::ControllerCapabilities{ la::avdecc::entity::ControllerCapability::Implemented }, std::nullopt, std::nullopt } };
	auto const interfaceInfo{ la::avdecc::entity::Entity::InterfaceInformation{ pi.getMacAddress(), 31u, 0u, std::nullopt, std::nullopt } };
	auto entity = la::avdecc::entity::ControllerEntity::create(&pi, commonInformation, la::avdecc::entity::Entity::InterfacesInformation{ { la::avdecc::entity::Entity::GlobalAvbInterfaceIndex, interfaceInfo } }, nullptr);

	entity->setControllerDelegate(nullptr);

	// Send ACMP command (Disconnect Stream)
	{
		auto acmpdu = la::avdecc::protocol::Acmpdu::create();

		// Set Ether2 fields
		acmpdu->setSrcAddress(pi.getMacAddress());
		acmpdu->setDestAddress(la::avdecc::protocol::Acmpdu::Multicast_Mac_Address);
		// Set ACMP fields
		acmpdu->setMessageType(la::avdecc::protocol::AcmpMessageType::DisconnectRxCommand);
		acmpdu->setStatus(la::avdecc::protocol::AcmpStatus::Success);
		acmpdu->setControllerEntityID(entity->getEntityID());
		acmpdu->setTalkerEntityID(s_TalkerEntityID);
		acmpdu->setListenerEntityID(s_ListenerEntityID);
		acmpdu->setTalkerUniqueID(0u);
		acmpdu->setListenerUniqueID(0u);
		acmpdu->setStreamDestAddress(la::avdecc::networkInterface::MacAddress{});
		acmpdu->setConnectionCount(0u);
		acmpdu->setSequenceID(666); // Not necessary, it's set by the ProtocolInterface layer
		acmpdu->setFlags({});
		acmpdu->setStreamVlanID(0u);

		// Send the message
		std::promise<void> commandResultPromise{};
		auto const error = pi.sendAcmpCommand(std::move(acmpdu),
			[&commandResultPromise](la::avdecc::protocol::Acmpdu const* const /*response*/, la::avdecc::protocol::ProtocolInterface::Error const error)
			{
				commandResultPromise.set_value();
				outputText("Got ACMP response with status: " + std::to_string(la::avdecc::utils::to_integral(error)) + "\n");
			});
		if (!!error)
		{
			outputText("Error sending ACMP command: " + std::to_string(la::avdecc::utils::to_integral(error)) + "\n");
		}
		else
		{
			// Wait for the command result
			auto status = commandResultPromise.get_future().wait_for(std::chrono::seconds(20));
			if (status == std::future_status::timeout)
			{
				outputText("ACMP command timed out\n");
			}
		}
	}

	// Send AEM-AECP command (Acquire Command)
	{
		auto aecpdu = la::avdecc::protocol::AemAecpdu::create(false);
		la::avdecc::protocol::SerializationBuffer buffer;

		// Manually fill the AEM payload
		buffer << static_cast<std::uint32_t>(0u /* Acquire Flags */) << static_cast<std::uint64_t>(0u /* Owner */) << static_cast<std::uint16_t>(0u /* DescriptorType */) << static_cast<std::uint16_t>(0u /* DescriptorIndex */); // Acquire payload

		// Set Ether2 fields
		aecpdu->setSrcAddress(pi.getMacAddress());
		aecpdu->setDestAddress(s_TargetMacAddress);
		// Set AECP fields
		aecpdu->setStatus(la::avdecc::protocol::AemAecpStatus::Success);
		aecpdu->setTargetEntityID(s_TargetEntityID);
		aecpdu->setControllerEntityID(entity->getEntityID());
		aecpdu->setSequenceID(666); // Not necessary, it's set by the ProtocolInterface layer
		// Set AEM fields
		auto& aem = static_cast<la::avdecc::protocol::AemAecpdu&>(*aecpdu);
		aem.setUnsolicited(false);
		aem.setCommandType(la::avdecc::protocol::AemCommandType::AcquireEntity);
		aem.setCommandSpecificData(buffer.data(), buffer.size());

		// Send the message
		std::promise<void> commandResultPromise{};
		auto const error = pi.sendAecpCommand(std::move(aecpdu),
			[&commandResultPromise](la::avdecc::protocol::Aecpdu const* const /*response*/, la::avdecc::protocol::ProtocolInterface::Error const error)
			{
				commandResultPromise.set_value();
				outputText("Got AEM-AECP response with status: " + std::to_string(la::avdecc::utils::to_integral(error)) + "\n");
			});
		if (!!error)
		{
			outputText("Error sending AEM-AECP command: " + std::to_string(la::avdecc::utils::to_integral(error)) + "\n");
		}
		else
		{
			// Wait for the command result
			auto status = commandResultPromise.get_future().wait_for(std::chrono::seconds(20));
			if (status == std::future_status::timeout)
			{
				outputText("AEM-AECP command timed out\n");
			}
		}
	}

	// Send MVU-AECP command (Get Milan Info)
	{
		auto aecpdu = la::avdecc::protocol::MvuAecpdu::create(false);

		// Set Ether2 fields
		aecpdu->setSrcAddress(pi.getMacAddress());
		aecpdu->setDestAddress(s_TargetMacAddress);
		// Set AECP fields
		aecpdu->setStatus(la::avdecc::protocol::AecpStatus::Success);
		aecpdu->setTargetEntityID(s_TargetEntityID);
		aecpdu->setControllerEntityID(entity->getEntityID());
		aecpdu->setSequenceID(666); // Not necessary, it's set by the ProtocolInterface layer
		// Set MVU fields
		auto& mvu = static_cast<la::avdecc::protocol::MvuAecpdu&>(*aecpdu);
		mvu.setCommandType(la::avdecc::protocol::MvuCommandType::GetMilanInfo);
		auto const reserved = std::uint16_t{ 0u };
		mvu.setCommandSpecificData(&reserved, sizeof(reserved));

		// Send the message
		std::promise<void> commandResultPromise{};
		auto const error = pi.sendAecpCommand(std::move(aecpdu),
			[&commandResultPromise](la::avdecc::protocol::Aecpdu const* const /*response*/, la::avdecc::protocol::ProtocolInterface::Error const error)
			{
				commandResultPromise.set_value();
				outputText("Got MVU-AECP response with status: " + std::to_string(la::avdecc::utils::to_integral(error)) + "\n");
			});
		if (!!error)
		{
			outputText("Error sending MVU-AECP command: " + std::to_string(la::avdecc::utils::to_integral(error)) + "\n");
		}
		else
		{
			// Wait for the command result
			auto status = commandResultPromise.get_future().wait_for(std::chrono::seconds(20));
			if (status == std::future_status::timeout)
			{
				outputText("MVU-AECP command timed out\n");
			}
		}
	}
}


int doJob()
{
	//class Observer : public la::avdecc::protocol::ProtocolInterface::Observer
	//{
	//private:
	//	// la::avdecc::protocol::ProtocolInterface::Observer overrides
	//	virtual void onTransportError(la::avdecc::protocol::ProtocolInterface* const /*pi*/) noexcept override {}
	//	virtual void onLocalEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& /*entity*/) noexcept override {}
	//	virtual void onLocalEntityOffline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept override {}
	//	virtual void onLocalEntityUpdated(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& /*entity*/) noexcept override {}
	//	virtual void onRemoteEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& /*entity*/) noexcept override {}
	//	virtual void onRemoteEntityOffline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept override {}
	//	virtual void onRemoteEntityUpdated(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::Entity const& /*entity*/) noexcept override {}
	//	virtual void onAecpCommand(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::LocalEntity const& /*entity*/, la::avdecc::protocol::Aecpdu const& /*aecpdu*/) noexcept override {}
	//	virtual void onAecpUnsolicitedResponse(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::LocalEntity const& /*entity*/, la::avdecc::protocol::Aecpdu const& /*aecpdu*/) noexcept override {}
	//	virtual void onAcmpSniffedCommand(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::LocalEntity const& /*entity*/, la::avdecc::protocol::Acmpdu const& /*acmpdu*/) noexcept override {}
	//	virtual void onAcmpSniffedResponse(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::LocalEntity const& /*entity*/, la::avdecc::protocol::Acmpdu const& /*acmpdu*/) noexcept override {}

	//	DECLARE_AVDECC_OBSERVER_GUARD(Observer);
	//};

	auto const protocolInterfaceType = chooseProtocolInterfaceType();
	auto intfc = chooseNetworkInterface();

	if (intfc.type == la::avdecc::networkInterface::Interface::Type::None || protocolInterfaceType == la::avdecc::protocol::ProtocolInterface::Type::None)
	{
		return 1;
	}

	try
	{
		outputText("Selected interface '" + intfc.alias + "' and protocol interface '" + la::avdecc::protocol::ProtocolInterface::typeToString(protocolInterfaceType) + "':\n");
		auto pi = la::avdecc::protocol::ProtocolInterface::create(protocolInterfaceType, intfc.name);

		// Test sending raw messages
		sendRawMessages(*pi);

		// Test sending controller type messages (commands)
		sendControllerCommands(*pi);

		pi->shutdown();
	}
	catch (la::avdecc::protocol::ProtocolInterface::Exception const& e)
	{
		outputText(std::string("Cannot create ProtocolInterface: ") + e.what() + "\n");
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
