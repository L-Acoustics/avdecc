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

	/* Connection Management Protocol (ACMP) */
	virtual void connectStream(UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, ConnectStreamHandler const& handler) const noexcept override;
	virtual void disconnectStream(UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, DisconnectStreamHandler const& handler) const noexcept override;
	virtual void getListenerStreamState(UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, GetListenerStreamStateHandler const& handler) const noexcept override;

	virtual void runMethodForEntity(UniqueIdentifier const entityID, RunMethodForEntityHandler const& handler) const override;

private:
	void checkAdvertiseEntity(ControlledEntityImpl* const entity) const noexcept;
	void handleStreamStateNotification(ControlledEntityImpl* const listenerEntity, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIndex const listenerStreamIndex, bool const isConnected, entity::ConnectionFlags const flags, bool const isSniffed) const noexcept;
	void updateAcquiredState(ControlledEntityImpl& entity, UniqueIdentifier const owningEntity, bool const undefined = false) const noexcept; // TBD: We should later have a descriptor and index passed too, so we now which part of the EM graph has been acquired/released
	/* Enumeration and Control Protocol (AECP) handlers */
	void onEntityDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::EntityDescriptor const& descriptor) noexcept;
	void onConfigurationDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationDescriptor const& descriptor) noexcept;
	void onLocaleDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::LocaleDescriptor const& descriptor) noexcept;
	void onStringsDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StringsDescriptor const& descriptor, entity::model::StringsIndex const baseStringDescriptorIndex) noexcept;
	void onStreamInputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamDescriptor const& descriptor) noexcept;
	void onStreamOutputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamDescriptor const& descriptor) noexcept;
	void onGetStreamInputAudioMapResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept;
	/* Connection Management Protocol (ACMP) handlers */
	void onConnectStreamResult(entity::ControllerEntity const* const controller, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept;
	void onDisconnectStreamResult(entity::ControllerEntity const* const controller, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept;
	void onGetListenerStreamStateResult(entity::ControllerEntity const* const controller, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept;

	// entity::ControllerEntity::Delegate overrides
	/* Global notifications */
	virtual void onTransportError() noexcept override;
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
	virtual void onEntityAcquired(UniqueIdentifier const acquiredEntity, UniqueIdentifier const owningEntity) noexcept override;
	virtual void onEntityReleased(UniqueIdentifier const releasedEntity, UniqueIdentifier const owningEntity) noexcept override;
	virtual void onStreamInputFormatChanged(UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept override;
	virtual void onStreamOutputFormatChanged(UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept override;
	virtual void onStreamInputAudioMappingsChanged(UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept override;
	virtual void onStreamOutputAudioMappingsChanged(UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept override;
	virtual void onStreamInputStarted(UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamOutputStarted(UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamInputStopped(UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;
	virtual void onStreamOutputStopped(UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept override;

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
	catch (la::avdecc::Exception const& e)
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
				_controller->releaseEntity(entityID, nullptr); // We don't need the result handler, let's just hope our message was properly sent and received!
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

void ControllerImpl::updateAcquiredState(ControlledEntityImpl& entity, UniqueIdentifier const owningEntity, bool const undefined) const noexcept
{
	if (undefined)
	{
		entity.setOwningController(getUninitializedIdentifier());
		entity.setAcquireState(ControlledEntity::AcquireState::Undefined);
	}
	else
	{
		entity.setOwningController(owningEntity);
		if (!isValidUniqueIdentifier(owningEntity)) // No more controller
			entity.setAcquireState(ControlledEntity::AcquireState::NotAcquired);
		else if (owningEntity == _controller->getEntityID()) // Controlled by myself
			entity.setAcquireState(ControlledEntity::AcquireState::Acquired);
		else // Or acquired by another controller
			entity.setAcquireState(ControlledEntity::AcquireState::AcquiredByOther);
	}
}

/* Enumeration and Control Protocol (AECP) handlers */
void ControllerImpl::onEntityDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::EntityDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onEntityDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(to_integral(status)) + ")");

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			controlledEntity->setEntityDescriptor(descriptor);
			controller->readConfigurationDescriptor(entityID, descriptor.currentConfiguration, std::bind(&ControllerImpl::onConfigurationDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		}
		else
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, controlledEntity.get(), QueryCommandError::EntityDescriptor);
		}
	}
}

void ControllerImpl::onConfigurationDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onConfigurationDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(to_integral(status)) + ")");

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
						controller->readLocaleDescriptor(entityID, entityDescriptor.currentConfiguration, index, std::bind(&ControllerImpl::onLocaleDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
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
						controller->readStreamInputDescriptor(entityID, entityDescriptor.currentConfiguration, index, std::bind(&ControllerImpl::onStreamInputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
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
						controller->readStreamOutputDescriptor(entityID, entityDescriptor.currentConfiguration, index, std::bind(&ControllerImpl::onStreamOutputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
					}
				}
				else
				{
					controlledEntity->setIgnoreQuery(ControlledEntity::EntityQuery::OutputStreamDescriptor);
				}
			}
		}
		else
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, controlledEntity.get(), QueryCommandError::ConfigurationDescriptor);
		}
	}
}

void ControllerImpl::onLocaleDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::LocaleDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onLocaleDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(to_integral(status)) + ")");

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			bool const allLocaleLoaded = controlledEntity->addLocaleDescriptor(descriptor);
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
							controller->readStringsDescriptor(entityID, entityDescriptor.currentConfiguration, localeDescriptor->baseStringDescriptorIndex + index, std::bind(&ControllerImpl::onStringsDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, localeDescriptor->baseStringDescriptorIndex));
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

void ControllerImpl::onStringsDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StringsDescriptor const& descriptor, entity::model::StringsIndex const baseStringDescriptorIndex) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStringsDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(to_integral(status)) + "," + std::to_string(baseStringDescriptorIndex) + ")");

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			// Only process strings descriptor if locale descriptor is complete (can be incomplete in case an entity went offline then online again, in the middle of a strings enumeration)
			if (controlledEntity->isQueryComplete(ControlledEntity::EntityQuery::LocaleDescriptor))
				controlledEntity->addStringsDescriptor(descriptor, baseStringDescriptorIndex);
		}
		else
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, controlledEntity.get(), QueryCommandError::StringsDescriptor);
		}
		checkAdvertiseEntity(controlledEntity.get());
	}
}

