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
* @file controllerCapabilityDelegate.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/utils.hpp"

#include "controllerCapabilityDelegate.hpp"
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
namespace controller
{
/* ************************************************************************** */
/* Static variables used for bindings                                         */
/* ************************************************************************** */
static model::AudioMappings const s_emptyMappings{}; // Empty AudioMappings used by timeout callback (needs a ref to an AudioMappings)
static model::StreamInfo const s_emptyStreamInfo{}; // Empty StreamInfo used by timeout callback (needs a ref to a StreamInfo)
static MemoryBuffer const s_emptyPackedControlValues{}; // Empty ControlValues used by timeout callback (needs a ref to a MemoryBuffer)
static model::AvbInfo const s_emptyAvbInfo{}; // Empty AvbInfo used by timeout callback (needs a ref to an AvbInfo)
static model::AsPath const s_emptyAsPath{}; // Empty AsPath used by timeout callback (needs a ref to an AsPath)
static model::AvdeccFixedString const s_emptyAvdeccFixedString{}; // Empty AvdeccFixedString used by timeout callback (needs a ref to a std::string)
static model::MilanInfo const s_emptyMilanInfo{}; // Empty MilanInfo used by timeout callback (need a ref to a MilanInfo)

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
CapabilityDelegate::CapabilityDelegate(protocol::ProtocolInterface* const protocolInterface, controller::Delegate* controllerDelegate, Interface& controllerInterface, UniqueIdentifier const controllerID) noexcept
	: _protocolInterface(protocolInterface)
	, _controllerDelegate(controllerDelegate)
	, _controllerInterface(controllerInterface)
	, _controllerID(controllerID)
{
}

CapabilityDelegate::~CapabilityDelegate() noexcept {}

/* ************************************************************************** */
/* Controller methods                                                         */
/* ************************************************************************** */
void CapabilityDelegate::setControllerDelegate(controller::Delegate* const delegate) noexcept
{
#pragma message("TODO: Protect the _controllerDelegate so it cannot be changed while it's being used (use pi's lock ?? Check for deadlocks!)")
	_controllerDelegate = delegate;
}

/* Discovery Protocol (ADP) */
/* Enumeration and Control Protocol (AECP) AEM */
void CapabilityDelegate::acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, Interface::AcquireEntityHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, UniqueIdentifier::getNullUniqueIdentifier(), descriptorType, descriptorIndex);
	try
	{
		auto const ser = protocol::aemPayload::serializeAcquireEntityCommand(isPersistent ? protocol::AemAcquireEntityFlags::Persistent : protocol::AemAcquireEntityFlags::None, UniqueIdentifier::getNullUniqueIdentifier(), descriptorType, descriptorIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::AcquireEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize acquireEntity: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::releaseEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, Interface::ReleaseEntityHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, UniqueIdentifier::getNullUniqueIdentifier(), descriptorType, descriptorIndex);
	try
	{
		auto const ser = protocol::aemPayload::serializeAcquireEntityCommand(protocol::AemAcquireEntityFlags::Release, UniqueIdentifier::getNullUniqueIdentifier(), descriptorType, descriptorIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::AcquireEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize releaseEntity: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::lockEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, Interface::LockEntityHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, UniqueIdentifier::getNullUniqueIdentifier(), descriptorType, descriptorIndex);
	try
	{
		auto const ser = protocol::aemPayload::serializeLockEntityCommand(protocol::AemLockEntityFlags::None, UniqueIdentifier::getNullUniqueIdentifier(), descriptorType, descriptorIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::LockEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize lockEntity: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::unlockEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, Interface::UnlockEntityHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, UniqueIdentifier::getNullUniqueIdentifier(), descriptorType, descriptorIndex);
	try
	{
		auto const ser = protocol::aemPayload::serializeLockEntityCommand(protocol::AemLockEntityFlags::Unlock, UniqueIdentifier::getNullUniqueIdentifier(), descriptorType, descriptorIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::LockEntity, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize unlockEntity: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::queryEntityAvailable(UniqueIdentifier const targetEntityID, Interface::QueryEntityAvailableHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1);
	sendAemAecpCommand(targetEntityID, protocol::AemCommandType::EntityAvailable, nullptr, 0, errorCallback, handler);
}

void CapabilityDelegate::queryControllerAvailable(UniqueIdentifier const targetEntityID, Interface::QueryControllerAvailableHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1);
	sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ControllerAvailable, nullptr, 0, errorCallback, handler);
}

void CapabilityDelegate::registerUnsolicitedNotifications(UniqueIdentifier const targetEntityID, Interface::RegisterUnsolicitedNotificationsHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1);
	sendAemAecpCommand(targetEntityID, protocol::AemCommandType::RegisterUnsolicitedNotification, nullptr, 0, errorCallback, handler);
}

void CapabilityDelegate::unregisterUnsolicitedNotifications(UniqueIdentifier const targetEntityID, Interface::UnregisterUnsolicitedNotificationsHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1);
	sendAemAecpCommand(targetEntityID, protocol::AemCommandType::DeregisterUnsolicitedNotification, nullptr, 0, errorCallback, handler);
}

