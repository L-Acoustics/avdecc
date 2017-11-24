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
* @file avdeccController.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/controller/avdeccController.hpp"
#include "la/avdecc/logger.hpp"
#include "avdeccControlledEntityImpl.hpp"
#include "config.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <cassert>

namespace la
{
namespace avdecc
{
namespace controller
{

bool LA_AVDECC_CONTROLLER_CALL_CONVENTION isCompatibleWithInterfaceVersion(std::uint32_t const interfaceVersion) noexcept
{
	/* Here you have to choose a compatibility mode
	*  1/ Strict mode
	*     The version of the library used to compile must be strictly equivalent to the one used at runtime.
	*  2/ Backward compatibility mode
	*     A newer version of the library used at runtime is backward compatible with an older one used to compile.
	*     When using that mode, each class must use a virtual interface, and each new version must derivate from the interface to propose new methods.
	*  3/ A combination of the 2 above, using code to determine which one depending on the passed interfaceVersion value.
	*/

	// Interface versions should be strictly equivalent
	return InterfaceVersion == interfaceVersion;
}

std::string LA_AVDECC_CONTROLLER_CALL_CONVENTION getVersion() noexcept
{
	return std::string(LA_AVDECC_CONTROLLER_LIB_VERSION);
}

std::uint32_t LA_AVDECC_CONTROLLER_CALL_CONVENTION getInterfaceVersion() noexcept
{
	return InterfaceVersion;
}


/* ************************************************************************** */
/* ControllerImpl class declaration                                           */
/* ************************************************************************** */
class ControllerImpl final : public Controller, private entity::ControllerEntity::Delegate
{
public:
	ControllerImpl(EndStation::ProtocolInterfaceType const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, entity::model::VendorEntityModel const vendorEntityModelID, std::string const& preferedLocale);
	virtual ~ControllerImpl() override;
	virtual void destroy() noexcept override;

	/* Controller configuration */
	virtual void enableEntityAdvertising(std::uint32_t const availableDuration) override;
	virtual void disableEntityAdvertising() noexcept override;

	/* Enumeration and Control Protocol (AECP) */
	virtual void acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, AcquireEntityHandler const& handler) const noexcept override;
	virtual void releaseEntity(UniqueIdentifier const targetEntityID, ReleaseEntityHandler const& handler) const noexcept override;
	virtual void setConfiguration(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept override;
	virtual void setStreamInputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept override;
	virtual void setStreamOutputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept override;
	virtual void addStreamInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, std::vector<entity::model::AudioMapping> const& mappings, AddStreamInputAudioMappingsHandler const& handler) const noexcept override;
	virtual void addStreamOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings, AddStreamOutputAudioMappingsHandler const& handler) const noexcept override;
	virtual void removeStreamInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, std::vector<entity::model::AudioMapping> const& mappings, RemoveStreamInputAudioMappingsHandler const& handler) const noexcept override;
	virtual void removeStreamOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings, RemoveStreamOutputAudioMappingsHandler const& handler) const noexcept override;
	virtual void startStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept override;
	virtual void stopStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept override;
	virtual void startStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept override;
	virtual void stopStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept override;
	virtual void setEntityName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityNameHandler const& handler) const noexcept override;
	virtual void setEntityGroupName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityGroupNameHandler const& handler) const noexcept override;
	virtual void setConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name, SetConfigurationNameHandler const& handler) const noexcept override;
	virtual void setStreamInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamInputNameHandler const& handler) const noexcept override;
	virtual void setStreamOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamOutputNameHandler const& handler) const noexcept override;

	/* Connection Management Protocol (ACMP) */
	virtual void connectStream(UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, ConnectStreamHandler const& handler) const noexcept override;
	virtual void disconnectStream(UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, DisconnectStreamHandler const& handler) const noexcept override;
	virtual void getListenerStreamState(UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, GetListenerStreamStateHandler const& handler) const noexcept override;

	virtual void runMethodForEntity(UniqueIdentifier const entityID, RunMethodForEntityHandler const& handler) const override;

private:
	void checkAdvertiseEntity(ControlledEntityImpl* const entity) const noexcept;
	void handleStreamStateNotification(ControlledEntityImpl* const listenerEntity, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIndex const listenerStreamIndex, bool const isConnected, entity::ConnectionFlags const flags, bool const isSniffed) const noexcept;
	/* Enumeration and Control Protocol (AECP) handlers */
	void onEntityDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::EntityDescriptor const& descriptor) noexcept;
	void onConfigurationDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ConfigurationDescriptor const& descriptor) noexcept;
	void onStreamInputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept;
	void onStreamOutputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept;
	void onLocaleDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex, entity::model::LocaleDescriptor const& descriptor) noexcept;
	void onStringsDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex, entity::model::StringsDescriptor const& descriptor, entity::model::StringsIndex const baseStringDescriptorIndex) noexcept;
	void onGetStreamInputAudioMapResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept;
	void onGetStreamOutputAudioMapResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept;
	/* Connection Management Protocol (ACMP) handlers */
	void onConnectStreamResult(entity::ControllerEntity const* const controller, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept;
	void onDisconnectStreamResult(entity::ControllerEntity const* const controller, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept;
	void onGetListenerStreamStateResult(entity::ControllerEntity const* const controller, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept;

	// entity::ControllerEntity::Delegate overrides
	/* Global notifications */
	virtual void onTransportError(entity::ControllerEntity const* const /*controller*/) noexcept override;
	/* Discovery Protocol (ADP) delegate */
	virtual void onEntityOnline(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*entityID*/, entity::Entity const& /*entity*/) noexcept override;
	virtual void onEntityUpdate(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*entityID*/, entity::Entity const& /*entity*/) noexcept override; // When GpgpGrandMasterID, GpgpDomainNumber or EntityCapabilities changed
	virtual void onEntityOffline(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*entityID*/) noexcept override;
	/* Connection Management Protocol sniffed messages (ACMP) */
	virtual void onConnectStreamSniffed(entity::ControllerEntity const* const controller, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	virtual void onFastConnectStreamSniffed(entity::ControllerEntity const* const controller, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	virtual void onDisconnectStreamSniffed(entity::ControllerEntity const* const controller, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	virtual void onGetListenerStreamStateSniffed(entity::ControllerEntity const* const controller, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept override;
	/* Unsolicited notifications (not triggered for our own commands, the command's 'result' method will be called in that case) and only if command has no error */
	virtual void onEntityAcquired(entity::ControllerEntity const* const controller, UniqueIdentifier const acquiredEntity, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept override;
	virtual void onEntityReleased(entity::ControllerEntity const* const controller, UniqueIdentifier const releasedEntity, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept override;
	virtual void onConfigurationChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex) noexcept override;
	virtual void onStreamInputFormatChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept override;
	virtual void onStreamOutputFormatChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept override;
	virtual void onStreamInputAudioMappingsChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept override;
	virtual void onStreamOutputAudioMappingsChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept override;
	virtual void onStreamInputInfoChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) noexcept override;
	virtual void onStreamOutputInfoChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) noexcept override;
	virtual void onEntityNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityName) noexcept override;
	virtual void onEntityGroupNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityGroupName) noexcept override;
	virtual void onConfigurationNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) noexcept override;
	virtual void onStreamInputNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept override;
	virtual void onStreamOutputNameChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept override;
	virtual void onStreamInputStarted(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamOutputStarted(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamInputStopped(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamOutputStopped(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;

	// Private methods used to update AEM and notify observers
	void updateAcquiredState(ControlledEntityImpl& entity, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, bool const undefined = false) const;
	void updateConfiguration(entity::ControllerEntity const* const controller, ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex) const;
	void updateStreamInputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const;
	void updateStreamOutputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const;
	void updateStreamInputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings) const;
	void updateStreamInputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings) const;
	void updateStreamOutputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings) const;
	void updateStreamOutputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings) const;
	void updateEntityName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityName) const;
	void updateEntityGroupName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityGroupName) const;
	void updateConfigurationName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) const;
	void updateStreamInputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName) const;
	void updateStreamOutputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName) const;

	mutable std::recursive_mutex _lockEntities{}; // Lock for _controlledEntities (required since ControllerEntity::Delegate notifications can occur from 2 different threads)
	std::unordered_map<UniqueIdentifier, ControlledEntityImpl::UniquePointer> _controlledEntities;
	EndStation::UniquePointer _endStation{ nullptr, nullptr };
	entity::ControllerEntity* _controller{ nullptr };
	std::string _preferedLocale{ "en-US" };
};