void ControllerImpl::onStreamInputDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStreamInputDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(to_integral(status)) + ")");

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			controlledEntity->addInputStreamDescriptor(descriptor);
		}
		else
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, controlledEntity.get(), QueryCommandError::StreamInputDescriptor);
		}
		checkAdvertiseEntity(controlledEntity.get());
	}
}

void ControllerImpl::onStreamOutputDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStreamOutputDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(to_integral(status)) + ")");

	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		if (!!status)
		{
			controlledEntity->addOutputStreamDescriptor(descriptor);
		}
		else
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, controlledEntity.get(), QueryCommandError::StreamOutputDescriptor);
		}
		checkAdvertiseEntity(controlledEntity.get());
	}
}

void ControllerImpl::onGetStreamInputAudioMapResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetStreamInputAudioMapResult(") + toHexString(entityID, true) + "," + std::to_string(to_integral(status)) + "," + std::to_string(streamIndex) + "," + std::to_string(numberOfMaps) + "," + std::to_string(mapIndex) + ")");

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

/* Connection Management Protocol (ACMP) handlers */
void ControllerImpl::onConnectStreamResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onConnectStreamResult(") + toHexString(talkerEntityID, true) + "," + std::to_string(talkerStreamIndex) + "," + toHexString(listenerEntityID, true) + "," + std::to_string(listenerStreamIndex) + "," + std::to_string(connectionCount) + "," + toHexString(to_integral(flags), true) + "," + std::to_string(to_integral(status)) + ")");
}

void ControllerImpl::onDisconnectStreamResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onDisconnectStreamResult(") + toHexString(talkerEntityID, true) + "," + std::to_string(talkerStreamIndex) + "," + toHexString(listenerEntityID, true) + "," + std::to_string(listenerStreamIndex) + "," + std::to_string(connectionCount) + "," + toHexString(to_integral(flags), true) + "," + std::to_string(to_integral(status)) + ")");
}

