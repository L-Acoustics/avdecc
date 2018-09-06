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
* @file avdeccControllerImplOverrides.cpp
* @author Christophe Calmejane
*/

#include "avdeccControllerImpl.hpp"
#include "avdeccControllerLogHelper.hpp"
#include "avdeccEntityModelCache.hpp"
#include "la/avdecc/internals/serialization.hpp"
#include <cstdlib> // free / malloc

namespace la
{
namespace avdecc
{
namespace controller
{

/* ************************************************************ */
/* Controller overrides                                         */
/* ************************************************************ */
ControllerImpl::ControllerImpl(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, UniqueIdentifier const entityModelID, std::string const& preferedLocale)
	: _preferedLocale(preferedLocale)
{
	try
	{
		_endStation = EndStation::create(protocolInterfaceType, interfaceName);
		_controller = _endStation->addControllerEntity(progID, entityModelID, this);
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
				AVDECC_ASSERT(false, "Unhandled exception");
				throw Exception(Error::InternalError, e.what());
		}
	}
	catch (Exception const& e)
	{
		AVDECC_ASSERT(false, "Unhandled exception");
		throw Exception(Error::InternalError, e.what());
	}

	// Create the delayed query thread
	_delayedQueryThread = std::thread([this]
	{
		setCurrentThreadName("avdecc::controller::DelayedQueries");
		decltype(_delayedQueries) queriesToSend{};
		while (!_shouldTerminate)
		{
			// Check all delayed queries if we need to send any of them, and copy them so we can send outside the loop
			{
				// Lock to protect _delayedQueries
				std::lock_guard<decltype(_lock)> const lg(_lock);

				// Get current time
				std::chrono::time_point<std::chrono::system_clock> currentTime = std::chrono::system_clock::now();

				for (auto it = _delayedQueries.begin(); it != _delayedQueries.end(); /* Iterate inside the loop */)
				{
					auto const& query = *it;
					if (currentTime > query.sendTime)
					{
						// Move the query to the "to process" list
						queriesToSend.emplace_back(std::move(*it));

						// Remove the command from the list
						it = _delayedQueries.erase(it);
					}
					else
					{
						++it;
					}
				}
			}

			// Now actually send queries, outside the lock
			while (!queriesToSend.empty() && !_shouldTerminate)
			{
				// Get first query from the list
				auto const& query = queriesToSend.front();

				auto controlledEntity = getControlledEntityImpl(query.entityID);

				// Entity still online
				if (controlledEntity)
				{
					// Send the query
					la::avdecc::invokeProtectedHandler(query.queryHandler, _controller);
				}

				// Remove the query from the list
				queriesToSend.pop_front();
			}

			// Wait a little bit so we don't burn the CPU
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	});
}

ControllerImpl::~ControllerImpl()
{
	// Notify the thread we are shutting down
	_shouldTerminate = true;

	// Wait for the thread to complete its pending tasks
	if (_delayedQueryThread.joinable())
		_delayedQueryThread.join();

	// First, remove ourself from the controller's delegate, we don't want notifications anymore (even if one is coming before the end of the destructor, it's not a big deal, _controlledEntities will be empty)
	_controller->setDelegate(nullptr);

	decltype(_controlledEntities) controlledEntities;

	// Move all controlled Entities (under lock), we don't want them to be accessible during destructor
	{
		// Lock to protect _controlledEntities
		std::lock_guard<decltype(_lock)> const lg(_lock);
		controlledEntities = std::move(_controlledEntities);
	}

	// Notify all entities they are going offline
	for (auto const& entityKV : controlledEntities)
	{
		auto const& entity = entityKV.second;
		if (entity->wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityOffline, this, entity.get());
		}
	}

	// Remove all observers, we don't want to trigger notifications for upcoming actions
	removeAllObservers();

