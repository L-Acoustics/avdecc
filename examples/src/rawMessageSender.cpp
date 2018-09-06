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
* @file rawMessageSender.cpp
* @author Christophe Calmejane
*/

/** ************************************************************************ **/
/** Example sending a raw message using a ProtocolInterface                  **/
/** ************************************************************************ **/

#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/utils.hpp>
#include <la/avdecc/networkInterfaceHelper.hpp>
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
	class Observer : public la::avdecc::protocol::ProtocolInterface::Observer
	{
	private:
		// la::avdecc::protocol::ProtocolInterface::Observer overrides
		virtual void onTransportError(la::avdecc::protocol::ProtocolInterface* const /*pi*/) noexcept override
		{
		}
		virtual void onLocalEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::DiscoveredEntity const& /*entity*/) noexcept override
		{
		}
		virtual void onLocalEntityOffline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept override
		{
		}
		virtual void onLocalEntityUpdated(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::DiscoveredEntity const& /*entity*/) noexcept override
		{
		}
		virtual void onRemoteEntityOnline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::DiscoveredEntity const& /*entity*/) noexcept override
		{
		}
		virtual void onRemoteEntityOffline(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept override
		{
		}
		virtual void onRemoteEntityUpdated(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::DiscoveredEntity const& /*entity*/) noexcept override
		{
		}
		virtual void onAecpCommand(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::LocalEntity const& /*entity*/, la::avdecc::protocol::Aecpdu const& /*aecpdu*/) noexcept override
		{
		}
		virtual void onAecpUnsolicitedResponse(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::LocalEntity const& /*entity*/, la::avdecc::protocol::Aecpdu const& /*aecpdu*/) noexcept override
		{
		}
		virtual void onAcmpSniffedCommand(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::LocalEntity const& /*entity*/, la::avdecc::protocol::Acmpdu const& /*acmpdu*/) noexcept override
		{
		}
		virtual void onAcmpSniffedResponse(la::avdecc::protocol::ProtocolInterface* const /*pi*/, la::avdecc::entity::LocalEntity const& /*entity*/, la::avdecc::protocol::Acmpdu const& /*acmpdu*/) noexcept override
		{
		}

		DECLARE_AVDECC_OBSERVER_GUARD(Observer);
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
		auto pi = la::avdecc::protocol::ProtocolInterface::create(protocolInterfaceType, intfc.name);
		Observer observer;
		pi->registerObserver(&observer);

		// Send a raw message (Entity Available message)
		auto adpdu = la::avdecc::protocol::Adpdu::create();

		// Set Ether2 fields
		adpdu->setSrcAddress(pi->getMacAddress());
		adpdu->setDestAddress(la::avdecc::protocol::Adpdu::Multicast_Mac_Address);
		// Set ADP fields
		adpdu->setMessageType(la::avdecc::protocol::AdpMessageType::EntityAvailable);
		adpdu->setValidTime(10);
		adpdu->setEntityID(0x0102030405060708);
		adpdu->setEntityModelID(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier());
		adpdu->setEntityCapabilities(la::avdecc::entity::EntityCapabilities::None);
		adpdu->setTalkerStreamSources(0u);
		adpdu->setTalkerCapabilities(la::avdecc::entity::TalkerCapabilities::None);
		adpdu->setListenerStreamSinks(0u);
		adpdu->setListenerCapabilities(la::avdecc::entity::ListenerCapabilities::None);
		adpdu->setControllerCapabilities(la::avdecc::entity::ControllerCapabilities::Implemented);
		adpdu->setAvailableIndex(0);
		adpdu->setGptpGrandmasterID(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier());
		adpdu->setGptpDomainNumber(0u);
		adpdu->setIdentifyControlIndex(0u);
		adpdu->setInterfaceIndex(0);
		adpdu->setAssociationID(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier());

		// Send the message
		pi->sendAdpMessage(std::move(adpdu));

		std::this_thread::sleep_for(std::chrono::seconds(1));

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