/* ************************************************************************** */
/* ControllerImpl class definition                                               */
/* ************************************************************************** */
ControllerImpl::ControllerImpl(EndStation::ProtocolInterfaceType const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, entity::model::VendorEntityModel const vendorEntityModelID, std::string const& preferedLocale)
	: _preferedLocale(preferedLocale)
{
	try
	{
		_endStation = EndStation::create(protocolInterfaceType, interfaceName);
		_controller = _endStation->addControllerEntity(progID, vendorEntityModelID, this);
	}
	catch (EndStation::Exception const& e)
	{
		auto const err = e.getError();
		switch (err)
		{
			case EndStation::Error::InvalidProtocolInterfaceType:
				throw Exception(Error::InvalidProtocolInterfaceType, e.what());
			case EndStation::Error::InterfaceOpenError:
				throw Exception(Error::InterfaceOpenError, e.what());
			case EndStation::Error::InterfaceNotFound:
				throw Exception(Error::InterfaceNotFound, e.what());
			case EndStation::Error::InterfaceInvalid:
				throw Exception(Error::InterfaceInvalid, e.what());
			default:
				assert(false && "Unhandled exception");
				throw Exception(Error::InternalError, e.what());
		}
	}
	catch (Exception const& e)
	{
		assert(false && "Unhandled exception");
		throw Exception(Error::InternalError, e.what());
	}
}

ControllerImpl::~ControllerImpl()
{
	// Notify all entities they are going offline
	{
		std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities
		for (auto const& entityKV : _controlledEntities)
		{
			auto const& entity = entityKV.second;
			if (entity->wasAdvertised())
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityOffline, entity.get());
			}
		}
	}

	// Remove all observers, we don't want to trigger notifications for upcoming actions
	removeAllObservers();

	// Release all acquired entities by this controller before destroying everything
	{
		std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities
		_controller->setDelegate(nullptr); // Remove the delegate, we don't want notifications anymore
		for (auto const& entityKV : _controlledEntities)
		{
			auto const& controlledEntity = entityKV.second;
			if (controlledEntity->isAcquired())
			{
				auto const& entityID = entityKV.first;
				_controller->releaseEntity(entityID, entity::model::DescriptorType::Entity, 0u, nullptr); // We don't need the result handler, let's just hope our message was properly sent and received!
			}
		}
		_controlledEntities.clear();
	}
}

void ControllerImpl::checkAdvertiseEntity(ControlledEntityImpl* const entity) const noexcept
{
	if (!entity->wasAdvertised())
	{
		if (entity->gotAllQueries())
		{
			entity->setAdvertised(true);
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityOnline, entity);
		}
	}
}

/* _lockEntities must be taken */
void ControllerImpl::handleStreamStateNotification(ControlledEntityImpl* const listenerEntity, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIndex const listenerStreamIndex, bool const isConnected, entity::ConnectionFlags const flags, bool const isSniffed) const noexcept
{
	try
	{
		// Convert talker EID to Null EID if it's the Uninitialized one - We prefer to store a not-connected listener using NullIdentifier
		auto talker = talkerEntityID;
		if (talkerEntityID == getUninitializedIdentifier())
			talker = getNullIdentifier();

		// Build a StreamConnectedState
		auto const state = entity::model::StreamConnectedState{ talker, talkerStreamIndex, isConnected, avdecc::hasFlag(flags, entity::ConnectionFlags::FastConnect) };
		auto const cachedState = listenerEntity->getConnectedSinkState(listenerStreamIndex);

		// Check the previous state, and detect if it changed
		if (state != cachedState)
		{
			// Update our internal cache
			listenerEntity->setInputStreamState(listenerStreamIndex, state);
			// Switching from a connected stream to another connected stream *might* happen, in case we missed the Disconnect
			// Notify observers if the stream state was sniffed (and listener was advertised)
			if (isSniffed && listenerEntity->wasAdvertised())
			{
				// Got disconnected
				if (!state.isConnected && cachedState.isConnected)
				{
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onDisconnectStreamSniffed, listenerEntity, listenerStreamIndex);
				}
				// New connection
				else if (state.isConnected && !cachedState.isConnected)
				{
					ControlledEntityImpl* talkerEntity{ nullptr };
					auto talkerEntityIt = _controlledEntities.find(talker);
					if (talkerEntityIt != _controlledEntities.end())
						talkerEntity = talkerEntityIt->second.get();

					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onConnectStreamSniffed, talkerEntity, listenerEntity, talkerStreamIndex, listenerStreamIndex);
				}
				// We missed a Disconnect, simulate it
				else if (state.isConnected && cachedState.isConnected)
				{
					Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Debug, std::string("Switching from a connected stream to another, we missed a Disconnect message!"));

					// Simulate the disconnect
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onDisconnectStreamSniffed, listenerEntity, listenerStreamIndex);

					// And notify the new connection
					ControlledEntityImpl* talkerEntity{ nullptr };
					auto talkerEntityIt = _controlledEntities.find(talker);
					if (talkerEntityIt != _controlledEntities.end())
						talkerEntity = talkerEntityIt->second.get();

					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onConnectStreamSniffed, talkerEntity, listenerEntity, talkerStreamIndex, listenerStreamIndex);
				}
			}
		}
	}
	catch (ControlledEntity::Exception const&)
	{
		// We don't care about exceptions from getConnectedSink
	}
	catch (...)
	{
		assert(false && "Unknown exception");
	}
}

/* Enumeration and Control Protocol (AECP) handlers */
void ControllerImpl::onEntityDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::EntityDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onEntityDescriptorResult(") + toHexString(entityID, true) + "," + entity::ControllerEntity::statusToString(status) + ")");

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			controlledEntity->setEntityDescriptor(descriptor);
			controller->readConfigurationDescriptor(entityID, descriptor.currentConfiguration, std::bind(&ControllerImpl::onConfigurationDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		}
		else
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, controlledEntity.get(), QueryCommandError::EntityDescriptor);
		}
	}
}

