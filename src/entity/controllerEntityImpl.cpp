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
* @file controllerEntityImpl.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/utils.hpp"
#include "logHelper.hpp"
#include "controllerEntityImpl.hpp"
#include "protocol/protocolAemPayloads.hpp"
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

/** Delay (in milliseconds) between 2 DISCOVER message broadcast */
constexpr int DISCOVER_SEND_DELAY = 10000;

static model::AudioMappings const s_emptyMappings{ 0 }; // Empty audio channel mappings used by timeout callback (needs a ref to an AudioMappings)
static model::StreamInfo const s_emptyStreamInfo{ }; // Empty stream info used by timeout callback (needs a ref to a StreamInfo)
static model::AvbInfo const s_emptyAvbInfo{}; // Empty avb interface info used by timeout callback (needs a ref to an AvbInfo)
static model::AvdeccFixedString const s_emptyAvdeccFixedString{}; // Empty AvdeccFixedString used by timeout callback (needs a ref to a std::string)

/* ************************************************************************** */
/* Exceptions                                                                 */
/* ************************************************************************** */
class InvalidDescriptorTypeException final : public Exception
{
public:
	InvalidDescriptorTypeException() : Exception("Invalid DescriptorType") {}
};

class ControlException final : public std::exception
{
public:
	ControlException(ControllerEntity::ControlStatus const status, char const* const text) : _status(status), _text(text)
	{
	}
	ControllerEntity::ControlStatus getStatus() const
	{
		return _status;
	}
	virtual char const* what() const noexcept override
	{
		return _text;
	}
private:
	ControllerEntity::ControlStatus _status{ ControllerEntity::ControlStatus::InternalError };
	char const* _text{ nullptr };
};


/* ************************************************************************** */
/* ControllerEntityImpl life cycle                                            */
/* ************************************************************************** */
ControllerEntityImpl::ControllerEntityImpl(protocol::ProtocolInterface* const protocolInterface, std::uint16_t const progID, UniqueIdentifier const entityModelID, ControllerEntity::Delegate* const delegate)
	: LocalEntityImpl(protocolInterface, progID, entityModelID, EntityCapabilities::None, 0, TalkerCapabilities::None, 0, ListenerCapabilities::None, ControllerCapabilities::Implemented, 0, protocolInterface->getInterfaceIndex(), UniqueIdentifier{})
	, _delegate(delegate)
{
	// Register observer
	getProtocolInterface()->registerObserver(this);

	// Create the state machine thread
	_discoveryThread = std::thread([this]
	{
		setCurrentThreadName("avdecc::ControllerDiscovery");
		while (!_shouldTerminate)
		{
			// Request a discovery
			auto* pi = getProtocolInterface();
			pi->discoverRemoteEntities();

			// Wait few seconds before sending another one
			std::chrono::time_point<std::chrono::system_clock> start{ std::chrono::system_clock::now() };
			do
			{
				// Wait a little bit so we don't burn the CPU
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			} while (!_shouldTerminate && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count() <= DISCOVER_SEND_DELAY);
		}
	});
}

ControllerEntityImpl::~ControllerEntityImpl() noexcept
{
	// Unregister ourself as a ProtocolInterface observer
	invokeProtectedMethod(&protocol::ProtocolInterface::unregisterObserver, getProtocolInterface(), this);

	// Notify the thread we are shutting down
	_shouldTerminate = true;

	// Wait for the thread to complete its pending tasks
	if (_discoveryThread.joinable())
		_discoveryThread.join();
}

/* ************************************************************************** */
/* ControllerEntityImpl internal methods                                      */
/* ************************************************************************** */
ControllerEntity::AemCommandStatus ControllerEntityImpl::convertErrorToAemCommandStatus(protocol::ProtocolInterface::Error const error) const noexcept
{
	switch (error)
	{
		case protocol::ProtocolInterface::Error::NoError:
			return AemCommandStatus::Success;
		case protocol::ProtocolInterface::Error::TransportError:
			return AemCommandStatus::NetworkError;
		case protocol::ProtocolInterface::Error::Timeout:
			return AemCommandStatus::TimedOut;
		case protocol::ProtocolInterface::Error::UnknownRemoteEntity:
			return AemCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::UnknownLocalEntity:
			AVDECC_ASSERT(false, "Trying to sendAemCommand from a non-existing local entity");
			return AemCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::InvalidEntityType:
			AVDECC_ASSERT(false, "Trying to sendAemCommand from a non-controller entity");
			return AemCommandStatus::InternalError;
		case protocol::ProtocolInterface::Error::InternalError:
			return AemCommandStatus::InternalError;
		default:
			AVDECC_ASSERT(false, "ProtocolInterface error code not handled");
	}
	return AemCommandStatus::InternalError;
}

ControllerEntity::ControlStatus ControllerEntityImpl::convertErrorToControlStatus(protocol::ProtocolInterface::Error const error) const noexcept
{
	switch (error)
	{
		case protocol::ProtocolInterface::Error::NoError:
			return ControlStatus::Success;
		case protocol::ProtocolInterface::Error::TransportError:
			return ControlStatus::NetworkError;
		case protocol::ProtocolInterface::Error::Timeout:
			return ControlStatus::TimedOut;
		case protocol::ProtocolInterface::Error::UnknownRemoteEntity:
			return ControlStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::UnknownLocalEntity:
			AVDECC_ASSERT(false, "Trying to sendAcmpCommand from a non-existing local entity");
			return ControlStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::InvalidEntityType:
			return ControlStatus::InternalError;
		case protocol::ProtocolInterface::Error::InternalError:
			return ControlStatus::InternalError;
		default:
			AVDECC_ASSERT(false, "ProtocolInterface error code not handled");
	}
	return ControlStatus::InternalError;
}

void ControllerEntityImpl::sendAemCommand(UniqueIdentifier const targetEntityID, protocol::AemCommandType const commandType, void const* const payload, size_t const payloadLength, OnAECPErrorCallback const& onErrorCallback, AnswerCallback const& answerCallback) const noexcept
{
	try
	{
		auto* pi = const_cast<ControllerEntityImpl*>(this)->getProtocolInterface();
		auto targetMacAddress = networkInterface::MacAddress{};

		// Search target mac address based on its entityID
		{
			// Lock ProtocolInterface
			std::lock_guard<decltype(*pi)> const lg(*pi);

			auto const it = _discoveredEntities.find(targetEntityID);

			if (it != _discoveredEntities.end())
			{
				// Get entity mac address
				targetMacAddress = it->second.getMacAddress();
			}
		}

		// Return an error if entity is not found in the list
		if (!networkInterface::isMacAddressValid(targetMacAddress))
		{
			invokeProtectedHandler(onErrorCallback, AemCommandStatus::UnknownEntity);
			return;
		}

		// Build AEM-AECPDU frame
		auto frame = protocol::AemAecpdu::create();
		auto* aem = static_cast<protocol::AemAecpdu*>(frame.get());

		// Set Ether2 fields
		aem->setSrcAddress(pi->getMacAddress());
		aem->setDestAddress(targetMacAddress);
		// Set AECP fields
		aem->setMessageType(protocol::AecpMessageType::AemCommand);
		aem->setStatus(protocol::AecpStatus::Success);
		aem->setTargetEntityID(targetEntityID);
		aem->setControllerEntityID(getEntityID());
		// No need to set the SequenceID, it's set by the ProtocolInterface layer
		// Set AEM fields
		aem->setUnsolicited(false);
		aem->setCommandType(commandType);
		aem->setCommandSpecificData(payload, payloadLength);

		auto const error = pi->sendAecpCommand(std::move(frame), targetMacAddress, [onErrorCallback, answerCallback, this](protocol::Aecpdu const* const response, protocol::ProtocolInterface::Error const error) noexcept
		{
			if (!error)
			{
				processAemResponse(response, onErrorCallback, answerCallback); // We sent an AEM command, we know it's an AEM response (so directly call processAemResponse)
			}
			else
			{
				invokeProtectedHandler(onErrorCallback, convertErrorToAemCommandStatus(error));
			}
		});
		if (!!error)
		{
			invokeProtectedHandler(onErrorCallback, convertErrorToAemCommandStatus(error));
		}
	}
	catch (std::invalid_argument const&)
	{
		invokeProtectedHandler(onErrorCallback, AemCommandStatus::ProtocolError);
	}
	catch (...)
	{
		invokeProtectedHandler(onErrorCallback, AemCommandStatus::InternalError);
	}
}

