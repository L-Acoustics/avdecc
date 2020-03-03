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
* @file endpointEntityImpl.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/utils.hpp"

#include "logHelper.hpp"
#include "endpointEntityImpl.hpp"
#include "endpointCapabilityDelegate.hpp"

#include <exception>
#include <cassert>
#include <chrono>
#include <unordered_map>
#include <algorithm>

namespace la
{
namespace avdecc
{
namespace entity
{
/* ************************************************************************** */
/* EndpointEntityImpl life cycle                                              */
/* ************************************************************************** */
EndpointEntityImpl::EndpointEntityImpl(protocol::ProtocolInterface* const protocolInterface, CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation, endpoint::Delegate* const endpointDelegate)
	: LocalEntityImpl(protocolInterface, commonInformation, interfacesInformation)
{
	// Entity is endpoint capable
	_endpointCapabilityDelegate = std::make_unique<endpoint::CapabilityDelegate>(getProtocolInterface(), endpointDelegate, *this, getEntityID());

	// Register observer
	getProtocolInterface()->registerObserver(this);
}

EndpointEntityImpl::~EndpointEntityImpl() noexcept
{
	// Unregister ourself as a ProtocolInterface observer
	invokeProtectedMethod(&protocol::ProtocolInterface::unregisterObserver, getProtocolInterface(), this);

	// Remove endpoint capability delegate
	_endpointCapabilityDelegate.reset();
}

void EndpointEntityImpl::destroy() noexcept
{
	delete this;
}

/* ************************************************************************** */
/* EndpointEntity overrides                                                   */
/* ************************************************************************** */
/* Discovery Protocol (ADP) */

/* Enumeration and Control Protocol (AECP) AEM */
void EndpointEntityImpl::queryEntityAvailable(UniqueIdentifier const targetEntityID, networkInterface::MacAddress const& targetMacAddress, QueryEntityAvailableHandler const& handler) const noexcept
{
	static_cast<endpoint::CapabilityDelegate&>(*_endpointCapabilityDelegate).queryEntityAvailable(targetEntityID, targetMacAddress, handler);
}

void EndpointEntityImpl::queryControllerAvailable(UniqueIdentifier const targetEntityID, networkInterface::MacAddress const& targetMacAddress, QueryControllerAvailableHandler const& handler) const noexcept
{
	static_cast<endpoint::CapabilityDelegate&>(*_endpointCapabilityDelegate).queryControllerAvailable(targetEntityID, targetMacAddress, handler);
}

/* Enumeration and Control Protocol (AECP) AA */
//void EndpointEntityImpl::addressAccess(UniqueIdentifier const targetEntityID, addressAccess::Tlvs const& tlvs, AddressAccessHandler const& handler) const noexcept
//{
//	static_cast<endpoint::CapabilityDelegate&>(*_endpointCapabilityDelegate).addressAccess(targetEntityID, tlvs, handler);
//}

/* Connection Management Protocol (ACMP) */
//void EndpointEntityImpl::connectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept
//{
//	static_cast<endpoint::CapabilityDelegate&>(*_endpointCapabilityDelegate).connectStream(talkerStream, listenerStream, handler);
//}
//
//void EndpointEntityImpl::disconnectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept
//{
//	static_cast<endpoint::CapabilityDelegate&>(*_endpointCapabilityDelegate).disconnectStream(talkerStream, listenerStream, handler);
//}
//
//void EndpointEntityImpl::disconnectTalkerStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept
//{
//	static_cast<endpoint::CapabilityDelegate&>(*_endpointCapabilityDelegate).disconnectTalkerStream(talkerStream, listenerStream, handler);
//}
//
//void EndpointEntityImpl::getTalkerStreamState(model::StreamIdentification const& talkerStream, GetTalkerStreamStateHandler const& handler) const noexcept
//{
//	static_cast<endpoint::CapabilityDelegate&>(*_endpointCapabilityDelegate).getTalkerStreamState(talkerStream, handler);
//}
//
//void EndpointEntityImpl::getListenerStreamState(model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept
//{
//	static_cast<endpoint::CapabilityDelegate&>(*_endpointCapabilityDelegate).getListenerStreamState(listenerStream, handler);
//}
//
//void EndpointEntityImpl::getTalkerStreamConnection(model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex, GetTalkerStreamConnectionHandler const& handler) const noexcept
//{
//	static_cast<endpoint::CapabilityDelegate&>(*_endpointCapabilityDelegate).getTalkerStreamConnection(talkerStream, connectionIndex, handler);
//}

void EndpointEntityImpl::sendAemAecpResponse(protocol::AemAecpdu const& aemAecpduCommand, protocol::AecpStatus const status, protocol::AemAecpdu::Payload const& payload) const noexcept
{
	LocalEntityImpl<>::sendAemAecpResponse(getProtocolInterface(), aemAecpduCommand, status, payload.first, payload.second);
}

/* ************************************************************************** */
/* EndpointEntity overrides                                                   */
/* ************************************************************************** */
void EndpointEntityImpl::setEndpointDelegate(endpoint::Delegate* const delegate) noexcept
{
	static_cast<endpoint::CapabilityDelegate&>(*_endpointCapabilityDelegate).setEndpointDelegate(delegate);
}

/* ************************************************************************** */
/* protocol::ProtocolInterface::Observer overrides                            */
/* ************************************************************************** */
/* **** Global notifications **** */
void EndpointEntityImpl::onTransportError(protocol::ProtocolInterface* const pi) noexcept
{
	_endpointCapabilityDelegate->onTransportError(pi);
}

/* **** Discovery notifications **** */
void EndpointEntityImpl::onLocalEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	_endpointCapabilityDelegate->onLocalEntityOnline(pi, entity);
}

void EndpointEntityImpl::onLocalEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept
{
	_endpointCapabilityDelegate->onLocalEntityOffline(pi, entityID);
}

void EndpointEntityImpl::onLocalEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	_endpointCapabilityDelegate->onLocalEntityUpdated(pi, entity);
}

void EndpointEntityImpl::onRemoteEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	_endpointCapabilityDelegate->onRemoteEntityOnline(pi, entity);
}

