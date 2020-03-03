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
* @file endpointCapabilityDelegate.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/endpointEntity.hpp"

#include "entityImpl.hpp"

namespace la
{
namespace avdecc
{
namespace entity
{
namespace endpoint
{
class CapabilityDelegate final : public entity::CapabilityDelegate
{
public:
	/* ************************************************************************** */
	/* CapabilityDelegate life cycle                                              */
	/* ************************************************************************** */
	CapabilityDelegate(protocol::ProtocolInterface* const protocolInterface, endpoint::Delegate* endpointDelegate, Interface& endpointInterface, UniqueIdentifier const endpointID) noexcept;
	virtual ~CapabilityDelegate() noexcept;

	/* ************************************************************************** */
	/* Endpoint methods                                                           */
	/* ************************************************************************** */
	void setEndpointDelegate(endpoint::Delegate* const delegate) noexcept;
	/* Discovery Protocol (ADP) */
	/* Enumeration and Control Protocol (AECP) AEM */
	void queryEntityAvailable(UniqueIdentifier const targetEntityID, networkInterface::MacAddress const& targetMacAddress, Interface::QueryEntityAvailableHandler const& handler) const noexcept;
	void queryControllerAvailable(UniqueIdentifier const targetEntityID, networkInterface::MacAddress const& targetMacAddress, Interface::QueryControllerAvailableHandler const& handler) const noexcept;
	/* Enumeration and Control Protocol (AECP) AA */
	//void addressAccess(UniqueIdentifier const targetEntityID, addressAccess::Tlvs const& tlvs, Interface::AddressAccessHandler const& handler) const noexcept;
	/* Connection Management Protocol (ACMP) */
	//void connectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, Interface::ConnectStreamHandler const& handler) const noexcept;
	//void disconnectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, Interface::DisconnectStreamHandler const& handler) const noexcept;
	//void disconnectTalkerStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, Interface::DisconnectTalkerStreamHandler const& handler) const noexcept;
	//void getTalkerStreamState(model::StreamIdentification const& talkerStream, Interface::GetTalkerStreamStateHandler const& handler) const noexcept;
	//void getListenerStreamState(model::StreamIdentification const& listenerStream, Interface::GetListenerStreamStateHandler const& handler) const noexcept;
	//void getTalkerStreamConnection(model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex, Interface::GetTalkerStreamConnectionHandler const& handler) const noexcept;

	/* ************************************************************************** */
	/* Endpoint notifications                                                     */
	/* ************************************************************************** */
	/* **** Statistics **** */
	//void onAecpRetry(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept;
	//void onAecpTimeout(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept;
	//void onAecpUnexpectedResponse(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept;
	//void onAecpResponseTime(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept;

	// Deleted compiler auto-generated methods
	CapabilityDelegate(CapabilityDelegate&&) = delete;
	CapabilityDelegate(CapabilityDelegate const&) = delete;
	CapabilityDelegate& operator=(CapabilityDelegate const&) = delete;
	CapabilityDelegate& operator=(CapabilityDelegate&&) = delete;

private:
	using DiscoveredEntities = std::unordered_map<UniqueIdentifier, Entity, UniqueIdentifier::hash>;

	/* ************************************************************************** */
	/* CapabilityDelegate overrides                                               */
	/* ************************************************************************** */
	virtual void onTransportError(protocol::ProtocolInterface* const pi) noexcept override;
	/* **** Discovery notifications **** */
	//virtual void onLocalEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	//virtual void onLocalEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept override;
	//virtual void onLocalEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	//virtual void onRemoteEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	//virtual void onRemoteEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept override;
	//virtual void onRemoteEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept override;
	/* **** AECP notifications **** */
	virtual bool onUnhandledAecpCommand(protocol::ProtocolInterface* const pi, protocol::Aecpdu const& aecpdu) noexcept override;
	/* **** ACMP notifications **** */
	virtual void onAcmpCommand(protocol::ProtocolInterface* const pi, protocol::Acmpdu const& acmpdu) noexcept override;
	virtual void onAcmpResponse(protocol::ProtocolInterface* const pi, protocol::Acmpdu const& acmpdu) noexcept override;

	/* ************************************************************************** */
	/* Internal methods                                                           */
	/* ************************************************************************** */
	//bool isResponseForController(protocol::AcmpMessageType const messageType) const noexcept;
	void sendAemAecpCommand(UniqueIdentifier const targetEntityID, networkInterface::MacAddress const& targetMacAddress, protocol::AemCommandType const commandType, void const* const payload, size_t const payloadLength, std::function<void(LocalEntity::AemCommandStatus)> const& handler) const noexcept;
	//void sendAaAecpCommand(UniqueIdentifier const targetEntityID, addressAccess::Tlvs const& tlvs, LocalEntityImpl<>::OnAaAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept;
	//void sendMvuAecpCommand(UniqueIdentifier const targetEntityID, protocol::MvuCommandType const commandType, void const* const payload, size_t const payloadLength, LocalEntityImpl<>::OnMvuAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept;
	void sendAcmpCommand(protocol::AcmpMessageType const messageType, UniqueIdentifier const controllerEntityID, UniqueIdentifier const talkerEntityID, model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, model::StreamIndex const listenerStreamIndex, std::uint16_t const connectionIndex, std::function<void(LocalEntity::ControlStatus)> const& handler) const noexcept;
	bool processAemAecpCommand(protocol::AemAecpdu const& command) const noexcept;
	//void processAaAecpResponse(protocol::Aecpdu const* const response, LocalEntityImpl<>::OnAaAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept;
	//void processMvuAecpResponse(protocol::Aecpdu const* const response, LocalEntityImpl<>::OnMvuAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept;
	//void processAcmpResponse(protocol::Acmpdu const* const response, LocalEntityImpl<>::OnACMPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed) const noexcept;

	/* ************************************************************************** */
	/* Internal variables                                                         */
	/* ************************************************************************** */
	protocol::ProtocolInterface* const _protocolInterface{ nullptr };
	endpoint::Delegate* _endpointDelegate{ nullptr };
	Interface& _endpointInterface;
	UniqueIdentifier const _endpointID{ UniqueIdentifier::getNullUniqueIdentifier() };
};

} // namespace endpoint
} // namespace entity
} // namespace avdecc
} // namespace la
