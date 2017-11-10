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
#pragma message("TODO: To remove: CommandException")
class CommandException final : public std::exception
{
public:
	CommandException(ControllerEntity::AemCommandStatus const status, char const* const text) : _status(status), _text(text)
	{
	}
	ControllerEntity::AemCommandStatus getStatus() const
	{
		return _status;
	}
	virtual char const* what() const noexcept override
	{
		return _text;
	}
private:
	ControllerEntity::AemCommandStatus _status{ ControllerEntity::AemCommandStatus::InternalError };
	char const* _text{ nullptr };
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
	// Unregister observer
	invokeProtectedMethod(&protocol::ProtocolInterface::unregisterObserver, getProtocolInterface(), this);

	_shouldTerminate = true;
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

	static std::unordered_map<protocol::AemCommandType::value_type, std::function<void(ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)>> s_Dispatch{
		// Acquire Entity
		{ protocol::AemCommandType::AcquireEntity.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				auto const payloadInfo = aem.getPayload();
				auto* const commandPayload = payloadInfo.first;
				auto const commandPayloadLength = payloadInfo.second;

				if (commandPayload == nullptr || commandPayloadLength < protocol::AecpAemAcquireEntityResponsePayloadSize) // Malformed packet
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: ACQUIRE_ENTITY");

				// Check payload for acquire/release status
				Deserializer des(commandPayload, commandPayloadLength);
				auto aemAcquireFlags{ protocol::AemAcquireEntityFlags::None };
				UniqueIdentifier ownerID;
				model::DescriptorType descriptorType;
				model::DescriptorIndex descriptorIndex;
				des >> aemAcquireFlags >> ownerID >> descriptorType >> descriptorIndex;
				assert(des.usedBytes() == protocol::AecpAemAcquireEntityResponsePayloadSize && "Used more bytes than specified in protocol constant");

				auto const targetID = aem.getTargetEntityID();
				auto* delegate = controller->getDelegate();
				if ((aemAcquireFlags & protocol::AemAcquireEntityFlags::Release) == protocol::AemAcquireEntityFlags::Release)
				{
					answerCallback.invoke<ReleaseEntityHandler>(controller, targetID, status, ownerID);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onEntityReleased, delegate, targetID, ownerID);
					}
				}
				else
				{
					answerCallback.invoke<AcquireEntityHandler>(controller, targetID, status, ownerID);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onEntityAcquired, delegate, targetID, ownerID);
					}
				}
			}
		},
		// Lock Entity
		{ protocol::AemCommandType::LockEntity.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				auto const payloadInfo = aem.getPayload();
				auto* const commandPayload = payloadInfo.first;
				auto const commandPayloadLength = payloadInfo.second;

				if (commandPayload == nullptr || commandPayloadLength < protocol::AecpAemLockEntityResponsePayloadSize) // Malformed packet
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: LOCK_ENTITY");

				// Check payload for lock/release status
				Deserializer des(commandPayload, commandPayloadLength);
				auto aemLockFlags{ protocol::AemLockEntityFlags::None };
				UniqueIdentifier lockID;
				model::DescriptorType descriptorType;
				model::DescriptorIndex descriptorIndex;
				des >> aemLockFlags >> lockID >> descriptorType >> descriptorIndex;
				assert(des.usedBytes() == protocol::AecpAemLockEntityResponsePayloadSize && "Used more bytes than specified in protocol constant");

				auto const targetID = aem.getTargetEntityID();
				//auto* delegate = controller->getDelegate();
				if ((aemLockFlags & protocol::AemLockEntityFlags::Release) == protocol::AemLockEntityFlags::Release)
				{
					answerCallback.invoke<UnlockEntityHandler>(controller, targetID, status);
					/*if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onEntityUnlocked, delegate, targetID, lockID);
					}*/
				}
				else
				{
					answerCallback.invoke<LockEntityHandler>(controller, targetID, status, lockID);
					/*if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onEntityLocked, delegate, targetID, lockID);
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
				auto const payloadInfo = aem.getPayload();
				auto* const commandPayload = payloadInfo.first;
				auto const commandPayloadLength = payloadInfo.second;

				if (commandPayload == nullptr || commandPayloadLength < protocol::AecpAemReadDescriptorResponsePayloadMinSize) // Malformed packet
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: READ_DESCRIPTOR");

				// Check payload read descriptor data
				Deserializer des(commandPayload, commandPayloadLength);
				model::ConfigurationIndex configuration_index;
				std::uint16_t reserved;
				model::DescriptorType descriptorType;
				model::DescriptorIndex descriptorIndex;
				des >> configuration_index >> reserved >> descriptorType >> descriptorIndex;
				assert(des.usedBytes() == protocol::AecpAemReadDescriptorResponsePayloadMinSize && "Used more bytes than specified in protocol constant");

				auto const targetID = aem.getTargetEntityID();
				switch (descriptorType)
				{
					case model::DescriptorType::Entity:
					{
						if (commandPayloadLength < protocol::AecpAemReadEntityDescriptorResponsePayloadSize) // Malformed packet
							throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: DESCRIPTOR_ENTITY");

						// Read descriptor fields
						model::EntityDescriptor entityDescriptor{ {descriptorType, descriptorIndex } };
						des >> entityDescriptor.entityID >> entityDescriptor.vendorEntityModelID >> entityDescriptor.entityCapabilities;
						des >> entityDescriptor.talkerStreamSources >> entityDescriptor.talkerCapabilities;
						des >> entityDescriptor.listenerStreamSinks >> entityDescriptor.listenerCapabilities;
						des >> entityDescriptor.controllerCapabilities;
						des >> entityDescriptor.availableIndex;
						des >> entityDescriptor.associationID;
						des >> entityDescriptor.entityName;
						des >> entityDescriptor.vendorNameString >> entityDescriptor.modelNameString;
						des >> entityDescriptor.firmwareVersion;
						des >> entityDescriptor.groupName;
						des >> entityDescriptor.serialNumber;
						des >> entityDescriptor.configurationsCount >> entityDescriptor.currentConfiguration;
						assert(des.usedBytes() == protocol::AecpAemReadEntityDescriptorResponsePayloadSize && "Used more bytes than specified in protocol constant");
						// Notify handlers
						answerCallback.invoke<EntityDescriptorHandler>(controller, targetID, status, entityDescriptor);
						break;
					}

					case model::DescriptorType::Configuration:
					{
						if (commandPayloadLength < protocol::AecpAemReadConfigurationDescriptorResponsePayloadMinSize) // Malformed packet
							throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: DESCRIPTOR_CONFIGURATION");

						// Read descriptor fields
						model::ConfigurationDescriptor configurationDescriptor{ {descriptorType, descriptorIndex } };
						des >> configurationDescriptor.objectName;
						des >> configurationDescriptor.localizedDescription;
						des >> configurationDescriptor.descriptorCountsCount >> configurationDescriptor.descriptorCountsOffset;
						// Check descriptor variable size
						constexpr size_t descriptorInfoSize = sizeof(model::DescriptorType) + sizeof(std::uint16_t);
						auto const descriptorCountsSize = descriptorInfoSize * configurationDescriptor.descriptorCountsCount;
						if (des.remaining() < descriptorCountsSize) // Malformed packet
							throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: DESCRIPTOR_CONFIGURATION");
						// Unpack descriptor remaining data
						for (auto index = 0u; index < configurationDescriptor.descriptorCountsCount; ++index)
						{
							model::DescriptorType type;
							std::uint16_t count;
							des >> type >> count;
							configurationDescriptor.descriptorCounts[type] = count;
						}
						assert(des.usedBytes() == (protocol::AecpAemReadConfigurationDescriptorResponsePayloadMinSize + descriptorCountsSize) && "Used more bytes than specified in protocol constant");
						// Notify handlers
						answerCallback.invoke<ConfigurationDescriptorHandler>(controller, targetID, status, configurationDescriptor);
						break;
					}

					case model::DescriptorType::Locale:
					{
						if (commandPayloadLength < protocol::AecpAemReadLocaleDescriptorResponsePayloadSize) // Malformed packet
							throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: DESCRIPTOR_LOCALE");

						// Read descriptor fields
						model::LocaleDescriptor localeDescriptor{ {descriptorType, descriptorIndex } };
						des >> localeDescriptor.localeID;
						des >> localeDescriptor.numberOfStringDescriptors >> localeDescriptor.baseStringDescriptorIndex;
						assert(des.usedBytes() == protocol::AecpAemReadLocaleDescriptorResponsePayloadSize && "Used more bytes than specified in protocol constant");
						// Notify handlers
						answerCallback.invoke<LocaleDescriptorHandler>(controller, targetID, status, localeDescriptor);
						break;
					}

					case model::DescriptorType::Strings:
					{
						if (commandPayloadLength < protocol::AecpAemReadStringsDescriptorResponsePayloadSize) // Malformed packet
							throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: DESCRIPTOR_STRINGS");

						// Read descriptor fields
						model::StringsDescriptor stringsDescriptor{ {descriptorType, descriptorIndex } };
						for (auto strIndex = 0u; strIndex < stringsDescriptor.strings.size(); ++strIndex)
						{
							des >> stringsDescriptor.strings[strIndex];
						}
						assert(des.usedBytes() == protocol::AecpAemReadStringsDescriptorResponsePayloadSize && "Used more bytes than specified in protocol constant");
						// Notify handlers
						answerCallback.invoke<StringsDescriptorHandler>(controller, targetID, status, stringsDescriptor);
						break;
					}

					case model::DescriptorType::StreamInput:
						if (commandPayloadLength < protocol::AecpAemReadStreamDescriptorResponsePayloadMinSize) // Malformed packet
							throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: DESCRIPTOR_STREAM_INPUT");
						// Don't break, let's handle both input and output streams together
					case model::DescriptorType::StreamOutput:
					{
						if (commandPayloadLength < protocol::AecpAemReadStreamDescriptorResponsePayloadMinSize) // Malformed packet
							throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: DESCRIPTOR_STREAM_OUTPUT");

						// Read descriptor fields
						model::StreamDescriptor streamDescriptor{ descriptorType, descriptorIndex };
						des >> streamDescriptor.objectName;
						des >> streamDescriptor.localizedDescription >> streamDescriptor.clockDomainIndex >> streamDescriptor.streamFlags;
						des >> streamDescriptor.currentFormat >> streamDescriptor.formatsOffset >> streamDescriptor.numberOfFormats;
						des >> streamDescriptor.backupTalkerEntityID_0 >> streamDescriptor.backupTalkerUniqueID_0;
						des >> streamDescriptor.backupTalkerEntityID_1 >> streamDescriptor.backupTalkerUniqueID_1;
						des >> streamDescriptor.backupTalkerEntityID_2 >> streamDescriptor.backupTalkerUniqueID_2;
						des >> streamDescriptor.backedupTalkerEntityID >> streamDescriptor.backedupTalkerUnique;
						des >> streamDescriptor.avbInterfaceIndex >> streamDescriptor.bufferLength;

						// Check descriptor variable size
						constexpr size_t formatInfoSize = sizeof(std::uint64_t);
						auto const formatsSize = formatInfoSize * streamDescriptor.numberOfFormats;
						if (des.remaining() < formatsSize) // Malformed packet
							throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: DESCRIPTOR_STREAM_INPUT/OUTPUT"); // Malformed packet

						// Unpack formats
						// Clause 7.2.6 says that the formats should start at streamDescriptor.formatsOffset from the begining of the descriptor
						// which starts after 'sizeof(configuration_index) + sizeof(reserved)' in our case since ReadDescriptor response includes descriptorType+descriptorIndex (see Claude 7.4.5.2)
						auto const formatsOffset = sizeof(configuration_index) + sizeof(reserved) + streamDescriptor.formatsOffset;
						if (formatsOffset < des.usedBytes())
							throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: DESCRIPTOR_STREAM_INPUT/OUTPUT"); // Malformed packet
						des.setUsedBytes(formatsOffset);
						// Let's loop over the formats
						for (auto index = 0u; index < streamDescriptor.numberOfFormats; ++index)
						{
							std::uint64_t format;
							des >> format;
							streamDescriptor.formats.push_back(format);
						}
						assert(des.usedBytes() == (protocol::AecpAemReadStreamDescriptorResponsePayloadMinSize + formatsSize) && "Used more bytes than specified in protocol constant");
						// Notify handlers
						if (descriptorType == model::DescriptorType::StreamInput)
							answerCallback.invoke<StreamInputDescriptorHandler>(controller, targetID, status, streamDescriptor);
						else if (descriptorType == model::DescriptorType::StreamOutput)
							answerCallback.invoke<StreamOutputDescriptorHandler>(controller, targetID, status, streamDescriptor);
						else
							throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: Unknown DESCRIPTOR_STREAM type"); // Malformed packet
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
				auto const payloadInfo = aem.getPayload();
				auto* const commandPayload = payloadInfo.first;
				auto const commandPayloadLength = payloadInfo.second;

				if (commandPayload == nullptr || commandPayloadLength < protocol::AecpAemSetStreamFormatResponsePayloadSize) // Malformed packet
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: SET_STREAM_FORMAT");

				// Check payload stream format data
				Deserializer des(commandPayload, commandPayloadLength);
				model::DescriptorType descriptorType;
				model::DescriptorIndex descriptorIndex;
				model::StreamFormat streamFormat;
				des >> descriptorType >> descriptorIndex >> streamFormat;
				assert(des.usedBytes() == protocol::AecpAemSetStreamFormatResponsePayloadSize && "Used more bytes than specified in protocol constant");

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
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: Unknown DESCRIPTOR_STREAM type"); // Malformed packet
			}
		},
		// Get Stream Format
		// Set Stream Info
		// Get Stream Info
		{ protocol::AemCommandType::GetStreamInfo.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				auto const payloadInfo = aem.getPayload();
				auto* const commandPayload = payloadInfo.first;
				auto const commandPayloadLength = payloadInfo.second;

				if (commandPayload == nullptr || commandPayloadLength < protocol::AecpAemGetStreamInfoResponsePayloadSize) // Malformed packet
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: GET_STREAM_INFO");

				// Check payload stream format data
				Deserializer des(commandPayload, commandPayloadLength);
				model::StreamInfo streamInfo;
				des >> streamInfo.common.descriptorType >> streamInfo.common.descriptorIndex;
				des >> streamInfo.streamInfoFlags >> streamInfo.streamFormat >> streamInfo.streamID >> streamInfo.msrpAccumulatedLatency;
				des.unpackBuffer(streamInfo.streamDestMac.data(), static_cast<std::uint16_t>(streamInfo.streamDestMac.size()));
				des >> streamInfo.msrpFailureCode >> streamInfo.reserved >> streamInfo.msrpFailureBridgeID >> streamInfo.streamVlanID >> streamInfo.reserved2;
				assert(des.usedBytes() == protocol::AecpAemGetStreamInfoResponsePayloadSize && "Used more bytes than specified in protocol constant");

				auto const targetID = aem.getTargetEntityID();
				auto* delegate = controller->getDelegate();
				// Notify handlers
				if (streamInfo.common.descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<GetStreamInputInfoHandler>(controller, targetID, status, streamInfo.common.descriptorIndex, streamInfo);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamInputInfoChanged, delegate, targetID, streamInfo.common.descriptorIndex, streamInfo);
					}
				}
				else if (streamInfo.common.descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<GetStreamOutputInfoHandler>(controller, targetID, status, streamInfo.common.descriptorIndex, streamInfo);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						invokeProtectedMethod(&ControllerEntity::Delegate::onStreamOutputInfoChanged, delegate, targetID, streamInfo.common.descriptorIndex, streamInfo);
					}
				}
				else
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: Unknown DESCRIPTOR_STREAM type"); // Malformed packet
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
				model::DescriptorType descriptorType = std::get<0>(result);
				model::DescriptorIndex descriptorIndex = std::get<1>(result);
				std::uint16_t nameIndex = std::get<2>(result);
				model::ConfigurationIndex configurationIndex = std::get<3>(result);
				model::AvdeccFixedString name = std::get<4>(result);
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
		{ protocol::AemCommandType::GetName.getValue(), [](ControllerEntityImpl const* const /*controller*/, AemCommandStatus const /*status*/, protocol::AemAecpdu const& /*aem*/, AnswerCallback const& /*answerCallback*/)
			{
				assert(false && "todo");
			}
		},
		// Start Streaming
		{ protocol::AemCommandType::StartStreaming.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				auto const payloadInfo = aem.getPayload();
				auto* const commandPayload = payloadInfo.first;
				auto const commandPayloadLength = payloadInfo.second;

				if (commandPayload == nullptr || commandPayloadLength < protocol::AecpAemStartStreamingResponsePayloadSize) // Malformed packet
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: START_STREAMING");

				// Check payload
				Deserializer des(commandPayload, commandPayloadLength);
				model::DescriptorType descriptorType;
				model::DescriptorIndex descriptorIndex;
				des >> descriptorType >> descriptorIndex;
				assert(des.usedBytes() == protocol::AecpAemStartStreamingResponsePayloadSize && "Used more bytes than specified in protocol constant");

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
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: Unknown DESCRIPTOR_STREAM type"); // Malformed packet
			}
		},
		// Stop Streaming
		{ protocol::AemCommandType::StopStreaming.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				auto const payloadInfo = aem.getPayload();
				auto* const commandPayload = payloadInfo.first;
				auto const commandPayloadLength = payloadInfo.second;

				if (commandPayload == nullptr || commandPayloadLength < protocol::AecpAemStopStreamingResponsePayloadSize) // Malformed packet
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: STOP_STREAMING");

				// Check payload
				Deserializer des(commandPayload, commandPayloadLength);
				model::DescriptorType descriptorType;
				model::DescriptorIndex descriptorIndex;
				des >> descriptorType >> descriptorIndex;
				assert(des.usedBytes() == protocol::AecpAemStopStreamingResponsePayloadSize && "Used more bytes than specified in protocol constant");

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
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: Unknown DESCRIPTOR_STREAM type"); // Malformed packet
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
				auto const payloadInfo = aem.getPayload();
				auto* const commandPayload = payloadInfo.first;
				auto const commandPayloadLength = payloadInfo.second;

				if (commandPayload == nullptr || commandPayloadLength < protocol::AecpAemGetAudioMapResponsePayloadMinSize) // Malformed packet
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: GET_AUDIO_MAP");

				// Check payload audio map data
				Deserializer des(commandPayload, commandPayloadLength);
				model::DescriptorType descriptorType;
				model::DescriptorIndex descriptorIndex;
				model::MapIndex mapIndex;
				model::MapIndex numberOfMaps;
				model::MapIndex numberOfMappings;
				std::uint16_t reserved;
				des >> descriptorType >> descriptorIndex >> mapIndex >> numberOfMaps >> numberOfMappings >> reserved;

				// Check descriptor variable size
				constexpr size_t mapInfoSize = sizeof(model::AudioMapping::streamIndex) + sizeof(model::AudioMapping::streamChannel) + sizeof(model::AudioMapping::clusterOffset) + sizeof(model::AudioMapping::clusterChannel);
				auto const mappingsSize = mapInfoSize * numberOfMappings;
				if (des.remaining() < mappingsSize) // Malformed packet
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: GET_AUDIO_MAP"); // Malformed packet
				// Unpack descriptor remaining data
				model::AudioMappings mappings;
				for (auto index = 0u; index < numberOfMappings; ++index)
				{
					model::AudioMapping mapping;
					des >> mapping.streamIndex >> mapping.streamChannel >> mapping.clusterOffset >> mapping.clusterChannel;
					mappings.push_back(mapping);
				}
				assert(des.usedBytes() == (protocol::AecpAemGetAudioMapResponsePayloadMinSize + mappingsSize) && "Used more bytes than specified in protocol constant");

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
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: Unknown DESCRIPTOR_STREAM type"); // Malformed packet
			}
		},
		// Add Audio Mappings
		{ protocol::AemCommandType::AddAudioMappings.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				auto const payloadInfo = aem.getPayload();
				auto* const commandPayload = payloadInfo.first;
				auto const commandPayloadLength = payloadInfo.second;

				if (commandPayload == nullptr || commandPayloadLength < protocol::AecpAemAddAudioMappingsResponsePayloadMinSize) // Malformed packet
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: ADD_AUDIO_MAPPINGS");

				// Check payload audio map data
				Deserializer des(commandPayload, commandPayloadLength);
				model::DescriptorType descriptorType;
				model::DescriptorIndex descriptorIndex;
				model::MapIndex numberOfMappings;
				std::uint16_t reserved;
				des >> descriptorType >> descriptorIndex >> numberOfMappings >> reserved;

				// Check descriptor variable size
				constexpr size_t mapInfoSize = sizeof(model::AudioMapping::streamIndex) + sizeof(model::AudioMapping::streamChannel) + sizeof(model::AudioMapping::clusterOffset) + sizeof(model::AudioMapping::clusterChannel);
				auto const mappingsSize = mapInfoSize * numberOfMappings;
				if (des.remaining() < mappingsSize) // Malformed packet
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: ADD_AUDIO_MAPPINGS"); // Malformed packet
				// Unpack descriptor remaining data
				model::AudioMappings mappings;
				for (auto index = 0u; index < numberOfMappings; ++index)
				{
					model::AudioMapping mapping;
					des >> mapping.streamIndex >> mapping.streamChannel >> mapping.clusterOffset >> mapping.clusterChannel;
					mappings.push_back(mapping);
				}
				assert(des.usedBytes() == (protocol::AecpAemAddAudioMappingsResponsePayloadMinSize + mappingsSize) && "Used more bytes than specified in protocol constant");

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
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: Unknown DESCRIPTOR_STREAM type"); // Malformed packet
			}
		},
		// Remove Audio Mappings
		{ protocol::AemCommandType::RemoveAudioMappings.getValue(), [](ControllerEntityImpl const* const controller, AemCommandStatus const status, protocol::AemAecpdu const& aem, AnswerCallback const& answerCallback)
			{
				auto const payloadInfo = aem.getPayload();
				auto* const commandPayload = payloadInfo.first;
				auto const commandPayloadLength = payloadInfo.second;

				if (commandPayload == nullptr || commandPayloadLength < protocol::AecpAemRemoveAudioMappingsResponsePayloadMinSize) // Malformed packet
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: REMOVE_AUDIO_MAPPINGS");

				// Check payload audio map data
				Deserializer des(commandPayload, commandPayloadLength);
				model::DescriptorType descriptorType;
				model::DescriptorIndex descriptorIndex;
				model::MapIndex numberOfMappings;
				std::uint16_t reserved;
				des >> descriptorType >> descriptorIndex >> numberOfMappings >> reserved;

				// Check descriptor variable size
				constexpr size_t mapInfoSize = sizeof(model::AudioMapping::streamIndex) + sizeof(model::AudioMapping::streamChannel) + sizeof(model::AudioMapping::clusterOffset) + sizeof(model::AudioMapping::clusterChannel);
				auto const mappingsSize = mapInfoSize * numberOfMappings;
				if (des.remaining() < mappingsSize) // Malformed packet
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: REMOVE_AUDIO_MAPPINGS"); // Malformed packet
				// Unpack descriptor remaining data
				model::AudioMappings mappings;
				for (auto index = 0u; index < numberOfMappings; ++index)
				{
					model::AudioMapping mapping;
					des >> mapping.streamIndex >> mapping.streamChannel >> mapping.clusterOffset >> mapping.clusterChannel;
					mappings.push_back(mapping);
				}
				assert(des.usedBytes() == (protocol::AecpAemRemoveAudioMappingsResponsePayloadMinSize + mappingsSize) && "Used more bytes than specified in protocol constant");

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
					throw CommandException(AemCommandStatus::ProtocolError, "Malformed AEM response: Unknown DESCRIPTOR_STREAM type"); // Malformed packet
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
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Unsolicited AEM response ") + std::string(aem.getCommandType()) + " not handled (" + toHexString(aem.getCommandType().getValue())  + ")");
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
				Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Info, std::string("Failed to process ") + std::string(aem.getCommandType()) + "AEM response: " + e.what());
			}
			invokeProtectedHandler(onErrorCallback, st);
			return;
		}
		catch (CommandException const& e)
		{
			auto st = e.getStatus();
#if defined(IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES)
			if (st == AemCommandStatus::ProtocolError && status != AemCommandStatus::Success)
			{
				// Allow this packet to go through as a non-success response, but some fields might have the default initial value which might not be valid (the spec says even in a response message, some fields have a meaningful value)
				st = status;
			}
#endif // IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Info, std::string("Failed to process AEM response: ") + e.what());
			invokeProtectedHandler(onErrorCallback, st);
			return;
		}
		catch (std::exception const& e) // Mainly unpacking errors
		{
			Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Info, std::string("Failed to process AEM response: ") + e.what());
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