void ControllerImpl::onGetListenerStreamStateResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const listenerEntityID, entity::model::StreamIndex const listenerStreamIndex, UniqueIdentifier const talkerEntityID, entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetListenerStreamStateResult: ") + toHexString(listenerEntityID, true) + " status=" + std::to_string(to_integral(status)));

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
void ControllerImpl::onTransportError() noexcept
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

		// Register for unsolicited notifications
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("Registering for unsolicited notifications"));
		controller->registerUnsolicitedNotifications(entityID, {});

		// Request its entity descriptor, if AEM is supported
		if (la::avdecc::hasFlag(entity.getEntityCapabilities(), la::avdecc::entity::EntityCapabilities::AemSupported))
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("Requesting entity's descriptor"));
			controller->readEntityDescriptor(entityID, std::bind(&ControllerImpl::onEntityDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		}
		else
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("Entity does not support AEM, advertising it"));
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
		updateAcquiredState(*controlledEntity, getUninitializedIdentifier(), true);
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
void ControllerImpl::onEntityAcquired(UniqueIdentifier const acquiredEntity, UniqueIdentifier const owningEntity) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(acquiredEntity);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		updateAcquiredState(*controlledEntity, owningEntity);
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityAcquired, controlledEntity.get(), owningEntity);
	}
}

void ControllerImpl::onEntityReleased(UniqueIdentifier const releasedEntity, UniqueIdentifier const owningEntity) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(releasedEntity);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		updateAcquiredState(*controlledEntity, owningEntity);
		// Only notify when there actually is a release of the entity
		if (!isValidUniqueIdentifier(owningEntity))
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityReleased, controlledEntity.get());
		}
	}
}

void ControllerImpl::onStreamInputFormatChanged(UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		try
		{
			auto& controlledEntity = entityIt->second;
			controlledEntity->setInputStreamFormat(streamIndex, streamFormat);
			if (controlledEntity->wasAdvertised())
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputFormatChanged, controlledEntity.get(), streamIndex, streamFormat);
			}
		}
		catch (la::avdecc::controller::ControlledEntity::Exception const& e)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Debug, std::string("onStreamInputFormatChanged(" + toHexString(entityID, true) + "," + std::to_string(streamIndex) + "," + toHexString(streamFormat, true) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamOutputFormatChanged(UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		try
		{
			auto& controlledEntity = entityIt->second;
			controlledEntity->setOutputStreamFormat(streamIndex, streamFormat);
			if (controlledEntity->wasAdvertised())
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputFormatChanged, controlledEntity.get(), streamIndex, streamFormat);
			}
		}
		catch (la::avdecc::controller::ControlledEntity::Exception const& e)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Debug, std::string("onStreamOutputFormatChanged(" + toHexString(entityID, true) + "," + std::to_string(streamIndex) + "," + toHexString(streamFormat, true) + ") failed: " + e.what()));
		}
	}
}

void ControllerImpl::onStreamInputAudioMappingsChanged(UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(entityID);
	if (entityIt != _controlledEntities.end())
	{
		auto& controlledEntity = entityIt->second;
		// Only support the case where numberOfMaps == 1
		if (numberOfMaps != 1 || mapIndex != 0)
			return;

		controlledEntity->clearInputStreamAudioMappings(streamIndex);
		controlledEntity->addInputStreamAudioMappings(streamIndex, mappings);

		if (controlledEntity->wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputAudioMappingsChanged, controlledEntity.get(), streamIndex);
		}
	}
}

void ControllerImpl::onStreamOutputAudioMappingsChanged(UniqueIdentifier const /*entityID*/, entity::model::StreamIndex const /*streamIndex*/, entity::model::MapIndex const /*numberOfMaps*/, entity::model::MapIndex const /*mapIndex*/, entity::model::AudioMappings const& /*mappings*/) noexcept
{
}

void ControllerImpl::onStreamInputStarted(UniqueIdentifier const /*entityID*/, entity::model::StreamIndex const /*streamIndex*/) noexcept
{
}