	// Try to release all acquired entities by this controller before destroying everything
	for (auto const& entityKV : controlledEntities)
	{
		auto const& controlledEntity = entityKV.second;
		if (controlledEntity->isAcquired())
		{
			auto const& entityID = entityKV.first;
			_controller->releaseEntity(entityID, entity::model::DescriptorType::Entity, 0u, nullptr); // We don't need the result handler, let's just hope our message was properly sent and received!
		}
	}
}

void ControllerImpl::destroy() noexcept
{
	delete this;
}

UniqueIdentifier ControllerImpl::getControllerEID() const noexcept
{
	return _controller->getEntityID();
}

/* Controller configuration */
void ControllerImpl::enableEntityAdvertising(std::uint32_t const availableDuration)
{
	if (!_controller->enableEntityAdvertising(availableDuration))
		throw Exception(Error::DuplicateProgID, "Specified ProgID already in use on the local computer");
	LOG_CONTROLLER_INFO(_controller->getEntityID(), "Controller advertising enabled");
}

void ControllerImpl::disableEntityAdvertising() noexcept
{
	_controller->disableEntityAdvertising();
	LOG_CONTROLLER_INFO(_controller->getEntityID(), "Controller advertising disabled");
}

void ControllerImpl::enableEntityModelCache() noexcept
{
	EntityModelCache::getInstance().enableCache();
	LOG_CONTROLLER_INFO(_controller->getEntityID(), "AEM Cache enabled");
}

void ControllerImpl::disableEntityModelCache() noexcept
{
	EntityModelCache::getInstance().disableCache();
	LOG_CONTROLLER_INFO(_controller->getEntityID(), "AEM Cache disabled");
}

/* Enumeration and Control Protocol (AECP) */
void ControllerImpl::acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, AcquireEntityHandler const& handler) const noexcept
{
	auto const descriptorType{ entity::model::DescriptorType::Entity };
	auto const descriptorIndex{ entity::model::DescriptorIndex{0u} };

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User acquireEntity (isPersistent={} DescriptorType={} DescriptorIndex={})", isPersistent, to_integral(descriptorType), descriptorIndex);

		// Already acquired or acquiring, don't do anything (we want to try to acquire if it's flagged as acquired by another controller, in case it went offline without notice)
		if (controlledEntity->isAcquired() || controlledEntity->isAcquiring())
		{
			LOG_CONTROLLER_TRACE(targetEntityID, "User acquireEntity not sent because entity is {}", (controlledEntity->isAcquired() ? "already acquired" : "being acquired"));
			return;
		}
		controlledEntity->setAcquireState(model::AcquireState::TryAcquire);
		_controller->acquireEntity(targetEntityID, isPersistent, descriptorType, descriptorIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User acquireEntityResult (OwningController={} DescriptorType={} DescriptorIndex={}): {}", toHexString(owningEntity, true), to_integral(descriptorType), descriptorIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				switch (status)
				{
					case entity::ControllerEntity::AemCommandStatus::Success:
						updateAcquiredState(*entity, owningEntity, descriptorType, descriptorIndex);
						break;
					case entity::ControllerEntity::AemCommandStatus::AcquiredByOther:
						updateAcquiredState(*entity, owningEntity, descriptorType, descriptorIndex);
						break;
					case entity::ControllerEntity::AemCommandStatus::NotImplemented:
					case entity::ControllerEntity::AemCommandStatus::NotSupported:
						updateAcquiredState(*entity, UniqueIdentifier{}, descriptorType, descriptorIndex);
						break;
					default:
						// In case of error, set the state to undefined
						updateAcquiredState(*entity, UniqueIdentifier{}, descriptorType, descriptorIndex, true);
						break;
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status, owningEntity);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status, owningEntity);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity, UniqueIdentifier::getNullUniqueIdentifier());
	}
}

void ControllerImpl::releaseEntity(UniqueIdentifier const targetEntityID, ReleaseEntityHandler const& handler) const noexcept
{
	auto const descriptorType{ entity::model::DescriptorType::Entity };
	auto const descriptorIndex{ entity::model::DescriptorIndex{ 0u } };

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User releaseEntity (DescriptorType={} DescriptorIndex={})", to_integral(descriptorType), descriptorIndex);
		_controller->releaseEntity(targetEntityID, descriptorType, descriptorIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User releaseEntity (OwningController={} DescriptorType={} DescriptorIndex={}): {}", toHexString(owningEntity, true), to_integral(descriptorType), descriptorIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the acquire state in case of success
				{
					updateAcquiredState(*entity, owningEntity, descriptorType, descriptorIndex);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status, owningEntity);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status, owningEntity);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity, UniqueIdentifier::getNullUniqueIdentifier());
	}
}

