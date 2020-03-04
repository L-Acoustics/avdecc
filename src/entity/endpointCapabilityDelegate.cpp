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
* @file endpointCapabilityDelegate.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/utils.hpp"

#include "endpointCapabilityDelegate.hpp"
#include "protocol/protocolAemPayloads.hpp"
#include "protocol/protocolMvuPayloads.hpp"

#include <exception>
#include <chrono>
#include <thread>

namespace la
{
namespace avdecc
{
namespace entity
{
namespace endpoint
{
/* ************************************************************************** */
/* Static variables used for bindings                                         */
/* ************************************************************************** */
//static model::AudioMappings const s_emptyMappings{}; // Empty AudioMappings used by timeout callback (needs a ref to an AudioMappings)
//static model::StreamInfo const s_emptyStreamInfo{}; // Empty StreamInfo used by timeout callback (needs a ref to a StreamInfo)
//static model::AvbInfo const s_emptyAvbInfo{}; // Empty AvbInfo used by timeout callback (needs a ref to an AvbInfo)
//static model::AsPath const s_emptyAsPath{}; // Empty AsPath used by timeout callback (needs a ref to an AsPath)
//static model::AvdeccFixedString const s_emptyAvdeccFixedString{}; // Empty AvdeccFixedString used by timeout callback (needs a ref to a std::string)
//static model::MilanInfo const s_emptyMilanInfo{}; // Empty MilanInfo used by timeout callback (need a ref to a MilanInfo)

/* ************************************************************************** */
/* Exceptions                                                                 */
/* ************************************************************************** */
class InvalidDescriptorTypeException final : public Exception
{
public:
	InvalidDescriptorTypeException()
		: Exception("Invalid DescriptorType")
	{
	}
};

/* ************************************************************************** */
/* CapabilityDelegate life cycle                                              */
/* ************************************************************************** */
CapabilityDelegate::CapabilityDelegate(protocol::ProtocolInterface* const protocolInterface, endpoint::Delegate* endpointDelegate, Interface& endpointInterface, UniqueIdentifier const endpointID) noexcept
	: _protocolInterface(protocolInterface)
	, _endpointDelegate(endpointDelegate)
	, _endpointInterface(endpointInterface)
	, _endpointID(endpointID)
{
}

CapabilityDelegate::~CapabilityDelegate() noexcept {}

/* ************************************************************************** */
/* Endpoint methods                                                           */
/* ************************************************************************** */
void CapabilityDelegate::setEndpointDelegate(endpoint::Delegate* const delegate) noexcept
{
#pragma message("TODO: Protect the _endpointDelegate so it cannot be changed while it's being used (use pi's lock ?? Check for deadlocks!)")
	_endpointDelegate = delegate;
}

/* Discovery Protocol (ADP) */
/* Enumeration and Control Protocol (AECP) AEM */
void CapabilityDelegate::queryEntityAvailable(UniqueIdentifier const targetEntityID, networkInterface::MacAddress const& targetMacAddress, Interface::QueryEntityAvailableHandler const& handler) const noexcept
{
	auto const resultHandler = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_endpointInterface, targetEntityID, std::placeholders::_1);
	sendAemAecpCommand(targetEntityID, targetMacAddress, protocol::AemCommandType::EntityAvailable, nullptr, 0, resultHandler);
}

void CapabilityDelegate::queryControllerAvailable(UniqueIdentifier const targetEntityID, networkInterface::MacAddress const& targetMacAddress, Interface::QueryControllerAvailableHandler const& handler) const noexcept
{
	auto const resultHandler = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_endpointInterface, targetEntityID, std::placeholders::_1);
	sendAemAecpCommand(targetEntityID, targetMacAddress, protocol::AemCommandType::ControllerAvailable, nullptr, 0, resultHandler);
}

/* Enumeration and Control Protocol (AECP) AA */
//void CapabilityDelegate::addressAccess(UniqueIdentifier const targetEntityID, addressAccess::Tlvs const& tlvs, Interface::AddressAccessHandler const& handler) const noexcept
//{
//	auto const errorCallback = LocalEntityImpl<>::makeAaAECPErrorHandler(handler, &_endpointInterface, targetEntityID, std::placeholders::_1, addressAccess::Tlvs{});
//	sendAaAecpCommand(targetEntityID, tlvs, errorCallback, handler);
//}

/* Connection Management Protocol (ACMP) */
//void CapabilityDelegate::connectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, Interface::ConnectStreamHandler const& handler) const noexcept
//{
//	auto const errorCallback = LocalEntityImpl<>::makeACMPErrorHandler(handler, &_endpointInterface, talkerStream, listenerStream, std::uint16_t(0), entity::ConnectionFlags{}, std::placeholders::_1);
//	sendAcmpCommand(protocol::AcmpMessageType::ConnectRxCommand, talkerStream.entityID, talkerStream.streamIndex, listenerStream.entityID, listenerStream.streamIndex, std::uint16_t(0), errorCallback, handler);
//}
//
//void CapabilityDelegate::disconnectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, Interface::DisconnectStreamHandler const& handler) const noexcept
//{
//	auto const errorCallback = LocalEntityImpl<>::makeACMPErrorHandler(handler, &_endpointInterface, talkerStream, listenerStream, std::uint16_t(0), entity::ConnectionFlags{}, std::placeholders::_1);
//	sendAcmpCommand(protocol::AcmpMessageType::DisconnectRxCommand, talkerStream.entityID, talkerStream.streamIndex, listenerStream.entityID, listenerStream.streamIndex, std::uint16_t(0), errorCallback, handler);
//}
//
//void CapabilityDelegate::disconnectTalkerStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, Interface::DisconnectTalkerStreamHandler const& handler) const noexcept
//{
//	auto const errorCallback = LocalEntityImpl<>::makeACMPErrorHandler(handler, &_endpointInterface, talkerStream, listenerStream, std::uint16_t(0), entity::ConnectionFlags{}, std::placeholders::_1);
//	sendAcmpCommand(protocol::AcmpMessageType::DisconnectTxCommand, talkerStream.entityID, talkerStream.streamIndex, listenerStream.entityID, listenerStream.streamIndex, std::uint16_t(0), errorCallback, handler);
//}
//
//void CapabilityDelegate::getTalkerStreamState(model::StreamIdentification const& talkerStream, Interface::GetTalkerStreamStateHandler const& handler) const noexcept
//{
//	auto const errorCallback = LocalEntityImpl<>::makeACMPErrorHandler(handler, &_endpointInterface, talkerStream, model::StreamIdentification{}, std::uint16_t(0), entity::ConnectionFlags{}, std::placeholders::_1);
//	sendAcmpCommand(protocol::AcmpMessageType::GetTxStateCommand, talkerStream.entityID, talkerStream.streamIndex, UniqueIdentifier::getNullUniqueIdentifier(), model::StreamIndex(0), std::uint16_t(0), errorCallback, handler);
//}
//
//void CapabilityDelegate::getListenerStreamState(model::StreamIdentification const& listenerStream, Interface::GetListenerStreamStateHandler const& handler) const noexcept
//{
//	auto const errorCallback = LocalEntityImpl<>::makeACMPErrorHandler(handler, &_endpointInterface, model::StreamIdentification{}, listenerStream, std::uint16_t(0), entity::ConnectionFlags{}, std::placeholders::_1);
//	sendAcmpCommand(protocol::AcmpMessageType::GetRxStateCommand, UniqueIdentifier::getNullUniqueIdentifier(), model::StreamIndex(0), listenerStream.entityID, listenerStream.streamIndex, std::uint16_t(0), errorCallback, handler);
//}
//
//void CapabilityDelegate::getTalkerStreamConnection(model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex, Interface::GetTalkerStreamConnectionHandler const& handler) const noexcept
//{
//	auto const errorCallback = LocalEntityImpl<>::makeACMPErrorHandler(handler, &_endpointInterface, talkerStream, model::StreamIdentification{}, connectionIndex, entity::ConnectionFlags{}, std::placeholders::_1);
//	sendAcmpCommand(protocol::AcmpMessageType::GetTxConnectionCommand, talkerStream.entityID, talkerStream.streamIndex, UniqueIdentifier::getNullUniqueIdentifier(), model::StreamIndex(0), connectionIndex, errorCallback, handler);
//}

/* ************************************************************************** */
/* LocalEntityImpl<>::CapabilityDelegate overrides                          */
/* ************************************************************************** */
void CapabilityDelegate::onTransportError(protocol::ProtocolInterface* const /*pi*/) noexcept
{
	utils::invokeProtectedMethod(&endpoint::Delegate::onTransportError, _endpointDelegate, &_endpointInterface);
}

/* **** Discovery notifications **** */
/* **** AECP notifications **** */
bool CapabilityDelegate::onUnhandledAecpCommand(protocol::ProtocolInterface* const /*pi*/, protocol::Aecpdu const& aecpdu) noexcept
{
	// Ignore messages not for me
	if (_endpointID != aecpdu.getTargetEntityID())
		return false;

	// AEM
	if (aecpdu.getMessageType() == protocol::AecpMessageType::AemCommand)
	{
		auto const& aem = static_cast<protocol::AemAecpdu const&>(aecpdu);

		return processAemAecpCommand(aem);
	}
	return false;
}

/* **** ACMP notifications **** */
void CapabilityDelegate::onAcmpCommand(protocol::ProtocolInterface* const /*pi*/, protocol::Acmpdu const& /*acmpdu*/) noexcept {}

void CapabilityDelegate::onAcmpResponse(protocol::ProtocolInterface* const /*pi*/, protocol::Acmpdu const& /*acmpdu*/) noexcept {}

/* ************************************************************************** */
/* Endpoint notifications                                                     */
/* ************************************************************************** */
/* **** Statistics **** */
//void CapabilityDelegate::onAecpRetry(protocol::ProtocolInterface* const /*pi*/, UniqueIdentifier const& entityID) noexcept
//{
//	// Statistics
//	utils::invokeProtectedMethod(&endpoint::Delegate::onAecpRetry, _endpointDelegate, &_endpointInterface, entityID);
//}
//
//void CapabilityDelegate::onAecpTimeout(protocol::ProtocolInterface* const /*pi*/, UniqueIdentifier const& entityID) noexcept
//{
//	// Statistics
//	utils::invokeProtectedMethod(&endpoint::Delegate::onAecpTimeout, _endpointDelegate, &_endpointInterface, entityID);
//}
//
//void CapabilityDelegate::onAecpUnexpectedResponse(protocol::ProtocolInterface* const /*pi*/, UniqueIdentifier const& entityID) noexcept
//{
//	// Statistics
//	utils::invokeProtectedMethod(&endpoint::Delegate::onAecpUnexpectedResponse, _endpointDelegate, &_endpointInterface, entityID);
//}
//
//void CapabilityDelegate::onAecpResponseTime(protocol::ProtocolInterface* const /*pi*/, UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept
//{
//	// Statistics
//	utils::invokeProtectedMethod(&endpoint::Delegate::onAecpResponseTime, _endpointDelegate, &_endpointInterface, entityID, responseTime);
//}

/* ************************************************************************** */
/* Internal methods                                                           */
/* ************************************************************************** */
//bool CapabilityDelegate::isResponseForController(protocol::AcmpMessageType const messageType) const noexcept
//{
//	if (messageType == protocol::AcmpMessageType::ConnectRxResponse || messageType == protocol::AcmpMessageType::DisconnectRxResponse || messageType == protocol::AcmpMessageType::GetRxStateResponse || messageType == protocol::AcmpMessageType::GetTxConnectionResponse)
//		return true;
//	return false;
//}

void CapabilityDelegate::sendAemAecpCommand(UniqueIdentifier const targetEntityID, networkInterface::MacAddress const& targetMacAddress, protocol::AemCommandType const commandType, void const* const payload, size_t const payloadLength, std::function<void(LocalEntity::AemCommandStatus)> const& handler) const noexcept
{
	LocalEntityImpl<>::sendAemAecpCommand(_protocolInterface, _endpointID, targetEntityID, targetMacAddress, commandType, payload, payloadLength,
		[handler](protocol::Aecpdu const* const /*response*/, LocalEntity::AemCommandStatus const status)
		{
			utils::invokeProtectedHandler(handler, status);
		});
}

void CapabilityDelegate::sendAcmpCommand(protocol::AcmpMessageType const messageType, UniqueIdentifier const controllerEntityID, UniqueIdentifier const talkerEntityID, model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, model::StreamIndex const listenerStreamIndex, std::uint16_t const connectionIndex, std::function<void(LocalEntity::ControlStatus)> const& handler) const noexcept
{
	LocalEntityImpl<>::sendAcmpCommand(_protocolInterface, messageType, controllerEntityID, talkerEntityID, talkerStreamIndex, listenerEntityID, listenerStreamIndex, connectionIndex,
		[handler](protocol::Acmpdu const* const /*response*/, LocalEntity::ControlStatus const status)
		{
			utils::invokeProtectedHandler(handler, status);
		});
}

bool CapabilityDelegate::processAemAecpCommand(protocol::AemAecpdu const& command) const noexcept
{
	auto const status = static_cast<LocalEntity::AemCommandStatus>(command.getStatus().getValue()); // We have to convert protocol status to our extended status

	// We are always expected to get a Success Status
	if (status != LocalEntity::AemCommandStatus::Success)
	{
		LOG_ENDPOINT_ENTITY_ERROR(command.getTargetEntityID(), "Ignoring received non Success AEM command: {}", std::string(command.getCommandType()));
		return false;
	}

	static std::unordered_map<protocol::AemCommandType::value_type, std::function<bool(endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)>> s_Dispatch
	{
		// Acquire Entity
		{ protocol::AemCommandType::AcquireEntity.getValue(),
			[](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
		// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const [flags, ownerID, descriptorType, descriptorIndex] = protocol::aemPayload::deserializeAcquireEntityCommand(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeAcquireEntityCommand(aem.getPayload());
				protocol::AemAcquireEntityFlags const flags = std::get<0>(result);
				UniqueIdentifier const ownerID = std::get<1>(result);
				entity::model::DescriptorType const descriptorType = std::get<2>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<3>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				if ((flags & protocol::AemAcquireEntityFlags::Release) == protocol::AemAcquireEntityFlags::Release)
				{
					return utils::invokeProtectedMethodWithReturn<bool>(&endpoint::Delegate::onQueryReleaseEntity, delegate, endpointInterface, targetID, aem, descriptorType, descriptorIndex);
				}
				else
				{
					return utils::invokeProtectedMethodWithReturn<bool>(&endpoint::Delegate::onQueryAcquireEntity, delegate, endpointInterface, targetID, aem, descriptorType, descriptorIndex);
				}
			} },
#if 0
		// Lock Entity
		{ protocol::AemCommandType::LockEntity.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[flags, lockedID, descriptorType, descriptorIndex] = protocol::aemPayload::deserializeLockEntityResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeLockEntityResponse(aem.getPayload());
				protocol::AemLockEntityFlags const flags = std::get<0>(result);
				UniqueIdentifier const lockedID = std::get<1>(result);
				entity::model::DescriptorType const descriptorType = std::get<2>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<3>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				if ((flags & protocol::AemLockEntityFlags::Unlock) == protocol::AemLockEntityFlags::Unlock)
				{
					answerCallback.invoke<controller::Interface::UnlockEntityHandler>(endpointInterface, targetID, status, lockedID, descriptorType, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onEntityUnlocked, delegate, endpointInterface, targetID, lockedID, descriptorType, descriptorIndex);
					}
				}
				else
				{
					answerCallback.invoke<controller::Interface::LockEntityHandler>(endpointInterface, targetID, status, lockedID, descriptorType, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onEntityLocked, delegate, endpointInterface, targetID, lockedID, descriptorType, descriptorIndex);
					}
				}
			}
		},
		// Entity Available
		{ protocol::AemCommandType::EntityAvailable.getValue(), [](endpoint::Delegate* const /*delegate*/, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
				auto const targetID = aem.getTargetEntityID();
				answerCallback.invoke<controller::Interface::QueryEntityAvailableHandler>(endpointInterface, targetID, status);
			}
		},
		// Controller Available
		{ protocol::AemCommandType::ControllerAvailable.getValue(), [](endpoint::Delegate* const /*delegate*/, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
				auto const targetID = aem.getTargetEntityID();
				answerCallback.invoke<controller::Interface::QueryControllerAvailableHandler>(endpointInterface, targetID, status);
			}
		},
		// Read Descriptor
		{ protocol::AemCommandType::ReadDescriptor.getValue(), [](endpoint::Delegate* const /*delegate*/, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
				auto const payload = aem.getPayload();
		// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[commonSize, configurationIndex, descriptorType, descriptorIndex] = protocol::aemPayload::deserializeReadDescriptorCommonResponse(payload);
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeReadDescriptorCommonResponse(payload);
				size_t const commonSize = std::get<0>(result);
				entity::model::ConfigurationIndex const configurationIndex = std::get<1>(result);
				entity::model::DescriptorType const descriptorType = std::get<2>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<3>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				auto const aemStatus = protocol::AemAecpStatus(static_cast<protocol::AemAecpStatus::value_type>(status));

				switch (descriptorType)
				{
					case model::DescriptorType::Entity:
					{
						// Deserialize entity descriptor
						auto entityDescriptor = protocol::aemPayload::deserializeReadEntityDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::EntityDescriptorHandler>(endpointInterface, targetID, status, entityDescriptor);
						break;
					}

					case model::DescriptorType::Configuration:
					{
						// Deserialize configuration descriptor
						auto configurationDescriptor = protocol::aemPayload::deserializeReadConfigurationDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::ConfigurationDescriptorHandler>(endpointInterface, targetID, status, static_cast<model::ConfigurationIndex>(descriptorIndex), configurationDescriptor); // Passing descriptorIndex as ConfigurationIndex here is NOT an error. See 7.4.5.1
						break;
					}

					case model::DescriptorType::AudioUnit:
					{
						// Deserialize audio unit descriptor
						auto audioUnitDescriptor = protocol::aemPayload::deserializeReadAudioUnitDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::AudioUnitDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, audioUnitDescriptor);
						break;
					}

					case model::DescriptorType::StreamInput:
					{
						// Deserialize stream input descriptor
						auto streamDescriptor = protocol::aemPayload::deserializeReadStreamDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::StreamInputDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, streamDescriptor);
						break;
					}

					case model::DescriptorType::StreamOutput:
					{
						// Deserialize stream output descriptor
						auto streamDescriptor = protocol::aemPayload::deserializeReadStreamDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::StreamOutputDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, streamDescriptor);
						break;
					}

