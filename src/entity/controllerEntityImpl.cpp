/*
* Copyright (C) 2016-2017, L-Acoustics and its contributors

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
#include "la/avdecc/logger.hpp"
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
ControllerEntityImpl::ControllerEntityImpl(protocol::ProtocolInterface* const protocolInterface, std::uint16_t const progID, model::VendorEntityModel const vendorEntityModelID, ControllerEntity::Delegate* const delegate)
	: LocalEntityImpl(protocolInterface, progID, vendorEntityModelID, EntityCapabilities::None, 0, TalkerCapabilities::None, 0, ListenerCapabilities::None, ControllerCapabilities::Implemented, 0, protocolInterface->getInterfaceIndex(), getNullIdentifier())
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
			assert(false && "Trying to sendAemCommand from a non-existing local entity");
			return AemCommandStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::InvalidEntityType:
			assert(false && "Trying to sendAemCommand from a non-controller entity");
			return AemCommandStatus::InternalError;
		case protocol::ProtocolInterface::Error::InternalError:
			return AemCommandStatus::InternalError;
		default:
			assert(false && "ProtocolInterface error code not handled");
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
			assert(false && "Trying to sendAcmpCommand from a non-existing local entity");
			return ControlStatus::UnknownEntity;
		case protocol::ProtocolInterface::Error::InvalidEntityType:
			return ControlStatus::InternalError;
		case protocol::ProtocolInterface::Error::InternalError:
			return ControlStatus::InternalError;
		default:
			assert(false && "ProtocolInterface error code not handled");
	}
	return ControlStatus::InternalError;
}

void ControllerEntityImpl::sendAemCommand(UniqueIdentifier const targetEntityID, protocol::AemCommandType const commandType, void const* const payload, size_t const payloadLength, OnAECPErrorCallback const& onErrorCallback, AnswerCallback const& answerCallback) const noexcept
{
	try
	{
		auto* pi = getProtocolInterface();
		auto targetMacAddress = networkInterface::MacAddress{};

		// Search target mac address based on its entityID
		{
			// Lock entities
			std::lock_guard<decltype(_lockDiscoveredEntities)> const lg(_lockDiscoveredEntities);

			auto const it = _discoveredEntities.find(targetEntityID);

			// Return an error if entity is not found in the list
			if (it == _discoveredEntities.end())
			{
				onErrorCallback(AemCommandStatus::UnknownEntity);
				return;
			}

			// Get entity mac address
			targetMacAddress = it->second.getMacAddress();
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
				processAemResponse(response, onErrorCallback, answerCallback); // We sent an AEM command, we know it's an AEM response (so directly call processAemResponse)
			else
				onErrorCallback(convertErrorToAemCommandStatus(error));
		});
		if (!!error)
			onErrorCallback(convertErrorToAemCommandStatus(error));
	}
	catch (std::invalid_argument const&)
	{
		onErrorCallback(AemCommandStatus::ProtocolError);
	}
	catch (...)
	{
		onErrorCallback(AemCommandStatus::InternalError);
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
						invokeProtectedMethod(&ControllerEntity::Delegate::onEntityReleased, delegate, targetID, ownerID, descriptorType, descriptorIndex);
					}
				}
				else
				{
					answerCallback.invoke<AcquireEntityHandler>(controller, targetID, status, ownerID, descriptorType, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onEntityAcquired, delegate, targetID, ownerID, descriptorType, descriptorIndex);
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
						invokeProtectedMethod(&ControllerEntity::Delegate::onEntityUnlocked, delegate, targetID, lockedID);
					}*/
				}
				else
				{
					answerCallback.invoke<LockEntityHandler>(controller, targetID, status, lockedID);
					/*if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onEntityLocked, delegate, targetID, lockedID);
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
						answerCallback.invoke<ConfigurationDescriptorHandler>(controller, targetID, status, configurationIndex, configurationDescriptor);
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

					default:
						assert(false && "Unhandled descriptor type");
						break;
				}
			}
		},
		// Write Descriptor
		// Set Configuration
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
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamInputFormatChanged, delegate, targetID, descriptorIndex, streamFormat);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<SetStreamOutputFormatHandler>(controller, targetID, status, descriptorIndex, streamFormat);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamOutputFormatChanged, delegate, targetID, descriptorIndex, streamFormat);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Get Stream Format
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
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamInputInfoChanged, delegate, targetID, descriptorIndex, streamInfo);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<GetStreamOutputInfoHandler>(controller, targetID, status, descriptorIndex, streamInfo);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamOutputInfoChanged, delegate, targetID, descriptorIndex, streamInfo);
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
							Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Invalid descriptorIndex in SET_NAME response for Entity Descriptor: ") + std::to_string(descriptorIndex));
						}
						if (configurationIndex != 0)
						{
							Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Invalid configurationIndex in SET_NAME response for Entity Descriptor: ") + std::to_string(configurationIndex));
						}
						switch (nameIndex)
						{
							case 0: // entity_name
								answerCallback.invoke<SetEntityNameHandler>(controller, targetID, status);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									invokeProtectedMethod(&ControllerEntity::Delegate::onEntityNameChanged, delegate, targetID, name);
								}
								break;
							case 1: // group_name
								answerCallback.invoke<SetEntityGroupNameHandler>(controller, targetID, status);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									invokeProtectedMethod(&ControllerEntity::Delegate::onEntityGroupNameChanged, delegate, targetID, name);
								}
								break;
							default:
								Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Unhandled nameIndex in SET_NAME response for Entity Descriptor: ") + std::to_string(to_integral(descriptorType)) + ", " + std::to_string(descriptorIndex) + ", " + std::to_string(nameIndex) + ", " + std::to_string(configurationIndex) + ", " + name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::Configuration:
					{
						if (configurationIndex != 0)
						{
							Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Invalid configurationIndex in SET_NAME response for Configuration Descriptor: ") + std::to_string(configurationIndex));
						}
						switch (nameIndex)
						{
							case 0: // configuration_name
								answerCallback.invoke<SetConfigurationNameHandler>(controller, targetID, status, descriptorIndex);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									invokeProtectedMethod(&ControllerEntity::Delegate::onConfigurationNameChanged, delegate, targetID, descriptorIndex, name);
								}
								break;
							default:
								Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Unhandled nameIndex in SET_NAME response for Configuration Descriptor: ") + std::to_string(to_integral(descriptorType)) + ", " + std::to_string(descriptorIndex) + ", " + std::to_string(nameIndex) + ", " + std::to_string(configurationIndex) + ", " + name.str());
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
									invokeProtectedMethod(&ControllerEntity::Delegate::onStreamInputNameChanged, delegate, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Unhandled nameIndex in SET_NAME response for StreamInput Descriptor: ") + std::to_string(to_integral(descriptorType)) + ", " + std::to_string(descriptorIndex) + ", " + std::to_string(nameIndex) + ", " + std::to_string(configurationIndex) + ", " + name.str());
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
									invokeProtectedMethod(&ControllerEntity::Delegate::onStreamOutputNameChanged, delegate, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Unhandled nameIndex in SET_NAME response for StreamOutput Descriptor: ") + std::to_string(to_integral(descriptorType)) + ", " + std::to_string(descriptorIndex) + ", " + std::to_string(nameIndex) + ", " + std::to_string(configurationIndex) + ", " + name.str());
								break;
						}
						break;
					}
					default:
						Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Unhandled descriptorType in SET_NAME response: ") + std::to_string(to_integral(descriptorType)) + ", " + std::to_string(descriptorIndex) + ", " + std::to_string(nameIndex) + ", " + std::to_string(configurationIndex) + ", " + name.str());
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
							Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Invalid descriptorIndex in GET_NAME response for Entity Descriptor: ") + std::to_string(descriptorIndex));
						}
						if (configurationIndex != 0)
						{
							Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Invalid configurationIndex in GET_NAME response for Entity Descriptor: ") + std::to_string(configurationIndex));
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
								Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Unhandled nameIndex in GET_NAME response for Entity Descriptor: ") + std::to_string(to_integral(descriptorType)) + ", " + std::to_string(descriptorIndex) + ", " + std::to_string(nameIndex) + ", " + std::to_string(configurationIndex) + ", " + name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::Configuration:
					{
						if (configurationIndex != 0)
						{
							Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Invalid configurationIndex in GET_NAME response for Configuration Descriptor: ") + std::to_string(configurationIndex));
						}
						switch (nameIndex)
						{
							case 0: // configuration_name
								answerCallback.invoke<GetConfigurationNameHandler>(controller, targetID, status, descriptorIndex, name);
								break;
							default:
								Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Unhandled nameIndex in GET_NAME response for Configuration Descriptor: ") + std::to_string(to_integral(descriptorType)) + ", " + std::to_string(descriptorIndex) + ", " + std::to_string(nameIndex) + ", " + std::to_string(configurationIndex) + ", " + name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::StreamInput:
					{
						switch (nameIndex)
						{
							case 0: // stream_name
								answerCallback.invoke<GetStreamInputNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Unhandled nameIndex in GET_NAME response for StreamInput Descriptor: ") + std::to_string(to_integral(descriptorType)) + ", " + std::to_string(descriptorIndex) + ", " + std::to_string(nameIndex) + ", " + std::to_string(configurationIndex) + ", " + name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::StreamOutput:
					{
						switch (nameIndex)
						{
							case 0: // stream_name
								answerCallback.invoke<GetStreamOutputNameHandler>(controller, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Unhandled nameIndex in GET_NAME response for StreamOutput Descriptor: ") + std::to_string(to_integral(descriptorType)) + ", " + std::to_string(descriptorIndex) + ", " + std::to_string(nameIndex) + ", " + std::to_string(configurationIndex) + ", " + name.str());
								break;
						}
						break;
					}
					default:
						Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Unhandled descriptorType in GET_NAME response: ") + std::to_string(to_integral(descriptorType)) + ", " + std::to_string(descriptorIndex) + ", " + std::to_string(nameIndex) + ", " + std::to_string(configurationIndex) + ", " + name.str());
						break;
				}
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
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamInputStarted, delegate, targetID, descriptorIndex);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<StartStreamOutputHandler>(controller, targetID, status, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamOutputStarted, delegate, targetID, descriptorIndex);
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
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamInputStopped, delegate, targetID, descriptorIndex);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<StopStreamOutputHandler>(controller, targetID, status, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamOutputStopped, delegate, targetID, descriptorIndex);
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
				entity::model::AudioMappings const mappings = std::get<4>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				auto* delegate = controller->getDelegate();
				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<GetStreamInputAudioMapHandler>(controller, targetID, status, descriptorIndex, numberOfMaps, mapIndex, mappings);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamInputAudioMappingsChanged, delegate, targetID, descriptorIndex, numberOfMaps, mapIndex, mappings);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<GetStreamOutputAudioMapHandler>(controller, targetID, status, descriptorIndex, numberOfMaps, mapIndex, mappings);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamOutputAudioMappingsChanged, delegate, targetID, descriptorIndex, numberOfMaps, mapIndex, mappings);
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
				entity::model::AudioMappings const mappings = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				// Notify handlers
#pragma message("TBD: Handle unsolicited notification (add handler, and handle it in controller code)")
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<AddStreamInputAudioMappingsHandler>(controller, targetID, status, descriptorIndex, mappings);
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<AddStreamOutputAudioMappingsHandler>(controller, targetID, status, descriptorIndex, mappings);
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
				entity::model::AudioMappings const mappings = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();
				// Notify handlers
#pragma message("TBD: Handle unsolicited notification (add handler, and handle it in controller code)")
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<RemoveStreamInputAudioMappingsHandler>(controller, targetID, status, descriptorIndex, mappings);
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<RemoveStreamOutputAudioMappingsHandler>(controller, targetID, status, descriptorIndex, mappings);
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Set Memory Object Length
		// Get Memory Object Length
		// Set Stream Backup
		// Get Stream Backup
	};

	auto const& it = s_Dispatch.find(aem.getCommandType().getValue());
	if (it == s_Dispatch.end())
	{
		// If this is an unsolicited notification, simply log we do not handle the message
		if (aem.getUnsolicited())
		{
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Unsolicited AEM response ") + std::string(aem.getCommandType()) + " not handled (" + toHexString(aem.getCommandType().getValue()) + ")");
		}
		// But if it's an expected response, this is an internal error since we sent a command and didn't implement the code to handle the response
		else
		{
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Error, std::string("Failed to process AEM response: Unhandled command type ") + std::string(aem.getCommandType()) + " (" + toHexString(aem.getCommandType().getValue()) + ")");
			invokeProtectedHandler(onErrorCallback, AemCommandStatus::InternalError);
		}
	}
	else
	{
		try
		{
			it->second(this, status, aem, answerCallback);
		}
		catch (protocol::aemPayload::IncorrectPayloadSizeException const& e)
		{
			auto st = AemCommandStatus::ProtocolError;
#if defined(IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES)
			if (status != AemCommandStatus::Success)
			{
				// Allow this packet to go through as a non-success response, but some fields might have the default initial value which might not be valid (the spec says even in a response message, some fields have a meaningful value)
				st = status;
				Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Info, std::string("Received an invalid non-success ") + std::string(aem.getCommandType()) + "AEM response (" + e.what() + ") from " + toHexString(aem.getTargetEntityID(), true) + " but still processing it because of define IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES");
			}
#endif // IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES
			if (st == AemCommandStatus::ProtocolError)
			{
				Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Info, std::string("Failed to process ") + std::string(aem.getCommandType()) + " AEM response: " + e.what());
			}
			invokeProtectedHandler(onErrorCallback, st);
			return;
		}
		catch (InvalidDescriptorTypeException const& e)
		{
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Info, std::string("Failed to process ") + std::string(aem.getCommandType()) + " AEM response: " + e.what());
			invokeProtectedHandler(onErrorCallback, AemCommandStatus::ProtocolError);
			return;
		}
		catch (std::exception const& e) // Mainly unpacking errors
		{
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Info, std::string("Failed to process ") + std::string(aem.getCommandType()) + " AEM response: " + e.what());
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
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Warn, std::string("Sending AEM response using own MacAddress as source, instead of the incorrect one from the AEM command"));
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

void ControllerEntityImpl::sendAcmpCommand(protocol::AcmpMessageType const messageType, UniqueIdentifier const talkerEntityID, model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, model::StreamIndex const listenerStreamIndex, OnACMPErrorCallback const& onErrorCallback, AnswerCallback const& answerCallback) const noexcept
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
		// Set ACMP fields
		acmp->setMessageType(messageType);
		acmp->setStatus(protocol::AcmpStatus::Success);
		acmp->setControllerEntityID(getEntityID());
		acmp->setTalkerEntityID(talkerEntityID);
		acmp->setListenerEntityID(listenerEntityID);
		acmp->setTalkerUniqueID(talkerStreamIndex);
		acmp->setListenerUniqueID(listenerStreamIndex);
		acmp->setStreamDestAddress({});
		acmp->setConnectionCount(0);
		// No need to set the SequenceID, it's set by the ProtocolInterface layer
		acmp->setFlags(ConnectionFlags::None);
		acmp->setStreamVlanID(0);

		auto const error = pi->sendAcmpCommand(std::move(frame), [onErrorCallback, answerCallback, this](protocol::Acmpdu const* const response, protocol::ProtocolInterface::Error const error) noexcept
		{
			if (!error)
				processAcmpResponse(response, onErrorCallback, answerCallback, false);
			else
				onErrorCallback(convertErrorToControlStatus(error));
		});
		if (!!error)
			onErrorCallback(convertErrorToControlStatus(error));
	}
	catch (std::invalid_argument const&)
	{
		onErrorCallback(ControlStatus::ProtocolError);
	}
	catch (...)
	{
		onErrorCallback(ControlStatus::InternalError);
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
				if (sniffed && delegate && avdecc::hasFlag(flags, entity::ConnectionFlags::FastConnect))
				{
					invokeProtectedMethod(&ControllerEntity::Delegate::onFastConnectStreamSniffed, delegate, controller, talkerEntityID, talkerStreamIndex, listenerEntityID, listenerStreamIndex, connectionCount, flags, status);
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
				answerCallback.invoke<ConnectStreamHandler>(controller, talkerEntityID, talkerStreamIndex, listenerEntityID, listenerStreamIndex, connectionCount, flags, status);
				auto* delegate = controller->getDelegate();
				if (sniffed && delegate)
				{
					invokeProtectedMethod(&ControllerEntity::Delegate::onConnectStreamSniffed, delegate, controller, talkerEntityID, talkerStreamIndex, listenerEntityID, listenerStreamIndex, connectionCount, flags, status);
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
				answerCallback.invoke<DisconnectStreamHandler>(controller, talkerEntityID, talkerStreamIndex, listenerEntityID, listenerStreamIndex, connectionCount, flags, status);
				auto* delegate = controller->getDelegate();
				if (sniffed && delegate)
				{
					invokeProtectedMethod(&ControllerEntity::Delegate::onDisconnectStreamSniffed, delegate, controller, talkerEntityID, talkerStreamIndex, listenerEntityID, listenerStreamIndex, connectionCount, flags, status);
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
				answerCallback.invoke<GetListenerStreamStateHandler>(controller, listenerEntityID, listenerStreamIndex, talkerEntityID, talkerStreamIndex, connectionCount, flags, status);
				auto* delegate = controller->getDelegate();
				if (sniffed && delegate)
				{
					invokeProtectedMethod(&ControllerEntity::Delegate::onGetListenerStreamStateSniffed, delegate, controller, listenerEntityID, listenerStreamIndex, talkerEntityID, talkerStreamIndex, connectionCount, flags, status);
				}
			}
		},
	};


	auto const& it = s_Dispatch.find(acmp.getMessageType().getValue());
	if (it == s_Dispatch.end())
	{
		// If this is a sniffed message, simply log we do not handle the message
		if (sniffed)
		{
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("ACMP response ") + std::string(acmp.getMessageType()) + " not handled (" + toHexString(acmp.getMessageType().getValue()) + ")");
		}
		// But if it's an expected response, this is an internal error since we sent a command and didn't implement the code to handle the response
		else
		{
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Error, std::string("Failed to process ACMP response: Unhandled message type ") + std::string(acmp.getMessageType()) + " (" + toHexString(acmp.getMessageType().getValue()) + ")");
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
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Info, std::string("Failed to process ACMP response: ") + e.what());
			invokeProtectedHandler(onErrorCallback, st);
			return;
		}
		catch (std::exception const& e) // Mainly unpacking errors
		{
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Info, std::string("Failed to process ACMP response: ") + e.what());
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
#pragma message("TBD: Change the API to allow partial EM acquire")
void ControllerEntityImpl::acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, AcquireEntityHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeAcquireEntityCommand(isPersistent ? protocol::AemAcquireEntityFlags::Persistent : protocol::AemAcquireEntityFlags::None, getNullIdentifier(), descriptorType, descriptorIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, getNullIdentifier(), descriptorType, descriptorIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::AcquireEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize acquireEntity: ") + e.what());
	}
}

#pragma message("TBD: Change the API to allow partial EM acquire")
void ControllerEntityImpl::releaseEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, ReleaseEntityHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeAcquireEntityCommand(protocol::AemAcquireEntityFlags::Release, getNullIdentifier(), descriptorType, descriptorIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, getNullIdentifier(), descriptorType, descriptorIndex);

		sendAemCommand(targetEntityID, protocol::AemCommandType::AcquireEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize releaseEntity: ") + e.what());
	}
}

#pragma message("TBD: Change the API to allow partial EM lock")
void ControllerEntityImpl::lockEntity(UniqueIdentifier const targetEntityID, LockEntityHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeLockEntityCommand(protocol::AemLockEntityFlags::None, getNullIdentifier(), model::DescriptorType::Entity, 0);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, getNullIdentifier());

		sendAemCommand(targetEntityID, protocol::AemCommandType::LockEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize lockEntity: ") + e.what());
	}
}

void ControllerEntityImpl::unlockEntity(UniqueIdentifier const targetEntityID, UnlockEntityHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeLockEntityCommand(protocol::AemLockEntityFlags::Unlock, getNullIdentifier(), model::DescriptorType::Entity, 0);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1);

		sendAemCommand(targetEntityID, protocol::AemCommandType::LockEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize unlockEntity: ") + e.what());
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
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(0, model::DescriptorType::Entity, 0);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, model::EntityDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize readEntityDescriptor: ") + e.what());
	}
}

void ControllerEntityImpl::readConfigurationDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, ConfigurationDescriptorHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(0, model::DescriptorType::Configuration, configurationIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, configurationIndex, model::ConfigurationDescriptor{});

		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize readConfigurationDescriptor: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize readStreamInputDescriptor: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize readStreamOutputDescriptor: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize readLocaleDescriptor: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize readStringsDescriptor: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize setStreamInputFormat: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize setStreamOutputFormat: ") + e.what());
	}
}

void ControllerEntityImpl::getStreamInputAudioMap(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::MapIndex const mapIndex, GetStreamInputAudioMapHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetAudioMapCommand(model::DescriptorType::StreamInput, streamIndex, mapIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex, model::MapIndex(0), mapIndex, s_emptyMappings);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetAudioMap, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize getStreamInputAudioMap: ") + e.what());
	}
}

void ControllerEntityImpl::getStreamOutputAudioMap(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::MapIndex const mapIndex, GetStreamOutputAudioMapHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeGetAudioMapCommand(model::DescriptorType::StreamOutput, streamIndex, mapIndex);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex, model::MapIndex(0), mapIndex, s_emptyMappings);

		sendAemCommand(targetEntityID, protocol::AemCommandType::GetAudioMap, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize getStreamOutputAudioMap: ") + e.what());
	}
}

void ControllerEntityImpl::addStreamInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, std::vector<model::AudioMapping> const& mappings, AddStreamInputAudioMappingsHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeAddAudioMappingsCommand(model::DescriptorType::StreamInput, streamIndex, mappings);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex, s_emptyMappings);

		sendAemCommand(targetEntityID, protocol::AemCommandType::AddAudioMappings, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize addStreamInputAudioMappings: ") + e.what());
	}
}

void ControllerEntityImpl::addStreamOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::AudioMappings const& mappings, AddStreamOutputAudioMappingsHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeAddAudioMappingsCommand(model::DescriptorType::StreamOutput, streamIndex, mappings);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex, s_emptyMappings);

		sendAemCommand(targetEntityID, protocol::AemCommandType::AddAudioMappings, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize addStreamOutputAudioMappings: ") + e.what());
	}
}

void ControllerEntityImpl::removeStreamInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, std::vector<model::AudioMapping> const& mappings, RemoveStreamInputAudioMappingsHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeRemoveAudioMappingsCommand(model::DescriptorType::StreamInput, streamIndex, mappings);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex, s_emptyMappings);

		sendAemCommand(targetEntityID, protocol::AemCommandType::RemoveAudioMappings, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize removeStreamInputAudioMappings: ") + e.what());
	}
}

void ControllerEntityImpl::removeStreamOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::AudioMappings const& mappings, RemoveStreamOutputAudioMappingsHandler const& handler) const noexcept
{
	try
	{
		auto const ser = protocol::aemPayload::serializeRemoveAudioMappingsCommand(model::DescriptorType::StreamOutput, streamIndex, mappings);
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, streamIndex, s_emptyMappings);

		sendAemCommand(targetEntityID, protocol::AemCommandType::RemoveAudioMappings, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize removeStreamOutputAudioMappings: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize getStreamInputInfo: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize setName: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize getName: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize setName: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize getName: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize setName: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize getName: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize setName: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize getName: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize setName: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize getName: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize startStreamInput: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize startStreamOutput: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize stopStreamInput: ") + e.what());
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
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize stopStreamOutput: ") + e.what());
	}
}

/* Connection Management Protocol (ACMP) */
void ControllerEntityImpl::connectStream(UniqueIdentifier const talkerEntityID, model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, model::StreamIndex const listenerStreamIndex, ConnectStreamHandler const& handler) const noexcept
{
	auto errorCallback = ControllerEntityImpl::makeACMPErrorHandler(handler, this, getNullIdentifier(), model::StreamIndex(0), listenerEntityID, listenerStreamIndex, std::uint16_t(0), entity::ConnectionFlags::None, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::ConnectRxCommand, talkerEntityID, talkerStreamIndex, listenerEntityID, listenerStreamIndex, errorCallback, handler);
}

void ControllerEntityImpl::disconnectStream(UniqueIdentifier const talkerEntityID, model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, model::StreamIndex const listenerStreamIndex, DisconnectStreamHandler const& handler) const noexcept
{
	auto errorCallback = ControllerEntityImpl::makeACMPErrorHandler(handler, this, getNullIdentifier(), model::StreamIndex(0), listenerEntityID, listenerStreamIndex, std::uint16_t(0), entity::ConnectionFlags::None, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::DisconnectRxCommand, talkerEntityID, talkerStreamIndex, listenerEntityID, listenerStreamIndex, errorCallback, handler);
}

void ControllerEntityImpl::getListenerStreamState(UniqueIdentifier const listenerEntityID, model::StreamIndex const listenerStreamIndex, GetListenerStreamStateHandler const& handler) const noexcept
{
	auto errorCallback = ControllerEntityImpl::makeACMPErrorHandler(handler, this, listenerEntityID, listenerStreamIndex, getNullIdentifier(), model::StreamIndex(0), std::uint16_t(0), entity::ConnectionFlags::None, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::GetRxStateCommand, getNullIdentifier(), model::StreamIndex(0), listenerEntityID, listenerStreamIndex, errorCallback, handler);
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
void ControllerEntityImpl::onTransportError(protocol::ProtocolInterface const* const /*pi*/) noexcept
{
	invokeProtectedMethod(&ControllerEntity::Delegate::onTransportError, _delegate);
}

/* **** Discovery notifications **** */
void ControllerEntityImpl::onLocalEntityOnline(protocol::ProtocolInterface const* const pi, DiscoveredEntity const& entity) noexcept
{
	// The controller doesn't make any difference btw a local and a remote entity, just ignore ourself
	if (getEntityID() != entity.getEntityID())
		onRemoteEntityOnline(pi, entity);
}

void ControllerEntityImpl::onLocalEntityOffline(protocol::ProtocolInterface const* const pi, UniqueIdentifier const entityID) noexcept
{
	// The controller doesn't make any difference btw a local and a remote entity, just ignore ourself
	if (getEntityID() != entityID)
		onRemoteEntityOffline(pi, entityID); // The controller doesn't make any difference btw a local and a remote entity
}

void ControllerEntityImpl::onLocalEntityUpdated(protocol::ProtocolInterface const* const pi, DiscoveredEntity const& entity) noexcept
{
	// The controller doesn't make any difference btw a local and a remote entity, just ignore ourself
	if (getEntityID() != entity.getEntityID())
		onRemoteEntityUpdated(pi, entity); // The controller doesn't make any difference btw a local and a remote entity
}

void ControllerEntityImpl::onRemoteEntityOnline(protocol::ProtocolInterface const* const /*pi*/, DiscoveredEntity const& entity) noexcept
{
	auto const entityID = entity.getEntityID();
	{
		// Lock entities
		std::lock_guard<decltype(_lockDiscoveredEntities)> const lg(_lockDiscoveredEntities);

		// Store or replace entity
		{
#pragma message("TBD: Replace with c++17 insert_or_assign")
			auto it = _discoveredEntities.find(entityID);
			assert(it == _discoveredEntities.end() && "ControllerEntityImpl::onRemoteEntityOnline: Entity already online");
			if (it == _discoveredEntities.end())
				_discoveredEntities.insert(std::make_pair(entityID, entity));
			else
				it->second = entity;
		}
	}

	invokeProtectedMethod(&ControllerEntity::Delegate::onEntityOnline, _delegate, this, entityID, entity);
}

void ControllerEntityImpl::onRemoteEntityOffline(protocol::ProtocolInterface const* const /*pi*/, UniqueIdentifier const entityID) noexcept
{
	{
		// Lock entities
		std::lock_guard<decltype(_lockDiscoveredEntities)> const lg(_lockDiscoveredEntities);

		// Remove entity
		_discoveredEntities.erase(entityID);
	}

	invokeProtectedMethod(&ControllerEntity::Delegate::onEntityOffline, _delegate, this, entityID);
}

void ControllerEntityImpl::onRemoteEntityUpdated(protocol::ProtocolInterface const* const /*pi*/, DiscoveredEntity const& entity) noexcept
{
	auto const entityID = entity.getEntityID();
	{
		// Lock entities
		std::lock_guard<decltype(_lockDiscoveredEntities)> const lg(_lockDiscoveredEntities);

		// Store or replace entity
		{
#pragma message("TBD: Replace with c++17 insert_or_assign")
			auto it = _discoveredEntities.find(entityID);
			assert(it != _discoveredEntities.end() && "ControllerEntityImpl::onRemoteEntityUpdated: Entity offline");
			if (it == _discoveredEntities.end())
				_discoveredEntities.insert(std::make_pair(entityID, entity));
			else
				it->second = entity;
		}
	}

	invokeProtectedMethod(&ControllerEntity::Delegate::onEntityUpdate, _delegate, this, entityID, entity);
}

/* **** AECP notifications **** */
void ControllerEntityImpl::onAecpCommand(protocol::ProtocolInterface const* const /*pi*/, entity::LocalEntity const& /*entity*/, protocol::Aecpdu const& aecpdu) noexcept
{
	auto const selfID = getEntityID();
	auto const targetID = aecpdu.getTargetEntityID();

	assert(targetID == selfID && "Should be filtered by controller_state_machine already... on mac too??");
	// Filter messages not for me
	if (targetID != selfID)
		return;

	if (aecpdu.getMessageType() == protocol::AecpMessageType::AemCommand)
	{
		auto const& aem = static_cast<protocol::AemAecpdu const&>(aecpdu);
		auto const controllerID = aem.getControllerEntityID();

		// Filter self messages
		if (controllerID == selfID)
			return;

		static std::unordered_map<protocol::AemCommandType::value_type, std::function<void(ControllerEntityImpl const* const controller, protocol::AemAecpdu const& aem)noexcept>> s_Dispatch{
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
			it->second(this, aem);
		}
		else
		{
			// Reflect back the payload, and return a NotSupported error code
			auto const payload = aem.getPayload();
			sendAemResponse(aem, protocol::AemAecpStatus::NotSupported, payload.first, payload.second);
		}
	}
}

void ControllerEntityImpl::onAecpUnsolicitedResponse(protocol::ProtocolInterface const* const /*pi*/, entity::LocalEntity const& /*entity*/, protocol::Aecpdu const& aecpdu) noexcept
{
	auto const messageType = aecpdu.getMessageType();

	if (messageType == protocol::AecpMessageType::AemResponse)
	{
		auto const& aem = static_cast<protocol::AemAecpdu const&>(aecpdu);
		assert(aem.getUnsolicited() && "Should only be triggered for unsollicited notifications");
		if (aem.getUnsolicited())
		{
			// Process AEM message without any error or answer callbacks, it's not an expected response
			processAemResponse(&aecpdu, nullptr, {});
		}
	}
}

/* **** ACMP notifications **** */
void ControllerEntityImpl::onAcmpSniffedCommand(protocol::ProtocolInterface const* const /*pi*/, entity::LocalEntity const& /*entity*/, protocol::Acmpdu const& /*acmpdu*/) noexcept
{
}

void ControllerEntityImpl::onAcmpSniffedResponse(protocol::ProtocolInterface const* const /*pi*/, entity::LocalEntity const& /*entity*/, protocol::Acmpdu const& acmpdu) noexcept
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
			return "Success";
		case AemCommandStatus::NotImplemented:
			return "The AVDECC Entity does not support the command type";
		case AemCommandStatus::NoSuchDescriptor:
			return "A descriptor with the descriptor_type and descriptor_index specified does not exist";
		case AemCommandStatus::LockedByOther:
			return "The AVDECC Entity has been locked by another AVDECC Controller";
		case AemCommandStatus::AcquiredByOther:
			return "The AVDECC Entity has been acquired by another AVDECC Controller";
		case AemCommandStatus::NotAuthenticated:
			return "The AVDECC Controller is not authenticated with the AVDECC Entity";
		case AemCommandStatus::AuthenticationDisabled:
			return "The AVDECC Controller is trying to use an authentication command when authentication isn't enable on the AVDECC Entity";
		case AemCommandStatus::BadArguments:
			return "One or more of the values in the fields of the frame were deemed to be bad by the AVDECC Entity(unsupported, incorrect combination, etc.)";
		case AemCommandStatus::NoResources:
			return "The AVDECC Entity cannot complete the command because it does not have the resources to support it";
		case AemCommandStatus::InProgress:
			assert(false); // Should not happen
			return "The AVDECC Entity is processing the command and will send a second response at a later time with the result of the command";
		case AemCommandStatus::EntityMisbehaving:
			return "The AVDECC Entity is generated an internal error while trying to process the command";
		case AemCommandStatus::NotSupported:
			return "The command is implemented but the target of the command is not supported.For example trying to set the value of a read - only Control";
		case AemCommandStatus::StreamIsRunning:
			return "The Stream is currently streaming and the command is one which cannot be executed on an Active Stream";
		// Library Error Codes
		case AemCommandStatus::NetworkError:
			return "Network error";
		case AemCommandStatus::ProtocolError:
			return "Protocol error";
		case AemCommandStatus::TimedOut:
			return "Command timed out";
		case AemCommandStatus::UnknownEntity:
			return "Unknown entity";
		case AemCommandStatus::InternalError:
			return "Internal error";
		default:
			assert(false && "Unhandled status");
			return "Unknown status";
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
			assert(false && "Unhandled status");
			return "Unknown status";
	}
}

ControllerEntity::ControllerEntity(UniqueIdentifier const entityID, networkInterface::MacAddress const& macAddress, model::VendorEntityModel const vendorEntityModelID, EntityCapabilities const entityCapabilities,
																	 std::uint16_t const talkerStreamSources, TalkerCapabilities const talkerCapabilities,
																	 std::uint16_t const listenerStreamSinks, ListenerCapabilities const listenerCapabilities,
																	 ControllerCapabilities const controllerCapabilities,
																	 std::uint16_t const identifyControlIndex, std::uint16_t const interfaceIndex, UniqueIdentifier const associationID) noexcept
	: LocalEntity(entityID, macAddress, vendorEntityModelID, entityCapabilities,
								talkerStreamSources, talkerCapabilities,
								listenerStreamSinks, listenerCapabilities,
								controllerCapabilities,
								identifyControlIndex, interfaceIndex, associationID)
{
}

} // namespace entity
} // namespace avdecc
} // namespace la