void ControllerEntityImpl::processAemResponse(protocol::Aecpdu const* const response, OnAECPErrorCallback const& onErrorCallback, AnswerCallback const& answerCallback) const noexcept
{
	auto const& aem = static_cast<protocol::AemAecpdu const&>(*response);
	auto const status = static_cast<AemCommandStatus>(aem.getStatus().getValue()); // We have to convert protocol status to our extended status

	static std::unordered_map<protocol::AemCommandType::value_type, std::function<void(ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)>> s_Dispatch
	{
		// Acquire Entity
		{ protocol::AemCommandType::AcquireEntity.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[flags, ownerID, descriptorType, descriptorIndex] = protocol::aemPayload::deserializeAcquireEntityResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeAcquireEntityResponse(aem.getPayload());
				protocol::AemAcquireEntityFlags const flags = std::get<0>(result);
				UniqueIdentifier const ownerID = std::get<1>(result);
				entity::model::DescriptorType const descriptorType = std::get<2>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<3>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				auto* delegate = controller->getDelegate();
				if ((flags & protocol::AemAcquireEntityFlags::Release) == protocol::AemAcquireEntityFlags::Release)
				{
					answerCallback.invoke<ReleaseEntityHandler>(controller, targetID, status, ownerID, descriptorType, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onEntityReleased, delegate, controller, targetID, ownerID, descriptorType, descriptorIndex);
					}
				}
				else
				{
					answerCallback.invoke<AcquireEntityHandler>(controller, targetID, status, ownerID, descriptorType, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onEntityAcquired, delegate, controller, targetID, ownerID, descriptorType, descriptorIndex);
					}
				}
			}
		},
		// Lock Entity
		{ protocol::AemCommandType::LockEntity.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[flags, lockedID, descriptorType, descriptorIndex] = protocol::aemPayload::deserializeLockEntityResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeLockEntityResponse(aem.getPayload());
				protocol::AemLockEntityFlags const flags = std::get<0>(result);
				UniqueIdentifier const lockedID = std::get<1>(result);
				//entity::model::DescriptorType const descriptorType = std::get<2>(result);
				//entity::model::DescriptorIndex const descriptorIndex = std::get<3>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				//auto* delegate = controller->getDelegate();
				if ((flags & protocol::AemLockEntityFlags::Unlock) == protocol::AemLockEntityFlags::Unlock)
				{
					answerCallback.invoke<UnlockEntityHandler>(controller, targetID, status);
					/*if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onEntityUnlocked, delegate, controller, targetID, lockedID);
					}*/
				}
				else
				{
					answerCallback.invoke<LockEntityHandler>(controller, targetID, status, lockedID);
					/*if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onEntityLocked, delegate, controller, targetID, lockedID);
					}*/
				}
			}
		},
		// Entity Available
		{ protocol::AemCommandType::EntityAvailable.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				auto const targetID = aem.getTargetEntityID();
				answerCallback.invoke<QueryEntityAvailableHandler>(controller, targetID, status);
			}
		},
		// Controller Available
		{ protocol::AemCommandType::ControllerAvailable.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				auto const targetID = aem.getTargetEntityID();
				answerCallback.invoke<QueryControllerAvailableHandler>(controller, targetID, status);
			}
		},
		// Read Descriptor
		{ protocol::AemCommandType::ReadDescriptor.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				auto const payload = aem.getPayload();
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[commonSize, configurationIndex, descriptorType, descriptorIndex] = protocol::aemPayload::deserializeReadDescriptorCommonResponse(payload);
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeReadDescriptorCommonResponse(payload);
				size_t const commonSize = std::get<0>(result);
				entity::model::ConfigurationIndex const configurationIndex = std::get<1>(result);
				entity::model::DescriptorType const descriptorType = std::get<2>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<3>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				auto const aemStatus = protocol::AemAecpStatus(static_cast<protocol::AemAecpStatus::value_type>(status));

				switch (descriptorType)
				{
					case model::DescriptorType::Entity:
					{
						// Deserialize entity descriptor
						auto entityDescriptor = protocol::aemPayload::deserializeReadEntityDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<EntityDescriptorHandler>(controller, targetID, status, entityDescriptor);
						break;
					}

					case model::DescriptorType::Configuration:
					{
						// Deserialize configuration descriptor
						auto configurationDescriptor = protocol::aemPayload::deserializeReadConfigurationDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<ConfigurationDescriptorHandler>(controller, targetID, status, static_cast<model::ConfigurationIndex>(descriptorIndex), configurationDescriptor); // Passing descriptorIndex as ConfigurationIndex here is NOT an error. See 7.4.5.1
						break;
					}

					case model::DescriptorType::AudioUnit:
					{
						// Deserialize audio unit descriptor
						auto audioUnitDescriptor = protocol::aemPayload::deserializeReadAudioUnitDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<AudioUnitDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, audioUnitDescriptor);
						break;
					}

					case model::DescriptorType::StreamInput:
					{
						// Deserialize stream input descriptor
						auto streamDescriptor = protocol::aemPayload::deserializeReadStreamDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<StreamInputDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, streamDescriptor);
						break;
					}

					case model::DescriptorType::StreamOutput:
					{
						// Deserialize stream output descriptor
						auto streamDescriptor = protocol::aemPayload::deserializeReadStreamDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<StreamOutputDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, streamDescriptor);
						break;
					}

					case model::DescriptorType::JackInput:
					{
						// Deserialize jack input descriptor
						auto jackDescriptor = protocol::aemPayload::deserializeReadJackDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<JackInputDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, jackDescriptor);
						break;
					}

					case model::DescriptorType::JackOutput:
					{
						// Deserialize jack output descriptor
						auto jackDescriptor = protocol::aemPayload::deserializeReadJackDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<JackOutputDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, jackDescriptor);
						break;
					}

					case model::DescriptorType::AvbInterface:
					{
						// Deserialize avb interface descriptor
						auto avbInterfaceDescriptor = protocol::aemPayload::deserializeReadAvbInterfaceDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<AvbInterfaceDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, avbInterfaceDescriptor);
						break;
					}

					case model::DescriptorType::ClockSource:
					{
						// Deserialize clock source descriptor
						auto clockSourceDescriptor = protocol::aemPayload::deserializeReadClockSourceDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<ClockSourceDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, clockSourceDescriptor);
						break;
					}

					case model::DescriptorType::MemoryObject:
					{
						// Deserialize memory object descriptor
						auto memoryObjectDescriptor = protocol::aemPayload::deserializeReadMemoryObjectDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<MemoryObjectDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, memoryObjectDescriptor);
						break;
					}

					case model::DescriptorType::Locale:
					{
						// Deserialize locale descriptor
						auto localeDescriptor = protocol::aemPayload::deserializeReadLocaleDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<LocaleDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, localeDescriptor);
						break;
					}

					case model::DescriptorType::Strings:
					{
						// Deserialize strings descriptor
						auto stringsDescriptor = protocol::aemPayload::deserializeReadStringsDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<StringsDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, stringsDescriptor);
						break;
					}

					case model::DescriptorType::StreamPortInput:
					{
						// Deserialize stream port descriptor
						auto streamPortDescriptor = protocol::aemPayload::deserializeReadStreamPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<StreamPortInputDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, streamPortDescriptor);
						break;
					}

					case model::DescriptorType::StreamPortOutput:
					{
						// Deserialize stream port descriptor
						auto streamPortDescriptor = protocol::aemPayload::deserializeReadStreamPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<StreamPortOutputDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, streamPortDescriptor);
						break;
					}

					case model::DescriptorType::ExternalPortInput:
					{
						// Deserialize external port descriptor
						auto externalPortDescriptor = protocol::aemPayload::deserializeReadExternalPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<ExternalPortInputDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, externalPortDescriptor);
						break;
					}

					case model::DescriptorType::ExternalPortOutput:
					{
						// Deserialize external port descriptor
						auto externalPortDescriptor = protocol::aemPayload::deserializeReadExternalPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<ExternalPortOutputDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, externalPortDescriptor);
						break;
					}

					case model::DescriptorType::InternalPortInput:
					{
						// Deserialize internal port descriptor
						auto internalPortDescriptor = protocol::aemPayload::deserializeReadInternalPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<InternalPortInputDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, internalPortDescriptor);
						break;
					}

					case model::DescriptorType::InternalPortOutput:
					{
						// Deserialize internal port descriptor
						auto internalPortDescriptor = protocol::aemPayload::deserializeReadInternalPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<InternalPortOutputDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, internalPortDescriptor);
						break;
					}

					case model::DescriptorType::AudioCluster:
					{
						// Deserialize audio cluster descriptor
						auto audioClusterDescriptor = protocol::aemPayload::deserializeReadAudioClusterDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<AudioClusterDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, audioClusterDescriptor);
						break;
					}

					case model::DescriptorType::AudioMap:
					{
						// Deserialize audio map descriptor
						auto audioMapDescriptor = protocol::aemPayload::deserializeReadAudioMapDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<AudioMapDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, audioMapDescriptor);
						break;
					}

					case model::DescriptorType::ClockDomain:
					{
						// Deserialize clock domain descriptor
						auto clockDomainDescriptor = protocol::aemPayload::deserializeReadClockDomainDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<ClockDomainDescriptorHandler>(controller, targetID, status, configurationIndex, descriptorIndex, clockDomainDescriptor);
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
		{ protocol::AemCommandType::SetConfiguration.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[configurationIndex] = protocol::aemPayload::deserializeSetConfigurationResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetConfigurationResponse(aem.getPayload());
				model::ConfigurationIndex const configurationIndex = std::get<0>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				auto* delegate = controller->getDelegate();

				// Notify handlers
				answerCallback.invoke<SetConfigurationHandler>(controller, targetID, status, configurationIndex);
				if (aem.getUnsolicited() && delegate && !!status)
				{
					invokeProtectedMethod(&ControllerEntity::Delegate::onConfigurationChanged, delegate, controller, targetID, configurationIndex);
				}
			}
		},
		// Get Configuration
		// Set Stream Format
		{ protocol::AemCommandType::SetStreamFormat.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, streamFormat] = protocol::aemPayload::deserializeSetStreamFormatResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetStreamFormatResponse(aem.getPayload());
				model::DescriptorType const descriptorType = std::get<0>(result);
				model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::StreamFormat const streamFormat = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				auto* delegate = controller->getDelegate();

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<SetStreamInputFormatHandler>(controller, targetID, status, descriptorIndex, streamFormat);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamInputFormatChanged, delegate, controller, targetID, descriptorIndex, streamFormat);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<SetStreamOutputFormatHandler>(controller, targetID, status, descriptorIndex, streamFormat);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamOutputFormatChanged, delegate, controller, targetID, descriptorIndex, streamFormat);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Get Stream Format
		{ protocol::AemCommandType::GetStreamFormat.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, streamFormat] = protocol::aemPayload::deserializeGetStreamFormatResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetStreamFormatResponse(aem.getPayload());
				model::DescriptorType const descriptorType = std::get<0>(result);
				model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::StreamFormat const streamFormat = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<GetStreamInputFormatHandler>(controller, targetID, status, descriptorIndex, streamFormat);
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<GetStreamOutputFormatHandler>(controller, targetID, status, descriptorIndex, streamFormat);
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Set Stream Info
		// Get Stream Info
		{ protocol::AemCommandType::GetStreamInfo.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, streamInfo] = protocol::aemPayload::deserializeGetStreamInfoResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetStreamInfoResponse(aem.getPayload());
				model::DescriptorType const descriptorType = std::get<0>(result);
				model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::StreamInfo const streamInfo = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				auto* delegate = controller->getDelegate();

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<GetStreamInputInfoHandler>(controller, targetID, status, descriptorIndex, streamInfo);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamInputInfoChanged, delegate, controller, targetID, descriptorIndex, streamInfo);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<GetStreamOutputInfoHandler>(controller, targetID, status, descriptorIndex, streamInfo);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamOutputInfoChanged, delegate, controller, targetID, descriptorIndex, streamInfo);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Set Name
		{ protocol::AemCommandType::SetName.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, nameIndex, configurationIndex, name] = protocol::aemPayload::deserializeSetNameResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetNameResponse(aem.getPayload());
				model::DescriptorType const descriptorType = std::get<0>(result);
				model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				std::uint16_t const nameIndex = std::get<2>(result);
				model::ConfigurationIndex const configurationIndex = std::get<3>(result);
				model::AvdeccFixedString const name = std::get<4>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				auto* delegate = controller->getDelegate();

				// Notify handlers
				switch (descriptorType)
				{
					case model::DescriptorType::Entity:
					{
						if (descriptorIndex != 0)
						{
							LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Invalid descriptorIndex in SET_NAME response for Entity Descriptor: {}", descriptorIndex);
						}
						if (configurationIndex != 0)
						{
							LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Invalid configurationIndex in SET_NAME response for Entity Descriptor: {}", configurationIndex);
						}
						switch (nameIndex)
						{
							case 0: // entity_name
								answerCallback.invoke<SetEntityNameHandler>(controller, targetID, status);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									invokeProtectedMethod(&ControllerEntity::Delegate::onEntityNameChanged, delegate, controller, targetID, name);
								}
								break;
							case 1: // group_name
								answerCallback.invoke<SetEntityGroupNameHandler>(controller, targetID, status);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									invokeProtectedMethod(&ControllerEntity::Delegate::onEntityGroupNameChanged, delegate, controller, targetID, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for Entity Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::Configuration:
					{
						if (configurationIndex != 0)
						{
							LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Invalid configurationIndex in SET_NAME response for Configuration Descriptor: ConfigurationIndex={}", configurationIndex);
						}
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<SetConfigurationNameHandler>(controller, targetID, status, descriptorIndex);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									invokeProtectedMethod(&ControllerEntity::Delegate::onConfigurationNameChanged, delegate, controller, targetID, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for Configuration Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AudioUnit:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<SetAudioUnitNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									invokeProtectedMethod(&ControllerEntity::Delegate::onAudioUnitNameChanged, delegate, controller, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for AudioUnit Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::StreamInput:
					{
						switch (nameIndex)
						{
							case 0: // stream_name
								answerCallback.invoke<SetStreamInputNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									invokeProtectedMethod(&ControllerEntity::Delegate::onStreamInputNameChanged, delegate, controller, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for StreamInput Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::StreamOutput:
					{
						switch (nameIndex)
						{
							case 0: // stream_name
								answerCallback.invoke<SetStreamOutputNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									invokeProtectedMethod(&ControllerEntity::Delegate::onStreamOutputNameChanged, delegate, controller, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for StreamOutput Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AvbInterface:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<SetAvbInterfaceNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									invokeProtectedMethod(&ControllerEntity::Delegate::onAvbInterfaceNameChanged, delegate, controller, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for AvbInterface Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::ClockSource:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<SetClockSourceNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									invokeProtectedMethod(&ControllerEntity::Delegate::onClockSourceNameChanged, delegate, controller, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for ClockSource Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::MemoryObject:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<SetMemoryObjectNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									invokeProtectedMethod(&ControllerEntity::Delegate::onMemoryObjectNameChanged, delegate, controller, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for MemoryObject Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AudioCluster:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<SetAudioClusterNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									invokeProtectedMethod(&ControllerEntity::Delegate::onAudioClusterNameChanged, delegate, controller, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for AudioCluster Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::ClockDomain:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<SetClockDomainNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									invokeProtectedMethod(&ControllerEntity::Delegate::onClockDomainNameChanged, delegate, controller, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for ClockDomain Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					default:
						LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled descriptorType in SET_NAME response: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
						break;
				}
			}
		},
		// Get Name
		{ protocol::AemCommandType::GetName.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, nameIndex, configurationIndex, name] = protocol::aemPayload::deserializeGetNameResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetNameResponse(aem.getPayload());
				model::DescriptorType const descriptorType = std::get<0>(result);
				model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				std::uint16_t const nameIndex = std::get<2>(result);
				model::ConfigurationIndex const configurationIndex = std::get<3>(result);
				model::AvdeccFixedString const name = std::get<4>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				switch (descriptorType)
				{
					case model::DescriptorType::Entity:
					{
						if (descriptorIndex != 0)
						{
							LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Invalid descriptorIndex in GET_NAME response for Entity Descriptor: DescriptorIndex={}", descriptorIndex);
						}
						if (configurationIndex != 0)
						{
							LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Invalid configurationIndex in GET_NAME response for Entity Descriptor: ConfigurationIndex={}", configurationIndex);
						}
						switch (nameIndex)
						{
							case 0: // entity_name
								answerCallback.invoke<GetEntityNameHandler>(controller, targetID, status, name);
								break;
							case 1: // group_name
								answerCallback.invoke<GetEntityGroupNameHandler>(controller, targetID, status, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for Entity Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::Configuration:
					{
						if (configurationIndex != 0)
						{
							LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Invalid configurationIndex in GET_NAME response for Configuration Descriptor: ConfigurationIndex={}", configurationIndex);
						}
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<GetConfigurationNameHandler>(controller, targetID, status, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for Configuration Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AudioUnit:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<GetAudioUnitNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for AudioUnit Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::StreamInput:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<GetStreamInputNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for StreamInput Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::StreamOutput:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<GetStreamOutputNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for StreamOutput Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AvbInterface:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<GetAvbInterfaceNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for AvbInterface Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::ClockSource:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<GetClockSourceNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for ClockSource Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::MemoryObject:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<GetMemoryObjectNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for MemoryObject Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AudioCluster:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<GetAudioClusterNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for AudioCluster Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::ClockDomain:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<GetClockDomainNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for ClockDomain Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					default:
						LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled descriptorType in GET_NAME response: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
						break;
				}
			}
		},
		// Set Sampling Rate
		{ protocol::AemCommandType::SetSamplingRate.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, samplingRate] = protocol::aemPayload::deserializeSetSamplingRateResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetSamplingRateResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::SamplingRate const samplingRate = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				auto* delegate = controller->getDelegate();

				// Notify handlers
				if (descriptorType == model::DescriptorType::AudioUnit)
				{
					answerCallback.invoke<SetAudioUnitSamplingRateHandler>(controller, targetID, status, descriptorIndex, samplingRate);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onAudioUnitSamplingRateChanged, delegate, controller, targetID, descriptorIndex, samplingRate);
					}
				}
				else if (descriptorType == model::DescriptorType::VideoCluster)
				{
					answerCallback.invoke<SetVideoClusterSamplingRateHandler>(controller, targetID, status, descriptorIndex, samplingRate);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onVideoClusterSamplingRateChanged, delegate, controller, targetID, descriptorIndex, samplingRate);
					}
				}
				else if (descriptorType == model::DescriptorType::SensorCluster)
				{
					answerCallback.invoke<SetSensorClusterSamplingRateHandler>(controller, targetID, status, descriptorIndex, samplingRate);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onSensorClusterSamplingRateChanged, delegate, controller, targetID, descriptorIndex , samplingRate);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Get Sampling Rate
		{ protocol::AemCommandType::GetSamplingRate.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, samplingRate] = protocol::aemPayload::deserializeGetSamplingRateResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetSamplingRateResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::SamplingRate const samplingRate = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				if (descriptorType == model::DescriptorType::AudioUnit)
				{
					answerCallback.invoke<GetAudioUnitSamplingRateHandler>(controller, targetID, status, descriptorIndex, samplingRate);
				}
				else if (descriptorType == model::DescriptorType::VideoCluster)
				{
					answerCallback.invoke<GetVideoClusterSamplingRateHandler>(controller, targetID, status, descriptorIndex, samplingRate);
				}
				else if (descriptorType == model::DescriptorType::SensorCluster)
				{
					answerCallback.invoke<GetSensorClusterSamplingRateHandler>(controller, targetID, status, descriptorIndex, samplingRate);
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Set Clock Source
		{ protocol::AemCommandType::SetClockSource.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, clockSourceIndex] = protocol::aemPayload::deserializeSetClockSourceResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetClockSourceResponse(aem.getPayload());
				//entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::ClockSourceIndex const clockSourceIndex = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				auto* delegate = controller->getDelegate();

				// Notify handlers
				answerCallback.invoke<SetClockSourceHandler>(controller, targetID, status, descriptorIndex, clockSourceIndex);
				if (aem.getUnsolicited() && delegate && !!status)
				{
					invokeProtectedMethod(&ControllerEntity::Delegate::onClockSourceChanged, delegate, controller, targetID, descriptorIndex, clockSourceIndex);
				}
			}
		},
		// Get Clock Source
		{ protocol::AemCommandType::GetClockSource.getValue(),[](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, clockSourceIndex] = protocol::aemPayload::deserializeGetClockSourceResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetClockSourceResponse(aem.getPayload());
				//entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::ClockSourceIndex const clockSourceIndex = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<GetClockSourceHandler>(controller, targetID, status, descriptorIndex, clockSourceIndex);
			}
		},
		// Start Streaming
		{ protocol::AemCommandType::StartStreaming.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex] = protocol::aemPayload::deserializeStartStreamingResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeStartStreamingResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				auto* delegate = controller->getDelegate();

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<StartStreamInputHandler>(controller, targetID, status, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamInputStarted, delegate, controller, targetID, descriptorIndex);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<StartStreamOutputHandler>(controller, targetID, status, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamOutputStarted, delegate, controller, targetID, descriptorIndex);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Stop Streaming
		{ protocol::AemCommandType::StopStreaming.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex] = protocol::aemPayload::deserializeStopStreamingResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeStopStreamingResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				auto* delegate = controller->getDelegate();

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<StopStreamInputHandler>(controller, targetID, status, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamInputStopped, delegate, controller, targetID, descriptorIndex);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<StopStreamOutputHandler>(controller, targetID, status, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamOutputStopped, delegate, controller, targetID, descriptorIndex);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Register Unsolicited Notifications
		{ protocol::AemCommandType::RegisterUnsolicitedNotification.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Ignore payload size and content, Apple's implementation is bugged and returns too much data
				auto const targetID = aem.getTargetEntityID();
				answerCallback.invoke<RegisterUnsolicitedNotificationsHandler>(controller, targetID, status);
			}
		},
		// Unregister Unsolicited Notifications
		{ protocol::AemCommandType::DeregisterUnsolicitedNotification.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Ignore payload size and content, Apple's implementation is bugged and returns too much data
				auto const targetID = aem.getTargetEntityID();
				answerCallback.invoke<UnregisterUnsolicitedNotificationsHandler>(controller, targetID, status);
			}
		},
		// GetAVBInfo
		{ protocol::AemCommandType::GetAvbInfo.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, avbInfo] = protocol::aemPayload::deserializeGetAvbInfoResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetAvbInfoResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::AvbInfo const avbInfo = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				auto* delegate = controller->getDelegate();

				// Notify handlers
				if (descriptorType == model::DescriptorType::AvbInterface)
				{
					answerCallback.invoke<GetAvbInfoHandler>(controller, targetID, status, descriptorIndex, avbInfo);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onAvbInfoChanged, delegate, controller, targetID, descriptorIndex, avbInfo);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// GetASPath
		// GetCounters
		// Get Audio Map
		{ protocol::AemCommandType::GetAudioMap.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, mapIndex, numberOfMaps, mappings] = protocol::aemPayload::deserializeGetAudioMapResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetAudioMapResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::MapIndex const mapIndex = std::get<2>(result);
				entity::model::MapIndex const numberOfMaps = std::get<3>(result);
				entity::model::AudioMappings const& mappings = std::get<4>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				auto* delegate = controller->getDelegate();

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamPortInput)
				{
					answerCallback.invoke<GetStreamPortInputAudioMapHandler>(controller, targetID, status, descriptorIndex, numberOfMaps, mapIndex, mappings);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamPortInputAudioMappingsChanged, delegate, controller, targetID, descriptorIndex, numberOfMaps, mapIndex, mappings);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamPortOutput)
				{
					answerCallback.invoke<GetStreamPortOutputAudioMapHandler>(controller, targetID, status, descriptorIndex, numberOfMaps, mapIndex, mappings);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamPortOutputAudioMappingsChanged, delegate, controller, targetID, descriptorIndex, numberOfMaps, mapIndex, mappings);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Add Audio Mappings
		{ protocol::AemCommandType::AddAudioMappings.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, mappings] = protocol::aemPayload::deserializeAddAudioMappingsResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeAddAudioMappingsResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::AudioMappings const& mappings = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
#pragma message("TODO: Handle unsolicited notification (add handler, and handle it in controller code)")
				if (descriptorType == model::DescriptorType::StreamPortInput)
				{
					answerCallback.invoke<AddStreamPortInputAudioMappingsHandler>(controller, targetID, status, descriptorIndex, mappings);
				}
				else if (descriptorType == model::DescriptorType::StreamPortOutput)
				{
					answerCallback.invoke<AddStreamPortOutputAudioMappingsHandler>(controller, targetID, status, descriptorIndex, mappings);
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Remove Audio Mappings
		{ protocol::AemCommandType::RemoveAudioMappings.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, mappings] = protocol::aemPayload::deserializeRemoveAudioMappingsResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeRemoveAudioMappingsResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::AudioMappings const& mappings = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
#pragma message("TODO: Handle unsolicited notification (add handler, and handle it in controller code)")
				if (descriptorType == model::DescriptorType::StreamPortInput)
				{
					answerCallback.invoke<RemoveStreamPortInputAudioMappingsHandler>(controller, targetID, status, descriptorIndex, mappings);
				}
				else if (descriptorType == model::DescriptorType::StreamPortOutput)
				{
					answerCallback.invoke<RemoveStreamPortOutputAudioMappingsHandler>(controller, targetID, status, descriptorIndex, mappings);
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Set Memory Object Length
		{ protocol::AemCommandType::SetMemoryObjectLength.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[configurationIndex, memoryObjectIndex, length] = protocol::aemPayload::deserializeSetMemoryObjectLengthResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetMemoryObjectLengthResponse(aem.getPayload());
				entity::model::ConfigurationIndex const configurationIndex = std::get<0>(result);
				entity::model::MemoryObjectIndex const memoryObjectIndex = std::get<1>(result);
				std::uint64_t const length = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				auto* delegate = controller->getDelegate();

				// Notify handlers
				answerCallback.invoke<SetMemoryObjectLengthHandler>(controller, targetID, status, configurationIndex, memoryObjectIndex, length);
				if (aem.getUnsolicited() && delegate && !!status)
				{
					invokeProtectedMethod(&ControllerEntity::Delegate::onMemoryObjectLengthChanged, delegate, controller, targetID, configurationIndex, memoryObjectIndex, length);
				}
			}
		},
		// Get Memory Object Length
		{ protocol::AemCommandType::GetMemoryObjectLength.getValue(),[](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[configurationIndex, memoryObjectIndex, length] = protocol::aemPayload::deserializeGetMemoryObjectLengthResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetMemoryObjectLengthResponse(aem.getPayload());
				entity::model::ConfigurationIndex const configurationIndex = std::get<0>(result);
				entity::model::MemoryObjectIndex const memoryObjectIndex = std::get<1>(result);
				std::uint64_t const length = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<GetMemoryObjectLengthHandler>(controller, targetID, status, configurationIndex, memoryObjectIndex, length);
			}
		},
		// Set Stream Backup
		// Get Stream Backup
	};

	auto const& it = s_Dispatch.find(aem.getCommandType().getValue());
	if (it == s_Dispatch.end())
	{
		// If this is an unsolicited notification, simply log we do not handle the message
		if (aem.getUnsolicited())
		{
			LOG_CONTROLLER_ENTITY_DEBUG(aem.getTargetEntityID(), "Unsolicited AEM response {} not handled ({})", std::string(aem.getCommandType()), toHexString(aem.getCommandType().getValue()));
		}
		// But if it's an expected response, this is an internal error since we sent a command and didn't implement the code to handle the response
		else
		{
			LOG_CONTROLLER_ENTITY_ERROR(aem.getTargetEntityID(), "Failed to process AEM response: Unhandled command type {} ({})", std::string(aem.getCommandType()), toHexString(aem.getCommandType().getValue()));
			invokeProtectedHandler(onErrorCallback, AemCommandStatus::InternalError);
		}
	}
	else
	{
		auto checkProcessInvalidNonSuccessResponse = [status, &aem, &onErrorCallback](char const* const what)
		{
			auto st = AemCommandStatus::ProtocolError;
#if defined(IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES)
			if (status != AemCommandStatus::Success)
			{
				// Allow this packet to go through as a non-success response, but some fields might have the default initial value which might not be valid (the spec says even in a response message, some fields have a meaningful value)
				st = status;
				LOG_CONTROLLER_ENTITY_INFO(aem.getTargetEntityID(), "Received an invalid non-success {} AEM response ({}) from {} but still processing it because of compilation option IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES", std::string(aem.getCommandType()), what, toHexString(aem.getTargetEntityID(), true));
			}
#endif // IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES
			if (st == AemCommandStatus::ProtocolError)
			{
				(void)what;
				LOG_CONTROLLER_ENTITY_INFO(aem.getTargetEntityID(), "Failed to process {} AEM response: {}", std::string(aem.getCommandType()), what);
			}
			invokeProtectedHandler(onErrorCallback, st);
		};

		try
		{
			it->second(this, status, aem, answerCallback);
		}
		catch (protocol::aemPayload::IncorrectPayloadSizeException const& e)
		{
			checkProcessInvalidNonSuccessResponse(e.what());
			return;
		}
		catch (InvalidDescriptorTypeException const& e)
		{
			checkProcessInvalidNonSuccessResponse(e.what());
			return;
		}
		catch ([[maybe_unused]] std::exception const& e) // Mainly unpacking errors
		{
			LOG_CONTROLLER_ENTITY_INFO(aem.getTargetEntityID(), "Failed to process {} AEM response: {}", std::string(aem.getCommandType()), e.what());
			invokeProtectedHandler(onErrorCallback, AemCommandStatus::ProtocolError);
			return;
		}
	}
}

void ControllerEntityImpl::sendAemResponse(protocol::AemAecpdu const& commandAem, protocol::AecpStatus const status, void const* const payload, size_t const payloadLength) const noexcept
{
	try
	{
		auto* pi = getProtocolInterface();
		auto targetMacAddress = networkInterface::MacAddress{};

		// Build AEM-AECPDU frame
		auto frame = protocol::AemAecpdu::create();
		auto* aem = static_cast<protocol::AemAecpdu*>(frame.get());

		// Set Ether2 fields
		if (commandAem.getDestAddress() != pi->getMacAddress())
		{
			LOG_CONTROLLER_ENTITY_WARN(commandAem.getTargetEntityID(), "Sending AEM response using own MacAddress as source, instead of the incorrect one from the AEM command");
		}
		aem->setSrcAddress(pi->getMacAddress()); // Using our MacAddress instead of the one from the Command, some devices incorrectly send some AEM messages to the multicast Ether2 MacAddress instead of targeting an entity
		aem->setDestAddress(commandAem.getSrcAddress());
		// Set AECP fields
		aem->setMessageType(protocol::AecpMessageType::AemResponse);
		aem->setStatus(status);
		aem->setTargetEntityID(commandAem.getTargetEntityID());
		aem->setControllerEntityID(commandAem.getControllerEntityID());
		aem->setSequenceID(commandAem.getSequenceID());
		// Set AEM fields
		aem->setUnsolicited(false);
		aem->setCommandType(commandAem.getCommandType());
		aem->setCommandSpecificData(payload, payloadLength);

		// We don't care about the send errors
		pi->sendAecpResponse(std::move(frame), targetMacAddress);
	}
	catch (...)
	{
	}
}

void ControllerEntityImpl::sendAcmpCommand(protocol::AcmpMessageType const messageType, UniqueIdentifier const talkerEntityID, model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, model::StreamIndex const listenerStreamIndex, uint16_t const connectionIndex, OnACMPErrorCallback const& onErrorCallback, AnswerCallback const& answerCallback) const noexcept
{
	try
	{
		auto* pi = getProtocolInterface();

		// Build ACMPDU frame
		auto frame = protocol::Acmpdu::create();
		auto* acmp = static_cast<protocol::Acmpdu*>(frame.get());

		// Set Ether2 fields
		acmp->setSrcAddress(pi->getMacAddress());
		// No need to set DestAddress, it's always the MultiCast address
		// Set AVTP fields
		acmp->setStreamID(0u);
		// Set ACMP fields
		acmp->setMessageType(messageType);
		acmp->setStatus(protocol::AcmpStatus::Success);
		acmp->setControllerEntityID(getEntityID());
		acmp->setTalkerEntityID(talkerEntityID);
		acmp->setListenerEntityID(listenerEntityID);
		acmp->setTalkerUniqueID(talkerStreamIndex);
		acmp->setListenerUniqueID(listenerStreamIndex);
		acmp->setStreamDestAddress({});
		acmp->setConnectionCount(connectionIndex);
		// No need to set the SequenceID, it's set by the ProtocolInterface layer
		acmp->setFlags(ConnectionFlags::None);
		acmp->setStreamVlanID(0);

		auto const error = pi->sendAcmpCommand(std::move(frame), [onErrorCallback, answerCallback, this](protocol::Acmpdu const* const response, protocol::ProtocolInterface::Error const error) noexcept
		{
			if (!error)
			{
				processAcmpResponse(response, onErrorCallback, answerCallback, false);
			}
			else
			{
				invokeProtectedHandler(onErrorCallback, convertErrorToControlStatus(error));
			}
		});
		if (!!error)
		{
			invokeProtectedHandler(onErrorCallback, convertErrorToControlStatus(error));
		}
	}
	catch (std::invalid_argument const&)
	{
		invokeProtectedHandler(onErrorCallback, ControlStatus::ProtocolError);
	}
	catch (...)
	{
		invokeProtectedHandler(onErrorCallback, ControlStatus::InternalError);
	}
}

void ControllerEntityImpl::processAcmpResponse(protocol::Acmpdu const* const response, OnACMPErrorCallback const& onErrorCallback, AnswerCallback const& answerCallback, bool const sniffed) const noexcept
{
	auto const& acmp = static_cast<protocol::Acmpdu const&>(*response);
	auto const status = static_cast<ControllerEntity::ControlStatus>(acmp.getStatus().getValue()); // We have to convert protocol status to our extended status

	static std::unordered_map<protocol::AcmpMessageType::value_type, std::function<void(ControllerEntityImpl const* const controller, ControllerEntity::ControlStatus const status, protocol::Acmpdu const& acmp, AnswerCallback const& answerCallback, bool const sniffed)>> s_Dispatch{
		// Connect TX response
		{ protocol::AcmpMessageType::ConnectTxResponse.getValue(), [](ControllerEntityImpl const* const controller, ControllerEntity::ControlStatus const status, protocol::Acmpdu const& acmp, AnswerCallback const& /*answerCallback*/, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				auto* delegate = controller->getDelegate();
				if (sniffed && delegate)
				{
					invokeProtectedMethod(&ControllerEntity::Delegate::onListenerConnectResponseSniffed, delegate, controller, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			}
		},
		// Disconnect TX response
		{ protocol::AcmpMessageType::DisconnectTxResponse.getValue(), [](ControllerEntityImpl const* const controller, ControllerEntity::ControlStatus const status, protocol::Acmpdu const& acmp, AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<DisconnectTalkerStreamHandler>(controller, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				auto* delegate = controller->getDelegate();
				if (sniffed && delegate)
				{
					invokeProtectedMethod(&ControllerEntity::Delegate::onListenerDisconnectResponseSniffed, delegate, controller, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			}
		},
		// Get TX state response
		{ protocol::AcmpMessageType::GetTxStateResponse.getValue(), [](ControllerEntityImpl const* const controller, ControllerEntity::ControlStatus const status, protocol::Acmpdu const& acmp, AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<GetTalkerStreamStateHandler>(controller, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				auto* delegate = controller->getDelegate();
				if (sniffed && delegate)
				{
					invokeProtectedMethod(&ControllerEntity::Delegate::onGetTalkerStreamStateResponseSniffed, delegate, controller, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			}
		},
		// Connect RX response
		{ protocol::AcmpMessageType::ConnectRxResponse.getValue(), [](ControllerEntityImpl const* const controller, ControllerEntity::ControlStatus const status, protocol::Acmpdu const& acmp, AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<ConnectStreamHandler>(controller, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				auto* delegate = controller->getDelegate();
				if (sniffed && delegate)
				{
					invokeProtectedMethod(&ControllerEntity::Delegate::onControllerConnectResponseSniffed, delegate, controller, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			}
		},
		// Disconnect RX response
		{ protocol::AcmpMessageType::DisconnectRxResponse.getValue(), [](ControllerEntityImpl const* const controller, ControllerEntity::ControlStatus const status, protocol::Acmpdu const& acmp, AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<DisconnectStreamHandler>(controller, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				auto* delegate = controller->getDelegate();
				if (sniffed && delegate)
				{
					invokeProtectedMethod(&ControllerEntity::Delegate::onControllerDisconnectResponseSniffed, delegate, controller, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			}
		},
		// Get RX state response
		{ protocol::AcmpMessageType::GetRxStateResponse.getValue(), [](ControllerEntityImpl const* const controller, ControllerEntity::ControlStatus const status, protocol::Acmpdu const& acmp, AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<GetListenerStreamStateHandler>(controller, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				auto* delegate = controller->getDelegate();
				if (sniffed && delegate)
				{
					invokeProtectedMethod(&ControllerEntity::Delegate::onGetListenerStreamStateResponseSniffed, delegate, controller, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			}
		},
		// Get TX connection response
		{ protocol::AcmpMessageType::GetTxConnectionResponse.getValue(), [](ControllerEntityImpl const* const controller, ControllerEntity::ControlStatus const status, protocol::Acmpdu const& acmp, AnswerCallback const& answerCallback, bool const /*sniffed*/)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<GetTalkerStreamConnectionHandler>(controller, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
			}
		},
	};
	
	auto const& it = s_Dispatch.find(acmp.getMessageType().getValue());
	if (it == s_Dispatch.end())
	{
		// If this is a sniffed message, simply log we do not handle the message
		if (sniffed)
		{
			LOG_CONTROLLER_ENTITY_DEBUG(acmp.getTalkerEntityID(), "ACMP response {} not handled ({})", std::string(acmp.getMessageType()), toHexString(acmp.getMessageType().getValue()));
		}
		// But if it's an expected response, this is an internal error since we sent a command and didn't implement the code to handle the response
		else
		{
			LOG_CONTROLLER_ENTITY_ERROR(acmp.getTalkerEntityID(), "Failed to process ACMP response: Unhandled message type {} ({})", std::string(acmp.getMessageType()), toHexString(acmp.getMessageType().getValue()));
			invokeProtectedHandler(onErrorCallback, ControlStatus::InternalError);
		}
	}
	else
	{
		try
		{
			it->second(this, status, acmp, answerCallback, sniffed);
		}
		catch (ControlException const& e)
		{
			auto st = e.getStatus();
			LOG_CONTROLLER_ENTITY_INFO(acmp.getTalkerEntityID(), "Failed to process ACMP response: {}", e.what());
			invokeProtectedHandler(onErrorCallback, st);
			return;
		}
		catch ([[maybe_unused]] std::exception const& e) // Mainly unpacking errors
		{
			LOG_CONTROLLER_ENTITY_INFO(acmp.getTalkerEntityID(), "Failed to process ACMP response: {}", e.what());
			invokeProtectedHandler(onErrorCallback, ControlStatus::ProtocolError);
			return;
		}
	}
}

/* ************************************************************************** */
/* ControllerEntity overrides                                                 */
/* ************************************************************************** */
/* Discovery Protocol (ADP) */

/* Enumeration and Control Protocol (AECP) */
void ControllerEntityImpl::acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, AcquireEntityHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeAcquireEntityCommand(isPersistent ? protocol::AemAcquireEntityFlags::Persistent : protocol::AemAcquireEntityFlags::None, UniqueIdentifier::getNullUniqueIdentifier(), descriptorType, descriptorIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, UniqueIdentifier::getNullUniqueIdentifier(), descriptorType, descriptorIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::AcquireEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize acquireEntity: {}", e.what());
	}
}

void ControllerEntityImpl::releaseEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, ReleaseEntityHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeAcquireEntityCommand(protocol::AemAcquireEntityFlags::Release, UniqueIdentifier::getNullUniqueIdentifier(), descriptorType, descriptorIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, UniqueIdentifier::getNullUniqueIdentifier(), descriptorType, descriptorIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::AcquireEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize releaseEntity: {}", e.what());
	}
}

void ControllerEntityImpl::lockEntity(UniqueIdentifier const targetEntityID, LockEntityHandler const& handler) const noexcept
{
#pragma message("TODO: Change the API to allow partial EM lock")
	try
	{
		auto const ser = protocol::aemPayload::serializeLockEntityCommand(protocol::AemLockEntityFlags::None, UniqueIdentifier::getNullUniqueIdentifier(), model::DescriptorType::Entity, 0);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, UniqueIdentifier::getNullUniqueIdentifier());

		sendAemCommand(targetEntityID, protocol::AemCommandType::LockEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize lockEntity: {}", e.what());
	}
}

void ControllerEntityImpl::unlockEntity(UniqueIdentifier const targetEntityID, UnlockEntityHandler const& handler) const noexcept
{
#pragma message("TODO: Change the API to allow partial EM unlock")
	try
	{
		auto const ser = protocol::aemPayload::serializeLockEntityCommand(protocol::AemLockEntityFlags::Unlock, UniqueIdentifier::getNullUniqueIdentifier(), model::DescriptorType::Entity, 0);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1);

		sendAemCommand(targetEntityID, protocol::AemCommandType::LockEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize unlockEntity: {}", e.what());
	}
}

void ControllerEntityImpl::queryEntityAvailable(UniqueIdentifier const targetEntityID, QueryEntityAvailableHandler const& handler) const noexcept
{
	auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1);

	sendAemCommand(targetEntityID, protocol::AemCommandType::EntityAvailable, nullptr, 0, errorCallback, handler);
}

void ControllerEntityImpl::queryControllerAvailable(UniqueIdentifier const targetEntityID, QueryControllerAvailableHandler const& handler) const noexcept
{
	auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1);

	sendAemCommand(targetEntityID, protocol::AemCommandType::ControllerAvailable, nullptr, 0, errorCallback, handler);
}

void ControllerEntityImpl::registerUnsolicitedNotifications(UniqueIdentifier const targetEntityID, RegisterUnsolicitedNotificationsHandler const& handler) const noexcept
{
	auto errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1);

	sendAemCommand(targetEntityID, protocol::AemCommandType::RegisterUnsolicitedNotification, nullptr, 0, errorCallback, handler);
}

void ControllerEntityImpl::unregisterUnsolicitedNotifications(UniqueIdentifier const targetEntityID, UnregisterUnsolicitedNotificationsHandler const& handler) const noexcept
{
	auto errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1);

	sendAemCommand(targetEntityID, protocol::AemCommandType::DeregisterUnsolicitedNotification, nullptr, 0, errorCallback, handler);
}

void ControllerEntityImpl::readEntityDescriptor(UniqueIdentifier const targetEntityID, EntityDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(model::ConfigurationIndex(0u), model::DescriptorType::Entity, model::DescriptorIndex(0u));
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, model::EntityDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readEntityDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readConfigurationDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, ConfigurationDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(model::ConfigurationIndex(0u), model::DescriptorType::Configuration, static_cast<model::DescriptorIndex>(configurationIndex)); // Passing configurationIndex as a DescriptorIndex is NOT an error. See 7.4.5.1
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, model::ConfigurationDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readConfigurationDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readAudioUnitDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, AudioUnitDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::AudioUnit, audioUnitIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, audioUnitIndex, model::AudioUnitDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readAudioUnitDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readStreamInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, StreamInputDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::StreamInput, streamIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, streamIndex, model::StreamDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readStreamInputDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readStreamOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, StreamOutputDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::StreamOutput, streamIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, streamIndex, model::StreamDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readStreamOutputDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readJackInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, JackInputDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::JackInput, jackIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, jackIndex, model::JackDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readJackInputDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readJackOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, JackOutputDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::JackOutput, jackIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, jackIndex, model::JackDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readJackOutputDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readAvbInterfaceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, AvbInterfaceDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::AvbInterface, avbInterfaceIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, avbInterfaceIndex, model::AvbInterfaceDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readAvbInterfaceDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readClockSourceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, ClockSourceDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::ClockSource, clockSourceIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, clockSourceIndex, model::ClockSourceDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readClockSourceDescriptor: '}", e.what());
	}
}

void ControllerEntityImpl::readMemoryObjectDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, MemoryObjectDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::MemoryObject, memoryObjectIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, memoryObjectIndex, model::MemoryObjectDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readMemoryObjectDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readLocaleDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::LocaleIndex const localeIndex, LocaleDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::Locale, localeIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, localeIndex, model::LocaleDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readLocaleDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readStringsDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StringsIndex const stringsIndex, StringsDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::Strings, stringsIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, stringsIndex, model::StringsDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readStringsDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readStreamPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamPortIndex const streamPortIndex, StreamPortInputDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::StreamPortInput, streamPortIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, streamPortIndex, model::StreamPortDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readStreamPortInputDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readStreamPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamPortIndex const streamPortIndex, StreamPortOutputDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::StreamPortOutput, streamPortIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, streamPortIndex, model::StreamPortDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readStreamPortOutputDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readExternalPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ExternalPortIndex const externalPortIndex, ExternalPortInputDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::ExternalPortInput, externalPortIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, externalPortIndex, model::ExternalPortDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readExternalPortInputDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readExternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ExternalPortIndex const externalPortIndex, ExternalPortOutputDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::ExternalPortOutput, externalPortIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, externalPortIndex, model::ExternalPortDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readExternalPortInputDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readInternalPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::InternalPortIndex const internalPortIndex, InternalPortInputDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::InternalPortInput, internalPortIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, internalPortIndex, model::InternalPortDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readInternalPortInputDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readInternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::InternalPortIndex const internalPortIndex, InternalPortOutputDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::InternalPortOutput, internalPortIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, internalPortIndex, model::InternalPortDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readInternalPortOutputDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readAudioClusterDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const clusterIndex, AudioClusterDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::AudioCluster, clusterIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, clusterIndex, model::AudioClusterDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readAudioClusterDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readAudioMapDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MapIndex const mapIndex, AudioMapDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::AudioMap, mapIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, mapIndex, model::AudioMapDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readAudioMapDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::readClockDomainDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, ClockDomainDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::ClockDomain, clockDomainIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, clockDomainIndex, model::ClockDomainDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readClockDomainDescriptor: {}", e.what());
	}
}

void ControllerEntityImpl::setConfiguration(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetConfigurationCommand(configurationIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetConfiguration, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setConfiguration: {}", e.what());
	}
}

void ControllerEntityImpl::setStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetStreamFormatCommand(model::DescriptorType::StreamInput, streamIndex, streamFormat);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex, model::StreamFormat());

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetStreamFormat, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setStreamInputFormat: {}", e.what());
	}
}

void ControllerEntityImpl::getStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputFormatHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetStreamFormatCommand(model::DescriptorType::StreamInput, streamIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex, model::StreamFormat());

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetStreamFormat, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getStreamInputFormat: {}", e.what());
	}
}

void ControllerEntityImpl::setStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetStreamFormatCommand(model::DescriptorType::StreamOutput, streamIndex, streamFormat);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex, model::StreamFormat());

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetStreamFormat, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setStreamOutputFormat: {}", e.what());
	}
}

void ControllerEntityImpl::getStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamOutputFormatHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetStreamFormatCommand(model::DescriptorType::StreamOutput, streamIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex, model::StreamFormat());

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetStreamFormat, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getStreamOutputFormat: {}", e.what());
	}
}

void ControllerEntityImpl::getStreamPortInputAudioMap(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::MapIndex const mapIndex, GetStreamPortInputAudioMapHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetAudioMapCommand(model::DescriptorType::StreamPortInput, streamPortIndex, mapIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamPortIndex, model::MapIndex(0), mapIndex, s_emptyMappings);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetAudioMap, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getStreamInputAudioMap: {}", e.what());
	}
}

void ControllerEntityImpl::getStreamPortOutputAudioMap(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::MapIndex const mapIndex, GetStreamPortOutputAudioMapHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetAudioMapCommand(model::DescriptorType::StreamPortOutput, streamPortIndex, mapIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamPortIndex, model::MapIndex(0), mapIndex, s_emptyMappings);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetAudioMap, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getStreamOutputAudioMap: {}", e.what());
	}
}

void ControllerEntityImpl::addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeAddAudioMappingsCommand(model::DescriptorType::StreamPortInput, streamPortIndex, mappings);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamPortIndex, s_emptyMappings);

		sendAemCommand(targetEntityID, protocol::AemCommandType::AddAudioMappings, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize addStreamInputAudioMappings: {}", e.what());
	}
}