void ControllerEntityImpl::lockEntity(UniqueIdentifier const targetEntityID, LockEntityHandler const& handler) const noexcept
{
	try
	{
		Serializer<protocol::AecpAemLockEntityCommandPayloadSize> ser;
		ser << protocol::AemLockEntityFlags::None; // aem_lock_flags
		ser << getNullIdentifier(); // locked_entity_id
#pragma message("TBD: Change the API to allow partial EM lock")
		ser << model::DescriptorType::Entity; // descriptor_type
		ser << model::DescriptorIndex{ 0 }; // descriptor_index

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
		Serializer<protocol::AecpAemLockEntityCommandPayloadSize> ser;
		ser << protocol::AemLockEntityFlags::Release; // aem_lock_flags
		ser << getNullIdentifier(); // locked_entity_id
#pragma message("TBD: Change the API to allow partial EM lock")
		ser << model::DescriptorType::Entity; // descriptor_type
		ser << model::DescriptorIndex{ 0 }; // descriptor_index

		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1);
		sendAemCommand(targetEntityID, protocol::AemCommandType::LockEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize unlockEntity: ") + e.what());
	}
}

void ControllerEntityImpl::acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, AcquireEntityHandler const& handler) const noexcept
{
	try
	{
		Serializer<protocol::AecpAemAcquireEntityCommandPayloadSize> ser;
		ser << (isPersistent ? protocol::AemAcquireEntityFlags::Persistent : protocol::AemAcquireEntityFlags::None); // aem_acquire_flags
		ser << getNullIdentifier(); // owner_entity_id
#pragma message("TBD: Change the API to allow partial EM acquire")
		ser << model::DescriptorType::Entity; // descriptor_type
		ser << model::DescriptorIndex{ 0 }; // descriptor_index

		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, getNullIdentifier());
		sendAemCommand(targetEntityID, protocol::AemCommandType::AcquireEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize acquireEntity: ") + e.what());
	}
}