void ControllerImpl::onConfigurationDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ConfigurationDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onConfigurationDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			controlledEntity->setConfigurationDescriptor(descriptor);
			auto const& entityDescriptor = controlledEntity->getEntityDescriptor();
			// Get Locales
			{
				auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::Locale);
				if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
				{
					auto count = countIt->second;
					for (auto index = entity::model::LocaleIndex(0); index < count; ++index)
					{
						controller->readLocaleDescriptor(entityID, entityDescriptor.currentConfiguration, index, std::bind(&ControllerImpl::onLocaleDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
					}
				}
				else
				{
					controlledEntity->setIgnoreQuery(ControlledEntity::EntityQuery::LocaleDescriptor);
					controlledEntity->setIgnoreQuery(ControlledEntity::EntityQuery::StringsDescriptor);
				}
			}
			// Get input streams
			{
				auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::StreamInput);
				if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
				{
					auto count = countIt->second;
					for (auto index = entity::model::StreamIndex(0); index < count; ++index)
					{
						controller->getListenerStreamState(entityID, index, std::bind(&ControllerImpl::onGetListenerStreamStateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8));
						controller->readStreamInputDescriptor(entityID, entityDescriptor.currentConfiguration, index, std::bind(&ControllerImpl::onStreamInputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
						controller->getStreamInputAudioMap(entityID, index, 0, std::bind(&ControllerImpl::onGetStreamInputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
					}
				}
				else
				{
					controlledEntity->setIgnoreQuery(ControlledEntity::EntityQuery::InputStreamDescriptor);
					controlledEntity->setIgnoreQuery(ControlledEntity::EntityQuery::InputStreamState);
					controlledEntity->setIgnoreQuery(ControlledEntity::EntityQuery::InputStreamAudioMappings);
				}
			}
			// Get output streams
			{
				auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::StreamOutput);
				if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
				{
					auto count = countIt->second;
					for (auto index = entity::model::StreamIndex(0); index < count; ++index)
					{
						controller->readStreamOutputDescriptor(entityID, entityDescriptor.currentConfiguration, index, std::bind(&ControllerImpl::onStreamOutputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
						controller->getStreamOutputAudioMap(entityID, index, 0, std::bind(&ControllerImpl::onGetStreamOutputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
					}
				}
				else
				{
					controlledEntity->setIgnoreQuery(ControlledEntity::EntityQuery::OutputStreamDescriptor);
					controlledEntity->setIgnoreQuery(ControlledEntity::EntityQuery::OutputStreamAudioMappings);
				}
			}
		}
		else
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, controlledEntity.get(), QueryCommandError::ConfigurationDescriptor);
		}
	}
}

void ControllerImpl::onStreamInputDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStreamInputDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(streamIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			controlledEntity->addInputStreamDescriptor(streamIndex, descriptor);
		}
		else
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, controlledEntity.get(), QueryCommandError::StreamInputDescriptor);
		}
		checkAdvertiseEntity(controlledEntity.get());
	}
}

void ControllerImpl::onStreamOutputDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStreamOutputDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(streamIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			controlledEntity->addOutputStreamDescriptor(streamIndex, descriptor);
		}
		else
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, controlledEntity.get(), QueryCommandError::StreamOutputDescriptor);
		}
		checkAdvertiseEntity(controlledEntity.get());
	}
}

void ControllerImpl::onLocaleDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex, entity::model::LocaleDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onLocaleDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(localeIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			bool const allLocaleLoaded = controlledEntity->addLocaleDescriptor(localeIndex, descriptor);
			if (allLocaleLoaded)
			{
				entity::model::LocaleDescriptor const* localeDescriptor{ nullptr };
				try
				{
					localeDescriptor = controlledEntity->findLocaleDescriptor(_preferedLocale);
					if (localeDescriptor == nullptr)
					{
#pragma message("TBD: Split _preferedLocale into language/country, then if findLocaleDescriptor fails and language is not 'en', try to find a locale for 'en'")
						localeDescriptor = controlledEntity->findLocaleDescriptor("en");
					}
					if (localeDescriptor != nullptr)
					{
						auto const& entityDescriptor = controlledEntity->getEntityDescriptor();
						for (auto index = entity::model::StringsIndex(0); index < localeDescriptor->numberOfStringDescriptors; ++index)
						{
							controller->readStringsDescriptor(entityID, entityDescriptor.currentConfiguration, localeDescriptor->baseStringDescriptorIndex + index, std::bind(&ControllerImpl::onStringsDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, localeDescriptor->baseStringDescriptorIndex));
						}
					}
				}
				catch (ControlledEntity::Exception const&) // No locale in the entity model
				{
				}
				catch (...)
				{
					assert(false && "Unknown exception");
				}
				if (localeDescriptor == nullptr)
					controlledEntity->setIgnoreQuery(ControlledEntity::EntityQuery::StringsDescriptor);
			}
		}
		else
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, controlledEntity.get(), QueryCommandError::LocaleDescriptor);
		}
		checkAdvertiseEntity(controlledEntity.get());
	}
}

void ControllerImpl::onStringsDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex, entity::model::StringsDescriptor const& descriptor, entity::model::StringsIndex const baseStringDescriptorIndex) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStringsDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(stringsIndex) + "," + std::to_string(baseStringDescriptorIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			// Only process strings descriptor if locale descriptor is complete (can be incomplete in case an entity went offline then online again, in the middle of a strings enumeration)
			if (controlledEntity->isQueryComplete(ControlledEntity::EntityQuery::LocaleDescriptor))
				controlledEntity->addStringsDescriptor(stringsIndex, descriptor, baseStringDescriptorIndex);
		}
		else
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, controlledEntity.get(), QueryCommandError::StringsDescriptor);
		}
		checkAdvertiseEntity(controlledEntity.get());
	}
}

void ControllerImpl::onGetStreamInputAudioMapResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetStreamInputAudioMapResult(") + toHexString(entityID, true) + "," + std::to_string(streamIndex) + "," + std::to_string(numberOfMaps) + "," + std::to_string(mapIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			if (mapIndex == 0)
				controlledEntity->clearInputStreamAudioMappings(streamIndex);
			bool isComplete = mapIndex == (numberOfMaps - 1);
			controlledEntity->addInputStreamAudioMappings(streamIndex, mappings, isComplete);
			if (!isComplete)
			{
				controller->getStreamInputAudioMap(entityID, streamIndex, mapIndex + 1, std::bind(&ControllerImpl::onGetStreamInputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
			}
		}
		else
		{
			// Many devices do not implement dynamic audio mapping (Apple, Motu) so we just ignore the states
			if (hasFlag(controlledEntity->getEntity().getTalkerCapabilities(), entity::TalkerCapabilities::Implemented) && (status == entity::ControllerEntity::AemCommandStatus::NotImplemented))
			{
				controlledEntity->clearInputStreamAudioMappings(streamIndex);
				controlledEntity->addInputStreamAudioMappings(streamIndex, mappings, true); // Fill with empty mapping
			}
			else
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, controlledEntity.get(), QueryCommandError::StreamInputAudioMap);
			}
		}
		checkAdvertiseEntity(controlledEntity.get());
	}
}