void ControllerEntityImpl::addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeAddAudioMappingsCommand(model::DescriptorType::StreamPortOutput, streamPortIndex, mappings);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamPortIndex, s_emptyMappings);

		sendAemCommand(targetEntityID, protocol::AemCommandType::AddAudioMappings, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize addStreamOutputAudioMappings: {}", e.what());
	}
}

void ControllerEntityImpl::removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeRemoveAudioMappingsCommand(model::DescriptorType::StreamPortInput, streamPortIndex, mappings);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamPortIndex, s_emptyMappings);

		sendAemCommand(targetEntityID, protocol::AemCommandType::RemoveAudioMappings, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize removeStreamInputAudioMappings: {}", e.what());
	}
}

void ControllerEntityImpl::removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeRemoveAudioMappingsCommand(model::DescriptorType::StreamPortOutput, streamPortIndex, mappings);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamPortIndex, s_emptyMappings);

		sendAemCommand(targetEntityID, protocol::AemCommandType::RemoveAudioMappings, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize removeStreamOutputAudioMappings: {}", e.what());
	}
}

void ControllerEntityImpl::getStreamInputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputInfoHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetStreamInfoCommand(model::DescriptorType::StreamInput, streamIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex, s_emptyStreamInfo);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetStreamInfo, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getStreamInputInfo: {}", e.what());
	}
}