					case model::DescriptorType::JackInput:
					{
						// Deserialize jack input descriptor
						auto jackDescriptor = protocol::aemPayload::deserializeReadJackDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::JackInputDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, jackDescriptor);
						break;
					}

					case model::DescriptorType::JackOutput:
					{
						// Deserialize jack output descriptor
						auto jackDescriptor = protocol::aemPayload::deserializeReadJackDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::JackOutputDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, jackDescriptor);
						break;
					}

					case model::DescriptorType::AvbInterface:
					{
						// Deserialize avb interface descriptor
						auto avbInterfaceDescriptor = protocol::aemPayload::deserializeReadAvbInterfaceDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::AvbInterfaceDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, avbInterfaceDescriptor);
						break;
					}

					case model::DescriptorType::ClockSource:
					{
						// Deserialize clock source descriptor
						auto clockSourceDescriptor = protocol::aemPayload::deserializeReadClockSourceDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::ClockSourceDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, clockSourceDescriptor);
						break;
					}

					case model::DescriptorType::MemoryObject:
					{
						// Deserialize memory object descriptor
						auto memoryObjectDescriptor = protocol::aemPayload::deserializeReadMemoryObjectDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::MemoryObjectDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, memoryObjectDescriptor);
						break;
					}

					case model::DescriptorType::Locale:
					{
						// Deserialize locale descriptor
						auto localeDescriptor = protocol::aemPayload::deserializeReadLocaleDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::LocaleDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, localeDescriptor);
						break;
					}

					case model::DescriptorType::Strings:
					{
						// Deserialize strings descriptor
						auto stringsDescriptor = protocol::aemPayload::deserializeReadStringsDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::StringsDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, stringsDescriptor);
						break;
					}

					case model::DescriptorType::StreamPortInput:
					{
						// Deserialize stream port descriptor
						auto streamPortDescriptor = protocol::aemPayload::deserializeReadStreamPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::StreamPortInputDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, streamPortDescriptor);
						break;
					}

					case model::DescriptorType::StreamPortOutput:
					{
						// Deserialize stream port descriptor
						auto streamPortDescriptor = protocol::aemPayload::deserializeReadStreamPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::StreamPortOutputDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, streamPortDescriptor);
						break;
					}

					case model::DescriptorType::ExternalPortInput:
					{
						// Deserialize external port descriptor
						auto externalPortDescriptor = protocol::aemPayload::deserializeReadExternalPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::ExternalPortInputDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, externalPortDescriptor);
						break;
					}

					case model::DescriptorType::ExternalPortOutput:
					{
						// Deserialize external port descriptor
						auto externalPortDescriptor = protocol::aemPayload::deserializeReadExternalPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::ExternalPortOutputDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, externalPortDescriptor);
						break;
					}

					case model::DescriptorType::InternalPortInput:
					{
						// Deserialize internal port descriptor
						auto internalPortDescriptor = protocol::aemPayload::deserializeReadInternalPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::InternalPortInputDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, internalPortDescriptor);
						break;
					}

					case model::DescriptorType::InternalPortOutput:
					{
						// Deserialize internal port descriptor
						auto internalPortDescriptor = protocol::aemPayload::deserializeReadInternalPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::InternalPortOutputDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, internalPortDescriptor);
						break;
					}

					case model::DescriptorType::AudioCluster:
					{
						// Deserialize audio cluster descriptor
						auto audioClusterDescriptor = protocol::aemPayload::deserializeReadAudioClusterDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::AudioClusterDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, audioClusterDescriptor);
						break;
					}

					case model::DescriptorType::AudioMap:
					{
						// Deserialize audio map descriptor
						auto audioMapDescriptor = protocol::aemPayload::deserializeReadAudioMapDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::AudioMapDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, audioMapDescriptor);
						break;
					}

					case model::DescriptorType::ClockDomain:
					{
						// Deserialize clock domain descriptor
						auto clockDomainDescriptor = protocol::aemPayload::deserializeReadClockDomainDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::ClockDomainDescriptorHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, clockDomainDescriptor);
						break;
					}

					default:
						AVDECC_ASSERT(false, "Unhandled descriptor type");
						break;
				}
			}
		},
		// Write Descriptor
		// Set Configuration
		{ protocol::AemCommandType::SetConfiguration.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[configurationIndex] = protocol::aemPayload::deserializeSetConfigurationResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetConfigurationResponse(aem.getPayload());
				model::ConfigurationIndex const configurationIndex = std::get<0>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::SetConfigurationHandler>(endpointInterface, targetID, status, configurationIndex);
				if (aem.getUnsolicited() && delegate && !!status)
				{
					utils::invokeProtectedMethod(&endpoint::Delegate::onConfigurationChanged, delegate, endpointInterface, targetID, configurationIndex);
				}
			}
		},
		// Get Configuration
		{ protocol::AemCommandType::GetConfiguration.getValue(), [](endpoint::Delegate* const /*delegate*/, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[configurationIndex] = protocol::aemPayload::deserializeGetConfigurationResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetConfigurationResponse(aem.getPayload());
				model::ConfigurationIndex const configurationIndex = std::get<0>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::GetConfigurationHandler>(endpointInterface, targetID, status, configurationIndex);
			}
		},
		// Set Stream Format
		{ protocol::AemCommandType::SetStreamFormat.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, streamFormat] = protocol::aemPayload::deserializeSetStreamFormatResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetStreamFormatResponse(aem.getPayload());
				model::DescriptorType const descriptorType = std::get<0>(result);
				model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::StreamFormat const streamFormat = std::get<2>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<controller::Interface::SetStreamInputFormatHandler>(endpointInterface, targetID, status, descriptorIndex, streamFormat);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamInputFormatChanged, delegate, endpointInterface, targetID, descriptorIndex, streamFormat);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<controller::Interface::SetStreamOutputFormatHandler>(endpointInterface, targetID, status, descriptorIndex, streamFormat);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamOutputFormatChanged, delegate, endpointInterface, targetID, descriptorIndex, streamFormat);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Get Stream Format
		{ protocol::AemCommandType::GetStreamFormat.getValue(), [](endpoint::Delegate* const /*delegate*/, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, streamFormat] = protocol::aemPayload::deserializeGetStreamFormatResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetStreamFormatResponse(aem.getPayload());
				model::DescriptorType const descriptorType = std::get<0>(result);
				model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::StreamFormat const streamFormat = std::get<2>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<controller::Interface::GetStreamInputFormatHandler>(endpointInterface, targetID, status, descriptorIndex, streamFormat);
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<controller::Interface::GetStreamOutputFormatHandler>(endpointInterface, targetID, status, descriptorIndex, streamFormat);
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Set Stream Info
		{ protocol::AemCommandType::SetStreamInfo.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, streamInfo] = protocol::aemPayload::deserializeSetStreamInfoResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetStreamInfoResponse(aem.getPayload());
				model::DescriptorType const descriptorType = std::get<0>(result);
				model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::StreamInfo const streamInfo = std::get<2>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<controller::Interface::SetStreamInputInfoHandler>(endpointInterface, targetID, status, descriptorIndex, streamInfo);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamInputInfoChanged, delegate, endpointInterface, targetID, descriptorIndex, streamInfo, false);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<controller::Interface::SetStreamOutputInfoHandler>(endpointInterface, targetID, status, descriptorIndex, streamInfo);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamOutputInfoChanged, delegate, endpointInterface, targetID, descriptorIndex, streamInfo, false);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Get Stream Info
		{ protocol::AemCommandType::GetStreamInfo.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, streamInfo] = protocol::aemPayload::deserializeGetStreamInfoResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetStreamInfoResponse(aem.getPayload());
				model::DescriptorType const descriptorType = std::get<0>(result);
				model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::StreamInfo const streamInfo = std::get<2>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<controller::Interface::GetStreamInputInfoHandler>(endpointInterface, targetID, status, descriptorIndex, streamInfo);
					if (aem.getUnsolicited() && delegate && !!status) // Unsolicited triggered by change in the SRP domain (Clause 7.5.2)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamInputInfoChanged, delegate, endpointInterface, targetID, descriptorIndex, streamInfo, true);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<controller::Interface::GetStreamOutputInfoHandler>(endpointInterface, targetID, status, descriptorIndex, streamInfo);
					if (aem.getUnsolicited() && delegate && !!status) // Unsolicited triggered by change in the SRP domain (Clause 7.5.2)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamOutputInfoChanged, delegate, endpointInterface, targetID, descriptorIndex, streamInfo, true);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Set Name
		{ protocol::AemCommandType::SetName.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, nameIndex, configurationIndex, name] = protocol::aemPayload::deserializeSetNameResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetNameResponse(aem.getPayload());
				model::DescriptorType const descriptorType = std::get<0>(result);
				model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				std::uint16_t const nameIndex = std::get<2>(result);
				model::ConfigurationIndex const configurationIndex = std::get<3>(result);
				model::AvdeccFixedString const name = std::get<4>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				switch (descriptorType)
				{
					case model::DescriptorType::Entity:
					{
						if (descriptorIndex != 0)
						{
							LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Invalid descriptorIndex in SET_NAME response for Entity Descriptor: {}", descriptorIndex);
						}
						if (configurationIndex != 0)
						{
							LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Invalid configurationIndex in SET_NAME response for Entity Descriptor: {}", configurationIndex);
						}
						switch (nameIndex)
						{
							case 0: // entity_name
								answerCallback.invoke<controller::Interface::SetEntityNameHandler>(endpointInterface, targetID, status, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&endpoint::Delegate::onEntityNameChanged, delegate, endpointInterface, targetID, name);
								}
								break;
							case 1: // group_name
								answerCallback.invoke<controller::Interface::SetEntityGroupNameHandler>(endpointInterface, targetID, status, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&endpoint::Delegate::onEntityGroupNameChanged, delegate, endpointInterface, targetID, name);
								}
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for Entity Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::Configuration:
					{
						if (configurationIndex != 0)
						{
							LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Invalid configurationIndex in SET_NAME response for Configuration Descriptor: ConfigurationIndex={}", configurationIndex);
						}
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::SetConfigurationNameHandler>(endpointInterface, targetID, status, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&endpoint::Delegate::onConfigurationNameChanged, delegate, endpointInterface, targetID, descriptorIndex, name);
								}
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for Configuration Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AudioUnit:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::SetAudioUnitNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&endpoint::Delegate::onAudioUnitNameChanged, delegate, endpointInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for AudioUnit Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::StreamInput:
					{
						switch (nameIndex)
						{
							case 0: // stream_name
								answerCallback.invoke<controller::Interface::SetStreamInputNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&endpoint::Delegate::onStreamInputNameChanged, delegate, endpointInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for StreamInput Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::StreamOutput:
					{
						switch (nameIndex)
						{
							case 0: // stream_name
								answerCallback.invoke<controller::Interface::SetStreamOutputNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&endpoint::Delegate::onStreamOutputNameChanged, delegate, endpointInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for StreamOutput Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AvbInterface:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::SetAvbInterfaceNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&endpoint::Delegate::onAvbInterfaceNameChanged, delegate, endpointInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for AvbInterface Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::ClockSource:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::SetClockSourceNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&endpoint::Delegate::onClockSourceNameChanged, delegate, endpointInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for ClockSource Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::MemoryObject:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::SetMemoryObjectNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&endpoint::Delegate::onMemoryObjectNameChanged, delegate, endpointInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for MemoryObject Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AudioCluster:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::SetAudioClusterNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&endpoint::Delegate::onAudioClusterNameChanged, delegate, endpointInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for AudioCluster Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::ClockDomain:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::SetClockDomainNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&endpoint::Delegate::onClockDomainNameChanged, delegate, endpointInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for ClockDomain Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					default:
						LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled descriptorType in SET_NAME response: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
						break;
				}
			}
		},
		// Get Name
		{ protocol::AemCommandType::GetName.getValue(), [](endpoint::Delegate* const /*delegate*/, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, nameIndex, configurationIndex, name] = protocol::aemPayload::deserializeGetNameResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetNameResponse(aem.getPayload());
				model::DescriptorType const descriptorType = std::get<0>(result);
				model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				std::uint16_t const nameIndex = std::get<2>(result);
				model::ConfigurationIndex const configurationIndex = std::get<3>(result);
				model::AvdeccFixedString const name = std::get<4>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				switch (descriptorType)
				{
					case model::DescriptorType::Entity:
					{
						if (descriptorIndex != 0)
						{
							LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Invalid descriptorIndex in GET_NAME response for Entity Descriptor: DescriptorIndex={}", descriptorIndex);
						}
						if (configurationIndex != 0)
						{
							LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Invalid configurationIndex in GET_NAME response for Entity Descriptor: ConfigurationIndex={}", configurationIndex);
						}
						switch (nameIndex)
						{
							case 0: // entity_name
								answerCallback.invoke<controller::Interface::GetEntityNameHandler>(endpointInterface, targetID, status, name);
								break;
							case 1: // group_name
								answerCallback.invoke<controller::Interface::GetEntityGroupNameHandler>(endpointInterface, targetID, status, name);
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for Entity Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::Configuration:
					{
						if (configurationIndex != 0)
						{
							LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Invalid configurationIndex in GET_NAME response for Configuration Descriptor: ConfigurationIndex={}", configurationIndex);
						}
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetConfigurationNameHandler>(endpointInterface, targetID, status, descriptorIndex, name);
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for Configuration Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AudioUnit:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetAudioUnitNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for AudioUnit Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::StreamInput:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetStreamInputNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for StreamInput Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::StreamOutput:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetStreamOutputNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for StreamOutput Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AvbInterface:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetAvbInterfaceNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for AvbInterface Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::ClockSource:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetClockSourceNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for ClockSource Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::MemoryObject:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetMemoryObjectNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for MemoryObject Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AudioCluster:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetAudioClusterNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for AudioCluster Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::ClockDomain:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetClockDomainNameHandler>(endpointInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for ClockDomain Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					default:
						LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled descriptorType in GET_NAME response: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
						break;
				}
			}
		},
		// Set Sampling Rate
		{ protocol::AemCommandType::SetSamplingRate.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, samplingRate] = protocol::aemPayload::deserializeSetSamplingRateResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetSamplingRateResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::SamplingRate const samplingRate = std::get<2>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				if (descriptorType == model::DescriptorType::AudioUnit)
				{
					answerCallback.invoke<controller::Interface::SetAudioUnitSamplingRateHandler>(endpointInterface, targetID, status, descriptorIndex, samplingRate);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onAudioUnitSamplingRateChanged, delegate, endpointInterface, targetID, descriptorIndex, samplingRate);
					}
				}
				else if (descriptorType == model::DescriptorType::VideoCluster)
				{
					answerCallback.invoke<controller::Interface::SetVideoClusterSamplingRateHandler>(endpointInterface, targetID, status, descriptorIndex, samplingRate);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onVideoClusterSamplingRateChanged, delegate, endpointInterface, targetID, descriptorIndex, samplingRate);
					}
				}
				else if (descriptorType == model::DescriptorType::SensorCluster)
				{
					answerCallback.invoke<controller::Interface::SetSensorClusterSamplingRateHandler>(endpointInterface, targetID, status, descriptorIndex, samplingRate);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onSensorClusterSamplingRateChanged, delegate, endpointInterface, targetID, descriptorIndex , samplingRate);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Get Sampling Rate
		{ protocol::AemCommandType::GetSamplingRate.getValue(), [](endpoint::Delegate* const /*delegate*/, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, samplingRate] = protocol::aemPayload::deserializeGetSamplingRateResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetSamplingRateResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::SamplingRate const samplingRate = std::get<2>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				if (descriptorType == model::DescriptorType::AudioUnit)
				{
					answerCallback.invoke<controller::Interface::GetAudioUnitSamplingRateHandler>(endpointInterface, targetID, status, descriptorIndex, samplingRate);
				}
				else if (descriptorType == model::DescriptorType::VideoCluster)
				{
					answerCallback.invoke<controller::Interface::GetVideoClusterSamplingRateHandler>(endpointInterface, targetID, status, descriptorIndex, samplingRate);
				}
				else if (descriptorType == model::DescriptorType::SensorCluster)
				{
					answerCallback.invoke<controller::Interface::GetSensorClusterSamplingRateHandler>(endpointInterface, targetID, status, descriptorIndex, samplingRate);
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Set Clock Source
		{ protocol::AemCommandType::SetClockSource.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, clockSourceIndex] = protocol::aemPayload::deserializeSetClockSourceResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetClockSourceResponse(aem.getPayload());
				//entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::ClockSourceIndex const clockSourceIndex = std::get<2>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::SetClockSourceHandler>(endpointInterface, targetID, status, descriptorIndex, clockSourceIndex);
				if (aem.getUnsolicited() && delegate && !!status)
				{
					utils::invokeProtectedMethod(&endpoint::Delegate::onClockSourceChanged, delegate, endpointInterface, targetID, descriptorIndex, clockSourceIndex);
				}
			}
		},
		// Get Clock Source
		{ protocol::AemCommandType::GetClockSource.getValue(),[](endpoint::Delegate* const /*delegate*/, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, clockSourceIndex] = protocol::aemPayload::deserializeGetClockSourceResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetClockSourceResponse(aem.getPayload());
				//entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::ClockSourceIndex const clockSourceIndex = std::get<2>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::GetClockSourceHandler>(endpointInterface, targetID, status, descriptorIndex, clockSourceIndex);
			}
		},
		// Start Streaming
		{ protocol::AemCommandType::StartStreaming.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex] = protocol::aemPayload::deserializeStartStreamingResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeStartStreamingResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<controller::Interface::StartStreamInputHandler>(endpointInterface, targetID, status, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamInputStarted, delegate, endpointInterface, targetID, descriptorIndex);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<controller::Interface::StartStreamOutputHandler>(endpointInterface, targetID, status, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamOutputStarted, delegate, endpointInterface, targetID, descriptorIndex);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Stop Streaming
		{ protocol::AemCommandType::StopStreaming.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex] = protocol::aemPayload::deserializeStopStreamingResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeStopStreamingResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<controller::Interface::StopStreamInputHandler>(endpointInterface, targetID, status, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamInputStopped, delegate, endpointInterface, targetID, descriptorIndex);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<controller::Interface::StopStreamOutputHandler>(endpointInterface, targetID, status, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamOutputStopped, delegate, endpointInterface, targetID, descriptorIndex);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