void ControllerImpl::onGetStreamOutputAudioMapResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetStreamOutputAudioMapResult(") + toHexString(entityID, true) + "," + std::to_string(streamIndex) + "," + std::to_string(numberOfMaps) + "," + std::to_string(mapIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			if (mapIndex == 0)
				controlledEntity->clearOutputStreamAudioMappings(streamIndex);
			bool isComplete = mapIndex == (numberOfMaps - 1);
			controlledEntity->addOutputStreamAudioMappings(streamIndex, mappings, isComplete);
			if (!isComplete)
			{
				controller->getStreamOutputAudioMap(entityID, streamIndex, mapIndex + 1, std::bind(&ControllerImpl::onGetStreamOutputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7));
			}
		}
		else
		{
			// Many devices do not implement dynamic audio mapping (Apple, Motu) so we just ignore the states
			if (hasFlag(controlledEntity->getEntity().getTalkerCapabilities(), entity::TalkerCapabilities::Implemented) && (status == entity::ControllerEntity::AemCommandStatus::NotImplemented))
			{
				controlledEntity->clearOutputStreamAudioMappings(streamIndex);
				controlledEntity->addOutputStreamAudioMappings(streamIndex, mappings, true); // Fill with empty mapping
			}
			else
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, controlledEntity.get(), QueryCommandError::StreamOutputAudioMap);
			}
		}
		checkAdvertiseEntity(controlledEntity.get());
	}
}

/* Connection Management Protocol (ACMP) handlers */
void ControllerImpl::onConnectStreamResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onConnectStreamResult(") + toHexString(talkerEntityID, true) + "," + std::to_string(talkerStreamIndex) + "," + toHexString(listenerEntityID, true) + "," + std::to_string(listenerStreamIndex) + "," + std::to_string(connectionCount) + "," + toHexString(to_integral(flags), true) + "," + entity::ControllerEntity::statusToString(status) + ")");
}

void ControllerImpl::onDisconnectStreamResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onDisconnectStreamResult(") + toHexString(talkerEntityID, true) + "," + std::to_string(talkerStreamIndex) + "," + toHexString(listenerEntityID, true) + "," + std::to_string(listenerStreamIndex) + "," + std::to_string(connectionCount) + "," + toHexString(to_integral(flags), true) + "," + entity::ControllerEntity::statusToString(status) + ")");
}

void ControllerImpl::onGetListenerStreamStateResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetListenerStreamStateResult(") + toHexString(listenerEntityID, true) + "," + entity::ControllerEntity::statusToString(status) + ")");

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto listenerEntityIt = _controlledEntities.find(listenerEntityID);
	if (listenerEntityIt != _controlledEntities.end())
	{
		auto& listener = listenerEntityIt->second;
		if (!!status)
		{
			// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
			handleStreamStateNotification(listener.get(), talkerEntityID, talkerStreamIndex, listenerStreamIndex, connectionCount != 0, flags, false);
		}
		else
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, listener.get(), QueryCommandError::ListenerStreamState);
		}
		checkAdvertiseEntity(listener.get());
	}
}

/* Global notifications */
void ControllerImpl::onTransportError(entity::ControllerEntity const* const /*controller*/) noexcept
{
	notifyObserversMethod<Controller::Observer>(&Controller::Observer::onTransportError);
}

/* Discovery Protocol (ADP) delegate */
void ControllerImpl::onEntityOnline(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::Entity const& entity) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onEntityOnline: ") + toHexString(entityID, true));

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

#ifdef DEBUG
	assert(_controlledEntities.find(entityID) == _controlledEntities.end());
#endif
	// Create and add the entity
	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt == _controlledEntities.end())
	{
		auto& controlledEntity = _controlledEntities.insert(std::make_pair(entityID, std::make_unique<ControlledEntityImpl>(entity))).first->second;

		// Check if AEM is supported by this entity
		if (hasFlag(entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		{
			// Register for unsolicited notifications
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("Registering for unsolicited notifications"));
			controller->registerUnsolicitedNotifications(entityID, {});

			// Request its entity descriptor
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("Requesting entity's descriptor"));
			controller->readEntityDescriptor(entityID, std::bind(&ControllerImpl::onEntityDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		}
		else
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("Entity does not support AEM, simply advertise it"));
			controlledEntity->setAllIgnored();
			checkAdvertiseEntity(controlledEntity.get());
		}
	}
	else
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Debug, std::string("onEntityOnline: Entity already registered, updating it"));
		// This should not happen, but just in case... update it
		onEntityUpdate(controller, entityID, entity);
	}

}

void ControllerImpl::onEntityUpdate(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::Entity const& entity) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onEntityUpdate: ") + toHexString(entityID, true));

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

#ifdef DEBUG
	assert(_controlledEntities.find(entityID) != _controlledEntities.end());
#endif
	// Update the entity
	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		controlledEntity->updateEntity(entity);
		// Maybe we need to notify the upper layers (someday, if required)
	}
}

void ControllerImpl::onEntityOffline(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onEntityOffline: ") + toHexString(entityID, true));

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto const& controlledEntity = entityIt->second;
		updateAcquiredState(*controlledEntity, getUninitializedIdentifier(), entity::model::DescriptorType::Entity, 0u, true);

		// Entity was advertised to the user, notify observers
		if (controlledEntity->wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityOffline, controlledEntity.get());
			controlledEntity->setAdvertised(false);
		}
		// Remove it
		_controlledEntities.erase(entityIt);
	}
}

/* Connection Management Protocol sniffed messages (ACMP) */
void ControllerImpl::onConnectStreamSniffed(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(listenerEntityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			// Do not trust the connectionCount value to determine if the listener is connected, but rather use the status code (SUCCESS means connection is established)
			handleStreamStateNotification(controlledEntity.get(), talkerEntityID, talkerStreamIndex, listenerStreamIndex, true, flags, true);
		}
		// We don't care about sniffed errors
	}
}

void ControllerImpl::onFastConnectStreamSniffed(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(listenerEntityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			// Do not trust the connectionCount value to determine if the listener is connected, but rather use the status code (SUCCESS means connection is established)
			handleStreamStateNotification(controlledEntity.get(), talkerEntityID, talkerStreamIndex, listenerStreamIndex, true, flags, true);
		}
		// We don't care about sniffed errors
	}
}

void ControllerImpl::onDisconnectStreamSniffed(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*talkerEntityID*/, entity::model::StreamIndex const /*talkerStreamIndex*/, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(listenerEntityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			// Do not trust the connectionCount value to determine if the listener is disconnected, but rather use the status code (SUCCESS means disconnected)
			handleStreamStateNotification(controlledEntity.get(), getNullIdentifier(), 0, listenerStreamIndex, false, flags, true);
		}
		// We don't care about sniffed errors
	}
}

void ControllerImpl::onGetListenerStreamStateSniffed(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(listenerEntityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
			handleStreamStateNotification(controlledEntity.get(), talkerEntityID, talkerStreamIndex, listenerStreamIndex, connectionCount != 0, flags, true);
			checkAdvertiseEntity(controlledEntity.get());
		}
		// We don't care about sniffed errors
	}
}