void ControllerEntityImpl::getStreamOutputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamOutputInfoHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetStreamInfoCommand(model::DescriptorType::StreamOutput, streamIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex, s_emptyStreamInfo);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetStreamInfo, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getStreamOutputInfo: {}", e.what());
	}
}

void ControllerEntityImpl::setEntityName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityName, SetEntityNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::Entity, 0, 0, 0, entityName);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1);

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
	}
}

void ControllerEntityImpl::getEntityName(UniqueIdentifier const targetEntityID, GetEntityNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::Entity, 0, 0, 0);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, s_emptyAvdeccFixedString);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
	}
}

void ControllerEntityImpl::setEntityGroupName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityGroupName, SetEntityGroupNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::Entity, 0, 1, 0, entityGroupName);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1);

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
	}
}

void ControllerEntityImpl::getEntityGroupName(UniqueIdentifier const targetEntityID, GetEntityGroupNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::Entity, 0, 1, 0);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, s_emptyAvdeccFixedString);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
	}
}

void ControllerEntityImpl::setConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& entityGroupName, SetConfigurationNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::Configuration, configurationIndex, 0, 0, entityGroupName);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
	}
}

void ControllerEntityImpl::getConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, GetConfigurationNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::Configuration, configurationIndex, 0, 0);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, s_emptyAvdeccFixedString);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
	}
}

