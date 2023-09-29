/*
* Copyright (C) 2016-2023, L-Acoustics and its contributors

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
* @file avdeccControllerImplOverrides.cpp
* @author Christophe Calmejane
*/

#include "avdeccControllerImpl.hpp"
#include "avdeccControllerLogHelper.hpp"
#include "avdeccEntityModelCache.hpp"
#ifdef ENABLE_AVDECC_FEATURE_JSON
#	include "avdeccControllerJsonTypes.hpp"
#	include "avdeccControlledEntityJsonSerializer.hpp"
#endif // ENABLE_AVDECC_FEATURE_JSON

#ifdef ENABLE_AVDECC_FEATURE_JSON
#	include <la/avdecc/internals/jsonTypes.hpp>
#endif // ENABLE_AVDECC_FEATURE_JSON
#include <la/avdecc/internals/serialization.hpp>
#include <la/avdecc/internals/protocolAemPayloadSizes.hpp>
#include <la/avdecc/executor.hpp>

#include <cstdlib> // free / malloc
#include <cstring> // strerror
#include <cerrno> // errno
#include <unordered_set>
#include <set>
#include <fstream>
#include <mutex>
#include <memory>

namespace la
{
namespace avdecc
{
namespace controller
{
/* ************************************************************ */
/* Controller overrides                                         */
/* ************************************************************ */
ControllerImpl::ControllerImpl(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, UniqueIdentifier const entityModelID, std::string const& preferedLocale, entity::model::EntityTree const* const entityModelTree, std::optional<std::string> const& executorName, entity::controller::Interface const* const virtualEntityInterface)
	: _preferedLocale(preferedLocale)
{
	try
	{
		_endStation = EndStation::create(protocolInterfaceType, interfaceName, executorName);
		_controller = _endStation->addControllerEntity(progID, entityModelID, entityModelTree, this);
		_controllerProxy = std::make_unique<ControllerVirtualProxy>(_endStation->getProtocolInterface(), _controller, virtualEntityInterface);
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
			case EndStation::Error::DuplicateEntityID:
				throw Exception(Error::DuplicateProgID, e.what());
			case EndStation::Error::InvalidEntityModel:
				throw Exception(Error::InvalidEntityModel, e.what());
			case EndStation::Error::DuplicateExecutorName:
				throw Exception(Error::DuplicateExecutorName, e.what());
			case EndStation::Error::UnknownExecutorName:
				throw Exception(Error::UnknownExecutorName, e.what());
			case EndStation::Error::InternalError:
				throw Exception(Error::InternalError, e.what());
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

	// Create the StateMachines thread
	_stateMachinesThread = std::thread(
		[this]
		{
			utils::setCurrentThreadName("avdecc::controller::StateMachines");
			auto entityIdentificationsStopped = std::unordered_set<UniqueIdentifier, UniqueIdentifier::hash>{};
			decltype(_controllerIdentifications) controllerIdentificationsStopped{};
			decltype(_delayedQueries) queriesToSend{};
			while (!_shouldTerminate)
			{
				// Entity Identification
				{
					// Check all ongoing identifications if we didn't receive any new message for some time, and copy them so we can notify outside the loop
					{
						// Lock to protect _entityIdentifications and _controllerIdentifications
						auto const lg = std::lock_guard{ _lock };

						// Get current time
						auto const currentTime = std::chrono::system_clock::now();

						// Check Entity to Controller Identification expiration
						for (auto it = _entityIdentifications.begin(); it != _entityIdentifications.end(); /* Iterate inside the loop */)
						{
							auto const entityID = it->first;
							auto const lastNotificationTime = it->second;

							if (currentTime > (lastNotificationTime + std::chrono::milliseconds(1200)))
							{
								// Move the entityID to the "to process" list
								entityIdentificationsStopped.insert(entityID);

								// Remove the entity from the list
								it = _entityIdentifications.erase(it);
							}
							else
							{
								++it;
							}
						}

						// Check Controller to Entity Identification expiration
						for (auto it = _controllerIdentifications.begin(); it != _controllerIdentifications.end(); /* Iterate inside the loop */)
						{
							auto& [entityID, state] = *it;

							if (currentTime > state.expireTime)
							{
								// Move to the "to process" list
								controllerIdentificationsStopped[entityID] = std::move(state);

								// Remove the entity from the list
								it = _controllerIdentifications.erase(it);
							}
							else
							{
								++it;
							}
						}
					}

					// Now actually process expirations, outside the lock
					for (auto const& entityID : entityIdentificationsStopped)
					{
						if (_shouldTerminate)
						{
							break;
						}

						// Take a "scoped locked" shared copy of the ControlledEntity
						auto controlledEntity = getControlledEntityImplGuard(entityID, true);

						// Entity still online
						if (controlledEntity)
						{
							// Notify
							notifyObserversMethod<Controller::Observer>(&Controller::Observer::onIdentificationStopped, this, controlledEntity.get());
						}
					}
					for (auto const& [entityID, state] : controllerIdentificationsStopped)
					{
						if (_shouldTerminate)
						{
							break;
						}

						// Never lock the ControlledEntities before calling the controller
						// Disable Identification
						_controllerProxy->setControlValues(entityID, state.controlIndex, makeIdentifyControlValues(false),
							[this](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ControlIndex const controlIndex, MemoryBuffer const& packedControlValues)
							{
								// Take a "scoped locked" shared copy of the ControlledEntity
								auto controlledEntity = getControlledEntityImplGuard(entityID);

								if (controlledEntity)
								{
									// Update source
									if (!!status) // Only change the control values in case of success
									{
										updateControlValues(*controlledEntity, controlIndex, packedControlValues, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
									}
								}
							});
					}

					// Clear the lists
					entityIdentificationsStopped.clear();
					controllerIdentificationsStopped.clear();
				}

				// Delayed Queries
				{
					// Check all delayed queries if we need to send any of them, and copy them so we can send outside the loop
					{
						// Lock to protect _delayedQueries
						auto const lg = std::lock_guard{ _lock };

						// Get current time
						auto const currentTime = std::chrono::system_clock::now();

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

						// Get a shared copy of the ControlledEntity so it stays alive while in the scope
						auto controlledEntity = getSharedControlledEntityImplHolder(query.entityID);

						// Entity still online
						if (controlledEntity)
						{
							// Send the query
							utils::invokeProtectedHandler(query.queryHandler, _controller);
						}

						// Remove the query from the list
						queriesToSend.pop_front();
					}
				}

				// Wait a little bit so we don't burn the CPU
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		});
}

void ControllerImpl::unregisterExclusiveAccessToken(UniqueIdentifier const entityID, ExclusiveAccessTokenImpl* const token) const noexcept
{
	auto shouldRelease{ false };
	{
		auto const lg = std::lock_guard{ _lock };

		// Search our reference to the specified entityID/token
		auto const tokensIt = _exclusiveAccessTokens.find(entityID);
		// The token might no longer be stored, in case the controller was destroyed and recreated before the token got destroyed
		if (tokensIt != _exclusiveAccessTokens.end())
		{
			auto& tokens = tokensIt->second;
			// Remove the reference from our list of tokens
			AVDECC_ASSERT_WITH_RET(tokens.erase(token) == 1, "Token reference not found for the specified entityID");
			if (tokens.empty())
			{
				shouldRelease = true;
				_exclusiveAccessTokens.erase(tokensIt);
			}
		}
	}

	if (shouldRelease)
	{
		switch (token->getAccessType())
		{
			case ExclusiveAccessToken::AccessType::Acquire:
			case ExclusiveAccessToken::AccessType::PersistentAcquire:
			{
				releaseEntity(entityID, nullptr);
				break;
			}
			case ExclusiveAccessToken::AccessType::Lock:
			{
				unlockEntity(entityID, nullptr);
				break;
			}
			default:
				AVDECC_ASSERT(false, "Unknown AccessType");
				break;
		}
	}
}

ControllerImpl::~ControllerImpl()
{
	AVDECC_ASSERT(!areControlledEntitiesSelfLocked(), "No ControlledEntity should be locked during this call. relinquish the ownership (with .reset()) before calling this method");

	// Notify the thread we are shutting down
	_shouldTerminate = true;

	// Wait for the thread to complete its pending tasks
	if (_stateMachinesThread.joinable())
	{
		_stateMachinesThread.join();
	}

	// First, remove ourself from the controller's delegate, we don't want notifications anymore (even if one is coming before the end of the destructor, it's not a big deal, _controlledEntities will be empty)
	_controller->setControllerDelegate(nullptr);

	auto controlledEntities = decltype(_controlledEntities){};
	auto exclusiveAccessTokens = decltype(_exclusiveAccessTokens){};

	// Move all controlled Entities and ExclusiveAccessTokens (under lock), we don't want them to be accessible during destructor
	{
		// Lock to protect data members
		auto const lg = std::lock_guard{ _lock };

		// Move Controlled Entities
		controlledEntities = std::move(_controlledEntities);

		// Move ExclusiveAccessTokens
		exclusiveAccessTokens = std::move(_exclusiveAccessTokens);
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

	// Invalidate all tokens
	for (auto& tokensKV : exclusiveAccessTokens)
	{
		for (auto* const token : tokensKV.second)
		{
			token->invalidateToken();
		}
	}

	// Remove all observers, we don't want to trigger notifications for upcoming actions
	removeAllObservers();

	// Perform some last actions on the entities before destroying everything
	for (auto const& [entityID, controlledEntity] : controlledEntities)
	{
		// Deregister from unsolicited notifications
		_controllerProxy->unregisterUnsolicitedNotifications(entityID, nullptr); // We don't need the result handler, let's just hope our message was properly sent and received!

		// Try to release all acquired / locked entities by this controller
		if (controlledEntity->isAcquired() || controlledEntity->isAcquireCommandInProgress())
		{
			_controllerProxy->releaseEntity(entityID, entity::model::DescriptorType::Entity, 0u, nullptr); // We don't need the result handler, let's just hope our message was properly sent and received!
		}
		if (controlledEntity->isLocked() || controlledEntity->isLockCommandInProgress())
		{
			_controllerProxy->unlockEntity(entityID, entity::model::DescriptorType::Entity, 0u, nullptr); // We don't need the result handler, let's just hope our message was properly sent and received!
		}
	}

	// Destroy the controller proxy before the class is destroyed
	_controllerProxy = nullptr;
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
void ControllerImpl::enableEntityAdvertising(std::uint32_t const availableDuration, std::optional<entity::model::AvbInterfaceIndex> const interfaceIndex)
{
	if (!_controller->enableEntityAdvertising(availableDuration, interfaceIndex))
		throw Exception(Error::DuplicateProgID, "Specified ProgID already in use on the local computer");
	LOG_CONTROLLER_INFO(_controller->getEntityID(), "Controller advertising enabled");
}

void ControllerImpl::disableEntityAdvertising(std::optional<entity::model::AvbInterfaceIndex> const interfaceIndex) noexcept
{
	_controller->disableEntityAdvertising(interfaceIndex);
	LOG_CONTROLLER_INFO(_controller->getEntityID(), "Controller advertising disabled");
}

bool ControllerImpl::discoverRemoteEntities() const noexcept
{
	LOG_CONTROLLER_INFO(_controller->getEntityID(), "Requesting remote entities discovery");
	return _controller->discoverRemoteEntities();
}

bool ControllerImpl::discoverRemoteEntity(UniqueIdentifier const entityID) const noexcept
{
	LOG_CONTROLLER_INFO(_controller->getEntityID(), "Requesting remote entity {} discovery", utils::toHexString(entityID, true));
	return _controller->discoverRemoteEntity(entityID);
}

void ControllerImpl::setAutomaticDiscoveryDelay(std::chrono::milliseconds const delay) noexcept
{
	_controller->setAutomaticDiscoveryDelay(delay);
	if (delay.count() == 0)
	{
		LOG_CONTROLLER_INFO(_controller->getEntityID(), "Controller automatic discovery disabled");
	}
	else
	{
		LOG_CONTROLLER_INFO(_controller->getEntityID(), "Controller automatic discovery delay set to {} msec", delay.count());
	}
}

void ControllerImpl::enableEntityModelCache() noexcept
{
	EntityModelCache::getInstance().enableCache();
	LOG_CONTROLLER_INFO(_controller->getEntityID(), "AEM-CACHE Enabled");
}

void ControllerImpl::disableEntityModelCache() noexcept
{
	EntityModelCache::getInstance().disableCache();
	LOG_CONTROLLER_INFO(_controller->getEntityID(), "AEM-CACHE Disabled");
}

void ControllerImpl::enableFullStaticEntityModelEnumeration() noexcept
{
	_fullStaticModelEnumeration = true;
}

void ControllerImpl::disableFullStaticEntityModelEnumeration() noexcept
{
	_fullStaticModelEnumeration = false;
}


/* Enumeration and Control Protocol (AECP) */
void ControllerImpl::acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, AcquireEntityHandler const& handler) const noexcept
{
	auto const descriptorType{ entity::model::DescriptorType::Entity };
	auto const descriptorIndex{ entity::model::DescriptorIndex{ 0u } };

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User acquireEntity (isPersistent={} DescriptorType={} DescriptorIndex={})", isPersistent, utils::to_integral(descriptorType), descriptorIndex);

		// Already acquired, notify of the success without sending a message
		if (controlledEntity->isAcquired())
		{
			LOG_CONTROLLER_TRACE(targetEntityID, "User acquireEntity not sent because entity is already acquired");
			utils::invokeProtectedHandler(handler, controlledEntity.get(), entity::ControllerEntity::AemCommandStatus::Success, controlledEntity->getOwningControllerID());
			return;
		}

		// Already trying to acquire or release, notify we are busy
		if (controlledEntity->isAcquireCommandInProgress())
		{
			LOG_CONTROLLER_TRACE(targetEntityID, "User acquireEntity not sent because entity is being acquired or released");
			utils::invokeProtectedHandler(handler, controlledEntity.get(), entity::ControllerEntity::AemCommandStatus::Busy, controlledEntity->getOwningControllerID());
			return;
		}

		// Try to acquire for all other cases, especially if it's acquired by another controller (in case it went offline without notice)
		controlledEntity->setAcquireState(model::AcquireState::AcquireInProgress);
		controlledEntity.reset(); // We have to relinquish the ownership of the ControlledEntity before calling the controller
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->acquireEntity(targetEntityID, isPersistent, descriptorType, descriptorIndex,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const owningEntity, [[maybe_unused]] entity::model::DescriptorType const descriptorType, [[maybe_unused]] entity::model::DescriptorIndex const descriptorIndex)
			{
				LOG_CONTROLLER_TRACE(entityID, "User acquireEntityResult (OwningController={} DescriptorType={} DescriptorIndex={}): {}", utils::toHexString(owningEntity, true), utils::to_integral(descriptorType), descriptorIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto& entity = *controlledEntity;

					// Always update acquired state (status is checked in getAcquiredInfoFromStatus)
					auto const [acquireState, owningController] = getAcquiredInfoFromStatus(entity, owningEntity, status, false);
					updateAcquiredState(entity, acquireState, owningController);

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity.wasAdvertised() ? &entity : nullptr, status, owningEntity);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status, owningEntity);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity, UniqueIdentifier::getNullUniqueIdentifier());
	}
}

void ControllerImpl::releaseEntity(UniqueIdentifier const targetEntityID, ReleaseEntityHandler const& handler) const noexcept
{
	auto const descriptorType{ entity::model::DescriptorType::Entity };
	auto const descriptorIndex{ entity::model::DescriptorIndex{ 0u } };

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User releaseEntity (DescriptorType={} DescriptorIndex={})", utils::to_integral(descriptorType), descriptorIndex);

		// Already trying to acquire or release, notify we are busy
		if (controlledEntity->isAcquireCommandInProgress())
		{
			LOG_CONTROLLER_TRACE(targetEntityID, "User releaseEntity not sent because entity is being acquired or released");
			utils::invokeProtectedHandler(handler, controlledEntity.get(), entity::ControllerEntity::AemCommandStatus::Busy, controlledEntity->getOwningControllerID());
			return;
		}

		controlledEntity->setAcquireState(model::AcquireState::ReleaseInProgress);
		controlledEntity.reset(); // We have to relinquish the ownership of the ControlledEntity before calling the controller
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->releaseEntity(targetEntityID, descriptorType, descriptorIndex,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const owningEntity, [[maybe_unused]] entity::model::DescriptorType const descriptorType, [[maybe_unused]] entity::model::DescriptorIndex const descriptorIndex)
			{
				LOG_CONTROLLER_TRACE(entityID, "User releaseEntity (OwningController={} DescriptorType={} DescriptorIndex={}): {}", utils::toHexString(owningEntity, true), utils::to_integral(descriptorType), descriptorIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto& entity = *controlledEntity;

					// Always update acquired state (status is checked in getAcquiredInfoFromStatus)
					auto const [acquireState, owningController] = getAcquiredInfoFromStatus(entity, owningEntity, status, true);
					updateAcquiredState(entity, acquireState, owningController);

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity.wasAdvertised() ? &entity : nullptr, status, owningEntity);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status, owningEntity);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity, UniqueIdentifier::getNullUniqueIdentifier());
	}
}

void ControllerImpl::lockEntity(UniqueIdentifier const targetEntityID, LockEntityHandler const& handler) const noexcept
{
	auto const descriptorType{ entity::model::DescriptorType::Entity };
	auto const descriptorIndex{ entity::model::DescriptorIndex{ 0u } };

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User lockEntity (DescriptorType={} DescriptorIndex={})", utils::to_integral(descriptorType), descriptorIndex);

		// Already locked, notify of the success without sending a message
		if (controlledEntity->isLocked())
		{
			LOG_CONTROLLER_TRACE(targetEntityID, "User lockEntity not sent because entity is already locked");
			utils::invokeProtectedHandler(handler, controlledEntity.get(), entity::ControllerEntity::AemCommandStatus::Success, controlledEntity->getLockingControllerID());
			return;
		}

		// Already trying to lock or unlock, notify we are busy
		if (controlledEntity->isLockCommandInProgress())
		{
			LOG_CONTROLLER_TRACE(targetEntityID, "User lockEntity not sent because entity is being locked or unlocked");
			utils::invokeProtectedHandler(handler, controlledEntity.get(), entity::ControllerEntity::AemCommandStatus::Busy, controlledEntity->getLockingControllerID());
			return;
		}

		// Try to lock for all other cases, especially if it's locked by another controller (in case it went offline without notice)
		controlledEntity->setLockState(model::LockState::LockInProgress);
		controlledEntity.reset(); // We have to relinquish the ownership of the ControlledEntity before calling the controller
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->lockEntity(targetEntityID, descriptorType, descriptorIndex,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const lockingEntity, [[maybe_unused]] entity::model::DescriptorType const descriptorType, [[maybe_unused]] entity::model::DescriptorIndex const descriptorIndex)
			{
				LOG_CONTROLLER_TRACE(entityID, "User lockEntityResult (LockingController={} DescriptorType={} DescriptorIndex={}): {}", utils::toHexString(lockingEntity, true), utils::to_integral(descriptorType), descriptorIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto& entity = *controlledEntity;

					// Always update locked state (status is checked in getLockedInfoFromStatus)
					auto const [lockState, lockingController] = getLockedInfoFromStatus(entity, lockingEntity, status, false);
					updateLockedState(entity, lockState, lockingController);

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity.wasAdvertised() ? &entity : nullptr, status, lockingEntity);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status, lockingEntity);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity, UniqueIdentifier::getNullUniqueIdentifier());
	}
}

void ControllerImpl::unlockEntity(UniqueIdentifier const targetEntityID, UnlockEntityHandler const& handler) const noexcept
{
	auto const descriptorType{ entity::model::DescriptorType::Entity };
	auto const descriptorIndex{ entity::model::DescriptorIndex{ 0u } };

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User unlockEntity (DescriptorType={} DescriptorIndex={})", utils::to_integral(descriptorType), descriptorIndex);

		// Already trying to lock or unlock, notify we are busy
		if (controlledEntity->isLockCommandInProgress())
		{
			LOG_CONTROLLER_TRACE(targetEntityID, "User unlockEntity not sent because entity is being locked or unlocked");
			utils::invokeProtectedHandler(handler, controlledEntity.get(), entity::ControllerEntity::AemCommandStatus::Busy, controlledEntity->getLockingControllerID());
			return;
		}

		controlledEntity->setLockState(model::LockState::UnlockInProgress);
		controlledEntity.reset(); // We have to relinquish the ownership of the ControlledEntity before calling the controller
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->unlockEntity(targetEntityID, descriptorType, descriptorIndex,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const lockingEntity, [[maybe_unused]] entity::model::DescriptorType const descriptorType, [[maybe_unused]] entity::model::DescriptorIndex const descriptorIndex)
			{
				LOG_CONTROLLER_TRACE(entityID, "User unlockEntity (LockingController={} DescriptorType={} DescriptorIndex={}): {}", utils::toHexString(lockingEntity, true), utils::to_integral(descriptorType), descriptorIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto& entity = *controlledEntity;

					// Always update locked state (status is checked in getLockedInfoFromStatus)
					auto const [lockState, lockingController] = getLockedInfoFromStatus(entity, lockingEntity, status, true);
					updateLockedState(entity, lockState, lockingController);

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity.wasAdvertised() ? &entity : nullptr, status, lockingEntity);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status, lockingEntity);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity, UniqueIdentifier::getNullUniqueIdentifier());
	}
}