#endif
			// Register Unsolicited Notifications
			{ protocol::AemCommandType::RegisterUnsolicitedNotification.getValue(),
				[](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
				{
					// Ignore payload size and content, Apple's implementation is bugged and returns too much data
					auto const targetID = aem.getTargetEntityID();
					return utils::invokeProtectedMethodWithReturn<bool>(&endpoint::Delegate::onQueryRegisterToUnsolicitedNotifications, delegate, endpointInterface, targetID, aem);
				} },
			// Unregister Unsolicited Notifications
			{ protocol::AemCommandType::DeregisterUnsolicitedNotification.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
				{
					// Ignore payload size and content, Apple's implementation is bugged and returns too much data
					auto const targetID = aem.getTargetEntityID();
					return utils::invokeProtectedMethodWithReturn<bool>(&endpoint::Delegate::onQueryDeregisteredFromUnsolicitedNotifications, delegate, endpointInterface, targetID, aem);
				} },
#if 0
		// GetAvbInfo
		{ protocol::AemCommandType::GetAvbInfo.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, avbInfo] = protocol::aemPayload::deserializeGetAvbInfoResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetAvbInfoResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::AvbInfo const avbInfo = std::get<2>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				if (descriptorType == model::DescriptorType::AvbInterface)
				{
					answerCallback.invoke<controller::Interface::GetAvbInfoHandler>(endpointInterface, targetID, status, descriptorIndex, avbInfo);
					if (aem.getUnsolicited() && delegate && !!status) // Unsolicited triggered by change in the SRP domain (Clause 7.5.2)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onAvbInfoChanged, delegate, endpointInterface, targetID, descriptorIndex, avbInfo);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// GetAsPath
		{ protocol::AemCommandType::GetAsPath.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorIndex, asPath] = protocol::aemPayload::deserializeGetAsPathResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetAsPathResponse(aem.getPayload());
				entity::model::DescriptorIndex const descriptorIndex = std::get<0>(result);
				entity::model::AsPath const asPath = std::get<1>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::GetAsPathHandler>(endpointInterface, targetID, status, descriptorIndex, asPath);
				if (aem.getUnsolicited() && delegate && !!status) // Unsolicited triggered by change in the SRP domain (Clause 7.5.2)
				{
					utils::invokeProtectedMethod(&endpoint::Delegate::onAsPathChanged, delegate, endpointInterface, targetID, descriptorIndex, asPath);
				}
			}
		},
		// GetCounters
		{ protocol::AemCommandType::GetCounters.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, validFlags, counters] = protocol::aemPayload::deserializeGetCountersResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetCountersResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::DescriptorCounterValidFlag const validFlags = std::get<2>(result);
				entity::model::DescriptorCounters const& counters = std::get<3>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				switch (descriptorType)
				{
					case model::DescriptorType::Entity:
					{
						EntityCounterValidFlags flags;
						flags.assign(validFlags);
						answerCallback.invoke<controller::Interface::GetEntityCountersHandler>(endpointInterface, targetID, status, flags, counters);
						if (aem.getUnsolicited() && delegate && !!status)
						{
							utils::invokeProtectedMethod(&endpoint::Delegate::onEntityCountersChanged, delegate, endpointInterface, targetID, flags, counters);
						}
						if (descriptorIndex != 0)
						{
							LOG_ENDPOINT_ENTITY_WARN(targetID, "GET_COUNTERS response for ENTITY descriptor uses a non-0 DescriptorIndex: {}", descriptorIndex);
						}
						break;
					}
					case model::DescriptorType::AvbInterface:
					{
						AvbInterfaceCounterValidFlags flags;
						flags.assign(validFlags);
						answerCallback.invoke<controller::Interface::GetAvbInterfaceCountersHandler>(endpointInterface, targetID, status, descriptorIndex, flags, counters);
						if (aem.getUnsolicited() && delegate && !!status)
						{
							utils::invokeProtectedMethod(&endpoint::Delegate::onAvbInterfaceCountersChanged, delegate, endpointInterface, targetID, descriptorIndex, flags, counters);
						}
						break;
					}
					case model::DescriptorType::ClockDomain:
					{
						ClockDomainCounterValidFlags flags;
						flags.assign(validFlags);
						answerCallback.invoke<controller::Interface::GetClockDomainCountersHandler>(endpointInterface, targetID, status, descriptorIndex, flags, counters);
						if (aem.getUnsolicited() && delegate && !!status)
						{
							utils::invokeProtectedMethod(&endpoint::Delegate::onClockDomainCountersChanged, delegate, endpointInterface, targetID, descriptorIndex, flags, counters);
						}
						break;
					}
					case model::DescriptorType::StreamInput:
					{
						StreamInputCounterValidFlags flags;
						flags.assign(validFlags);
						answerCallback.invoke<controller::Interface::GetStreamInputCountersHandler>(endpointInterface, targetID, status, descriptorIndex, flags, counters);
						if (aem.getUnsolicited() && delegate && !!status)
						{
							utils::invokeProtectedMethod(&endpoint::Delegate::onStreamInputCountersChanged, delegate, endpointInterface, targetID, descriptorIndex, flags, counters);
						}
						break;
					}
					case model::DescriptorType::StreamOutput:
					{
						StreamOutputCounterValidFlags flags;
						flags.assign(validFlags);
						answerCallback.invoke<controller::Interface::GetStreamOutputCountersHandler>(endpointInterface, targetID, status, descriptorIndex, flags, counters);
						if (aem.getUnsolicited() && delegate && !!status)
						{
							utils::invokeProtectedMethod(&endpoint::Delegate::onStreamOutputCountersChanged, delegate, endpointInterface, targetID, descriptorIndex, flags, counters);
						}
						break;
					}
					default:
						LOG_ENDPOINT_ENTITY_DEBUG(targetID, "Unhandled descriptorType in GET_COUNTERS response: DescriptorType={} DescriptorIndex={}", utils::to_integral(descriptorType), descriptorIndex);
						break;
				}
			}
		},
		// Get Audio Map
		{ protocol::AemCommandType::GetAudioMap.getValue(), []([[maybe_unused]] endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, mapIndex, numberOfMaps, mappings] = protocol::aemPayload::deserializeGetAudioMapResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetAudioMapResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::MapIndex const mapIndex = std::get<2>(result);
				entity::model::MapIndex const numberOfMaps = std::get<3>(result);
				entity::model::AudioMappings const& mappings = std::get<4>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamPortInput)
				{
					answerCallback.invoke<controller::Interface::GetStreamPortInputAudioMapHandler>(endpointInterface, targetID, status, descriptorIndex, numberOfMaps, mapIndex, mappings);
#	ifdef ALLOW_GET_AUDIO_MAP_UNSOL
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamPortInputAudioMappingsChanged, delegate, endpointInterface, targetID, descriptorIndex, numberOfMaps, mapIndex, mappings);
					}