void ControllerEntityImpl::setAudioUnitName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, model::AvdeccFixedString const& audioUnitName, SetAudioUnitNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::AudioUnit, audioUnitIndex, 0, configurationIndex, audioUnitName);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, audioUnitIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
	}
}

void ControllerEntityImpl::getAudioUnitName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const audioUnitIndex, GetAudioUnitNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::AudioUnit, audioUnitIndex, 0, configurationIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, audioUnitIndex, s_emptyAvdeccFixedString);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
	}
}

void ControllerEntityImpl::setStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamInputName, SetStreamInputNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::StreamInput, streamIndex, 0, configurationIndex, streamInputName);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, streamIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
	}
}

void ControllerEntityImpl::getStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, GetStreamInputNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::StreamInput, streamIndex, 0, configurationIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, streamIndex, s_emptyAvdeccFixedString);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
	}
}

void ControllerEntityImpl::setStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamOutputName, SetStreamOutputNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::StreamOutput, streamIndex, 0, configurationIndex, streamOutputName);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, streamIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
	}
}

void ControllerEntityImpl::getStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, GetStreamOutputNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::StreamOutput, streamIndex, 0, configurationIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, streamIndex, s_emptyAvdeccFixedString);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
	}
}

void ControllerEntityImpl::setAvbInterfaceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, model::AvdeccFixedString const& avbInterfaceName, SetAvbInterfaceNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::AvbInterface, avbInterfaceIndex, 0, configurationIndex, avbInterfaceName);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, avbInterfaceIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
	}
}