void ControllerEntityImpl::releaseEntity(UniqueIdentifier const targetEntityID, ReleaseEntityHandler const& handler) const noexcept
{
	try
	{
		Serializer<protocol::AecpAemAcquireEntityCommandPayloadSize> ser;
		ser << protocol::AemAcquireEntityFlags::Release; // aem_acquire_flags
		ser << getNullIdentifier(); // owner_entity_id
#pragma message("TBD: Change the API to allow partial EM acquire")
		ser << model::DescriptorType::Entity; // descriptor_type
		ser << model::DescriptorIndex{ 0 }; // descriptor_index

		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, getNullIdentifier());
		sendAemCommand(targetEntityID, protocol::AemCommandType::AcquireEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize releaseEntity: ") + e.what());
	}
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
		Serializer<protocol::AecpAemReadDescriptorCommandPayloadSize> ser;
		ser << model::ConfigurationIndex{ 0x0000 }; // configuration_index
		ser << std::uint16_t{ 0x0000 }; // reserved
		ser << model::DescriptorType::Entity; // descriptor_type
		ser << model::DescriptorIndex{ 0 }; // descriptor_index

		model::EntityDescriptor const emptyDescriptor{ { model::DescriptorType::Entity, model::DescriptorIndex{ 0 } } };
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, emptyDescriptor);
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
		Serializer<protocol::AecpAemReadDescriptorCommandPayloadSize> ser;
		ser << model::ConfigurationIndex{ 0x0000 }; // configuration_index
		ser << std::uint16_t{ 0x0000 }; // reserved
		ser << model::DescriptorType::Configuration; // descriptor_type
		ser << model::DescriptorIndex{ configurationIndex }; // descriptor_index

		model::ConfigurationDescriptor const emptyDescriptor{ { model::DescriptorType::Configuration, configurationIndex } };
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, emptyDescriptor);
		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize readConfigurationDescriptor: ") + e.what());
	}
}

