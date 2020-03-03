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
* @file endpointEntityImpl.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/endpointEntity.hpp"
#include "la/avdecc/internals/protocolInterface.hpp"
#include "la/avdecc/internals/protocolAemAecpdu.hpp"
#include "la/avdecc/internals/protocolAaAecpdu.hpp"
#include "la/avdecc/internals/protocolMvuAecpdu.hpp"

#include "entityImpl.hpp"

#include <unordered_map>
#include <functional>
#include <thread>
#include <mutex>

namespace la
{
namespace avdecc
{
namespace entity
{
/* ************************************************************************** */
/* EndpointEntityImpl                                                         */
/* ************************************************************************** */
class EndpointEntityImpl : public LocalEntityImpl<EndpointEntity>
{
private:
	friend class LocalEntityGuard<EndpointEntityImpl>; // The only way to construct EndpointEntityImpl, through LocalEntityGuard

	/* ************************************************************************** */
	/* EndpointEntityImpl life cycle                                              */
	/* ************************************************************************** */
	EndpointEntityImpl(protocol::ProtocolInterface* const protocolInterface, CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation, endpoint::Delegate* const endpointDelegate);
	virtual ~EndpointEntityImpl() noexcept;
	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept override;

private:
	/* ************************************************************************** */
	/* endpoint::Interface overrides                                              */
	/* ************************************************************************** */
	/* Discovery Protocol (ADP) */
	/* Enumeration and Control Protocol (AECP) AEM */
	virtual void queryEntityAvailable(UniqueIdentifier const targetEntityID, networkInterface::MacAddress const& targetMacAddress, QueryEntityAvailableHandler const& handler) const noexcept override;
	virtual void queryControllerAvailable(UniqueIdentifier const targetEntityID, networkInterface::MacAddress const& targetMacAddress, QueryControllerAvailableHandler const& handler) const noexcept override;
	/* Enumeration and Control Protocol (AECP) AA */
	//virtual void addressAccess(UniqueIdentifier const targetEntityID, addressAccess::Tlvs const& tlvs, AddressAccessHandler const& handler) const noexcept override;
	/* Connection Management Protocol (ACMP) */
	//virtual void connectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept override;
	//virtual void disconnectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept override;
	//virtual void disconnectTalkerStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept override;
	//virtual void getTalkerStreamState(model::StreamIdentification const& talkerStream, GetTalkerStreamStateHandler const& handler) const noexcept override;
	//virtual void getListenerStreamState(model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept override;
	//virtual void getTalkerStreamConnection(model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex, GetTalkerStreamConnectionHandler const& handler) const noexcept override;

	virtual void sendAemAecpResponse(protocol::AemAecpdu const& aemAecpduCommand, protocol::AecpStatus const status, protocol::AemAecpdu::Payload const& payload) const noexcept override;

	/* ************************************************************************** */
	/* endpoint::Endpoint overrides                                               */
	/* ************************************************************************** */
	/* Other methods */
	virtual void setEndpointDelegate(endpoint::Delegate* const delegate) noexcept override;

	/* ************************************************************************** */
	/* protocol::ProtocolInterface::Observer overrides                            */
	/* ************************************************************************** */
	/* **** Global notifications **** */
	virtual void onTransportError(protocol::ProtocolInterface* const pi) noexcept override;
	/* **** Discovery notifications **** */
	virtual void onLocalEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	virtual void onLocalEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept override;
	virtual void onLocalEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	virtual void onRemoteEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	virtual void onRemoteEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept override;
	virtual void onRemoteEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	/* **** AECP notifications **** */
	// virtual void onAecpCommand(protocol::ProtocolInterface* const pi, LocalEntity const& entity, protocol::Aecpdu const& aecpdu) noexcept override; Already defined in base class LocalEntityImpl, dispatching through onUnhandledAecpCommand
	virtual void onAecpAemUnsolicitedResponse(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aecpdu) noexcept override;
	virtual void onAecpAemIdentifyNotification(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aecpdu) noexcept override;
	/* **** ACMP notifications **** */
	virtual void onAcmpCommand(protocol::ProtocolInterface* const pi, protocol::Acmpdu const& acmpdu) noexcept override;
	virtual void onAcmpResponse(protocol::ProtocolInterface* const pi, protocol::Acmpdu const& acmpdu) noexcept override;

	/* ************************************************************************** */
	/* LocalEntityImpl overrides                                                  */
	/* ************************************************************************** */
	virtual bool onUnhandledAecpCommand(protocol::ProtocolInterface* const pi, protocol::Aecpdu const& aecpdu) noexcept override;
	virtual bool onUnhandledAecpVuCommand(protocol::ProtocolInterface* const pi, protocol::VuAecpdu::ProtocolIdentifier const& protocolIdentifier, protocol::Aecpdu const& aecpdu) noexcept override;

	/* ************************************************************************** */
	/* Internal variables                                                         */
	/* ************************************************************************** */
	CapabilityDelegate::UniquePointer _endpointCapabilityDelegate{ nullptr };
};

} // namespace entity
} // namespace avdecc
} // namespace la