void ControllerEntityImpl::getAvbInterfaceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const avbInterfaceIndex, GetAvbInterfaceNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::AvbInterface, avbInterfaceIndex, 0, configurationIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, avbInterfaceIndex, s_emptyAvdeccFixedString);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
	}
}

void ControllerEntityImpl::setClockSourceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, model::AvdeccFixedString const& clockSourceName, SetClockSourceNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::ClockSource, clockSourceIndex, 0, configurationIndex, clockSourceName);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, clockSourceIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
	}
}

void ControllerEntityImpl::getClockSourceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const clockSourceIndex, GetClockSourceNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::ClockSource, clockSourceIndex, 0, configurationIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, clockSourceIndex, s_emptyAvdeccFixedString);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
	}
}

void ControllerEntityImpl::setMemoryObjectName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, model::AvdeccFixedString const& memoryObjectName, SetMemoryObjectNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::MemoryObject, memoryObjectIndex, 0, configurationIndex, memoryObjectName);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, memoryObjectIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
	}
}

void ControllerEntityImpl::getMemoryObjectName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const memoryObjectIndex, GetMemoryObjectNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::MemoryObject, memoryObjectIndex, 0, configurationIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, memoryObjectIndex, s_emptyAvdeccFixedString);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
	}
}

void ControllerEntityImpl::setAudioClusterName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const audioClusterIndex, model::AvdeccFixedString const& audioClusterName, SetAudioClusterNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::AudioCluster, audioClusterIndex, 0, configurationIndex, audioClusterName);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, audioClusterIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
	}
}

void ControllerEntityImpl::getAudioClusterName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const audioClusterIndex, GetAudioClusterNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::AudioCluster, audioClusterIndex, 0, configurationIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, audioClusterIndex, s_emptyAvdeccFixedString);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
	}
}