void ControllerEntityImpl::readLocaleDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::LocaleIndex const localeIndex, LocaleDescriptorHandler const& handler) const noexcept
{
	try
	{
		Serializer<protocol::AecpAemReadDescriptorCommandPayloadSize> ser;
		ser << model::ConfigurationIndex{ configurationIndex }; // configuration_index
		ser << std::uint16_t{ 0x0000 }; // reserved
		ser << model::DescriptorType::Locale; // descriptor_type
		ser << model::DescriptorIndex{ localeIndex }; // descriptor_index

		model::LocaleDescriptor const emptyDescriptor{ { model::DescriptorType::Locale, localeIndex } };
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, emptyDescriptor);
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
		Serializer<protocol::AecpAemReadDescriptorCommandPayloadSize> ser;
		ser << model::ConfigurationIndex{ configurationIndex }; // configuration_index
		ser << std::uint16_t{ 0x0000 }; // reserved
		ser << model::DescriptorType::Strings; // descriptor_type
		ser << model::DescriptorIndex{ stringsIndex }; // descriptor_index

		model::StringsDescriptor const emptyDescriptor{ { model::DescriptorType::Strings, stringsIndex } };
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, emptyDescriptor);
		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize readStringsDescriptor: ") + e.what());
	}
}

void ControllerEntityImpl::readStreamInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, StreamInputDescriptorHandler const& handler) const noexcept
{
	try
	{
		Serializer<protocol::AecpAemReadDescriptorCommandPayloadSize> ser;
		ser << model::ConfigurationIndex{ configurationIndex }; // configuration_index
		ser << std::uint16_t{ 0x0000 }; // reserved
		ser << model::DescriptorType::StreamInput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index

		model::StreamDescriptor const emptyDescriptor{ model::DescriptorType::StreamInput, streamIndex };
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, emptyDescriptor);
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
		Serializer<protocol::AecpAemReadDescriptorCommandPayloadSize> ser;
		ser << model::ConfigurationIndex{ configurationIndex }; // configuration_index
		ser << std::uint16_t{ 0x0000 }; // reserved
		ser << model::DescriptorType::StreamOutput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index

		model::StreamDescriptor const emptyDescriptor{ model::DescriptorType::StreamOutput, streamIndex };
		auto const errorCallback = ControllerEntityImpl::makeAECPErrorHandler(handler, this, targetEntityID, std::placeholders::_1, emptyDescriptor);
		sendAemCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch (std::exception const& e)
	{
		Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("Failed to serialize readStreamOutputDescriptor: ") + e.what());
	}
}