#	endif // ALLOW_GET_AUDIO_MAP_UNSOL
				}
				else if (descriptorType == model::DescriptorType::StreamPortOutput)
				{
					answerCallback.invoke<controller::Interface::GetStreamPortOutputAudioMapHandler>(endpointInterface, targetID, status, descriptorIndex, numberOfMaps, mapIndex, mappings);
#	ifdef ALLOW_GET_AUDIO_MAP_UNSOL
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamPortOutputAudioMappingsChanged, delegate, endpointInterface, targetID, descriptorIndex, numberOfMaps, mapIndex, mappings);
					}
#	endif
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Add Audio Mappings
		{ protocol::AemCommandType::AddAudioMappings.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, mappings] = protocol::aemPayload::deserializeAddAudioMappingsResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeAddAudioMappingsResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::AudioMappings const& mappings = std::get<2>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

		// Notify handlers
				if (descriptorType == model::DescriptorType::StreamPortInput)
				{
					answerCallback.invoke<controller::Interface::AddStreamPortInputAudioMappingsHandler>(endpointInterface, targetID, status, descriptorIndex, mappings);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamPortInputAudioMappingsAdded, delegate, endpointInterface, targetID, descriptorIndex, mappings);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamPortOutput)
				{
					answerCallback.invoke<controller::Interface::AddStreamPortOutputAudioMappingsHandler>(endpointInterface, targetID, status, descriptorIndex, mappings);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamPortOutputAudioMappingsAdded, delegate, endpointInterface, targetID, descriptorIndex, mappings);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Remove Audio Mappings
		{ protocol::AemCommandType::RemoveAudioMappings.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, mappings] = protocol::aemPayload::deserializeRemoveAudioMappingsResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeRemoveAudioMappingsResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::AudioMappings const& mappings = std::get<2>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

		// Notify handlers
				if (descriptorType == model::DescriptorType::StreamPortInput)
				{
					answerCallback.invoke<controller::Interface::RemoveStreamPortInputAudioMappingsHandler>(endpointInterface, targetID, status, descriptorIndex, mappings);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamPortInputAudioMappingsRemoved, delegate, endpointInterface, targetID, descriptorIndex, mappings);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamPortOutput)
				{
					answerCallback.invoke<controller::Interface::RemoveStreamPortOutputAudioMappingsHandler>(endpointInterface, targetID, status, descriptorIndex, mappings);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&endpoint::Delegate::onStreamPortOutputAudioMappingsRemoved, delegate, endpointInterface, targetID, descriptorIndex, mappings);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Start Operation
		{ protocol::AemCommandType::StartOperation.getValue(), [](endpoint::Delegate* const /*delegate*/, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, operationID, operationType, memoryBuffer] = protocol::aemPayload::deserializeStartOperationResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeStartOperationResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::OperationID const operationID = std::get<2>(result);
				entity::model::MemoryObjectOperationType const operationType = std::get<3>(result);
				MemoryBuffer const memoryBuffer = std::get<4>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::StartOperationHandler>(endpointInterface, targetID, status, descriptorType, descriptorIndex, operationID, operationType, memoryBuffer);
			}
		},
		// Abort Operation
		{ protocol::AemCommandType::AbortOperation.getValue(), [](endpoint::Delegate* const /*delegate*/, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, operationID] = protocol::aemPayload::deserializeAbortOperationResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeAbortOperationResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::OperationID const operationID = std::get<2>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::AbortOperationHandler>(endpointInterface, targetID, status, descriptorType, descriptorIndex, operationID);
			}
		},
		// Operation Status
		{ protocol::AemCommandType::OperationStatus.getValue(), [](controller::Delegate* const delegate, Interface const* const endpointInterface, LocalEntity::AemCommandStatus const /*status*/, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& /*answerCallback*/)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, operationID, percentComplete] = protocol::aemPayload::deserializeOperationStatusResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeOperationStatusResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::OperationID const operationID = std::get<2>(result);
				std::uint16_t const percentComplete = std::get<3>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				AVDECC_ASSERT(aem.getUnsolicited(), "OperationStatus can only be an unsolicited response");
				utils::invokeProtectedMethod(&endpoint::Delegate::onOperationStatus, delegate, endpointInterface, targetID, descriptorType, descriptorIndex, operationID, percentComplete);
			}
		},
		// Set Memory Object Length
		{ protocol::AemCommandType::SetMemoryObjectLength.getValue(), [](endpoint::Delegate* const delegate, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[configurationIndex, memoryObjectIndex, length] = protocol::aemPayload::deserializeSetMemoryObjectLengthResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetMemoryObjectLengthResponse(aem.getPayload());
				entity::model::ConfigurationIndex const configurationIndex = std::get<0>(result);
				entity::model::MemoryObjectIndex const memoryObjectIndex = std::get<1>(result);
				std::uint64_t const length = std::get<2>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::SetMemoryObjectLengthHandler>(endpointInterface, targetID, status, configurationIndex, memoryObjectIndex, length);
				if (aem.getUnsolicited() && delegate && !!status)
				{
					utils::invokeProtectedMethod(&endpoint::Delegate::onMemoryObjectLengthChanged, delegate, endpointInterface, targetID, configurationIndex, memoryObjectIndex, length);
				}
			}
		},
		// Get Memory Object Length
		{ protocol::AemCommandType::GetMemoryObjectLength.getValue(),[](endpoint::Delegate* const /*delegate*/, Interface const* const endpointInterface, protocol::AemAecpdu const& aem)
			{
	// Deserialize payload
#	ifdef __cpp_structured_bindings
				auto const[configurationIndex, memoryObjectIndex, length] = protocol::aemPayload::deserializeGetMemoryObjectLengthResponse(aem.getPayload());
#	else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetMemoryObjectLengthResponse(aem.getPayload());
				entity::model::ConfigurationIndex const configurationIndex = std::get<0>(result);
				entity::model::MemoryObjectIndex const memoryObjectIndex = std::get<1>(result);
				std::uint64_t const length = std::get<2>(result);
#	endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::GetMemoryObjectLengthHandler>(endpointInterface, targetID, status, configurationIndex, memoryObjectIndex, length);
			}
		},
		// Set Stream Backup
		// Get Stream Backup