void ControllerImpl::setConfiguration(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setConfiguration (ConfigurationIndex={})", configurationIndex);
		_controller->setConfiguration(targetEntityID, configurationIndex, [this, handler](entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setConfiguration (ConfigurationIndex={}): {}", configurationIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status)
				{
					updateConfiguration(controller, *entity, configurationIndex);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setStreamInputFormat (StreamIndex={} streamFormat={})", streamIndex, streamFormat);
		_controller->setStreamInputFormat(targetEntityID, streamIndex, streamFormat, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setStreamInputFormat (StreamIndex={} streamFormat={}): {}", streamIndex, streamFormat, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status)
				{
					updateStreamInputFormat(*entity, streamIndex, streamFormat);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setStreamOutputFormat (StreamIndex={} streamFormat={})", streamIndex, streamFormat);
		_controller->setStreamOutputFormat(targetEntityID, streamIndex, streamFormat, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setStreamOutputFormat (StreamIndex={} streamFormat={}): {}", streamIndex, streamFormat, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status)
				{
					updateStreamOutputFormat(*entity, streamIndex, streamFormat);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setEntityName (Name={})", name.str());
		_controller->setEntityName(targetEntityID, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setEntityName (): {}", entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the name in case of success
				{
					updateEntityName(*entity, name);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setEntityGroupName (Name={})", name.str());
		_controller->setEntityGroupName(targetEntityID, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setEntityGroupName (): {}", entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the name in case of success
				{
					updateEntityGroupName(*entity, name);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setConfigurationName (ConfigurationIndex={} Name={})", configurationIndex, name.str());
		_controller->setConfigurationName(targetEntityID, configurationIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setConfigurationName (ConfigurationIndex={}): {}", configurationIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the name in case of success
				{
					updateConfigurationName(*entity, configurationIndex, name);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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

void ControllerImpl::setAudioUnitName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& name, SetAudioUnitNameHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setAudioUnitName (ConfigurationIndex={} AudioUnitIndex={} Name={})", configurationIndex, audioUnitIndex, name.str());
		_controller->setAudioUnitName(targetEntityID, configurationIndex, audioUnitIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setAudioUnitName (ConfigurationIndex={} AudioUnitIndex={}): {}", configurationIndex, audioUnitIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the name in case of success
				{
					updateAudioUnitName(*entity, configurationIndex, audioUnitIndex, name);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setStreamInputName (ConfigurationIndex={} StreamIndex={} Name={})", configurationIndex, streamIndex, name.str());
		_controller->setStreamInputName(targetEntityID, configurationIndex, streamIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setStreamInputName (ConfigurationIndex={} StreamIndex={}): {}", configurationIndex, streamIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the name in case of success
				{
					updateStreamInputName(*entity, configurationIndex, streamIndex, name);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setStreamOutputName (ConfigurationIndex={} StreamIndex={} Name={})", configurationIndex, streamIndex, name.str());
		_controller->setStreamOutputName(targetEntityID, configurationIndex, streamIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setStreamOutputName (ConfigurationIndex={} StreamIndex={}): {}", configurationIndex, streamIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the name in case of success
				{
					updateStreamOutputName(*entity, configurationIndex, streamIndex, name);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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

void ControllerImpl::setAvbInterfaceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& name, SetAvbInterfaceNameHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setAvbInterfaceName (ConfigurationIndex={} AvbInterfaceIndex={} Name={})", configurationIndex, avbInterfaceIndex, name.str());
		_controller->setAvbInterfaceName(targetEntityID, configurationIndex, avbInterfaceIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setAvbInterfaceName (ConfigurationIndex={} AvbInterfaceIndex={}): {}", configurationIndex, avbInterfaceIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the name in case of success
				{
					updateAvbInterfaceName(*entity, configurationIndex, avbInterfaceIndex, name);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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

void ControllerImpl::setClockSourceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& name, SetClockSourceNameHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setClockSourceName (ConfigurationIndex={} ClockSourceIndex={} Name={})", configurationIndex, clockSourceIndex, name.str());
		_controller->setClockSourceName(targetEntityID, configurationIndex, clockSourceIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setClockSourceName (ConfigurationIndex={} ClockSourceIndex={}): {}", configurationIndex, clockSourceIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the name in case of success
				{
					updateClockSourceName(*entity, configurationIndex, clockSourceIndex, name);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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

void ControllerImpl::setMemoryObjectName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& name, SetMemoryObjectNameHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setMemoryObjectName (ConfigurationIndex={} MemoryObjectIndex={} Name={})", configurationIndex, memoryObjectIndex, name.str());
		_controller->setMemoryObjectName(targetEntityID, configurationIndex, memoryObjectIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setMemoryObjectName (ConfigurationIndex={} MemoryObjectIndex={}): {}", configurationIndex, memoryObjectIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the name in case of success
				{
					updateMemoryObjectName(*entity, configurationIndex, memoryObjectIndex, name);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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

void ControllerImpl::setAudioClusterName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& name, SetAudioClusterNameHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setAudioClusterName (ConfigurationIndex={} AudioClusterIndex={} Name={})", configurationIndex, audioClusterIndex, name.str());
		_controller->setAudioClusterName(targetEntityID, configurationIndex, audioClusterIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setAudioClusterName (ConfigurationIndex={} AudioClusterIndex={}): {}", configurationIndex, audioClusterIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the name in case of success
				{
					updateAudioClusterName(*entity, configurationIndex, audioClusterIndex, name);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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

void ControllerImpl::setClockDomainName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& name, SetClockDomainNameHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setClockDomainName (ConfigurationIndex={} ClockDomainIndex={} Name={})", configurationIndex, clockDomainIndex, name.str());
		_controller->setClockDomainName(targetEntityID, configurationIndex, clockDomainIndex, name, [this, name, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setClockDomainName (ConfigurationIndex={} ClockDomainIndex={}): {}", configurationIndex, clockDomainIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the name in case of success
				{
					updateClockDomainName(*entity, configurationIndex, clockDomainIndex, name);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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

void ControllerImpl::setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setAudioUnitSamplingRate (AudioUnitIndex={} SamplingRate={})", audioUnitIndex, samplingRate);
		_controller->setAudioUnitSamplingRate(targetEntityID, audioUnitIndex, samplingRate, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setAudioUnitSamplingRate (AudioUnitIndex={} SamplingRate={}): {}", audioUnitIndex, samplingRate, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the sampling rate in case of success
				{
					updateAudioUnitSamplingRate(*entity, audioUnitIndex, samplingRate);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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

void ControllerImpl::setClockSource(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setClockSource (ClockDomainIndex={} ClockSourceIndex={})", clockDomainIndex, clockSourceIndex);
		_controller->setClockSource(targetEntityID, clockDomainIndex, clockSourceIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setClockSource (ClockDomainIndex={} ClockSourceIndex={}): {}", clockDomainIndex, clockSourceIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the clock source in case of success
				{
					updateClockSource(*entity, clockDomainIndex, clockSourceIndex);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User startStreamInput (StreamIndex={})", streamIndex);
		_controller->startStreamInput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User startStreamInput (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the running status in case of success
				{
					updateStreamInputRunningStatus(*entity, streamIndex, true);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User stopStreamInput (StreamIndex={})", streamIndex);
		_controller->stopStreamInput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User stopStreamInput (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the running status in case of success
				{
					updateStreamInputRunningStatus(*entity, streamIndex, false);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User startStreamOutput (StreamIndex={})", streamIndex);
		_controller->startStreamOutput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User startStreamOutput (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the running status in case of success
				{
					updateStreamOutputRunningStatus(*entity, streamIndex, true);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User stopStreamOutput (StreamIndex={})", streamIndex);
		_controller->stopStreamOutput(targetEntityID, streamIndex, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
		{
			LOG_CONTROLLER_TRACE(entityID, "User stopStreamOutput (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status) // Only change the running status in case of success
				{
					updateStreamOutputRunningStatus(*entity, streamIndex, false);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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

void ControllerImpl::addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User addStreamInputAudioMappings (StreamPortIndex={})", streamPortIndex); // TODO: Convert mappings to string and add to log
		_controller->addStreamPortInputAudioMappings(targetEntityID, streamPortIndex, mappings, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
		{
			LOG_CONTROLLER_TRACE(entityID, "User addStreamInputAudioMappings (StreamPortIndex={}): {}", streamPortIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status)
				{
					updateStreamPortInputAudioMappingsAdded(*entity, streamPortIndex, mappings);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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

void ControllerImpl::addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User addStreamOutputAudioMappings (StreamPortIndex={})", streamPortIndex);
		_controller->addStreamPortOutputAudioMappings(targetEntityID, streamPortIndex, mappings, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
		{
			LOG_CONTROLLER_TRACE(entityID, "User addStreamOutputAudioMappings (StreamPortIndex={}): {}", streamPortIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status)
				{
					updateStreamPortOutputAudioMappingsAdded(*entity, streamPortIndex, mappings);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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

void ControllerImpl::removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User removeStreamInputAudioMappings (StreamPortIndex={})", streamPortIndex);
		_controller->removeStreamPortInputAudioMappings(targetEntityID, streamPortIndex, mappings, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
		{
			LOG_CONTROLLER_TRACE(entityID, "User removeStreamInputAudioMappings (StreamPortIndex={}): {}", streamPortIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status)
				{
					updateStreamPortInputAudioMappingsRemoved(*entity, streamPortIndex, mappings);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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

void ControllerImpl::removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User removeStreamOutputAudioMappings (StreamPortIndex={})", streamPortIndex);
		_controller->removeStreamPortOutputAudioMappings(targetEntityID, streamPortIndex, mappings, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
		{
			LOG_CONTROLLER_TRACE(entityID, "User removeStreamOutputAudioMappings (StreamPortIndex={}): {}", streamPortIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status)
				{
					updateStreamPortOutputAudioMappingsRemoved(*entity, streamPortIndex, mappings);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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

void ControllerImpl::setMemoryObjectLength(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, SetMemoryObjectLengthHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setMemoryObjectLength (ConfigurationIndex={} MemoryObjectIndex={} Length={})", configurationIndex, memoryObjectIndex, length);
		_controller->setMemoryObjectLength(targetEntityID, configurationIndex, memoryObjectIndex, length, [this, handler](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length)
		{
			LOG_CONTROLLER_TRACE(entityID, "User setMemoryObjectLength (ConfigurationIndex={} MemoryObjectIndex={}): {}", configurationIndex, memoryObjectIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				if (!!status)
				{
					updateMemoryObjectLength(*entity, configurationIndex, memoryObjectIndex, length);
				}
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
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

entity::addressAccess::Tlv ControllerImpl::makeNextReadDeviceMemoryTlv(std::uint64_t const baseAddress, std::uint64_t const length, std::uint64_t const currentSize) const noexcept
{
	try
	{
		if (currentSize < length)
		{
			auto const remaining = length - currentSize;
			auto const nextQuerySize = static_cast<size_t>(remaining > la::avdecc::protocol::AaAecpMaxSingleTlvMemoryDataLength ? la::avdecc::protocol::AaAecpMaxSingleTlvMemoryDataLength : remaining);
			return entity::addressAccess::Tlv{ baseAddress + currentSize, nextQuerySize };
		}
	}
	catch (...)
	{
	}
	return entity::addressAccess::Tlv{};
}

entity::addressAccess::Tlv ControllerImpl::makeNextWriteDeviceMemoryTlv(std::uint64_t const baseAddress, DeviceMemoryBuffer const& memoryBuffer, std::uint64_t const currentSize) const noexcept
{
	try
	{
		auto const length = memoryBuffer.size();
		if (currentSize < length)
		{
			auto const remaining = length - currentSize;
			auto const nextQuerySize = static_cast<size_t>(remaining > la::avdecc::protocol::AaAecpMaxSingleTlvMemoryDataLength ? la::avdecc::protocol::AaAecpMaxSingleTlvMemoryDataLength : remaining);
			return entity::addressAccess::Tlv{ baseAddress + currentSize, protocol::AaMode::Write, memoryBuffer.data() + currentSize, nextQuerySize };
		}
	}
	catch (...)
	{
	}
	return entity::addressAccess::Tlv{};
}

void ControllerImpl::onUserReadDeviceMemoryResult(UniqueIdentifier const targetEntityID, entity::ControllerEntity::AaCommandStatus const status, entity::addressAccess::Tlvs const& tlvs, std::uint64_t const baseAddress, std::uint64_t const length, ReadDeviceMemoryHandler&& handler, DeviceMemoryBuffer&& memoryBuffer) const noexcept
{
	LOG_CONTROLLER_TRACE(targetEntityID, "User readDeviceMemory chunk (BaseAddress={} Length={}): {}", baseAddress, length, entity::ControllerEntity::statusToString(status));
	if (!!status)
	{
		// Copy the TLV data to the memory buffer
		for (auto const& tlv : tlvs)
		{
			auto const& tlvData = tlv.getMemoryData();
			memoryBuffer.append(tlvData.data(), tlvData.size());
		}

		// Check if we need to query another portion of the device memory
		auto tlv = makeNextReadDeviceMemoryTlv(baseAddress, length, memoryBuffer.size());
		if (tlv)
		{
			LOG_CONTROLLER_TRACE(targetEntityID, "User readDeviceMemory chunk (BaseAddress={}, Length={}, Pos={}, ChunkLength={})", baseAddress, length, tlv.getAddress() - baseAddress, tlv.size());
			_controller->addressAccess(targetEntityID, { std::move(tlv) }, [this, baseAddress, length, handler = std::move(handler), memoryBuffer = std::move(memoryBuffer)](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AaCommandStatus const status, entity::addressAccess::Tlvs const& tlvs) mutable
			{
				onUserReadDeviceMemoryResult(entityID, status, tlvs, baseAddress, length, std::move(handler), std::move(memoryBuffer));
			});
			return;
		}
	}
	else
	{
		memoryBuffer.clear();
	}

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status, memoryBuffer);
	}
	else // The entity went offline right after we sent our message
	{
		invokeProtectedHandler(handler, nullptr, status, memoryBuffer);
	}
}

void ControllerImpl::onUserWriteDeviceMemoryResult(UniqueIdentifier const targetEntityID, entity::ControllerEntity::AaCommandStatus const status, std::uint64_t const baseAddress, std::uint64_t const sentSize, WriteDeviceMemoryHandler&& handler, DeviceMemoryBuffer&& memoryBuffer) const noexcept
{
	LOG_CONTROLLER_TRACE(targetEntityID, "User writeDeviceMemory chunk (BaseAddress={} Length={}): {}", baseAddress, memoryBuffer.size(), entity::ControllerEntity::statusToString(status));
	if (!!status)
	{
		// Check if we need to send another portion of the device memory
		auto tlv = makeNextWriteDeviceMemoryTlv(baseAddress, memoryBuffer, sentSize);
		if (tlv)
		{
			_controller->addressAccess(targetEntityID, { std::move(tlv) }, [this, baseAddress, sentSize = sentSize + tlv.size(), handler = std::move(handler), memoryBuffer = std::move(memoryBuffer)](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AaCommandStatus const status, entity::addressAccess::Tlvs const& /*tlvs*/) mutable
			{
				onUserWriteDeviceMemoryResult(entityID, status, baseAddress, sentSize, std::move(handler), std::move(memoryBuffer));
			});
			return;
		}
	}

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
	}
	else // The entity went offline right after we sent our message
	{
		invokeProtectedHandler(handler, nullptr, status);
	}
}

void ControllerImpl::readDeviceMemory(UniqueIdentifier const targetEntityID, std::uint64_t const address, std::uint64_t const length, ReadDeviceMemoryHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		DeviceMemoryBuffer memoryBuffer{};
#pragma message("TODO: Find a way to have the DeviceMemoryBuffer being properly moved all the way through the lambdas and handlers. Currently some handlers are copied, so the DeviceMemoryBuffer is copied instead of being moved causing unecessary reallocations")
		memoryBuffer.reserve(static_cast<size_t>(length));

		auto tlv = makeNextReadDeviceMemoryTlv(address, length, 0u);
		if (tlv)
		{
			LOG_CONTROLLER_TRACE(targetEntityID, "User readDeviceMemory chunk (BaseAddress={}, Length={}, Pos={}, ChunkLength={})", address, length, 0, tlv.size());
			_controller->addressAccess(targetEntityID, { std::move(tlv) }, [this, baseAddress = address, length, handlerCopy = handler, memoryBuffer = std::move(memoryBuffer)](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AaCommandStatus const status, entity::addressAccess::Tlvs const& tlvs) mutable
			{
				onUserReadDeviceMemoryResult(entityID, status, tlvs, baseAddress, length, std::move(handlerCopy), std::move(memoryBuffer));
			});
		}
		else
		{
			invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AaCommandStatus::TlvInvalid, DeviceMemoryBuffer{});
		}
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AaCommandStatus::UnknownEntity, DeviceMemoryBuffer{});
	}
}

void ControllerImpl::startOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const operationID, entity::model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, StartOperationHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User startOperation (DescriptorType={}, DescriptorIndex={}, OperationID={}, OperationType={})", static_cast<std::uint16_t>(descriptorType), descriptorIndex, operationID, static_cast<std::uint16_t>(operationType));

		_controller->startOperation(targetEntityID, descriptorType, descriptorIndex, operationID, operationType, memoryBuffer, [this, handler](la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/, [[maybe_unused]] std::uint16_t const operationID, la::avdecc::entity::model::MemoryObjectOperationType const /*operationType*/, MemoryBuffer const& memoryBuffer)
		{
			(void)operationID; // Because of a bug in VS 15.7 that doesn't take into account [[maybe_unused]] for lambda parameters
			LOG_CONTROLLER_TRACE(entityID, "User startOperation (operationID={}): {}", operationID, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto controlledEntity = getControlledEntityImpl(entityID);

			if (controlledEntity)
			{
				auto* const entity = controlledEntity.get();
				invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status, memoryBuffer);
			}
			else // The entity went offline right after we sent our message
			{
				invokeProtectedHandler(handler, nullptr, status, memoryBuffer);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity, MemoryBuffer{});
	}
}

void ControllerImpl::startUploadOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint64_t const dataLength, StartOperationHandler const& handler) const
{
#pragma message("TODO: Modify the Serializer/Deserializer classes so they can use a provided buffer (MemoryBuffer), instead of always using a static internal buffer. Template the class so the container is that!")
	Serializer<sizeof(dataLength)> ser{};

	ser << dataLength;

	MemoryBuffer buffer{ ser.data(), ser.usedBytes() };
	startOperation(targetEntityID, descriptorType, descriptorIndex, 0, entity::model::MemoryObjectOperationType::Upload, buffer, handler);
}

void ControllerImpl::writeDeviceMemory(UniqueIdentifier const targetEntityID, std::uint64_t const address, DeviceMemoryBuffer memoryBuffer, WriteDeviceMemoryHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(targetEntityID);

	if (controlledEntity)
	{
#pragma message("TODO: Find a way to have the DeviceMemoryBuffer being properly moved all the way through the lambdas and handlers. Currently some handlers are copied, so the DeviceMemoryBuffer is copied instead of being moved causing unecessary allocations")
		auto tlv = makeNextWriteDeviceMemoryTlv(address, memoryBuffer, 0u);
		if (tlv)
		{
			LOG_CONTROLLER_TRACE(targetEntityID, "User writeDeviceMemory chunk (BaseAddress={}, Length={}, Pos={}, ChunkLength={})", address, memoryBuffer.size(), 0, tlv.size());
			_controller->addressAccess(targetEntityID, { std::move(tlv) }, [this, baseAddress = address, sentSize = tlv.size(), handlerCopy = handler, memoryBuffer = std::move(memoryBuffer)](entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AaCommandStatus const status, entity::addressAccess::Tlvs const& /*tlvs*/) mutable
			{
				onUserWriteDeviceMemoryResult(entityID, status, baseAddress, sentSize, std::move(handlerCopy), std::move(memoryBuffer));
			});
		}
		else
		{
			invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AaCommandStatus::TlvInvalid);
		}
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AaCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::connectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(listenerStream.entityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User connectStream (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={})", talkerStream.entityID.getValue(), talkerStream.streamIndex, listenerStream.entityID.getValue(), listenerStream.streamIndex);
		_controller->connectStream(talkerStream, listenerStream, [this, handler](entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
		{
			LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User connectStream (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={}): {}", talkerStream.entityID.getValue(), talkerStream.streamIndex, listenerStream.entityID.getValue(), listenerStream.streamIndex, entity::ControllerEntity::statusToString(status));

			if (!!status)
			{
				// Do not trust the connectionCount value to determine if the listener is connected, but rather use the status code (SUCCESS means connection is established)
				handleListenerStreamStateNotification(talkerStream, listenerStream, true, flags, false);
			}

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto listener = getControlledEntityImpl(listenerStream.entityID);
			auto talker = getControlledEntityImpl(talkerStream.entityID);
			invokeProtectedHandler(handler, talker.get(), listener.get(), talkerStream.streamIndex, listenerStream.streamIndex, status);
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, nullptr, entity::model::StreamIndex(0), entity::model::StreamIndex(0), entity::ControllerEntity::ControlStatus::UnknownEntity);
	}
}

void ControllerImpl::disconnectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(listenerStream.entityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User disconnectStream (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={})", talkerStream.entityID.getValue(), talkerStream.streamIndex, listenerStream.entityID.getValue(), listenerStream.streamIndex);
		_controller->disconnectStream(talkerStream, listenerStream, [this, handler](entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
		{
			LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User disconnectStream (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={}): {}", talkerStream.entityID.getValue(), talkerStream.streamIndex, listenerStream.entityID.getValue(), listenerStream.streamIndex, entity::ControllerEntity::statusToString(status));

			bool shouldNotifyHandler{ true }; // Shall we notify the handler right now, or do we have to send another message before

			if (!!status) // No error, update the connection state
			{
				// Do not trust the connectionCount value to determine if the listener is disconnected, but rather use the status code (SUCCESS means disconnected)
				handleListenerStreamStateNotification(talkerStream, listenerStream, false, flags, false);
			}
			else
			{
				// In case of a disconnect we might get an error (forwarded from the talker) but the stream is actually disconnected.
				// In that case, we have to query the listener stream state in order to know the actual connection state
				if (status != entity::ControllerEntity::ControlStatus::NotConnected)
				{
					shouldNotifyHandler = false; // Don't notify handler right now, wait for getListenerStreamState answer
					_controller->getListenerStreamState(listenerStream, [this, handler, disconnectStatus = status](entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
					{
						entity::ControllerEntity::ControlStatus controlStatus{ disconnectStatus };

						if (!!status)
						{
							// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
							bool const isStillConnected = connectionCount != 0;
							handleListenerStreamStateNotification(talkerStream, listenerStream, isStillConnected, flags, false);
							// Status to return depends if we actually got disconnected (success in that case)
							controlStatus = isStillConnected ? disconnectStatus : entity::ControllerEntity::ControlStatus::Success;
						}

						// Take a copy of the ControlledEntity so we don't have to keep the lock
						auto listener = getControlledEntityImpl(listenerStream.entityID);
						invokeProtectedHandler(handler, listener.get(), listenerStream.streamIndex, controlStatus);
					});
				}
			}

			if (shouldNotifyHandler)
			{
				// Take a copy of the ControlledEntity so we don't have to keep the lock
				auto listener = getControlledEntityImpl(listenerStream.entityID);
				invokeProtectedHandler(handler, listener.get(), listenerStream.streamIndex, status);
			}
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, entity::model::StreamIndex(0), entity::ControllerEntity::ControlStatus::UnknownEntity);
	}
}

void ControllerImpl::disconnectTalkerStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(talkerStream.entityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User disconnectTalkerStream (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={})", talkerStream.entityID.getValue(), talkerStream.streamIndex, listenerStream.entityID.getValue(), listenerStream.streamIndex);
		_controller->disconnectTalkerStream(talkerStream, listenerStream, [this, handler](entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
		{
			LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User disconnectTalkerStream (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={}): {}", talkerStream.entityID.getValue(), talkerStream.streamIndex, listenerStream.entityID.getValue(), listenerStream.streamIndex, entity::ControllerEntity::statusToString(status));

			auto st = status;
			if (st == entity::ControllerEntity::ControlStatus::NotConnected)
			{
				st = entity::ControllerEntity::ControlStatus::Success;
			}
			if (!!status) // No error, update the connection state
			{
				// Do not trust the connectionCount value to determine if the listener is disconnected, but rather use the status code (SUCCESS means disconnected)
				handleTalkerStreamStateNotification(talkerStream, listenerStream, false, flags, true);
			}

			invokeProtectedHandler(handler, st);
		});
	}
	else
	{
		invokeProtectedHandler(handler, entity::ControllerEntity::ControlStatus::UnknownEntity);
	}
}

void ControllerImpl::getListenerStreamState(entity::model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(listenerStream.entityID);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User getListenerStreamState (ListenerID={} ListenerIndex={})", listenerStream.entityID.getValue(), listenerStream.streamIndex);
		_controller->getListenerStreamState(listenerStream, [this, handler](entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
		{
			LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User getListenerStreamState (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={}): {}", talkerStream.entityID.getValue(), talkerStream.streamIndex, listenerStream.entityID.getValue(), listenerStream.streamIndex, entity::ControllerEntity::statusToString(status));

			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto listener = getControlledEntityImpl(listenerStream.entityID);
			auto talker = getControlledEntityImpl(talkerStream.entityID);

			if (!!status)
			{
				// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
				handleListenerStreamStateNotification(talkerStream, listenerStream, connectionCount != 0, flags, false);
			}

			invokeProtectedHandler(handler, talker.get(), listener.get(), talkerStream.streamIndex, listenerStream.streamIndex, connectionCount, flags, status);
		});
	}
	else
	{
		invokeProtectedHandler(handler, nullptr, nullptr, entity::model::StreamIndex(0), entity::model::StreamIndex(0), uint16_t(0), entity::ConnectionFlags::None, entity::ControllerEntity::ControlStatus::UnknownEntity);
	}
}

ControlledEntityGuard ControllerImpl::getControlledEntity(UniqueIdentifier const entityID) const noexcept
{
	auto entity = getControlledEntityImpl(entityID);
	if (entity && entity->wasAdvertised())
	{
		return ControlledEntityGuard{ std::move(entity) };
	}
	return{};
}

void ControllerImpl::lock() noexcept
{
	_controller->lock();
}

void ControllerImpl::unlock() noexcept
{
	_controller->unlock();
}

} // namespace controller
} // namespace avdecc
} // namespace la