/* Unsolicited notifications (not triggered for our own commands, the command's 'result' method will be called in that case) and only if command has no error */
void ControllerImpl::onEntityAcquired(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		try
		{
			auto& controlledEntity = entityIt->second;
			updateAcquiredState(*controlledEntity, owningEntity, descriptorType, descriptorIndex);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			assert(false && "onEntityAcquired succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onEntityAcquired(" + toHexString(entityID, true) + "," + toHexString(owningEntity, true) + "," + std::to_string(to_integral(descriptorType)) + "," + std::to_string(descriptorIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onEntityReleased(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		try
		{
			auto& controlledEntity = entityIt->second;
			updateAcquiredState(*controlledEntity, owningEntity, descriptorType, descriptorIndex);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			assert(false && "onEntityReleased succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onEntityReleased(" + toHexString(entityID, true) + "," + toHexString(owningEntity, true) + "," + std::to_string(to_integral(descriptorType)) + "," + std::to_string(descriptorIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onConfigurationChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		try
		{
			auto& controlledEntity = entityIt->second;
			updateConfiguration(controller, *controlledEntity, configurationIndex);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			assert(false && "onConfigurationChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onConfigurationChanged(" + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamInputFormatChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		try
		{
			auto& controlledEntity = entityIt->second;
			updateStreamInputFormat(*controlledEntity, streamIndex, streamFormat);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			assert(false && "onStreamInputFormatChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamInputFormatChanged(" + toHexString(entityID, true) + "," + std::to_string(streamIndex) + "," + toHexString(streamFormat, true) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamOutputFormatChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		try
		{
			auto& controlledEntity = entityIt->second;
			updateStreamOutputFormat(*controlledEntity, streamIndex, streamFormat);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			assert(false && "onStreamOutputFormatChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamOutputFormatChanged(" + toHexString(entityID, true) + "," + std::to_string(streamIndex) + "," + toHexString(streamFormat, true) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamInputAudioMappingsChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		try
		{
			auto& controlledEntity = entityIt->second;
			// Only support the case where numberOfMaps == 1
			if (numberOfMaps != 1 || mapIndex != 0)
				return;

			controlledEntity->clearInputStreamAudioMappings(streamIndex);
			updateStreamInputAudioMappingsAdded(*controlledEntity, streamIndex, mappings);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			assert(false && "onStreamInputAudioMappingsChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamInputAudioMappingsChanged(" + toHexString(entityID, true) + "," + std::to_string(streamIndex) + "," + std::to_string(numberOfMaps) + "," + std::to_string(mapIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamOutputAudioMappingsChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		try
		{
			auto& controlledEntity = entityIt->second;
			// Only support the case where numberOfMaps == 1
			if (numberOfMaps != 1 || mapIndex != 0)
				return;

			controlledEntity->clearOutputStreamAudioMappings(streamIndex);
			updateStreamOutputAudioMappingsAdded(*controlledEntity, streamIndex, mappings);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			assert(false && "onStreamOutputAudioMappingsChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamOutputAudioMappingsChanged(" + toHexString(entityID, true) + "," + std::to_string(streamIndex) + "," + std::to_string(numberOfMaps) + "," + std::to_string(mapIndex) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamInputInfoChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*entityID*/, entity::model::StreamIndex const /*streamIndex*/, entity::model::StreamInfo const& /*info*/) noexcept
{
}

void ControllerImpl::onStreamOutputInfoChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*entityID*/, entity::model::StreamIndex const /*streamIndex*/, entity::model::StreamInfo const& /*info*/) noexcept
{
}

void ControllerImpl::onEntityNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityName) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		try
		{
			auto& controlledEntity = entityIt->second;
			updateEntityName(*controlledEntity, entityName);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			assert(false && "onEntityNameChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onEntityNameChanged(" + toHexString(entityID, true) + "," + std::string(entityName) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onEntityGroupNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityGroupName) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		try
		{
			auto& controlledEntity = entityIt->second;
			updateEntityGroupName(*controlledEntity, entityGroupName);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			assert(false && "onEntityGroupNameChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onEntityGroupNameChanged(" + toHexString(entityID, true) + "," + std::string(entityGroupName) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onConfigurationNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		try
		{
			auto& controlledEntity = entityIt->second;
			updateConfigurationName(*controlledEntity, configurationIndex, configurationName);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			assert(false && "onConfigurationNameChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onConfigurationNameChanged(" + toHexString(entityID, true) + "," + std::to_string(configurationIndex ) + "," + std::string(configurationName) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamInputNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		try
		{
			auto& controlledEntity = entityIt->second;
			updateStreamInputName(*controlledEntity, configurationIndex, streamIndex, streamName);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			assert(false && "onStreamInputNameChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamInputNameChanged(" + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(streamIndex ) + "," + std::string(streamName) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamOutputNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		try
		{
			auto& controlledEntity = entityIt->second;
			updateStreamOutputName(*controlledEntity, configurationIndex, streamIndex, streamName);
		}
		catch (controller::ControlledEntity::Exception const& e)
		{
			assert(false && "onStreamOutputNameChanged succeeded on the entity, but failed to update local model");
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("onStreamOutputNameChanged(" + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(streamIndex) + "," + std::string(streamName) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamInputStarted(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*entityID*/, entity::model::StreamIndex const /*streamIndex*/) noexcept
{
}

void ControllerImpl::onStreamOutputStarted(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*entityID*/, entity::model::StreamIndex const /*streamIndex*/) noexcept
{
}

void ControllerImpl::onStreamInputStopped(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*entityID*/, entity::model::StreamIndex const /*streamIndex*/) noexcept
{
}

void ControllerImpl::onStreamOutputStopped(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*entityID*/, entity::model::StreamIndex const /*streamIndex*/) noexcept
{
}

/* ************************************************************************** */
/* Controller overrides                                                       */
/* ************************************************************************** */
/* Controller configuration */
void ControllerImpl::enableEntityAdvertising(std::uint32_t const availableDuration)
{
	if(!_controller->enableEntityAdvertising(availableDuration))
		throw Exception(Error::DuplicateProgID, "Specified ProgID already in use on the local computer");
}

void ControllerImpl::disableEntityAdvertising() noexcept
{
	_controller->disableEntityAdvertising();
}

/* Enumeration and Control Protocol (AECP) */
void ControllerImpl::acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, AcquireEntityHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("acquireEntity requested for ") + toHexString(targetEntityID, true));

		// Already acquired or acquiring, don't do anything (we want to try to acquire if it's flagged as acquired by another controller, in case it went offline without notice)
		if (controlledEntity->isAcquired() || controlledEntity->isAcquiring())
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("acquireEntity not sent, ") + toHexString(targetEntityID, true) + "is " + (controlledEntity->isAcquired() ? "already acquired" : "being acquired"));
			return;
		}
		controlledEntity->setAcquireState(ControlledEntity::AcquireState::TryAcquire);
		_controller->acquireEntity(targetEntityID, isPersistent, entity::model::DescriptorType::Entity, 0u, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("acquireEntity result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				try
				{
					switch (status)
					{
						case entity::ControllerEntity::AemCommandStatus::Success:
							updateAcquiredState(*controlledEntity, owningEntity, descriptorType, descriptorIndex);
							break;
						case entity::ControllerEntity::AemCommandStatus::AcquiredByOther:
							updateAcquiredState(*controlledEntity, owningEntity, descriptorType, descriptorIndex);
							break;
						case entity::ControllerEntity::AemCommandStatus::NotImplemented:
						case entity::ControllerEntity::AemCommandStatus::NotSupported:
							updateAcquiredState(*controlledEntity, getNullIdentifier(), descriptorType, descriptorIndex);
							break;
						default:
							// In case of error, set the state to undefined
							updateAcquiredState(*controlledEntity, getUninitializedIdentifier(), descriptorType, descriptorIndex, true);
							break;
					}
				}
				catch (controller::ControlledEntity::Exception const& e)
				{
					assert(false && "acquireEntity succeeded on the entity, but failed to update local model");
					Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("acquireEntity succeeded on the entity, but failed to update local model: ") + e.what());
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status, owningEntity);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status, owningEntity);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity, getNullIdentifier());
	}
}

void ControllerImpl::releaseEntity(UniqueIdentifier const targetEntityID, ReleaseEntityHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("releaseEntity requested for ") + toHexString(targetEntityID, true));
		_controller->releaseEntity(targetEntityID, entity::model::DescriptorType::Entity, 0u, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("releaseEntity result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				try
				{
					if (status == entity::ControllerEntity::AemCommandStatus::Success) // Only change the acquire state in case of success
						updateAcquiredState(*controlledEntity, owningEntity, descriptorType, descriptorIndex);
				}
				catch (controller::ControlledEntity::Exception const& e)
				{
					assert(false && "releaseEntity succeeded on the entity, but failed to update local model");
					Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("releaseEntity succeeded on the entity, but failed to update local model: ") + e.what());
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status, owningEntity);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status, owningEntity);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity, getNullIdentifier());
	}
}

void ControllerImpl::setConfiguration(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setConfiguration requested for ") + toHexString(targetEntityID, true));
		_controller->setConfiguration(targetEntityID, configurationIndex, [this, handler](entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setConfiguration result for ") + toHexString(entityID, true) + " -> " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				if (!!status)
				{
					try
					{
						updateConfiguration(controller, *controlledEntity, configurationIndex);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						assert(false && "setConfiguration succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setConfiguration succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamInputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamInputFormat requested for ") + toHexString(targetEntityID, true));
		_controller->setStreamInputFormat(targetEntityID, streamIndex, streamFormat, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamInputFormat result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				if (!!status)
				{
					try
					{
						updateStreamInputFormat(*controlledEntity, streamIndex, streamFormat);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						assert(false && "setStreamInputFormat succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setStreamInputFormat succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamOutputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamOutputFormat requested for ") + toHexString(targetEntityID, true));
		_controller->setStreamOutputFormat(targetEntityID, streamIndex, streamFormat, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamOutputFormat result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				if (!!status)
				{
					try
					{
						updateStreamOutputFormat(*controlledEntity, streamIndex, streamFormat);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						assert(false && "setStreamOutputFormat succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setStreamOutputFormat succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::addStreamInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, std::vector<entity::model::AudioMapping> const& mappings, AddStreamInputAudioMappingsHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("addStreamInputAudioMappings requested for ") + toHexString(targetEntityID, true));
		_controller->addStreamInputAudioMappings(targetEntityID, streamIndex, mappings, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("addStreamInputAudioMappings result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				if (!!status)
				{
					try
					{
						updateStreamInputAudioMappingsAdded(*controlledEntity, streamIndex, mappings);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						assert(false && "addStreamInputAudioMappings succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("addStreamInputAudioMappings succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::addStreamOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings, AddStreamOutputAudioMappingsHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("addStreamOutputAudioMappings requested for ") + toHexString(targetEntityID, true));
		_controller->addStreamOutputAudioMappings(targetEntityID, streamIndex, mappings, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("addStreamOutputAudioMappings result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				if (!!status)
				{
					try
					{
						updateStreamOutputAudioMappingsAdded(*controlledEntity, streamIndex, mappings);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						assert(false && "addStreamOutputAudioMappings succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("addStreamOutputAudioMappings succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::removeStreamInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, std::vector<entity::model::AudioMapping> const& mappings, RemoveStreamInputAudioMappingsHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("removeStreamInputAudioMappings requested for ") + toHexString(targetEntityID, true));
		_controller->removeStreamInputAudioMappings(targetEntityID, streamIndex, mappings, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("removeStreamInputAudioMappings result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				if (!!status)
				{
					try
					{
						updateStreamInputAudioMappingsRemoved(*controlledEntity, streamIndex, mappings);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						assert(false && "removeStreamInputAudioMappings succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("removeStreamInputAudioMappings succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::removeStreamOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings, RemoveStreamOutputAudioMappingsHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("removeStreamOutputAudioMappings requested for ") + toHexString(targetEntityID, true));
		_controller->removeStreamOutputAudioMappings(targetEntityID, streamIndex, mappings, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("removeStreamOutputAudioMappings result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				if (!!status)
				{
					try
					{
						updateStreamOutputAudioMappingsRemoved(*controlledEntity, streamIndex, mappings);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						assert(false && "removeStreamOutputAudioMappings succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("removeStreamOutputAudioMappings succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::startStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("startStreamInput requested for ") + toHexString(targetEntityID, true) + ":" + std::to_string(streamIndex));
		_controller->startStreamInput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("startStreamInput result for ") + toHexString(entityID, true) + ":" + std::to_string(streamIndex) + " -> " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				try
				{
#pragma message("TBD: Save the stream running state, if !!status")
				}
				catch (controller::ControlledEntity::Exception const& e)
				{
					assert(false && "startStreamInput succeeded on the entity, but failed to update local model");
					Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("startStreamInput succeeded on the entity, but failed to update local model: ") + e.what());
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::stopStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("stopStreamInput requested for ") + toHexString(targetEntityID, true) + ":" + std::to_string(streamIndex));
		_controller->stopStreamInput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("stopStreamInput result for ") + toHexString(entityID, true) + ":" + std::to_string(streamIndex) + " -> " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				try
				{
#pragma message("TBD: Save the stream running state, if !!status")
				}
				catch (controller::ControlledEntity::Exception const& e)
				{
					assert(false && "stopStreamInput succeeded on the entity, but failed to update local model");
					Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("stopStreamInput succeeded on the entity, but failed to update local model: ") + e.what());
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::startStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("startStreamOutput requested for ") + toHexString(targetEntityID, true) + ":" + std::to_string(streamIndex));
		_controller->startStreamOutput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("startStreamOutput result for ") + toHexString(entityID, true) + ":" + std::to_string(streamIndex) + " -> " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				try
				{
#pragma message("TBD: Save the stream running state, if !!status")
				}
				catch (controller::ControlledEntity::Exception const& e)
				{
					assert(false && "startStreamOutput succeeded on the entity, but failed to update local model");
					Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("startStreamOutput succeeded on the entity, but failed to update local model: ") + e.what());
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::stopStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("stopStreamOutput requested for ") + toHexString(targetEntityID, true) + ":" + std::to_string(streamIndex));
		_controller->stopStreamOutput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("stopStreamOutput result for ") + toHexString(entityID, true) + ":" + std::to_string(streamIndex) + " -> " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				try
				{
#pragma message("TBD: Save the stream running state, if !!status")
				}
				catch (controller::ControlledEntity::Exception const& e)
				{
					assert(false && "stopStreamOutput succeeded on the entity, but failed to update local model");
					Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("stopStreamOutput succeeded on the entity, but failed to update local model: ") + e.what());
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setEntityName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityNameHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setEntityName requested for ") + toHexString(targetEntityID, true));
		_controller->setEntityName(targetEntityID, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setEntityName result for ") + toHexString(entityID, true) + " -> " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				if (!!status) // Only change the name in case of success
				{
					try
					{
						updateEntityName(*controlledEntity, name);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						assert(false && "setEntityName succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setEntityName succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setEntityGroupName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityGroupNameHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setEntityGroupName requested for ") + toHexString(targetEntityID, true));
		_controller->setEntityGroupName(targetEntityID, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setEntityGroupName result for ") + toHexString(entityID, true) + " -> " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				if (!!status) // Only change the name in case of success
				{
					try
					{
						updateEntityGroupName(*controlledEntity, name);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						assert(false && "setEntityGroupName succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setEntityGroupName succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name, SetConfigurationNameHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setConfigurationName requested for ") + toHexString(targetEntityID, true));
		_controller->setConfigurationName(targetEntityID, configurationIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setConfigurationName result for ") + toHexString(entityID, true) + " -> " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				if (!!status) // Only change the name in case of success
				{
					try
					{
						updateConfigurationName(*controlledEntity, configurationIndex, name);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						assert(false && "setConfigurationName succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setConfigurationName succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamInputNameHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities
	
	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamInputName requested for ") + toHexString(targetEntityID, true));
		_controller->setStreamInputName(targetEntityID, configurationIndex, streamIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamInputName result for ") + toHexString(entityID, true) + " -> " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities
			
			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				if (!!status) // Only change the name in case of success
				{
					try
					{
						updateStreamInputName(*controlledEntity, configurationIndex, streamIndex, name);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						assert(false && "setStreamInputName succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setStreamInputName succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamOutputNameHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities
	
	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamOutputName requested for ") + toHexString(targetEntityID, true));
		_controller->setStreamOutputName(targetEntityID, configurationIndex, streamIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamOutputName result for ") + toHexString(entityID, true) + " -> " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities
			
			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				if (!!status) // Only change the name in case of success
				{
					try
					{
						updateStreamOutputName(*controlledEntity, configurationIndex, streamIndex, name);
					}
					catch (controller::ControlledEntity::Exception const& e)
					{
						assert(false && "setStreamOutputName succeeded on the entity, but failed to update local model");
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Warn, std::string("setStreamOutputName succeeded on the entity, but failed to update local model: ") + e.what());
					}
				}
				invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::connectStream(UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, ConnectStreamHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(listenerEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("connectStream requested for ") + toHexString(listenerEntityID, true));
		_controller->connectStream(talkerEntityID, talkerStreamIndex, listenerEntityID, listenerStreamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("connectStream result for ") + toHexString(listenerEntityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			ControlledEntityImpl const* talker{ nullptr };
			ControlledEntityImpl* listener{ nullptr };
			entity::ControllerEntity::ControlStatus controlStatus{ entity::ControllerEntity::ControlStatus::UnknownEntity };

			auto listenerEntityIt = _controlledEntities.find(listenerEntityID);
			if (listenerEntityIt != _controlledEntities.end())
			{
				controlStatus = status;
				listener = listenerEntityIt->second.get();
				auto talkerEntityIt = _controlledEntities.find(talkerEntityID);
				if (talkerEntityIt != _controlledEntities.end())
					talker = talkerEntityIt->second.get();
				if (!!status)
					// Do not trust the connectionCount value to determine if the listener is connected, but rather use the status code (SUCCESS means connection is established)
					handleStreamStateNotification(listener, talkerEntityID, talkerStreamIndex, listenerStreamIndex, true, flags, false);
			}
			invokeProtectedHandler(handler, talker, listener, talkerStreamIndex, listenerStreamIndex, controlStatus);
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, nullptr, entity::model::StreamIndex(0), entity::model::StreamIndex(0), entity::ControllerEntity::ControlStatus::UnknownEntity);
	}
}

void ControllerImpl::disconnectStream(UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, DisconnectStreamHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(listenerEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("disconnectStream requested for ") + toHexString(listenerEntityID, true));
		_controller->disconnectStream(talkerEntityID, talkerStreamIndex, listenerEntityID, listenerStreamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const /*talkerEntityID*/, entity::model::StreamIndex const /*talkerStreamIndex*/, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("disconnectStream result for ") + toHexString(listenerEntityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			ControlledEntityImpl* listener{ nullptr };
			entity::ControllerEntity::ControlStatus controlStatus{ entity::ControllerEntity::ControlStatus::UnknownEntity };

			bool shouldNotifyHandler{ true }; // Shall we notify the handler right now, or do we have to send another message before
			auto listenerEntityIt = _controlledEntities.find(listenerEntityID);
			if (listenerEntityIt != _controlledEntities.end())
			{
				controlStatus = status;
				listener = listenerEntityIt->second.get();
				// In case of a disconnect we might get an error (forwarded from the talker) but the stream is actually disconnected.
				// In that case, we have to query the listener stream state in order to know the actual connection state
				if (!status && status != entity::ControllerEntity::ControlStatus::NotConnected)
				{
					shouldNotifyHandler = false; // Don't notify handler right now, wait for getListenerStreamState answer
					_controller->getListenerStreamState(listenerEntityID, listenerStreamIndex, [this, handler, disconnectStatus = status](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
					{
						std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

						ControlledEntityImpl* listener{ nullptr };
						entity::ControllerEntity::ControlStatus controlStatus{ entity::ControllerEntity::ControlStatus::UnknownEntity };

						auto listenerEntityIt = _controlledEntities.find(listenerEntityID);
						if (listenerEntityIt != _controlledEntities.end())
						{
							listener = listenerEntityIt->second.get();
							if (!!status)
							{
								// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
								bool const isStillConnected = connectionCount != 0;
								handleStreamStateNotification(listener, talkerEntityID, talkerStreamIndex, listenerStreamIndex, isStillConnected, flags, false);
								// Status to return depends if we actually got disconnected (success in that case)
								controlStatus = isStillConnected ? disconnectStatus : entity::ControllerEntity::ControlStatus::Success;
							}
						}
						invokeProtectedHandler(handler, listener, listenerStreamIndex, controlStatus);
					});
				}
				else if (!!status) // No error, update the connection state
				{
					// Do not trust the connectionCount value to determine if the listener is disconnected, but rather use the status code (SUCCESS means disconnected)
					handleStreamStateNotification(listener, getNullIdentifier(), 0, listenerStreamIndex, false, flags, false);
				}
			}
			if (shouldNotifyHandler)
				invokeProtectedHandler(handler, listener, listenerStreamIndex, controlStatus);
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::model::StreamIndex(0), entity::ControllerEntity::ControlStatus::UnknownEntity);
	}
}

void ControllerImpl::getListenerStreamState(UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, GetListenerStreamStateHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(listenerEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("disconnectStream requested for ") + toHexString(listenerEntityID, true));
		_controller->getListenerStreamState(listenerEntityID, listenerStreamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("disconnectStream result for ") + toHexString(listenerEntityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			ControlledEntityImpl const* talker{ nullptr };
			ControlledEntityImpl* listener{ nullptr };
			entity::ControllerEntity::ControlStatus controlStatus{ entity::ControllerEntity::ControlStatus::UnknownEntity };

			auto listenerEntityIt = _controlledEntities.find(listenerEntityID);
			if (listenerEntityIt != _controlledEntities.end())
			{
				controlStatus = status;
				listener = listenerEntityIt->second.get();
				if (!!status)
				{
					// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
					handleStreamStateNotification(listener, talkerEntityID, talkerStreamIndex, listenerStreamIndex, connectionCount != 0, flags, false);
					checkAdvertiseEntity(listener);

					auto talkerEntityIt = _controlledEntities.find(talkerEntityID);
					if (talkerEntityIt != _controlledEntities.end())
						talker = talkerEntityIt->second.get();
				}
			}
			invokeProtectedHandler(handler, talker, listener, talkerStreamIndex, listenerStreamIndex, connectionCount, flags, controlStatus);
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, nullptr, entity::model::StreamIndex(0), entity::model::StreamIndex(0), uint16_t(0), entity::ConnectionFlags::None, entity::ControllerEntity::ControlStatus::UnknownEntity);
	}
}

void ControllerImpl::updateAcquiredState(ControlledEntityImpl& controlledEntity, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const /*descriptorIndex*/, bool const undefined) const
{
#pragma message("TBD: Handle acquire state tree")
	if (descriptorType == entity::model::DescriptorType::Entity)
	{
		auto owningController{ getUninitializedIdentifier() };
		auto acquireState{ ControlledEntity::AcquireState::Undefined };
		if (undefined)
		{
			owningController = getUninitializedIdentifier();
			acquireState = ControlledEntity::AcquireState::Undefined;
		}
		else
		{
			owningController = owningEntity;

			if (!isValidUniqueIdentifier(owningEntity)) // No more controller
				acquireState = ControlledEntity::AcquireState::NotAcquired;
			else if (owningEntity == _controller->getEntityID()) // Controlled by myself
				acquireState = ControlledEntity::AcquireState::Acquired;
			else // Or acquired by another controller
				acquireState = ControlledEntity::AcquireState::AcquiredByOther;
		}

		controlledEntity.setAcquireState(acquireState);
		controlledEntity.setOwningController(owningController);

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAcquireStateChanged, &controlledEntity, acquireState, owningController);
		}
	}
}

void ControllerImpl::updateConfiguration(entity::ControllerEntity const* const controller, ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityDescriptor();
		entityDescriptor.currentConfiguration = configurationIndex;

		// Right now, simulate the entity going offline then online again - TBD: Handle multiple configurations, see https://github.com/L-Acoustics/avdecc/issues/3
		auto const e = controlledEntity.getEntity(); // Make a copy of the Entity object since it will be destroyed during onEntityOffline
		auto const entityID = e.getEntityID();
		auto* self = const_cast<ControllerImpl*>(this);
		self->onEntityOffline(controller, entityID);
		self->onEntityOnline(controller, entityID, e);
	}
	catch (Exception const&)
	{
	}
}

void ControllerImpl::updateStreamInputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const
{
	controlledEntity.setInputStreamFormat(streamIndex, streamFormat);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputFormatChanged, &controlledEntity, streamIndex, streamFormat);
	}
}

void ControllerImpl::updateStreamOutputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const
{
	controlledEntity.setOutputStreamFormat(streamIndex, streamFormat);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputFormatChanged, &controlledEntity, streamIndex, streamFormat);
	}
}

void ControllerImpl::updateStreamInputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings) const
{
	controlledEntity.addInputStreamAudioMappings(streamIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputAudioMappingsChanged, &controlledEntity, streamIndex);
	}
}

void ControllerImpl::updateStreamInputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings) const
{
	controlledEntity.removeInputStreamAudioMappings(streamIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputAudioMappingsChanged, &controlledEntity, streamIndex);
	}
}

void ControllerImpl::updateStreamOutputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings) const
{
	controlledEntity.addOutputStreamAudioMappings(streamIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputAudioMappingsChanged, &controlledEntity, streamIndex);
	}
}

void ControllerImpl::updateStreamOutputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings) const
{
	controlledEntity.removeOutputStreamAudioMappings(streamIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputAudioMappingsChanged, &controlledEntity, streamIndex);
	}
}

void ControllerImpl::updateEntityName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityName) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityDescriptor();
		entityDescriptor.entityName = entityName;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityNameChanged, &controlledEntity, entityName);
		}
	}
	catch (Exception const&)
	{
	}
}

void ControllerImpl::updateEntityGroupName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityGroupName) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityDescriptor();
		entityDescriptor.groupName = entityGroupName;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityGroupNameChanged, &controlledEntity, entityGroupName);
		}
	}
	catch (Exception const&)
	{
	}
}

void ControllerImpl::updateConfigurationName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityDescriptor();

#pragma message("TBD: Handle multiple configurations, not just the active one")
		if (entityDescriptor.currentConfiguration == configurationIndex)
		{
			auto& configurationDescriptor = controlledEntity.getConfigurationDescriptor();
			configurationDescriptor.objectName = configurationName;
		}

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onConfigurationNameChanged, &controlledEntity, configurationIndex, configurationName);
		}
	}
	catch (Exception const&)
	{
	}
}
	
void ControllerImpl::updateStreamInputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityDescriptor();

#pragma message("TBD: Handle multiple configurations, not just the active one")
		if (entityDescriptor.currentConfiguration == configurationIndex)
		{
			auto& streamDescriptor = controlledEntity.getStreamInputDescriptor(streamIndex);
			streamDescriptor.objectName = streamInputName;
		}

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputNameChanged, &controlledEntity, configurationIndex, streamIndex, streamInputName);
		}
	}
	catch (Exception const&)
	{
	}
}

void ControllerImpl::updateStreamOutputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityDescriptor();

#pragma message("TBD: Handle multiple configurations, not just the active one")
		if (entityDescriptor.currentConfiguration == configurationIndex)
		{
			auto& streamDescriptor = controlledEntity.getStreamOutputDescriptor(streamIndex);
			streamDescriptor.objectName = streamOutputName;
		}

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputNameChanged, &controlledEntity, configurationIndex, streamIndex, streamOutputName);
		}
	}
	catch (Exception const&)
	{
	}
}

void ControllerImpl::runMethodForEntity(UniqueIdentifier const entityID, RunMethodForEntityHandler const& handler) const
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	try
	{
		handler(entityIt == _controlledEntities.end() ? nullptr : entityIt->second.get());
	}
	catch (...)
	{
	}
}

void ControllerImpl::destroy() noexcept
{
	delete this;
}

Controller* LA_AVDECC_CONTROLLER_CALL_CONVENTION Controller::createRawController(EndStation::ProtocolInterfaceType const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, entity::model::VendorEntityModel const vendorEntityModelID, std::string const& preferedLocale)
{
	return new ControllerImpl(protocolInterfaceType, interfaceName, progID, vendorEntityModelID, preferedLocale);
}

} // namespace controller
} // namespace avdecc
} // namespace la