void EndpointEntityImpl::onRemoteEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept
{
	_endpointCapabilityDelegate->onRemoteEntityOffline(pi, entityID);
}

void EndpointEntityImpl::onRemoteEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	_endpointCapabilityDelegate->onRemoteEntityUpdated(pi, entity);
}

/* **** AECP notifications **** */
void EndpointEntityImpl::onAecpAemUnsolicitedResponse(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aecpdu) noexcept
{
	_endpointCapabilityDelegate->onAecpAemUnsolicitedResponse(pi, aecpdu);
}

void EndpointEntityImpl::onAecpAemIdentifyNotification(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aecpdu) noexcept
{
	_endpointCapabilityDelegate->onAecpAemIdentifyNotification(pi, aecpdu);
}

/* **** ACMP notifications **** */
void EndpointEntityImpl::onAcmpCommand(protocol::ProtocolInterface* const pi, protocol::Acmpdu const& acmpdu) noexcept
{
	_endpointCapabilityDelegate->onAcmpCommand(pi, acmpdu);
}

void EndpointEntityImpl::onAcmpResponse(protocol::ProtocolInterface* const pi, protocol::Acmpdu const& acmpdu) noexcept
{
	_endpointCapabilityDelegate->onAcmpResponse(pi, acmpdu);
}

/* ************************************************************************** */
/* LocalEntityImpl overrides                                                  */
/* ************************************************************************** */
bool EndpointEntityImpl::onUnhandledAecpCommand(protocol::ProtocolInterface* const pi, protocol::Aecpdu const& aecpdu) noexcept
{
	return _endpointCapabilityDelegate->onUnhandledAecpCommand(pi, aecpdu);
}

bool EndpointEntityImpl::onUnhandledAecpVuCommand(protocol::ProtocolInterface* const pi, protocol::VuAecpdu::ProtocolIdentifier const& protocolIdentifier, protocol::Aecpdu const& aecpdu) noexcept
{
	return _endpointCapabilityDelegate->onUnhandledAecpVuCommand(pi, protocolIdentifier, aecpdu);
}

/* ************************************************************************** */
/* EndpointEntity methods                                                     */
/* ************************************************************************** */
/** Entry point */
EndpointEntity* LA_AVDECC_CALL_CONVENTION EndpointEntity::createRawEndpointEntity(protocol::ProtocolInterface* const protocolInterface, CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation, endpoint::Delegate* const delegate)
{
	return new LocalEntityGuard<EndpointEntityImpl>(protocolInterface, commonInformation, interfacesInformation, delegate);
}

/** Constructor */
EndpointEntity::EndpointEntity(CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation)
	: LocalEntity(commonInformation, interfacesInformation)
{
}

} // namespace entity
} // namespace avdecc
} // namespace la