void ControllerEntityImpl::setStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept
{
	try
	{
		Serializer<protocol::AecpAemSetStreamFormatCommandPayloadSize> ser;
		ser << model::DescriptorType::StreamInput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index
		ser << streamFormat; // stream_format

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
		Serializer<protocol::AecpAemSetStreamFormatCommandPayloadSize> ser;
		ser << model::DescriptorType::StreamOutput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index
		ser << streamFormat; // stream_format

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
		Serializer<protocol::AecpAemGetAudioMapCommandPayloadSize> ser;
		ser << model::DescriptorType::StreamInput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index
		ser << model::MapIndex(mapIndex); // map_index
		ser << std::uint16_t{ 0x0000 }; // reserved

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
		Serializer<protocol::AecpAemGetAudioMapCommandPayloadSize> ser;
		ser << model::DescriptorType::StreamOutput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index
		ser << model::MapIndex(mapIndex); // map_index
		ser << std::uint16_t{ 0x0000 }; // reserved

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
		Serializer<protocol::AecpAemAddAudioMappingsCommandPayloadMaxSize> ser;
		ser << model::DescriptorType::StreamInput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index
		ser << model::MapIndex(mappings.size()); // number_of_mappings
		ser << std::uint16_t{ 0x0000 }; // reserved
		for (auto const& map : mappings)
		{
			ser << map.streamIndex << map.streamChannel << map.clusterOffset << map.clusterChannel;
		}

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
		Serializer<protocol::AecpAemAddAudioMappingsCommandPayloadMaxSize> ser;
		ser << model::DescriptorType::StreamOutput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index
		ser << model::MapIndex(mappings.size()); // number_of_mappings
		ser << std::uint16_t{ 0x0000 }; // reserved
		for (auto const& map : mappings)
		{
			ser << map.streamIndex << map.streamChannel << map.clusterOffset << map.clusterChannel;
		}

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
		Serializer<protocol::AecpAemRemoveAudioMappingsCommandPayloadMaxSize> ser;
		ser << model::DescriptorType::StreamInput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index
		ser << model::MapIndex(mappings.size()); // number_of_mappings
		ser << std::uint16_t{ 0x0000 }; // reserved
		for (auto const& map : mappings)
		{
			ser << map.streamIndex << map.streamChannel << map.clusterOffset << map.clusterChannel;
		}

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
		Serializer<protocol::AecpAemRemoveAudioMappingsCommandPayloadMaxSize> ser;
		ser << model::DescriptorType::StreamOutput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index
		ser << model::MapIndex(mappings.size()); // number_of_mappings
		ser << std::uint16_t{ 0x0000 }; // reserved
		for (auto const& map : mappings)
		{
			ser << map.streamIndex << map.streamChannel << map.clusterOffset << map.clusterChannel;
		}

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
		Serializer<protocol::AecpAemGetStreamInfoCommandPayloadSize> ser;
		ser << model::DescriptorType::StreamInput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index

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
		Serializer<protocol::AecpAemStartStreamingCommandPayloadSize> ser;
		ser << model::DescriptorType::StreamInput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index

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
		Serializer<protocol::AecpAemStartStreamingCommandPayloadSize> ser;
		ser << model::DescriptorType::StreamOutput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index

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
		Serializer<protocol::AecpAemStopStreamingCommandPayloadSize> ser;
		ser << model::DescriptorType::StreamInput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index

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
		Serializer<protocol::AecpAemStopStreamingCommandPayloadSize> ser;
		ser << model::DescriptorType::StreamOutput; // descriptor_type
		ser << model::DescriptorIndex{ streamIndex }; // descriptor_index

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