void CapabilityDelegate::readEntityDescriptor(UniqueIdentifier const targetEntityID, Interface::EntityDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, model::EntityDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(model::ConfigurationIndex(0u), model::DescriptorType::Entity, model::DescriptorIndex(0u));
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readEntityDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readConfigurationDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, Interface::ConfigurationDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, model::ConfigurationDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(model::ConfigurationIndex(0u), model::DescriptorType::Configuration, static_cast<model::DescriptorIndex>(configurationIndex)); // Passing configurationIndex as a DescriptorIndex is NOT an error. See 7.4.5.1
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readConfigurationDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readAudioUnitDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, Interface::AudioUnitDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, audioUnitIndex, model::AudioUnitDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::AudioUnit, audioUnitIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readAudioUnitDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readStreamInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, Interface::StreamInputDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, streamIndex, model::StreamDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::StreamInput, streamIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readStreamInputDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readStreamOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, Interface::StreamOutputDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, streamIndex, model::StreamDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::StreamOutput, streamIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readStreamOutputDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readJackInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, Interface::JackInputDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, jackIndex, model::JackDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::JackInput, jackIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readJackInputDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readJackOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, Interface::JackOutputDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, jackIndex, model::JackDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::JackOutput, jackIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readJackOutputDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readAvbInterfaceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, Interface::AvbInterfaceDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, avbInterfaceIndex, model::AvbInterfaceDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::AvbInterface, avbInterfaceIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readAvbInterfaceDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readClockSourceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, Interface::ClockSourceDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, clockSourceIndex, model::ClockSourceDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::ClockSource, clockSourceIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readClockSourceDescriptor: '}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readMemoryObjectDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, Interface::MemoryObjectDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, memoryObjectIndex, model::MemoryObjectDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::MemoryObject, memoryObjectIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readMemoryObjectDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readLocaleDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::LocaleIndex const localeIndex, Interface::LocaleDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, localeIndex, model::LocaleDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::Locale, localeIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readLocaleDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readStringsDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StringsIndex const stringsIndex, Interface::StringsDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, stringsIndex, model::StringsDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::Strings, stringsIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readStringsDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readStreamPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamPortIndex const streamPortIndex, Interface::StreamPortInputDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, streamPortIndex, model::StreamPortDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::StreamPortInput, streamPortIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readStreamPortInputDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readStreamPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamPortIndex const streamPortIndex, Interface::StreamPortOutputDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, streamPortIndex, model::StreamPortDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::StreamPortOutput, streamPortIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readStreamPortOutputDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readExternalPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ExternalPortIndex const externalPortIndex, Interface::ExternalPortInputDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, externalPortIndex, model::ExternalPortDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::ExternalPortInput, externalPortIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readExternalPortInputDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readExternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ExternalPortIndex const externalPortIndex, Interface::ExternalPortOutputDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, externalPortIndex, model::ExternalPortDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::ExternalPortOutput, externalPortIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readExternalPortInputDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readInternalPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::InternalPortIndex const internalPortIndex, Interface::InternalPortInputDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, internalPortIndex, model::InternalPortDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::InternalPortInput, internalPortIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readInternalPortInputDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readInternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::InternalPortIndex const internalPortIndex, Interface::InternalPortOutputDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, internalPortIndex, model::InternalPortDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::InternalPortOutput, internalPortIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readInternalPortOutputDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readAudioClusterDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const clusterIndex, Interface::AudioClusterDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, clusterIndex, model::AudioClusterDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::AudioCluster, clusterIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readAudioClusterDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readAudioMapDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MapIndex const mapIndex, Interface::AudioMapDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, mapIndex, model::AudioMapDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::AudioMap, mapIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readAudioMapDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readControlDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, Interface::ControlDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, controlIndex, model::ControlDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::Control, controlIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readControlDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::readClockDomainDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, Interface::ClockDomainDescriptorHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, clockDomainIndex, model::ClockDomainDescriptor{});
	try
	{
		auto const ser = protocol::aemPayload::serializeReadDescriptorCommand(configurationIndex, model::DescriptorType::ClockDomain, clockDomainIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::ReadDescriptor, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize readClockDomainDescriptor: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setConfiguration(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, Interface::SetConfigurationHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetConfigurationCommand(configurationIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetConfiguration, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setConfiguration: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getConfiguration(UniqueIdentifier const targetEntityID, Interface::GetConfigurationHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, model::ConfigurationIndex{ 0u });
	sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetConfiguration, nullptr, 0, errorCallback, handler);
}

void CapabilityDelegate::setStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, Interface::SetStreamInputFormatHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamIndex, model::StreamFormat());
	try
	{
		auto const ser = protocol::aemPayload::serializeSetStreamFormatCommand(model::DescriptorType::StreamInput, streamIndex, streamFormat);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetStreamFormat, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setStreamInputFormat: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::GetStreamInputFormatHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamIndex, model::StreamFormat());
	try
	{
		auto const ser = protocol::aemPayload::serializeGetStreamFormatCommand(model::DescriptorType::StreamInput, streamIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetStreamFormat, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getStreamInputFormat: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, Interface::SetStreamOutputFormatHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamIndex, model::StreamFormat());
	try
	{
		auto const ser = protocol::aemPayload::serializeSetStreamFormatCommand(model::DescriptorType::StreamOutput, streamIndex, streamFormat);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetStreamFormat, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setStreamOutputFormat: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::GetStreamOutputFormatHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamIndex, model::StreamFormat());
	try
	{
		auto const ser = protocol::aemPayload::serializeGetStreamFormatCommand(model::DescriptorType::StreamOutput, streamIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetStreamFormat, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getStreamOutputFormat: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getStreamPortInputAudioMap(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::MapIndex const mapIndex, Interface::GetStreamPortInputAudioMapHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamPortIndex, model::MapIndex(0), mapIndex, s_emptyMappings);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetAudioMapCommand(model::DescriptorType::StreamPortInput, streamPortIndex, mapIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetAudioMap, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getStreamInputAudioMap: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getStreamPortOutputAudioMap(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::MapIndex const mapIndex, Interface::GetStreamPortOutputAudioMapHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamPortIndex, model::MapIndex(0), mapIndex, s_emptyMappings);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetAudioMapCommand(model::DescriptorType::StreamPortOutput, streamPortIndex, mapIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetAudioMap, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getStreamOutputAudioMap: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, Interface::AddStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamPortIndex, s_emptyMappings);
	try
	{
		auto const ser = protocol::aemPayload::serializeAddAudioMappingsCommand(model::DescriptorType::StreamPortInput, streamPortIndex, mappings);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::AddAudioMappings, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize addStreamInputAudioMappings: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, Interface::AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamPortIndex, s_emptyMappings);
	try
	{
		auto const ser = protocol::aemPayload::serializeAddAudioMappingsCommand(model::DescriptorType::StreamPortOutput, streamPortIndex, mappings);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::AddAudioMappings, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize addStreamOutputAudioMappings: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, Interface::RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamPortIndex, s_emptyMappings);
	try
	{
		auto const ser = protocol::aemPayload::serializeRemoveAudioMappingsCommand(model::DescriptorType::StreamPortInput, streamPortIndex, mappings);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::RemoveAudioMappings, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize removeStreamInputAudioMappings: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, Interface::RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamPortIndex, s_emptyMappings);
	try
	{
		auto const ser = protocol::aemPayload::serializeRemoveAudioMappingsCommand(model::DescriptorType::StreamPortOutput, streamPortIndex, mappings);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::RemoveAudioMappings, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize removeStreamOutputAudioMappings: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setStreamInputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamInfo const& info, Interface::SetStreamInputInfoHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamIndex, s_emptyStreamInfo);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetStreamInfoCommand(model::DescriptorType::StreamInput, streamIndex, info);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetStreamInfo, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setStreamInputInfo: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setStreamOutputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamInfo const& info, Interface::SetStreamOutputInfoHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamIndex, s_emptyStreamInfo);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetStreamInfoCommand(model::DescriptorType::StreamOutput, streamIndex, info);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetStreamInfo, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setStreamOutputInfo: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getStreamInputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::GetStreamInputInfoHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamIndex, s_emptyStreamInfo);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetStreamInfoCommand(model::DescriptorType::StreamInput, streamIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetStreamInfo, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getStreamInputInfo: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getStreamOutputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::GetStreamOutputInfoHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamIndex, s_emptyStreamInfo);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetStreamInfoCommand(model::DescriptorType::StreamOutput, streamIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetStreamInfo, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getStreamOutputInfo: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setEntityName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityName, Interface::SetEntityNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::Entity, 0, 0, 0, entityName);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getEntityName(UniqueIdentifier const targetEntityID, Interface::GetEntityNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::Entity, 0, 0, 0);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setEntityGroupName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityGroupName, Interface::SetEntityGroupNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::Entity, 0, 1, 0, entityGroupName);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getEntityGroupName(UniqueIdentifier const targetEntityID, Interface::GetEntityGroupNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::Entity, 0, 1, 0);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setConfigurationName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvdeccFixedString const& configurationName, Interface::SetConfigurationNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::Configuration, configurationIndex, 0, 0, configurationName);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getConfigurationName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, Interface::GetConfigurationNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::Configuration, configurationIndex, 0, 0);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setAudioUnitName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, model::AvdeccFixedString const& audioUnitName, Interface::SetAudioUnitNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, audioUnitIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::AudioUnit, audioUnitIndex, 0, configurationIndex, audioUnitName);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getAudioUnitName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, Interface::GetAudioUnitNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, audioUnitIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::AudioUnit, audioUnitIndex, 0, configurationIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamInputName, Interface::SetStreamInputNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, streamIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::StreamInput, streamIndex, 0, configurationIndex, streamInputName);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, Interface::GetStreamInputNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, streamIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::StreamInput, streamIndex, 0, configurationIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamOutputName, Interface::SetStreamOutputNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, streamIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::StreamOutput, streamIndex, 0, configurationIndex, streamOutputName);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, Interface::GetStreamOutputNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, streamIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::StreamOutput, streamIndex, 0, configurationIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setAvbInterfaceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, model::AvdeccFixedString const& avbInterfaceName, Interface::SetAvbInterfaceNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, avbInterfaceIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::AvbInterface, avbInterfaceIndex, 0, configurationIndex, avbInterfaceName);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getAvbInterfaceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, Interface::GetAvbInterfaceNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, avbInterfaceIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::AvbInterface, avbInterfaceIndex, 0, configurationIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setClockSourceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, model::AvdeccFixedString const& clockSourceName, Interface::SetClockSourceNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, clockSourceIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::ClockSource, clockSourceIndex, 0, configurationIndex, clockSourceName);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getClockSourceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, Interface::GetClockSourceNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, clockSourceIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::ClockSource, clockSourceIndex, 0, configurationIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setMemoryObjectName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, model::AvdeccFixedString const& memoryObjectName, Interface::SetMemoryObjectNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, memoryObjectIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::MemoryObject, memoryObjectIndex, 0, configurationIndex, memoryObjectName);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getMemoryObjectName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, Interface::GetMemoryObjectNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, memoryObjectIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::MemoryObject, memoryObjectIndex, 0, configurationIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setAudioClusterName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const audioClusterIndex, model::AvdeccFixedString const& audioClusterName, Interface::SetAudioClusterNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, audioClusterIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::AudioCluster, audioClusterIndex, 0, configurationIndex, audioClusterName);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getAudioClusterName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const audioClusterIndex, Interface::GetAudioClusterNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, audioClusterIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::AudioCluster, audioClusterIndex, 0, configurationIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setControlName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, model::AvdeccFixedString const& controlName, Interface::SetControlNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, controlIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::Control, controlIndex, 0, configurationIndex, controlName);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getControlName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, Interface::GetControlNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, controlIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::Control, controlIndex, 0, configurationIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setClockDomainName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, model::AvdeccFixedString const& clockDomainName, Interface::SetClockDomainNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, clockDomainIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetNameCommand(model::DescriptorType::ClockDomain, clockDomainIndex, 0, configurationIndex, clockDomainName);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getClockDomainName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, Interface::GetClockDomainNameHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, clockDomainIndex, s_emptyAvdeccFixedString);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetNameCommand(model::DescriptorType::ClockDomain, clockDomainIndex, 0, configurationIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetName, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getName: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, model::AudioUnitIndex const audioUnitIndex, model::SamplingRate const samplingRate, Interface::SetAudioUnitSamplingRateHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, audioUnitIndex, model::SamplingRate::getNullSamplingRate());
	try
	{
		auto const ser = protocol::aemPayload::serializeSetSamplingRateCommand(model::DescriptorType::AudioUnit, audioUnitIndex, samplingRate);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetSamplingRate, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setAudioUnitSamplingRate: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, model::AudioUnitIndex const audioUnitIndex, Interface::GetAudioUnitSamplingRateHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, audioUnitIndex, model::SamplingRate::getNullSamplingRate());
	try
	{
		auto const ser = protocol::aemPayload::serializeGetSamplingRateCommand(model::DescriptorType::AudioUnit, audioUnitIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetSamplingRate, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getAudioUnitSamplingRate: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const videoClusterIndex, model::SamplingRate const samplingRate, Interface::SetVideoClusterSamplingRateHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, videoClusterIndex, model::SamplingRate::getNullSamplingRate());
	try
	{
		auto const ser = protocol::aemPayload::serializeSetSamplingRateCommand(model::DescriptorType::VideoCluster, videoClusterIndex, samplingRate);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetSamplingRate, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setVideoClusterSamplingRate: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const videoClusterIndex, Interface::GetVideoClusterSamplingRateHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, videoClusterIndex, model::SamplingRate::getNullSamplingRate());
	try
	{
		auto const ser = protocol::aemPayload::serializeGetSamplingRateCommand(model::DescriptorType::VideoCluster, videoClusterIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetSamplingRate, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getVideoClusterSamplingRate: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const sensorClusterIndex, model::SamplingRate const samplingRate, Interface::SetSensorClusterSamplingRateHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, sensorClusterIndex, model::SamplingRate::getNullSamplingRate());
	try
	{
		auto const ser = protocol::aemPayload::serializeSetSamplingRateCommand(model::DescriptorType::SensorCluster, sensorClusterIndex, samplingRate);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetSamplingRate, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setSensorClusterSamplingRate: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const sensorClusterIndex, Interface::GetSensorClusterSamplingRateHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, sensorClusterIndex, model::SamplingRate::getNullSamplingRate());
	try
	{
		auto const ser = protocol::aemPayload::serializeGetSamplingRateCommand(model::DescriptorType::SensorCluster, sensorClusterIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetSamplingRate, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getSensorClusterSamplingRate: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setClockSource(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, model::ClockSourceIndex const clockSourceIndex, Interface::SetClockSourceHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, clockDomainIndex, model::ClockSourceIndex{ 0u });
	try
	{
		auto const ser = protocol::aemPayload::serializeSetClockSourceCommand(model::DescriptorType::ClockDomain, clockDomainIndex, clockSourceIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetClockSource, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setClockSource: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getClockSource(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, Interface::GetClockSourceHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, clockDomainIndex, model::ClockSourceIndex{ 0u });
	try
	{
		auto const ser = protocol::aemPayload::serializeGetClockSourceCommand(model::DescriptorType::ClockDomain, clockDomainIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetClockSource, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getClockSource: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setControlValues(UniqueIdentifier const targetEntityID, model::ControlIndex const controlIndex, model::ControlValues const& controlValues, Interface::SetControlValuesHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, controlIndex, s_emptyPackedControlValues);
	try
	{
		auto const ser = protocol::aemPayload::serializeSetControlCommand(model::DescriptorType::Control, controlIndex, controlValues);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetControl, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] protocol::aemPayload::UnsupportedValueException const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setControlValues: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setControlValues: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getControlValues(UniqueIdentifier const targetEntityID, model::ControlIndex const controlIndex, Interface::GetControlValuesHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, controlIndex, s_emptyPackedControlValues);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetControlCommand(model::DescriptorType::Control, controlIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetControl, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getControlValues: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::startStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::StartStreamInputHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamIndex);
	try
	{
		auto const ser = protocol::aemPayload::serializeStartStreamingCommand(model::DescriptorType::StreamInput, streamIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::StartStreaming, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize startStreamInput: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::startStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::StartStreamOutputHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamIndex);
	try
	{
		auto const ser = protocol::aemPayload::serializeStartStreamingCommand(model::DescriptorType::StreamOutput, streamIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::StartStreaming, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize startStreamOutput: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::stopStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::StopStreamInputHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamIndex);
	try
	{
		auto const ser = protocol::aemPayload::serializeStopStreamingCommand(model::DescriptorType::StreamInput, streamIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::StopStreaming, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize stopStreamInput: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::stopStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::StopStreamOutputHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamIndex);
	try
	{
		auto const ser = protocol::aemPayload::serializeStopStreamingCommand(model::DescriptorType::StreamOutput, streamIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::StopStreaming, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize stopStreamOutput: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getAvbInfo(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, Interface::GetAvbInfoHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, avbInterfaceIndex, s_emptyAvbInfo);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetAvbInfoCommand(model::DescriptorType::AvbInterface, avbInterfaceIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetAvbInfo, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getAvbInfo: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getAsPath(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, Interface::GetAsPathHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, avbInterfaceIndex, s_emptyAsPath);
	try
	{
		auto const ser = protocol::aemPayload::serializeGetAsPathCommand(avbInterfaceIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetAsPath, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getAsPath: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getEntityCounters(UniqueIdentifier const targetEntityID, Interface::GetEntityCountersHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, EntityCounterValidFlags{}, model::DescriptorCounters{});
	try
	{
		auto const ser = protocol::aemPayload::serializeGetCountersCommand(model::DescriptorType::Entity, 0);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetCounters, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getEntityCounters: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getAvbInterfaceCounters(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, Interface::GetAvbInterfaceCountersHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, avbInterfaceIndex, AvbInterfaceCounterValidFlags{}, model::DescriptorCounters{});
	try
	{
		auto const ser = protocol::aemPayload::serializeGetCountersCommand(model::DescriptorType::AvbInterface, avbInterfaceIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetCounters, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getAvbInterfaceCounters: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getClockDomainCounters(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, Interface::GetClockDomainCountersHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, clockDomainIndex, ClockDomainCounterValidFlags{}, model::DescriptorCounters{});
	try
	{
		auto const ser = protocol::aemPayload::serializeGetCountersCommand(model::DescriptorType::ClockDomain, clockDomainIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetCounters, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getClockDomainCounters: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getStreamInputCounters(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::GetStreamInputCountersHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamIndex, StreamInputCounterValidFlags{}, model::DescriptorCounters{});
	try
	{
		auto const ser = protocol::aemPayload::serializeGetCountersCommand(model::DescriptorType::StreamInput, streamIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetCounters, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getStreamInputCounters: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getStreamOutputCounters(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, Interface::GetStreamOutputCountersHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, streamIndex, StreamOutputCounterValidFlags{}, model::DescriptorCounters{});
	try
	{
		auto const ser = protocol::aemPayload::serializeGetCountersCommand(model::DescriptorType::StreamOutput, streamIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetCounters, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getStreamOutputCounters: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::startOperation(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, Interface::StartOperationHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, descriptorType, descriptorIndex, model::OperationID{ 0u }, operationType, MemoryBuffer{});
	try
	{
		auto const ser = protocol::aemPayload::serializeStartOperationCommand(descriptorType, descriptorIndex, model::OperationID{ 0u }, operationType, memoryBuffer);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::StartOperation, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize startOperation: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::abortOperation(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, model::OperationID const operationID, Interface::AbortOperationHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, descriptorType, descriptorIndex, operationID);
	try
	{
		auto const ser = protocol::aemPayload::serializeAbortOperationCommand(descriptorType, descriptorIndex, operationID);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::AbortOperation, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize abortOperation: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::setMemoryObjectLength(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, Interface::SetMemoryObjectLengthHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, memoryObjectIndex, std::uint64_t{ 0u });
	try
	{
		auto const ser = protocol::aemPayload::serializeSetMemoryObjectLengthCommand(configurationIndex, memoryObjectIndex, length);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::SetMemoryObjectLength, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize setMemoryObjectLength: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

void CapabilityDelegate::getMemoryObjectLength(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, Interface::GetMemoryObjectLengthHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAemAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, configurationIndex, memoryObjectIndex, std::uint64_t{ 0u });
	try
	{
		auto const ser = protocol::aemPayload::serializeGetMemoryObjectLengthCommand(configurationIndex, memoryObjectIndex);
		sendAemAecpCommand(targetEntityID, protocol::AemCommandType::GetMemoryObjectLength, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getMemoryObjectLength: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::AemCommandStatus::ProtocolError);
	}
}

/* Enumeration and Control Protocol (AECP) AA */
void CapabilityDelegate::addressAccess(UniqueIdentifier const targetEntityID, addressAccess::Tlvs const& tlvs, Interface::AddressAccessHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeAaAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, addressAccess::Tlvs{});
	sendAaAecpCommand(targetEntityID, tlvs, errorCallback, handler);
}

/* Enumeration and Control Protocol (AECP) MVU (Milan Vendor Unique) */
void CapabilityDelegate::getMilanInfo(UniqueIdentifier const targetEntityID, Interface::GetMilanInfoHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeMvuAECPErrorHandler(handler, &_controllerInterface, targetEntityID, std::placeholders::_1, s_emptyMilanInfo);
	try
	{
		auto const ser = protocol::mvuPayload::serializeGetMilanInfoCommand();
		sendMvuAecpCommand(targetEntityID, protocol::MvuCommandType::GetMilanInfo, ser.data(), ser.size(), errorCallback, handler);
	}
	catch ([[maybe_unused]] std::exception const& e)
	{
		LOG_CONTROLLER_ENTITY_DEBUG(targetEntityID, "Failed to serialize getMilanInfo: {}", e.what());
		utils::invokeProtectedHandler(errorCallback, LocalEntity::MvuCommandStatus::ProtocolError);
	}
}

/* Connection Management Protocol (ACMP) */
void CapabilityDelegate::connectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, Interface::ConnectStreamHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeACMPErrorHandler(handler, &_controllerInterface, talkerStream, listenerStream, std::uint16_t(0), entity::ConnectionFlags{}, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::ConnectRxCommand, talkerStream.entityID, talkerStream.streamIndex, listenerStream.entityID, listenerStream.streamIndex, std::uint16_t(0), errorCallback, handler);
}

void CapabilityDelegate::disconnectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, Interface::DisconnectStreamHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeACMPErrorHandler(handler, &_controllerInterface, talkerStream, listenerStream, std::uint16_t(0), entity::ConnectionFlags{}, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::DisconnectRxCommand, talkerStream.entityID, talkerStream.streamIndex, listenerStream.entityID, listenerStream.streamIndex, std::uint16_t(0), errorCallback, handler);
}

void CapabilityDelegate::disconnectTalkerStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, Interface::DisconnectTalkerStreamHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeACMPErrorHandler(handler, &_controllerInterface, talkerStream, listenerStream, std::uint16_t(0), entity::ConnectionFlags{}, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::DisconnectTxCommand, talkerStream.entityID, talkerStream.streamIndex, listenerStream.entityID, listenerStream.streamIndex, std::uint16_t(0), errorCallback, handler);
}

void CapabilityDelegate::getTalkerStreamState(model::StreamIdentification const& talkerStream, Interface::GetTalkerStreamStateHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeACMPErrorHandler(handler, &_controllerInterface, talkerStream, model::StreamIdentification{}, std::uint16_t(0), entity::ConnectionFlags{}, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::GetTxStateCommand, talkerStream.entityID, talkerStream.streamIndex, UniqueIdentifier::getNullUniqueIdentifier(), model::StreamIndex(0), std::uint16_t(0), errorCallback, handler);
}

void CapabilityDelegate::getListenerStreamState(model::StreamIdentification const& listenerStream, Interface::GetListenerStreamStateHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeACMPErrorHandler(handler, &_controllerInterface, model::StreamIdentification{}, listenerStream, std::uint16_t(0), entity::ConnectionFlags{}, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::GetRxStateCommand, UniqueIdentifier::getNullUniqueIdentifier(), model::StreamIndex(0), listenerStream.entityID, listenerStream.streamIndex, std::uint16_t(0), errorCallback, handler);
}

void CapabilityDelegate::getTalkerStreamConnection(model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex, Interface::GetTalkerStreamConnectionHandler const& handler) const noexcept
{
	auto const errorCallback = LocalEntityImpl<>::makeACMPErrorHandler(handler, &_controllerInterface, talkerStream, model::StreamIdentification{}, connectionIndex, entity::ConnectionFlags{}, std::placeholders::_1);
	sendAcmpCommand(protocol::AcmpMessageType::GetTxConnectionCommand, talkerStream.entityID, talkerStream.streamIndex, UniqueIdentifier::getNullUniqueIdentifier(), model::StreamIndex(0), connectionIndex, errorCallback, handler);
}

/* ************************************************************************** */
/* LocalEntityImpl<>::CapabilityDelegate overrides                          */
/* ************************************************************************** */
void CapabilityDelegate::onTransportError(protocol::ProtocolInterface* const /*pi*/) noexcept
{
	utils::invokeProtectedMethod(&controller::Delegate::onTransportError, _controllerDelegate, &_controllerInterface);
}

/* **** Discovery notifications **** */
void CapabilityDelegate::onLocalEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	// Ignore ourself
	if (entity.getEntityID() == _controllerID)
		return;

	// Forward to RemoteEntityOnline, we handle all discovered entities the same way
	onRemoteEntityOnline(pi, entity);
}

void CapabilityDelegate::onLocalEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept
{
	// Ignore ourself
	if (entityID == _controllerID)
		return;

	// Forward to RemoteEntityOffline, we handle all discovered entities the same way
	onRemoteEntityOffline(pi, entityID);
}

void CapabilityDelegate::onLocalEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	// Ignore ourself
	if (entity.getEntityID() == _controllerID)
		return;

	// Forward to RemoteEntityUpdated, we handle all discovered entities the same way
	onRemoteEntityUpdated(pi, entity);
}

void CapabilityDelegate::onRemoteEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	auto const entityID = entity.getEntityID();
	{
		// Lock ProtocolInterface
		std::lock_guard<decltype(*pi)> const lg(*pi);

		// Store or replace entity
		{
#ifdef __cpp_lib_unordered_map_try_emplace
			AVDECC_ASSERT(_discoveredEntities.find(entityID) == _discoveredEntities.end(), "CapabilityDelegate::onRemoteEntityOnline: Entity already online");
			_discoveredEntities.insert_or_assign(entityID, DiscoveredEntity{ entity, getMainInterfaceIndex(entity) });
#else // !__cpp_lib_unordered_map_try_emplace
			auto it = _discoveredEntities.find(entityID);
			if (AVDECC_ASSERT_WITH_RET(it == _discoveredEntities.end(), "CapabilityDelegate::onRemoteEntityOnline: Entity already online"))
				_discoveredEntities.insert(std::make_pair(entityID, DiscoveredEntity{ entity, getMainInterfaceIndex(entity) }));
			else
				it->second = DiscoveredEntity{ entity, getMainInterfaceIndex(entity) };
#endif // __cpp_lib_unordered_map_try_emplace
		}
	}

	utils::invokeProtectedMethod(&controller::Delegate::onEntityOnline, _controllerDelegate, &_controllerInterface, entityID, entity);
}

void CapabilityDelegate::onRemoteEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept
{
	{
		// Lock ProtocolInterface
		std::lock_guard<decltype(*pi)> const lg(*pi);

		// Remove entity
		_discoveredEntities.erase(entityID);
	}

	utils::invokeProtectedMethod(&controller::Delegate::onEntityOffline, _controllerDelegate, &_controllerInterface, entityID);
}

void CapabilityDelegate::onRemoteEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	enum class Action
	{
		NotifyUpdate = 0,
		ForwardOnline = 1,
		ForwardOffline = 2,
		ForwardOfflineOnline = 3,
	};

	auto const entityID = entity.getEntityID();
	auto action = Action::NotifyUpdate;
	{
		// Lock ProtocolInterface
		std::lock_guard<decltype(*pi)> const lg(*pi);

		auto discoveredEntityIt = _discoveredEntities.find(entityID);
		if (AVDECC_ASSERT_WITH_RET(discoveredEntityIt != _discoveredEntities.end(), "CapabilityDelegate::onRemoteEntityUpdated: Entity not found"))
		{
			auto& discoveredEntity = discoveredEntityIt->second;

			// Entity still has its "main" interface index, we can proceed with the update
			if (entity.hasInterfaceIndex(discoveredEntity.mainInterfaceIndex))
			{
				discoveredEntity.entity = entity;
			}
			else
			{
				if (AVDECC_ASSERT_WITH_RET(!entity.getInterfacesInformation().empty(), "CapabilityDelegate::onRemoteEntityUpdated called but entity has no valid AvbInterface"))
				{
					LOG_CONTROLLER_ENTITY_INFO(entityID, "Entity 'main' (first discovered) AvbInterface timed out, forcing it offline/online");
					// Fallback to EntityOffline then EntityOnline
					action = Action::ForwardOfflineOnline;
				}
				else
				{
					LOG_CONTROLLER_ENTITY_INFO(entityID, "Entity 'main' (first discovered) AvbInterface timed out but no other interface (should not happen), forcing it offline");
					// Fallback to EntityOffline
					action = Action::ForwardOffline;
				}
			}
		}
		else
		{
			// Fallback to EntityOnline
			action = Action::ForwardOnline;
		}
	}

	// To everything else outside the lock
	switch (action)
	{
		case Action::NotifyUpdate:
			utils::invokeProtectedMethod(&controller::Delegate::onEntityUpdate, _controllerDelegate, &_controllerInterface, entityID, entity);
			break;
		case Action::ForwardOnline:
			onRemoteEntityOnline(pi, entity);
			break;
		case Action::ForwardOffline:
			onRemoteEntityOffline(pi, entityID);
			break;
		case Action::ForwardOfflineOnline:
			onRemoteEntityOffline(pi, entityID);
			onRemoteEntityOnline(pi, entity);
			break;
		default:
			AVDECC_ASSERT(false, "Unhandled Action");
			break;
	}
}

/* **** AECP notifications **** */
bool CapabilityDelegate::onUnhandledAecpCommand(protocol::ProtocolInterface* const pi, protocol::Aecpdu const& aecpdu) noexcept
{
	if (aecpdu.getMessageType() == protocol::AecpMessageType::AemCommand)
	{
		auto const& aem = static_cast<protocol::AemAecpdu const&>(aecpdu);

		if (!AVDECC_ASSERT_WITH_RET(_controllerID != aecpdu.getControllerEntityID(), "Message from self should not pass through this function, or maybe if the same entity has Controller/Talker/Listener capabilities? (in that case allow the message to be processed, the ProtocolInterface will optimize the sending)"))
			return true;

		if (aem.getCommandType() == protocol::AemCommandType::ControllerAvailable)
		{
			// We are being asked if we are available, and we are! Reply that
			LocalEntityImpl<>::sendAemAecpResponse(pi, aem, protocol::AemAecpStatus::Success, nullptr, 0u);
			return true;
		}
	}
	return false;
}

void CapabilityDelegate::onAecpAemUnsolicitedResponse(protocol::ProtocolInterface* const /*pi*/, protocol::AemAecpdu const& aecpdu) noexcept
{
	// Ignore messages not for me
	if (_controllerID != aecpdu.getControllerEntityID())
		return;

	auto const messageType = aecpdu.getMessageType();

	if (messageType == protocol::AecpMessageType::AemResponse)
	{
		auto const& aem = static_cast<protocol::AemAecpdu const&>(aecpdu);
		if (AVDECC_ASSERT_WITH_RET(aem.getUnsolicited(), "Should only be triggered for unsollicited notifications"))
		{
			// Process AEM message without any error or answer callbacks, it's not an expected response
			processAemAecpResponse(&aecpdu, nullptr, {});
			// Statistics
			utils::invokeProtectedMethod(&controller::Delegate::onAemAecpUnsolicitedReceived, _controllerDelegate, &_controllerInterface, aecpdu.getTargetEntityID());
		}
	}
}

void CapabilityDelegate::onAecpAemIdentifyNotification(protocol::ProtocolInterface* const /*pi*/, protocol::AemAecpdu const& aecpdu) noexcept
{
	// Forward the event
	utils::invokeProtectedMethod(&controller::Delegate::onEntityIdentifyNotification, _controllerDelegate, &_controllerInterface, aecpdu.getTargetEntityID());
}

/* **** ACMP notifications **** */
void CapabilityDelegate::onAcmpCommand(protocol::ProtocolInterface* const /*pi*/, protocol::Acmpdu const& /*acmpdu*/) noexcept
{
	// Controllers do not care about ACMP Commands (which can only be sniffed ones)
}

void CapabilityDelegate::onAcmpResponse(protocol::ProtocolInterface* const /*pi*/, protocol::Acmpdu const& acmpdu) noexcept
{
	// Controllers only care about sniffed ACMP Responses here (responses to their commands have already been processed by the ProtocolInterface)

	// Check if it's a response for a Controller (since the communication btw listener and talkers uses our controllerID, we don't want to detect talker's response as ours)
	auto const expectedControllerResponseType = isResponseForController(acmpdu.getMessageType());

	// Only process sniffed responses (ie. Talker response to Listener, or Listener response to another Controller)
	if (_controllerID != acmpdu.getControllerEntityID() || !expectedControllerResponseType)
	{
		processAcmpResponse(&acmpdu, LocalEntityImpl<>::OnACMPErrorCallback(), LocalEntityImpl<>::AnswerCallback(), true);
	}
}

/* ************************************************************************** */
/* Controller notifications                                                   */
/* ************************************************************************** */
/* **** Statistics **** */
void CapabilityDelegate::onAecpRetry(protocol::ProtocolInterface* const /*pi*/, UniqueIdentifier const& entityID) noexcept
{
	// Statistics
	utils::invokeProtectedMethod(&controller::Delegate::onAecpRetry, _controllerDelegate, &_controllerInterface, entityID);
}

void CapabilityDelegate::onAecpTimeout(protocol::ProtocolInterface* const /*pi*/, UniqueIdentifier const& entityID) noexcept
{
	// Statistics
	utils::invokeProtectedMethod(&controller::Delegate::onAecpTimeout, _controllerDelegate, &_controllerInterface, entityID);
}

void CapabilityDelegate::onAecpUnexpectedResponse(protocol::ProtocolInterface* const /*pi*/, UniqueIdentifier const& entityID) noexcept
{
	// Statistics
	utils::invokeProtectedMethod(&controller::Delegate::onAecpUnexpectedResponse, _controllerDelegate, &_controllerInterface, entityID);
}

void CapabilityDelegate::onAecpResponseTime(protocol::ProtocolInterface* const /*pi*/, UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept
{
	// Statistics
	utils::invokeProtectedMethod(&controller::Delegate::onAecpResponseTime, _controllerDelegate, &_controllerInterface, entityID, responseTime);
}

/* ************************************************************************** */
/* Internal methods                                                           */
/* ************************************************************************** */
model::AvbInterfaceIndex CapabilityDelegate::getMainInterfaceIndex(Entity const& entity) const noexcept
{
	// Get the "main" avb interface index (ie. the first in the list)
	return entity.getInterfacesInformation().begin()->first;
}

bool CapabilityDelegate::isResponseForController(protocol::AcmpMessageType const messageType) const noexcept
{
	if (messageType == protocol::AcmpMessageType::ConnectRxResponse || messageType == protocol::AcmpMessageType::DisconnectRxResponse || messageType == protocol::AcmpMessageType::GetRxStateResponse || messageType == protocol::AcmpMessageType::GetTxConnectionResponse)
		return true;
	return false;
}

void CapabilityDelegate::sendAemAecpCommand(UniqueIdentifier const targetEntityID, protocol::AemCommandType const commandType, void const* const payload, size_t const payloadLength, LocalEntityImpl<>::OnAemAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept
{
	auto targetMacAddress = networkInterface::MacAddress{};

	// Search target mac address based on its entityID
	{
		// Lock ProtocolInterface
		std::lock_guard<decltype(*_protocolInterface)> const lg(*_protocolInterface);

		auto const it = _discoveredEntities.find(targetEntityID);

		if (it != _discoveredEntities.end())
		{
			// Get entity mac address
			auto const& discoveredEntity = it->second;
			targetMacAddress = discoveredEntity.entity.getMacAddress(discoveredEntity.mainInterfaceIndex);
		}
	}

	// Return an error if entity is not found in the list
	if (!networkInterface::isMacAddressValid(targetMacAddress))
	{
		utils::invokeProtectedHandler(onErrorCallback, LocalEntity::AemCommandStatus::UnknownEntity);
		return;
	}

	LocalEntityImpl<>::sendAemAecpCommand(_protocolInterface, _controllerID, targetEntityID, targetMacAddress, commandType, payload, payloadLength,
		[this, onErrorCallback, answerCallback](protocol::Aecpdu const* const response, LocalEntity::AemCommandStatus const status)
		{
			if (!!status)
			{
				processAemAecpResponse(response, onErrorCallback, answerCallback); // We sent an AEM command, we know it's an AEM response (so directly call processAemAecpResponse)
			}
			else
			{
				utils::invokeProtectedHandler(onErrorCallback, status);
			}
		});
}

void CapabilityDelegate::sendAaAecpCommand(UniqueIdentifier const targetEntityID, addressAccess::Tlvs const& tlvs, LocalEntityImpl<>::OnAaAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept
{
	auto targetMacAddress = networkInterface::MacAddress{};

	// Search target mac address based on its entityID
	{
		// Lock ProtocolInterface
		std::lock_guard<decltype(*_protocolInterface)> const lg(*_protocolInterface);

		auto const it = _discoveredEntities.find(targetEntityID);

		if (it != _discoveredEntities.end())
		{
			// Get entity mac address
			auto const& discoveredEntity = it->second;
			targetMacAddress = discoveredEntity.entity.getMacAddress(discoveredEntity.mainInterfaceIndex);
		}
	}

	// Return an error if entity is not found in the list
	if (!networkInterface::isMacAddressValid(targetMacAddress))
	{
		utils::invokeProtectedHandler(onErrorCallback, LocalEntity::AaCommandStatus::UnknownEntity);
		return;
	}

	LocalEntityImpl<>::sendAaAecpCommand(_protocolInterface, _controllerID, targetEntityID, targetMacAddress, tlvs,
		[this, onErrorCallback, answerCallback](protocol::Aecpdu const* const response, LocalEntity::AaCommandStatus const status)
		{
			if (!!status)
			{
				processAaAecpResponse(response, onErrorCallback, answerCallback); // We sent an Address Access command, we know it's an Address Access response (so directly call processAaAecpResponse)
			}
			else
			{
				utils::invokeProtectedHandler(onErrorCallback, status);
			}
		});
}

void CapabilityDelegate::sendMvuAecpCommand(UniqueIdentifier const targetEntityID, protocol::MvuCommandType const commandType, void const* const payload, size_t const payloadLength, LocalEntityImpl<>::OnMvuAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept
{
	auto targetMacAddress = networkInterface::MacAddress{};

	// Search target mac address based on its entityID
	{
		// Lock ProtocolInterface
		std::lock_guard<decltype(*_protocolInterface)> const lg(*_protocolInterface);

		auto const it = _discoveredEntities.find(targetEntityID);

		if (it != _discoveredEntities.end())
		{
			// Get entity mac address
			auto const& discoveredEntity = it->second;
			targetMacAddress = discoveredEntity.entity.getMacAddress(discoveredEntity.mainInterfaceIndex);
		}
	}

	// Return an error if entity is not found in the list
	if (!networkInterface::isMacAddressValid(targetMacAddress))
	{
		utils::invokeProtectedHandler(onErrorCallback, LocalEntity::MvuCommandStatus::UnknownEntity);
		return;
	}

	LocalEntityImpl<>::sendMvuAecpCommand(_protocolInterface, _controllerID, targetEntityID, targetMacAddress, commandType, payload, payloadLength,
		[this, onErrorCallback, answerCallback](protocol::Aecpdu const* const response, LocalEntity::MvuCommandStatus const status)
		{
			if (!!status)
			{
				processMvuAecpResponse(response, onErrorCallback, answerCallback); // We sent an MVU command, we know it's an MVU response (so directly call processMvuAecpResponse)
			}
			else
			{
				utils::invokeProtectedHandler(onErrorCallback, status);
			}
		});
}

void CapabilityDelegate::sendAcmpCommand(protocol::AcmpMessageType const messageType, UniqueIdentifier const talkerEntityID, model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, model::StreamIndex const listenerStreamIndex, std::uint16_t const connectionIndex, LocalEntityImpl<>::OnACMPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept
{
	LocalEntityImpl<>::sendAcmpCommand(_protocolInterface, messageType, _controllerID, talkerEntityID, talkerStreamIndex, listenerEntityID, listenerStreamIndex, connectionIndex,
		[this, onErrorCallback, answerCallback](protocol::Acmpdu const* const response, LocalEntity::ControlStatus const status)
		{
			if (!!status)
			{
				processAcmpResponse(response, onErrorCallback, answerCallback, false);
			}
			else
			{
				utils::invokeProtectedHandler(onErrorCallback, status);
			}
		});
}

void CapabilityDelegate::processAemAecpResponse(protocol::Aecpdu const* const response, LocalEntityImpl<>::OnAemAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept
{
	auto const& aem = static_cast<protocol::AemAecpdu const&>(*response);
	auto const status = static_cast<LocalEntity::AemCommandStatus>(aem.getStatus().getValue()); // We have to convert protocol status to our extended status

	static std::unordered_map<protocol::AemCommandType::value_type, std::function<void(controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)>> s_Dispatch
	{
		// Acquire Entity
		{ protocol::AemCommandType::AcquireEntity.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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

				if ((flags & protocol::AemAcquireEntityFlags::Release) == protocol::AemAcquireEntityFlags::Release)
				{
					answerCallback.invoke<controller::Interface::ReleaseEntityHandler>(controllerInterface, targetID, status, ownerID, descriptorType, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onEntityReleased, delegate, controllerInterface, targetID, ownerID, descriptorType, descriptorIndex);
					}
				}
				else
				{
					answerCallback.invoke<controller::Interface::AcquireEntityHandler>(controllerInterface, targetID, status, ownerID, descriptorType, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onEntityAcquired, delegate, controllerInterface, targetID, ownerID, descriptorType, descriptorIndex);
					}
				}
			}
		},
		// Lock Entity
		{ protocol::AemCommandType::LockEntity.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
	// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[flags, lockedID, descriptorType, descriptorIndex] = protocol::aemPayload::deserializeLockEntityResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeLockEntityResponse(aem.getPayload());
				protocol::AemLockEntityFlags const flags = std::get<0>(result);
				UniqueIdentifier const lockedID = std::get<1>(result);
				entity::model::DescriptorType const descriptorType = std::get<2>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<3>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				if ((flags & protocol::AemLockEntityFlags::Unlock) == protocol::AemLockEntityFlags::Unlock)
				{
					answerCallback.invoke<controller::Interface::UnlockEntityHandler>(controllerInterface, targetID, status, lockedID, descriptorType, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onEntityUnlocked, delegate, controllerInterface, targetID, lockedID, descriptorType, descriptorIndex);
					}
				}
				else
				{
					answerCallback.invoke<controller::Interface::LockEntityHandler>(controllerInterface, targetID, status, lockedID, descriptorType, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onEntityLocked, delegate, controllerInterface, targetID, lockedID, descriptorType, descriptorIndex);
					}
				}
			}
		},
		// Entity Available
		{ protocol::AemCommandType::EntityAvailable.getValue(), [](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
				auto const targetID = aem.getTargetEntityID();
				answerCallback.invoke<controller::Interface::QueryEntityAvailableHandler>(controllerInterface, targetID, status);
			}
		},
		// Controller Available
		{ protocol::AemCommandType::ControllerAvailable.getValue(), [](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
				auto const targetID = aem.getTargetEntityID();
				answerCallback.invoke<controller::Interface::QueryControllerAvailableHandler>(controllerInterface, targetID, status);
			}
		},
		// Read Descriptor
		{ protocol::AemCommandType::ReadDescriptor.getValue(), [](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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
						answerCallback.invoke<controller::Interface::EntityDescriptorHandler>(controllerInterface, targetID, status, entityDescriptor);
						break;
					}

					case model::DescriptorType::Configuration:
					{
						// Deserialize configuration descriptor
						auto configurationDescriptor = protocol::aemPayload::deserializeReadConfigurationDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::ConfigurationDescriptorHandler>(controllerInterface, targetID, status, static_cast<model::ConfigurationIndex>(descriptorIndex), configurationDescriptor); // Passing descriptorIndex as ConfigurationIndex here is NOT an error. See 7.4.5.1
						break;
					}

					case model::DescriptorType::AudioUnit:
					{
						// Deserialize audio unit descriptor
						auto audioUnitDescriptor = protocol::aemPayload::deserializeReadAudioUnitDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::AudioUnitDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, audioUnitDescriptor);
						break;
					}

					case model::DescriptorType::StreamInput:
					{
						// Deserialize stream input descriptor
						auto streamDescriptor = protocol::aemPayload::deserializeReadStreamDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::StreamInputDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, streamDescriptor);
						break;
					}

					case model::DescriptorType::StreamOutput:
					{
						// Deserialize stream output descriptor
						auto streamDescriptor = protocol::aemPayload::deserializeReadStreamDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::StreamOutputDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, streamDescriptor);
						break;
					}

					case model::DescriptorType::JackInput:
					{
						// Deserialize jack input descriptor
						auto jackDescriptor = protocol::aemPayload::deserializeReadJackDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::JackInputDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, jackDescriptor);
						break;
					}

					case model::DescriptorType::JackOutput:
					{
						// Deserialize jack output descriptor
						auto jackDescriptor = protocol::aemPayload::deserializeReadJackDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::JackOutputDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, jackDescriptor);
						break;
					}

					case model::DescriptorType::AvbInterface:
					{
						// Deserialize avb interface descriptor
						auto avbInterfaceDescriptor = protocol::aemPayload::deserializeReadAvbInterfaceDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::AvbInterfaceDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, avbInterfaceDescriptor);
						break;
					}

					case model::DescriptorType::ClockSource:
					{
						// Deserialize clock source descriptor
						auto clockSourceDescriptor = protocol::aemPayload::deserializeReadClockSourceDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::ClockSourceDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, clockSourceDescriptor);
						break;
					}

					case model::DescriptorType::MemoryObject:
					{
						// Deserialize memory object descriptor
						auto memoryObjectDescriptor = protocol::aemPayload::deserializeReadMemoryObjectDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::MemoryObjectDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, memoryObjectDescriptor);
						break;
					}

					case model::DescriptorType::Locale:
					{
						// Deserialize locale descriptor
						auto localeDescriptor = protocol::aemPayload::deserializeReadLocaleDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::LocaleDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, localeDescriptor);
						break;
					}

					case model::DescriptorType::Strings:
					{
						// Deserialize strings descriptor
						auto stringsDescriptor = protocol::aemPayload::deserializeReadStringsDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::StringsDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, stringsDescriptor);
						break;
					}

					case model::DescriptorType::StreamPortInput:
					{
						// Deserialize stream port descriptor
						auto streamPortDescriptor = protocol::aemPayload::deserializeReadStreamPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::StreamPortInputDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, streamPortDescriptor);
						break;
					}

					case model::DescriptorType::StreamPortOutput:
					{
						// Deserialize stream port descriptor
						auto streamPortDescriptor = protocol::aemPayload::deserializeReadStreamPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::StreamPortOutputDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, streamPortDescriptor);
						break;
					}

					case model::DescriptorType::ExternalPortInput:
					{
						// Deserialize external port descriptor
						auto externalPortDescriptor = protocol::aemPayload::deserializeReadExternalPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::ExternalPortInputDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, externalPortDescriptor);
						break;
					}

					case model::DescriptorType::ExternalPortOutput:
					{
						// Deserialize external port descriptor
						auto externalPortDescriptor = protocol::aemPayload::deserializeReadExternalPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::ExternalPortOutputDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, externalPortDescriptor);
						break;
					}

					case model::DescriptorType::InternalPortInput:
					{
						// Deserialize internal port descriptor
						auto internalPortDescriptor = protocol::aemPayload::deserializeReadInternalPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::InternalPortInputDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, internalPortDescriptor);
						break;
					}

					case model::DescriptorType::InternalPortOutput:
					{
						// Deserialize internal port descriptor
						auto internalPortDescriptor = protocol::aemPayload::deserializeReadInternalPortDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::InternalPortOutputDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, internalPortDescriptor);
						break;
					}

					case model::DescriptorType::AudioCluster:
					{
						// Deserialize audio cluster descriptor
						auto audioClusterDescriptor = protocol::aemPayload::deserializeReadAudioClusterDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::AudioClusterDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, audioClusterDescriptor);
						break;
					}

					case model::DescriptorType::AudioMap:
					{
						// Deserialize audio map descriptor
						auto audioMapDescriptor = protocol::aemPayload::deserializeReadAudioMapDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::AudioMapDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, audioMapDescriptor);
						break;
					}

					case model::DescriptorType::Control:
					{
						// Deserialize control descriptor
						auto controlDescriptor = protocol::aemPayload::deserializeReadControlDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::ControlDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, controlDescriptor);
						break;
					}

					case model::DescriptorType::ClockDomain:
					{
						// Deserialize clock domain descriptor
						auto clockDomainDescriptor = protocol::aemPayload::deserializeReadClockDomainDescriptorResponse(payload, commonSize, aemStatus);
						// Notify handlers
						answerCallback.invoke<controller::Interface::ClockDomainDescriptorHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, clockDomainDescriptor);
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
		{ protocol::AemCommandType::SetConfiguration.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
	// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[configurationIndex] = protocol::aemPayload::deserializeSetConfigurationResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetConfigurationResponse(aem.getPayload());
				model::ConfigurationIndex const configurationIndex = std::get<0>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::SetConfigurationHandler>(controllerInterface, targetID, status, configurationIndex);
				if (aem.getUnsolicited() && delegate && !!status)
				{
					utils::invokeProtectedMethod(&controller::Delegate::onConfigurationChanged, delegate, controllerInterface, targetID, configurationIndex);
				}
			}
		},
		// Get Configuration
		{ protocol::AemCommandType::GetConfiguration.getValue(), [](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
	// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[configurationIndex] = protocol::aemPayload::deserializeGetConfigurationResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetConfigurationResponse(aem.getPayload());
				model::ConfigurationIndex const configurationIndex = std::get<0>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::GetConfigurationHandler>(controllerInterface, targetID, status, configurationIndex);
			}
		},
		// Set Stream Format
		{ protocol::AemCommandType::SetStreamFormat.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<controller::Interface::SetStreamInputFormatHandler>(controllerInterface, targetID, status, descriptorIndex, streamFormat);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamInputFormatChanged, delegate, controllerInterface, targetID, descriptorIndex, streamFormat);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<controller::Interface::SetStreamOutputFormatHandler>(controllerInterface, targetID, status, descriptorIndex, streamFormat);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamOutputFormatChanged, delegate, controllerInterface, targetID, descriptorIndex, streamFormat);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Get Stream Format
		{ protocol::AemCommandType::GetStreamFormat.getValue(), [](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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
					answerCallback.invoke<controller::Interface::GetStreamInputFormatHandler>(controllerInterface, targetID, status, descriptorIndex, streamFormat);
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<controller::Interface::GetStreamOutputFormatHandler>(controllerInterface, targetID, status, descriptorIndex, streamFormat);
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Set Stream Info
		{ protocol::AemCommandType::SetStreamInfo.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
	// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, streamInfo] = protocol::aemPayload::deserializeSetStreamInfoResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetStreamInfoResponse(aem.getPayload());
				model::DescriptorType const descriptorType = std::get<0>(result);
				model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::StreamInfo const streamInfo = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<controller::Interface::SetStreamInputInfoHandler>(controllerInterface, targetID, status, descriptorIndex, streamInfo);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamInputInfoChanged, delegate, controllerInterface, targetID, descriptorIndex, streamInfo, false);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<controller::Interface::SetStreamOutputInfoHandler>(controllerInterface, targetID, status, descriptorIndex, streamInfo);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamOutputInfoChanged, delegate, controllerInterface, targetID, descriptorIndex, streamInfo, false);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Get Stream Info
		{ protocol::AemCommandType::GetStreamInfo.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<controller::Interface::GetStreamInputInfoHandler>(controllerInterface, targetID, status, descriptorIndex, streamInfo);
					if (aem.getUnsolicited() && delegate && !!status) // Unsolicited triggered by change in the SRP domain (Clause 7.5.2)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamInputInfoChanged, delegate, controllerInterface, targetID, descriptorIndex, streamInfo, true);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<controller::Interface::GetStreamOutputInfoHandler>(controllerInterface, targetID, status, descriptorIndex, streamInfo);
					if (aem.getUnsolicited() && delegate && !!status) // Unsolicited triggered by change in the SRP domain (Clause 7.5.2)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamOutputInfoChanged, delegate, controllerInterface, targetID, descriptorIndex, streamInfo, true);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Set Name
		{ protocol::AemCommandType::SetName.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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
								answerCallback.invoke<controller::Interface::SetEntityNameHandler>(controllerInterface, targetID, status, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&controller::Delegate::onEntityNameChanged, delegate, controllerInterface, targetID, name);
								}
								break;
							case 1: // group_name
								answerCallback.invoke<controller::Interface::SetEntityGroupNameHandler>(controllerInterface, targetID, status, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&controller::Delegate::onEntityGroupNameChanged, delegate, controllerInterface, targetID, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for Entity Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
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
								answerCallback.invoke<controller::Interface::SetConfigurationNameHandler>(controllerInterface, targetID, status, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&controller::Delegate::onConfigurationNameChanged, delegate, controllerInterface, targetID, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for Configuration Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AudioUnit:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::SetAudioUnitNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&controller::Delegate::onAudioUnitNameChanged, delegate, controllerInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for AudioUnit Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::StreamInput:
					{
						switch (nameIndex)
						{
							case 0: // stream_name
								answerCallback.invoke<controller::Interface::SetStreamInputNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&controller::Delegate::onStreamInputNameChanged, delegate, controllerInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for StreamInput Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::StreamOutput:
					{
						switch (nameIndex)
						{
							case 0: // stream_name
								answerCallback.invoke<controller::Interface::SetStreamOutputNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&controller::Delegate::onStreamOutputNameChanged, delegate, controllerInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for StreamOutput Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AvbInterface:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::SetAvbInterfaceNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&controller::Delegate::onAvbInterfaceNameChanged, delegate, controllerInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for AvbInterface Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::ClockSource:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::SetClockSourceNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&controller::Delegate::onClockSourceNameChanged, delegate, controllerInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for ClockSource Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::MemoryObject:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::SetMemoryObjectNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&controller::Delegate::onMemoryObjectNameChanged, delegate, controllerInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for MemoryObject Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AudioCluster:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::SetAudioClusterNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&controller::Delegate::onAudioClusterNameChanged, delegate, controllerInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for AudioCluster Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::Control:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::SetControlNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&controller::Delegate::onControlNameChanged, delegate, controllerInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for Control Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::ClockDomain:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::SetClockDomainNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								if (aem.getUnsolicited() && delegate && !!status)
								{
									utils::invokeProtectedMethod(&controller::Delegate::onClockDomainNameChanged, delegate, controllerInterface, targetID, configurationIndex, descriptorIndex, name);
								}
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in SET_NAME response for ClockDomain Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					default:
						LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled descriptorType in SET_NAME response: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
						break;
				}
			}
		},
		// Get Name
		{ protocol::AemCommandType::GetName.getValue(), [](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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
								answerCallback.invoke<controller::Interface::GetEntityNameHandler>(controllerInterface, targetID, status, name);
								break;
							case 1: // group_name
								answerCallback.invoke<controller::Interface::GetEntityGroupNameHandler>(controllerInterface, targetID, status, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for Entity Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
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
								answerCallback.invoke<controller::Interface::GetConfigurationNameHandler>(controllerInterface, targetID, status, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for Configuration Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AudioUnit:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetAudioUnitNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for AudioUnit Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::StreamInput:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetStreamInputNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for StreamInput Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::StreamOutput:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetStreamOutputNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for StreamOutput Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AvbInterface:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetAvbInterfaceNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for AvbInterface Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::ClockSource:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetClockSourceNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for ClockSource Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::MemoryObject:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetMemoryObjectNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for MemoryObject Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::AudioCluster:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetAudioClusterNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for AudioCluster Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::Control:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetControlNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for Control Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					case model::DescriptorType::ClockDomain:
					{
						switch (nameIndex)
						{
							case 0: // object_name
								answerCallback.invoke<controller::Interface::GetClockDomainNameHandler>(controllerInterface, targetID, status, configurationIndex, descriptorIndex, name);
								break;
							default:
								LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled nameIndex in GET_NAME response for ClockDomain Descriptor: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
								break;
						}
						break;
					}
					default:
						LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled descriptorType in GET_NAME response: DescriptorType={} DescriptorIndex={} NameIndex={} ConfigurationIndex={} Name={}", utils::to_integral(descriptorType), descriptorIndex, nameIndex, configurationIndex, name.str());
						break;
				}
			}
		},
		// Set Sampling Rate
		{ protocol::AemCommandType::SetSamplingRate.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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

				// Notify handlers
				if (descriptorType == model::DescriptorType::AudioUnit)
				{
					answerCallback.invoke<controller::Interface::SetAudioUnitSamplingRateHandler>(controllerInterface, targetID, status, descriptorIndex, samplingRate);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onAudioUnitSamplingRateChanged, delegate, controllerInterface, targetID, descriptorIndex, samplingRate);
					}
				}
				else if (descriptorType == model::DescriptorType::VideoCluster)
				{
					answerCallback.invoke<controller::Interface::SetVideoClusterSamplingRateHandler>(controllerInterface, targetID, status, descriptorIndex, samplingRate);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onVideoClusterSamplingRateChanged, delegate, controllerInterface, targetID, descriptorIndex, samplingRate);
					}
				}
				else if (descriptorType == model::DescriptorType::SensorCluster)
				{
					answerCallback.invoke<controller::Interface::SetSensorClusterSamplingRateHandler>(controllerInterface, targetID, status, descriptorIndex, samplingRate);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onSensorClusterSamplingRateChanged, delegate, controllerInterface, targetID, descriptorIndex , samplingRate);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Get Sampling Rate
		{ protocol::AemCommandType::GetSamplingRate.getValue(), [](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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
					answerCallback.invoke<controller::Interface::GetAudioUnitSamplingRateHandler>(controllerInterface, targetID, status, descriptorIndex, samplingRate);
				}
				else if (descriptorType == model::DescriptorType::VideoCluster)
				{
					answerCallback.invoke<controller::Interface::GetVideoClusterSamplingRateHandler>(controllerInterface, targetID, status, descriptorIndex, samplingRate);
				}
				else if (descriptorType == model::DescriptorType::SensorCluster)
				{
					answerCallback.invoke<controller::Interface::GetSensorClusterSamplingRateHandler>(controllerInterface, targetID, status, descriptorIndex, samplingRate);
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Set Clock Source
		{ protocol::AemCommandType::SetClockSource.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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

				// Notify handlers
				answerCallback.invoke<controller::Interface::SetClockSourceHandler>(controllerInterface, targetID, status, descriptorIndex, clockSourceIndex);
				if (aem.getUnsolicited() && delegate && !!status)
				{
					utils::invokeProtectedMethod(&controller::Delegate::onClockSourceChanged, delegate, controllerInterface, targetID, descriptorIndex, clockSourceIndex);
				}
			}
		},
		// Get Clock Source
		{ protocol::AemCommandType::GetClockSource.getValue(),[](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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
				answerCallback.invoke<controller::Interface::GetClockSourceHandler>(controllerInterface, targetID, status, descriptorIndex, clockSourceIndex);
			}
		},
		// Set Control
		{ protocol::AemCommandType::SetControl.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
	// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const [descriptorType, descriptorIndex, packedControlValues] = protocol::aemPayload::deserializeSetControlResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeSetControlResponse(aem.getPayload());
				//entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				MemoryBuffer const& packedControlValues = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::SetControlValuesHandler>(controllerInterface, targetID, status, descriptorIndex, packedControlValues);
				if (aem.getUnsolicited() && delegate && !!status)
				{
					utils::invokeProtectedMethod(&controller::Delegate::onControlValuesChanged, delegate, controllerInterface, targetID, descriptorIndex, packedControlValues);
				}
			}
		},
		// Get Control
		{ protocol::AemCommandType::GetControl.getValue(),[](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
	// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const [descriptorType, descriptorIndex, controlValues] = protocol::aemPayload::deserializeGetControlResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetControlResponse(aem.getPayload());
				//entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::ControlValues const controlValues = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::GetControlValuesHandler>(controllerInterface, targetID, status, descriptorIndex, controlValues);
			}
		},
		// Start Streaming
		{ protocol::AemCommandType::StartStreaming.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<controller::Interface::StartStreamInputHandler>(controllerInterface, targetID, status, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamInputStarted, delegate, controllerInterface, targetID, descriptorIndex);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<controller::Interface::StartStreamOutputHandler>(controllerInterface, targetID, status, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamOutputStarted, delegate, controllerInterface, targetID, descriptorIndex);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Stop Streaming
		{ protocol::AemCommandType::StopStreaming.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamInput)
				{
					answerCallback.invoke<controller::Interface::StopStreamInputHandler>(controllerInterface, targetID, status, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamInputStopped, delegate, controllerInterface, targetID, descriptorIndex);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamOutput)
				{
					answerCallback.invoke<controller::Interface::StopStreamOutputHandler>(controllerInterface, targetID, status, descriptorIndex);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamOutputStopped, delegate, controllerInterface, targetID, descriptorIndex);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Register Unsolicited Notifications
		{ protocol::AemCommandType::RegisterUnsolicitedNotification.getValue(), [](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
				// Ignore payload size and content, Apple's implementation is bugged and returns too much data
				auto const targetID = aem.getTargetEntityID();
				answerCallback.invoke<controller::Interface::RegisterUnsolicitedNotificationsHandler>(controllerInterface, targetID, status);
			}
		},
		// Unregister Unsolicited Notifications
		{ protocol::AemCommandType::DeregisterUnsolicitedNotification.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
				// Ignore payload size and content, Apple's implementation is bugged and returns too much data
				auto const targetID = aem.getTargetEntityID();

				answerCallback.invoke<controller::Interface::UnregisterUnsolicitedNotificationsHandler>(controllerInterface, targetID, status);
				if (aem.getUnsolicited() && delegate && !!status)
				{
					utils::invokeProtectedMethod(&controller::Delegate::onDeregisteredFromUnsolicitedNotifications, delegate, controllerInterface, targetID);
				}
			}
		},
		// GetAvbInfo
		{ protocol::AemCommandType::GetAvbInfo.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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

				// Notify handlers
				if (descriptorType == model::DescriptorType::AvbInterface)
				{
					answerCallback.invoke<controller::Interface::GetAvbInfoHandler>(controllerInterface, targetID, status, descriptorIndex, avbInfo);
					if (aem.getUnsolicited() && delegate && !!status) // Unsolicited triggered by change in the SRP domain (Clause 7.5.2)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onAvbInfoChanged, delegate, controllerInterface, targetID, descriptorIndex, avbInfo);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// GetAsPath
		{ protocol::AemCommandType::GetAsPath.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
	// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorIndex, asPath] = protocol::aemPayload::deserializeGetAsPathResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetAsPathResponse(aem.getPayload());
				entity::model::DescriptorIndex const descriptorIndex = std::get<0>(result);
				entity::model::AsPath const asPath = std::get<1>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::GetAsPathHandler>(controllerInterface, targetID, status, descriptorIndex, asPath);
				if (aem.getUnsolicited() && delegate && !!status) // Unsolicited triggered by change in the SRP domain (Clause 7.5.2)
				{
					utils::invokeProtectedMethod(&controller::Delegate::onAsPathChanged, delegate, controllerInterface, targetID, descriptorIndex, asPath);
				}
			}
		},
		// GetCounters
		{ protocol::AemCommandType::GetCounters.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
	// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, validFlags, counters] = protocol::aemPayload::deserializeGetCountersResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeGetCountersResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::DescriptorCounterValidFlag const validFlags = std::get<2>(result);
				entity::model::DescriptorCounters const& counters = std::get<3>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				switch (descriptorType)
				{
					case model::DescriptorType::Entity:
					{
						EntityCounterValidFlags flags;
						flags.assign(validFlags);
						answerCallback.invoke<controller::Interface::GetEntityCountersHandler>(controllerInterface, targetID, status, flags, counters);
						if (aem.getUnsolicited() && delegate && !!status)
						{
							utils::invokeProtectedMethod(&controller::Delegate::onEntityCountersChanged, delegate, controllerInterface, targetID, flags, counters);
						}
						if (descriptorIndex != 0)
						{
							LOG_CONTROLLER_ENTITY_WARN(targetID, "GET_COUNTERS response for ENTITY descriptor uses a non-0 DescriptorIndex: {}", descriptorIndex);
						}
						break;
					}
					case model::DescriptorType::AvbInterface:
					{
						AvbInterfaceCounterValidFlags flags;
						flags.assign(validFlags);
						answerCallback.invoke<controller::Interface::GetAvbInterfaceCountersHandler>(controllerInterface, targetID, status, descriptorIndex, flags, counters);
						if (aem.getUnsolicited() && delegate && !!status)
						{
							utils::invokeProtectedMethod(&controller::Delegate::onAvbInterfaceCountersChanged, delegate, controllerInterface, targetID, descriptorIndex, flags, counters);
						}
						break;
					}
					case model::DescriptorType::ClockDomain:
					{
						ClockDomainCounterValidFlags flags;
						flags.assign(validFlags);
						answerCallback.invoke<controller::Interface::GetClockDomainCountersHandler>(controllerInterface, targetID, status, descriptorIndex, flags, counters);
						if (aem.getUnsolicited() && delegate && !!status)
						{
							utils::invokeProtectedMethod(&controller::Delegate::onClockDomainCountersChanged, delegate, controllerInterface, targetID, descriptorIndex, flags, counters);
						}
						break;
					}
					case model::DescriptorType::StreamInput:
					{
						StreamInputCounterValidFlags flags;
						flags.assign(validFlags);
						answerCallback.invoke<controller::Interface::GetStreamInputCountersHandler>(controllerInterface, targetID, status, descriptorIndex, flags, counters);
						if (aem.getUnsolicited() && delegate && !!status)
						{
							utils::invokeProtectedMethod(&controller::Delegate::onStreamInputCountersChanged, delegate, controllerInterface, targetID, descriptorIndex, flags, counters);
						}
						break;
					}
					case model::DescriptorType::StreamOutput:
					{
						StreamOutputCounterValidFlags flags;
						flags.assign(validFlags);
						answerCallback.invoke<controller::Interface::GetStreamOutputCountersHandler>(controllerInterface, targetID, status, descriptorIndex, flags, counters);
						if (aem.getUnsolicited() && delegate && !!status)
						{
							utils::invokeProtectedMethod(&controller::Delegate::onStreamOutputCountersChanged, delegate, controllerInterface, targetID, descriptorIndex, flags, counters);
						}
						break;
					}
					default:
						LOG_CONTROLLER_ENTITY_DEBUG(targetID, "Unhandled descriptorType in GET_COUNTERS response: DescriptorType={} DescriptorIndex={}", utils::to_integral(descriptorType), descriptorIndex);
						break;
				}
			}
		},
		// Get Audio Map
		{ protocol::AemCommandType::GetAudioMap.getValue(), []([[maybe_unused]] controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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

				// Notify handlers
				if (descriptorType == model::DescriptorType::StreamPortInput)
				{
					answerCallback.invoke<controller::Interface::GetStreamPortInputAudioMapHandler>(controllerInterface, targetID, status, descriptorIndex, numberOfMaps, mapIndex, mappings);
#ifdef ALLOW_GET_AUDIO_MAP_UNSOL
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamPortInputAudioMappingsChanged, delegate, controllerInterface, targetID, descriptorIndex, numberOfMaps, mapIndex, mappings);
					}
#endif // ALLOW_GET_AUDIO_MAP_UNSOL
				}
				else if (descriptorType == model::DescriptorType::StreamPortOutput)
				{
					answerCallback.invoke<controller::Interface::GetStreamPortOutputAudioMapHandler>(controllerInterface, targetID, status, descriptorIndex, numberOfMaps, mapIndex, mappings);
#ifdef ALLOW_GET_AUDIO_MAP_UNSOL
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamPortOutputAudioMappingsChanged, delegate, controllerInterface, targetID, descriptorIndex, numberOfMaps, mapIndex, mappings);
					}
#endif
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Add Audio Mappings
		{ protocol::AemCommandType::AddAudioMappings.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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
				if (descriptorType == model::DescriptorType::StreamPortInput)
				{
					answerCallback.invoke<controller::Interface::AddStreamPortInputAudioMappingsHandler>(controllerInterface, targetID, status, descriptorIndex, mappings);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamPortInputAudioMappingsAdded, delegate, controllerInterface, targetID, descriptorIndex, mappings);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamPortOutput)
				{
					answerCallback.invoke<controller::Interface::AddStreamPortOutputAudioMappingsHandler>(controllerInterface, targetID, status, descriptorIndex, mappings);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamPortOutputAudioMappingsAdded, delegate, controllerInterface, targetID, descriptorIndex, mappings);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Remove Audio Mappings
		{ protocol::AemCommandType::RemoveAudioMappings.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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
				if (descriptorType == model::DescriptorType::StreamPortInput)
				{
					answerCallback.invoke<controller::Interface::RemoveStreamPortInputAudioMappingsHandler>(controllerInterface, targetID, status, descriptorIndex, mappings);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamPortInputAudioMappingsRemoved, delegate, controllerInterface, targetID, descriptorIndex, mappings);
					}
				}
				else if (descriptorType == model::DescriptorType::StreamPortOutput)
				{
					answerCallback.invoke<controller::Interface::RemoveStreamPortOutputAudioMappingsHandler>(controllerInterface, targetID, status, descriptorIndex, mappings);
					if (aem.getUnsolicited() && delegate && !!status)
					{
						utils::invokeProtectedMethod(&controller::Delegate::onStreamPortOutputAudioMappingsRemoved, delegate, controllerInterface, targetID, descriptorIndex, mappings);
					}
				}
				else
					throw InvalidDescriptorTypeException();
			}
		},
		// Start Operation
		{ protocol::AemCommandType::StartOperation.getValue(), [](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
	// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, operationID, operationType, memoryBuffer] = protocol::aemPayload::deserializeStartOperationResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeStartOperationResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::OperationID const operationID = std::get<2>(result);
				entity::model::MemoryObjectOperationType const operationType = std::get<3>(result);
				MemoryBuffer const memoryBuffer = std::get<4>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::StartOperationHandler>(controllerInterface, targetID, status, descriptorType, descriptorIndex, operationID, operationType, memoryBuffer);
			}
		},
		// Abort Operation
		{ protocol::AemCommandType::AbortOperation.getValue(), [](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
	// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, operationID] = protocol::aemPayload::deserializeAbortOperationResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeAbortOperationResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::OperationID const operationID = std::get<2>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				answerCallback.invoke<controller::Interface::AbortOperationHandler>(controllerInterface, targetID, status, descriptorType, descriptorIndex, operationID);
			}
		},
		// Operation Status
		{ protocol::AemCommandType::OperationStatus.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const /*status*/, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& /*answerCallback*/)
			{
	// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const[descriptorType, descriptorIndex, operationID, percentComplete] = protocol::aemPayload::deserializeOperationStatusResponse(aem.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::aemPayload::deserializeOperationStatusResponse(aem.getPayload());
				entity::model::DescriptorType const descriptorType = std::get<0>(result);
				entity::model::DescriptorIndex const descriptorIndex = std::get<1>(result);
				entity::model::OperationID const operationID = std::get<2>(result);
				std::uint16_t const percentComplete = std::get<3>(result);
#endif // __cpp_structured_bindings

				auto const targetID = aem.getTargetEntityID();

				// Notify handlers
				AVDECC_ASSERT(aem.getUnsolicited(), "OperationStatus can only be an unsolicited response");
				utils::invokeProtectedMethod(&controller::Delegate::onOperationStatus, delegate, controllerInterface, targetID, descriptorType, descriptorIndex, operationID, percentComplete);
			}
		},
		// Set Memory Object Length
		{ protocol::AemCommandType::SetMemoryObjectLength.getValue(), [](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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

				// Notify handlers
				answerCallback.invoke<controller::Interface::SetMemoryObjectLengthHandler>(controllerInterface, targetID, status, configurationIndex, memoryObjectIndex, length);
				if (aem.getUnsolicited() && delegate && !!status)
				{
					utils::invokeProtectedMethod(&controller::Delegate::onMemoryObjectLengthChanged, delegate, controllerInterface, targetID, configurationIndex, memoryObjectIndex, length);
				}
			}
		},
		// Get Memory Object Length
		{ protocol::AemCommandType::GetMemoryObjectLength.getValue(),[](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::AemCommandStatus const status, protocol::AemAecpdu const& aem, LocalEntityImpl<>::AnswerCallback const& answerCallback)
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
				answerCallback.invoke<controller::Interface::GetMemoryObjectLengthHandler>(controllerInterface, targetID, status, configurationIndex, memoryObjectIndex, length);
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
			LOG_CONTROLLER_ENTITY_DEBUG(aem.getTargetEntityID(), "Unsolicited AEM response {} not handled ({})", std::string(aem.getCommandType()), utils::toHexString(aem.getCommandType().getValue()));
		}
		// But if it's an expected response, this is an internal error since we sent a command and didn't implement the code to handle the response
		else
		{
			LOG_CONTROLLER_ENTITY_ERROR(aem.getTargetEntityID(), "Failed to process AEM response: Unhandled command type {} ({})", std::string(aem.getCommandType()), utils::toHexString(aem.getCommandType().getValue()));
			utils::invokeProtectedHandler(onErrorCallback, LocalEntity::AemCommandStatus::InternalError);
		}
	}
	else
	{
		auto checkProcessInvalidNonSuccessResponse = [status, &aem, &onErrorCallback]([[maybe_unused]] char const* const what)
		{
			auto st = LocalEntity::AemCommandStatus::ProtocolError;
#if defined(IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES)
			if (status != LocalEntity::AemCommandStatus::Success)
			{
				// Allow this packet to go through as a non-success response, but some fields might have the default initial value which might not be valid (the spec says even in a response message, some fields have a meaningful value)
				st = status;
				LOG_CONTROLLER_ENTITY_INFO(aem.getTargetEntityID(), "Received an invalid non-success {} AEM response ({}) from {} but still processing it because of compilation option IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES", std::string(aem.getCommandType()), what, utils::toHexString(aem.getTargetEntityID(), true));
			}
#endif // IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES
			if (st == LocalEntity::AemCommandStatus::ProtocolError)
			{
				LOG_CONTROLLER_ENTITY_ERROR(aem.getTargetEntityID(), "Failed to process {} AEM response: {}", std::string(aem.getCommandType()), what);
			}
			utils::invokeProtectedHandler(onErrorCallback, st);
		};

		try
		{
			it->second(_controllerDelegate, &_controllerInterface, status, aem, answerCallback);
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
		catch ([[maybe_unused]] protocol::aemPayload::UnsupportedValueException const& e)
		{
			LOG_CONTROLLER_ENTITY_ERROR(aem.getTargetEntityID(), "Failed to process {} AEM response: {}", std::string(aem.getCommandType()), e.what());
			utils::invokeProtectedHandler(onErrorCallback, LocalEntity::AemCommandStatus::ProtocolError);
			return;
		}
		catch ([[maybe_unused]] std::exception const& e) // Mainly unpacking errors
		{
			LOG_CONTROLLER_ENTITY_ERROR(aem.getTargetEntityID(), "Failed to process {} AEM response: {}", std::string(aem.getCommandType()), e.what());
			utils::invokeProtectedHandler(onErrorCallback, LocalEntity::AemCommandStatus::ProtocolError);
			return;
		}
	}
}