void ControllerImpl::onStreamOutputStarted(UniqueIdentifier const /*entityID*/, entity::model::StreamIndex const /*streamIndex*/) noexcept
{
}

void ControllerImpl::onStreamInputStopped(UniqueIdentifier const /*entityID*/, entity::model::StreamIndex const /*streamIndex*/) noexcept
{
}

void ControllerImpl::onStreamOutputStopped(UniqueIdentifier const /*entityID*/, entity::model::StreamIndex const /*streamIndex*/) noexcept
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
		_controller->acquireEntity(targetEntityID, isPersistent, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const owningEntity)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("acquireEntity result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				switch (status)
				{
					case entity::ControllerEntity::AemCommandStatus::Success:
						updateAcquiredState(*controlledEntity, owningEntity);
						break;
					case entity::ControllerEntity::AemCommandStatus::AcquiredByOther:
						updateAcquiredState(*controlledEntity, owningEntity);
						break;
					case entity::ControllerEntity::AemCommandStatus::NotImplemented:
					case entity::ControllerEntity::AemCommandStatus::NotSupported:
						updateAcquiredState(*controlledEntity, getNullIdentifier());
						break;
					default:
						// In case of error, set the state to undefined
						updateAcquiredState(*controlledEntity, getUninitializedIdentifier(), true);
						break;
				}
				la::avdecc::invokeProtectedHandler(handler, controlledEntity.get(), status, owningEntity);
			}
			else // The entity went offline right after we sent our message
			{
				la::avdecc::invokeProtectedHandler(handler, nullptr, status, owningEntity);
			}
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity, getNullIdentifier());
	}
}

void ControllerImpl::releaseEntity(UniqueIdentifier const targetEntityID, ReleaseEntityHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("releaseEntity requested for ") + toHexString(targetEntityID, true));
		_controller->releaseEntity(targetEntityID, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const owningEntity)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("releaseEntity result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				if (status == entity::ControllerEntity::AemCommandStatus::Success) // Only change the acquire state in case of success
					updateAcquiredState(*controlledEntity, owningEntity);
				la::avdecc::invokeProtectedHandler(handler, controlledEntity.get(), status, owningEntity);
			}
			else // The entity went offline right after we sent our message
			{
				la::avdecc::invokeProtectedHandler(handler, nullptr, status, owningEntity);
			}
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity, getNullIdentifier());
	}
}

void ControllerImpl::setStreamInputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamInputFormat requested for ") + toHexString(targetEntityID, true));
		_controller->setStreamInputFormat(targetEntityID, streamIndex, streamFormat, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamInputFormat result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
				if (!!status)
					controlledEntity->setInputStreamFormat(streamIndex, streamFormat);
				la::avdecc::invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				la::avdecc::invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamOutputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamOutputFormat requested for ") + toHexString(targetEntityID, true));
		_controller->setStreamOutputFormat(targetEntityID, streamIndex, streamFormat, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("setStreamOutputFormat result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
#pragma message("TBD: Save the stream format, if !!status")
				la::avdecc::invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				la::avdecc::invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
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
					controlledEntity->addInputStreamAudioMappings(streamIndex, mappings);
				la::avdecc::invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				la::avdecc::invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::addStreamOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings, AddStreamOutputAudioMappingsHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("addStreamOutputAudioMappings requested for ") + toHexString(targetEntityID, true));
		_controller->addStreamOutputAudioMappings(targetEntityID, streamIndex, mappings, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const /*streamIndex*/, entity::model::AudioMappings const& /*mappings*/)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("addStreamOutputAudioMappings result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
#pragma message("TBD: Save the mappings, if !!status")
				la::avdecc::invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				la::avdecc::invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
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
					controlledEntity->removeInputStreamAudioMappings(streamIndex, mappings);
				la::avdecc::invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				la::avdecc::invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::removeStreamOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::AudioMappings const& mappings, RemoveStreamOutputAudioMappingsHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("removeStreamOutputAudioMappings requested for ") + toHexString(targetEntityID, true));
		_controller->removeStreamOutputAudioMappings(targetEntityID, streamIndex, mappings, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const /*streamIndex*/, entity::model::AudioMappings const& /*mappings*/)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("removeStreamOutputAudioMappings result for ") + toHexString(entityID, true) + ": " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
#pragma message("TBD: Save the mappings, if !!status")
				la::avdecc::invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				la::avdecc::invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::startStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("startStreamInput requested for ") + toHexString(targetEntityID, true) + ":" + std::to_string(streamIndex));
		_controller->startStreamInput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("startStreamInput result for ") + toHexString(entityID, true) + ":" + std::to_string(streamIndex) + " -> " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
#pragma message("TBD: Save the stream running state, if !!status")
				la::avdecc::invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				la::avdecc::invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::stopStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("stopStreamInput requested for ") + toHexString(targetEntityID, true) + ":" + std::to_string(streamIndex));
		_controller->stopStreamInput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("stopStreamInput result for ") + toHexString(entityID, true) + ":" + std::to_string(streamIndex) + " -> " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