#endif
	};

	auto const& it = s_Dispatch.find(command.getCommandType().getValue());
	if (it == s_Dispatch.end())
	{
		// Unhandled command, log it
		LOG_ENDPOINT_ENTITY_DEBUG(command.getTargetEntityID(), "AEM command {} not handled ({})", std::string(command.getCommandType()), utils::toHexString(command.getCommandType().getValue()));
	}
	else
	{
		try
		{
			return it->second(_endpointDelegate, &_endpointInterface, command);
		}
		catch (protocol::aemPayload::IncorrectPayloadSizeException const& e)
		{
			// Incorrect payload size, log it
			LOG_ENDPOINT_ENTITY_ERROR(command.getTargetEntityID(), "Failed to process {} AEM command: {}", std::string(command.getCommandType()), e.what());
		}
		catch (InvalidDescriptorTypeException const& e)
		{
			// Invalid descriptor type, log it
			LOG_ENDPOINT_ENTITY_ERROR(command.getTargetEntityID(), "Invalid DescriptorType for AEM command: {}", std::string(command.getCommandType()), e.what());
		}
		catch ([[maybe_unused]] std::exception const& e) // Mainly unpacking errors
		{
			LOG_ENDPOINT_ENTITY_ERROR(command.getTargetEntityID(), "Failed to process {} AEM command: {}", std::string(command.getCommandType()), e.what());
		}
	}
	return false;
}