void ControllerEntityImpl::setClockDomainName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, model::AvdeccFixedString const& clockDomainName, SetClockDomainNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::ClockDomain, clockDomainIndex, 0, configurationIndex, clockDomainName);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, clockDomainIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
	}
}

void ControllerEntityImpl::getClockDomainName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const clockDomainIndex, GetClockDomainNameHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::ClockDomain, clockDomainIndex, 0, configurationIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, clockDomainIndex, s_emptyAvdeccFixedString);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
	}
}

void ControllerEntityImpl::setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, model::AudioUnitIndex const audioUnitIndex, model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetSamplingRateCommand(model::DescriptorType::AudioUnit, audioUnitIndex, samplingRate);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, audioUnitIndex, model::getNullSamplingRate());

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetSamplingRate, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setAudioUnitSamplingRate: {}", e.what());
	}
}

void ControllerEntityImpl::getAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, model::AudioUnitIndex const audioUnitIndex, GetAudioUnitSamplingRateHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetSamplingRateCommand(model::DescriptorType::AudioUnit, audioUnitIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, audioUnitIndex, model::getNullSamplingRate());

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetSamplingRate, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getAudioUnitSamplingRate: {}", e.what());
	}
}

void ControllerEntityImpl::setVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const videoClusterIndex, model::SamplingRate const samplingRate, SetVideoClusterSamplingRateHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetSamplingRateCommand(model::DescriptorType::VideoCluster, videoClusterIndex, samplingRate);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, videoClusterIndex, model::getNullSamplingRate());

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetSamplingRate, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setVideoClusterSamplingRate: {}", e.what());
	}
}

void ControllerEntityImpl::getVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const videoClusterIndex, GetVideoClusterSamplingRateHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetSamplingRateCommand(model::DescriptorType::VideoCluster, videoClusterIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, videoClusterIndex, model::getNullSamplingRate());

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetSamplingRate, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getVideoClusterSamplingRate: {}", e.what());
	}
}

void ControllerEntityImpl::setSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const sensorClusterIndex, model::SamplingRate const samplingRate, SetSensorClusterSamplingRateHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetSamplingRateCommand(model::DescriptorType::SensorCluster, sensorClusterIndex, samplingRate);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, sensorClusterIndex, model::getNullSamplingRate());

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetSamplingRate, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setSensorClusterSamplingRate: {}", e.what());
	}
}

void ControllerEntityImpl::getSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const sensorClusterIndex, GetSensorClusterSamplingRateHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetSamplingRateCommand(model::DescriptorType::SensorCluster, sensorClusterIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, sensorClusterIndex, model::getNullSamplingRate());

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetSamplingRate, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getSensorClusterSamplingRate: {}", e.what());
	}
}

void ControllerEntityImpl::setClockSource(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetClockSourceCommand(model::DescriptorType::ClockDomain, clockDomainIndex, clockSourceIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, clockDomainIndex, model::ClockSourceIndex{ 0u });

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetClockSource, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setClockSource: {}", e.what());
	}
}

void ControllerEntityImpl::getClockSource(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, GetClockSourceHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetClockSourceCommand(model::DescriptorType::ClockDomain, clockDomainIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, clockDomainIndex, model::ClockSourceIndex{ 0u });

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetClockSource, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getClockSource: {}", e.what());
	}
}

void ControllerEntityImpl::startStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeStartStreamingCommand(model::DescriptorType::StreamInput, streamIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::StartStreaming, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize startStreamInput: {}", e.what());
	}
}

void ControllerEntityImpl::startStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeStartStreamingCommand(model::DescriptorType::StreamOutput, streamIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::StartStreaming, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize startStreamOutput: {}", e.what());
	}
}

void ControllerEntityImpl::stopStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeStopStreamingCommand(model::DescriptorType::StreamInput, streamIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::StopStreaming, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize stopStreamInput: {}", e.what());
	}
}

void ControllerEntityImpl::stopStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeStopStreamingCommand(model::DescriptorType::StreamOutput, streamIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::StopStreaming, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize stopStreamOutput: {}", e.what());
	}
}

void ControllerEntityImpl::getAvbInfo(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInfoHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetAvbInfoCommand(model::DescriptorType::AvbInterface, avbInterfaceIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, avbInterfaceIndex, s_emptyAvbInfo);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetAvbInfo, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getAvbInfo: {}", e.what());
	}
}

void ControllerEntityImpl::setMemoryObjectLength(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, SetMemoryObjectLengthHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeSetMemoryObjectLengthCommand(configurationIndex, memoryObjectIndex, length);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, memoryObjectIndex, std::uint64_t{ 0u });

		sendAemCommand(targetEntityID, protocol::AemCommandType::SetMemoryObjectLength, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setMemoryObjectLength: {}", e.what());
	}
}

void ControllerEntityImpl::getMemoryObjectLength(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, GetMemoryObjectLengthHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetMemoryObjectLengthCommand(configurationIndex, memoryObjectIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, memoryObjectIndex, std::uint64_t{ 0u });

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetMemoryObjectLength, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getMemoryObjectLength: {}", e.what());
	}
}

/* Connection Management Protocol (ACMP) */
void ControllerEntityImpl::connectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept
{
	auto errorCallback = ControllerEntityImpl::makeACMPErrorHandler(handler, this, talkerStream, listenerStream, std::uint16_t(0), entity::ConnectionFlags::None, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::ConnectRxCommand, talkerStream.entityID, talkerStream.streamIndex, listenerStream.entityID, listenerStream.streamIndex, std::uint16_t(0), errorCallback, handler);
}

void ControllerEntityImpl::disconnectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept
{
	auto errorCallback = ControllerEntityImpl::makeACMPErrorHandler(handler, this, talkerStream, listenerStream, std::uint16_t(0), entity::ConnectionFlags::None, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::DisconnectRxCommand, talkerStream.entityID, talkerStream.streamIndex, listenerStream.entityID, listenerStream.streamIndex, std::uint16_t(0), errorCallback, handler);
}

void ControllerEntityImpl::disconnectTalkerStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept
{
	auto errorCallback = ControllerEntityImpl::makeACMPErrorHandler(handler, this, talkerStream, listenerStream, std::uint16_t(0), entity::ConnectionFlags::None, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::DisconnectTxCommand, talkerStream.entityID, talkerStream.streamIndex, listenerStream.entityID, listenerStream.streamIndex, std::uint16_t(0), errorCallback, handler);
}

void ControllerEntityImpl::getTalkerStreamState(model::StreamIdentification const& talkerStream, GetTalkerStreamStateHandler const& handler) const noexcept
{
	auto errorCallback = ControllerEntityImpl::makeACMPErrorHandler(handler, this, talkerStream, model::StreamIdentification{}, std::uint16_t(0), entity::ConnectionFlags::None, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::GetTxStateCommand, talkerStream.entityID, talkerStream.streamIndex, UniqueIdentifier::getNullUniqueIdentifier(), model::StreamIndex(0), std::uint16_t(0), errorCallback, handler);
}