#pragma message("TBD: Save the stream running state, if !!status")
				la::avdecc::invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				la::avdecc::invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::startStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("startStreamOutput requested for ") + toHexString(targetEntityID, true) + ":" + std::to_string(streamIndex));
		_controller->startStreamOutput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("startStreamOutput result for ") + toHexString(entityID, true) + ":" + std::to_string(streamIndex) + " -> " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
#pragma message("TBD: Save the stream running state, if !!status")
				la::avdecc::invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				la::avdecc::invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::stopStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept
{
	std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

	auto entityIt = _controlledEntities.find(targetEntityID);
	if (entityIt != _controlledEntities.end())
	{
		Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("stopStreamOutput requested for ") + toHexString(targetEntityID, true) + ":" + std::to_string(streamIndex));
		_controller->stopStreamOutput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)
		{
			Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("stopStreamOutput result for ") + toHexString(entityID, true) + ":" + std::to_string(streamIndex) + " -> " + entity::ControllerEntity::statusToString(status));
			std::lock_guard<decltype(_lockEntities)> const lg(_lockEntities); // Lock _controlledEntities

			auto entityIt = _controlledEntities.find(entityID);
			if (entityIt != _controlledEntities.end())
			{
				auto& controlledEntity = entityIt->second;
#pragma message("TBD: Save the stream running state, if !!status")
				la::avdecc::invokeProtectedHandler(handler, controlledEntity.get(), status);
			}
			else // The entity went offline right after we sent our message
			{
				la::avdecc::invokeProtectedHandler(handler, nullptr, status);
			}
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
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
			la::avdecc::invokeProtectedHandler(handler, talker, listener, talkerStreamIndex, listenerStreamIndex, controlStatus);
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, nullptr, entity::model::StreamIndex(0), entity::model::StreamIndex(0), entity::ControllerEntity::ControlStatus::UnknownEntity);
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
						la::avdecc::invokeProtectedHandler(handler, listener, listenerStreamIndex, controlStatus);
					});
				}
				else if (!!status) // No error, update the connection state
				{
					// Do not trust the connectionCount value to determine if the listener is disconnected, but rather use the status code (SUCCESS means disconnected)
					handleStreamStateNotification(listener, getNullIdentifier(), 0, listenerStreamIndex, false, flags, false);
				}
			}
			if (shouldNotifyHandler)
				la::avdecc::invokeProtectedHandler(handler, listener, listenerStreamIndex, controlStatus);
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, entity::model::StreamIndex(0), entity::ControllerEntity::ControlStatus::UnknownEntity);
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
			la::avdecc::invokeProtectedHandler(handler, talker, listener, talkerStreamIndex, listenerStreamIndex, connectionCount, flags, controlStatus);
		});
	}
	else
	{
		la::avdecc::invokeProtectedHandler(handler, nullptr, nullptr, entity::model::StreamIndex(0), entity::model::StreamIndex(0), uint16_t(0), entity::ConnectionFlags::None, entity::ControllerEntity::ControlStatus::UnknownEntity);
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