void ControllerImpl::setConfiguration(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setConfiguration (ConfigurationIndex={})", configurationIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setConfiguration(targetEntityID, configurationIndex,
			[this, handler](entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setConfiguration (ConfigurationIndex={}): {}", configurationIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update configuration
					if (!!status)
					{
						updateConfiguration(controller, *entity, configurationIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamInputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setStreamInputFormat (StreamIndex={} streamFormat={})", streamIndex, utils::toHexString(streamFormat.getValue(), true));
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setStreamInputFormat(targetEntityID, streamIndex, streamFormat,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setStreamInputFormat (StreamIndex={} streamFormat={}): {}", streamIndex, utils::toHexString(streamFormat.getValue(), true), entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update format
					if (!!status)
					{
						updateStreamInputFormat(*entity, streamIndex, streamFormat, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamOutputFormat(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setStreamOutputFormat (StreamIndex={} streamFormat={})", streamIndex, utils::toHexString(streamFormat.getValue(), true));
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setStreamOutputFormat(targetEntityID, streamIndex, streamFormat,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setStreamOutputFormat (StreamIndex={} streamFormat={}): {}", streamIndex, utils::toHexString(streamFormat.getValue(), true), entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update format
					if (!!status)
					{
						updateStreamOutputFormat(*entity, streamIndex, streamFormat, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamInputInfo(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, SetStreamInputInfoHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setStreamInputInfo (StreamIndex={})", streamIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setStreamInputInfo(targetEntityID, streamIndex, info,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setStreamInputInfo (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update info
					if (!!status)
					{
						updateStreamInputInfo(*entity, streamIndex, info, false, false, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull); // StreamFormat not required to be set in SetStreamInfo / Milan Extended Information not set in SetStreamInfo
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamOutputInfo(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, SetStreamOutputInfoHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setStreamOutputInfo (StreamIndex={})", streamIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setStreamOutputInfo(targetEntityID, streamIndex, info,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setStreamOutputInfo (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update info
					if (!!status)
					{
						updateStreamOutputInfo(*entity, streamIndex, info, false, false, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull); // StreamFormat not required to be set in SetStreamInfo / Milan Extended Information not set in SetStreamInfo
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setEntityName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setEntityName (Name={})", name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setEntityName(targetEntityID, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AvdeccFixedString const& entityName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setEntityName (): {}", entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateEntityName(*entity, entityName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setEntityGroupName(UniqueIdentifier const targetEntityID, entity::model::AvdeccFixedString const& name, SetEntityGroupNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setEntityGroupName (Name={})", name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setEntityGroupName(targetEntityID, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AvdeccFixedString const& entityGroupName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setEntityGroupName (): {}", entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateEntityGroupName(*entity, entityGroupName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setConfigurationName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name, SetConfigurationNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setConfigurationName (ConfigurationIndex={} Name={})", configurationIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setConfigurationName(targetEntityID, configurationIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setConfigurationName (ConfigurationIndex={}): {}", configurationIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateConfigurationName(*entity, configurationIndex, configurationName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setAudioUnitName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& name, SetAudioUnitNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setAudioUnitName (ConfigurationIndex={} AudioUnitIndex={} Name={})", configurationIndex, audioUnitIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setAudioUnitName(targetEntityID, configurationIndex, audioUnitIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& audioUnitName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setAudioUnitName (ConfigurationIndex={} AudioUnitIndex={}): {}", configurationIndex, audioUnitIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateAudioUnitName(*entity, configurationIndex, audioUnitIndex, audioUnitName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamInputNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setStreamInputName (ConfigurationIndex={} StreamIndex={} Name={})", configurationIndex, streamIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setStreamInputName(targetEntityID, configurationIndex, streamIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setStreamInputName (ConfigurationIndex={} StreamIndex={}): {}", configurationIndex, streamIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateStreamInputName(*entity, configurationIndex, streamIndex, streamInputName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setStreamOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& name, SetStreamOutputNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setStreamOutputName (ConfigurationIndex={} StreamIndex={} Name={})", configurationIndex, streamIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setStreamOutputName(targetEntityID, configurationIndex, streamIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setStreamOutputName (ConfigurationIndex={} StreamIndex={}): {}", configurationIndex, streamIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateStreamOutputName(*entity, configurationIndex, streamIndex, streamOutputName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setJackInputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& name, SetJackInputNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setJackInputName (ConfigurationIndex={} JackIndex={} Name={})", configurationIndex, jackIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setJackInputName(targetEntityID, configurationIndex, jackIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackInputName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setJackInputName (ConfigurationIndex={} JackIndex={}): {}", configurationIndex, jackIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateJackInputName(*entity, configurationIndex, jackIndex, jackInputName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setJackOutputName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& name, SetJackOutputNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setJackOutputName (ConfigurationIndex={} JackIndex={} Name={})", configurationIndex, jackIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setJackOutputName(targetEntityID, configurationIndex, jackIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackOutputName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setJackOutputName (ConfigurationIndex={} JackIndex={}): {}", configurationIndex, jackIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateJackOutputName(*entity, configurationIndex, jackIndex, jackOutputName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setAvbInterfaceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& name, SetAvbInterfaceNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setAvbInterfaceName (ConfigurationIndex={} AvbInterfaceIndex={} Name={})", configurationIndex, avbInterfaceIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setAvbInterfaceName(targetEntityID, configurationIndex, avbInterfaceIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& avbInterfaceName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setAvbInterfaceName (ConfigurationIndex={} AvbInterfaceIndex={}): {}", configurationIndex, avbInterfaceIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateAvbInterfaceName(*entity, configurationIndex, avbInterfaceIndex, avbInterfaceName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setClockSourceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& name, SetClockSourceNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setClockSourceName (ConfigurationIndex={} ClockSourceIndex={} Name={})", configurationIndex, clockSourceIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setClockSourceName(targetEntityID, configurationIndex, clockSourceIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& clockSourceName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setClockSourceName (ConfigurationIndex={} ClockSourceIndex={}): {}", configurationIndex, clockSourceIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateClockSourceName(*entity, configurationIndex, clockSourceIndex, clockSourceName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setMemoryObjectName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& name, SetMemoryObjectNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setMemoryObjectName (ConfigurationIndex={} MemoryObjectIndex={} Name={})", configurationIndex, memoryObjectIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setMemoryObjectName(targetEntityID, configurationIndex, memoryObjectIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& memoryObjectName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setMemoryObjectName (ConfigurationIndex={} MemoryObjectIndex={}): {}", configurationIndex, memoryObjectIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateMemoryObjectName(*entity, configurationIndex, memoryObjectIndex, memoryObjectName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setAudioClusterName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& name, SetAudioClusterNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setAudioClusterName (ConfigurationIndex={} AudioClusterIndex={} Name={})", configurationIndex, audioClusterIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setAudioClusterName(targetEntityID, configurationIndex, audioClusterIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& audioClusterName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setAudioClusterName (ConfigurationIndex={} AudioClusterIndex={}): {}", configurationIndex, audioClusterIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateAudioClusterName(*entity, configurationIndex, audioClusterIndex, audioClusterName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setControlName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, entity::model::AvdeccFixedString const& name, SetControlNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setControlName (ConfigurationIndex={} ControlIndex={} Name={})", configurationIndex, controlIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setControlName(targetEntityID, configurationIndex, controlIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, entity::model::AvdeccFixedString const& controlName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setControlName (ConfigurationIndex={} ControlIndex={}): {}", configurationIndex, controlIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateControlName(*entity, configurationIndex, controlIndex, controlName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setClockDomainName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& name, SetClockDomainNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setClockDomainName (ConfigurationIndex={} ClockDomainIndex={} Name={})", configurationIndex, clockDomainIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setClockDomainName(targetEntityID, configurationIndex, clockDomainIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& clockDomainName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setClockDomainName (ConfigurationIndex={} ClockDomainIndex={}): {}", configurationIndex, clockDomainIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateClockDomainName(*entity, configurationIndex, clockDomainIndex, clockDomainName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setTimingName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex, entity::model::AvdeccFixedString const& name, SetTimingNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setTimingName (ConfigurationIndex={} TimingIndex={} Name={})", configurationIndex, timingIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setTimingName(targetEntityID, configurationIndex, timingIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex, entity::model::AvdeccFixedString const& timingName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setTimingName (ConfigurationIndex={} TimingIndex={}): {}", configurationIndex, timingIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updateTimingName(*entity, configurationIndex, timingIndex, timingName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setPtpInstanceName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex, entity::model::AvdeccFixedString const& name, SetPtpInstanceNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setPtpInstanceName (ConfigurationIndex={} PtpInstanceIndex={} Name={})", configurationIndex, ptpInstanceIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setPtpInstanceName(targetEntityID, configurationIndex, ptpInstanceIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex, entity::model::AvdeccFixedString const& ptpInstanceName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setPtpInstanceName (ConfigurationIndex={} PtpInstanceIndex={}): {}", configurationIndex, ptpInstanceIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updatePtpInstanceName(*entity, configurationIndex, ptpInstanceIndex, ptpInstanceName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setPtpPortName(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex, entity::model::AvdeccFixedString const& name, SetPtpPortNameHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setPtpPortName (ConfigurationIndex={} PtpPortIndex={} Name={})", configurationIndex, ptpPortIndex, name.str());
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setPtpPortName(targetEntityID, configurationIndex, ptpPortIndex, name,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex, entity::model::AvdeccFixedString const& ptpPortName)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setPtpPortName (ConfigurationIndex={} PtpPortIndex={}): {}", configurationIndex, ptpPortIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update name
					if (!!status) // Only change the name in case of success
					{
						updatePtpPortName(*entity, configurationIndex, ptpPortIndex, ptpPortName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setAssociationID(UniqueIdentifier const targetEntityID, UniqueIdentifier const associationID, SetAssociationIDHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setAssociationID (AssociationID={})", associationID);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setAssociation(targetEntityID, associationID,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const associationID)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setAssociationID (AssociationID={}): {}", associationID, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update association
					if (!!status) // Only change the Association ID in case of success
					{
						updateAssociationID(*entity, associationID, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setAudioUnitSamplingRate (AudioUnitIndex={} SamplingRate={})", audioUnitIndex, samplingRate);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setAudioUnitSamplingRate(targetEntityID, audioUnitIndex, samplingRate,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setAudioUnitSamplingRate (AudioUnitIndex={} SamplingRate={}): {}", audioUnitIndex, samplingRate, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update rate
					if (!!status) // Only change the sampling rate in case of success
					{
						updateAudioUnitSamplingRate(*entity, audioUnitIndex, samplingRate, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setClockSource(UniqueIdentifier const targetEntityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setClockSource (ClockDomainIndex={} ClockSourceIndex={})", clockDomainIndex, clockSourceIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setClockSource(targetEntityID, clockDomainIndex, clockSourceIndex,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setClockSource (ClockDomainIndex={} ClockSourceIndex={}): {}", clockDomainIndex, clockSourceIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update source
					if (!!status) // Only change the clock source in case of success
					{
						updateClockSource(*entity, clockDomainIndex, clockSourceIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setControlValues(UniqueIdentifier const targetEntityID, entity::model::ControlIndex const controlIndex, entity::model::ControlValues const& controlValues, SetControlValuesHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setControlValues (ControlIndex={})", controlIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setControlValues(targetEntityID, controlIndex, controlValues,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ControlIndex const controlIndex, MemoryBuffer const& packedControlValues)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setControlValues (ControlIndex={}): {}", controlIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update source
					auto st = status;
					if (!!st) // Only change the control values in case of success
					{
						if (!updateControlValues(*entity, controlIndex, packedControlValues, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull))
						{
							st = entity::ControllerEntity::AemCommandStatus::ProtocolError;
						}
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, st);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::startStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User startStreamInput (StreamIndex={})", streamIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->startStreamInput(targetEntityID, streamIndex,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
			{
				LOG_CONTROLLER_TRACE(entityID, "User startStreamInput (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update status
					if (!!status) // Only change the running status in case of success
					{
						updateStreamInputRunningStatus(*entity, streamIndex, true, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::stopStreamInput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User stopStreamInput (StreamIndex={})", streamIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->stopStreamInput(targetEntityID, streamIndex,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
			{
				LOG_CONTROLLER_TRACE(entityID, "User stopStreamInput (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update status
					if (!!status) // Only change the running status in case of success
					{
						updateStreamInputRunningStatus(*entity, streamIndex, false, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::startStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User startStreamOutput (StreamIndex={})", streamIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->startStreamOutput(targetEntityID, streamIndex,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
			{
				LOG_CONTROLLER_TRACE(entityID, "User startStreamOutput (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update status
					if (!!status) // Only change the running status in case of success
					{
						updateStreamOutputRunningStatus(*entity, streamIndex, true, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::stopStreamOutput(UniqueIdentifier const targetEntityID, entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User stopStreamOutput (StreamIndex={})", streamIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->stopStreamOutput(targetEntityID, streamIndex,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex)
			{
				LOG_CONTROLLER_TRACE(entityID, "User stopStreamOutput (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update status
					if (!!status) // Only change the running status in case of success
					{
						updateStreamOutputRunningStatus(*entity, streamIndex, false, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	// Validate parameters in regard with protocol restrictions
	if (mappings.size() > protocol::aemPayload::AecpAemMaxAddRemoveAudioMappings)
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::BadArguments);
		return;
	}

	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User addStreamInputAudioMappings (StreamPortIndex={})", streamPortIndex); // TODO: Convert mappings to string and add to log
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->addStreamPortInputAudioMappings(targetEntityID, streamPortIndex, mappings,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
			{
				LOG_CONTROLLER_TRACE(entityID, "User addStreamInputAudioMappings (StreamPortIndex={}): {}", streamPortIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update mappings
					if (!!status)
					{
						updateStreamPortInputAudioMappingsAdded(*entity, streamPortIndex, mappings, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	// Validate parameters in regard with protocol restrictions
	if (mappings.size() > protocol::aemPayload::AecpAemMaxAddRemoveAudioMappings)
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::BadArguments);
		return;
	}

	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User addStreamOutputAudioMappings (StreamPortIndex={})", streamPortIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->addStreamPortOutputAudioMappings(targetEntityID, streamPortIndex, mappings,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
			{
				LOG_CONTROLLER_TRACE(entityID, "User addStreamOutputAudioMappings (StreamPortIndex={}): {}", streamPortIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update mappings
					if (!!status)
					{
						updateStreamPortOutputAudioMappingsAdded(*entity, streamPortIndex, mappings, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	// Validate parameters in regard with protocol restrictions
	if (mappings.size() > protocol::aemPayload::AecpAemMaxAddRemoveAudioMappings)
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::BadArguments);
		return;
	}

	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User removeStreamInputAudioMappings (StreamPortIndex={})", streamPortIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->removeStreamPortInputAudioMappings(targetEntityID, streamPortIndex, mappings,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
			{
				LOG_CONTROLLER_TRACE(entityID, "User removeStreamInputAudioMappings (StreamPortIndex={}): {}", streamPortIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update mappings
					if (!!status)
					{
						updateStreamPortInputAudioMappingsRemoved(*entity, streamPortIndex, mappings, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	// Validate parameters in regard with protocol restrictions
	if (mappings.size() > protocol::aemPayload::AecpAemMaxAddRemoveAudioMappings)
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::BadArguments);
		return;
	}

	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User removeStreamOutputAudioMappings (StreamPortIndex={})", streamPortIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->removeStreamPortOutputAudioMappings(targetEntityID, streamPortIndex, mappings,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
			{
				LOG_CONTROLLER_TRACE(entityID, "User removeStreamOutputAudioMappings (StreamPortIndex={}): {}", streamPortIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update mappings
					if (!!status)
					{
						updateStreamPortOutputAudioMappingsRemoved(*entity, streamPortIndex, mappings, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::setMemoryObjectLength(UniqueIdentifier const targetEntityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, SetMemoryObjectLengthHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User setMemoryObjectLength (ConfigurationIndex={} MemoryObjectIndex={} Length={})", configurationIndex, memoryObjectIndex, length);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setMemoryObjectLength(targetEntityID, configurationIndex, memoryObjectIndex, length,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length)
			{
				LOG_CONTROLLER_TRACE(entityID, "User setMemoryObjectLength (ConfigurationIndex={} MemoryObjectIndex={}): {}", configurationIndex, memoryObjectIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update length
					if (!!status)
					{
						updateMemoryObjectLength(*entity, configurationIndex, memoryObjectIndex, length, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::identifyEntity(UniqueIdentifier const targetEntityID, std::chrono::milliseconds const duration, IdentifyEntityHandler const& handler) const noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User identifyEntity ()");

		auto const identifyIndexOpt = controlledEntity->getIdentifyControlIndex();
		if (!identifyIndexOpt)
		{
			LOG_CONTROLLER_TRACE(targetEntityID, "User identifyEntity not sent because entity does not have a valid Identify Control Descriptor");
			utils::invokeProtectedHandler(handler, controlledEntity.get(), entity::ControllerEntity::AemCommandStatus::NoSuchDescriptor);
			return;
		}

		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->setControlValues(targetEntityID, *identifyIndexOpt, makeIdentifyControlValues(true),
			[this, handler, duration](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ControlIndex const controlIndex, MemoryBuffer const& packedControlValues)
			{
				LOG_CONTROLLER_TRACE(entityID, "User identifyEntity (): {}", entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();

					// Update source
					if (!!status) // Only change the control values in case of success
					{
						updateControlValues(*entity, controlIndex, packedControlValues, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

						// Register to the State Machine, if duration is not 0
						if (duration != std::chrono::milliseconds{ 0 })
						{
							// Lock to protect _entityIdentifications
							auto const lg = std::lock_guard{ _lock };

							// Get current time
							auto const currentTime = std::chrono::system_clock::now();

							_controllerIdentifications[entityID] = ControllerIdentificationState{ currentTime + duration, controlIndex };
						}
					}

					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

entity::addressAccess::Tlv ControllerImpl::makeNextReadDeviceMemoryTlv(std::uint64_t const baseAddress, std::uint64_t const length, std::uint64_t const currentSize) const noexcept
{
	try
	{
		if (currentSize < length)
		{
			auto const remaining = length - currentSize;
			auto const nextQuerySize = static_cast<size_t>(remaining > protocol::AaAecpMaxSingleTlvMemoryDataLength ? protocol::AaAecpMaxSingleTlvMemoryDataLength : remaining);
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
			auto const nextQuerySize = static_cast<size_t>(remaining > protocol::AaAecpMaxSingleTlvMemoryDataLength ? protocol::AaAecpMaxSingleTlvMemoryDataLength : remaining);
			return entity::addressAccess::Tlv{ baseAddress + currentSize, protocol::AaMode::Write, memoryBuffer.data() + currentSize, nextQuerySize };
		}
	}
	catch (...)
	{
	}
	return entity::addressAccess::Tlv{};
}

void ControllerImpl::onUserReadDeviceMemoryResult(UniqueIdentifier const targetEntityID, entity::ControllerEntity::AaCommandStatus const status, entity::addressAccess::Tlvs const& tlvs, std::uint64_t const baseAddress, std::uint64_t const length, ReadDeviceMemoryProgressHandler const& progressHandler, ReadDeviceMemoryCompletionHandler const& completionHandler, DeviceMemoryBuffer&& memoryBuffer) const noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(targetEntityID);
	auto* const entity = controlledEntity ? (controlledEntity->wasAdvertised() ? controlledEntity.get() : nullptr) : nullptr;

	LOG_CONTROLLER_TRACE(targetEntityID, "User readDeviceMemory chunk (BaseAddress={} Length={}): {}", utils::toHexString(baseAddress, true), length, entity::ControllerEntity::statusToString(status));
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
			// Notify progress update
			try
			{
				auto const progress = memoryBuffer.size() / static_cast<float>(length) * 100.0f;
				if (progressHandler && progressHandler(entity, progress))
				{
					// Invoke result handler
					utils::invokeProtectedHandler(completionHandler, entity, entity::ControllerEntity::AaCommandStatus::Aborted, memoryBuffer);
					return;
				}
			}
			catch (...)
			{
				// Ignore exceptions in user handler
			}
			// Read next TLV
			LOG_CONTROLLER_TRACE(targetEntityID, "User readDeviceMemory chunk (BaseAddress={}, Length={}, Pos={}, ChunkLength={})", utils::toHexString(baseAddress, true), length, tlv.getAddress() - baseAddress, tlv.size());
			_controllerProxy->addressAccess(targetEntityID, { std::move(tlv) },
				[this, baseAddress, length, progressHandler = std::move(progressHandler), completionHandler = std::move(completionHandler), memoryBuffer = std::move(memoryBuffer)](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AaCommandStatus const status, entity::addressAccess::Tlvs const& tlvs) mutable
				{
					onUserReadDeviceMemoryResult(entityID, status, tlvs, baseAddress, length, std::move(progressHandler), std::move(completionHandler), std::move(memoryBuffer));
				});
			return;
		}
	}
	else
	{
		memoryBuffer.clear();
	}

	// Notify of completion (either error or all TLVs processed)
	utils::invokeProtectedHandler(completionHandler, entity, status, memoryBuffer);
}

void ControllerImpl::onUserWriteDeviceMemoryResult(UniqueIdentifier const targetEntityID, entity::ControllerEntity::AaCommandStatus const status, std::uint64_t const baseAddress, std::uint64_t const sentSize, WriteDeviceMemoryProgressHandler const& progressHandler, WriteDeviceMemoryCompletionHandler const& completionHandler, DeviceMemoryBuffer&& memoryBuffer) const noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(targetEntityID);
	auto* const entity = controlledEntity ? (controlledEntity->wasAdvertised() ? controlledEntity.get() : nullptr) : nullptr;

	LOG_CONTROLLER_TRACE(targetEntityID, "User writeDeviceMemory chunk (BaseAddress={} Length={}): {}", utils::toHexString(baseAddress, true), memoryBuffer.size(), entity::ControllerEntity::statusToString(status));
	if (!!status)
	{
		// Check if we need to send another portion of the device memory
		auto tlv = makeNextWriteDeviceMemoryTlv(baseAddress, memoryBuffer, sentSize);
		if (tlv)
		{
			// Notify progress update
			try
			{
				auto const progress = sentSize / static_cast<float>(memoryBuffer.size()) * 100.0f;
				if (progressHandler && progressHandler(entity, progress))
				{
					// Invoke result handler
					utils::invokeProtectedHandler(completionHandler, entity, entity::ControllerEntity::AaCommandStatus::Aborted);
					return;
				}
			}
			catch (...)
			{
				// Ignore exceptions in user handler
			}
			LOG_CONTROLLER_TRACE(targetEntityID, "User writeDeviceMemory chunk (BaseAddress={}, Length={}, Pos={}, ChunkLength={})", utils::toHexString(baseAddress, true), memoryBuffer.size(), tlv.getAddress() - baseAddress, tlv.size());
			// We are moving the tlv, so we have to get its size before that
			auto const newSentSize = sentSize + tlv.size();
			// Write next TLV
			_controllerProxy->addressAccess(targetEntityID, { std::move(tlv) },
				[this, baseAddress, sentSize = newSentSize, progressHandler = std::move(progressHandler), completionHandler = std::move(completionHandler), memoryBuffer = std::move(memoryBuffer)](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AaCommandStatus const status, entity::addressAccess::Tlvs const& /*tlvs*/) mutable
				{
					onUserWriteDeviceMemoryResult(entityID, status, baseAddress, sentSize, std::move(progressHandler), std::move(completionHandler), std::move(memoryBuffer));
				});
			return;
		}
	}

	// Notify of completion (either error or all TLVs processed)
	utils::invokeProtectedHandler(completionHandler, entity, status);
}

void ControllerImpl::readDeviceMemory(UniqueIdentifier const targetEntityID, std::uint64_t const address, std::uint64_t const length, ReadDeviceMemoryProgressHandler const& progressHandler, ReadDeviceMemoryCompletionHandler const& completionHandler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		DeviceMemoryBuffer memoryBuffer{};
#pragma message("TODO: Find a way to have the DeviceMemoryBuffer being properly moved all the way through the lambdas and handlers. Currently some handlers are copied, so the DeviceMemoryBuffer is copied instead of being moved causing unecessary reallocations")
		memoryBuffer.reserve(static_cast<size_t>(length));

		auto tlv = makeNextReadDeviceMemoryTlv(address, length, 0u);
		if (tlv)
		{
			LOG_CONTROLLER_TRACE(targetEntityID, "User readDeviceMemory chunk (BaseAddress={}, Length={}, Pos={}, ChunkLength={})", utils::toHexString(address, true), length, 0, tlv.size());
			auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
			_controllerProxy->addressAccess(targetEntityID, { std::move(tlv) },
				[this, baseAddress = address, length, progressHandlerCopy = progressHandler, completionHandlerCopy = completionHandler, memoryBuffer = std::move(memoryBuffer)](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AaCommandStatus const status, entity::addressAccess::Tlvs const& tlvs) mutable
				{
					onUserReadDeviceMemoryResult(entityID, status, tlvs, baseAddress, length, std::move(progressHandlerCopy), std::move(completionHandlerCopy), std::move(memoryBuffer));
				});
		}
		else
		{
			utils::invokeProtectedHandler(completionHandler, nullptr, entity::ControllerEntity::AaCommandStatus::TlvInvalid, DeviceMemoryBuffer{});
		}
	}
	else
	{
		utils::invokeProtectedHandler(completionHandler, nullptr, entity::ControllerEntity::AaCommandStatus::UnknownEntity, DeviceMemoryBuffer{});
	}
}

void ControllerImpl::startOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, StartOperationHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User startOperation (DescriptorType={}, DescriptorIndex={}, OperationType={})", static_cast<std::uint16_t>(descriptorType), descriptorIndex, static_cast<std::uint16_t>(operationType));

		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->startOperation(targetEntityID, descriptorType, descriptorIndex, operationType, memoryBuffer,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/, la::avdecc::entity::model::OperationID const operationID, la::avdecc::entity::model::MemoryObjectOperationType const /*operationType*/, la::avdecc::MemoryBuffer const& memoryBuffer)
			{
				LOG_CONTROLLER_TRACE(entityID, "User startOperation (OperationID={}): {}", operationID, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();
					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status, operationID, memoryBuffer);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status, operationID, memoryBuffer);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity, entity::model::OperationID{ 0u }, MemoryBuffer{});
	}
}

void ControllerImpl::abortOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, AbortOperationHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User abortOperation (DescriptorType={}, DescriptorIndex={}, OperationID={})", static_cast<std::uint16_t>(descriptorType), descriptorIndex, operationID);

		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->abortOperation(targetEntityID, descriptorType, descriptorIndex, operationID,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::DescriptorType const /*descriptorType*/, entity::model::DescriptorIndex const /*descriptorIndex*/, entity::model::OperationID const /*operationID*/)
			{
				LOG_CONTROLLER_TRACE(entityID, "User abortOperation (): {}", entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();
					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::reboot(UniqueIdentifier const targetEntityID, RebootHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User reboot ()");

		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->reboot(targetEntityID,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status)
			{
				LOG_CONTROLLER_TRACE(entityID, "User reboot (): {}", entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();
					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::rebootToFirmware(UniqueIdentifier const targetEntityID, entity::model::MemoryObjectIndex const memoryObjectIndex, RebootHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(targetEntityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(targetEntityID, "User rebootToFirmware (MemoryObjectIndex={})", memoryObjectIndex);

		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->rebootToFirmware(targetEntityID, memoryObjectIndex,
			[this, handler](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::MemoryObjectIndex const /*memoryObjectIndex*/)
			{
				LOG_CONTROLLER_TRACE(entityID, "User rebootToFirmware (): {}", entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntity
				auto controlledEntity = getControlledEntityImplGuard(entityID);

				if (controlledEntity)
				{
					auto* const entity = controlledEntity.get();
					// Invoke result handler
					utils::invokeProtectedHandler(handler, entity->wasAdvertised() ? entity : nullptr, status);
				}
				else // The entity went offline right after we sent our message
				{
					utils::invokeProtectedHandler(handler, nullptr, status);
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::ControllerEntity::AemCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::startMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, entity::model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, StartMemoryObjectOperationHandler const& handler) const noexcept
{
	startOperation(targetEntityID, entity::model::DescriptorType::MemoryObject, descriptorIndex, operationType, memoryBuffer,
		[handler](controller::ControlledEntity const* const entity, entity::ControllerEntity::AemCommandStatus const status, entity::model::OperationID const operationID, MemoryBuffer const& /*memoryBuffer*/)
		{
			utils::invokeProtectedHandler(handler, entity, status, operationID);
		});
}

void ControllerImpl::startStoreMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, StartMemoryObjectOperationHandler const& handler) const noexcept
{
	startMemoryObjectOperation(targetEntityID, descriptorIndex, entity::model::MemoryObjectOperationType::Store, MemoryBuffer{}, handler);
}

void ControllerImpl::startStoreAndRebootMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, StartMemoryObjectOperationHandler const& handler) const noexcept
{
	startMemoryObjectOperation(targetEntityID, descriptorIndex, entity::model::MemoryObjectOperationType::StoreAndReboot, MemoryBuffer{}, handler);
}

void ControllerImpl::startReadMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, StartMemoryObjectOperationHandler const& handler) const noexcept
{
	startMemoryObjectOperation(targetEntityID, descriptorIndex, entity::model::MemoryObjectOperationType::Read, MemoryBuffer{}, handler);
}

void ControllerImpl::startEraseMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, StartMemoryObjectOperationHandler const& handler) const noexcept
{
	startMemoryObjectOperation(targetEntityID, descriptorIndex, entity::model::MemoryObjectOperationType::Erase, MemoryBuffer{}, handler);
}

void ControllerImpl::startUploadMemoryObjectOperation(UniqueIdentifier const targetEntityID, entity::model::DescriptorIndex const descriptorIndex, std::uint64_t const dataLength, StartMemoryObjectOperationHandler const& handler) const noexcept
{
#pragma message("TODO: Modify the Serializer/Deserializer classes so they can use a provided buffer (MemoryBuffer), instead of always using a static internal buffer. Template the class so the container is that!")
	Serializer<sizeof(dataLength)> ser{};

	ser << dataLength;

	MemoryBuffer buffer{ ser.data(), ser.usedBytes() };
	startMemoryObjectOperation(targetEntityID, descriptorIndex, entity::model::MemoryObjectOperationType::Upload, buffer, handler);
}

void ControllerImpl::writeDeviceMemory(UniqueIdentifier const targetEntityID, std::uint64_t const address, DeviceMemoryBuffer memoryBuffer, WriteDeviceMemoryProgressHandler const& progressHandler, WriteDeviceMemoryCompletionHandler const& completionHandler) const noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(targetEntityID, true);

	if (controlledEntity)
	{
#pragma message("TODO: Find a way to have the DeviceMemoryBuffer being properly moved all the way through the lambdas and handlers. Currently some handlers are copied, so the DeviceMemoryBuffer is copied instead of being moved causing unecessary allocations")
		auto tlv = makeNextWriteDeviceMemoryTlv(address, memoryBuffer, 0u);
		if (tlv)
		{
			LOG_CONTROLLER_TRACE(targetEntityID, "User writeDeviceMemory chunk (BaseAddress={}, Length={}, Pos={}, ChunkLength={})", utils::toHexString(address, true), memoryBuffer.size(), 0, tlv.size());
			// Always temporarily unlock the ControlledEntities before calling the controller
			auto const guard = ControlledEntityUnlockerGuard{ *this };
			// We are moving the tlv, so we have to get its size before that
			auto const tlvSize = tlv.size();
			_controllerProxy->addressAccess(targetEntityID, { std::move(tlv) },
				[this, baseAddress = address, sentSize = tlvSize, progressHandlerCopy = progressHandler, completionHandlerCopy = completionHandler, memoryBuffer = std::move(memoryBuffer)](entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AaCommandStatus const status, entity::addressAccess::Tlvs const& /*tlvs*/) mutable
				{
					onUserWriteDeviceMemoryResult(entityID, status, baseAddress, sentSize, std::move(progressHandlerCopy), std::move(completionHandlerCopy), std::move(memoryBuffer));
				});
		}
		else
		{
			utils::invokeProtectedHandler(completionHandler, nullptr, entity::ControllerEntity::AaCommandStatus::TlvInvalid);
		}
	}
	else
	{
		utils::invokeProtectedHandler(completionHandler, nullptr, entity::ControllerEntity::AaCommandStatus::UnknownEntity);
	}
}

void ControllerImpl::connectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(listenerStream.entityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User connectStream (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={})", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->connectStream(talkerStream, listenerStream,
			[this, handler](entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
			{
				LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User connectStream (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={}): {}", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntities
				auto listener = getControlledEntityImplGuard(listenerStream.entityID);
				auto talker = getControlledEntityImplGuard(talkerStream.entityID);

				if (!!status)
				{
					// Do not trust the connectionCount value to determine if the listener is connected, but rather use the fact there was no error in the command
					handleListenerStreamStateNotification(talkerStream, listenerStream, true, flags, false);
				}

				// Invoke result handler
				utils::invokeProtectedHandler(handler, talker.get(), listener.get(), talkerStream.streamIndex, listenerStream.streamIndex, status);
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, nullptr, entity::model::StreamIndex(0), entity::model::StreamIndex(0), entity::ControllerEntity::ControlStatus::UnknownEntity);
	}
}

void ControllerImpl::disconnectStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(listenerStream.entityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User disconnectStream (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={})", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->disconnectStream(talkerStream, listenerStream,
			[this, handler](entity::controller::Interface const* const /*controller*/, [[maybe_unused]] entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
			{
				LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User disconnectStream (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={}): {}", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex, entity::ControllerEntity::statusToString(status));

				if (!!status || status == entity::ControllerEntity::ControlStatus::NotConnected) // No error, update the connection state
				{
					// Do not trust the connectionCount value to determine if the listener is disconnected, but rather use the fact there was no error (NOT_CONNECTED is actually not an error) in the command
					handleListenerStreamStateNotification({}, listenerStream, false, flags, false);

					// Take a "scoped locked" shared copy of the ControlledEntity
					auto listener = getControlledEntityImplGuard(listenerStream.entityID);

					// Invoke result handler
					utils::invokeProtectedHandler(handler, listener.get(), listenerStream.streamIndex, entity::ControllerEntity::ControlStatus::Success); // Force SUCCESS as status
				}
				else
				{
					// In case of a disconnect we might get an error (forwarded from the talker) but the stream is actually disconnected.
					// In that case, we have to query the listener stream state in order to know the actual connection state
					// Also don't notify the result handler right now, wait for getListenerStreamState answer
					_controllerProxy->getListenerStreamState(listenerStream,
						[this, handler, disconnectStatus = status](entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
						{
							entity::ControllerEntity::ControlStatus controlStatus{ disconnectStatus };
							// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
							auto const isStillConnected = connectionCount != 0;

							if (!!status)
							{
								// Status to return depends if we actually got disconnected (success in that case)
								controlStatus = isStillConnected ? disconnectStatus : entity::ControllerEntity::ControlStatus::Success;
							}

							// Take a "scoped locked" shared copy of the ControlledEntity
							auto listener = getControlledEntityImplGuard(listenerStream.entityID);

							if (!!status)
							{
								handleListenerStreamStateNotification(talkerStream, listenerStream, isStillConnected, flags, false);
							}

							// Invoke result handler
							utils::invokeProtectedHandler(handler, listener.get(), listenerStream.streamIndex, controlStatus);
						});
				}
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, entity::model::StreamIndex(0), entity::ControllerEntity::ControlStatus::UnknownEntity);
	}
}

void ControllerImpl::disconnectTalkerStream(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(talkerStream.entityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User disconnectTalkerStream (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={})", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->disconnectTalkerStream(talkerStream, listenerStream,
			[this, handler](entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
			{
				LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User disconnectTalkerStream (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={}): {}", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex, entity::ControllerEntity::statusToString(status));

				auto st = status;
				if (st == entity::ControllerEntity::ControlStatus::NotConnected)
				{
					st = entity::ControllerEntity::ControlStatus::Success;
				}

				if (!!status) // No error, update the connection state
				{
					// Do not trust the connectionCount value to determine if the listener is connected, but rather use the fact there was no error in the command
					handleTalkerStreamStateNotification(talkerStream, listenerStream, false, flags, true);
				}

				// Invoke result handler
				utils::invokeProtectedHandler(handler, st);
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, entity::ControllerEntity::ControlStatus::UnknownEntity);
	}
}

void ControllerImpl::getListenerStreamState(entity::model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept
{
	// Get a shared copy of the ControlledEntity so it stays alive while in the scope
	auto controlledEntity = getSharedControlledEntityImplHolder(listenerStream.entityID, true);

	if (controlledEntity)
	{
		LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User getListenerStreamState (ListenerID={} ListenerIndex={})", utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex);
		auto const guard = ControlledEntityUnlockerGuard{ *this }; // Always temporarily unlock the ControlledEntities before calling the controller
		_controllerProxy->getListenerStreamState(listenerStream,
			[this, handler](entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status)
			{
				LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "User getListenerStreamState (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={}): {}", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex, entity::ControllerEntity::statusToString(status));

				// Take a "scoped locked" shared copy of the ControlledEntities
				auto listener = getControlledEntityImplGuard(listenerStream.entityID);
				auto talker = getControlledEntityImplGuard(talkerStream.entityID);

				if (!!status)
				{
					// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
					handleListenerStreamStateNotification(talkerStream, listenerStream, connectionCount != 0, flags, false);
				}

				// Invoke result handler
				utils::invokeProtectedHandler(handler, talker.get(), listener.get(), talkerStream.streamIndex, listenerStream.streamIndex, connectionCount, flags, status);
			});
	}
	else
	{
		utils::invokeProtectedHandler(handler, nullptr, nullptr, entity::model::StreamIndex(0), entity::model::StreamIndex(0), std::uint16_t(0), entity::ConnectionFlags{}, entity::ControllerEntity::ControlStatus::UnknownEntity);
	}
}

ControlledEntityGuard ControllerImpl::getControlledEntityGuard(UniqueIdentifier const entityID) const noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto entity = getControlledEntityImplGuard(entityID);
	if (entity && entity->wasAdvertised())
	{
		return ControlledEntityGuard{ entity.release() };
	}
	return {};
}

void ControllerImpl::requestExclusiveAccess(UniqueIdentifier const entityID, ExclusiveAccessToken::AccessType const type, RequestExclusiveAccessResultHandler&& handler) const noexcept
{
	// Helper lambda
	auto const onResult = [this, entityID, type, handler = std::move(handler)](ControlledEntity const* const entity, entity::ControllerEntity::AemCommandStatus const status)
	{
		auto token = ExclusiveAccessToken::UniquePointer{ nullptr, nullptr };

		if (status == entity::ControllerEntity::AemCommandStatus::Success)
		{
			// Lock to protect data members
			auto const lg = std::lock_guard{ _lock };

			// Get or create tokens list for this entityID
			auto& tokens = _exclusiveAccessTokens[entityID];

			// Create a new token and store a raw pointer to it
			token = ExclusiveAccessTokenImpl::create(this, entityID, type);
			tokens.insert(static_cast<ExclusiveAccessTokenImpl*>(token.get()));
		}

		// Notify handler
		la::avdecc::utils::invokeProtectedHandler(handler, entity, status, std::move(token));
	};

	// Request exclusive access based on specified type
	switch (type)
	{
		case ExclusiveAccessToken::AccessType::Acquire:
		case ExclusiveAccessToken::AccessType::PersistentAcquire:
		{
			acquireEntity(entityID, type == ExclusiveAccessToken::AccessType::PersistentAcquire,
				[onResult = std::move(onResult)](auto const* const entity, auto const status, auto const /*owningEntity*/)
				{
					onResult(entity, status);
				});
			break;
		}
		case ExclusiveAccessToken::AccessType::Lock:
		{
			lockEntity(entityID,
				[onResult = std::move(onResult)](auto const* const entity, auto const status, auto const /*lockingEntity*/)
				{
					onResult(entity, status);
				});
			break;
		}
		default:
			AVDECC_ASSERT(false, "Unknown AccessType");
			onResult(nullptr, la::avdecc::entity::ControllerEntity::AemCommandStatus::InternalError);
			break;
	}
}

void ControllerImpl::lock() noexcept
{
	_controller->lock();
}

void ControllerImpl::unlock() noexcept
{
	_controller->unlock();
}

/* Model serialization methods */
std::tuple<avdecc::jsonSerializer::SerializationError, std::string> ControllerImpl::serializeAllControlledEntitiesAsJson([[maybe_unused]] std::string const& filePath, [[maybe_unused]] entity::model::jsonSerializer::Flags const flags, [[maybe_unused]] std::string const& dumpSource, [[maybe_unused]] bool const continueOnError) const noexcept
{
#ifndef ENABLE_AVDECC_FEATURE_JSON
	return { avdecc::jsonSerializer::SerializationError::NotSupported, "Serialization feature not supported by the library (was not compiled)" };

#else // ENABLE_AVDECC_FEATURE_JSON

	// Create the object
	auto object = json{};

	// Dump information of the dump itself
	object[jsonSerializer::keyName::Controller_DumpVersion] = jsonSerializer::keyValue::Controller_DumpVersion;
	object[jsonSerializer::keyName::Controller_Informative_DumpSource] = dumpSource;

	// Lock to protect _controlledEntities
	std::lock_guard<decltype(_lock)> const lg(_lock);

	// Define a comparator operator for ControlledEntities as we want to dump entities ordered by EntityID
	auto const entityComparator = [](ControlledEntityImpl const* const& lhs, ControlledEntityImpl const* const& rhs)
	{
		return lhs->getEntity().getEntityID() < rhs->getEntity().getEntityID();
	};
	auto entities = std::set<ControlledEntityImpl const*, decltype(entityComparator)>{ entityComparator };

	// Process all known entities and add them to a sorted set
	for (auto const& entityIt : _controlledEntities)
	{
		entities.insert(entityIt.second.get());
	}

	auto error = avdecc::jsonSerializer::SerializationError::NoError;
	auto errorText = std::string{};
	// Serialize all known entities, sorted by EntityID
	for (auto const* const entity : entities)
	{
		// Try to serialize
		try
		{
			auto const entityObject = jsonSerializer::createJsonObject(*entity, flags);

			object[jsonSerializer::keyName::Controller_Entities].push_back(entityObject);
		}
		catch (avdecc::jsonSerializer::SerializationException const& e)
		{
			if (continueOnError)
			{
				error = avdecc::jsonSerializer::SerializationError::Incomplete;
				errorText = e.what();
				continue;
			}
			return { e.getError(), e.what() };
		}
	}

	// Try to open the output file
	auto const mode = std::ios::binary | std::ios::out;
	auto ofs = std::ofstream{ utils::filePathFromUTF8String(filePath), mode }; // We always want to read as 'binary', we don't want the cr/lf shit to alter the size of our allocated buffer (all modern code should handle both lf and cr/lf)

	// Failed to open file to writting
	if (!ofs.is_open())
	{
		return { avdecc::jsonSerializer::SerializationError::AccessDenied, std::strerror(errno) };
	}

	// Everything is fine, write the JSON object to disk
	if (flags.test(entity::model::jsonSerializer::Flag::BinaryFormat))
	{
		auto const binary = json::to_msgpack(object);
		ofs.write(reinterpret_cast<char const*>(binary.data()), binary.size() * sizeof(decltype(binary)::value_type));
	}
	else
	{
		ofs << std::setw(4) << object << std::endl;
	}

	return { error, errorText };
#endif // ENABLE_AVDECC_FEATURE_JSON
}

std::tuple<avdecc::jsonSerializer::SerializationError, std::string> ControllerImpl::serializeControlledEntityAsJson([[maybe_unused]] UniqueIdentifier const entityID, [[maybe_unused]] std::string const& filePath, [[maybe_unused]] entity::model::jsonSerializer::Flags const flags, [[maybe_unused]] std::string const& dumpSource) const noexcept
{
#ifndef ENABLE_AVDECC_FEATURE_JSON
	return { avdecc::jsonSerializer::SerializationError::NotSupported, "Serialization feature not supported by the library (was not compiled)" };

#else // ENABLE_AVDECC_FEATURE_JSON

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto const entity = getControlledEntityImplGuard(entityID, true);
	if (!entity)
	{
		return { avdecc::jsonSerializer::SerializationError::UnknownEntity, "Entity offline" };
	}

	// Try to serialize
	try
	{
		// Try to serialize
		auto object = jsonSerializer::createJsonObject(*entity, flags);

		// Add informative metadata to the object before serialization
		object[jsonSerializer::keyName::Controller_Informative_DumpSource] = dumpSource;

		// Try to open the output file
		auto const mode = std::ios::binary | std::ios::out;
		auto ofs = std::ofstream{ utils::filePathFromUTF8String(filePath), mode }; // We always want to read as 'binary', we don't want the cr/lf shit to alter the size of our allocated buffer (all modern code should handle both lf and cr/lf)

		// Failed to open file to writting
		if (!ofs.is_open())
		{
			return { avdecc::jsonSerializer::SerializationError::AccessDenied, std::strerror(errno) };
		}

		// Everything is fine, write the JSON object to disk
		if (flags.test(entity::model::jsonSerializer::Flag::BinaryFormat))
		{
			auto const binary = json::to_msgpack(object);
			ofs.write(reinterpret_cast<char const*>(binary.data()), binary.size() * sizeof(decltype(binary)::value_type));
		}
		else
		{
			ofs << std::setw(4) << object << std::endl;
		}

		return { avdecc::jsonSerializer::SerializationError::NoError, "" };
	}
	catch (avdecc::jsonSerializer::SerializationException const& e)
	{
		return { e.getError(), e.what() };
	}
#endif // ENABLE_AVDECC_FEATURE_JSON
}

/* Model deserialization methods */
std::tuple<avdecc::jsonSerializer::DeserializationError, std::string> ControllerImpl::loadVirtualEntitiesFromJsonNetworkState([[maybe_unused]] std::string const& filePath, [[maybe_unused]] entity::model::jsonSerializer::Flags const flags, [[maybe_unused]] bool const continueOnError) noexcept
{
#ifndef ENABLE_AVDECC_FEATURE_JSON
	return { avdecc::jsonSerializer::DeserializationError::NotSupported, "Deserialization feature not supported by the library (was not compiled)" };

#else // ENABLE_AVDECC_FEATURE_JSON

	auto [error, errorText, controlledEntities] = deserializeJsonNetworkState(filePath, flags, continueOnError, _entitiesSharedLockInformation);

	for (auto& controlledEntity : controlledEntities)
	{
		auto const [err, errTxt] = registerVirtualControlledEntity(std::move(controlledEntity));
		if (!!err)
		{
			if (continueOnError)
			{
				error = err;
				errorText = errTxt;
				continue;
			}
			return { err, errTxt };
		}
	}
	return { error, errorText };
#endif // ENABLE_AVDECC_FEATURE_JSON
}

std::tuple<avdecc::jsonSerializer::DeserializationError, std::string> ControllerImpl::loadVirtualEntityFromJson([[maybe_unused]] std::string const& filePath, [[maybe_unused]] entity::model::jsonSerializer::Flags const flags) noexcept
{
#ifndef ENABLE_AVDECC_FEATURE_JSON
	return { avdecc::jsonSerializer::DeserializationError::NotSupported, "Deserialization feature not supported by the library (was not compiled)" };

#else // ENABLE_AVDECC_FEATURE_JSON

	auto [error, errorText, controlledEntity] = deserializeJson(filePath, flags, _entitiesSharedLockInformation);
	if (!error)
	{
		return registerVirtualControlledEntity(std::move(controlledEntity));
	}
	else
	{
		return { error, errorText };
	}
#endif // ENABLE_AVDECC_FEATURE_JSON
}

std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, std::vector<SharedControlledEntity>> ControllerImpl::deserializeControlledEntitiesFromJsonNetworkState([[maybe_unused]] std::string const& filePath, [[maybe_unused]] entity::model::jsonSerializer::Flags const flags, [[maybe_unused]] bool const continueOnError) noexcept
{
#ifndef ENABLE_AVDECC_FEATURE_JSON
	return { avdecc::jsonSerializer::DeserializationError::NotSupported, "Deserialization feature not supported by the library (was not compiled)", {} };

#else // ENABLE_AVDECC_FEATURE_JSON

	auto [error, errorText, controlledEntities] = deserializeJsonNetworkState(filePath, flags, continueOnError, std::make_shared<ControlledEntityImpl::LockInformation>());
	auto entities = std::vector<SharedControlledEntity>{};

	for (auto& controlledEntity : controlledEntities)
	{
		// We need to run some setup on a detached virtual entity
		setupDetachedVirtualControlledEntity(*controlledEntity);

		entities.push_back(controlledEntity);
	}
	return { error, errorText, entities };
#endif // ENABLE_AVDECC_FEATURE_JSON
}

std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, SharedControlledEntity> ControllerImpl::deserializeControlledEntityFromJson([[maybe_unused]] std::string const& filePath, [[maybe_unused]] entity::model::jsonSerializer::Flags const flags) noexcept
{
#ifndef ENABLE_AVDECC_FEATURE_JSON
	return { avdecc::jsonSerializer::DeserializationError::NotSupported, "Deserialization feature not supported by the library (was not compiled)", nullptr };

#else // ENABLE_AVDECC_FEATURE_JSON

	auto [error, errorText, controlledEntity] = deserializeJson(filePath, flags, std::make_shared<ControlledEntityImpl::LockInformation>());
	if (!error)
	{
		// We need to run some setup on a detached virtual entity
		setupDetachedVirtualControlledEntity(*controlledEntity);
	}
	return { error, errorText, controlledEntity };
#endif // ENABLE_AVDECC_FEATURE_JSON
}

std::tuple<avdecc::jsonSerializer::DeserializationError, std::string> ControllerImpl::loadEntityModelFile(std::string const& /*filePath*/) noexcept
{
	// TODO:
	//  - Call EndStation::deserializeEntityModelFromJson
	//  - Feed the cache with the loaded model
	return { avdecc::jsonSerializer::DeserializationError::NotSupported, "Not supported yet" };
}

bool ControllerImpl::refreshEntity(UniqueIdentifier const entityID) noexcept
{
	// Check if entity is not virtual
	{
		// Lock to protect _controlledEntities
		std::lock_guard<decltype(_lock)> const lg(_lock);

		auto entityIt = _controlledEntities.find(entityID);
		// Entity not found
		if (entityIt == _controlledEntities.end())
		{
			return false;
		}
		// Entity is virtual
		if (entityIt->second->isVirtual())
		{
			return false;
		}
	}

	// Ready to remove using the network executor
	auto const exName = _endStation->getProtocolInterface()->getExecutorName();
	ExecutorManager::getInstance().pushJob(exName,
		[this, entityID]()
		{
			// Make a copy of the Entity object since it will be destroyed during onEntityOffline
			auto e = std::unique_ptr<entity::Entity>{ nullptr };
			{
				{
					// Lock to protect _controlledEntities
					std::lock_guard<decltype(_lock)> const lg(_lock);

					auto entityIt = _controlledEntities.find(entityID);
					// Entity not found
					if (entityIt == _controlledEntities.end())
					{
						return;
					}
					e = std::make_unique<entity::Entity>(entityIt->second->getEntity());
				}
			}

			auto const lg = std::lock_guard{ *_controller }; // Lock the Controller itself (thus, lock it's ProtocolInterface), since we are on the Networking Thread
			onEntityOffline(_controller, entityID);
			onEntityOnline(_controller, entityID, *e);
		});

	// Flush executor to be sure everything is loaded before returning
	ExecutorManager::getInstance().flush(exName);

	return true;
}

bool ControllerImpl::unloadVirtualEntity(UniqueIdentifier const entityID) noexcept
{
	// Check if entity is virtual
	{
		// Lock to protect _controlledEntities
		std::lock_guard<decltype(_lock)> const lg(_lock);

		auto entityIt = _controlledEntities.find(entityID);
		// Entity not found
		if (entityIt == _controlledEntities.end())
		{
			return false;
		}
		// Entity is not virtual
		if (!entityIt->second->isVirtual())
		{
			return false;
		}
	}

	// Ready to remove using the network executor
	auto const exName = _endStation->getProtocolInterface()->getExecutorName();
	ExecutorManager::getInstance().pushJob(exName,
		[this, entityID]()
		{
			auto const lg = std::lock_guard{ *_controller }; // Lock the Controller itself (thus, lock it's ProtocolInterface), since we are on the Networking Thread

			onEntityOffline(_controller, entityID);
		});

	// Flush executor to be sure everything is loaded before returning
	ExecutorManager::getInstance().flush(exName);

	return true;
}

} // namespace controller
} // namespace avdecc
} // namespace la