void ControllerEntityImpl::getListenerStreamState(model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept
{
	auto errorCallback = ControllerEntityImpl::makeACMPErrorHandler(handler, this, model::StreamIdentification{}, listenerStream, std::uint16_t(0), entity::ConnectionFlags::None, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::GetRxStateCommand, UniqueIdentifier::getNullUniqueIdentifier(), model::StreamIndex(0), listenerStream.entityID, listenerStream.streamIndex, std::uint16_t(0), errorCallback, handler);
}
void ControllerEntityImpl::getTalkerStreamConnection(model::StreamIdentification const& talkerStream, uint16_t const connectionIndex, GetTalkerStreamConnectionHandler const& handler) const noexcept
{
	auto errorCallback = ControllerEntityImpl::makeACMPErrorHandler(handler, this, talkerStream, model::StreamIdentification{}, connectionIndex, entity::ConnectionFlags::None, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::GetTxConnectionCommand, talkerStream.entityID, talkerStream.streamIndex, UniqueIdentifier::getNullUniqueIdentifier(), model::StreamIndex(0), connectionIndex, errorCallback, handler);
}

/* Other methods */
void ControllerEntityImpl::setDelegate(Delegate* const delegate) noexcept
{
	_delegate = delegate;
}

ControllerEntityImpl::Delegate* ControllerEntityImpl::getDelegate() const noexcept
{
	return _delegate;
}

/* ************************************************************************** */
/* protocol::ProtocolInterface::Observer overrides                            */
/* ************************************************************************** */
/* **** Global notifications **** */
void ControllerEntityImpl::onTransportError(protocol::ProtocolInterface* const /*pi*/) noexcept
{
	invokeProtectedMethod(&ControllerEntity::Delegate::onTransportError, _delegate, this);
}

/* **** Discovery notifications **** */
void ControllerEntityImpl::onLocalEntityOnline(protocol::ProtocolInterface* const pi, DiscoveredEntity const& entity) noexcept
{
	// The controller doesn't make any difference btw a local and a remote entity, just ignore ourself
	if (getEntityID() != entity.getEntityID())
		onRemoteEntityOnline(pi, entity);
}

void ControllerEntityImpl::onLocalEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept
{
	// The controller doesn't make any difference btw a local and a remote entity, just ignore ourself
	if (getEntityID() != entityID)
		onRemoteEntityOffline(pi, entityID); // The controller doesn't make any difference btw a local and a remote entity
}

void ControllerEntityImpl::onLocalEntityUpdated(protocol::ProtocolInterface* const pi, DiscoveredEntity const& entity) noexcept
{
	// The controller doesn't make any difference btw a local and a remote entity, just ignore ourself
	if (getEntityID() != entity.getEntityID())
		onRemoteEntityUpdated(pi, entity); // The controller doesn't make any difference btw a local and a remote entity
}

void ControllerEntityImpl::onRemoteEntityOnline(protocol::ProtocolInterface* const pi, DiscoveredEntity const& entity) noexcept
{
	auto const entityID = entity.getEntityID();
	{
		// Lock ProtocolInterface
		std::lock_guard<decltype(*pi)> const lg(*pi);

		// Store or replace entity
		{
#pragma message("TODO: Replace with c++17 insert_or_assign")
			auto it = _discoveredEntities.find(entityID);
			if (AVDECC_ASSERT_WITH_RET(it == _discoveredEntities.end(), "ControllerEntityImpl::onRemoteEntityOnline: Entity already online"))
				_discoveredEntities.insert(std::make_pair(entityID, entity));
			else
				it->second = entity;
		}
	}

	invokeProtectedMethod(&ControllerEntity::Delegate::onEntityOnline, _delegate, this, entityID, entity);
}

void ControllerEntityImpl::onRemoteEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept
{
	{
		// Lock ProtocolInterface
		std::lock_guard<decltype(*pi)> const lg(*pi);

		// Remove entity
		_discoveredEntities.erase(entityID);
	}

	invokeProtectedMethod(&ControllerEntity::Delegate::onEntityOffline, _delegate, this, entityID);
}

void ControllerEntityImpl::onRemoteEntityUpdated(protocol::ProtocolInterface* const pi, DiscoveredEntity const& entity) noexcept
{
	auto const entityID = entity.getEntityID();
	{
		// Lock ProtocolInterface
		std::lock_guard<decltype(*pi)> const lg(*pi);

		// Store or replace entity
		{
#pragma message("TODO: Replace with c++17 insert_or_assign")
			auto it = _discoveredEntities.find(entityID);
			if (!AVDECC_ASSERT_WITH_RET(it != _discoveredEntities.end(), "ControllerEntityImpl::onRemoteEntityUpdated: Entity offline"))
				_discoveredEntities.insert(std::make_pair(entityID, entity));
			else
				it->second = entity;
		}
	}

	invokeProtectedMethod(&ControllerEntity::Delegate::onEntityUpdate, _delegate, this, entityID, entity);
}

/* **** AECP notifications **** */
void ControllerEntityImpl::onAecpCommand(protocol::ProtocolInterface* const /*pi*/, entity::LocalEntity const& /*entity*/, protocol::Aecpdu const& aecpdu) noexcept
{
	auto const selfID = getEntityID();
	auto const targetID = aecpdu.getTargetEntityID();

	// Filter messages not for me
	if (!AVDECC_ASSERT_WITH_RET(targetID == selfID, "Should be filtered by controller_state_machine already... on mac too??"))
		return;

	if (aecpdu.getMessageType() == protocol::AecpMessageType::AemCommand)
	{
		auto const& aem = static_cast<protocol::AemAecpdu const&>(aecpdu);
		auto const controllerID = aem.getControllerEntityID();

		// Filter self messages
		if (controllerID == selfID)
			return;

		static std::unordered_map<protocol::AemCommandType::value_type, std::function<void(ControllerEntityImpl const* const controller, protocol::AemAecpdu const& aem)>> s_Dispatch{
			// Entity Available
			{ protocol::AemCommandType::EntityAvailable.getValue(), [](ControllerEntityImpl const* const controller, protocol::AemAecpdu const& aem)
				{
					// We are being asked if we are available, and we are! Reply that
					controller->sendAemResponse(aem, protocol::AemAecpStatus::Success, nullptr, 0u);
				}
			},
			// Controller Available
			{ protocol::AemCommandType::ControllerAvailable.getValue(), [](ControllerEntityImpl const* const controller, protocol::AemAecpdu const& aem)
				{
					// We are being asked if we are available, and we are! Reply that
					controller->sendAemResponse(aem, protocol::AemAecpStatus::Success, nullptr, 0u);
				}
			},
		};

		auto const& it = s_Dispatch.find(aem.getCommandType().getValue());
		if (it != s_Dispatch.end())
		{
			invokeProtectedHandler(it->second, this, aem);
		}
		else
		{
			// Reflect back the payload, and return a NotSupported error code
			auto const payload = aem.getPayload();
			sendAemResponse(aem, protocol::AemAecpStatus::NotSupported, payload.first, payload.second);
		}
	}
}

void ControllerEntityImpl::onAecpUnsolicitedResponse(protocol::ProtocolInterface* const /*pi*/, entity::LocalEntity const& /*entity*/, protocol::Aecpdu const& aecpdu) noexcept
{
	auto const messageType = aecpdu.getMessageType();

	if (messageType == protocol::AecpMessageType::AemResponse)
	{
		auto const& aem = static_cast<protocol::AemAecpdu const&>(aecpdu);
		if (AVDECC_ASSERT_WITH_RET(aem.getUnsolicited(), "Should only be triggered for unsollicited notifications"))
		{
			// Process AEM message without any error or answer callbacks, it's not an expected response
			processAemResponse(&aecpdu, nullptr, {});
		}
	}
}

/* **** ACMP notifications **** */
void ControllerEntityImpl::onAcmpSniffedCommand(protocol::ProtocolInterface* const /*pi*/, entity::LocalEntity const& /*entity*/, protocol::Acmpdu const& /*acmpdu*/) noexcept
{
}

void ControllerEntityImpl::onAcmpSniffedResponse(protocol::ProtocolInterface* const /*pi*/, entity::LocalEntity const& /*entity*/, protocol::Acmpdu const& acmpdu) noexcept
{
	processAcmpResponse(&acmpdu, OnACMPErrorCallback(), AnswerCallback(), true);
}

/* ************************************************************************** */
/* Utility methods                                                            */
/* ************************************************************************** */
std::string LA_AVDECC_CALL_CONVENTION ControllerEntity::statusToString(ControllerEntity::AemCommandStatus const status)
{
	switch (status)
	{
		// AVDECC Error Codes
		case AemCommandStatus::Success:
			return "Success.";
		case AemCommandStatus::NotImplemented:
			return "The AVDECC Entity does not support the command type.";
		case AemCommandStatus::NoSuchDescriptor:
			return "A descriptor with the descriptor_type and descriptor_index specified does not exist.";
		case AemCommandStatus::LockedByOther:
			return "The AVDECC Entity has been locked by another AVDECC Controller.";
		case AemCommandStatus::AcquiredByOther:
			return "The AVDECC Entity has been acquired by another AVDECC Controller.";
		case AemCommandStatus::NotAuthenticated:
			return "The AVDECC Controller is not authenticated with the AVDECC Entity.";
		case AemCommandStatus::AuthenticationDisabled:
			return "The AVDECC Controller is trying to use an authentication command when authentication isn't enable on the AVDECC Entity.";
		case AemCommandStatus::BadArguments:
			return "One or more of the values in the fields of the frame were deemed to be bad by the AVDECC Entity(unsupported, incorrect combination, etc.).";
		case AemCommandStatus::NoResources:
			return "The AVDECC Entity cannot complete the command because it does not have the resources to support it.";
		case AemCommandStatus::InProgress:
			AVDECC_ASSERT(false, "Should not happen");
			return "The AVDECC Entity is processing the command and will send a second response at a later time with the result of the command.";
		case AemCommandStatus::EntityMisbehaving:
			return "The AVDECC Entity is generated an internal error while trying to process the command.";
		case AemCommandStatus::NotSupported:
			return "The command is implemented but the target of the command is not supported. For example trying to set the value of a read - only Control.";
		case AemCommandStatus::StreamIsRunning:
			return "The Stream is currently streaming and the command is one which cannot be executed on an Active Stream.";
		// Library Error Codes
		case AemCommandStatus::NetworkError:
			return "Network error.";
		case AemCommandStatus::ProtocolError:
			return "Protocol error.";
		case AemCommandStatus::TimedOut:
			return "Command timed out.";
		case AemCommandStatus::UnknownEntity:
			return "Unknown entity.";
		case AemCommandStatus::InternalError:
			return "Internal error.";
		default:
			AVDECC_ASSERT(false, "Unhandled status");
			return "Unknown status.";
	}
}

std::string LA_AVDECC_CALL_CONVENTION ControllerEntity::statusToString(ControllerEntity::ControlStatus const status)
{
	switch (status)
	{
		// AVDECC Error Codes
		case ControlStatus::Success:
			return "Success";
		case ControlStatus::ListenerUnknownID:
			return "Listener does not have the specified unique identifier";
		case ControlStatus::TalkerUnknownID:
			return "Talker does not have the specified unique identifier";
		case ControlStatus::TalkerDestMacFail:
			return "Talker could not allocate a destination MAC for the Stream";
		case ControlStatus::TalkerNoStreamIndex:
			return "Talker does not have an available Stream index for the Stream";
		case ControlStatus::TalkerNoBandwidth:
			return "Talker could not allocate bandwidth for the Stream";
		case ControlStatus::TalkerExclusive:
			return "Talker already has an established Stream and only supports one Listener";
		case ControlStatus::ListenerTalkerTimeout:
			return "Listener had timeout for all retries when trying to send command to Talker";
		case ControlStatus::ListenerExclusive:
			return "The AVDECC Listener already has an established connection to a Stream";
		case ControlStatus::StateUnavailable:
			return "Could not get the state from the AVDECC Entity";
		case ControlStatus::NotConnected:
			return "Trying to disconnect when not connected or not connected to the AVDECC Talker specified";
		case ControlStatus::NoSuchConnection:
			return "Trying to obtain connection info for an AVDECC Talker connection which does not exist";
		case ControlStatus::CouldNotSendMessage:
			return "The AVDECC Listener failed to send the message to the AVDECC Talker";
		case ControlStatus::TalkerMisbehaving:
			return "Talker was unable to complete the command because an internal error occurred";
		case ControlStatus::ListenerMisbehaving:
			return "Listener was unable to complete the command because an internal error occurred";
		// Reserved
		case ControlStatus::ControllerNotAuthorized:
			return "The AVDECC Controller with the specified Entity ID is not authorized to change Stream connections";
		case ControlStatus::IncompatibleRequest:
			return "The AVDECC Listener is trying to connect to an AVDECC Talker that is already streaming with a different traffic class, etc. or does not support the requested traffic class";
		case ControlStatus::NotSupported:
			return "The command is not supported";
		// Library Error Codes
		case ControlStatus::ProtocolError:
			return "Protocol error";
		case ControlStatus::TimedOut:
			return "Control timed out";
		case ControlStatus::UnknownEntity:
			return "Unknown entity";
		case ControlStatus::InternalError:
			return "Internal error";
		default:
			AVDECC_ASSERT(false, "Unhandled status");
			return "Unknown status";
	}
}

ControllerEntity::ControllerEntity(UniqueIdentifier const entityID, networkInterface::MacAddress const& macAddress, UniqueIdentifier const entityModelID, EntityCapabilities const entityCapabilities,
																	 std::uint16_t const talkerStreamSources, TalkerCapabilities const talkerCapabilities,
																	 std::uint16_t const listenerStreamSinks, ListenerCapabilities const listenerCapabilities,
																	 ControllerCapabilities const controllerCapabilities,
																	 std::uint16_t const identifyControlIndex, std::uint16_t const interfaceIndex, UniqueIdentifier const associationID) noexcept
	: LocalEntity(entityID, macAddress, entityModelID, entityCapabilities,
								talkerStreamSources, talkerCapabilities,
								listenerStreamSinks, listenerCapabilities,
								controllerCapabilities,
								identifyControlIndex, interfaceIndex, associationID)
{
}

} // namespace entity
} // namespace avdecc
} // namespace la