void CapabilityDelegate::processAaAecpResponse(protocol::Aecpdu const* const response, LocalEntityImpl<>::OnAaAECPErrorCallback const& /*onErrorCallback*/, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept
{
	auto const& aa = static_cast<protocol::AaAecpdu const&>(*response);
	auto const status = static_cast<LocalEntity::AaCommandStatus>(aa.getStatus().getValue()); // We have to convert protocol status to our extended status
	auto const targetID = aa.getTargetEntityID();

	answerCallback.invoke<controller::Interface::AddressAccessHandler>(&_controllerInterface, targetID, status, aa.getTlvData());
}

void CapabilityDelegate::processMvuAecpResponse(protocol::Aecpdu const* const response, LocalEntityImpl<>::OnMvuAECPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback) const noexcept
{
	auto const& mvu = static_cast<protocol::MvuAecpdu const&>(*response);
	auto const status = static_cast<LocalEntity::MvuCommandStatus>(mvu.getStatus().getValue()); // We have to convert protocol status to our extended status

	static std::unordered_map<protocol::MvuCommandType::value_type, std::function<void(controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::MvuCommandStatus const status, protocol::MvuAecpdu const& mvu, LocalEntityImpl<>::AnswerCallback const& answerCallback)>> s_Dispatch{
		// Get Milan Info
		{ protocol::MvuCommandType::GetMilanInfo.getValue(),
			[](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::MvuCommandStatus const status, protocol::MvuAecpdu const& mvu, LocalEntityImpl<>::AnswerCallback const& answerCallback)
			{
	// Deserialize payload
#ifdef __cpp_structured_bindings
				auto const [milanInfo] = protocol::mvuPayload::deserializeGetMilanInfoResponse(mvu.getPayload());
#else // !__cpp_structured_bindings
				auto const result = protocol::mvuPayload::deserializeGetMilanInfoResponse(mvu.getPayload());
				entity::model::MilanInfo const milanInfo = std::get<0>(result);
#endif // __cpp_structured_bindings

				auto const targetID = mvu.getTargetEntityID();

				answerCallback.invoke<controller::Interface::GetMilanInfoHandler>(controllerInterface, targetID, status, milanInfo);
			} },
	};

	auto const& it = s_Dispatch.find(mvu.getCommandType().getValue());
	if (it == s_Dispatch.end())
	{
		// It's an expected response, this is an internal error since we sent a command and didn't implement the code to handle the response
		LOG_CONTROLLER_ENTITY_ERROR(mvu.getTargetEntityID(), "Failed to process MVU response: Unhandled command type {} ({})", std::string(mvu.getCommandType()), utils::toHexString(mvu.getCommandType().getValue()));
		utils::invokeProtectedHandler(onErrorCallback, LocalEntity::MvuCommandStatus::InternalError);
	}
	else
	{
		try
		{
			it->second(_controllerDelegate, &_controllerInterface, status, mvu, answerCallback);
		}
		catch ([[maybe_unused]] protocol::mvuPayload::IncorrectPayloadSizeException const& e)
		{
			LOG_CONTROLLER_ENTITY_ERROR(mvu.getTargetEntityID(), "Failed to process {} MVU response: {}", std::string(mvu.getCommandType()), e.what());
			utils::invokeProtectedHandler(onErrorCallback, LocalEntity::MvuCommandStatus::ProtocolError);
			return;
		}
		catch ([[maybe_unused]] InvalidDescriptorTypeException const& e)
		{
			LOG_CONTROLLER_ENTITY_ERROR(mvu.getTargetEntityID(), "Failed to process {} MVU response: {}", std::string(mvu.getCommandType()), e.what());
			utils::invokeProtectedHandler(onErrorCallback, LocalEntity::MvuCommandStatus::ProtocolError);
			return;
		}
		catch ([[maybe_unused]] std::exception const& e) // Mainly unpacking errors
		{
			LOG_CONTROLLER_ENTITY_ERROR(mvu.getTargetEntityID(), "Failed to process {} MVU response: {}", std::string(mvu.getCommandType()), e.what());
			utils::invokeProtectedHandler(onErrorCallback, LocalEntity::MvuCommandStatus::ProtocolError);
			return;
		}
	}
}

void CapabilityDelegate::processAcmpResponse(protocol::Acmpdu const* const response, LocalEntityImpl<>::OnACMPErrorCallback const& onErrorCallback, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed) const noexcept
{
	auto const& acmp = static_cast<protocol::Acmpdu const&>(*response);
	auto const status = static_cast<LocalEntity::ControlStatus>(acmp.getStatus().getValue()); // We have to convert protocol status to our extended status

	static std::unordered_map<protocol::AcmpMessageType::value_type, std::function<void(controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed)>> s_Dispatch{
		// Connect TX response
		{ protocol::AcmpMessageType::ConnectTxResponse.getValue(),
			[](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& /*answerCallback*/, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				if (sniffed && delegate)
				{
					utils::invokeProtectedMethod(&controller::Delegate::onListenerConnectResponseSniffed, delegate, controllerInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			} },
		// Disconnect TX response
		{ protocol::AcmpMessageType::DisconnectTxResponse.getValue(),
			[](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<controller::Interface::DisconnectTalkerStreamHandler>(controllerInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				if (sniffed && delegate)
				{
					utils::invokeProtectedMethod(&controller::Delegate::onListenerDisconnectResponseSniffed, delegate, controllerInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			} },
		// Get TX state response
		{ protocol::AcmpMessageType::GetTxStateResponse.getValue(),
			[](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<controller::Interface::GetTalkerStreamStateHandler>(controllerInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				if (sniffed && delegate)
				{
					utils::invokeProtectedMethod(&controller::Delegate::onGetTalkerStreamStateResponseSniffed, delegate, controllerInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			} },
		// Connect RX response
		{ protocol::AcmpMessageType::ConnectRxResponse.getValue(),
			[](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<controller::Interface::ConnectStreamHandler>(controllerInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				if (sniffed && delegate)
				{
					utils::invokeProtectedMethod(&controller::Delegate::onControllerConnectResponseSniffed, delegate, controllerInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			} },
		// Disconnect RX response
		{ protocol::AcmpMessageType::DisconnectRxResponse.getValue(),
			[](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<controller::Interface::DisconnectStreamHandler>(controllerInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				if (sniffed && delegate)
				{
					utils::invokeProtectedMethod(&controller::Delegate::onControllerDisconnectResponseSniffed, delegate, controllerInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			} },
		// Get RX state response
		{ protocol::AcmpMessageType::GetRxStateResponse.getValue(),
			[](controller::Delegate* const delegate, Interface const* const controllerInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const sniffed)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<controller::Interface::GetListenerStreamStateHandler>(controllerInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				if (sniffed && delegate)
				{
					utils::invokeProtectedMethod(&controller::Delegate::onGetListenerStreamStateResponseSniffed, delegate, controllerInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
				}
			} },
		// Get TX connection response
		{ protocol::AcmpMessageType::GetTxConnectionResponse.getValue(),
			[](controller::Delegate* const /*delegate*/, Interface const* const controllerInterface, LocalEntity::ControlStatus const status, protocol::Acmpdu const& acmp, LocalEntityImpl<>::AnswerCallback const& answerCallback, bool const /*sniffed*/)
			{
				auto const talkerEntityID = acmp.getTalkerEntityID();
				auto const talkerStreamIndex = acmp.getTalkerUniqueID();
				auto const listenerEntityID = acmp.getListenerEntityID();
				auto const listenerStreamIndex = acmp.getListenerUniqueID();
				auto const connectionCount = acmp.getConnectionCount();
				auto const flags = acmp.getFlags();
				answerCallback.invoke<controller::Interface::GetTalkerStreamConnectionHandler>(controllerInterface, model::StreamIdentification{ talkerEntityID, talkerStreamIndex }, model::StreamIdentification{ listenerEntityID, listenerStreamIndex }, connectionCount, flags, status);
			} },
	};

	auto const& it = s_Dispatch.find(acmp.getMessageType().getValue());
	if (it == s_Dispatch.end())
	{
		// If this is a sniffed message, simply log we do not handle the message
		if (sniffed)
		{
			LOG_CONTROLLER_ENTITY_DEBUG(acmp.getTalkerEntityID(), "ACMP response {} not handled ({})", std::string(acmp.getMessageType()), utils::toHexString(acmp.getMessageType().getValue()));
		}
		// But if it's an expected response, this is an internal error since we sent a command and didn't implement the code to handle the response
		else
		{
			LOG_CONTROLLER_ENTITY_ERROR(acmp.getTalkerEntityID(), "Failed to process ACMP response: Unhandled message type {} ({})", std::string(acmp.getMessageType()), utils::toHexString(acmp.getMessageType().getValue()));
			utils::invokeProtectedHandler(onErrorCallback, LocalEntity::ControlStatus::InternalError);
		}
	}
	else
	{
		try
		{
			it->second(_controllerDelegate, &_controllerInterface, status, acmp, answerCallback, sniffed);
		}
		catch ([[maybe_unused]] std::exception const& e) // Mainly unpacking errors
		{
			LOG_CONTROLLER_ENTITY_ERROR(acmp.getTalkerEntityID(), "Failed to process ACMP response: {}", e.what());
			utils::invokeProtectedHandler(onErrorCallback, LocalEntity::ControlStatus::ProtocolError);
			return;
		}
	}
}

} // namespace controller
} // namespace entity
} // namespace avdecc
} // namespace la