#if 0
void CapabilityDelegate::processAcmpResponse(protocol::Acmpdu const* const response, LocalEntityImpl<>::OnACMPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed) const noexcept
{
	auto const& acmp = static_cast<protocol::Acmpdu const&>(*response);
	auto const status = static_cast<LocalEntity::ControlStatus>(acmp.getStatus().getValue()); // We have to convert protocol status to our extended status

	static std::unordered_map<protocol::AcmpMessageType::value_type, std::function<void(controller::Delegate* const delegate, Interface const* const endpointInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed)>> s_Dispatch{
		// Connect TX response
		{ protocol::AcmpMessageType::ConnectTxResponse.getValue(),
			[](controller::Delegate* const delegate, Interface const* const endpointInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& /*answerCallback*/, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				if (sniffed && delegate)
				{
					utils::invokeProtectedMethod(&endpoint::Delegate::onListenerConnectResponseSniffed, delegate, endpointInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			} },
		// Disconnect TX response
		{ protocol::AcmpMessageType::DisconnectTxResponse.getValue(),
			[](controller::Delegate* const delegate, Interface const* const endpointInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<controller::Interface::DisconnectTalkerStreamHandler>(endpointInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				if (sniffed && delegate)
				{
					utils::invokeProtectedMethod(&endpoint::Delegate::onListenerDisconnectResponseSniffed, delegate, endpointInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			} },
		// Get TX state response
		{ protocol::AcmpMessageType::GetTxStateResponse.getValue(),
			[](controller::Delegate* const delegate, Interface const* const endpointInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<controller::Interface::GetTalkerStreamStateHandler>(endpointInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				if (sniffed && delegate)
				{
					utils::invokeProtectedMethod(&endpoint::Delegate::onGetTalkerStreamStateResponseSniffed, delegate, endpointInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			} },
		// Connect RX response
		{ protocol::AcmpMessageType::ConnectRxResponse.getValue(),
			[](controller::Delegate* const delegate, Interface const* const endpointInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<controller::Interface::ConnectStreamHandler>(endpointInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				if (sniffed && delegate)
				{
					utils::invokeProtectedMethod(&endpoint::Delegate::onControllerConnectResponseSniffed, delegate, endpointInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			} },
		// Disconnect RX response
		{ protocol::AcmpMessageType::DisconnectRxResponse.getValue(),
			[](controller::Delegate* const delegate, Interface const* const endpointInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<controller::Interface::DisconnectStreamHandler>(endpointInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				if (sniffed && delegate)
				{
					utils::invokeProtectedMethod(&endpoint::Delegate::onControllerDisconnectResponseSniffed, delegate, endpointInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			} },
		// Get RX state response
		{ protocol::AcmpMessageType::GetRxStateResponse.getValue(),
			[](controller::Delegate* const delegate, Interface const* const endpointInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<controller::Interface::GetListenerStreamStateHandler>(endpointInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				if (sniffed && delegate)
				{
					utils::invokeProtectedMethod(&endpoint::Delegate::onGetListenerStreamStateResponseSniffed, delegate, endpointInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			} },
		// Get TX connection response
		{ protocol::AcmpMessageType::GetTxConnectionResponse.getValue(),
			[](controller::Delegate* const /*delegate*/, Interface const* const endpointInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const /*sniffed*/)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<controller::Interface::GetTalkerStreamConnectionHandler>(endpointInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
			} },
	};

	auto const& it = s_Dispatch.find(acmp.getMessageType().getValue());
	if (it == s_Dispatch.end())
	{
		// If this is a sniffed message, simply log we do not handle the message
		if (sniffed)
		{
			LOG_ENDPOINT_ENTITY_DEBUG(acmp.getTalkerEntityID(), "ACMP response {} not handled ({})", std::string(acmp.getMessageType()), utils::toHexString(acmp.getMessageType().getValue()));
		}
		// But if it's an expected response, this is an internal error since we sent a command and didn't implement the code to handle the response
		else
		{
			LOG_ENDPOINT_ENTITY_ERROR(acmp.getTalkerEntityID(), "Failed to process ACMP response: Unhandled message type {} ({})", std::string(acmp.getMessageType()), utils::toHexString(acmp.getMessageType().getValue()));
			utils::invokeProtectedHandler(onErrorCallback, LocalEntity::ControlStatus::InternalError);
		}
	}
	else
	{
		try
		{
			it->second(_controllerDelegate, &_endpointInterface, status, acmp, answerCallback, sniffed);
		}
		catch ([[maybe_unused]] std::exception const& e) // Mainly unpacking errors
		{
			LOG_ENDPOINT_ENTITY_ERROR(acmp.getTalkerEntityID(), "Failed to process ACMP response: {}", e.what());
			utils::invokeProtectedHandler(onErrorCallback, LocalEntity::ControlStatus::ProtocolError);
			return;
		}
	}
}
#endif

} // namespace endpoint
} // namespace entity
} // namespace avdecc
} // namespace la
