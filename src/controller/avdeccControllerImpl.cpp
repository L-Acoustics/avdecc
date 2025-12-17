/*
* Copyright (C) 2016-2025, L-Acoustics and its contributors

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
* @file avdeccControllerImpl.cpp
* @author Christophe Calmejane
*/

#ifndef __cpp_structured_bindings
#	error "__cpp_structured_bindings not supported by the compiler. Check minimum requirements."
#endif

#include "avdeccControllerImpl.hpp"
#include "avdeccControllerLogHelper.hpp"
#include "avdeccEntityModelCache.hpp"
#include "entityModelChecksum.hpp"

#ifdef ENABLE_AVDECC_FEATURE_JSON
#	include "avdeccControllerJsonTypes.hpp"
#	include "avdeccControlledEntityJsonSerializer.hpp"
#	include <la/avdecc/internals/jsonTypes.hpp>
#endif // ENABLE_AVDECC_FEATURE_JSON
#include <la/avdecc/internals/streamFormatInfo.hpp>
#include <la/avdecc/internals/entityModelControlValuesTraits.hpp>
#include <la/avdecc/internals/protocolAemPayloadSizes.hpp>
#include <la/avdecc/internals/protocolInterface.hpp>
#include <la/avdecc/executor.hpp>
#include <la/avdecc/utils.hpp>

#include <fstream>
#include <unordered_set>
#include <map>
#include <utility>
#include <set>

namespace la
{
namespace avdecc
{
namespace controller
{
/* ************************************************************ */
/* Private methods used to update AEM and notify observers      */
/* ************************************************************ */
void ControllerImpl::updateEntity(ControlledEntityImpl& controlledEntity, entity::Entity const& entity) const noexcept
{
	// Get previous entity info, so we can check what changed
	auto const oldEntity = static_cast<entity::Entity>(controlledEntity.getEntity()); // Make a copy of the Entity object since it might be altered in this function before checking for difference (use static_cast to prevent warning with 'auto' not being a reference)

	auto const& oldInterfacesInfo = oldEntity.getInterfacesInformation();
	auto const& newInterfacesInfo = entity.getInterfacesInformation();

	// Only do checks if entity was advertised to the user
	if (controlledEntity.wasAdvertised())
	{
		// Check for any removed interface (don't compare info yet, just if one was removed)
		for (auto const& oldInfoKV : oldInterfacesInfo)
		{
			auto const oldIndex = oldInfoKV.first;

			// Not present in new list, it was removed
			if (!entity.hasInterfaceIndex(oldIndex))
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityRedundantInterfaceOffline, this, &controlledEntity, oldIndex);
			}
		}

		// Check for any added interface (don't compare info yet, just if one was added)
		for (auto const& [newIndex, interfaceInfo] : newInterfacesInfo)
		{
			// Not present in old list, it was added
			if (!oldEntity.hasInterfaceIndex(newIndex))
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityRedundantInterfaceOnline, this, &controlledEntity, newIndex, interfaceInfo);
			}
		}
	}

	// For each interface, check if gPTP info changed (if we have the info)
	for (auto const& infoKV : newInterfacesInfo)
	{
		auto const& information = infoKV.second;

		// Only if we have valid gPTP information
		if (information.gptpGrandmasterID)
		{
			auto const avbInterfaceIndex = infoKV.first;
			auto shouldUpdate = false;

			// Get Old Information
			if (oldEntity.hasInterfaceIndex(avbInterfaceIndex))
			{
				auto const& oldInfo = oldEntity.getInterfaceInformation(avbInterfaceIndex);
				// gPTP changed (or didn't have)
				if (!oldInfo.gptpGrandmasterID || *oldInfo.gptpGrandmasterID != *information.gptpGrandmasterID || *oldInfo.gptpDomainNumber != *information.gptpDomainNumber)
				{
					shouldUpdate = true;
				}
			}
			// The AvbInterface was not found in the previous stored entity. Looks like cable redundancy and we just discovered the other interface
			else
			{
				shouldUpdate = true;
			}

			if (shouldUpdate)
			{
				updateGptpInformation(controlledEntity, avbInterfaceIndex, information.macAddress, *information.gptpGrandmasterID, *information.gptpDomainNumber, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
		}
	}

	auto const caps = entity.getEntityCapabilities();

	// Until we have confirmation that an entity should always send the AssociationID value (if supported) in ADP, we have to check for the presence of the VALID bit before changing the value
	// If this is confirmed, then we'll always change the value and use std::nullopt if the VALID bit is not set
	auto const associationID = entity.getAssociationID();
	if (caps.test(entity::EntityCapability::AssociationIDValid))
	{
		// Set the new AssociationID and notify if it changed
		updateAssociationID(controlledEntity, associationID, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
	}
	else
	{
		// At least check if the AssociationID was set to something and print a warning
		if (associationID)
		{
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Entity previously declared a VALID AssociationID, but it's not defined anymore in ADP");
		}
	}

	// Only do checks if entity was advertised to the user (we already changed the values anyway)
	if (controlledEntity.wasAdvertised())
	{
		// Check if Capabilities changed
		if (oldEntity.getEntityCapabilities() != caps)
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityCapabilitiesChanged, this, &controlledEntity);
		}
	}

	// Update the full entity info (for information not separately handled)
	controlledEntity.setEntity(entity);
}

void ControllerImpl::addCompatibilityFlag(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, ControlledEntity::CompatibilityFlag const flag) noexcept
{
	auto const oldFlags = controlledEntity.getCompatibilityFlags();
	auto newFlags = oldFlags;

	switch (flag)
	{
		case ControlledEntity::CompatibilityFlag::IEEE17221:
			if (!newFlags.test(ControlledEntity::CompatibilityFlag::Misbehaving))
			{
				newFlags.set(flag);
			}
			break;
		case ControlledEntity::CompatibilityFlag::Milan:
			if (!newFlags.test(ControlledEntity::CompatibilityFlag::Misbehaving))
			{
				newFlags.set(ControlledEntity::CompatibilityFlag::IEEE17221); // A Milan device is also an IEEE1722.1 compatible device
				newFlags.set(flag);
				// If device was already with IEEE warning, also add MilanWarning flag
				if (newFlags.test(ControlledEntity::CompatibilityFlag::IEEE17221Warning))
				{
					newFlags.set(ControlledEntity::CompatibilityFlag::MilanWarning);
				}
			}
			break;
		case ControlledEntity::CompatibilityFlag::IEEE17221Warning:
			if (AVDECC_ASSERT_WITH_RET(newFlags.test(ControlledEntity::CompatibilityFlag::IEEE17221), "Adding IEEE17221Warning flag for a non IEEE17221 device"))
			{
				newFlags.set(flag);
				// If device was Milan compliant, also add MilanWarning flag
				if (newFlags.test(ControlledEntity::CompatibilityFlag::Milan))
				{
					newFlags.set(ControlledEntity::CompatibilityFlag::MilanWarning);
				}
			}
			break;
		case ControlledEntity::CompatibilityFlag::MilanWarning:
			setMilanWarningCompatibilityFlag(controller, controlledEntity, "Milan", "Minor warnings in the model/behavior that do not retrograde a Milan entity");
			return;
		case ControlledEntity::CompatibilityFlag::Misbehaving:
			setMisbehavingCompatibilityFlag(controller, controlledEntity, "IEEE1722.1-2021", "Entity is sending incoherent values (misbehaving) in violation of the standard");
			return;
		default:
			AVDECC_ASSERT(false, "Unknown CompatibilityFlag");
			return;
	}

	if (oldFlags != newFlags)
	{
		controlledEntity.setCompatibilityFlags(newFlags);

		if (controller)
		{
			AVDECC_ASSERT(controller->_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");
			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				controller->notifyObserversMethod<Controller::Observer>(&Controller::Observer::onCompatibilityChanged, controller, &controlledEntity, newFlags, controlledEntity.getMilanCompatibilityVersion());
			}
		}
	}
}

void ControllerImpl::setMisbehavingCompatibilityFlag(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, std::string const& specClause, std::string const& message) noexcept
{
	// If entity was not already marked as misbehaving
	if (!controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Misbehaving))
	{
		// A misbehaving device is not IEEE1722.1 compatible (so also not Milan compatible)
		removeCompatibilityFlag(controller, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, specClause, message);

		// Now set the Misbehaving flag
		auto flags = controlledEntity.getCompatibilityFlags();
		flags.set(ControlledEntity::CompatibilityFlag::Misbehaving);
		LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Entity is sending incoherent values (misbehaving)");
		controlledEntity.setCompatibilityFlags(flags);

		if (controller)
		{
			AVDECC_ASSERT(controller->_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");
			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				controller->notifyObserversMethod<Controller::Observer>(&Controller::Observer::onCompatibilityChanged, controller, &controlledEntity, flags, controlledEntity.getMilanCompatibilityVersion());
			}
		}
	}
}

void ControllerImpl::setMilanWarningCompatibilityFlag(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, std::string const& specClause, std::string const& message) noexcept
{
	auto const oldFlags = controlledEntity.getCompatibilityFlags();
	auto const oldMilanCompatibilityVersion = controlledEntity.getMilanCompatibilityVersion();
	auto newFlags = oldFlags;
	auto newMilanCompatibilityVersion = oldMilanCompatibilityVersion;

	LOG_CONTROLLER_COMPAT(controlledEntity.getEntity().getEntityID(), "[{}] {}", specClause, message);

	// If entity was not already marked as MilanWarning
	if (!oldFlags.test(ControlledEntity::CompatibilityFlag::MilanWarning))
	{
		if (AVDECC_ASSERT_WITH_RET(oldFlags.test(ControlledEntity::CompatibilityFlag::Milan), "Adding MilanWarning flag for a non Milan device"))
		{
			// Set the MilanWarning flag
			newFlags.set(ControlledEntity::CompatibilityFlag::MilanWarning);
			controlledEntity.setCompatibilityFlags(newFlags);

			if (controller)
			{
				AVDECC_ASSERT(controller->_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");
				// Create a compatibilityChanged event
				controlledEntity.addCompatibilityChangedEvent(ControlledEntity::CompatibilityChangedEvent{ oldFlags, oldMilanCompatibilityVersion, newFlags, newMilanCompatibilityVersion, specClause, message });
				// Entity was advertised to the user, notify observers
				if (controlledEntity.wasAdvertised())
				{
					controller->notifyObserversMethod<Controller::Observer>(&Controller::Observer::onCompatibilityChanged, controller, &controlledEntity, newFlags, newMilanCompatibilityVersion);
				}
			}
		}
	}
}

void ControllerImpl::removeCompatibilityFlag(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, ControlledEntity::CompatibilityFlag const flag, std::string const& specClause, std::string const& message) noexcept
{
	auto const oldFlags = controlledEntity.getCompatibilityFlags();
	auto const oldMilanCompatibilityVersion = controlledEntity.getMilanCompatibilityVersion();
	auto newFlags = oldFlags;
	auto newMilanCompatibilityVersion = oldMilanCompatibilityVersion;

	LOG_CONTROLLER_COMPAT(controlledEntity.getEntity().getEntityID(), "[{}] {}", specClause, message);

	switch (flag)
	{
		case ControlledEntity::CompatibilityFlag::IEEE17221:
			// If device was IEEE1722.1 compliant
			if (newFlags.test(ControlledEntity::CompatibilityFlag::IEEE17221))
			{
				LOG_CONTROLLER_COMPAT(controlledEntity.getEntity().getEntityID(), "Entity not fully IEEE1722.1 compliant");
				newFlags.reset(flag);
			}
			// A non compliant IEEE1722.1 device is not Milan compliant either
			[[fallthrough]];
		case ControlledEntity::CompatibilityFlag::Milan:
			// If device was Milan compliant
			if (newFlags.test(ControlledEntity::CompatibilityFlag::Milan))
			{
				LOG_CONTROLLER_COMPAT(controlledEntity.getEntity().getEntityID(), "Entity not fully Milan compliant");
				newMilanCompatibilityVersion = entity::model::MilanVersion{};
				newFlags.reset(flag);
			}
			break;
		case ControlledEntity::CompatibilityFlag::IEEE17221Warning:
			AVDECC_ASSERT(false, "Should not be possible to remove the IEEE17221Warning flag");
			break;
		case ControlledEntity::CompatibilityFlag::MilanWarning:
			AVDECC_ASSERT(false, "Should not be possible to remove the MilanWarning flag");
			break;
		case ControlledEntity::CompatibilityFlag::Misbehaving:
			AVDECC_ASSERT(false, "Should not be possible to remove the Misbehaving flag");
			break;
		default:
			AVDECC_ASSERT(false, "Unknown CompatibilityFlag");
			return;
	}

	if (oldFlags != newFlags || oldMilanCompatibilityVersion != newMilanCompatibilityVersion)
	{
		controlledEntity.setCompatibilityFlags(newFlags);
		controlledEntity.setMilanCompatibilityVersion(newMilanCompatibilityVersion);

		if (controller)
		{
			AVDECC_ASSERT(controller->_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");
			// Create a compatibilityChanged event
			controlledEntity.addCompatibilityChangedEvent(ControlledEntity::CompatibilityChangedEvent{ oldFlags, oldMilanCompatibilityVersion, newFlags, newMilanCompatibilityVersion, specClause, message });
			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				controller->notifyObserversMethod<Controller::Observer>(&Controller::Observer::onCompatibilityChanged, controller, &controlledEntity, newFlags, newMilanCompatibilityVersion);
			}
		}
	}
}

void ControllerImpl::decreaseMilanCompatibilityVersion(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, entity::model::MilanVersion const& version, std::string const& specClause, std::string const& message) noexcept
{
	auto const oldMilanCompatibilityVersion = controlledEntity.getMilanCompatibilityVersion();

	// Make sure we are not increasing the version
	if (version > oldMilanCompatibilityVersion)
	{
		return;
	}

	// If version gets down to 0, remove the Milan flag from the CompatibilityFlags
	if (version == entity::model::MilanVersion{})
	{
		removeCompatibilityFlag(controller, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, specClause, message);
		return;
	}

	if (oldMilanCompatibilityVersion != version)
	{
		LOG_CONTROLLER_COMPAT(controlledEntity.getEntity().getEntityID(), "[{}] {}", specClause, message);
		LOG_CONTROLLER_COMPAT(controlledEntity.getEntity().getEntityID(), "Downgrading Milan compatibility version from {} to {}", oldMilanCompatibilityVersion.to_string(), version.to_string());
		controlledEntity.setMilanCompatibilityVersion(version);

		if (controller)
		{
			AVDECC_ASSERT(controller->_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");
			// Create a compatibilityChanged event
			auto const compatibilityFlags = controlledEntity.getCompatibilityFlags();
			controlledEntity.addCompatibilityChangedEvent(ControlledEntity::CompatibilityChangedEvent{ compatibilityFlags, oldMilanCompatibilityVersion, compatibilityFlags, version, specClause, message });
			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				controller->notifyObserversMethod<Controller::Observer>(&Controller::Observer::onCompatibilityChanged, controller, &controlledEntity, controlledEntity.getCompatibilityFlags(), version);
			}
		}
	}
}

void ControllerImpl::updateUnsolicitedNotificationsSubscription(ControlledEntityImpl& controlledEntity, bool const isSubscribed, bool const triggeredByEntity) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const oldValue = controlledEntity.isSubscribedToUnsolicitedNotifications();

	if (oldValue != isSubscribed)
	{
		controlledEntity.setSubscribedToUnsolicitedNotifications(isSubscribed);

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onUnsolicitedRegistrationChanged, this, &controlledEntity, isSubscribed, triggeredByEntity);
		}
	}
}

void ControllerImpl::updateAcquiredState(ControlledEntityImpl& controlledEntity, model::AcquireState const acquireState, UniqueIdentifier const owningEntity) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setAcquireState(acquireState);
	controlledEntity.setOwningController(owningEntity);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		// If the Entity is getting released, check for any owned ExclusiveAccess tokens and invalidate them
		if (acquireState == model::AcquireState::NotAcquired)
		{
			removeExclusiveAccessTokens(controlledEntity.getEntity().getEntityID(), ExclusiveAccessToken::AccessType::Acquire);
		}

		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAcquireStateChanged, this, &controlledEntity, acquireState, owningEntity);
	}
}

void ControllerImpl::updateLockedState(ControlledEntityImpl& controlledEntity, model::LockState const lockState, UniqueIdentifier const lockingEntity) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setLockState(lockState);
	controlledEntity.setLockingController(lockingEntity);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		// If the Entity is getting unlocked, check for any owned ExclusiveAccess tokens and invalidate them
		if (lockState == model::LockState::NotLocked)
		{
			removeExclusiveAccessTokens(controlledEntity.getEntity().getEntityID(), ExclusiveAccessToken::AccessType::Lock);
		}

		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onLockStateChanged, this, &controlledEntity, lockState, lockingEntity);
	}
}

void ControllerImpl::updateConfiguration(entity::controller::Interface const* const /*controller*/, ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
#ifdef ENABLE_AVDECC_FEATURE_JSON
	if (controlledEntity.isVirtual())
	{
		// FIXME: Move 'canChangeVirtualEntityConfiguration' to a real public method
		auto const canChangeVirtualEntityConfiguration = [](ControlledEntityImpl const& controlledEntity, entity::model::ConfigurationIndex const /*configurationIndex*/) noexcept
		{
			// Check if this is a virtual entity
			if (!controlledEntity.isVirtual())
			{
				return false;
			}
			// Check if the model is valid for the new configuration (ask the AemCache)
			try
			{
				auto const& currentConfigNode = controlledEntity.getCurrentConfigurationNode();
				return EntityModelCache::isModelValidForConfiguration(currentConfigNode);
			}
			catch (ControlledEntity::Exception const&)
			{
				return false;
			}
		};
		// For a virtual entity, make sure a change of configuration is possible
		if (canChangeVirtualEntityConfiguration(controlledEntity, configurationIndex))
		{
			// Changing the configuration on a Virtual entity is tricky: A different configuration is like a different entity, some part of the model is only valid for the current configuration (like connections) so we need to make sure we update all related entities accordingly. We'll do that by temporarily removing the entity (declare it offline)
			auto* self = const_cast<ControllerImpl*>(this);

			auto const entityID = controlledEntity.getEntity().getEntityID();

			// Deregister the ControlledEntity
			auto sharedControlledEntity = self->deregisterVirtualControlledEntity(entityID);

			// Change the current configuration
			controlledEntity.setCurrentConfiguration(configurationIndex, notFoundBehavior);

			// Re-register entity
			self->registerVirtualControlledEntity(std::move(sharedControlledEntity));
		}
		else
		{
			// Otherwise remote the entity and log an error
			auto const entityID = controlledEntity.getEntity().getEntityID();
			// Shouldn't have been called if the configuration was not valid, log an error and remove the entity
			LOG_CONTROLLER_ERROR(entityID, "Requested Virtual entity configuration is not valid (call canChangeVirtualEntityConfiguration() before trying to change the configuration of a Virtual entity), removing entity");
			forgetRemoteEntity(entityID);
		}
	}
	else
#endif // ENABLE_AVDECC_FEATURE_JSON
	{
		// For real entities, simulate going offline then online again (to properly update the model)
		auto const entityID = controlledEntity.getEntity().getEntityID();
		forgetRemoteEntity(entityID);
		discoverRemoteEntity(entityID);
		// We don't need to change the current configuration, the entity will be re-enumarated
	}
}

void ControllerImpl::updateStreamInputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(notFoundBehavior);
	if (!currentConfigurationIndexOpt)
	{
		return;
	}

	auto* const streamDynamicModel = controlledEntity.getModelAccessStrategy().getStreamInputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
	if (streamDynamicModel)
	{
		if (streamDynamicModel->streamFormat != streamFormat)
		{
			streamDynamicModel->streamFormat = streamFormat;

			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputFormatChanged, this, &controlledEntity, streamIndex, streamFormat);
			}
		}
	}
}

void ControllerImpl::updateStreamOutputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(notFoundBehavior);
	if (!currentConfigurationIndexOpt)
	{
		return;
	}

	auto* const streamDynamicModel = controlledEntity.getModelAccessStrategy().getStreamOutputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
	if (streamDynamicModel)
	{
		if (streamDynamicModel->streamFormat != streamFormat)
		{
			streamDynamicModel->streamFormat = streamFormat;

			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputFormatChanged, this, &controlledEntity, streamIndex, streamFormat);
			}
		}
	}
}

static void updateStreamDynamicInfoData(entity::model::StreamNodeDynamicModel* const streamDynamicModel, entity::model::StreamInfo const& info, std::function<void(std::uint32_t const msrpAccumulatedLatency)> const& msrpAccumulatedLatencyChangedHandler, std::function<void(entity::model::StreamDynamicInfo const& streamDynamicInfo)> const& dynamicInfoUpdatedHandler) noexcept
{
	// Make a copy (or create if first time), we'll move it back later
	auto dynamicInfo = streamDynamicModel->streamDynamicInfo ? *streamDynamicModel->streamDynamicInfo : decltype(streamDynamicModel->streamDynamicInfo)::value_type{};

	// Update each field
	dynamicInfo.isClassB = info.streamInfoFlags.test(entity::StreamInfoFlag::ClassB);
	dynamicInfo.hasSavedState = info.streamInfoFlags.test(entity::StreamInfoFlag::SavedState);
	dynamicInfo.doesSupportEncrypted = info.streamInfoFlags.test(entity::StreamInfoFlag::SupportsEncrypted);
	dynamicInfo.arePdusEncrypted = info.streamInfoFlags.test(entity::StreamInfoFlag::EncryptedPdu);
	dynamicInfo.hasSrpRegistrationFailed = info.streamInfoFlags.test(entity::StreamInfoFlag::SrpRegistrationFailed);
	dynamicInfo._streamInfoFlags = info.streamInfoFlags;

	if (info.streamInfoFlags.test(entity::StreamInfoFlag::StreamIDValid))
	{
		dynamicInfo.streamID = info.streamID;
	}
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::MsrpAccLatValid))
	{
		dynamicInfo.msrpAccumulatedLatency = info.msrpAccumulatedLatency;

		// Call msrpAccumulatedLatencyChangedHandler handler
		utils::invokeProtectedHandler(msrpAccumulatedLatencyChangedHandler, *dynamicInfo.msrpAccumulatedLatency);
	}
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::StreamDestMacValid))
	{
		dynamicInfo.streamDestMac = info.streamDestMac;
	}
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::MsrpFailureValid))
	{
		dynamicInfo.msrpFailureCode = info.msrpFailureCode;
		dynamicInfo.msrpFailureBridgeID = info.msrpFailureBridgeID;
	}
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::StreamVlanIDValid))
	{
		dynamicInfo.streamVlanID = info.streamVlanID;
	}
	// Milan 1.0 additions - Only replace if we have the extended info in the payload (otherwise we'll keep the previous value)
	if (info.streamInfoFlagsEx)
	{
		dynamicInfo.streamInfoFlagsEx = info.streamInfoFlagsEx;
	}
	if (info.probingStatus)
	{
		dynamicInfo.probingStatus = info.probingStatus;
	}
	if (info.acmpStatus)
	{
		dynamicInfo.acmpStatus = info.acmpStatus;
	}

	// Move the data back
	streamDynamicModel->streamDynamicInfo = std::move(dynamicInfo);

	// Call dynamicInfoUpdatedHandler handler
	utils::invokeProtectedHandler(dynamicInfoUpdatedHandler, *streamDynamicModel->streamDynamicInfo);
}

static bool computeIsOverLatency(std::chrono::nanoseconds const& presentationTimeOffset, std::optional<std::uint32_t> const& msrpAccumulatedLatency) noexcept
{
	// If we have msrpAccumulatedLatency
	if (msrpAccumulatedLatency)
	{
		return *msrpAccumulatedLatency > static_cast<std::decay_t<decltype(msrpAccumulatedLatency)>::value_type>(presentationTimeOffset.count());
	}

	return false;
}

void ControllerImpl::updateStreamInputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, bool const streamFormatRequired, bool const milanExtendedRequired, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto hasStreamFormat = info.streamInfoFlags.test(entity::StreamInfoFlag::StreamFormatValid);

	// Try to detect non compliant entities
	if (streamFormatRequired)
	{
		// No StreamFormatValid bit
		if (!hasStreamFormat)
		{
			removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4.15/7.4.16", "StreamFormatValid bit not set in STREAM_INFO response");
			// Check if we have something that looks like a valid streamFormat in the field
			auto const formatType = entity::model::StreamFormatInfo::create(info.streamFormat)->getType();
			if (formatType != entity::model::StreamFormatInfo::Type::None && formatType != entity::model::StreamFormatInfo::Type::Unsupported)
			{
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "StreamFormatValid bit not set but stream_format field appears to contain a valid value in STREAM_INFO response");
			}
		}
		// Or Invalid StreamFormat
		else if (!info.streamFormat)
		{
			hasStreamFormat = false;
			removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4.15/7.4.16", "StreamFormatValid bit set but invalid stream_format field in STREAM_INFO response");
		}
	}

	// Set some compatibility related variables
	auto isImplementingMilan = false;
	auto isImplementingMilanButLessThan1_3 = false;
	{
		auto const milanInfo = controlledEntity.getMilanInfo();
		if (milanInfo)
		{
			isImplementingMilan = milanInfo->specificationVersion >= entity::model::MilanVersion{ 1, 0 };
			if (milanInfo->specificationVersion >= entity::model::MilanVersion{ 1, 0 } && milanInfo->specificationVersion < entity::model::MilanVersion{ 1, 3 })
			{
				isImplementingMilanButLessThan1_3 = true;
			}
		}
	}

	// If implementing Milan, check some mandatory values for flags
	if (isImplementingMilan)
	{
		// ClEntriesValid must not be set
		if (info.streamInfoFlags.test(entity::StreamInfoFlag::ClEntriesValid))
		{
			// Was a reserved field in Milan < 1.3
			if (isImplementingMilanButLessThan1_3)
			{
				// Do not downgrade the Milan compatibility to not penalize too much a Milan device that have passed the Milan 1.2 compliance test, just add a warning flag
				setMilanWarningCompatibilityFlag(this, controlledEntity, "Milan 1.2 - 5.4.2.10.1", "StreamInfoFlag bit 24 is reserved and must be set to 0");
			}
			else
			{
				removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.4.2.10.1", "StreamInfoFlag CL_ENTRIES_VALID must not be set");
			}
		}
	}

	// If Milan Extended Information is required (for GetStreamInfo, not SetStreamInfo) and entity is Milan compatible, check if it's present
	// This is only required for Milan devices up to 1.2, Milan 1.3 and later devices should always send the IEEE variants
	if (milanExtendedRequired && isImplementingMilanButLessThan1_3)
	{
		if (!info.streamInfoFlagsEx || !info.probingStatus || !info.acmpStatus)
		{
			removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.2 - 5.4.2.9/5.4.2.10", "Milan mandatory extended GET_STREAM_INFO not found");
		}
	}

	// Update each individual part of StreamInfo
	if (hasStreamFormat)
	{
		updateStreamInputFormat(controlledEntity, streamIndex, info.streamFormat, notFoundBehavior);
	}
	updateStreamInputRunningStatus(controlledEntity, streamIndex, !info.streamInfoFlags.test(entity::StreamInfoFlag::StreamingWait), notFoundBehavior);

	// According to clarification (from IEEE1722.1 call) a device should always send the complete, up-to-date, status in a GET/SET_STREAM_INFO response (either unsolicited or not)
	// This means that we should always replace the previously stored StreamInfo data with the last one received
	// Unfortunately it proves very difficult to do so for some devices (like when receiving a SET_STREAM_INFO with only one field set, it must generate a GET_STREAM_INFO with all fields set)
	// So we'll retrieve the current StreamDynamicInfo and update it with the new data

	// Retrieve StreamDynamicInfo
	auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const streamDynamicModel = controlledEntity.getModelAccessStrategy().getStreamInputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
		if (streamDynamicModel)
		{
			updateStreamDynamicInfoData(streamDynamicModel, info,
				[this, &controlledEntity, streamIndex](std::uint32_t const msrpAccumulatedLatency)
				{
					// Check for Diagnostics - Latency Error
					{
						// Only if the entity has been advertised, onPreAdvertiseEntity will take care of the non-advertised ones later
						if (controlledEntity.wasAdvertised())
						{
							auto isOverLatency = false;

							// Only if Latency is greater than 0
							if (msrpAccumulatedLatency > 0)
							{
								auto const& sink = controlledEntity.getSinkConnectionInformation(streamIndex);

								// If the Stream is Connected, search for the Talker we are connected to
								if (sink.state == entity::model::StreamInputConnectionInfo::State::Connected)
								{
									// Take a "scoped locked" shared copy of the ControlledEntity // Only process advertised entities, onPreAdvertiseEntity will take care of the non-advertised ones later
									auto talkerEntity = getControlledEntityImplGuard(sink.talkerStream.entityID, true);

									if (talkerEntity)
									{
										auto const& talker = *talkerEntity;

										try
										{
											auto const& talkerStreamOutputNode = talker.getStreamOutputNode(talker.getCurrentConfigurationIndex(), sink.talkerStream.streamIndex);
											isOverLatency = computeIsOverLatency(talkerStreamOutputNode.dynamicModel.presentationTimeOffset, msrpAccumulatedLatency);
										}
										catch (ControlledEntity::Exception const&)
										{
											// Ignore Exception
										}
									}
								}
							}

							updateStreamInputLatency(controlledEntity, streamIndex, isOverLatency);
						}
					}
				},
				[this, &controlledEntity, streamIndex](entity::model::StreamDynamicInfo const& streamDynamicInfo)
				{
					// Entity was advertised to the user, notify observers
					if (controlledEntity.wasAdvertised())
					{
						notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputDynamicInfoChanged, this, &controlledEntity, streamIndex, streamDynamicInfo);
					}
				});
		}
	}
}

void ControllerImpl::updateStreamOutputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, bool const streamFormatRequired, bool const milanExtendedRequired, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto hasStreamFormat = info.streamInfoFlags.test(entity::StreamInfoFlag::StreamFormatValid);

	// Try to detect non compliant entities
	if (streamFormatRequired)
	{
		// No StreamFormatValid bit
		if (!hasStreamFormat)
		{
			removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4.15/7.4.16", "StreamFormatValid bit not set in STREAM_INFO response");
			// Check if we have something that looks like a valid streamFormat in the field
			auto const formatType = entity::model::StreamFormatInfo::create(info.streamFormat)->getType();
			if (formatType != entity::model::StreamFormatInfo::Type::None && formatType != entity::model::StreamFormatInfo::Type::Unsupported)
			{
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "StreamFormatValid bit not set but stream_format field appears to contain a valid value in STREAM_INFO response");
			}
		}
		// Or Invalid StreamFormat
		else if (!info.streamFormat)
		{
			hasStreamFormat = false;
			removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4.15/7.4.16", "StreamFormatValid bit set but invalid stream_format field in GET_STREAM_INFO response");
		}
	}

	// Set some compatibility related variables
	auto isImplementingMilan = false;
	auto isImplementingMilanButLessThan1_3 = false;
	{
		auto const milanInfo = controlledEntity.getMilanInfo();
		if (milanInfo)
		{
			isImplementingMilan = milanInfo->specificationVersion >= entity::model::MilanVersion{ 1, 0 };
			if (milanInfo->specificationVersion >= entity::model::MilanVersion{ 1, 0 } && milanInfo->specificationVersion < entity::model::MilanVersion{ 1, 3 })
			{
				isImplementingMilanButLessThan1_3 = true;
			}
		}
	}

	// If implementing Milan, check some mandatory values for flags
	if (isImplementingMilan)
	{
		// ClEntriesValid must not be set
		if (info.streamInfoFlags.test(entity::StreamInfoFlag::ClEntriesValid))
		{
			// Was a reserved field in Milan < 1.3
			if (isImplementingMilanButLessThan1_3)
			{
				// Do not downgrade the Milan compatibility to not penalize too much a Milan device that have passed the Milan 1.2 compliance test, just add a warning flag
				setMilanWarningCompatibilityFlag(this, controlledEntity, "Milan 1.2 - 5.4.2.10.1", "StreamInfoFlag bit 24 is reserved and must be set to 0");
			}
			else
			{
				removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.4.2.10.1", "StreamInfoFlag CL_ENTRIES_VALID must not be set");
			}
		}
	}

	// If Milan Extended Information is required (for GetStreamInfo, not SetStreamInfo) and entity is Milan compatible, check if it's present
	// This is only required for Milan devices up to 1.2, Milan 1.3 and later devices should always send the IEEE variants
	if (milanExtendedRequired && isImplementingMilanButLessThan1_3)
	{
		if (!info.streamInfoFlagsEx || !info.probingStatus || !info.acmpStatus)
		{
			removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.2 - 5.4.2.9/5.4.2.10", "Milan mandatory extended GET_STREAM_INFO not found");
		}
	}

	// Update each individual part of StreamInfo
	if (hasStreamFormat)
	{
		updateStreamOutputFormat(controlledEntity, streamIndex, info.streamFormat, notFoundBehavior);
	}
	updateStreamOutputRunningStatus(controlledEntity, streamIndex, !info.streamInfoFlags.test(entity::StreamInfoFlag::StreamingWait), notFoundBehavior);

	// According to clarification (from IEEE1722.1 call) a device should always send the complete, up-to-date, status in a GET/SET_STREAM_INFO response (either unsolicited or not)
	// This means that we should always replace the previously stored StreamInfo data with the last one received
	// Unfortunately it proves very difficult to do so for some devices (like when receiving a SET_STREAM_INFO with only one field set, it must generate a GET_STREAM_INFO with all fields set)
	// So we'll retrieve the current StreamDynamicInfo and update it with the new data

	// Retrieve StreamDynamicInfo
	auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const streamDynamicModel = controlledEntity.getModelAccessStrategy().getStreamOutputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
		if (streamDynamicModel)
		{
			updateStreamDynamicInfoData(streamDynamicModel, info,
				[this, &controlledEntity, streamIndex, notFoundBehavior, isImplementingMilanButLessThan1_3](std::uint32_t const msrpAccumulatedLatency)
				{
					// Milan devices use the msrpAccumulatedLatency value to compute the Max Transit Time
					// This changed since Milan 1.3 to use the same mechanism as IEEE 1722.1 devices
					if (isImplementingMilanButLessThan1_3)
					{
						// Forward to updateMaxTransitTime method
						updateMaxTransitTime(controlledEntity, streamIndex, std::chrono::nanoseconds{ msrpAccumulatedLatency }, notFoundBehavior);
					}
				},
				[this, &controlledEntity, streamIndex](entity::model::StreamDynamicInfo const& streamDynamicInfo)
				{
					// Entity was advertised to the user, notify observers
					if (controlledEntity.wasAdvertised())
					{
						notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputDynamicInfoChanged, this, &controlledEntity, streamIndex, streamDynamicInfo);
					}
				});
		}
	}
}

void ControllerImpl::updateEntityName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto* const dynamicModel = controlledEntity.getModelAccessStrategy().getEntityNodeDynamicModel(notFoundBehavior);
	if (dynamicModel)
	{
		if (dynamicModel->entityName != entityName)
		{
			dynamicModel->entityName = entityName;

			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityNameChanged, this, &controlledEntity, entityName);
			}
		}
	}
}

void ControllerImpl::updateEntityGroupName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityGroupName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto* const dynamicModel = controlledEntity.getModelAccessStrategy().getEntityNodeDynamicModel(notFoundBehavior);
	if (dynamicModel)
	{
		if (dynamicModel->groupName != entityGroupName)
		{
			dynamicModel->groupName = entityGroupName;

			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityGroupNameChanged, this, &controlledEntity, entityGroupName);
			}
		}
	}
}

void ControllerImpl::updateConfigurationName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setConfigurationName(configurationIndex, configurationName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onConfigurationNameChanged, this, &controlledEntity, configurationIndex, configurationName);
	}
}

void ControllerImpl::updateAudioUnitName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& audioUnitName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, audioUnitIndex, &TreeModelAccessStrategy::getAudioUnitNodeDynamicModel, audioUnitName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAudioUnitNameChanged, this, &controlledEntity, configurationIndex, audioUnitIndex, audioUnitName);
	}
}

void ControllerImpl::updateStreamInputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, streamIndex, &TreeModelAccessStrategy::getStreamInputNodeDynamicModel, streamInputName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputNameChanged, this, &controlledEntity, configurationIndex, streamIndex, streamInputName);
	}
}

void ControllerImpl::updateStreamOutputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, streamIndex, &TreeModelAccessStrategy::getStreamOutputNodeDynamicModel, streamOutputName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputNameChanged, this, &controlledEntity, configurationIndex, streamIndex, streamOutputName);
	}
}

void ControllerImpl::updateJackInputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackInputName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, jackIndex, &TreeModelAccessStrategy::getJackInputNodeDynamicModel, jackInputName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onJackInputNameChanged, this, &controlledEntity, configurationIndex, jackIndex, jackInputName);
	}
}

void ControllerImpl::updateJackOutputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackOutputName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, jackIndex, &TreeModelAccessStrategy::getJackOutputNodeDynamicModel, jackOutputName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onJackOutputNameChanged, this, &controlledEntity, configurationIndex, jackIndex, jackOutputName);
	}
}

void ControllerImpl::updateAvbInterfaceName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& avbInterfaceName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, avbInterfaceIndex, &TreeModelAccessStrategy::getAvbInterfaceNodeDynamicModel, avbInterfaceName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAvbInterfaceNameChanged, this, &controlledEntity, configurationIndex, avbInterfaceIndex, avbInterfaceName);
	}
}

void ControllerImpl::updateClockSourceName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& clockSourceName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, clockSourceIndex, &TreeModelAccessStrategy::getClockSourceNodeDynamicModel, clockSourceName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onClockSourceNameChanged, this, &controlledEntity, configurationIndex, clockSourceIndex, clockSourceName);
	}
}

void ControllerImpl::updateMemoryObjectName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& memoryObjectName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, memoryObjectIndex, &TreeModelAccessStrategy::getMemoryObjectNodeDynamicModel, memoryObjectName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onMemoryObjectNameChanged, this, &controlledEntity, configurationIndex, memoryObjectIndex, memoryObjectName);
	}
}

void ControllerImpl::updateAudioClusterName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& audioClusterName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, audioClusterIndex, &TreeModelAccessStrategy::getAudioClusterNodeDynamicModel, audioClusterName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAudioClusterNameChanged, this, &controlledEntity, configurationIndex, audioClusterIndex, audioClusterName);
	}
}

void ControllerImpl::updateControlName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, entity::model::AvdeccFixedString const& controlName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, controlIndex, &TreeModelAccessStrategy::getControlNodeDynamicModel, controlName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onControlNameChanged, this, &controlledEntity, configurationIndex, controlIndex, controlName);
	}
}

void ControllerImpl::updateClockDomainName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& clockDomainName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, clockDomainIndex, &TreeModelAccessStrategy::getClockDomainNodeDynamicModel, clockDomainName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onClockDomainNameChanged, this, &controlledEntity, configurationIndex, clockDomainIndex, clockDomainName);
	}
}

void ControllerImpl::updateTimingName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex, entity::model::AvdeccFixedString const& timingName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, timingIndex, &TreeModelAccessStrategy::getTimingNodeDynamicModel, timingName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onTimingNameChanged, this, &controlledEntity, configurationIndex, timingIndex, timingName);
	}
}

void ControllerImpl::updatePtpInstanceName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex, entity::model::AvdeccFixedString const& ptpInstanceName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, ptpInstanceIndex, &TreeModelAccessStrategy::getPtpInstanceNodeDynamicModel, ptpInstanceName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onPtpInstanceNameChanged, this, &controlledEntity, configurationIndex, ptpInstanceIndex, ptpInstanceName);
	}
}

void ControllerImpl::updatePtpPortName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex, entity::model::AvdeccFixedString const& ptpPortName, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, ptpPortIndex, &TreeModelAccessStrategy::getPtpPortNodeDynamicModel, ptpPortName, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onPtpPortNameChanged, this, &controlledEntity, configurationIndex, ptpPortIndex, ptpPortName);
	}
}

void ControllerImpl::updateAssociationID(ControlledEntityImpl& controlledEntity, std::optional<UniqueIdentifier> const associationID, TreeModelAccessStrategy::NotFoundBehavior const /*notFoundBehavior*/) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto& entity = controlledEntity.getEntity();
	auto const previousAssociationID = entity.getAssociationID();
	entity.setAssociationID(associationID);

	// Sanity check
	auto const caps = entity.getEntityCapabilities();

	if (!caps.test(entity::EntityCapability::AssociationIDSupported))
	{
		removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 6.2.2.9", "Entity changed its ASSOCIATION_ID but it said ASSOCIATION_ID_NOT_SUPPORTED in ADPDU");
	}

	// Only do checks if entity was advertised to the user (we already changed the values anyway)
	if (controlledEntity.wasAdvertised())
	{
		// Notify if AssociationID changed
		if (previousAssociationID != associationID)
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityAssociationIDChanged, this, &controlledEntity);
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAssociationIDChanged, this, &controlledEntity, associationID);
		}
	}
}

void ControllerImpl::updateAudioUnitSamplingRate(ControlledEntityImpl& controlledEntity, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setSamplingRate(audioUnitIndex, samplingRate, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAudioUnitSamplingRateChanged, this, &controlledEntity, audioUnitIndex, samplingRate);
	}
}

void ControllerImpl::updateClockSource(ControlledEntityImpl& controlledEntity, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	auto const& e = controlledEntity.getEntity();
	auto const entityID = e.getEntityID();

	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setClockSource(clockDomainIndex, clockSourceIndex, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onClockSourceChanged, this, &controlledEntity, clockDomainIndex, clockSourceIndex);
	}

	// Process all entities and update media clock if needed
	{
		// Lock to protect _controlledEntities
		auto const lg = std::lock_guard{ _lock };

		for (auto& [eid, entity] : _controlledEntities)
		{
			if (entity->wasAdvertised() && entity->getEntity().getEntityCapabilities().test(entity::EntityCapability::AemSupported) && entity->hasAnyConfiguration())
			{
				auto* const configNode = entity->getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
				if (configNode != nullptr)
				{
					for (auto& clockDomainKV : configNode->clockDomains)
					{
						auto& clockDomainNode = clockDomainKV.second;
						// Check if the chain has a node on that clock source changed entity
						for (auto nodeIt = clockDomainNode.mediaClockChain.begin(); nodeIt != clockDomainNode.mediaClockChain.end(); ++nodeIt)
						{
							if (nodeIt->entityID == entityID)
							{
								// Save the domain/stream indexes, we'll continue from it
								auto const continueDomainIndex = nodeIt->clockDomainIndex;
								auto const continueStreamOutputIndex = nodeIt->streamOutputIndex;

								// Remove this node and all following nodes
								clockDomainNode.mediaClockChain.erase(nodeIt, clockDomainNode.mediaClockChain.end());

								// Update the chain starting from this entity
								computeAndUpdateMediaClockChain(*entity, clockDomainNode, entityID, continueDomainIndex, continueStreamOutputIndex, {});
								break;
							}
						}
					}
				}
			}
		}
	}
}

bool ControllerImpl::updateControlValues(ControlledEntityImpl& controlledEntity, entity::model::ControlIndex const controlIndex, MemoryBuffer const& packedControlValues, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(notFoundBehavior);
	if (!currentConfigurationIndexOpt)
	{
		return false;
	}

	auto const* const controlStaticModel = controlledEntity.getModelAccessStrategy().getControlNodeStaticModel(*currentConfigurationIndexOpt, controlIndex, notFoundBehavior);
	if (controlStaticModel)
	{
		auto const controlValueType = controlStaticModel->controlValueType.getType();
		auto const numberOfValues = controlStaticModel->numberOfValues;
		auto const controlValuesOpt = entity::model::unpackDynamicControlValues(packedControlValues, controlValueType, numberOfValues);
		auto const controlType = controlStaticModel->controlType;

		if (controlValuesOpt)
		{
			auto const& controlValues = *controlValuesOpt;

			// Validate ControlValues
			auto const [validationResult, specClause, message] = validateControlValues(controlledEntity.getEntity().getEntityID(), controlIndex, controlType, controlValueType, controlStaticModel->values, controlValues);
			auto isOutOfBounds = false;
			switch (validationResult)
			{
				case DynamicControlValuesValidationResultKind::InvalidValues:
					// Flag the entity as "Not fully IEEE1722.1 compliant"
					removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, specClause, message);
					break;
				case DynamicControlValuesValidationResultKind::CurrentValueOutOfRange:
					isOutOfBounds = true;
					break;
				default:
					break;
			}
			updateControlCurrentValueOutOfBounds(this, controlledEntity, controlIndex, isOutOfBounds);
			controlledEntity.setControlValues(controlIndex, controlValues, notFoundBehavior);

			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onControlValuesChanged, this, &controlledEntity, controlIndex, controlValues);

				// Check for Identify Control
				if (entity::model::StandardControlType::Identify == controlType.getValue() && controlValueType == entity::model::ControlValueType::Type::ControlLinearUInt8 && numberOfValues == 1)
				{
					auto const identifyOpt = getIdentifyControlValue(controlValues);
					if (identifyOpt)
					{
						// Notify
						if (*identifyOpt)
						{
							notifyObserversMethod<Controller::Observer>(&Controller::Observer::onIdentificationStarted, this, &controlledEntity);
						}
						else
						{
							notifyObserversMethod<Controller::Observer>(&Controller::Observer::onIdentificationStopped, this, &controlledEntity);
						}
					}
				}
			}

			return true;
		}
	}
	return false;
}

void ControllerImpl::updateStreamInputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(notFoundBehavior);
	if (!currentConfigurationIndexOpt)
	{
		return;
	}

	auto* const streamDynamicModel = controlledEntity.getModelAccessStrategy().getStreamInputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
	if (streamDynamicModel)
	{
		// Never initialized or changed
		if (!streamDynamicModel->isStreamRunning || *streamDynamicModel->isStreamRunning != isRunning)
		{
			streamDynamicModel->isStreamRunning = isRunning;

			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				// Running status changed, notify observers
				if (isRunning)
				{
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputStarted, this, &controlledEntity, streamIndex);
				}
				else
				{
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputStopped, this, &controlledEntity, streamIndex);
				}
			}
		}
	}
}

void ControllerImpl::updateStreamOutputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(notFoundBehavior);
	if (!currentConfigurationIndexOpt)
	{
		return;
	}

	auto* const streamDynamicModel = controlledEntity.getModelAccessStrategy().getStreamOutputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
	if (streamDynamicModel)
	{
		// Never initialized or changed
		if (!streamDynamicModel->isStreamRunning || *streamDynamicModel->isStreamRunning != isRunning)
		{
			streamDynamicModel->isStreamRunning = isRunning;

			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				// Running status changed, notify observers
				if (isRunning)
				{
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputStarted, this, &controlledEntity, streamIndex);
				}
				else
				{
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputStopped, this, &controlledEntity, streamIndex);
				}
			}
		}
	}
}

void ControllerImpl::updateGptpInformation(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, networkInterface::MacAddress const& macAddress, UniqueIdentifier const& gptpGrandmasterID, std::uint8_t const gptpDomainNumber, TreeModelAccessStrategy::NotFoundBehavior const /*notFoundBehavior*/) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto infoChanged = false;

	try
	{
		// First update gPTP Info in ADP structures
		auto& entity = controlledEntity.getEntity();
		auto const caps = entity.getEntityCapabilities();
		if (caps.test(entity::EntityCapability::GptpSupported))
		{
			// Search which InterfaceInformation matches this AvbInterfaceIndex (searching by Index, or by MacAddress in case the Index was not specified in ADP)
			for (auto& [interfaceIndex, interfaceInfo] : entity.getInterfacesInformation())
			{
				// Do we even have gPTP info on this InterfaceInfo
				if (interfaceInfo.gptpGrandmasterID)
				{
					// Match with the passed AvbInterfaceIndex, or with macAddress if this ADP is the GlobalAvbInterfaceIndex
					if (interfaceIndex == avbInterfaceIndex || (interfaceIndex == entity::Entity::GlobalAvbInterfaceIndex && macAddress == interfaceInfo.macAddress))
					{
						// Alter InterfaceInfo with new gPTP info
						if (*interfaceInfo.gptpGrandmasterID != gptpGrandmasterID || *interfaceInfo.gptpDomainNumber != gptpDomainNumber)
						{
							interfaceInfo.gptpGrandmasterID = gptpGrandmasterID;
							interfaceInfo.gptpDomainNumber = gptpDomainNumber;
							infoChanged |= true;
						}
					}
				}
			}
		}

		// If AEM is supported
		auto const isAemSupported = entity.getEntityCapabilities().test(entity::EntityCapability::AemSupported);
		if (isAemSupported && controlledEntity.hasAnyConfiguration())
		{
			// Then update gPTP Info in existing AvbDescriptors (don't create if not created yet)
			auto* const configurationNode = controlledEntity.getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			if (configurationNode != nullptr)
			{
				for (auto& [interfaceIndex, avbInterfaceNode] : configurationNode->avbInterfaces)
				{
					// Match with the passed AvbInterfaceIndex, or with macAddress if passed AvbInterfaceIndex is the GlobalAvbInterfaceIndex
					if (interfaceIndex == avbInterfaceIndex || (avbInterfaceIndex == entity::Entity::GlobalAvbInterfaceIndex && macAddress == avbInterfaceNode.dynamicModel.macAddress))
					{
						// Alter InterfaceInfo with new gPTP info
						if (avbInterfaceNode.dynamicModel.gptpGrandmasterID != gptpGrandmasterID || avbInterfaceNode.dynamicModel.gptpDomainNumber != gptpDomainNumber)
						{
							avbInterfaceNode.dynamicModel.gptpGrandmasterID = gptpGrandmasterID;
							avbInterfaceNode.dynamicModel.gptpDomainNumber = gptpDomainNumber;
							infoChanged |= true;
						}
					}
				}
			}
		}

		// Check for Diagnostics - Redundancy Warning
		checkRedundancyWarningDiagnostics(this, controlledEntity);
	}
	catch (ControlledEntity::Exception const&)
	{
		AVDECC_ASSERT(false, "Unexpected exception");
	}

	// Only do checks if entity was advertised to the user (we already changed the values anyway)
	if (controlledEntity.wasAdvertised())
	{
		// Info changed
		if (infoChanged)
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onGptpChanged, this, &controlledEntity, avbInterfaceIndex, gptpGrandmasterID, gptpDomainNumber);
		}
	}
}

void ControllerImpl::updateAvbInfo(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Build AvbInterfaceInfo structure
	auto const avbInterfaceInfo = entity::model::AvbInterfaceInfo{ info.propagationDelay, info.flags, info.mappings };

	// Update AvbInterfaceInfo
	auto const previousInfo = controlledEntity.setAvbInterfaceInfo(avbInterfaceIndex, avbInterfaceInfo, notFoundBehavior);

	// Only do checks if entity was advertised to the user (we already changed the values anyway)
	if (controlledEntity.wasAdvertised())
	{
		// Info changed
		if (previousInfo != avbInterfaceInfo)
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAvbInterfaceInfoChanged, this, &controlledEntity, avbInterfaceIndex, avbInterfaceInfo);
		}
	}

	// Update gPTP info
	auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto const* const avbInterfaceDynamicModel = controlledEntity.getModelAccessStrategy().getAvbInterfaceNodeDynamicModel(*currentConfigurationIndexOpt, avbInterfaceIndex, notFoundBehavior);
		if (avbInterfaceDynamicModel)
		{
			updateGptpInformation(controlledEntity, avbInterfaceIndex, avbInterfaceDynamicModel->macAddress, info.gptpGrandmasterID, info.gptpDomainNumber, notFoundBehavior);
		}
	}
}

void ControllerImpl::updateAsPath(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AsPath const& asPath, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const previousPath = controlledEntity.setAsPath(avbInterfaceIndex, asPath, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		// Changed
		if (previousPath != asPath)
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAsPathChanged, this, &controlledEntity, avbInterfaceIndex, asPath);
		}
	}
}

void ControllerImpl::updateAvbInterfaceLinkStatus(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, ControlledEntity::InterfaceLinkStatus const linkStatus) noexcept
{
	auto const previousLinkStatus = controlledEntity.setAvbInterfaceLinkStatus(avbInterfaceIndex, linkStatus);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		// Changed
		if (previousLinkStatus != linkStatus)
		{
			controller->notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAvbInterfaceLinkStatusChanged, controller, &controlledEntity, avbInterfaceIndex, linkStatus);
		}
	}
}

void ControllerImpl::updateEntityCounters(ControlledEntityImpl& controlledEntity, entity::EntityCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Get previous counters
	auto* const entityCounters = controlledEntity.getEntityCounters(notFoundBehavior);
	if (entityCounters)
	{
		// Update (or set) counters
		for (auto counter : validCounters)
		{
			(*entityCounters)[counter] = counters[validCounters.getPosition(counter)];
		}

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityCountersChanged, this, &controlledEntity, *entityCounters);
		}
	}
}

void ControllerImpl::updateAvbInterfaceCounters(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::AvbInterfaceCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Get previous counters
	auto* const avbInterfaceCounters = controlledEntity.getAvbInterfaceCounters(avbInterfaceIndex, notFoundBehavior);
	if (avbInterfaceCounters)
	{
		// Update (or set) counters
		for (auto counter : validCounters)
		{
			(*avbInterfaceCounters)[counter] = counters[validCounters.getPosition(counter)];
		}

		// Check for link status update
		checkAvbInterfaceLinkStatus(this, controlledEntity, avbInterfaceIndex, *avbInterfaceCounters);

		// If Milan device, validate counters values
		if (controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
		{
			// LinkDown should either be equal to LinkUp or be one more (Milan 1.3 Clause 5.3.6.3)
			// We are safe to get those counters, check for their presence during first enumeration has already been done
			auto const upValue = counters[validCounters.getPosition(entity::AvbInterfaceCounterValidFlag::LinkUp)];
			auto const downValue = counters[validCounters.getPosition(entity::AvbInterfaceCounterValidFlag::LinkDown)];
			if (upValue != downValue && upValue != (downValue + 1))
			{
				removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.3.6.3", "Invalid LINK_UP / LINK_DOWN counters value on AVB_INTERFACE: " + std::to_string(avbInterfaceIndex) + " (" + std::to_string(upValue) + " / " + std::to_string(downValue) + ")");
			}
		}

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAvbInterfaceCountersChanged, this, &controlledEntity, avbInterfaceIndex, *avbInterfaceCounters);
		}
	}
}

void ControllerImpl::updateClockDomainCounters(ControlledEntityImpl& controlledEntity, entity::model::ClockDomainIndex const clockDomainIndex, entity::ClockDomainCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Get previous counters
	auto* const clockDomainCounters = controlledEntity.getClockDomainCounters(clockDomainIndex, notFoundBehavior);
	if (clockDomainCounters)
	{
		// Update (or set) counters
		for (auto counter : validCounters)
		{
			(*clockDomainCounters)[counter] = counters[validCounters.getPosition(counter)];
		}

		// If Milan device, validate counters values
		if (controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
		{
			// Unlocked should either be equal to Locked or be one more (Milan 1.3 Clause 5.3.11.2)
			// We are safe to get those counters, check for their presence during first enumeration has already been done
			auto const lockedValue = (*clockDomainCounters)[entity::ClockDomainCounterValidFlag::Locked];
			auto const unlockedValue = (*clockDomainCounters)[entity::ClockDomainCounterValidFlag::Unlocked];
			if (lockedValue != unlockedValue && lockedValue != (unlockedValue + 1))
			{
				removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.3.11.2", "Invalid LOCKED / UNLOCKED counters value on CLOCK_DOMAIN: " + std::to_string(clockDomainIndex) + " (" + std::to_string(lockedValue) + " / " + std::to_string(unlockedValue) + ")");
			}
		}

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onClockDomainCountersChanged, this, &controlledEntity, clockDomainIndex, *clockDomainCounters);
		}
	}
}

void ControllerImpl::updateStreamInputCounters(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::StreamInputCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Get previous counters
	auto* const streamCounters = controlledEntity.getStreamInputCounters(streamIndex, notFoundBehavior);
	if (streamCounters)
	{
		// Update (or set) counters
		for (auto counter : validCounters)
		{
			(*streamCounters)[counter] = counters[validCounters.getPosition(counter)];
		}

		// If Milan device, validate counters values
		if (controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
		{
			// MediaUnlocked should either be equal to MediaLocked or be one more (Milan 1.3 Clause 5.3.8.10)
			// We are safe to get those counters, check for their presence during first enumeration has already been done
			auto const lockedValue = (*streamCounters)[entity::StreamInputCounterValidFlag::MediaLocked];
			auto const unlockedValue = (*streamCounters)[entity::StreamInputCounterValidFlag::MediaUnlocked];
			if (lockedValue != unlockedValue && lockedValue != (unlockedValue + 1))
			{
				removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.3.8.10", "Invalid MEDIA_LOCKED / MEDIA_UNLOCKED counters value on STREAM_INPUT: " + std::to_string(streamIndex) + " (" + std::to_string(lockedValue) + " / " + std::to_string(unlockedValue) + ")");
			}
		}

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputCountersChanged, this, &controlledEntity, streamIndex, *streamCounters);
		}
	}
}

entity::model::StreamOutputCounters::CounterType ControllerImpl::getStreamOutputCounterType(ControlledEntityImpl& controlledEntity) noexcept
{
	// Counters type depends on the Milan specification version and other fields
	auto const milanInfo = controlledEntity.getMilanInfo();
	if (milanInfo)
	{
		// At least Milan 1.0, use the Milan type counters
		// This changed since Milan 1.3 to use the same mechanism as IEEE 1722.1 devices
		if (milanInfo->specificationVersion >= entity::model::MilanVersion{ 1, 0 } && milanInfo->specificationVersion < entity::model::MilanVersion{ 1, 3 })
		{
			return entity::model::StreamOutputCounters::CounterType::Milan_12;
		}

		// Check for the TalkerSignalPresence flag, if present use the special SignalPresence counters
		if (milanInfo->featuresFlags.test(entity::MilanInfoFeaturesFlag::TalkerSignalPresence))
		{
			return entity::model::StreamOutputCounters::CounterType::Milan_SignalPresence;
		}

		// Otherwise use the 1722.1 type counters
		return entity::model::StreamOutputCounters::CounterType::IEEE17221_2021;
	}

	// Otherwise use the 1722.1 type counters
	return entity::model::StreamOutputCounters::CounterType::IEEE17221_2021;
}

void ControllerImpl::updateSignalPresenceCounters(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::DescriptorCounter const signalPresence1, entity::model::DescriptorCounter const signalPresence2, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(notFoundBehavior);
	if (!currentConfigurationIndexOpt)
	{
		return;
	}

	auto* const streamDynamicModel = controlledEntity.getModelAccessStrategy().getStreamOutputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
	if (streamDynamicModel)
	{
		// Convert signalPresence counters to SignalPresenceChannels by reversing the bits and combining them into a std::bitset.

		// Convert and reverse bit order
		auto const sp1_reversed = utils::reverseBits(static_cast<std::uint32_t>(signalPresence1)); // signalPresence1: MSB = channel 0, LSB = channel 31
		auto const sp2_reversed = utils::reverseBits(static_cast<std::uint32_t>(signalPresence2)); // signalPresence2: MSB = channel 32, LSB = channel 63

		// Combine into 64-bit value: channels 0-31 in lower 32 bits, channels 32-63 in upper 32 bits
		auto const signalPresence = entity::model::SignalPresenceChannels{ static_cast<entity::model::SignalPresenceChannelsUnderlyingType>(sp1_reversed) | (static_cast<entity::model::SignalPresenceChannelsUnderlyingType>(sp2_reversed) << 32) };

		if (streamDynamicModel->signalPresence != signalPresence)
		{
			streamDynamicModel->signalPresence = signalPresence;

			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputSignalPresenceChanged, this, &controlledEntity, streamIndex, signalPresence);
			}
		}
	}
}

void ControllerImpl::updateStreamOutputCounters(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamOutputCounters const& counters, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Get previous counters
	auto* const streamCounters = controlledEntity.getStreamOutputCounters(streamIndex, notFoundBehavior);
	if (streamCounters)
	{
		// Use operator|= to update the counters (will take care of the type if it's different)
		streamCounters->operator|=(counters);

		// If Milan compatible device, validate counters values
		if (controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
		{
			try // Should not be needed, but just in case
			{
				switch (streamCounters->getCounterType())
				{
					case entity::model::StreamOutputCounters::CounterType::Milan_12: // Milan 1.0 to 1.3 (exclusive)
					{
						auto milan12Counters = streamCounters->getCounters<entity::StreamOutputCounterValidFlagsMilan12>();
						// StreamStop should either be equal to StreamStart or be one more (Milan 1.2 Clause 5.3.7.7)
						// We are safe to get those counters, check for their presence during first enumeration has already been done
						auto const startValue = milan12Counters[entity::StreamOutputCounterValidFlagMilan12::StreamStart];
						auto const stopValue = milan12Counters[entity::StreamOutputCounterValidFlagMilan12::StreamStop];
						if (startValue != stopValue && startValue != (stopValue + 1))
						{
							removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.2 - 5.3.7.7", "Invalid STREAM_START / STREAM_STOP counters value on STREAM_OUTPUT: " + std::to_string(streamIndex) + " (" + std::to_string(startValue) + " / " + std::to_string(stopValue) + ")");
						}
						break;
					}
					case entity::model::StreamOutputCounters::CounterType::IEEE17221_2021: // Milan 1.3 and later
					{
						auto milan13Counters = streamCounters->getCounters<entity::StreamOutputCounterValidFlags17221>();
						// StreamStop should either be equal to StreamStart or be one more (Milan 1.3 Clause 5.3.7.7)
						// We are safe to get those counters, check for their presence during first enumeration has already been done
						auto const startValue = milan13Counters[entity::StreamOutputCounterValidFlag17221::StreamStart];
						auto const stopValue = milan13Counters[entity::StreamOutputCounterValidFlag17221::StreamStop];
						if (startValue != stopValue && startValue != (stopValue + 1))
						{
							removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.3.7.7", "Invalid STREAM_START / STREAM_STOP counters value on STREAM_OUTPUT: " + std::to_string(streamIndex) + " (" + std::to_string(startValue) + " / " + std::to_string(stopValue) + ")");
						}
						break;
					}
					case entity::model::StreamOutputCounters::CounterType::Milan_SignalPresence: // Milan 1.3 and later (With TalkerSignalPresence flag set)
					{
						auto milanSignalPresenceCounters = streamCounters->getCounters<entity::StreamOutputCounterValidFlagsMilanSignalPresence>();
						// StreamStop should either be equal to StreamStart or be one more (Milan 1.3 Clause 5.3.7.7)
						// We are safe to get those counters, check for their presence during first enumeration has already been done
						auto const startValue = milanSignalPresenceCounters[entity::StreamOutputCounterValidFlagMilanSignalPresence::StreamStart];
						auto const stopValue = milanSignalPresenceCounters[entity::StreamOutputCounterValidFlagMilanSignalPresence::StreamStop];
						if (startValue != stopValue && startValue != (stopValue + 1))
						{
							removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.3.7.7", "Invalid STREAM_START / STREAM_STOP counters value on STREAM_OUTPUT: " + std::to_string(streamIndex) + " (" + std::to_string(startValue) + " / " + std::to_string(stopValue) + ")");
						}
						updateSignalPresenceCounters(controlledEntity, streamIndex, milanSignalPresenceCounters[entity::StreamOutputCounterValidFlagMilanSignalPresence::SignalPresence1], milanSignalPresenceCounters[entity::StreamOutputCounterValidFlagMilanSignalPresence::SignalPresence2], notFoundBehavior);
						break;
					}
					default: // Unsupported type
						AVDECC_ASSERT(false, "Unsupported StreamOutputCounters type");
						LOG_CONTROLLER_DEBUG(controlledEntity.getEntity().getEntityID(), "Unsupported StreamOutputCounters type: {}", utils::to_integral(streamCounters->getCounterType()));
						break;
				}
			}
			catch (std::invalid_argument const&)
			{
				removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.3.7.7", "Invalid STREAM_OUTPUT counters type");
			}
		}

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputCountersChanged, this, &controlledEntity, streamIndex, *streamCounters);
		}
	}
}

void ControllerImpl::updateMemoryObjectLength(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto* const memoryObjectNode = controlledEntity.getModelAccessStrategy().getMemoryObjectNode(configurationIndex, memoryObjectIndex, notFoundBehavior);
	if (memoryObjectNode)
	{
		// Validate some fields
		if (length > memoryObjectNode->staticModel.maximumLength)
		{
			removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4.72/7.4.73", "MemoryObject length is greater than maximumLength: " + std::to_string(length) + " > " + std::to_string(memoryObjectNode->staticModel.maximumLength));
			controlledEntity.setIgnoreCachedEntityModel();
		}
	}

	controlledEntity.setMemoryObjectLength(configurationIndex, memoryObjectIndex, length, notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onMemoryObjectLengthChanged, this, &controlledEntity, configurationIndex, memoryObjectIndex, length);
	}
}

void ControllerImpl::updateStreamPortInputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.addStreamPortInputAudioMappings(streamPortIndex, validateMappings<entity::model::DescriptorType::StreamPortInput>(controlledEntity, streamPortIndex, mappings), notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortInputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}

#ifdef ENABLE_AVDECC_FEATURE_CBR
	// Process all added mappings and update channel connections if needed
	{
		auto* const configurationNode = controlledEntity.getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
		if (configurationNode != nullptr)
		{
			auto const* const staticModel = controlledEntity.getModelAccessStrategy().getStreamPortInputNodeStaticModel(configurationNode->descriptorIndex, streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			auto const* const dynamicModel = controlledEntity.getModelAccessStrategy().getStreamPortInputNodeDynamicModel(configurationNode->descriptorIndex, streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			if (staticModel != nullptr && dynamicModel != nullptr)
			{
				// Lock to protect _controlledEntities
				auto const lg = std::lock_guard{ _lock };

				// Get the complete list of mappings (now includes the added ones)
				auto const& allMappings = dynamicModel->dynamicAudioMap;

				// Track processed cluster+channel combinations to avoid duplicates
				auto processedClusters = std::unordered_set<model::ClusterIdentification, model::ClusterIdentification::Hash>{};

				for (auto const& addedMapping : mappings)
				{
					auto const globalClusterIndex = static_cast<entity::model::ClusterIndex>(staticModel->baseCluster + addedMapping.clusterOffset);
					auto const clusterIdentification = model::ClusterIdentification{ globalClusterIndex, addedMapping.clusterChannel };

					// Skip if we already processed this cluster+channel combination
					if (processedClusters.count(clusterIdentification) > 0)
					{
						continue;
					}
					processedClusters.insert(clusterIdentification);

					// Check if the added mapping stream has a redundant partner
					auto const redundantIndex = controlledEntity.getRedundantStreamInputIndex(addedMapping.streamIndex);
					auto primaryMapping = std::optional<entity::model::AudioMapping>{};
					auto secondaryMapping = std::optional<entity::model::AudioMapping>{};

					if (redundantIndex)
					{
						// This is a redundant stream - need to find both primary and secondary mappings
						for (auto const& mapping : allMappings)
						{
							if (mapping.clusterOffset == addedMapping.clusterOffset && mapping.clusterChannel == addedMapping.clusterChannel)
							{
								// Determine if it's primary or secondary
								if (controlledEntity.isRedundantPrimaryStreamInput(mapping.streamIndex))
								{
									primaryMapping = mapping;
								}
								else
								{
									secondaryMapping = mapping;
								}
							}
						}
					}
					else
					{
						// Non-redundant stream, treat as primary
						primaryMapping = addedMapping;
					}

					// Get the matching ChannelConnection (should exist) and update it
					auto const channelConnIt = configurationNode->channelConnections.find(clusterIdentification);
					if (AVDECC_ASSERT_WITH_RET(channelConnIt != configurationNode->channelConnections.end(), "Failed to find ChannelConnection for updated StreamPortInput AudioMapping"))
					{
						auto& channelConnection = channelConnIt->second;
						auto const mappingsInfo = std::make_tuple(!!redundantIndex, primaryMapping, secondaryMapping);
						computeAndUpdateChannelConnectionFromListenerMapping(controlledEntity, *configurationNode, clusterIdentification, mappingsInfo, channelConnection);
					}
				}
			}
		}
	}
#endif // ENABLE_AVDECC_FEATURE_CBR
}

void ControllerImpl::updateStreamPortInputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.removeStreamPortInputAudioMappings(streamPortIndex, validateMappings<entity::model::DescriptorType::StreamPortInput>(controlledEntity, streamPortIndex, mappings), notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortInputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}

#ifdef ENABLE_AVDECC_FEATURE_CBR
	// Process all removed mappings and update channel connections if needed
	{
		auto* const configurationNode = controlledEntity.getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
		if (configurationNode != nullptr)
		{
			auto const* const staticModel = controlledEntity.getModelAccessStrategy().getStreamPortInputNodeStaticModel(configurationNode->descriptorIndex, streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			auto const* const dynamicModel = controlledEntity.getModelAccessStrategy().getStreamPortInputNodeDynamicModel(configurationNode->descriptorIndex, streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			if (staticModel != nullptr && dynamicModel != nullptr)
			{
				// Lock to protect _controlledEntities
				auto const lg = std::lock_guard{ _lock };

				// Get the current list of mappings (after removal)
				auto const& remainingMappings = dynamicModel->dynamicAudioMap;

				// Track processed cluster+channel combinations to avoid duplicates
				auto processedClusters = std::unordered_set<model::ClusterIdentification, model::ClusterIdentification::Hash>{};

				for (auto const& removedMapping : mappings)
				{
					auto const globalClusterIndex = static_cast<entity::model::ClusterIndex>(staticModel->baseCluster + removedMapping.clusterOffset);
					auto const clusterIdentification = model::ClusterIdentification{ globalClusterIndex, removedMapping.clusterChannel };

					// Skip if we already processed this cluster+channel combination
					if (processedClusters.count(clusterIdentification) > 0)
					{
						continue;
					}
					processedClusters.insert(clusterIdentification);

					// Check if there are still mappings (including redundant state) for this cluster+channel after removal
					auto redundantIndex = controlledEntity.getRedundantStreamInputIndex(removedMapping.streamIndex);
					auto primaryMapping = std::optional<entity::model::AudioMapping>{};
					auto secondaryMapping = std::optional<entity::model::AudioMapping>{};

					// If this is a redundant stream, we need to see if the partner mapping still exists
					if (redundantIndex)
					{
						// Look for remaining mappings for this cluster+channel combination
						for (auto const& mapping : remainingMappings)
						{
							if (mapping.clusterOffset == removedMapping.clusterOffset && mapping.clusterChannel == removedMapping.clusterChannel)
							{
								// This is the redundant partner that remains - determine if it's primary or secondary
								if (controlledEntity.isRedundantPrimaryStreamInput(mapping.streamIndex))
								{
									primaryMapping = mapping;
								}
								else
								{
									secondaryMapping = mapping;
								}
								// No need to continue searching
								break;
							}
						}
						// Neither primary nor secondary mapping found, means both were removed
						if (!primaryMapping && !secondaryMapping)
						{
							// No more mappings for this cluster+channel, clear redundantIndex
							redundantIndex = std::nullopt;
						}
					}
					// Non-redundant stream, nothing to do as primaryMapping is already empty if removed

					// Get the matching ChannelConnection (should exist) and update it
					auto const channelConnIt = configurationNode->channelConnections.find(clusterIdentification);
					if (AVDECC_ASSERT_WITH_RET(channelConnIt != configurationNode->channelConnections.end(), "Failed to find ChannelConnection for updated StreamPortInput AudioMapping"))
					{
						auto& channelConnection = channelConnIt->second;
						auto const mappingsInfo = std::make_tuple(!!redundantIndex, primaryMapping, secondaryMapping);
						computeAndUpdateChannelConnectionFromListenerMapping(controlledEntity, *configurationNode, clusterIdentification, mappingsInfo, channelConnection);
					}
				}
			}
		}
	}
#endif // ENABLE_AVDECC_FEATURE_CBR
}

void ControllerImpl::updateStreamPortOutputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.addStreamPortOutputAudioMappings(streamPortIndex, validateMappings<entity::model::DescriptorType::StreamPortOutput>(controlledEntity, streamPortIndex, mappings), notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortOutputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}

#ifdef ENABLE_AVDECC_FEATURE_CBR
	// Process all entities and update channel connections if needed
	{
		// Get some information about the controlled entity
		auto const entityID = controlledEntity.getEntity().getEntityID();
		auto baseClusterIndexOpt = std::optional<entity::model::ClusterIndex>{};
		{
			auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			if (currentConfigurationIndexOpt)
			{
				auto const* const staticModel = controlledEntity.getModelAccessStrategy().getStreamPortOutputNodeStaticModel(*currentConfigurationIndexOpt, streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
				if (staticModel)
				{
					baseClusterIndexOpt = staticModel->baseCluster;
				}
			}
		}

		if (AVDECC_ASSERT_WITH_RET(baseClusterIndexOpt, "Failed to get StreamPortOutput baseClusterIndex"))
		{
			auto const baseClusterIndex = *baseClusterIndexOpt;

			// Lock to protect _controlledEntities
			auto const lg = std::lock_guard{ _lock };

			for (auto& [eid, entity] : _controlledEntities)
			{
				if (eid != entityID && entity->wasAdvertised() && entity->getEntity().getEntityCapabilities().test(entity::EntityCapability::AemSupported) && entity->hasAnyConfiguration())
				{
					auto* const configNode = entity->getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					if (configNode != nullptr)
					{
						computeAndUpdateChannelConnectionsFromTalkerMappings(*entity, entityID, baseClusterIndex, mappings, configNode->channelConnections, false);
					}
				}
			}
		}
	}
#endif // ENABLE_AVDECC_FEATURE_CBR
}

void ControllerImpl::updateStreamPortOutputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.removeStreamPortOutputAudioMappings(streamPortIndex, validateMappings<entity::model::DescriptorType::StreamPortOutput>(controlledEntity, streamPortIndex, mappings), notFoundBehavior);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortOutputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}

#ifdef ENABLE_AVDECC_FEATURE_CBR
	// Process all entities and update channel connections if needed
	{
		// Get some information about the controlled entity
		auto const entityID = controlledEntity.getEntity().getEntityID();
		auto baseClusterIndexOpt = std::optional<entity::model::ClusterIndex>{};
		{
			auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			if (currentConfigurationIndexOpt)
			{
				auto const* const staticModel = controlledEntity.getModelAccessStrategy().getStreamPortOutputNodeStaticModel(*currentConfigurationIndexOpt, streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
				if (staticModel)
				{
					baseClusterIndexOpt = staticModel->baseCluster;
				}
			}
		}

		if (AVDECC_ASSERT_WITH_RET(baseClusterIndexOpt, "Failed to get StreamPortOutput baseClusterIndex"))
		{
			auto const baseClusterIndex = *baseClusterIndexOpt;

			// Lock to protect _controlledEntities
			auto const lg = std::lock_guard{ _lock };

			for (auto& [eid, entity] : _controlledEntities)
			{
				if (eid != entityID && entity->wasAdvertised() && entity->getEntity().getEntityCapabilities().test(entity::EntityCapability::AemSupported) && entity->hasAnyConfiguration())
				{
					auto* const configNode = entity->getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
					if (configNode != nullptr)
					{
						computeAndUpdateChannelConnectionsFromTalkerMappings(*entity, entityID, baseClusterIndex, mappings, configNode->channelConnections, true);
					}
				}
			}
		}
	}
#endif // ENABLE_AVDECC_FEATURE_CBR
}

void ControllerImpl::updateOperationStatus(ControlledEntityImpl& controlledEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, std::uint16_t const percentComplete, TreeModelAccessStrategy::NotFoundBehavior const /*notFoundBehavior*/) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		if (percentComplete == 0) /* IEEE1722.1-2013 Clause 7.4.55.2 */
		{
			// Failure
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onOperationCompleted, this, &controlledEntity, descriptorType, descriptorIndex, operationID, true);
		}
		else if (percentComplete == 1000)
		{
			// Completed successfully
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onOperationCompleted, this, &controlledEntity, descriptorType, descriptorIndex, operationID, false);
		}
		else if (percentComplete == 0xFFFF)
		{
			// Unknown progress but continuing
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onOperationProgress, this, &controlledEntity, descriptorType, descriptorIndex, operationID, -1.0f);
		}
		else if (percentComplete < 1000)
		{
			// In progress
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onOperationProgress, this, &controlledEntity, descriptorType, descriptorIndex, operationID, percentComplete / 10.0f);
		}
		else
		{
			// Invalid value
			AVDECC_ASSERT(percentComplete > 1000, "Unknown percentComplete value");
		}
	}
}

void ControllerImpl::updateMaxTransitTime(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, std::chrono::nanoseconds const& maxTransitTime, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(notFoundBehavior);
	if (!currentConfigurationIndexOpt)
	{
		return;
	}

	auto* const streamDynamicModel = controlledEntity.getModelAccessStrategy().getStreamOutputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
	if (streamDynamicModel)
	{
		// Create streamDynamicInfo if not already created
		if (!streamDynamicModel->streamDynamicInfo.has_value())
		{
			streamDynamicModel->streamDynamicInfo = entity::model::StreamDynamicInfo{};
			// Should probably not happen if the entity has been advertised
			if (controlledEntity.wasAdvertised())
			{
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Received SET_MAX_TRANSIT_TIME update");
			}
		}

		// Value changed
		if (streamDynamicModel->presentationTimeOffset != maxTransitTime)
		{
			// Update maxTransitTime
			streamDynamicModel->presentationTimeOffset = maxTransitTime;

			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onMaxTransitTimeChanged, this, &controlledEntity, streamIndex, maxTransitTime);
			}
		}
	}
}

void ControllerImpl::updateRedundancyWarning(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, bool const isWarning) noexcept
{
	auto& diags = controlledEntity.getDiagnostics();
	auto const notify = diags.redundancyWarning != isWarning;

	diags.redundancyWarning = isWarning;

	// Entity was advertised to the user, notify observers
	if (controller && notify && controlledEntity.wasAdvertised())
	{
		AVDECC_ASSERT(controller->_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");
		controller->notifyObserversMethod<Controller::Observer>(&Controller::Observer::onDiagnosticsChanged, controller, &controlledEntity, diags);
	}
}

void ControllerImpl::updateControlCurrentValueOutOfBounds(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, entity::model::ControlIndex const controlIndex, bool const isOutOfBounds) noexcept
{
	auto& diags = controlledEntity.getDiagnostics();
	auto const previouslyInError = diags.controlCurrentValueOutOfBounds.count(controlIndex) > 0;

	// State changed
	if (isOutOfBounds != previouslyInError)
	{
		// Was not in the list and now needs to be
		if (isOutOfBounds)
		{
			diags.controlCurrentValueOutOfBounds.insert(controlIndex);
		}
		// Was in the list and now needs to be removed
		else
		{
			diags.controlCurrentValueOutOfBounds.erase(controlIndex);
		}

		// Entity was advertised to the user, notify observers
		if (controller && controlledEntity.wasAdvertised())
		{
			AVDECC_ASSERT(controller->_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");
			controller->notifyObserversMethod<Controller::Observer>(&Controller::Observer::onDiagnosticsChanged, controller, &controlledEntity, diags);
		}
	}
}

void ControllerImpl::updateStreamInputLatency(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isOverLatency) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto& diags = controlledEntity.getDiagnostics();
	auto const previouslyInError = diags.streamInputOverLatency.count(streamIndex) > 0;

	// State changed
	if (isOverLatency != previouslyInError)
	{
		// Was not in the list and now needs to be
		if (isOverLatency)
		{
			diags.streamInputOverLatency.insert(streamIndex);
		}
		// Was in the list and now needs to be removed
		else
		{
			diags.streamInputOverLatency.erase(streamIndex);
		}

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onDiagnosticsChanged, this, &controlledEntity, diags);
		}
	}
}

void ControllerImpl::updateSystemUniqueID(ControlledEntityImpl& controlledEntity, UniqueIdentifier const uniqueID, entity::model::AvdeccFixedString const& systemName) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setSystemUniqueID(uniqueID, systemName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onSystemUniqueIDChanged, this, &controlledEntity, uniqueID, systemName);
	}
}

void ControllerImpl::updateMediaClockReferenceInfo(ControlledEntityImpl& controlledEntity, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::DefaultMediaClockReferencePriority const defaultPriority, entity::model::MediaClockReferenceInfo const& info, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(notFoundBehavior);
	if (!currentConfigurationIndexOpt)
	{
		return;
	}

	auto* const domainNode = controlledEntity.getModelAccessStrategy().getClockDomainNode(*currentConfigurationIndexOpt, clockDomainIndex, notFoundBehavior);
	if (domainNode)
	{
		auto const advertised = controlledEntity.wasAdvertised();

		// Only validate if the entity has been advertised
		// Although we may loose some device inconsistencies during enumeration, it's easier to handle this way
		// Because this method may be called from onGetMediaClockReferenceInfoResult (response to enumeration query) or from onMediaClockReferenceInfoChanged (unsolicited response) which may actually be received before the controller sends the enumeration query).
		// And we have no way to detect which one is the case here because the defaultMediaClockPriority variable is construct-initialized to 'Default'
		if (advertised)
		{
			// Check if defaultPriority has changed after the entity has been advertised this is a critical error from the device
			if (domainNode->staticModel.defaultMediaClockPriority != defaultPriority)
			{
				ControllerImpl::decreaseMilanCompatibilityVersion(this, controlledEntity, entity::model::MilanVersion{ 1, 0 }, "Milan 1.3 - 5.4.4.4/5.4.4.5", "Read-only 'DefaultMediaClockReferencePriority' value changed for CLOCK_DOMAIN: " + std::to_string(clockDomainIndex) + " (" + std::to_string(utils::to_integral(domainNode->staticModel.defaultMediaClockPriority)) + " -> " + std::to_string(utils::to_integral(defaultPriority)) + ")");
			}
		}
		domainNode->staticModel.defaultMediaClockPriority = defaultPriority;

		// Info changed
		if (domainNode->dynamicModel.mediaClockReferenceInfo != info)
		{
			domainNode->dynamicModel.mediaClockReferenceInfo = info;

			// Entity was advertised to the user, notify observers
			if (advertised)
			{
				// Notify observers
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onMediaClockReferenceInfoChanged, this, &controlledEntity, clockDomainIndex, info);
			}
		}
	}
}

void ControllerImpl::updateStreamInputInfoEx(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInputInfoEx const& streamInputInfoEx, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(notFoundBehavior);
	if (!currentConfigurationIndexOpt)
	{
		return;
	}

	auto* const streamDynamicModel = controlledEntity.getModelAccessStrategy().getStreamInputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
	if (streamDynamicModel)
	{
		// Create streamDynamicInfo if not already created
		if (!streamDynamicModel->streamDynamicInfo.has_value())
		{
			streamDynamicModel->streamDynamicInfo = entity::model::StreamDynamicInfo{};
			// Should probably not happen if the entity has been advertised
			if (controlledEntity.wasAdvertised())
			{
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Received GET_STREAM_INPUT_INFO_EX update");
			}
		}

		// Update connection status
		handleListenerStreamStateNotification(streamInputInfoEx.talkerStream, entity::model::StreamIdentification{ controlledEntity.getEntity().getEntityID(), streamIndex }, !!streamInputInfoEx.talkerStream.entityID, std::nullopt, true);

		// Update Milan specific fields in StreamDynamicInfo
		auto const dynamicInfoChanged = (streamDynamicModel->streamDynamicInfo->probingStatus != streamInputInfoEx.probingStatus) || (streamDynamicModel->streamDynamicInfo->acmpStatus != streamInputInfoEx.acmpStatus);
		if (dynamicInfoChanged)
		{
			streamDynamicModel->streamDynamicInfo->probingStatus = streamInputInfoEx.probingStatus;
			streamDynamicModel->streamDynamicInfo->acmpStatus = streamInputInfoEx.acmpStatus;
			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputDynamicInfoChanged, this, &controlledEntity, streamIndex, *(streamDynamicModel->streamDynamicInfo));
			}
		}
	}
}


/* ************************************************************ */
/* Private methods                                              */
/* ************************************************************ */
entity::model::ControlValues ControllerImpl::makeIdentifyControlValues(bool const isEnabled) noexcept
{
	auto values = entity::model::LinearValues<entity::model::LinearValueDynamic<std::uint8_t>>{};

	values.addValue({ isEnabled ? std::uint8_t{ 0xFF } : std::uint8_t{ 0x00 } });

	return entity::model::ControlValues{ values };
}

std::optional<bool> ControllerImpl::getIdentifyControlValue(entity::model::ControlValues const& values) noexcept
{
	AVDECC_ASSERT(values.areDynamicValues() && values.getType() == entity::model::ControlValueType::Type::ControlLinearUInt8, "Doesn't look like Identify Control Value");
	try
	{
		if (values.size() == 1)
		{
			auto const dynamicValues = values.getValues<entity::model::LinearValues<entity::model::LinearValueDynamic<std::uint8_t>>>(); // We have to store the copy or it will go out of scope if using it directly in the range-based loop
			auto const& value = dynamicValues.getValues()[0];
			if (value.currentValue == 0)
			{
				return false;
			}
			else if (value.currentValue == 255)
			{
				return true;
			}
		}
	}
	catch (...)
	{
	}
	return std::nullopt;
}

void ControllerImpl::checkAvbInterfaceLinkStatus(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInterfaceCounters const& avbInterfaceCounters) noexcept
{
	// We must not access avbInterfaceCounters through operator[] or it will create a value if it does not exist. We want the LinkStatus update to be available even for non Milan devices
	auto const upIt = avbInterfaceCounters.find(entity::AvbInterfaceCounterValidFlag::LinkUp);
	auto const downIt = avbInterfaceCounters.find(entity::AvbInterfaceCounterValidFlag::LinkDown);
	if (upIt != avbInterfaceCounters.end() && downIt != avbInterfaceCounters.end())
	{
		auto const upValue = upIt->second;
		auto const downValue = downIt->second;
		auto const isUp = upValue == (downValue + 1);
		updateAvbInterfaceLinkStatus(controller, controlledEntity, avbInterfaceIndex, isUp ? ControlledEntity::InterfaceLinkStatus::Up : ControlledEntity::InterfaceLinkStatus::Down);
	}
}

void ControllerImpl::checkRedundancyWarningDiagnostics(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity) noexcept
{
	auto isWarning = false;

	// Only for a Milan redundant device
	if (controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan) && controlledEntity.isMilanRedundant())
	{
		// Check if AVB_INTERFACE_0 and AVB_INTERFACE_1 have the same gPTP
		try
		{
			auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(TreeModelAccessStrategy::NotFoundBehavior::Throw);
			auto const& avbInterfaceNode0 = controlledEntity.getAvbInterfaceNode(*currentConfigurationIndexOpt, entity::model::AvbInterfaceIndex{ 0u });
			auto const& avbInterfaceNode1 = controlledEntity.getAvbInterfaceNode(*currentConfigurationIndexOpt, entity::model::AvbInterfaceIndex{ 1u });
			isWarning = avbInterfaceNode0.dynamicModel.gptpGrandmasterID == avbInterfaceNode1.dynamicModel.gptpGrandmasterID;
		}
		catch (ControlledEntity::Exception const&)
		{
			setMilanWarningCompatibilityFlag(nullptr, controlledEntity, "Milan 1.3 - 8.2.2", "Entity is declared Milan Redundant but does not have AVB_INTERFACE_0 and AVB_INTERFACE_1");
		}
	}

	updateRedundancyWarning(controller, controlledEntity, isWarning);
}

void ControllerImpl::removeExclusiveAccessTokens(UniqueIdentifier const entityID, ExclusiveAccessToken::AccessType const type) const noexcept
{
	auto tokensToInvalidate = decltype(_exclusiveAccessTokens)::mapped_type{};

	// PersistentAcquire and Acquire should be handled identically
	auto typeToCheck = type;
	if (typeToCheck == ExclusiveAccessToken::AccessType::PersistentAcquire)
	{
		typeToCheck = ExclusiveAccessToken::AccessType::Acquire;
	}

	// Remove all matching ExclusiveAccessTokens, under lock
	{
		// Lock to protect data members
		auto const lg = std::lock_guard{ _lock };

		// Get tokens for specified EntityID
		if (auto const tokensIt = _exclusiveAccessTokens.find(entityID); tokensIt != _exclusiveAccessTokens.end())
		{
			auto& tokens = tokensIt->second;
			// Remove tokens matching type
			for (auto it = tokens.begin(); it != tokens.end(); /* Iterate inside the loop */)
			{
				auto* const token = *it;

				// PersistentAcquire and Acquire should be handled identically
				auto tokenType = token->getAccessType();
				if (tokenType == ExclusiveAccessToken::AccessType::PersistentAcquire)
				{
					tokenType = ExclusiveAccessToken::AccessType::Acquire;
				}
				if (tokenType == type)
				{
					tokensToInvalidate.insert(token);
					// Remove from the list
					it = tokens.erase(it);
				}
				else
				{
					++it;
				}
			}

			// Remove the reference from our list of tokens
			if (tokens.empty())
			{
				_exclusiveAccessTokens.erase(tokensIt);
			}
		}
	}

	// Invalidate tokens outside the lock
	for (auto* token : tokensToInvalidate)
	{
		token->invalidateToken();
	}
}

bool ControllerImpl::areControlledEntitiesSelfLocked() const noexcept
{
	return _entitiesSharedLockInformation->isSelfLocked();
}

std::tuple<model::AcquireState, UniqueIdentifier> ControllerImpl::getAcquiredInfoFromStatus(ControlledEntityImpl& entity, UniqueIdentifier const owningEntity, entity::ControllerEntity::AemCommandStatus const status, bool const releaseEntityResult) const noexcept
{
	auto acquireState{ model::AcquireState::Undefined };
	auto owningController{ UniqueIdentifier{} };

	switch (status)
	{
		// Valid responses
		case entity::ControllerEntity::AemCommandStatus::Success:
			if (releaseEntityResult)
			{
				acquireState = model::AcquireState::NotAcquired;
				if (owningEntity)
				{
					LOG_CONTROLLER_WARN(entity.getEntity().getEntityID(), "OwningEntity field is not set to 0 on a ReleaseEntity response");
				}
			}
			else
			{
				// Full status check based on returned owningEntity, some devices return SUCCESS although the requesting controller is not the one currently owning the entity
				acquireState = owningEntity ? (owningEntity == getControllerEID() ? model::AcquireState::Acquired : model::AcquireState::AcquiredByOther) : model::AcquireState::NotAcquired;
				owningController = owningEntity;
			}
			// Remove "Milan compatibility" as device does support a forbidden command
			if (entity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
			{
				removeCompatibilityFlag(this, entity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.4.2.1", "Milan device must not implement ACQUIRE_ENTITY");
			}
			break;
		case entity::ControllerEntity::AemCommandStatus::AcquiredByOther:
			acquireState = model::AcquireState::AcquiredByOther;
			owningController = owningEntity;
			// Remove "Milan compatibility" as device does support a forbidden command
			if (entity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
			{
				removeCompatibilityFlag(this, entity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.4.2.1", "Milan device must not implement ACQUIRE_ENTITY");
			}
			break;
		case entity::ControllerEntity::AemCommandStatus::BadArguments:
			// Interpret BadArguments (when releasing) as trying to Release an Entity that is Not Acquired at all
			if (releaseEntityResult)
			{
				acquireState = model::AcquireState::NotAcquired;
			}
			break;
		case entity::ControllerEntity::AemCommandStatus::NotImplemented:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::NotSupported:
			acquireState = model::AcquireState::NotSupported;
			break;

		// All other cases, set to undefined
		default:
			break;
	}

	return std::make_tuple(acquireState, owningController);
}

std::tuple<model::LockState, UniqueIdentifier> ControllerImpl::getLockedInfoFromStatus(ControlledEntityImpl& entity, UniqueIdentifier const lockingEntity, entity::ControllerEntity::AemCommandStatus const status, bool const unlockEntityResult) const noexcept
{
	auto lockState{ model::LockState::Undefined };
	auto lockingController{ UniqueIdentifier{} };

	switch (status)
	{
		// Valid responses
		case entity::ControllerEntity::AemCommandStatus::Success:
			if (unlockEntityResult)
			{
				lockState = model::LockState::NotLocked;
				if (lockingEntity)
				{
					LOG_CONTROLLER_WARN(entity.getEntity().getEntityID(), "LockingEntity field is not set to 0 on a UnlockEntity response");
				}
			}
			else
			{
				// Full status check based on returned owningEntity, some devices return SUCCESS although the requesting controller is not the one currently owning the entity
				lockState = lockingEntity ? (lockingEntity == getControllerEID() ? model::LockState::Locked : model::LockState::LockedByOther) : model::LockState::NotLocked;
				lockingController = lockingEntity;
			}
			break;
		case entity::ControllerEntity::AemCommandStatus::LockedByOther:
			lockState = model::LockState::LockedByOther;
			lockingController = lockingEntity;
			break;
		case entity::ControllerEntity::AemCommandStatus::BadArguments:
			// Interpret BadArguments (when unlocking) as trying to Unlock an Entity that is Not Locked at all
			if (unlockEntityResult)
			{
				lockState = model::LockState::NotLocked;
			}
			break;
		case entity::ControllerEntity::AemCommandStatus::NotImplemented:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::NotSupported:
			lockState = model::LockState::NotSupported;
			// Remove "Milan compatibility" as device doesn't support a mandatory command
			if (entity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
			{
				removeCompatibilityFlag(this, entity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.4.2.2", "Milan device must implement LOCK_ENTITY");
			}
			break;

		// All other cases, set to undefined
		default:
			break;
	}

	return std::make_tuple(lockState, lockingController);
}

void ControllerImpl::addDelayedQuery(std::chrono::milliseconds const delay, UniqueIdentifier const entityID, DelayedQueryHandler&& queryHandler) noexcept
{
	// Lock to protect _delayedQueries
	std::lock_guard<decltype(_lock)> const lg(_lock);

	_delayedQueries.emplace_back(DelayedQuery{ std::chrono::system_clock::now() + delay, entityID, std::move(queryHandler) });
}

void ControllerImpl::chooseLocale(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, std::string const& preferedLocale, std::function<void(entity::model::StringsIndex const stringsIndex)> const& missingStringsHandler) noexcept
{
	model::LocaleNode const* localeNode{ nullptr };
	try
	{
		localeNode = entity->findLocaleNode(configurationIndex, preferedLocale);
		if (localeNode == nullptr)
		{
#pragma message("TODO: Split _preferedLocale into language/country, then if findLocaleDescriptor fails and language is not 'en', try to find a locale for 'en'")
			localeNode = entity->findLocaleNode(configurationIndex, "en");
		}
		if (localeNode != nullptr)
		{
			//auto const& configNode = entity->getConfigurationNode(configurationIndex);
			auto const& localeStaticModel = localeNode->staticModel;

			entity->setSelectedLocaleStringsIndexesRange(configurationIndex, localeStaticModel.baseStringDescriptorIndex, localeStaticModel.numberOfStringDescriptors, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			for (auto index = entity::model::StringsIndex(0); index < localeStaticModel.numberOfStringDescriptors; ++index)
			{
				// Check if we already have the Strings descriptor
				auto const stringsIndex = static_cast<decltype(index)>(localeStaticModel.baseStringDescriptorIndex + index);
				if (auto const stringsNodeIt = localeNode->strings.find(stringsIndex); stringsNodeIt != localeNode->strings.end())
				{
					// Already in cache, no need to query (just have to copy strings to Configuration for quick access)
					auto const& stringsStaticModel = stringsNodeIt->second.staticModel;
					entity->setLocalizedStrings(configurationIndex, index, stringsStaticModel.strings);
				}
				else
				{
					utils::invokeProtectedHandler(missingStringsHandler, stringsIndex);
				}
			}
		}
	}
	catch ([[maybe_unused]] ControlledEntity::Exception const& e)
	{
		// Ignore exception
		LOG_CONTROLLER_DEBUG(entity->getEntity().getEntityID(), "chooseLocale cannot find requested locale: {}", e.what());
	}
}

void ControllerImpl::queryInformation(ControlledEntityImpl* const entity, ControlledEntityImpl::MilanInfoType const milanInfoType, std::chrono::milliseconds const delayQuery) noexcept
{
	// Immediately set as expected
	entity->setMilanInfoExpected(milanInfoType);

	auto const entityID = entity->getEntity().getEntityID();
	std::function<void(entity::ControllerEntity*)> queryFunc{};

	switch (milanInfoType)
	{
		case ControlledEntityImpl::MilanInfoType::MilanInfo:
			queryFunc = [this, entityID](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getMilanInfo ()");
				controller->getMilanInfo(entityID, std::bind(&ControllerImpl::onGetMilanInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
			};
			break;
		default:
			AVDECC_ASSERT(false, "Unhandled MilanInfoType");
			break;
	}

	// Not delayed, call now
	if (delayQuery == std::chrono::milliseconds{ 0 })
	{
		if (queryFunc)
		{
			queryFunc(_controller);
		}
	}
	else
	{
		addDelayedQuery(delayQuery, entityID, std::move(queryFunc));
	}
}

void ControllerImpl::queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::chrono::milliseconds const delayQuery) noexcept
{
	// Immediately set as expected
	entity->setDescriptorExpected(configurationIndex, descriptorType, descriptorIndex);

	auto const entityID = entity->getEntity().getEntityID();
	std::function<void(entity::ControllerEntity*)> queryFunc{};

	switch (descriptorType)
	{
		case entity::model::DescriptorType::Entity:
			queryFunc = [this, entityID](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readEntityDescriptor ()");
				controller->readEntityDescriptor(entityID, std::bind(&ControllerImpl::onEntityDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
			};
			break;
		case entity::model::DescriptorType::Configuration:
			queryFunc = [this, entityID, configurationIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readConfigurationDescriptor (ConfigurationIndex={})", configurationIndex);
				controller->readConfigurationDescriptor(entityID, configurationIndex, std::bind(&ControllerImpl::onConfigurationDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
			};
			break;
		case entity::model::DescriptorType::AudioUnit:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readAudioUnitDescriptor (ConfigurationIndex={} AudioUnitIndex={})", configurationIndex, descriptorIndex);
				controller->readAudioUnitDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAudioUnitDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::StreamInput:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readStreamInputDescriptor (ConfigurationIndex={} StreamIndex={})", configurationIndex, descriptorIndex);
				controller->readStreamInputDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onStreamInputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::StreamOutput:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readStreamOutputDescriptor (ConfigurationIndex={} StreamIndex={})", configurationIndex, descriptorIndex);
				controller->readStreamOutputDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onStreamOutputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::JackInput:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readJackInputDescriptor (ConfigurationIndex={} JackIndex={})", configurationIndex, descriptorIndex);
				controller->readJackInputDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onJackInputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::JackOutput:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readJackOutputDescriptor (ConfigurationIndex={} JackIndex={})", configurationIndex, descriptorIndex);
				controller->readJackOutputDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onJackOutputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::AvbInterface:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readAvbInterfaceDescriptor (ConfigurationIndex={}, AvbInterfaceIndex={})", configurationIndex, descriptorIndex);
				controller->readAvbInterfaceDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAvbInterfaceDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, ControlledEntityImpl::EnumerationStep::GetStaticModel));
			};
			break;
		case entity::model::DescriptorType::ClockSource:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readClockSourceDescriptor (ConfigurationIndex={} ClockSourceIndex={})", configurationIndex, descriptorIndex);
				controller->readClockSourceDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onClockSourceDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, ControlledEntityImpl::EnumerationStep::GetStaticModel));
			};
			break;
		case entity::model::DescriptorType::MemoryObject:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readMemoryObjectDescriptor (ConfigurationIndex={}, MemoryObjectIndex={})", configurationIndex, descriptorIndex);
				controller->readMemoryObjectDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onMemoryObjectDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::Locale:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readLocaleDescriptor (ConfigurationIndex={} LocaleIndex={})", configurationIndex, descriptorIndex);
				controller->readLocaleDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onLocaleDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::Strings:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readStringsDescriptor (ConfigurationIndex={} StringsIndex={})", configurationIndex, descriptorIndex);
				controller->readStringsDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onStringsDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::StreamPortInput:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readStreamPortInputDescriptor (ConfigurationIndex={}, StreamPortIndex={})", configurationIndex, descriptorIndex);
				controller->readStreamPortInputDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onStreamPortInputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::StreamPortOutput:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readStreamPortOutputDescriptor (ConfigurationIndex={} StreamPortIndex={})", configurationIndex, descriptorIndex);
				controller->readStreamPortOutputDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onStreamPortOutputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::AudioCluster:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readAudioClusterDescriptor (ConfigurationIndex={} ClusterIndex={})", configurationIndex, descriptorIndex);
				controller->readAudioClusterDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAudioClusterDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::AudioMap:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readAudioMapDescriptor (ConfigurationIndex={} MapIndex={})", configurationIndex, descriptorIndex);
				controller->readAudioMapDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAudioMapDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::Control:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readControlDescriptor (ConfigurationIndex={}, ControlIndex={})", configurationIndex, descriptorIndex);
				controller->readControlDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onControlDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::ClockDomain:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readClockDomainDescriptor (ConfigurationIndex={}, ClockDomainIndex={})", configurationIndex, descriptorIndex);
				controller->readClockDomainDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onClockDomainDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::Timing:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readTimingDescriptor (ConfigurationIndex={}, TimingIndex={})", configurationIndex, descriptorIndex);
				controller->readTimingDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onTimingDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::PtpInstance:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readPtpInstanceDescriptor (ConfigurationIndex={}, PtpInstanceIndex={})", configurationIndex, descriptorIndex);
				controller->readPtpInstanceDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onPtpInstanceDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::PtpPort:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readPtpPortDescriptor (ConfigurationIndex={}, PtpPortIndex={})", configurationIndex, descriptorIndex);
				controller->readPtpPortDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onPtpPortDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		default:
			AVDECC_ASSERT(false, "Unhandled DescriptorType");
			break;
	}

	// Not delayed, call now
	if (delayQuery == std::chrono::milliseconds{ 0 })
	{
		if (queryFunc)
		{
			queryFunc(_controller);
		}
	}
	else
	{
		addDelayedQuery(delayQuery, entityID, std::move(queryFunc));
	}
}

void ControllerImpl::queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex, std::chrono::milliseconds const delayQuery) noexcept
{
	// Immediately set as expected
	entity->setDynamicInfoExpected(configurationIndex, dynamicInfoType, descriptorIndex, subIndex);

	auto const entityID = entity->getEntity().getEntityID();
	std::function<void(entity::ControllerEntity*)> queryFunc{};

	switch (dynamicInfoType)
	{
		case ControlledEntityImpl::DynamicInfoType::AcquiredState:
			queryFunc = [this, entityID](entity::ControllerEntity* const controller) noexcept
			{
				// Send an ACQUIRE command with the RELEASE flag to detect the current acquired state of the entity
				// It won't change the current acquired state except if we were the acquiring controller, which doesn't matter anyway because having to enumerate the device again means we got interrupted in the middle of something and it's best to start over
				LOG_CONTROLLER_TRACE(entityID, "acquireEntity (ReleaseFlag)");
				controller->releaseEntity(entityID, entity::model::DescriptorType::Entity, 0u, std::bind(&ControllerImpl::onGetAcquiredStateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::LockedState:
			queryFunc = [this, entityID](entity::ControllerEntity* const controller) noexcept
			{
				// Send a LOCK command with the RELEASE flag to detect the current locked state of the entity
				// It won't change the current locked state except if we were the locking controller, which doesn't matter anyway because having to enumerate the device again means we got interrupted in the middle of something and it's best to start over
				LOG_CONTROLLER_TRACE(entityID, "lockEntity (ReleaseFlag)");
				controller->unlockEntity(entityID, entity::model::DescriptorType::Entity, 0u, std::bind(&ControllerImpl::onGetLockedStateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::InputStreamPortAudioMappings:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex, subIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamPortInputAudioMap (StreamPortIndex={})", descriptorIndex);
				controller->getStreamPortInputAudioMap(entityID, descriptorIndex, subIndex, std::bind(&ControllerImpl::onGetStreamPortInputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::OutputStreamPortAudioMappings:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex, subIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamPortOutputAudioMap (StreamPortIndex={})", descriptorIndex);
				controller->getStreamPortOutputAudioMap(entityID, descriptorIndex, subIndex, std::bind(&ControllerImpl::onGetStreamPortOutputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::InputStreamState:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getListenerStreamState (StreamIndex={})", descriptorIndex);
				controller->getListenerStreamState({ entityID, descriptorIndex }, std::bind(&ControllerImpl::onGetListenerStreamStateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::OutputStreamState:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getTalkerStreamState (StreamIndex={})", descriptorIndex);
				controller->getTalkerStreamState({ entityID, descriptorIndex }, std::bind(&ControllerImpl::onGetTalkerStreamStateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::OutputStreamConnection:
			AVDECC_ASSERT(false, "Another overload of this method should be called for this DynamicInfoType");
			break;
		case ControlledEntityImpl::DynamicInfoType::InputStreamInfo:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamInputInfo (StreamIndex={})", descriptorIndex);
				controller->getStreamInputInfo(entityID, descriptorIndex, std::bind(&ControllerImpl::onGetStreamInputInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::OutputStreamInfo:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamOutputInfo (StreamIndex={})", descriptorIndex);
				controller->getStreamOutputInfo(entityID, descriptorIndex, std::bind(&ControllerImpl::onGetStreamOutputInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::GetAvbInfo:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getAvbInfo (AvbInterfaceIndex={})", descriptorIndex);
				controller->getAvbInfo(entityID, descriptorIndex, std::bind(&ControllerImpl::onGetAvbInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::GetAsPath:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getAsPath (AvbInterfaceIndex={})", descriptorIndex);
				controller->getAsPath(entityID, descriptorIndex, std::bind(&ControllerImpl::onGetAsPathResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::GetEntityCounters:
			queryFunc = [this, entityID](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getEntityCounters ()");
				controller->getEntityCounters(entityID, std::bind(&ControllerImpl::onGetEntityCountersResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::GetAvbInterfaceCounters:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getAvbInterfaceCounters (AvbInterfaceIndex={})", descriptorIndex);
				controller->getAvbInterfaceCounters(entityID, descriptorIndex, std::bind(&ControllerImpl::onGetAvbInterfaceCountersResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::GetClockDomainCounters:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getClockDomainCounters (ClockDomainIndex={})", descriptorIndex);
				controller->getClockDomainCounters(entityID, descriptorIndex, std::bind(&ControllerImpl::onGetClockDomainCountersResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::GetStreamInputCounters:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamInputCounters (StreamIndex={})", descriptorIndex);
				controller->getStreamInputCounters(entityID, descriptorIndex, std::bind(&ControllerImpl::onGetStreamInputCountersResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::GetStreamOutputCounters:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamOutputCounters (StreamIndex={})", descriptorIndex);
				controller->getStreamOutputCounters(entityID, descriptorIndex, std::bind(&ControllerImpl::onGetStreamOutputCountersResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::GetMaxTransitTime:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getMaxTransitTime (StreamIndex={})", descriptorIndex);
				controller->getMaxTransitTime(entityID, descriptorIndex, std::bind(&ControllerImpl::onGetMaxTransitTimeResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::GetSystemUniqueID:
			queryFunc = [this, entityID](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getSystemUniqueID ()");
				controller->getSystemUniqueID(entityID, std::bind(&ControllerImpl::onGetSystemUniqueIDResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::GetMediaClockReferenceInfo:
			queryFunc = [this, entityID, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getMediaClockReferenceInfo (MediaClockIndex={})", descriptorIndex);
				controller->getMediaClockReferenceInfo(entityID, descriptorIndex, std::bind(&ControllerImpl::onGetMediaClockReferenceInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::InputStreamInfoEx:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamInputInfoEx (StreamIndex={})", descriptorIndex);
				controller->getStreamInputInfoEx(entityID, descriptorIndex, std::bind(&ControllerImpl::onGetStreamInputInfoExResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		default:
			AVDECC_ASSERT(false, "Unhandled DynamicInfoType");
			break;
	}

	// Not delayed, call now
	if (delayQuery == std::chrono::milliseconds{ 0 })
	{
		if (queryFunc)
		{
			queryFunc(_controller);
		}
	}
	else
	{
		addDelayedQuery(delayQuery, entityID, std::move(queryFunc));
	}
}

void ControllerImpl::queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::StreamIdentification const& talkerStream, std::uint16_t const subIndex, std::chrono::milliseconds const delayQuery) noexcept
{
	if (!AVDECC_ASSERT_WITH_RET(dynamicInfoType == ControlledEntityImpl::DynamicInfoType::OutputStreamConnection, "Another overload of this method should be called for DynamicInfoType different than OutputStreamConnection"))
	{
		return;
	}

	// Immediately set as expected
	entity->setDynamicInfoExpected(configurationIndex, dynamicInfoType, talkerStream.streamIndex, subIndex);

	auto const entityID = entity->getEntity().getEntityID();
	std::function<void(entity::ControllerEntity*)> queryFunc{};

	queryFunc = [this, configurationIndex, talkerStream, subIndex](entity::ControllerEntity* const controller) noexcept
	{
		LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "getTalkerStreamConnection (TalkerID={} TalkerIndex={} SubIndex={})", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, subIndex);
		controller->getTalkerStreamConnection(talkerStream, subIndex, std::bind(&ControllerImpl::onGetTalkerStreamConnectionResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex, subIndex));
	};

	// Not delayed, call now
	if (delayQuery == std::chrono::milliseconds{ 0 })
	{
		if (queryFunc)
		{
			queryFunc(_controller);
		}
	}
	else
	{
		addDelayedQuery(delayQuery, entityID, std::move(queryFunc));
	}
}

void ControllerImpl::queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::chrono::milliseconds const delayQuery) noexcept
{
	// Immediately set as expected
	entity->setDescriptorDynamicInfoExpected(configurationIndex, descriptorDynamicInfoType, descriptorIndex);

	auto const entityID = entity->getEntity().getEntityID();
	std::function<void(entity::ControllerEntity*)> queryFunc{};

	switch (descriptorDynamicInfoType)
	{
		case ControlledEntityImpl::DescriptorDynamicInfoType::ConfigurationName:
			queryFunc = [this, entityID, configurationIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getConfigurationName (ConfigurationIndex={})", configurationIndex);
				controller->getConfigurationName(entityID, configurationIndex, std::bind(&ControllerImpl::onConfigurationNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getAudioUnitName (ConfigurationIndex={} AudioUnitIndex={})", configurationIndex, descriptorIndex);
				controller->getAudioUnitName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAudioUnitNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitSamplingRate:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getAudioUnitSamplingRate (ConfigurationIndex={} AudioUnitIndex={})", configurationIndex, descriptorIndex);
				controller->getAudioUnitSamplingRate(entityID, descriptorIndex, std::bind(&ControllerImpl::onAudioUnitSamplingRateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamInputName (ConfigurationIndex={} StreamIndex={})", configurationIndex, descriptorIndex);
				controller->getStreamInputName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onInputStreamNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamFormat:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamInputFormat (ConfigurationIndex={} StreamIndex={})", configurationIndex, descriptorIndex);
				controller->getStreamInputFormat(entityID, descriptorIndex, std::bind(&ControllerImpl::onInputStreamFormatResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamOutputName (ConfigurationIndex={} StreamIndex={})", configurationIndex, descriptorIndex);
				controller->getStreamOutputName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onOutputStreamNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamFormat:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamOutputFormat (ConfigurationIndex={} StreamIndex={})", configurationIndex, descriptorIndex);
				controller->getStreamOutputFormat(entityID, descriptorIndex, std::bind(&ControllerImpl::onOutputStreamFormatResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::InputJackName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getJackInputName (ConfigurationIndex={} JackIndex={})", configurationIndex, descriptorIndex);
				controller->getJackInputName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onInputJackNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::OutputJackName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getJackOutputName (ConfigurationIndex={} JackIndex={})", configurationIndex, descriptorIndex);
				controller->getJackOutputName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onOutputJackNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::AvbInterfaceDescriptor:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readAvbInterfaceDescriptor (ConfigurationIndex={}, AvbInterfaceIndex={})", configurationIndex, descriptorIndex);
				controller->readAvbInterfaceDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAvbInterfaceDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::ClockSourceDescriptor:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readClockSourceDescriptor (ConfigurationIndex={} ClockSourceIndex={})", configurationIndex, descriptorIndex);
				controller->readClockSourceDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onClockSourceDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getMemoryObjectName (ConfigurationIndex={} MemoryObjectIndex={})", configurationIndex, descriptorIndex);
				controller->getMemoryObjectName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onMemoryObjectNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectLength:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getMemoryObjectLength (ConfigurationIndex={} MemoryObjectIndex={})", configurationIndex, descriptorIndex);
				controller->getMemoryObjectLength(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onMemoryObjectLengthResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getAudioClusterName (ConfigurationIndex={} AudioClusterIndex={})", configurationIndex, descriptorIndex);
				controller->getAudioClusterName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAudioClusterNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::ControlName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getControlName (ConfigurationIndex={} ControlIndex={})", configurationIndex, descriptorIndex);
				controller->getControlName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onControlNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::ControlValues:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getControl (ConfigurationIndex={} ControlIndex={})", configurationIndex, descriptorIndex);
				controller->getControlValues(entityID, descriptorIndex, std::bind(&ControllerImpl::onControlValuesResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getClockDomainName (ConfigurationIndex={} ClockDomainIndex={})", configurationIndex, descriptorIndex);
				controller->getClockDomainName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onClockDomainNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainSourceIndex:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getClockSource (ConfigurationIndex={} ClockDomainIndex={})", configurationIndex, descriptorIndex);
				controller->getClockSource(entityID, descriptorIndex, std::bind(&ControllerImpl::onClockDomainSourceIndexResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::TimingName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getTimingName (ConfigurationIndex={} TimingIndex={})", configurationIndex, descriptorIndex);
				controller->getTimingName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onTimingNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::PtpInstanceName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getPtpInstanceName (ConfigurationIndex={} PtpInstanceIndex={})", configurationIndex, descriptorIndex);
				controller->getPtpInstanceName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onPtpInstanceNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::PtpPortName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getPtpPortName (ConfigurationIndex={} PtpPortIndex={})", configurationIndex, descriptorIndex);
				controller->getPtpPortName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onPtpPortNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		default:
			AVDECC_ASSERT(false, "Unhandled DescriptorDynamicInfoType");
			break;
	}

	// Not delayed, call now
	if (delayQuery == std::chrono::milliseconds{ 0 })
	{
		if (queryFunc)
		{
			queryFunc(_controller);
		}
	}
	else
	{
		addDelayedQuery(delayQuery, entityID, std::move(queryFunc));
	}
}

void ControllerImpl::queryInformation(ControlledEntityImpl* const entity, entity::controller::DynamicInfoParameters const& dynamicInfoParameters, std::uint16_t const packetID, ControlledEntityImpl::EnumerationStep const step, std::chrono::milliseconds const delayQuery) noexcept
{
	// Immediately set as expected
	entity->setPackedDynamicInfoExpected(packetID);

	auto const entityID = entity->getEntity().getEntityID();
	std::function<void(entity::ControllerEntity*)> queryFunc{};

	queryFunc = [this, entityID, dynamicInfoParameters, packetID, step](entity::ControllerEntity* const controller) noexcept
	{
		LOG_CONTROLLER_TRACE(entityID, "getDynamicInfo (PacketID={} Step={})", packetID, avdecc::utils::to_integral(step));
		controller->getDynamicInfo(entityID, dynamicInfoParameters, std::bind(&ControllerImpl::onGetDynamicInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, dynamicInfoParameters, packetID, step));
	};

	// Not delayed, call now
	if (delayQuery == std::chrono::milliseconds{ 0 })
	{
		if (queryFunc)
		{
			queryFunc(_controller);
		}
	}
	else
	{
		addDelayedQuery(delayQuery, entityID, std::move(queryFunc));
	}
}

void ControllerImpl::getMilanInfo(ControlledEntityImpl* const entity) noexcept
{
	auto const caps = entity->getEntity().getEntityCapabilities();

	// Check if AEM and VendorUnique is supported by this entity
	if (caps.test(entity::EntityCapability::AemSupported) && caps.test(entity::EntityCapability::VendorUniqueSupported))
	{
		// Get MilanInfo
		queryInformation(entity, ControlledEntityImpl::MilanInfoType::MilanInfo);
	}

	// Got all expected Milan information
	if (entity->gotAllExpectedMilanInfo())
	{
		// Clear this enumeration step and check for next one
		entity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetMilanInfo);
		checkEnumerationSteps(entity);
	}
}

void ControllerImpl::checkDynamicInfoSupported(ControlledEntityImpl* const entity) noexcept
{
	//auto const caps = entity->getEntity().getEntityCapabilities();
	auto const entityID = entity->getEntity().getEntityID();

	// Immediately set as expected
	entity->setCheckDynamicInfoSupportedExpected();

	// Query an empty getDynamicInfo to check if it is supported
	LOG_CONTROLLER_TRACE(entityID, "empty getDynamicInfo ()");
	_controller->getDynamicInfo(entityID, {}, std::bind(&ControllerImpl::onEmptyGetDynamicInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void ControllerImpl::registerUnsol(ControlledEntityImpl* const entity) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();

	// Immediately set as expected
	entity->setRegisterUnsolExpected();

	// Register for unsolicited notifications
	LOG_CONTROLLER_TRACE(entityID, "registerUnsolicitedNotifications ()");
	_controller->registerUnsolicitedNotifications(entityID, std::bind(&ControllerImpl::onRegisterUnsolicitedNotificationsResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void ControllerImpl::unregisterUnsol(ControlledEntityImpl* const entity) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();

	// Unregister from unsolicited notifications
	LOG_CONTROLLER_TRACE(entityID, "unregisterUnsolicitedNotifications ()");
	_controller->unregisterUnsolicitedNotifications(entityID, std::bind(&ControllerImpl::onUnregisterUnsolicitedNotificationsResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void ControllerImpl::getStaticModel(ControlledEntityImpl* const entity) noexcept
{
	// Always start with Entity Descriptor, the response from it will schedule subsequent descriptors queries
	queryInformation(entity, 0, entity::model::DescriptorType::Entity, 0);
}

void ControllerImpl::getDynamicInfo(ControlledEntityImpl* const entity) noexcept
{
	class DynamicInfoVisitor : public model::EntityModelVisitor
	{
	public:
		explicit DynamicInfoVisitor(ControllerImpl* const controller, ControlledEntityImpl* const entity) noexcept
			: _controller{ controller }
			, _entity{ entity }
			, _usePackedDynamicInfo{ _entity->isPackedDynamicInfoSupported() }
			, _milanCompatibilityVersion{ _entity->getMilanCompatibilityVersion() }
		{
			auto const milanInfo = _entity->getMilanInfo();
			if (milanInfo)
			{
				_milanSpecVersion = milanInfo->specificationVersion;
			}
		}

		entity::controller::DynamicInfoParameters const& getDynamicInfoParameters() const noexcept
		{
			return _dynamicInfoParameters;
		}

		// Deleted compiler auto-generated methods
		DynamicInfoVisitor(DynamicInfoVisitor const&) = delete;
		DynamicInfoVisitor(DynamicInfoVisitor&&) = delete;
		DynamicInfoVisitor& operator=(DynamicInfoVisitor const&) = delete;
		DynamicInfoVisitor& operator=(DynamicInfoVisitor&&) = delete;

	private:
		bool shouldGetStreamInputInfoEx() const noexcept
		{
			// Milan devices are using an extended version of AEM-GET_STREAM_INFO and ACMP-RX_STATE to report connection status and some extra fields required by Milan
			// This changed since Milan 1.3 to use a MVU specific command for both
			if (_milanSpecVersion >= entity::model::MilanVersion{ 1, 3 })
			{
				return true;
			}
			return false;
		}
		bool shouldGetMaxTransitTime() const noexcept
		{
			// Milan devices are using GET_STREAM_INFO to report MaxTransitTime, so we do not need to query GET_MAX_TRANSIT_TIME
			// This changed since Milan 1.3 to use the same mechanism as IEEE 1722.1 devices
			if (_milanSpecVersion >= entity::model::MilanVersion{ 1, 0 } && _milanSpecVersion < entity::model::MilanVersion{ 1, 3 })
			{
				return false;
			}
			return true;
		}
		// model::EntityModelVisitor overrides
		virtual void visit(ControlledEntity const* const /*entity*/, model::EntityNode const& /*node*/) noexcept override
		{
			// Get AcquiredState / LockedState (global entity information not related to current configuration)
			{
				// Milan devices don't implement AcquireEntity, no need to query its state
				if (_entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
				{
					_entity->setAcquireState(model::AcquireState::NotSupported);
				}
				else
				{
					_controller->queryInformation(_entity, 0u, ControlledEntityImpl::DynamicInfoType::AcquiredState, 0u);
				}
				_controller->queryInformation(_entity, 0u, ControlledEntityImpl::DynamicInfoType::LockedState, 0u);
			}

			// Entity Counters
			if (_usePackedDynamicInfo)
			{
				_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetCounters, { entity::model::DescriptorType::Entity, entity::model::DescriptorIndex{ 0u } } });
			}
			else
			{
				_controller->queryInformation(_entity, 0u, ControlledEntityImpl::DynamicInfoType::GetEntityCounters, 0u);
			}

			// Get Milan global dynamic information (for Milan >= 1.2 devices)
			if (_milanCompatibilityVersion >= entity::model::MilanVersion{ 1, 2 })
			{
				// Get SystemUniqueID
				_controller->queryInformation(_entity, entity::model::getInvalidDescriptorIndex(), ControlledEntityImpl::DynamicInfoType::GetSystemUniqueID, entity::model::getInvalidDescriptorIndex());
			}
		}
		virtual void visit(ControlledEntity const* const /*entity*/, model::EntityNode const* const /*parent*/, model::ConfigurationNode const& node) noexcept override
		{
			_currentConfigurationIndex = node.descriptorIndex;
		}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::AudioUnitNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::StreamInputNode const& node) noexcept override
		{
			// StreamInfo
			_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamInfo, node.descriptorIndex);

			// Counters
			if (_usePackedDynamicInfo)
			{
				_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetCounters, { entity::model::DescriptorType::StreamInput, node.descriptorIndex } });
			}
			else
			{
				_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::GetStreamInputCounters, node.descriptorIndex);
			}

			if (shouldGetStreamInputInfoEx())
			{
				// StreamInputInfoEx
				_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamInfoEx, node.descriptorIndex);
			}
			else
			{
				// RX_STATE
				_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamState, node.descriptorIndex);
			}
		}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::StreamOutputNode const& node) noexcept override
		{
			// StreamInfo
			_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamInfo, node.descriptorIndex);

			// Counters
			if (_usePackedDynamicInfo)
			{
				_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetCounters, { entity::model::DescriptorType::StreamOutput, node.descriptorIndex } });
			}
			else
			{
				_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::GetStreamOutputCounters, node.descriptorIndex);
			}

			// MaxTransitTime
			if (shouldGetMaxTransitTime())
			{
				if (_usePackedDynamicInfo)
				{
					_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetMaxTransitTime, { node.descriptorIndex } });
				}
				else
				{
					_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::GetMaxTransitTime, node.descriptorIndex);
				}
			}

			// TX_STATE
			_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamState, node.descriptorIndex);
		}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::JackInputNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::JackOutputNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::JackNode const* const /*parent*/, model::ControlNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::AvbInterfaceNode const& node) noexcept override
		{
			// AvbInfo
			_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInfo, node.descriptorIndex);
			// AsPath
			_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::GetAsPath, node.descriptorIndex);
			// Counters
			if (_usePackedDynamicInfo)
			{
				_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetCounters, { entity::model::DescriptorType::AvbInterface, node.descriptorIndex } });
			}
			else
			{
				_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInterfaceCounters, node.descriptorIndex);
			}
		}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::ClockSourceNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::MemoryObjectNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::LocaleNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::LocaleNode const* const /*parent*/, model::StringsNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::AudioUnitNode const* const /*parent*/, model::StreamPortInputNode const& node) noexcept override
		{
			if (node.staticModel.numberOfMaps == 0)
			{
				// AudioMappings
				// TODO: IEEE1722.1-2013 Clause 7.4.44.3 recommands to Lock or Acquire the entity before getting the dynamic audio map
				_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamPortAudioMappings, node.descriptorIndex);
			}
		}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::AudioUnitNode const* const /*parent*/, model::StreamPortOutputNode const& node) noexcept override
		{
			if (node.staticModel.numberOfMaps == 0)
			{
				// AudioMappings
				// TODO: IEEE1722.1-2013 Clause 7.4.44.3 recommands to Lock or Acquire the entity before getting the dynamic audio map
				_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamPortAudioMappings, node.descriptorIndex);
			}
		}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::AudioUnitNode const* const /*grandParent*/, model::StreamPortNode const* const /*parent*/, model::AudioClusterNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::AudioUnitNode const* const /*grandParent*/, model::StreamPortNode const* const /*parent*/, model::AudioMapNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::AudioUnitNode const* const /*grandParent*/, model::StreamPortNode const* const /*parent*/, model::ControlNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::AudioUnitNode const* const /*parent*/, model::ControlNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::ControlNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::ClockDomainNode const& node) noexcept override
		{
			// Counters
			if (_usePackedDynamicInfo)
			{
				_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetCounters, { entity::model::DescriptorType::ClockDomain, node.descriptorIndex } });
			}
			else
			{
				_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::GetClockDomainCounters, node.descriptorIndex);
			}
			// Get MediaClockReferenceInfo information (for Milan >= 1.2 devices)
			if (_milanCompatibilityVersion >= entity::model::MilanVersion{ 1, 2 })
			{
				_controller->queryInformation(_entity, entity::model::getInvalidDescriptorIndex(), ControlledEntityImpl::DynamicInfoType::GetMediaClockReferenceInfo, node.descriptorIndex);
			}
		}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::ClockDomainNode const* const /*parent*/, model::ClockSourceNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::TimingNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::PtpInstanceNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::TimingNode const* const /*parent*/, la::avdecc::controller::model::PtpInstanceNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::PtpPortNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::TimingNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::TimingNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::PtpPortNode const& /*node*/) noexcept override {}
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::RedundantStreamInputNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::RedundantStreamOutputNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::RedundantStreamNode const* const /*parent*/, model::StreamInputNode const& /*node*/) noexcept override {}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::RedundantStreamNode const* const /*parent*/, model::StreamOutputNode const& /*node*/) noexcept override {}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

		// Private members
		ControllerImpl* _controller{ nullptr };
		ControlledEntityImpl* _entity{ nullptr };
		entity::model::ConfigurationIndex _currentConfigurationIndex{ entity::model::getInvalidDescriptorIndex() };
		bool _usePackedDynamicInfo{ false };
		entity::model::MilanVersion _milanCompatibilityVersion{};
		entity::model::MilanVersion _milanSpecVersion{};
		entity::controller::DynamicInfoParameters _dynamicInfoParameters{};
	};

	// Visit all known descriptor and get associated dynamic information
	auto visitor = DynamicInfoVisitor{ this, entity };
	entity->accept(&visitor, false);
	// Flush all packed dynamic info queries
	flushPackedDynamicInfoQueries(entity, visitor.getDynamicInfoParameters(), ControlledEntityImpl::EnumerationStep::GetDynamicInfo);

	// Got all expected dynamic information
	if (entity->gotAllExpectedDynamicInfo())
	{
		// Clear this enumeration step and check for next one
		entity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
		checkEnumerationSteps(entity);
	}
}

void ControllerImpl::getDescriptorDynamicInfo(ControlledEntityImpl* const entity) noexcept
{
	auto const caps = entity->getEntity().getEntityCapabilities();
	// Check if AEM is supported by this entity
	if (caps.test(entity::EntityCapability::AemSupported) && entity->hasAnyConfiguration())
	{
		class DynamicInfoModelVisitor : public model::EntityModelVisitor
		{
		public:
			DynamicInfoModelVisitor(ControllerImpl* const controller, ControlledEntityImpl* const entity) noexcept
				: _controller{ controller }
				, _entity{ entity }
				, _usePackedDynamicInfo{ _entity->isPackedDynamicInfoSupported() }
			{
			}

			entity::controller::DynamicInfoParameters const& getDynamicInfoParameters() const noexcept
			{
				return _dynamicInfoParameters;
			}

			// Deleted compiler auto-generated methods
			DynamicInfoModelVisitor(DynamicInfoModelVisitor const&) = delete;
			DynamicInfoModelVisitor(DynamicInfoModelVisitor&&) = delete;
			DynamicInfoModelVisitor& operator=(DynamicInfoModelVisitor const&) = delete;
			DynamicInfoModelVisitor& operator=(DynamicInfoModelVisitor&&) = delete;

		private:
			// Private methods
			void getControlNodeDynamicInformation(la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ControlIndex const controlIndex) noexcept
			{
				// Get ControlName
				if (_usePackedDynamicInfo)
				{
					_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetName, { configurationIndex, entity::model::DescriptorType::Control, controlIndex, std::uint16_t{ 0u } } });
				}
				else
				{
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ControlName, controlIndex);
				}
				// Get ControlValues
				_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ControlValues, controlIndex);
			}
			// model::EntityModelVisitor overrides
			virtual void visit(ControlledEntity const* const /*entity*/, model::EntityNode const& node) noexcept override
			{
				// Store the current configuration index
				_currentConfigurationIndex = node.dynamicModel.currentConfiguration;
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::EntityNode const* const /*parent*/, model::ConfigurationNode const& node) noexcept override
			{
				auto const configurationIndex = node.descriptorIndex;

				// Get configuration dynamic model
				auto* const configDynamicModel = _entity->getModelAccessStrategy().getConfigurationNodeDynamicModel(configurationIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
				if (configDynamicModel)
				{
					// We can set the currentConfiguration value right now, we know it
					configDynamicModel->isActiveConfiguration = configurationIndex == _currentConfigurationIndex;

					// Get ConfigurationName
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetName, { entity::model::ConfigurationIndex{ 0u }, entity::model::DescriptorType::Configuration, configurationIndex, std::uint16_t{ 0u } } }); // CAUTION: configurationIndex should be set to 0 for CONFIGURATION_DESCRIPTOR, the index for the requested configuration name is actually passed as the descriptorIndex field
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ConfigurationName, 0u);
					}

					if (configDynamicModel->isActiveConfiguration)
					{
						// Choose a locale
						chooseLocale(_entity, configurationIndex, _controller->_preferedLocale,
							[this, configurationIndex](entity::model::StringsIndex const stringsIndex)
							{
								// Strings not in cache, we need to query the device
								_controller->queryInformation(_entity, configurationIndex, entity::model::DescriptorType::Strings, stringsIndex);
							});
					}
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::AudioUnitNode const& node) noexcept override
			{
				auto const configurationIndex = parent->descriptorIndex;
				auto const audioUnitIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					// Get AudioUnitName
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetName, { configurationIndex, entity::model::DescriptorType::AudioUnit, audioUnitIndex, std::uint16_t{ 0u } } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitName, audioUnitIndex);
					}
					// Get AudioUnitSamplingRate
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetSamplingRate, { entity::model::DescriptorType::AudioUnit, audioUnitIndex } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitSamplingRate, audioUnitIndex);
					}
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::StreamInputNode const& node) noexcept override
			{
				auto const configurationIndex = parent->descriptorIndex;
				auto const streamIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					// Get InputStreamName
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetName, { configurationIndex, entity::model::DescriptorType::StreamInput, streamIndex, std::uint16_t{ 0u } } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamName, streamIndex);
					}
					// Get InputStreamFormat
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetStreamFormat, { entity::model::DescriptorType::StreamInput, streamIndex } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamFormat, streamIndex);
					}
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::StreamOutputNode const& node) noexcept override
			{
				auto const configurationIndex = parent->descriptorIndex;
				auto const streamIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					// Get OutputStreamName
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetName, { configurationIndex, entity::model::DescriptorType::StreamOutput, streamIndex, std::uint16_t{ 0u } } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamName, streamIndex);
					}
					// Get OutputStreamFormat
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetStreamFormat, { entity::model::DescriptorType::StreamOutput, streamIndex } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamFormat, streamIndex);
					}
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::JackInputNode const& node) noexcept override
			{
				auto const configurationIndex = parent->descriptorIndex;
				auto const jackIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					// Get InputJackName
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetName, { configurationIndex, entity::model::DescriptorType::JackInput, jackIndex, std::uint16_t{ 0u } } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputJackName, jackIndex);
					}
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::JackOutputNode const& node) noexcept override
			{
				auto const configurationIndex = parent->descriptorIndex;
				auto const jackIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					// Get OutputJackName
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetName, { configurationIndex, entity::model::DescriptorType::JackOutput, jackIndex, std::uint16_t{ 0u } } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputJackName, jackIndex);
					}
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::JackNode const* const /*parent*/, model::ControlNode const& node) noexcept override
			{
				auto const configurationIndex = grandParent->descriptorIndex;
				auto const controlIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					getControlNodeDynamicInformation(configurationIndex, controlIndex);
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::AvbInterfaceNode const& node) noexcept override
			{
				// AVB_INTERFACE descriptor contains 'dynamic' fields (not part of the static model) that cannot be retrieved through a simple command, we have to query the whole descriptor
				auto const configurationIndex = parent->descriptorIndex;
				auto const avbInterfaceIndex = node.descriptorIndex;

				_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AvbInterfaceDescriptor, avbInterfaceIndex);
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ClockSourceNode const& node) noexcept override
			{
				// CLOCK_SOURCE descriptor contains 'dynamic' fields (not part of the static model) that cannot be retrieved through a simple command, we have to query the whole descriptor
				auto const configurationIndex = parent->descriptorIndex;
				auto const clockSourceIndex = node.descriptorIndex;

				_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockSourceDescriptor, clockSourceIndex);
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::MemoryObjectNode const& node) noexcept override
			{
				auto const configurationIndex = parent->descriptorIndex;
				auto const memoryObjectIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					// Get MemoryObjectName
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetName, { configurationIndex, entity::model::DescriptorType::MemoryObject, memoryObjectIndex, std::uint16_t{ 0u } } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectName, memoryObjectIndex);
					}
					// Get MemoryObjectLength
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetMemoryObjectLength, { configurationIndex, memoryObjectIndex } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectLength, memoryObjectIndex);
					}
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::LocaleNode const& /*node*/) noexcept override
			{
				// Nothing to get
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::LocaleNode const* const /*parent*/, model::StringsNode const& /*node*/) noexcept override
			{
				// Nothing to get
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::AudioUnitNode const* const /*parent*/, model::StreamPortInputNode const& /*node*/) noexcept override
			{
				// Nothing to get
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::AudioUnitNode const* const /*parent*/, model::StreamPortOutputNode const& /*node*/) noexcept override
			{
				// Nothing to get
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const /*grandParent*/, model::StreamPortNode const* const /*parent*/, model::AudioClusterNode const& node) noexcept override
			{
				auto const configurationIndex = grandGrandParent->descriptorIndex;
				auto const clusterIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					// Get AudioClusterName
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetName, { configurationIndex, entity::model::DescriptorType::AudioCluster, clusterIndex, std::uint16_t{ 0u } } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName, clusterIndex);
					}
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::AudioUnitNode const* const /*grandParent*/, model::StreamPortNode const* const /*parent*/, model::AudioMapNode const& /*node*/) noexcept override
			{
				// Nothing to get
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const /*grandParent*/, model::StreamPortNode const* const /*parent*/, model::ControlNode const& node) noexcept override
			{
				auto const configurationIndex = grandGrandParent->descriptorIndex;
				auto const controlIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					getControlNodeDynamicInformation(configurationIndex, controlIndex);
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::AudioUnitNode const* const /*parent*/, model::ControlNode const& node) noexcept override
			{
				auto const configurationIndex = grandParent->descriptorIndex;
				auto const controlIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					getControlNodeDynamicInformation(configurationIndex, controlIndex);
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ControlNode const& node) noexcept override
			{
				auto const configurationIndex = parent->descriptorIndex;
				auto const controlIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					getControlNodeDynamicInformation(configurationIndex, controlIndex);
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ClockDomainNode const& node) noexcept override
			{
				auto const configurationIndex = parent->descriptorIndex;
				auto const clockDomainIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					// Get ClockDomainName
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetName, { configurationIndex, entity::model::DescriptorType::ClockDomain, clockDomainIndex, std::uint16_t{ 0u } } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainName, clockDomainIndex);
					}
					// Get ClockDomainSourceIndex
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetClockSource, { clockDomainIndex } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainSourceIndex, clockDomainIndex);
					}
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::ClockDomainNode const* const /*parent*/, model::ClockSourceNode const& /*node*/) noexcept override
			{
				// Runtime built node (virtual node)
			}
			virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::TimingNode const& node) noexcept override
			{
				auto const configurationIndex = parent->descriptorIndex;
				auto const timingIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					// Get TimingName
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetName, { configurationIndex, entity::model::DescriptorType::Timing, timingIndex, std::uint16_t{ 0u } } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::TimingName, timingIndex);
					}
				}
			}
			virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::PtpInstanceNode const& node) noexcept override
			{
				auto const configurationIndex = parent->descriptorIndex;
				auto const ptpInstanceIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					// Get PtpInstanceName
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetName, { configurationIndex, entity::model::DescriptorType::PtpInstance, ptpInstanceIndex, std::uint16_t{ 0u } } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::PtpInstanceName, ptpInstanceIndex);
					}
				}
			}
			virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::TimingNode const* const /*parent*/, la::avdecc::controller::model::PtpInstanceNode const& /*node*/) noexcept override
			{
				// Runtime built node (virtual node)
			}
			virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& node) noexcept override
			{
				auto const configurationIndex = grandParent->descriptorIndex;
				auto const controlIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					getControlNodeDynamicInformation(configurationIndex, controlIndex);
				}
			}
			virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::PtpPortNode const& node) noexcept override
			{
				auto const configurationIndex = grandParent->descriptorIndex;
				auto const ptpPortIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					// Get PtpPortName
					if (_usePackedDynamicInfo)
					{
						_dynamicInfoParameters.emplace_back(entity::controller::DynamicInfoParameter{ entity::LocalEntity::AemCommandStatus::Success, protocol::AemCommandType::GetName, { configurationIndex, entity::model::DescriptorType::PtpPort, ptpPortIndex, std::uint16_t{ 0u } } });
					}
					else
					{
						_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::PtpPortName, ptpPortIndex);
					}
				}
			}
			virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::TimingNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept override
			{
				// Runtime built node (virtual node)
			}
			virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::TimingNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::PtpPortNode const& /*node*/) noexcept override
			{
				// Runtime built node (virtual node)
			}
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::RedundantStreamInputNode const& /*node*/) noexcept override
			{
				// Runtime built node (virtual node)
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::RedundantStreamOutputNode const& /*node*/) noexcept override
			{
				// Runtime built node (virtual node)
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::RedundantStreamNode const* const /*parent*/, model::StreamInputNode const& /*node*/) noexcept override
			{
				// Runtime built node (virtual node)
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::RedundantStreamNode const* const /*parent*/, model::StreamOutputNode const& /*node*/) noexcept override
			{
				// Runtime built node (virtual node)
			}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

			ControllerImpl* _controller{ nullptr };
			ControlledEntityImpl* _entity{ nullptr };
			entity::model::ConfigurationIndex _currentConfigurationIndex{ entity::model::getInvalidDescriptorIndex() };
			bool _usePackedDynamicInfo{ false };
			entity::controller::DynamicInfoParameters _dynamicInfoParameters{};
		};

		// Visit the model, and retrieve dynamic info
		auto visitor = DynamicInfoModelVisitor{ this, entity };
		entity->accept(&visitor, true);
		// Flush all packed dynamic info queries
		flushPackedDynamicInfoQueries(entity, visitor.getDynamicInfoParameters(), ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
	}

	// Get all expected descriptor dynamic information
	if (entity->gotAllExpectedDescriptorDynamicInfo())
	{
		// Clear this enumeration step and check for next one
		entity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
		checkEnumerationSteps(entity);
	}
}

void ControllerImpl::flushPackedDynamicInfoQueries(ControlledEntityImpl* const entity, entity::controller::DynamicInfoParameters const& dynamicInfoParameters, ControlledEntityImpl::EnumerationStep const step) noexcept
{
	if (dynamicInfoParameters.empty())
	{
		return;
	}

	auto const sendQuery = [this, entity, step](auto const& params, auto const& packetID)
	{
		// Send the query
		queryInformation(entity, params, packetID, step);
	};
	static auto s_ResponseSizes = std::unordered_map<protocol::AemCommandType, size_t, protocol::AemCommandType::Hash>{
		{ protocol::AemCommandType::GetConfiguration, protocol::aemPayload::AecpAemGetConfigurationResponsePayloadSize }, // GetConfiguration
		{ protocol::AemCommandType::GetStreamFormat, protocol::aemPayload::AecpAemGetStreamFormatResponsePayloadSize }, // GetStreamFormat
		// GetVideoFormat
		// GetSensorFormat
		// { protocol::AemCommandType::GetStreamInfo, protocol::aemPayload::AecpAemGetStreamInfoResponsePayloadSize }, // GetStreamInfo // DO NOT USE, too many different payload sizes (1722.1-2013, 1722.1-2021, Milan 1.0)
		{ protocol::AemCommandType::GetName, protocol::aemPayload::AecpAemGetNameResponsePayloadSize }, // GetName
		{ protocol::AemCommandType::GetAssociationID, protocol::aemPayload::AecpAemGetAssociationIDResponsePayloadSize }, // GetAssociationID
		{ protocol::AemCommandType::GetSamplingRate, protocol::aemPayload::AecpAemGetSamplingRateResponsePayloadSize }, // GetSamplingRate
		{ protocol::AemCommandType::GetClockSource, protocol::aemPayload::AecpAemGetClockSourceResponsePayloadSize }, // GetClockSource
		// GetSignalSelector
		{ protocol::AemCommandType::GetCounters, protocol::aemPayload::AecpAemGetCountersResponsePayloadSize }, // GetCounters
		{ protocol::AemCommandType::GetMemoryObjectLength, protocol::aemPayload::AecpAemGetMemoryObjectLengthResponsePayloadSize }, // GetMemoryObjectLength
		// GetStreamBackup
		{ protocol::AemCommandType::GetMaxTransitTime, protocol::aemPayload::AecpAemGetMaxTransitTimeResponsePayloadSize }, // GetMaxTransitTime
	};

	auto packetID = std::uint16_t{ 0u };
	auto currentPos = 0u;
	auto const paramsCount = dynamicInfoParameters.size();

#pragma message("TODO: Optimize this code by trying to pack as much as possible in a single query (searching for another smaller command if one doesn't fit)")
	// Build DynamicInfoParameters structs (packing as much as possible), until we sent all the queries
	auto params = decltype(dynamicInfoParameters){};
	auto currentSize = size_t{ 0u };
	while (currentPos < paramsCount)
	{
		auto const& param = dynamicInfoParameters.at(currentPos);

		// Get response size for the command
		if (auto const& it = s_ResponseSizes.find(param.commandType); it != s_ResponseSizes.end())
		{
			auto const responseSize = static_cast<size_t>(it->second + protocol::aemPayload::AecpAemGetDynamicInfoStructureHeaderSize);
			// Check if we can add this command to the current query (not exceeding the maximum payload size)
			if ((currentSize + responseSize) <= protocol::AemAecpdu::MaximumPayloadLength_17221)
			{
				// Add this command to the current query
				params.emplace_back(param);
				currentSize += responseSize;
			}
			else
			{
				// Send the current query
				sendQuery(params, packetID);

				// Start a new query
				params.clear();
				params.emplace_back(param);
				currentSize = responseSize;
				++packetID;
			}
		}
		else
		{
			LOG_CONTROLLER_DEBUG(entity->getEntity().getEntityID(), "ControllerImpl::flushPackedDynamicInfoQueries: Unhandled AemCommandType: {}", static_cast<std::string>(param.commandType));
			AVDECC_ASSERT(false, "Unhandled AemCommandType");
		}

		// Next
		++currentPos;
	}

	// Send the last query
	if (!params.empty())
	{
		sendQuery(params, packetID);
	}
}

class CreateCachedModelVisitor : public model::EntityModelVisitor
{
public:
	model::EntityNode&& getModel() noexcept
	{
		return std::move(_model);
	}

private:
	// model::EntityModelVisitor overrides
	virtual void visit(ControlledEntity const* const /*entity*/, model::EntityNode const& node) noexcept override
	{
		// Create a new EntityNode
		auto entityNode = model::EntityNode{};
		// Copy all static information
		entityNode.staticModel = node.staticModel;
		// Move to the model
		_model = std::move(entityNode);
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::EntityNode const* const /*parent*/, model::ConfigurationNode const& node) noexcept override
	{
		// Create a new ConfigurationNode
		auto configurationNode = model::ConfigurationNode{ node.descriptorIndex };
		// Copy all static information
		configurationNode.staticModel = node.staticModel;
		// Move to the model
		_currentConfiguration = &(_model.configurations.emplace(node.descriptorIndex, std::move(configurationNode)).first->second);
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::AudioUnitNode const& node) noexcept override
	{
		// Create a new AudioUnitNode
		auto audioUnitNode = model::AudioUnitNode{ node.descriptorIndex };
		// Copy all static information
		audioUnitNode.staticModel = node.staticModel;
		// Move to the model
		_currentAudioUnit = &(_currentConfiguration->audioUnits.emplace(node.descriptorIndex, std::move(audioUnitNode)).first->second);
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::StreamInputNode const& node) noexcept override
	{
		// Create a new StreamInputNode
		auto streamInputNode = model::StreamInputNode{ node.descriptorIndex };
		// Copy all static information
		streamInputNode.staticModel = node.staticModel;
		// Move to the model
		_currentConfiguration->streamInputs.emplace(node.descriptorIndex, std::move(streamInputNode));
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::StreamOutputNode const& node) noexcept override
	{
		// Create a new StreamOutputNode
		auto streamOutputNode = model::StreamOutputNode{ node.descriptorIndex };
		// Copy all static information
		streamOutputNode.staticModel = node.staticModel;
		// Move to the model
		_currentConfiguration->streamOutputs.emplace(node.descriptorIndex, std::move(streamOutputNode));
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::JackInputNode const& node) noexcept override
	{
		// Create a new JackInputNode
		auto jackInputNode = model::JackInputNode{ node.descriptorIndex };
		// Copy all static information
		jackInputNode.staticModel = node.staticModel;
		// Move to the model
		_currentJack = &(_currentConfiguration->jackInputs.emplace(node.descriptorIndex, std::move(jackInputNode)).first->second);
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::JackOutputNode const& node) noexcept override
	{
		// Create a new JackOutputNode
		auto jackOutputNode = model::JackOutputNode{ node.descriptorIndex };
		// Copy all static information
		jackOutputNode.staticModel = node.staticModel;
		// Move to the model
		_currentJack = &(_currentConfiguration->jackOutputs.emplace(node.descriptorIndex, std::move(jackOutputNode)).first->second);
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::JackNode const* const /*parent*/, model::ControlNode const& node) noexcept override
	{
		// Create a new ControlNode
		auto controlNode = model::ControlNode{ node.descriptorIndex };
		// Copy all static information
		controlNode.staticModel = node.staticModel;
		// Move to the model
		_currentJack->controls.emplace(node.descriptorIndex, std::move(controlNode));
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::AvbInterfaceNode const& node) noexcept override
	{
		// Create a new AvbInterfaceNode
		auto avbInterfaceNode = model::AvbInterfaceNode{ node.descriptorIndex };
		// Copy all static information
		avbInterfaceNode.staticModel = node.staticModel;
		// Move to the model
		_currentConfiguration->avbInterfaces.emplace(node.descriptorIndex, std::move(avbInterfaceNode));
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::ClockSourceNode const& node) noexcept override
	{
		// Create a new ClockSourceNode
		auto clockSourceNode = model::ClockSourceNode{ node.descriptorIndex };
		// Copy all static information
		clockSourceNode.staticModel = node.staticModel;
		// Move to the model
		_currentConfiguration->clockSources.emplace(node.descriptorIndex, std::move(clockSourceNode));
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::MemoryObjectNode const& node) noexcept override
	{
		// Create a new MemoryObjectNode
		auto memoryObjectNode = model::MemoryObjectNode{ node.descriptorIndex };
		// Copy all static information
		memoryObjectNode.staticModel = node.staticModel;
		// Move to the model
		_currentConfiguration->memoryObjects.emplace(node.descriptorIndex, std::move(memoryObjectNode));
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::LocaleNode const& node) noexcept override
	{
		// Create a new LocaleNode
		auto localeNode = model::LocaleNode{ node.descriptorIndex };
		// Copy all static information
		localeNode.staticModel = node.staticModel;
		// Move to the model
		_currentLocale = &(_currentConfiguration->locales.emplace(node.descriptorIndex, std::move(localeNode)).first->second);
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::LocaleNode const* const /*parent*/, model::StringsNode const& node) noexcept override
	{
		// Create a new StringsNode
		auto stringsNode = model::StringsNode{ node.descriptorIndex };
		// Copy all static information
		stringsNode.staticModel = node.staticModel;
		// Move to the model
		_currentLocale->strings.emplace(node.descriptorIndex, std::move(stringsNode));
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::AudioUnitNode const* const /*parent*/, model::StreamPortInputNode const& node) noexcept override
	{
		// Create a new StreamPortInputNode
		auto streamPortInputNode = model::StreamPortInputNode{ node.descriptorIndex };
		// Copy all static information
		streamPortInputNode.staticModel = node.staticModel;
		// Move to the model
		_currentStreamPort = &(_currentAudioUnit->streamPortInputs.emplace(node.descriptorIndex, std::move(streamPortInputNode)).first->second);
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::AudioUnitNode const* const /*parent*/, model::StreamPortOutputNode const& node) noexcept override
	{
		// Create a new StreamPortOutputNode
		auto streamPortOutputNode = model::StreamPortOutputNode{ node.descriptorIndex };
		// Copy all static information
		streamPortOutputNode.staticModel = node.staticModel;
		// Move to the model
		_currentStreamPort = &(_currentAudioUnit->streamPortOutputs.emplace(node.descriptorIndex, std::move(streamPortOutputNode)).first->second);
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::AudioUnitNode const* const /*grandParent*/, model::StreamPortNode const* const /*parent*/, model::AudioClusterNode const& node) noexcept override
	{
		// Create a new AudioClusterNode
		auto audioClusterNode = model::AudioClusterNode{ node.descriptorIndex };
		// Copy all static information
		audioClusterNode.staticModel = node.staticModel;
		// Move to the model
		_currentStreamPort->audioClusters.emplace(node.descriptorIndex, std::move(audioClusterNode));
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::AudioUnitNode const* const /*grandParent*/, model::StreamPortNode const* const /*parent*/, model::AudioMapNode const& node) noexcept override
	{
		// Create a new AudioMapNode
		auto audioMapNode = model::AudioMapNode{ node.descriptorIndex };
		// Copy all static information
		audioMapNode.staticModel = node.staticModel;
		// Move to the model
		_currentStreamPort->audioMaps.emplace(node.descriptorIndex, std::move(audioMapNode));
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::AudioUnitNode const* const /*grandParent*/, model::StreamPortNode const* const /*parent*/, model::ControlNode const& node) noexcept override
	{
		// Create a new ControlNode
		auto controlNode = model::ControlNode{ node.descriptorIndex };
		// Copy all static information
		controlNode.staticModel = node.staticModel;
		// Move to the model
		_currentStreamPort->controls.emplace(node.descriptorIndex, std::move(controlNode));
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::AudioUnitNode const* const /*parent*/, model::ControlNode const& node) noexcept override
	{
		// Create a new ControlNode
		auto controlNode = model::ControlNode{ node.descriptorIndex };
		// Copy all static information
		controlNode.staticModel = node.staticModel;
		// Move to the model
		_currentAudioUnit->controls.emplace(node.descriptorIndex, std::move(controlNode));
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::ControlNode const& node) noexcept override
	{
		// Create a new ControlNode
		auto controlNode = model::ControlNode{ node.descriptorIndex };
		// Copy all static information
		controlNode.staticModel = node.staticModel;
		// Move to the model
		_currentConfiguration->controls.emplace(node.descriptorIndex, std::move(controlNode));
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::ClockDomainNode const& node) noexcept override
	{
		// Create a new ClockDomainNode
		auto clockDomainNode = model::ClockDomainNode{ node.descriptorIndex };
		// Copy all static information
		clockDomainNode.staticModel = node.staticModel;
		// Move to the model
		_currentConfiguration->clockDomains.emplace(node.descriptorIndex, std::move(clockDomainNode));
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::ClockDomainNode const* const /*parent*/, model::ClockSourceNode const& /*node*/) noexcept override
	{
		// Runtime built node (virtual node)
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::TimingNode const& node) noexcept override
	{
		// Create a new TimingNode
		auto timingNode = model::TimingNode{ node.descriptorIndex };
		// Copy all static information
		timingNode.staticModel = node.staticModel;
		// Move to the model
		_currentConfiguration->timings.emplace(node.descriptorIndex, std::move(timingNode));
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::PtpInstanceNode const& node) noexcept override
	{
		// Create a new PtpInstanceNode
		auto ptpInstanceNode = model::PtpInstanceNode{ node.descriptorIndex };
		// Copy all static information
		ptpInstanceNode.staticModel = node.staticModel;
		// Move to the model
		_currentPtpInstance = &(_currentConfiguration->ptpInstances.emplace(node.descriptorIndex, std::move(ptpInstanceNode)).first->second);
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::TimingNode const* const /*parent*/, la::avdecc::controller::model::PtpInstanceNode const& /*node*/) noexcept override
	{
		// Runtime built node (virtual node)
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& node) noexcept override
	{
		// Create a new ControlNode
		auto controlNode = model::ControlNode{ node.descriptorIndex };
		// Copy all static information
		controlNode.staticModel = node.staticModel;
		// Move to the model
		_currentPtpInstance->controls.emplace(node.descriptorIndex, std::move(controlNode));
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::PtpPortNode const& node) noexcept override
	{
		// Create a new PtpPortNode
		auto ptpPortNode = model::PtpPortNode{ node.descriptorIndex };
		// Copy all static information
		ptpPortNode.staticModel = node.staticModel;
		// Move to the model
		_currentPtpInstance->ptpPorts.emplace(node.descriptorIndex, std::move(ptpPortNode));
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::TimingNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept override
	{
		// Runtime built node (virtual node)
	}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::TimingNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::PtpPortNode const& /*node*/) noexcept override
	{
		// Runtime built node (virtual node)
	}
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::RedundantStreamInputNode const& /*node*/) noexcept override
	{
		// Runtime built node (virtual node)
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::RedundantStreamOutputNode const& /*node*/) noexcept override
	{
		// Runtime built node (virtual node)
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::RedundantStreamNode const* const /*parent*/, model::StreamInputNode const& /*node*/) noexcept override
	{
		// Runtime built node (virtual node)
	}
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::RedundantStreamNode const* const /*parent*/, model::StreamOutputNode const& /*node*/) noexcept override
	{
		// Runtime built node (virtual node)
	}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	// Private members
	model::EntityNode _model{};
	model::ConfigurationNode* _currentConfiguration{ nullptr };
	model::JackNode* _currentJack{ nullptr };
	model::LocaleNode* _currentLocale{ nullptr };
	model::AudioUnitNode* _currentAudioUnit{ nullptr };
	model::StreamPortNode* _currentStreamPort{ nullptr };
	model::PtpInstanceNode* _currentPtpInstance{ nullptr };
};


void ControllerImpl::checkEnumerationSteps(ControlledEntityImpl* const controlledEntity) noexcept
{
	auto& entity = *controlledEntity;
	auto const steps = entity.getEnumerationSteps();

	// Always start with retrieving MilanInfo from the device
	if (steps.test(ControlledEntityImpl::EnumerationStep::GetMilanInfo))
	{
		getMilanInfo(controlledEntity);
		return;
	}
	// Then check if GET_DYNAMIC_INFO command is supported (required for fast enumeration)
	if (steps.test(ControlledEntityImpl::EnumerationStep::CheckPackedDynamicInfoSupported))
	{
		checkDynamicInfoSupported(controlledEntity);
		return;
	}
	// Then register to unsolicited notifications
	if (steps.test(ControlledEntityImpl::EnumerationStep::RegisterUnsol))
	{
		registerUnsol(controlledEntity);
		return;
	}
	// Then get the static AEM
	if (steps.test(ControlledEntityImpl::EnumerationStep::GetStaticModel))
	{
		getStaticModel(controlledEntity);
		return;
	}
	// Notify the entity model has been fully enumerated
	entity.onEntityModelEnumerated();
	// Then get descriptors dynamic information (if AEM was cached)
	if (steps.test(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo))
	{
		getDescriptorDynamicInfo(controlledEntity);
		return;
	}
	// Finally retrieve all other dynamic information
	if (steps.test(ControlledEntityImpl::EnumerationStep::GetDynamicInfo))
	{
		getDynamicInfo(controlledEntity);
		return;
	}

	// Ready to advertise the entity
	if (!entity.wasAdvertised())
	{
		if (!entity.gotFatalEnumerationError())
		{
			// Notify the ControlledEntity it has been fully loaded
			entity.onEntityFullyLoaded();

			// Validate the entity, now that it's fully enumerated
			validateEntity(entity);

			// Validation didn't go that well, cancel entity advertising
			if (entity.gotFatalEnumerationError())
			{
				return;
			}

			// Do some final controller related steps before advertising entity
			onPreAdvertiseEntity(entity);

			// Check for AEM caching
			auto& entityModelCache = EntityModelCache::getInstance();
			auto const& e = entity.getEntity();
			auto const& entityID = e.getEntityID();
			auto const& entityModelID = e.getEntityModelID();
			// If AEM Cache is Enabled, the entity has a valid non-Group EntityModelID, and it's not in the ignore list
			if (entityModelCache.isCacheEnabled() && entityModelID && !entityModelID.isGroupIdentifier() && !entity.shouldIgnoreCachedEntityModel())
			{
				if (EntityModelCache::isValidEntityModelID(entityModelID))
				{
					// Create a copy of the static part of EntityModel
					auto visitor = CreateCachedModelVisitor{};
					controlledEntity->accept(&visitor, true); // Always visit all configurations, we need to retrieve the locales/strings from all configurations regardless of full static model enumeration

					// Store EntityModel in the cache for later use
					entityModelCache.cacheEntityModel(entityModelID, visitor.getModel(), _fullStaticModelEnumeration);
					LOG_CONTROLLER_INFO(entityID, "AEM-CACHE: Cached model for EntityModelID {}", utils::toHexString(entityModelID, true, false));
				}
				else
				{
					LOG_CONTROLLER_INFO(entityID, "AEM-CACHE: Not caching model with invalid EntityModelID {} (invalid Vendor OUI-24)", utils::toHexString(entityModelID, true, false));
				}
			}

			// Advertise the entity
			entity.setAdvertised(true);

			// Notify it is online
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityOnline, this, controlledEntity);

			// Do some final controller related steps after advertising entity
			onPostAdvertiseEntity(entity);
		}
	}
}

entity::model::AudioMappings ControllerImpl::validateMappings(ControlledEntityImpl& controlledEntity, std::uint16_t const maxStreams, model::StreamPortNode const& streamPortNode, entity::model::AudioMappings const& mappings) const noexcept
{
	auto fixedMappings = std::decay_t<decltype(mappings)>{};

	for (auto const& mapping : mappings)
	{
		if (mapping.streamIndex >= maxStreams)
		{
			// Flag the entity as "Misbehaving"
			setMisbehavingCompatibilityFlag(this, controlledEntity, "IEEE1722.1-2021 - 7.4.44.2.1/7.2.2", "Invalid Mapping received: StreamIndex is greater than maximum number of Streams");
			continue;
		}
		if (mapping.clusterOffset >= streamPortNode.staticModel.numberOfClusters)
		{
			// Flag the entity as "Misbehaving"
			setMisbehavingCompatibilityFlag(this, controlledEntity, "IEEE1722.1-2021 - 7.4.44.2.1/7.2.13", "Invalid Mapping received: ClusterOffset is greater than cluster in the StreamPort");
			continue;
		}
		if (auto const clusterIt = streamPortNode.audioClusters.find(streamPortNode.staticModel.baseCluster + mapping.clusterOffset); clusterIt != streamPortNode.audioClusters.end())
		{
			if (mapping.clusterChannel >= clusterIt->second.staticModel.channelCount)
			{
				// Flag the entity as "Misbehaving"
				setMisbehavingCompatibilityFlag(this, controlledEntity, "IEEE1722.1-2021 - 7.4.44.2.1/7.2.16", "Invalid Mapping received: ClusterChannel is greater than channels in the AudioCluster");
				continue;
			}
		}

		fixedMappings.push_back(mapping);
	}

	return fixedMappings;
}

bool ControllerImpl::validateIdentifyControl(ControlledEntityImpl& controlledEntity, model::ControlNode const& identifyControlNode) noexcept
{
	AVDECC_ASSERT(entity::model::StandardControlType::Identify == identifyControlNode.staticModel.controlType.getValue(), "validateIdentifyControl should only be called on an IDENTIFY Control Descriptor Type");
	auto const controlIndex = identifyControlNode.descriptorIndex;

	try
	{
		auto const controlValueType = identifyControlNode.staticModel.controlValueType.getType();
		if (controlValueType == entity::model::ControlValueType::Type::ControlLinearUInt8)
		{
			auto const staticValues = identifyControlNode.staticModel.values.getValues<entity::model::LinearValues<entity::model::LinearValueStatic<std::uint8_t>>>();
			if (staticValues.countValues() == 1)
			{
				auto const& staticValue = staticValues.getValues()[0];
				if (staticValue.minimum == 0 && staticValue.maximum == 255 && staticValue.step == 255 && staticValue.unit.getMultiplier() == 0 && staticValue.unit.getUnit() == entity::model::ControlValueUnit::Unit::Unitless)
				{
					auto const dynamicValues = identifyControlNode.dynamicModel.values.getValues<entity::model::LinearValues<entity::model::LinearValueDynamic<std::uint8_t>>>();
					if (dynamicValues.countValues() == 1)
					{
						auto const& dynamicValue = dynamicValues.getValues()[0];
						if (dynamicValue.currentValue == 0 || dynamicValue.currentValue == 255)
						{
							// Warning only checks
							if (identifyControlNode.staticModel.signalType != entity::model::DescriptorType::Invalid || identifyControlNode.staticModel.signalIndex != 0)
							{
								// Flag the entity as "Not fully IEEE1722.1 compliant"
								removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.2.22", "ControlDescriptor at Index " + std::to_string(controlIndex) + " is not a valid Identify Control: SignalType should be set to INVALID and SignalIndex to 0");
							}

							// All (or almost) ok
							return true;
						}
						else
						{
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.2.22", "ControlDescriptor at Index " + std::to_string(controlIndex) + " is not a valid Identify Control: CurrentValue should either be 0 or 255 but is " + std::to_string(dynamicValue.currentValue));
						}
					}
					else
					{
						// Flag the entity as "Not fully IEEE1722.1 compliant"
						removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.2.22", "ControlDescriptor at Index " + std::to_string(controlIndex) + " is not a valid Identify Control: DynamicValues should only contain one value but has " + std::to_string(dynamicValues.countValues()));
					}
				}
				else
				{
					// Flag the entity as "Not fully IEEE1722.1 compliant"
					removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.2.22", "ControlDescriptor at Index " + std::to_string(controlIndex) + " is not a valid Identify Control: One or many fields are incorrect and should be min=0, max=255, step=255, Unit=UNITLESS/0");
				}
			}
			else
			{
				// Flag the entity as "Not fully IEEE1722.1 compliant"
				removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.2.22", "ControlDescriptor at Index " + std::to_string(controlIndex) + " is not a valid Identify Control: Should only contain one value but has " + std::to_string(staticValues.countValues()));
			}
		}
		else
		{
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.2.22", "ControlDescriptor at Index " + std::to_string(controlIndex) + " is not a valid Identify Control: ControlValueType should be CONTROL_LINEAR_UINT8 but is " + entity::model::controlValueTypeToString(controlValueType));
		}
	}
	catch (std::invalid_argument const& e)
	{
		// Flag the entity as "Not fully IEEE1722.1 compliant"
		removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.2.22", "ControlDescriptor at Index " + std::to_string(controlIndex) + " is not a valid Identify Control: " + e.what());
	}
	catch (...)
	{
		// Flag the entity as "Not fully IEEE1722.1 compliant"
		removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.2.22", "ControlDescriptor at Index " + std::to_string(controlIndex) + " is not a valid Identify Control: Unknown exception trying to read descriptor");
	}

	return false;
}

ControllerImpl::DynamicControlValuesValidationResult ControllerImpl::validateControlValues([[maybe_unused]] UniqueIdentifier const entityID, entity::model::ControlIndex const controlIndex, UniqueIdentifier const& controlType, entity::model::ControlValueType::Type const controlValueType, entity::model::ControlValues const& staticValues, entity::model::ControlValues const& dynamicValues) noexcept
{
	if (!staticValues)
	{
		// Returning true here because uninitialized values may be due to a type unknown to the library
		return DynamicControlValuesValidationResult{ DynamicControlValuesValidationResultKind::Valid, "INTERNAL", "StaticValues (type " + entity::model::controlValueTypeToString(controlValueType) + ") for ControlDescriptor at Index " + std::to_string(controlIndex) + " are not initialized (probably unhandled type)" };
	}

	if (staticValues.areDynamicValues())
	{
		return DynamicControlValuesValidationResult{ DynamicControlValuesValidationResultKind::InvalidValues, "INTERNAL", "StaticValues for ControlDescriptor at Index " + std::to_string(controlIndex) + " are dynamic instead of static" };
	}

	if (!dynamicValues)
	{
		// Returning true here because uninitialized values may be due to a type unknown to the library
		return DynamicControlValuesValidationResult{ DynamicControlValuesValidationResultKind::InvalidValues, "INTERNAL", "DynamicValues (type " + entity::model::controlValueTypeToString(controlValueType) + ") for ControlDescriptor at Index " + std::to_string(controlIndex) + " are not initialized (probably unhandled type)" };
	}

	if (!dynamicValues.areDynamicValues())
	{
		return DynamicControlValuesValidationResult{ DynamicControlValuesValidationResultKind::InvalidValues, "INTERNAL", "DynamicValues for ControlDescriptor at Index " + std::to_string(controlIndex) + " are static instead of dynamic" };
	}

	auto const [result, errMessage] = entity::model::validateControlValues(staticValues, dynamicValues);

	// No error during validation
	if (result == entity::model::ControlValuesValidationResult::Valid)
	{
		return DynamicControlValuesValidationResult{ DynamicControlValuesValidationResultKind::Valid, "", "" };
	}

	// Checking for special (allowed) cases that are only warnings
	switch (result)
	{
		case entity::model::ControlValuesValidationResult::CurrentValueBelowMinimum:
		case entity::model::ControlValuesValidationResult::CurrentValueAboveMaximum:
		{
			switch (controlType.getValue())
			{
				case utils::to_integral(entity::model::StandardControlType::PowerStatus):
				case utils::to_integral(entity::model::StandardControlType::FanStatus):
				case utils::to_integral(entity::model::StandardControlType::Temperature):
					LOG_CONTROLLER_DEBUG(entityID, "Warning for DynamicValues for ControlDescriptor at Index {}: {}", controlIndex, errMessage);
					return DynamicControlValuesValidationResult{ DynamicControlValuesValidationResultKind::CurrentValueOutOfRange, "", "" };
				default:
					// Also return CurrentValueOutOfRange for non-standard controls
					if (controlType.getVendorID() != entity::model::StandardControlTypeVendorID)
					{
						LOG_CONTROLLER_DEBUG(entityID, "Warning for DynamicValues for Non-Standard ControlDescriptor at Index {}: {}", controlIndex, errMessage);
						return DynamicControlValuesValidationResult{ DynamicControlValuesValidationResultKind::CurrentValueOutOfRange, "", "" };
					}
					break;
			}
		}
		default:
			break;
	}

	return DynamicControlValuesValidationResult{ DynamicControlValuesValidationResultKind::InvalidValues, "INTERNAL", "DynamicValues for ControlDescriptor at Index " + std::to_string(controlIndex) + " are not valid: " + errMessage };
}

void ControllerImpl::validateControlDescriptors(ControlledEntityImpl& controlledEntity) noexcept
{
	class ControlDescriptorValidationVisitor : public model::DefaultedEntityModelVisitor
	{
	public:
		ControlDescriptorValidationVisitor(ControlledEntityImpl& controlledEntity) noexcept
			: _controlledEntity{ controlledEntity }
			, _entity{ _controlledEntity.getEntity() }
			, _entityID{ _entity.getEntityID() }
		{
			if (_entity.getEntityCapabilities().test(entity::EntityCapability::AemIdentifyControlIndexValid))
			{
				_adpIdentifyControlIndex = _entity.getIdentifyControlIndex();
				if (!_adpIdentifyControlIndex)
				{
					// Flag the entity as "Not fully IEEE1722.1 compliant"
					removeCompatibilityFlag(nullptr, _controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 6.2.2", "AEM_IDENTIFY_CONTROL_INDEX_VALID bit is set in ADP but ControlIndex is invalid: CONTROL index not defined in ADP");
				}
			}
		}

		void validate() const noexcept
		{
			// Check we found a valid Identify Control at either Configuration or Jack level, if ADP contains a valid Identify Control Index
			if (_adpIdentifyControlIndex && !_foundADPIdentifyControlIndex)
			{
				// Flag the entity as "Not fully IEEE1722.1 compliant"
				removeCompatibilityFlag(nullptr, _controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 6.2.2", "AEM_IDENTIFY_CONTROL_INDEX_VALID bit is set in ADP but ControlIndex is invalid: No valid CONTROL at index " + std::to_string(*_adpIdentifyControlIndex));
			}
		}

		std::optional<entity::model::ControlIndex> getIdentifyControlIndex() const noexcept
		{
			// If ADP contains a valid Identify Control Index, use it
			if (_foundADPIdentifyControlIndex)
			{
				return _adpIdentifyControlIndex;
			}
			if (!_controlIndices.empty())
			{
				// Right now, return the first Identify Control found
				return *_controlIndices.begin();
			}
			return std::nullopt;
		}

		// Deleted compiler auto-generated methods
		ControlDescriptorValidationVisitor(ControlDescriptorValidationVisitor const&) = delete;
		ControlDescriptorValidationVisitor(ControlDescriptorValidationVisitor&&) = delete;
		ControlDescriptorValidationVisitor& operator=(ControlDescriptorValidationVisitor const&) = delete;
		ControlDescriptorValidationVisitor& operator=(ControlDescriptorValidationVisitor&&) = delete;

	private:
		static bool isIdentifyControl(model::ControlNode const& node) noexcept
		{
			return entity::model::StandardControlType::Identify == node.staticModel.controlType.getValue();
		}

		// Validate this is the Identify Control advertised by ADP and it is valid. Returns true if this is an Identify Control (valid or not), false otherwise
		[[nodiscard]] bool validateAdpIdentifyControl(model::ControlNode const& node) noexcept
		{
			auto const controlIndex = node.descriptorIndex;

			if (_adpIdentifyControlIndex && *_adpIdentifyControlIndex == controlIndex)
			{
				if (isIdentifyControl(node))
				{
					if (validateIdentifyControl(_controlledEntity, node))
					{
						_foundADPIdentifyControlIndex = true;
						_controlIndices.insert(controlIndex);
					}
					// Note: No need to remove compatibility flag or log a warning in else statement, the validateIdentifyControl method already did it
					return true;
				}
				else
				{
					// Flag the entity as "Not fully IEEE1722.1 compliant"
					removeCompatibilityFlag(nullptr, _controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 6.2.2", "AEM_IDENTIFY_CONTROL_INDEX_VALID bit is set in ADP but ControlIndex is invalid: ControlType should be IDENTIFY but is " + entity::model::controlTypeToString(node.staticModel.controlType));
				}
			}
			return false;
		}

		void validateControl(model::ControlNode const& node, bool const checkIdentifyControl = true, bool const identifyAllowed = false) noexcept
		{
			auto const controlIndex = node.descriptorIndex;

			// Check if we have an Identify Control (not already checked)
			if (checkIdentifyControl && isIdentifyControl(node))
			{
				if (validateIdentifyControl(_controlledEntity, node))
				{
					if (identifyAllowed)
					{
						_controlIndices.insert(controlIndex);
					}
					else
					{
						// Flag the entity as "Not fully IEEE1722.1 compliant"
						removeCompatibilityFlag(nullptr, _controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 6.2.2", "ControlDescriptor at Index " + std::to_string(controlIndex) + " is a valid Identify Control but it's neither at CONFIGURATION nor JACK level");
					}
				}
				// Note: No need to remove compatibility flag or log a warning in else statement, the validateIdentifyControl method already did it
			}

			// Validate ControlType
			auto const controlType = node.staticModel.controlType;
			if (!controlType.isValid())
			{
				// Flag the entity as "Not fully IEEE1722.1 compliant"
				removeCompatibilityFlag(nullptr, _controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.2.22", "control_type for CONTROL descriptor at index " + std::to_string(controlIndex) + " is not a valid EUI-64: " + utils::toHexString(controlType));
			}

			// Validate ControlValues
			auto const [validationResult, specClause, message] = validateControlValues(_entityID, controlIndex, controlType, node.staticModel.controlValueType.getType(), node.staticModel.values, node.dynamicModel.values);
			auto isOutOfBounds = false;
			switch (validationResult)
			{
				case DynamicControlValuesValidationResultKind::InvalidValues:
					// Flag the entity as "Not fully IEEE1722.1 compliant"
					removeCompatibilityFlag(nullptr, _controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, specClause, message);
					break;
				case DynamicControlValuesValidationResultKind::CurrentValueOutOfRange:
					isOutOfBounds = true;
					break;
				default:
					break;
			}
			updateControlCurrentValueOutOfBounds(nullptr, _controlledEntity, controlIndex, isOutOfBounds);
		}

		// model::DefaultedEntityModelVisitor overrides
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::JackNode const* const /*parent*/, model::ControlNode const& node) noexcept override
		{
			// Jack level, we need to validate ADP Identify Control Index if present and this index
			auto const isIdentifyControl = validateAdpIdentifyControl(node);

			// Validate the Control
			validateControl(node, !isIdentifyControl, true);
		}

		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::AudioUnitNode const* const /*grandParent*/, model::StreamPortNode const* const /*parent*/, model::ControlNode const& node) noexcept override
		{
			// Validate the Control
			validateControl(node);
		}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::AudioUnitNode const* const /*parent*/, model::ControlNode const& node) noexcept override
		{
			// Validate the Control
			validateControl(node);
		}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::ControlNode const& node) noexcept override
		{
			// Configuration level, we need to validate ADP Identify Control Index if present and this index
			auto const isIdentifyControl = validateAdpIdentifyControl(node);

			// Validate the Control
			validateControl(node, !isIdentifyControl, true);
		}

		ControlledEntityImpl& _controlledEntity;
		entity::Entity& _entity;
		UniqueIdentifier _entityID{};
		std::optional<entity::model::ControlIndex> _adpIdentifyControlIndex{ std::nullopt };
		std::set<entity::model::ControlIndex> _controlIndices{};
		bool _foundADPIdentifyControlIndex{ false };
	};

	auto const& e = controlledEntity.getEntity();
	auto const isAemSupported = e.getEntityCapabilities().test(entity::EntityCapability::AemSupported);

	// If AEM is supported
	if (isAemSupported && controlledEntity.hasAnyConfiguration())
	{
		// Use a visitor to:
		//  1/ Find an Identify Control Descriptor: must exist at Configuration or Jack level, if advertised in ADP. Otherwise just store the ControlIndex
		//  2/ Validate all Control Descriptors
		auto visitor = ControlDescriptorValidationVisitor{ controlledEntity };

		// Run the visitor on the entity model
		controlledEntity.accept(&visitor);

		// Validate post-visitor checks
		visitor.validate();

		// If we found a valid Identify Control Descriptor, store it in the entity
		if (auto const identifyControlIndex = visitor.getIdentifyControlIndex(); identifyControlIndex)
		{
			controlledEntity.setIdentifyControlIndex(*identifyControlIndex);
		}
	}
}

void ControllerImpl::validateRedundancy(ControlledEntityImpl& controlledEntity) noexcept
{
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	// Only for Milan devices
	if (controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
	{
		try
		{
			// Check if the current configuration has at least one Redundant Stream
			auto const& configurationNode = controlledEntity.getCurrentConfigurationNode();
			auto const hasRedundantStream = !configurationNode.redundantStreamInputs.empty() || !configurationNode.redundantStreamOutputs.empty();

			// Check the entity correctly declares the Milan Redundancy Flag
			auto const milanInfo = controlledEntity.getMilanInfo();
			if (hasRedundantStream != milanInfo->featuresFlags.test(la::avdecc::entity::MilanInfoFeaturesFlag::Redundancy))
			{
				if (hasRedundantStream)
				{
					setMilanWarningCompatibilityFlag(nullptr, controlledEntity, "Milan 1.3 - 5.4.4.1", "Redundant Streams detected, but MilanInfo features_flags does not contain REDUNDANCY bit");
				}
				else
				{
					setMilanWarningCompatibilityFlag(nullptr, controlledEntity, "Milan 1.3 - 5.4.4.1", "MilanInfo features_flags contains REDUNDANCY bit, but active Configuration does not have a single valid Redundant Stream");
				}
			}

			// No need to check for AVB Interface association, the buildRedundancyNodesByType method already did the check when creating redundantStreamInputs and redundantStreamOutputs

			// Set the entity as Milan Redundant for the active configuration
			controlledEntity.setMilanRedundant(hasRedundantStream);
		}
		catch (ControlledEntity::Exception const&)
		{
			// Something went wrong during enumeration, set critical error
			controlledEntity.setGetFatalEnumerationError();
		}
	}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
}

// Checks if AAF audio format is a Milan Base Format - Milan 1.3 Clause 6.2
static bool isMilanBaseAudioFormat(entity::model::StreamFormatInfo const& formatInfo) noexcept
{
	AVDECC_ASSERT(formatInfo.getType() == entity::model::StreamFormatInfo::Type::AAF, "Format is not AAF");

	// Milan Base Audio Format has either 1, 2, 4, 6 or 8 channels
	switch (formatInfo.getChannelsCount())
	{
		case 1:
		case 2:
		case 4:
		case 6:
		case 8:
			break;
		default:
			// Check for up-to
			if (formatInfo.isUpToChannelsCount() && formatInfo.getChannelsCount() >= 1)
			{
				break;
			}
			// Not a Milan Base Audio Format
			return false;
	}

	// Milan Base Audio Format has either 48, 96 or 192 kHz sample rate
	auto const [pull, freq] = formatInfo.getSamplingRate().getPullBaseFrequency();
	if (pull != 0)
	{
		return false;
	}
	switch (freq)
	{
		case 48000:
		case 96000:
		case 192000:
			break;
		default:
			return false;
	}

	// Milan Base Audio Format has 32 bits depth
	if (formatInfo.getSampleBitDepth() != 32)
	{
		return false;
	}
	return true;
}

void ControllerImpl::validateEntityModel(ControlledEntityImpl& controlledEntity) noexcept
{
	auto const& e = controlledEntity.getEntity();
	auto const entityID = e.getEntityID();
	auto const isAemSupported = e.getEntityCapabilities().test(entity::EntityCapability::AemSupported);

	// If AEM is supported
	if (isAemSupported)
	{
		// IEEE1722.1-2013 Clause 7.2.1 - A device is required to have at least one Configuration Descriptor
		if (!controlledEntity.hasAnyConfiguration())
		{
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.2.1", "A device is required to have at least one Configuration Descriptor");
			return;
		}

		// IEEE1722.1-2021 Clause 7.2.2 - The descriptor_counts field is the counts of the top level descriptors.
		try
		{
			static auto const s_TopLevelDescriptors = std::set<entity::model::DescriptorType>{ entity::model::DescriptorType::AudioUnit, entity::model::DescriptorType::VideoUnit, entity::model::DescriptorType::SensorUnit, entity::model::DescriptorType::StreamInput, entity::model::DescriptorType::StreamOutput, entity::model::DescriptorType::JackInput, entity::model::DescriptorType::JackOutput, entity::model::DescriptorType::AvbInterface, entity::model::DescriptorType::ClockSource, entity::model::DescriptorType::Control, entity::model::DescriptorType::SignalSelector, entity::model::DescriptorType::Mixer, entity::model::DescriptorType::Matrix, entity::model::DescriptorType::Locale, entity::model::DescriptorType::MatrixSignal, entity::model::DescriptorType::MemoryObject,
				entity::model::DescriptorType::SignalSplitter, entity::model::DescriptorType::SignalCombiner, entity::model::DescriptorType::SignalDemultiplexer, entity::model::DescriptorType::SignalMultiplexer, entity::model::DescriptorType::SignalTranscoder, entity::model::DescriptorType::ClockDomain, entity::model::DescriptorType::ControlBlock, entity::model::DescriptorType::Timing, entity::model::DescriptorType::PtpInstance };
			auto const& currentConfigurationNode = controlledEntity.getCurrentConfigurationNode();
			for (auto const [descriptorType, count] : currentConfigurationNode.staticModel.descriptorCounts)
			{
				// If a declared 'count' is not in the list of top level descriptors, flag the entity as "Not fully IEEE1722.1 compliant"
				if (s_TopLevelDescriptors.count(descriptorType) == 0)
				{
					// First check if this is an unknown descriptor (to this version of the library), so we don't print a warning for something we don't know
					if (utils::to_integral(descriptorType) > utils::to_integral(entity::model::DescriptorType::LAST_VALID_DESCRIPTOR))
					{
						LOG_CONTROLLER_DEBUG(entityID, "Unknown DescriptorType {} found in descriptor_counts field", utils::toHexString(utils::to_integral(descriptorType)));
						continue;
					}
					LOG_CONTROLLER_WARN(entityID, "[IEEE1722.1-2021 Clause 7.2.2] The descriptor_counts field is the counts of the top level descriptors: DescriptorType {} is not a top level descriptor", entity::model::descriptorTypeToString(descriptorType));
					addCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221Warning);
					return;
				}
			}
		}
		catch (ControlledEntity::Exception const&)
		{
			LOG_CONTROLLER_DEBUG(entityID, "Couldn't find any current configuration although at least one was declared");
			// Ignore
		}

		// Check IEEE1722.1 aemxml/json requirements
#ifdef ENABLE_AVDECC_FEATURE_JSON
		try
		{
			// Try to serialize the entity model and check for errors
			entity::model::jsonSerializer::createJsonObject(controlledEntity.getEntityModelTree(), entity::model::jsonSerializer::Flags{ entity::model::jsonSerializer::Flag::ProcessStaticModel, entity::model::jsonSerializer::Flag::ProcessDynamicModel });
		}
		catch (json::exception const&)
		{
			AVDECC_ASSERT(false, "json::exception is not expected to be thrown here");
		}
		catch (avdecc::jsonSerializer::SerializationException const& e)
		{
			if (e.getError() == avdecc::jsonSerializer::SerializationError::InvalidDescriptorIndex)
			{
				// Flag the entity as "Not fully IEEE1722.1 compliant"
				removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2013 - 7.2", "Invalid Descriptor Numbering: " + std::string{ e.what() });
			}
		}
		catch (...)
		{
			AVDECC_ASSERT(false, "Exception type other than avdecc::jsonSerializer::SerializationException are not expected to be thrown here");
		}
#endif // ENABLE_AVDECC_FEATURE_JSON

		// Check Milan requirements
		try
		{
			using CapableStreams = std::map<entity::model::StreamIndex, bool>;
			auto const validateStreamFormats = [](auto const& /*entityID*/, auto& controlledEntity, auto const& streams, auto const& milanSpecificationVersion)
			{
				auto avnuAudioCapableStreams = CapableStreams{};
				auto avnuCrfCapableStreams = CapableStreams{};

				for (auto const& [streamIndex, streamNode] : streams)
				{
					auto streamHasAAFFormat = false;
					auto streamHasAVnuBaseFormat = false;
					auto streamHasAVnuCrf = false;
					for (auto const& format : streamNode.staticModel.formats)
					{
						auto const f = entity::model::StreamFormatInfo::create(format);
						switch (f->getType())
						{
							case entity::model::StreamFormatInfo::Type::AAF:
								streamHasAAFFormat = true;
								if (isMilanBaseAudioFormat(*f))
								{
									streamHasAVnuBaseFormat = true;
									avnuAudioCapableStreams[streamIndex] = true;
								}
								break;
							case entity::model::StreamFormatInfo::Type::ClockReference:
								// Milan 1.3 - Clause 7.3
								if (format.getValue() == 0x041060010000BB80)
								{
									streamHasAVnuCrf = true;
									avnuCrfCapableStreams[streamIndex] = true;
								}
								break;
							default:
								break;
						}
					}
					// Check Milan 1.3 specific requirements
					if (milanSpecificationVersion >= entity::model::MilanVersion{ 1, 3 })
					{
						// [Milan 1.3 Clause 5.3.3.4] If a Stream Input/Output supports the Avnu Pro Audio CRF Media Clock Stream Format, it shall not support any other AAF Audio Stream Formats, and vice versa
						if (streamHasAAFFormat && streamHasAVnuCrf) // Since Milan 1.3, all AAF variants are mutually exclusive with AVnu CRF
						{
							// Decrease "Milan compatibility" down to 1.2 (for now)
							decreaseMilanCompatibilityVersion(nullptr, controlledEntity, entity::model::MilanVersion{ 1, 2 }, "Milan 1.3 - 5.3.3.4", "If a Stream Input/Output supports the Avnu Pro Audio CRF Media Clock Stream Format, it shall not support any other AAF Audio Stream Formats, and vice versa");
						}
					}
					// Now check Milan 1.2 specific requirements which is less restrictive
					if (milanSpecificationVersion >= entity::model::MilanVersion{ 1, 0 })
					{
						// [Milan 1.2 Clause 5.3.3.4] If a STREAM_INPUT/OUTPUT supports the Avnu Pro Audio CRF Media Clock Stream Format, it shall not support the Avnu Pro Audio AAF Audio Stream Format, and vice versa
						if (streamHasAVnuBaseFormat && streamHasAVnuCrf)
						{
							// Remove "Milan compatibility"
							removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.2 - 5.3.3.4", " If a STREAM_INPUT/OUTPUT supports the Avnu Pro Audio CRF Media Clock Stream Format, it shall not support the Avnu Pro Audio AAF Audio Stream Format, and vice versa");
						}
					}
				}
				return std::make_pair(avnuAudioCapableStreams, avnuCrfCapableStreams);
			};

			auto const countCapableStreamsForDomain = [](auto const& streams, auto const& capableStreams, auto const domainIndex)
			{
				auto countStreams = 0u;
				// Process non-redundant streams
				for (auto const& [streamIndex, streamNode] : streams)
				{
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					if (!streamNode.isRedundant)
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					{
						if (streamNode.staticModel.clockDomainIndex == domainIndex)
						{
							if (capableStreams.count(streamIndex) > 0)
							{
								++countStreams;
							}
						}
					}
				}
				return countStreams;
			};

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
			auto const countCapableRedundantStreamsForDomain = [](auto const& streams, auto const& redundantStreams, auto const& capableStreams, auto const domainIndex)
			{
				auto countStreams = 0u;
				// Process redundant streams
				for (auto const& [virtualIndex, redundantStreamNode] : redundantStreams)
				{
					// Take the first (not necessarily the primary) stream
					auto const streamIndex = *redundantStreamNode.redundantStreams.begin();
					if (auto const streamIt = streams.find(streamIndex); streamIt != streams.end())
					{
						if (streamIt->second.staticModel.clockDomainIndex == domainIndex)
						{
							if (capableStreams.count(streamIndex) > 0)
							{
								++countStreams;
							}
						}
					}
				}
				return countStreams;
			};
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

			// Milan devices AEM validation
			if (controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
			{
				auto const milanInfo = controlledEntity.getMilanInfo();
				if (milanInfo)
				{
					auto const& configurationNode = controlledEntity.getCurrentConfigurationNode();

					auto avnuAudioInputStreams = CapableStreams{};
					auto avnuCrfInputStreams = CapableStreams{};
					auto avnuAudioOutputStreams = CapableStreams{};
					auto avnuCrfOutputStreams = CapableStreams{};
					auto isAVnuAudioMediaListener = false;
					auto isAVnuAudioMediaTalker = false;
					// Validate stream formats
					if (!configurationNode.streamInputs.empty())
					{
						auto caps = validateStreamFormats(entityID, controlledEntity, configurationNode.streamInputs, milanInfo->specificationVersion);
						avnuAudioInputStreams = std::move(caps.first);
						avnuCrfInputStreams = std::move(caps.second);
						isAVnuAudioMediaListener = !avnuAudioInputStreams.empty();
					}
					if (!configurationNode.streamOutputs.empty())
					{
						auto caps = validateStreamFormats(entityID, controlledEntity, configurationNode.streamOutputs, milanInfo->specificationVersion);
						avnuAudioOutputStreams = std::move(caps.first);
						avnuCrfOutputStreams = std::move(caps.second);
						isAVnuAudioMediaTalker = !avnuAudioOutputStreams.empty();
					}

					// Validate AAF requirements
					// [Milan Formats] A PAAD-AE shall have at least one Configuration that contains at least one Stream which advertises support for a Base format in its list of supported formats
					if (!isAVnuAudioMediaListener && !isAVnuAudioMediaTalker)
					{
						// Remove "Milan compatibility"
						removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 6.3/6.4", "A PAAD-AE shall have at least one Configuration that contains at least one Stream which advertises support for a Base format in its list of supported formats");
					}

					// Validate CRF requirements for domains
					for (auto const& [domainIndex, domainNode] : configurationNode.clockDomains)
					{
						auto avnuAudioInputStreamsForDomain = countCapableStreamsForDomain(configurationNode.streamInputs, avnuAudioInputStreams, domainIndex);
						auto avnuCrfInputStreamsForDomain = countCapableStreamsForDomain(configurationNode.streamInputs, avnuCrfInputStreams, domainIndex);
						auto avnuCrfOutputStreamsForDomain = countCapableStreamsForDomain(configurationNode.streamOutputs, avnuCrfOutputStreams, domainIndex);
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
						avnuAudioInputStreamsForDomain += countCapableRedundantStreamsForDomain(configurationNode.streamInputs, configurationNode.redundantStreamInputs, avnuAudioInputStreams, domainIndex);
						avnuCrfInputStreamsForDomain += countCapableRedundantStreamsForDomain(configurationNode.streamInputs, configurationNode.redundantStreamInputs, avnuCrfInputStreams, domainIndex);
						avnuCrfOutputStreamsForDomain += countCapableRedundantStreamsForDomain(configurationNode.streamOutputs, configurationNode.redundantStreamOutputs, avnuCrfOutputStreams, domainIndex);
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
						if (isAVnuAudioMediaListener)
						{
							if (avnuAudioInputStreamsForDomain >= 2)
							{
								// [Milan 1.3 Clause 7.2.2] For each supported clock domain, an AAF Media Listener with two or more AAF Media Inputs shall implement a CRF Media Clock Input
								if (avnuCrfInputStreamsForDomain == 0u)
								{
									// Remove "Milan compatibility"
									removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 7.2.2", "For each supported clock domain, an AAF Media Listener with two or more AAF Media Inputs shall implement a CRF Media Clock Input");
								}
								// [Milan 1.3 Clause 7.2.3] For each supported clock domain, an AAF Media Listener with two or more AAF Media Inputs shall implement a CRF Media Clock Output
								if (avnuCrfOutputStreamsForDomain == 0u)
								{
									// Remove "Milan compatibility"
									removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 7.2.3", "For each supported clock domain, an AAF Media Listener with two or more AAF Media Inputs shall implement a CRF Media Clock Output");
								}
							}
						}
						if (isAVnuAudioMediaTalker)
						{
							// [Milan 1.3 Clause 7.2.2] For each supported clock domain, an AAF Media Talker shall implement a CRF Media Clock Input
							if (avnuCrfInputStreamsForDomain == 0u)
							{
								// Remove "Milan compatibility"
								removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 7.2.2", "For each supported clock domain, an AAF Media Talker shall implement a CRF Media Clock Input");
							}
							// [Milan 1.3 Clause 7.2.3] For each supported clock domain, an AAF Media Talker capable of synchronizing to an external clock source (not an AVB stream) shall implement a CRF Media Clock Output
							// TODO
						}
					}
				}
				else
				{
					// Flag the entity as "Not Milan compliant"
					removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.4.4.1", "MilanInfo is not available for this entity although it is advertised as Milan compatible");
				}
			}
		}
		catch (ControlledEntity::Exception const&)
		{
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021", "Invalid current CONFIGURATION descriptor");
		}
		catch (...)
		{
			AVDECC_ASSERT(false, "Unhandled exception");
		}
	}
}

/** Final validations to be run on the entity, now that it's fully enumerated (both attached and detached entities) */
void ControllerImpl::validateEntity(ControlledEntityImpl& controlledEntity) noexcept
{
	// Validate entity control descriptors
	// This is something to be done by the controller, only it should have the knowledge of what is correct or not
	validateControlDescriptors(controlledEntity);

	// Validate entity is correctly declared (or not) as a Milan Redundant device
	validateRedundancy(controlledEntity);

	// Validate entity model, if existing
	validateEntityModel(controlledEntity);

	// Check for AvbInterfaceCounters - Link Status
	auto const& e = controlledEntity.getEntity();
	auto const isAemSupported = e.getEntityCapabilities().test(entity::EntityCapability::AemSupported);

	// If AEM is supported
	if (isAemSupported && controlledEntity.hasAnyConfiguration())
	{
		try
		{
			for (auto const& [avbInterfaceIndex, avbInterfaceNode] : controlledEntity.getCurrentConfigurationNode().avbInterfaces)
			{
				if (avbInterfaceNode.dynamicModel.counters)
				{
					checkAvbInterfaceLinkStatus(nullptr, controlledEntity, avbInterfaceIndex, *avbInterfaceNode.dynamicModel.counters);
				}
			}
		}
		catch (...)
		{
			AVDECC_ASSERT(false, "Should not throw");
		}
	}

	// Check for Diagnostics - Redundancy Warning
	checkRedundancyWarningDiagnostics(nullptr, controlledEntity);
}

std::tuple<bool, std::optional<entity::model::AudioMapping>, std::optional<entity::model::AudioMapping>> ControllerImpl::getMappingForInputClusterIdentification(model::StreamPortInputNode const& streamPortNode, model::ClusterIdentification const& clusterIdentification, std::function<bool(entity::model::StreamIndex)> const& isRedundantPrimaryStreamInput, std::function<bool(entity::model::StreamIndex)> const& isRedundantSecondaryStreamInput) noexcept
{
	auto const baseClusterIndex = streamPortNode.staticModel.baseCluster;
	auto const numberOfClusters = streamPortNode.staticModel.numberOfClusters;
	auto const globalClusterIndex = clusterIdentification.clusterIndex;
	auto const clusterChannel = clusterIdentification.clusterChannel;

	// Ensure the clusterIndex is in the valid range for this StreamPort
	if (!AVDECC_ASSERT_WITH_RET(globalClusterIndex >= baseClusterIndex && globalClusterIndex < static_cast<entity::model::ClusterIndex>(baseClusterIndex + numberOfClusters), "ClusterIndex is out of range for this StreamPort"))
	{
		return {};
	}

	// Calculate the clusterOffset (relative to baseCluster)
	auto const clusterOffset = static_cast<entity::model::ClusterIndex>(globalClusterIndex - baseClusterIndex);

	// Function to search mappings for redundant pairs
	auto const searchMappings = [&](entity::model::AudioMappings const& mappings)
	{
		auto isRedundantMapping = false;
		auto primaryMapping = std::optional<entity::model::AudioMapping>{};
		auto secondaryMapping = std::optional<entity::model::AudioMapping>{};

		// Find all mappings with matching cluster offset and channel
		for (auto const& mapping : mappings)
		{
			if (mapping.clusterOffset == clusterOffset && mapping.clusterChannel == clusterChannel)
			{
				auto const isPrimary = isRedundantPrimaryStreamInput(mapping.streamIndex);
				auto const isSecondary = isRedundantSecondaryStreamInput(mapping.streamIndex);

				if (isPrimary)
				{
					primaryMapping = mapping;
					isRedundantMapping = true;
				}
				else if (isSecondary)
				{
					secondaryMapping = mapping;
					isRedundantMapping = true;
				}
				else
				{
					// Non-redundant mapping: return in first element (primary), second element remains std::nullopt
					primaryMapping = mapping;
					// No need to continue searching
					break;
				}
			}
		}

		return std::make_tuple(isRedundantMapping, primaryMapping, secondaryMapping);
	};

	// Search in static mappings first (AudioMaps) - device can only have static OR dynamic active at a time
	for (auto const& [audioMapIndex, audioMapNode] : streamPortNode.audioMaps)
	{
		auto const result = searchMappings(audioMapNode.staticModel.mappings);
		if (std::get<1>(result).has_value() || std::get<2>(result).has_value())
		{
			return result;
		}
	}

	// Search in dynamic mappings - return result directly as it's already {nullopt, nullopt} if nothing found
	return searchMappings(streamPortNode.dynamicModel.dynamicAudioMap);
}

std::optional<entity::model::AudioMapping> ControllerImpl::getMappingForStreamChannelIdentification(model::StreamPortNode const& streamPortNode, entity::model::StreamIndex const streamIndex, std::uint16_t const streamChannel) noexcept
{
	// Search in static mappings (AudioMaps)
	for (auto const& [audioMapIndex, audioMapNode] : streamPortNode.audioMaps)
	{
		for (auto const& mapping : audioMapNode.staticModel.mappings)
		{
			if (mapping.streamIndex == streamIndex && mapping.streamChannel == streamChannel)
			{
				return mapping;
			}
		}
	}

	// Search in dynamic mappings
	for (auto const& mapping : streamPortNode.dynamicModel.dynamicAudioMap)
	{
		if (mapping.streamIndex == streamIndex && mapping.streamChannel == streamChannel)
		{
			return mapping;
		}
	}

	// No mapping found
	return std::nullopt;
}

// _lock should be taken when calling this method
void ControllerImpl::computeAndUpdateMediaClockChain(ControlledEntityImpl& controlledEntity, model::ClockDomainNode& clockDomainNode, UniqueIdentifier const continueFromEntityID, entity::model::ClockDomainIndex const continueFromEntityDomainIndex, std::optional<entity::model::StreamIndex> const continueFromStreamOutputIndex, UniqueIdentifier const beingAdvertisedEntity) const noexcept
{
	auto encounteredEntities = std::unordered_set<UniqueIdentifier, UniqueIdentifier::hash>{}; // Used to detect recursivity

	// If we don't start from the begining, add previously encountered entities in the set so we can properly detect recursivity
	for (auto const& node : clockDomainNode.mediaClockChain)
	{
		encounteredEntities.insert(node.entityID);
	}

	// Get the starting point ClockDomainIndex
	auto const continueFromEntityClockDomainIndex = continueFromEntityDomainIndex;

	auto currentEntityID = continueFromEntityID;
	auto currentStreamOutput = continueFromStreamOutputIndex; // StreamOutput for chain continuation
	auto keepSearching = true;
	while (keepSearching)
	{
		auto node = model::MediaClockChainNode{};
		node.entityID = currentEntityID;
		node.streamOutputIndex = currentStreamOutput;

		// Firstly check for recursivity
		if (encounteredEntities.count(currentEntityID) > 0u)
		{
			node.status = model::MediaClockChainNode::Status::Recursive;
			keepSearching = false;
		}
		else
		{
			// Add entity to the list of processed entity
			encounteredEntities.insert(currentEntityID);

			// Get matching ControlledEntityImpl, if online (or being advertised when this method is called)
			if (auto const entityIt = _controlledEntities.find(currentEntityID); entityIt != _controlledEntities.end() && (beingAdvertisedEntity == currentEntityID || entityIt->second->wasAdvertised()))
			{
				auto const& currentEntity = *entityIt->second;

				try
				{
					auto const currentConfigIndex = currentEntity.getCurrentConfigurationIndex();

					// Get current clock domain for this node (default with the provided one for the first run of the loop)
					auto currentClockDomainIndex = continueFromEntityClockDomainIndex;

					// And override it if this is a continuation of the chain
					AVDECC_ASSERT(currentClockDomainIndex != entity::model::getInvalidDescriptorIndex() || currentStreamOutput, "currentClockDomainIndex and currentStreamOutput cannot both be invalid");
#if 0
					if (currentStreamOutput)
#else // Better optimized version preventing retrieval of the StreamOutputNode during the first iteration of the loop if both continueFromEntityDomainIndex and continueFromStreamOutputIndex are defined
					if (currentClockDomainIndex == entity::model::getInvalidDescriptorIndex() || (currentStreamOutput && currentClockDomainIndex != entity::model::getInvalidDescriptorIndex() && currentEntityID != continueFromEntityID))
#endif
					{
						// Find stream output
						auto const& soNode = currentEntity.getStreamOutputNode(currentConfigIndex, *currentStreamOutput);
						currentClockDomainIndex = soNode.staticModel.clockDomainIndex;
					}
					node.clockDomainIndex = currentClockDomainIndex;

					// Get the clock domain node
					auto const& cdNode = currentEntity.getClockDomainNode(currentConfigIndex, currentClockDomainIndex);
					auto const currentCSIndex = cdNode.dynamicModel.clockSourceIndex;
					node.clockSourceIndex = currentCSIndex;

					// Get the active clock source node for this clock domain
					auto const& csNode = currentEntity.getClockSourceNode(currentConfigIndex, currentCSIndex);

					// Follow the clock source used by this domain
					switch (csNode.staticModel.clockSourceType)
					{
						case entity::model::ClockSourceType::Internal:
							node.type = model::MediaClockChainNode::Type::Internal;
							keepSearching = false;
							break;
						case entity::model::ClockSourceType::External:
							node.type = model::MediaClockChainNode::Type::External;
							keepSearching = false;
							break;
						case entity::model::ClockSourceType::InputStream:
						{
							node.type = model::MediaClockChainNode::Type::StreamInput;

							// Validate location type
							if (csNode.staticModel.clockSourceLocationType != entity::model::DescriptorType::StreamInput)
							{
								throw ControlledEntity::Exception(ControlledEntity::Exception::Type::NotSupported, "Invalid ClockSource location");
							}

							// Find stream input
							auto const& siNode = currentEntity.getStreamInputNode(currentConfigIndex, csNode.staticModel.clockSourceLocationIndex);
							node.streamInputIndex = csNode.staticModel.clockSourceLocationIndex;

							// Get connection info
							auto const& connInfo = siNode.dynamicModel.connectionInfo;

							// Stop searching if stream is not connected
							if (connInfo.state != entity::model::StreamInputConnectionInfo::State::Connected)
							{
								node.status = model::MediaClockChainNode::Status::StreamNotConnected;
								keepSearching = false;
							}
							else
							{
								// Go to next entity in the chain
								currentEntityID = connInfo.talkerStream.entityID;

								// And update the stream output index to continue from this one
								currentStreamOutput = connInfo.talkerStream.streamIndex;
							}
							break;
						}
						default:
							node.status = model::MediaClockChainNode::Status::UnsupportedClockSource;
							keepSearching = false;
							break;
					}
				}
				catch (ControlledEntity::Exception const&)
				{
					node.status = model::MediaClockChainNode::Status::AemError;
					keepSearching = false;
				}
			}
			else
			{
				// Entity is offline
				node.status = model::MediaClockChainNode::Status::EntityOffline;
				keepSearching = false;
			}
		}

		// Add to the chain
		clockDomainNode.mediaClockChain.emplace_back(std::move(node));
	}

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onMediaClockChainChanged, this, &controlledEntity, clockDomainNode.descriptorIndex, clockDomainNode.mediaClockChain);
	}
}

#ifdef ENABLE_AVDECC_FEATURE_CBR
// _lock should be taken when calling this method
bool ControllerImpl::computeAndUpdateChannelConnectionFromStreamIdentification(entity::model::StreamIdentification const& streamIdentification, model::ChannelConnectionIdentification& channelConnectionIdentification) const noexcept
{
	auto changed = false;

	if (channelConnectionIdentification.streamIdentification != streamIdentification)
	{
		// Update the Stream Identification
		channelConnectionIdentification.streamIdentification = streamIdentification;
		changed = true;

		// If not connected, clear Cluster Identification
		if (!channelConnectionIdentification.streamIdentification.entityID)
		{
			channelConnectionIdentification.talkerClusterIdentification = {};
		}
		else
		{
			auto talkerEntity = getControlledEntityImplGuard(channelConnectionIdentification.streamIdentification.entityID);
			if (talkerEntity)
			{
				auto* const talkerConfigurationNode = talkerEntity->getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
				if (talkerConfigurationNode && channelConnectionIdentification.streamChannelIdentification)
				{
					// Process all Audio Units
					for (auto const& [audioUnitIndex, audioUnitNode] : talkerConfigurationNode->audioUnits)
					{
						// Process all Stream Port Outputs
						for (auto const& [streamPortIndex, streamPortNode] : audioUnitNode.streamPortOutputs)
						{
							auto const talkerMapping = getMappingForStreamChannelIdentification(streamPortNode, channelConnectionIdentification.streamIdentification.streamIndex, channelConnectionIdentification.streamChannelIdentification.streamChannel);
							if (talkerMapping)
							{
								// We have a mapping, set the Cluster Identification
								channelConnectionIdentification.talkerClusterIdentification = model::ClusterIdentification{ static_cast<entity::model::ClusterIndex>(streamPortNode.staticModel.baseCluster + talkerMapping->clusterOffset), talkerMapping->clusterChannel };
								break;
							}
						}
					}
				}
			}
		}
	}

	return changed;
}

// _lock should be taken when calling this method
void ControllerImpl::computeAndUpdateChannelConnectionFromListenerMapping(ControlledEntityImpl& controlledEntity, model::ConfigurationNode const& configurationNode, model::ClusterIdentification const& clusterIdentification, std::tuple<bool, std::optional<entity::model::AudioMapping>, std::optional<entity::model::AudioMapping>> const& audioMappingsInfo, model::ChannelIdentification& channelIdentification) const noexcept
{
	auto const updateChannelConnectionIdentification = [this, &configurationNode](auto& channelConnectionIdentification, auto const& audioMapping)
	{
		auto streamChannelIdentification = model::StreamChannelIdentification{};
		if (audioMapping)
		{
			streamChannelIdentification = model::StreamChannelIdentification{ (*audioMapping).streamIndex, (*audioMapping).streamChannel };
		}

		if (channelConnectionIdentification.streamChannelIdentification != streamChannelIdentification)
		{
			// No mapping anymore, clear all fields
			if (!streamChannelIdentification.isValid())
			{
				channelConnectionIdentification = {};
			}
			else
			{
				auto const& mapping = *audioMapping;

				// We have a mapping, set the StreamChannel Identification
				channelConnectionIdentification.streamChannelIdentification = streamChannelIdentification;

				// Update the ChannelConnection based on the current stream mapping
				if (auto const streamInputNodeIt = configurationNode.streamInputs.find(mapping.streamIndex); streamInputNodeIt != configurationNode.streamInputs.end())
				{
					auto const& streamInputNode = streamInputNodeIt->second;
					auto const& connectionInfo = streamInputNode.dynamicModel.connectionInfo;
					computeAndUpdateChannelConnectionFromStreamIdentification(connectionInfo.talkerStream, channelConnectionIdentification);
				}
			}
			return true;
		}
		return false;
	};

	auto changed = false;

	// Process single (or primary) mapping
	auto const& primaryAudioMapping = std::get<1>(audioMappingsInfo);

	changed |= updateChannelConnectionIdentification(channelIdentification.channelConnectionIdentification, primaryAudioMapping);

#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	auto const isRedundantMapping = std::get<0>(audioMappingsInfo);
	auto const& secondaryAudioMapping = std::get<2>(audioMappingsInfo);
	// Check for channelIdentification.secondaryChannelConnectionIdentification creation or deletion
	if (isRedundantMapping && !channelIdentification.secondaryChannelConnectionIdentification)
	{
		channelIdentification.secondaryChannelConnectionIdentification = model::ChannelConnectionIdentification{};
	}
	else if (!isRedundantMapping && channelIdentification.secondaryChannelConnectionIdentification)
	{
		channelIdentification.secondaryChannelConnectionIdentification = std::nullopt;
		changed = true;
	}

	// Process secondary mapping for redundancy, if redundant device
	if (channelIdentification.secondaryChannelConnectionIdentification)
	{
		changed |= updateChannelConnectionIdentification(*channelIdentification.secondaryChannelConnectionIdentification, secondaryAudioMapping);
	}
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	// Notify if changed
	if (changed && controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onChannelInputConnectionChanged, this, &controlledEntity, clusterIdentification, channelIdentification);
	}
}

// _lock should be taken when calling this method
void ControllerImpl::computeAndUpdateChannelConnectionsFromTalkerMappings(ControlledEntityImpl& controlledEntity, UniqueIdentifier const& talkerEntityID, entity::model::ClusterIndex const baseClusterIndex, entity::model::AudioMappings const& mappings, model::ChannelConnections& channelConnections, bool const removeMappings) const noexcept
{
	auto const updateChannelConnectionIdentification = [&mappings, baseClusterIndex, removeMappings](auto& channelConnectionIdentification)
	{
		// Check if any of the mappings relate to the connection (beware we have to convert controlledEntity's clusterIndex to global index using baseClusterIndex)
		for (auto const& mapping : mappings)
		{
			// This entity has a connection to the mapping's stream index/channel
			if (channelConnectionIdentification.streamIdentification.streamIndex == mapping.streamIndex && channelConnectionIdentification.streamChannelIdentification.streamChannel == mapping.streamChannel)
			{
				auto const globalClusterIndex = static_cast<entity::model::ClusterIndex>(baseClusterIndex + mapping.clusterOffset);
				auto const talkerClusterId = removeMappings ? model::ClusterIdentification{} : model::ClusterIdentification{ globalClusterIndex, mapping.clusterChannel };

				// Changed
				if (talkerClusterId != channelConnectionIdentification.talkerClusterIdentification)
				{
					// Update the ChannelConnection
					channelConnectionIdentification.talkerClusterIdentification = talkerClusterId;

					return true;
				}
				break;
			}
		}
		return false;
	};

	for (auto& [clusterIdentification, channelIdentification] : channelConnections)
	{
		auto changed = false;

		// This entity has a connection to the controlledEntity
		if (channelIdentification.channelConnectionIdentification.streamIdentification.entityID == talkerEntityID)
		{
			changed |= updateChannelConnectionIdentification(channelIdentification.channelConnectionIdentification);
		}
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
		// This entity has a secondary connection to the controlledEntity (for redundancy)
		if (channelIdentification.secondaryChannelConnectionIdentification && channelIdentification.secondaryChannelConnectionIdentification->streamIdentification.entityID == talkerEntityID)
		{
			changed |= updateChannelConnectionIdentification(*channelIdentification.secondaryChannelConnectionIdentification);
		}
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
		if (changed)
		{
			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onChannelInputConnectionChanged, this, &controlledEntity, clusterIdentification, channelIdentification);
			}
		}
	}
}

// _lock should be taken when calling this method
void ControllerImpl::computeAndUpdateChannelConnectionsFromConfigurationNode(ControlledEntityImpl& controlledEntity, UniqueIdentifier const& talkerEntityID, model::ConfigurationNode const& talkerConfigurationNode, model::ChannelConnections& channelConnections) const noexcept
{
	// Process all talker Audio Units
	for (auto const& [audioUnitIndex, audioUnitNode] : talkerConfigurationNode.audioUnits)
	{
		// Process all talker Stream Port Outputs
		for (auto const& [streamPortIndex, streamPortNode] : audioUnitNode.streamPortOutputs)
		{
			// Process static mappings (AudioMaps)
			for (auto const& [audioMapIndex, audioMapNode] : streamPortNode.audioMaps)
			{
				computeAndUpdateChannelConnectionsFromTalkerMappings(controlledEntity, talkerEntityID, streamPortNode.staticModel.baseCluster, audioMapNode.staticModel.mappings, channelConnections, false);
			}

			// Process dynamic mappings
			computeAndUpdateChannelConnectionsFromTalkerMappings(controlledEntity, talkerEntityID, streamPortNode.staticModel.baseCluster, streamPortNode.dynamicModel.dynamicAudioMap, channelConnections, false);
		}
	}
}
#endif // ENABLE_AVDECC_FEATURE_CBR

/** Actions to be done on the entity, just before advertising, which require looking at other already advertised entities (only for attached entities) */
void ControllerImpl::onPreAdvertiseEntity(ControlledEntityImpl& controlledEntity) noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const& e = controlledEntity.getEntity();
	auto const entityID = e.getEntityID();
	auto const isAemSupported = e.getEntityCapabilities().test(entity::EntityCapability::AemSupported);
	auto const hasAnyConfiguration = controlledEntity.hasAnyConfiguration();
	auto const isVirtualEntity = controlledEntity.isVirtual();
	auto const hasTalkerCapabilities = e.getTalkerCapabilities().test(entity::TalkerCapability::Implemented);
	auto const hasListenerCapabilities = e.getListenerCapabilities().test(entity::ListenerCapability::Implemented);

	// Lock to protect _controlledEntities
	auto const lg = std::lock_guard{ _lock };

	// Now that this entity is ready to be advertised, update states that are linked to the connection with another entity, in case it was not advertised during processing
	if (isAemSupported && hasAnyConfiguration)
	{
		auto* const controlledEntityConfigurationNode = controlledEntity.getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

		// States related to Listener capabilities
		if (hasListenerCapabilities && controlledEntityConfigurationNode != nullptr)
		{
			try
			{
				// Process all our input streams that are connected to another talker
				for (auto const& [streamIndex, streamInputNode] : controlledEntityConfigurationNode->streamInputs)
				{
					auto isOverLatency = false;

					// If the Stream is Connected, search for the Talker we are connected to
					if (streamInputNode.dynamicModel.connectionInfo.state == entity::model::StreamInputConnectionInfo::State::Connected)
					{
						auto const talkerEntityID = streamInputNode.dynamicModel.connectionInfo.talkerStream.entityID;

						if (auto const entityIt = _controlledEntities.find(talkerEntityID); entityIt != _controlledEntities.end())
						{
							auto& talkerEntity = *(entityIt->second);

							// Don't process self, not yet advertised entities, nor different virtual/physical kind
							if (talkerEntityID == entityID || !talkerEntity.wasAdvertised() || isVirtualEntity != talkerEntity.isVirtual())
							{
								continue;
							}

							auto const talkerStreamIndex = streamInputNode.dynamicModel.connectionInfo.talkerStream.streamIndex;

							// We want to inform the talker we are connected to (already advertised only, the other ones will update once ready to advertise themselves)
							{
								talkerEntity.addStreamOutputConnection(talkerStreamIndex, { entityID, streamIndex }, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
								notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputConnectionsChanged, this, &talkerEntity, talkerStreamIndex, talkerEntity.getStreamOutputConnections(talkerStreamIndex));
							}

							// Check for Latency Error (if the TalkerEntity was not advertised when this listener was enumerating, it couldn't check Talker's PresentationTime, so do it now)
							try
							{
								auto const& talkerStreamOutputNode = talkerEntity.getStreamOutputNode(talkerEntity.getCurrentConfigurationIndex(), talkerStreamIndex);
								// If we have StreamDynamicInfo data
								if (streamInputNode.dynamicModel.streamDynamicInfo)
								{
									isOverLatency = computeIsOverLatency(talkerStreamOutputNode.dynamicModel.presentationTimeOffset, (*streamInputNode.dynamicModel.streamDynamicInfo).msrpAccumulatedLatency);
								}
							}
							catch (ControlledEntity::Exception const&)
							{
								// Ignore Exception
							}
						}
					}

					// We want to always set the Stream Input Latency flag
					updateStreamInputLatency(controlledEntity, streamIndex, isOverLatency);
				}
			}
			catch (...)
			{
				AVDECC_ASSERT(false, "Unexpected exception");
			}
		}

		// Compute Media Clock Chain for this newly advertised entity
		if (controlledEntityConfigurationNode != nullptr)
		{
			for (auto& [clockDomainIndex, clockDomainNode] : controlledEntityConfigurationNode->clockDomains)
			{
				computeAndUpdateMediaClockChain(controlledEntity, clockDomainNode, entityID, clockDomainIndex, std::nullopt, entityID);
			}
		}

#ifdef ENABLE_AVDECC_FEATURE_CBR
		// Compute Channel Connections for this newly advertised entity
		if (controlledEntityConfigurationNode != nullptr)
		{
			// Process all Audio Units
			for (auto const& [audioUnitIndex, audioUnitNode] : controlledEntityConfigurationNode->audioUnits)
			{
				// Process all Stream Port Inputs
				for (auto const& [streamPortIndex, streamPortNode] : audioUnitNode.streamPortInputs)
				{
					// Process all Audio Clusters
					for (auto const& [clusterIndex, clusterNode] : streamPortNode.audioClusters)
					{
						// Process all Channels
						for (auto clusterChannel = std::uint16_t{ 0u }; clusterChannel < clusterNode.staticModel.channelCount; ++clusterChannel)
						{
							auto const clusterIdentification = model::ClusterIdentification{ clusterIndex, clusterChannel };
							auto const mappingsInfo = getMappingForInputClusterIdentification(streamPortNode, clusterIdentification, std::bind(&ControlledEntityImpl::isRedundantPrimaryStreamInput, &controlledEntity, std::placeholders::_1), std::bind(&ControlledEntityImpl::isRedundantSecondaryStreamInput, &controlledEntity, std::placeholders::_1));
							// Insert default ChannelConnection (should not exist)
							auto& channelConnection = controlledEntityConfigurationNode->channelConnections[clusterIdentification];
							computeAndUpdateChannelConnectionFromListenerMapping(controlledEntity, *controlledEntityConfigurationNode, clusterIdentification, mappingsInfo, channelConnection);
						}
					}
				}
			}
		}
#endif // ENABLE_AVDECC_FEATURE_CBR

		// States related to Talker capabilities, Media Clock Chain and Channel Connections
		// For these, we need to process all other entities that may be connected to us
		for (auto& [eid, entity] : _controlledEntities)
		{
			// Don't process self, not yet advertised entities, non AEM, without configuration, nor different virtual/physical kind
			if (eid == entityID || !entity->wasAdvertised() || !entity->getEntity().getEntityCapabilities().test(entity::EntityCapability::AemSupported) || !entity->hasAnyConfiguration() || isVirtualEntity != entity->isVirtual())
			{
				continue;
			}

			auto* const configNode = entity->getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			// States related to Talker capabilities
			if (hasTalkerCapabilities && controlledEntityConfigurationNode != nullptr && configNode != nullptr)
			{
				// Check each of this Listener's Input Streams
				for (auto const& [streamIndex, streamInputNode] : configNode->streamInputs)
				{
					// If the Stream is Connected
					if (streamInputNode.dynamicModel.connectionInfo.state == entity::model::StreamInputConnectionInfo::State::Connected)
					{
						// Check against all the Talker's Output Streams
						for (auto const& [streamOutputIndex, streamOutputNode] : controlledEntityConfigurationNode->streamOutputs)
						{
							auto const talkerIdentification = entity::model::StreamIdentification{ entityID, streamOutputIndex };

							// Connected to our talker
							if (streamInputNode.dynamicModel.connectionInfo.talkerStream == talkerIdentification)
							{
								// We want to build an accurate list of connections, based on the known listeners (already advertised only, the other ones will update once ready to advertise themselves)
								{
									// Add this listener to our list of connected entities
									controlledEntity.addStreamOutputConnection(streamOutputIndex, { eid, streamIndex }, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
									// Do not trigger onStreamOutputConnectionsChanged notification, we are just about to advertise the entity
								}

								// Check for Latency Error (if the Listener was advertised before this Talker, it couldn't check Talker's PresentationTime, so do it now)
								// If we have StreamDynamicInfo data
								if (streamInputNode.dynamicModel.streamDynamicInfo)
								{
									auto const isOverLatency = computeIsOverLatency(streamOutputNode.dynamicModel.presentationTimeOffset, (*streamInputNode.dynamicModel.streamDynamicInfo).msrpAccumulatedLatency);
									updateStreamInputLatency(*entity, streamIndex, isOverLatency);
								}
							}
						}
					}
				}
			}

			// Media Clock Chain - Update entity for which the chain ends on this newly added entity
			if (configNode != nullptr)
			{
				for (auto& clockDomainKV : configNode->clockDomains)
				{
					auto& clockDomainNode = clockDomainKV.second;

					// Get the domain index matching the StreamOutput of this entity
					if (AVDECC_ASSERT_WITH_RET(!clockDomainNode.mediaClockChain.empty(), "At least one node should be in the chain"))
					{
						// Check if the chain is incomplete due to this entity being Offline
						auto const& lastNode = clockDomainNode.mediaClockChain.back();
						if (lastNode.entityID == entityID && AVDECC_ASSERT_WITH_RET(lastNode.status == model::MediaClockChainNode::Status::EntityOffline, "Newly discovered entity should be offline"))
						{
							// Save the domain/stream indexes, we'll continue from it
							auto const continueDomainIndex = lastNode.clockDomainIndex;
							auto const continueStreamOutputIndex = lastNode.streamOutputIndex;

							// Remove that entity from the chain, it will be recomputed
							clockDomainNode.mediaClockChain.pop_back();

							// Get the domain index matching the StreamOutput of this entity
							if (AVDECC_ASSERT_WITH_RET(!clockDomainNode.mediaClockChain.empty(), "At least one node should still be in the chain"))
							{
								// Update the chain starting from this entity
								computeAndUpdateMediaClockChain(*entity, clockDomainNode, entityID, continueDomainIndex, continueStreamOutputIndex, entityID);
							}
						}
					}
				}
			}

#ifdef ENABLE_AVDECC_FEATURE_CBR
			// Channel Connections - Update entity that have connections to our Stream Outputs
			if (configNode != nullptr)
			{
				computeAndUpdateChannelConnectionsFromConfigurationNode(*entity, entityID, *controlledEntityConfigurationNode, configNode->channelConnections);
			}
#endif // ENABLE_AVDECC_FEATURE_CBR
		}
	}
}

void ControllerImpl::onPostAdvertiseEntity(ControlledEntityImpl& controlledEntity) noexcept
{
	auto const isVirtualEntity = controlledEntity.isVirtual();

	// If entity is currently identifying itself, notify
	if (controlledEntity.isIdentifying())
	{
		if (!isVirtualEntity)
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onIdentificationStarted, this, &controlledEntity);
		}
	}
}

void ControllerImpl::onPreUnadvertiseEntity(ControlledEntityImpl& controlledEntity) noexcept
{
	auto const& e = controlledEntity.getEntity();
	auto const entityID = e.getEntityID();
	auto const isAemSupported = e.getEntityCapabilities().test(entity::EntityCapability::AemSupported);
	auto const hasAnyConfiguration = controlledEntity.hasAnyConfiguration();
	auto const isVirtualEntity = controlledEntity.isVirtual();

	// For a Listener, we want to inform all the talkers we are connected to, that we left
	if (e.getListenerCapabilities().test(entity::ListenerCapability::Implemented) && isAemSupported && hasAnyConfiguration)
	{
		AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

		try
		{
			auto const& configurationNode = controlledEntity.getCurrentConfigurationNode();

			for (auto const& [streamIndex, streamInputNode] : configurationNode.streamInputs)
			{
				// If the Stream is Connected, search for the Talker we are connected to
				if (streamInputNode.dynamicModel.connectionInfo.state == entity::model::StreamInputConnectionInfo::State::Connected)
				{
					// Take a "scoped locked" shared copy of the ControlledEntity
					auto talkerEntity = getControlledEntityImplGuard(streamInputNode.dynamicModel.connectionInfo.talkerStream.entityID, true);

					// Only process same virtual/physical kind
					if (talkerEntity && isVirtualEntity == talkerEntity->isVirtual())
					{
						auto& talker = *talkerEntity;
						auto const talkerStreamIndex = streamInputNode.dynamicModel.connectionInfo.talkerStream.streamIndex;
						talker.delStreamOutputConnection(talkerStreamIndex, { entityID, streamIndex }, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
						notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputConnectionsChanged, this, &talker, talkerStreamIndex, talker.getStreamOutputConnections(talkerStreamIndex));
					}
				}
			}
		}
		catch (...)
		{
			AVDECC_ASSERT(false, "Unexpected exception");
		}
	}

	// Lock to protect _controlledEntities
	auto const lg = std::lock_guard{ _lock };

	// Update all other entities that may be affected by this departing entity
	// - Media Clock Chain: entities that have a chain node on the departing entity
	// - Channel Connections: listeners that are connected to this departing talker
	for (auto& [eid, entity] : _controlledEntities)
	{
		// Don't process self (departing entity), not advertised entities, non AEM, without configuration, nor different virtual/physical kind
		if (eid == entityID || !entity->wasAdvertised() || !entity->getEntity().getEntityCapabilities().test(entity::EntityCapability::AemSupported) || !entity->hasAnyConfiguration() || isVirtualEntity != entity->isVirtual())
		{
			continue;
		}

		auto* const configNode = entity->getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
		if (configNode == nullptr)
		{
			continue;
		}

		// Media Clock Chain - Update entities for which the chain has a node on the departing entity
		for (auto& clockDomainKV : configNode->clockDomains)
		{
			auto& clockDomainNode = clockDomainKV.second;
			// Check if the chain has a node on that departing entity
			for (auto nodeIt = clockDomainNode.mediaClockChain.begin(); nodeIt != clockDomainNode.mediaClockChain.end(); ++nodeIt)
			{
				if (nodeIt->entityID == entityID)
				{
					// Save the domain/stream indexes, we'll continue from it
					auto const continueDomainIndex = nodeIt->clockDomainIndex;
					auto const continueStreamOutputIndex = nodeIt->streamOutputIndex;

					// Remove this node and all following nodes
					clockDomainNode.mediaClockChain.erase(nodeIt, clockDomainNode.mediaClockChain.end());

					// Update the chain starting from this entity
					computeAndUpdateMediaClockChain(*entity, clockDomainNode, entityID, continueDomainIndex, continueStreamOutputIndex, {});
					break;
				}
			}
		}

#ifdef ENABLE_AVDECC_FEATURE_CBR
		// Channel Connections - Update channel connections for listeners that were connected to this departing talker
		if (entity->getEntity().getListenerCapabilities().test(entity::ListenerCapability::Implemented))
		{
			// Check all channel connections in this listener
			for (auto& [clusterIdentification, channelIdentification] : configNode->channelConnections)
			{
				auto changed = false;
				// If this channel is connected to the departing talker
				if (channelIdentification.channelConnectionIdentification.streamIdentification.entityID == entityID)
				{
					// Clear the talker mapping information (since we can't query the offline talker anymore)
					channelIdentification.channelConnectionIdentification.talkerClusterIdentification = model::ClusterIdentification{};
					changed = true;
				}
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				// If this channel has a secondary connection to the departing talker
				if (channelIdentification.secondaryChannelConnectionIdentification && channelIdentification.secondaryChannelConnectionIdentification->streamIdentification.entityID == entityID)
				{
					// Clear the talker mapping information (since we can't query the offline talker anymore)
					channelIdentification.secondaryChannelConnectionIdentification->talkerClusterIdentification = model::ClusterIdentification{};
					changed = true;
				}
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
				if (changed)
				{
					// Notify observers of the channel connection change
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onChannelInputConnectionChanged, this, entity.get(), clusterIdentification, channelIdentification);
				}
			}
		}
#endif // ENABLE_AVDECC_FEATURE_CBR
	}
}

/* This method handles Milan Requirements when a command is not supported by the entity, removing associated compatibility flag. */
void ControllerImpl::checkMilanRequirements(ControlledEntityImpl* const entity, MilanRequirements const& milanRequirements, std::string const& specClause, std::string const& message) const noexcept
{
	auto const milanCompatibilityVersion = entity->getMilanCompatibilityVersion();
	auto downgradeToVersion = entity::model::MilanVersion{};
#ifdef DEBUG
	auto lastMinVersion = entity::model::MilanVersion{};
	auto lastMaxVersion = entity::model::MilanVersion{};
#endif // DEBUG

	// Process all Milan requirements:
	// - check if the current compatibility version is comprised in the required versions
	// - if not, do nothing (ie. requirement is optional)
	// - if yes, update the compatibility version to the max version of the previous requirement of the list
	for (auto const& requiredVersions : milanRequirements)
	{
#ifdef DEBUG
		// Sanity check - Make sure the requirements are increasing
		if (!AVDECC_ASSERT_WITH_RET(!requiredVersions.requiredUntil || requiredVersions.requiredSince <= *requiredVersions.requiredUntil, "Milan requirements are not in ascending order"))
		{
			LOG_CONTROLLER_DEBUG(entity->getEntity().getEntityID(), "Milan requirements are not in ascending order");
		}

		// Sanity check - Make sure the requirements are in ascending order and not overlapping
		if (!AVDECC_ASSERT_WITH_RET(lastMinVersion < requiredVersions.requiredSince, "Milan requirements are not in ascending order"))
		{
			LOG_CONTROLLER_DEBUG(entity->getEntity().getEntityID(), "Milan requirements are not in ascending order");
		}
		if (!AVDECC_ASSERT_WITH_RET(lastMaxVersion < requiredVersions.requiredSince, "Milan requirements are overlapping"))
		{
			LOG_CONTROLLER_DEBUG(entity->getEntity().getEntityID(), "Milan requirements are overlapping");
		}

		// Update the last min/max version
		lastMinVersion = requiredVersions.requiredSince;
		lastMaxVersion = requiredVersions.requiredUntil ? *requiredVersions.requiredUntil : entity::model::MilanVersion{};
#endif // DEBUG

		// Check the current compatibility version is at least the required version
		if (milanCompatibilityVersion >= requiredVersions.requiredSince)
		{
			// Check if the current compatibility version is lower than the max version (if defined, otherwise max is infinite)
			if (!requiredVersions.requiredUntil || milanCompatibilityVersion <= *requiredVersions.requiredUntil)
			{
				// Command should be supported but it's not, downgrade the compatibility version
				decreaseMilanCompatibilityVersion(this, *entity, requiredVersions.downgradeTo ? *requiredVersions.downgradeTo : downgradeToVersion, specClause, message);
				return;
			}
		}
		// Update the autodetect downgrade version
		downgradeToVersion = requiredVersions.requiredUntil ? *requiredVersions.requiredUntil : entity::model::MilanVersion{};
	}
}

ControllerImpl::FailureAction ControllerImpl::getFailureActionForMvuCommandStatus(entity::ControllerEntity::MvuCommandStatus const status) const noexcept
{
	switch (status)
	{
		// Cases where the device seems busy
		case entity::ControllerEntity::MvuCommandStatus::EntityLocked: // Should not happen for a read operation but some devices are bugged, so retry anyway
		{
			return FailureAction::Busy;
		}

		// Query timed out
		case entity::ControllerEntity::MvuCommandStatus::TimedOut:
		{
			return FailureAction::TimedOut;
		}

		// Cases we want to flag as error and misbehaving entity, but continue enumeration
		case entity::ControllerEntity::MvuCommandStatus::BaseProtocolViolation:
		{
			return FailureAction::MisbehaveContinue;
		}

		// Cases we want to flag as error (should not have happened, we have a possible non certified entity) but continue enumeration
		case entity::ControllerEntity::MvuCommandStatus::NoSuchDescriptor:
			[[fallthrough]];
		case entity::ControllerEntity::MvuCommandStatus::ProtocolError:
		{
			return FailureAction::ErrorContinue;
		}

		// Case inbetween NotSupported and actual device error that should not happen
		case entity::ControllerEntity::MvuCommandStatus::PayloadTooShort:
			[[fallthrough]];
		case entity::ControllerEntity::MvuCommandStatus::BadArguments:
		{
			return FailureAction::BadArguments;
		}

		// Cases the caller should decide whether to continue enumeration or not
		case entity::ControllerEntity::MvuCommandStatus::NotImplemented:
		{
			return FailureAction::NotSupported;
		}

		// Cases the library do not implement
		case entity::ControllerEntity::MvuCommandStatus::PartialImplementation:
		{
			return FailureAction::WarningContinue;
		}

		// Cases that are errors and we want to discard this entity
		case entity::ControllerEntity::MvuCommandStatus::UnknownEntity:
			[[fallthrough]];
		case entity::ControllerEntity::MvuCommandStatus::EntityMisbehaving:
			[[fallthrough]];
		case entity::ControllerEntity::MvuCommandStatus::NetworkError:
			[[fallthrough]];
		case entity::ControllerEntity::MvuCommandStatus::InternalError:
			[[fallthrough]];
		default:
		{
			return FailureAction::ErrorFatal;
		}
	}
}

ControllerImpl::FailureAction ControllerImpl::getFailureActionForAemCommandStatus(entity::ControllerEntity::AemCommandStatus const status) const noexcept
{
	switch (status)
	{
		// Cases where the device seems busy
		case entity::ControllerEntity::AemCommandStatus::LockedByOther: // Should not happen for a read operation but some devices are bugged, so retry anyway
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::AcquiredByOther: // Should not happen for a read operation but some devices are bugged, so retry anyway
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::NoResources:
		{
			return FailureAction::Busy;
		}

		// Query timed out
		case entity::ControllerEntity::AemCommandStatus::TimedOut:
		{
			return FailureAction::TimedOut;
		}

		// Authentication required for this command
		case entity::ControllerEntity::AemCommandStatus::NotAuthenticated:
		{
			return FailureAction::NotAuthenticated;
		}

		// Cases we want to flag as error and misbehaving entity, but continue enumeration
		case entity::ControllerEntity::AemCommandStatus::BaseProtocolViolation:
		{
			return FailureAction::MisbehaveContinue;
		}

		// Cases we want to flag as error (should not have happened, we have a possible non certified entity) but continue enumeration
		case entity::ControllerEntity::AemCommandStatus::NoSuchDescriptor:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::AuthenticationDisabled:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::StreamIsRunning:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::ProtocolError:
		{
			return FailureAction::ErrorContinue;
		}

		// Case inbetween NotSupported and actual device error that should not happen
		case entity::ControllerEntity::AemCommandStatus::BadArguments:
		{
			return FailureAction::BadArguments;
		}

		// Cases the caller should decide whether to continue enumeration or not
		case entity::ControllerEntity::AemCommandStatus::NotImplemented:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::NotSupported:
		{
			return FailureAction::NotSupported;
		}

		// Cases the library do not implement
		case entity::ControllerEntity::AemCommandStatus::PartialImplementation:
		{
			return FailureAction::WarningContinue;
		}

		// Cases that are errors and we want to discard this entity
		case entity::ControllerEntity::AemCommandStatus::UnknownEntity:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::EntityMisbehaving:
#if defined(CONTINUE_MISBEHAVE_AEM_RESPONSES)
			return FailureAction::MisbehaveContinue;
#else // !CONTINUE_MISBEHAVE_AEM_RESPONSES
			[[fallthrough]];
#endif // CONTINUE_MISBEHAVE_AEM_RESPONSES
		case entity::ControllerEntity::AemCommandStatus::NetworkError:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::InternalError:
			[[fallthrough]];
		default:
		{
			return FailureAction::ErrorFatal;
		}
	}
}

ControllerImpl::FailureAction ControllerImpl::getFailureActionForControlStatus(entity::ControllerEntity::ControlStatus const status) const noexcept
{
	switch (status)
	{
		// Cases we want to flag as error and misbehaving entity, but continue enumeration
		case entity::ControllerEntity::ControlStatus::BaseProtocolViolation:
		{
			return FailureAction::MisbehaveContinue;
		}

		// Cases where the device seems busy
		case entity::ControllerEntity::ControlStatus::StateUnavailable:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::CouldNotSendMessage:
		{
			return FailureAction::Busy;
		}

		// Query timed out
		case entity::ControllerEntity::ControlStatus::TimedOut:
		{
			return FailureAction::TimedOut;
		}

		// Cases we want to ignore and continue enumeration
		case entity::ControllerEntity::ControlStatus::NotConnected:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::NoSuchConnection:
		{
			return FailureAction::WarningContinue;
		}

		// Cases we want to flag as error (should not have happened, we have a possible non certified entity) but continue enumeration
		case entity::ControllerEntity::ControlStatus::TalkerDestMacFail:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::TalkerNoBandwidth:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::TalkerExclusive:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::ListenerTalkerTimeout:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::ListenerExclusive:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::TalkerNoStreamIndex:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::ControllerNotAuthorized:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::IncompatibleRequest:
		{
			return FailureAction::ErrorContinue;
		}

		// Cases the caller should decide whether to continue enumeration or not
		case entity::ControllerEntity::ControlStatus::NotSupported:
		{
			return FailureAction::NotSupported;
		}

		// Cases that are errors and we want to discard this entity
		case entity::ControllerEntity::ControlStatus::UnknownEntity:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::ListenerUnknownID:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::TalkerUnknownID:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::TalkerMisbehaving:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::ListenerMisbehaving:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::NetworkError:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::ProtocolError:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::InternalError:
			[[fallthrough]];
		default:
		{
			return FailureAction::ErrorFatal;
		}
	}
}

/* This method handles non-success AemCommandStatus returned while trying to check if GET_DYNAMIC_INFO command is supported */
bool ControllerImpl::processEmptyGetDynamicInfoFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, MilanRequirements const& milanRequirements) noexcept
{
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const action = getFailureActionForAemCommandStatus(status);
	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			return true;
		case FailureAction::BadArguments:
			[[fallthrough]];
		case FailureAction::ErrorContinue:
			return true;
		case FailureAction::NotAuthenticated:
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::NotSupported:
			checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.4.2.29", "Milan mandatory command not supported by the entity: GET_DYNAMIC_INFO");
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
			auto const [shouldRetry, retryTimer] = entity->getCheckDynamicInfoSupportedRetryTimer();
			if (shouldRetry)
			{
				checkDynamicInfoSupported(entity);
			}
			else
			{
				// Too many retries, result depends on FailureAction and AemCommandStatus
				if (action == FailureAction::TimedOut)
				{
					checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.4.2.29", "Too many timeouts for Milan mandatory command: GET_DYNAMIC_INFO");
				}
				else if (action == FailureAction::Busy)
				{
					switch (status)
					{
						case entity::ControllerEntity::AemCommandStatus::LockedByOther: // Should not happen for a read operation but some devices are bugged
							[[fallthrough]];
						case entity::ControllerEntity::AemCommandStatus::AcquiredByOther: // Should not happen for a read operation but some devices are bugged
						{
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4.76", "Too many unexpected errors for AEM command: GET_DYNAMIC_INFO (" + entity::LocalEntity::statusToString(status) + ")");
							break;
						}
						default:
							break;
					}
				}
			}
			return true;
		}
		case FailureAction::ErrorFatal:
			return false;
		default:
			return false;
	}
}

/* This method handles non-success AemCommandStatus returned while using GET_DYNAMIC_INFO commands */
ControllerImpl::PackedDynamicInfoFailureAction ControllerImpl::processGetDynamicInfoFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::controller::DynamicInfoParameters const& dynamicInfoParameters, std::uint16_t const packetID, ControlledEntityImpl::EnumerationStep const step, MilanRequirements const& milanRequirements) noexcept
{
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const entityID = entity->getEntity().getEntityID();
	auto const action = getFailureActionForAemCommandStatus(status);
	auto checkScheduleRetry = false;
	auto fallbackEnumerationMode = false;

	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			fallbackEnumerationMode = true;
			break;
		case FailureAction::BadArguments:
			fallbackEnumerationMode = true;
			break;
		case FailureAction::ErrorContinue:
			fallbackEnumerationMode = true;
			break;
		case FailureAction::NotAuthenticated:
			return PackedDynamicInfoFailureAction::Continue;
		case FailureAction::WarningContinue:
			return PackedDynamicInfoFailureAction::Continue;
		case FailureAction::NotSupported:
			checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.4.2.29", "Milan mandatory command not supported by the entity: GET_DYNAMIC_INFO");
			fallbackEnumerationMode = true;
			break;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
			// Check if we should retry the command, if not we'll fallback to default enumeration
			checkScheduleRetry = true;
			fallbackEnumerationMode = true;
			break;
		}
		case FailureAction::ErrorFatal:
			return PackedDynamicInfoFailureAction::Fatal;
		default:
			return PackedDynamicInfoFailureAction::Fatal;
	}

	if (checkScheduleRetry)
	{
		auto const [shouldRetry, retryTimer] = entity->getGetDynamicInfoRetryTimer();
		if (shouldRetry)
		{
			queryInformation(entity, dynamicInfoParameters, packetID, step, retryTimer);
			return PackedDynamicInfoFailureAction::Continue;
		}
		else
		{
			// Too many retries, result depends on FailureAction and AemCommandStatus
			if (action == FailureAction::TimedOut)
			{
				checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.4.2.29", "Too many timeouts for Milan mandatory command: GET_DYNAMIC_INFO");
			}
			else if (action == FailureAction::Busy)
			{
				switch (status)
				{
					case entity::ControllerEntity::AemCommandStatus::LockedByOther: // Should not happen for a read operation but some devices are bugged
						[[fallthrough]];
					case entity::ControllerEntity::AemCommandStatus::AcquiredByOther: // Should not happen for a read operation but some devices are bugged
					{
						// Flag the entity as "Not fully IEEE1722.1 compliant"
						removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4.76", "Too many unexpected errors for AEM command: GET_DYNAMIC_INFO (" + entity::LocalEntity::statusToString(status) + ")");
						break;
					}
					default:
						break;
				}
			}
		}
		// Do not return now, we want to check if we should fallback to default enumeration
	}

	if (fallbackEnumerationMode)
	{
		// Disable fast enumeration mode
		entity->setPackedDynamicInfoSupported(false);
		// Clear all inflight queries
		entity->clearAllExpectedPackedDynamicInfo();

		// If we are in the middle of the GetDescriptorDynamicInfo step
		if (step == ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo)
		{
			// Set the entity as not using the cached EntityModel
			entity->setNotUsingCachedEntityModel();
			// Clear all DescriptorDynamicInfo queries
			entity->clearAllExpectedDescriptorDynamicInfo();
			// Fallback to full DescriptorDynamicInfo enumeration by restarting the enumeration
			entity->addEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
			AVDECC_ASSERT(entity->getEnumerationSteps().test(ControlledEntityImpl::EnumerationStep::GetDynamicInfo), "GetDynamicInfo step should be set");
			LOG_CONTROLLER_ERROR(entityID, "Failed to use cached EntityModel (too many DescriptorDynamic query retries), falling back to full StaticModel enumeration");
		}
		else if (step == ControlledEntityImpl::EnumerationStep::GetDynamicInfo)
		{
			// Clear all DynamicInfo queries
			entity->clearAllExpectedDynamicInfo();
			// Restart GetDynamicInfo enumeration without using fast enumeration mode
			entity->addEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
			LOG_CONTROLLER_ERROR(entityID, "Error getting DynamicInfo using fast enumeration mode, falling back to normal enumeration mode");
			AVDECC_ASSERT(!entity->getEnumerationSteps().test(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo), "GetDescriptorDynamicInfo step should not be set");
		}
		else
		{
			entity->setNotUsingCachedEntityModel();
			entity->clearAllExpectedDescriptorDynamicInfo();
			entity->clearAllExpectedPackedDynamicInfo();
			entity->addEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
			entity->addEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
			AVDECC_ASSERT(false, "Unexpected enumeration step");
		}
		return PackedDynamicInfoFailureAction::RestartStep;
	}

	return PackedDynamicInfoFailureAction::Fatal;
}

/* This method handles non-success AemCommandStatus returned while trying to RegisterUnsolicitedNotifications */
bool ControllerImpl::processRegisterUnsolFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, MilanRequirements const& milanRequirements) noexcept
{
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const action = getFailureActionForAemCommandStatus(status);
	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			return true;
		case FailureAction::BadArguments:
			[[fallthrough]];
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4.37", "Error registering for unsolicited notifications: " + entity::LocalEntity::statusToString(status));
			return true;
		case FailureAction::NotAuthenticated:
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::NotSupported:
			checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.4.2.21", "Milan mandatory command not supported by the entity: REGISTER_UNSOLICITED_NOTIFICATION");
			// Remove "Unsolicited notifications supported" as device does not support the command
			entity->setUnsolicitedNotificationsSupported(false);
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
			auto const [shouldRetry, retryTimer] = entity->getRegisterUnsolRetryTimer();
			if (shouldRetry)
			{
				registerUnsol(entity);
			}
			else
			{
				// Too many retries, result depends on FailureAction and AemCommandStatus
				if (action == FailureAction::TimedOut)
				{
					checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.4.2.21", "Too many timeouts for Milan mandatory command: REGISTER_UNSOLICITED_NOTIFICATION");
				}
				else if (action == FailureAction::Busy)
				{
					switch (status)
					{
						case entity::ControllerEntity::AemCommandStatus::LockedByOther: // Should not happen for a read operation but some devices are bugged
							[[fallthrough]];
						case entity::ControllerEntity::AemCommandStatus::AcquiredByOther: // Should not happen for a read operation but some devices are bugged
						{
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4.37", "Error registering for unsolicited notifications: " + entity::LocalEntity::statusToString(status));
							break;
						}
						default:
							break;
					}
				}
			}
			return true;
		}
		case FailureAction::ErrorFatal:
			return false;
		default:
			return false;
	}
}

/* This method handles non-success AemCommandStatus returned while getting EnumerationStep::GetMilanModel (MVU) */
bool ControllerImpl::processGetMilanInfoFailureStatus(entity::ControllerEntity::MvuCommandStatus const status, ControlledEntityImpl* const entity, ControlledEntityImpl::MilanInfoType const milanInfoType, MilanRequirements const& milanRequirements) noexcept
{
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const action = getFailureActionForMvuCommandStatus(status);
	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			return true;
		case FailureAction::BadArguments:
			[[fallthrough]];
		case FailureAction::ErrorContinue:
			// Remove "Milan compatibility" as device does not properly implement mandatory MVU
			if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
			{
				removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.4.3", "Milan mandatory MVU command not properly implemented by the entity");
			}
			return true;
		case FailureAction::NotAuthenticated:
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::NotSupported:
			checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.4.4.1", "Milan mandatory MVU command not supported by the entity: GET_MILAN_INFO");
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
			auto const [shouldRetry, retryTimer] = entity->getQueryMilanInfoRetryTimer();
			if (shouldRetry)
			{
				queryInformation(entity, milanInfoType, retryTimer);
			}
			else
			{
				// Too many retries, result depends on FailureAction and MvuCommandStatus
				if (action == FailureAction::TimedOut)
				{
					checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.4.4.1", "Too many timeouts for Milan mandatory MVU command: GET_MILAN_INFO");
				}
				else if (action == FailureAction::Busy)
				{
					// Flag the entity as "Not fully IEEE1722.1 compliant"
					removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4.37", "Too many unexpected errors for AEM command: REGISTER_UNSOLICITED_NOTIFICATION (" + entity::LocalEntity::statusToString(status) + ")");
				}
			}
			return true;
		}
		case FailureAction::ErrorFatal:
			return false;
		default:
			return false;
	}
}

/* This method handles non-success AemCommandStatus returned while getting EnumerationStep::GetStaticModel (AEM) */
bool ControllerImpl::processGetStaticModelFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const action = getFailureActionForAemCommandStatus(status);
	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			return true;
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4", "Error getting IEEE1722.1 mandatory descriptor (" + entity::model::descriptorTypeToString(descriptorType) + "): " + entity::LocalEntity::statusToString(status));
			return true;
		case FailureAction::NotAuthenticated:
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::BadArguments: // Getting the static model of an entity is not mandatory in 1722.1, thus we can ignore a BadArguments status
			[[fallthrough]];
		case FailureAction::NotSupported:
			// Remove "Milan compatibility" as device does not support mandatory descriptor
			if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
			{
				removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.3.2", "Milan mandatory descriptor not supported by the entity: " + entity::model::descriptorTypeToString(descriptorType));
			}
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
			auto const [shouldRetry, retryTimer] = entity->getQueryDescriptorRetryTimer();
			if (shouldRetry)
			{
				queryInformation(entity, configurationIndex, descriptorType, descriptorIndex, retryTimer);
			}
			else
			{
				// Too many retries, result depends on FailureAction and AemCommandStatus
				if (action == FailureAction::TimedOut)
				{
					// Remove "Milan compatibility" as device does not respond to mandatory command
					if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
					{
						removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan, "Milan 1.3 - 5.3.2", "Milan mandatory descriptor not supported by the entity: " + entity::model::descriptorTypeToString(descriptorType));
					}
				}
				else if (action == FailureAction::Busy)
				{
					switch (status)
					{
						case entity::ControllerEntity::AemCommandStatus::LockedByOther: // Should not happen for a read operation but some devices are bugged
							[[fallthrough]];
						case entity::ControllerEntity::AemCommandStatus::AcquiredByOther: // Should not happen for a read operation but some devices are bugged
							[[fallthrough]];
						case entity::ControllerEntity::AemCommandStatus::NoResources: // Should not happen for a read operation but some devices are bugged
						{
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4", "Too many unexpected errors for AEM command: READ_DESCRIPTOR (" + entity::LocalEntity::statusToString(status) + ")");
							break;
						}
						default:
							break;
					}
				}
			}
			return true;
		}
		case FailureAction::ErrorFatal:
			return false;
		default:
			return false;
	}
}

/* This method handles non-success AemCommandStatus returned while getting EnumerationStep::GetDynamicInfo for AECP commands */
bool ControllerImpl::processGetAecpDynamicInfoFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex, MilanRequirements const& milanRequirements) noexcept
{
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const action = getFailureActionForAemCommandStatus(status);
	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			return true;
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4", "Error getting IEEE1722.1 dynamic info (" + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType) + "): " + entity::LocalEntity::statusToString(status));
			return true;
		case FailureAction::NotAuthenticated:
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::BadArguments: // Getting the AECP dynamic info of an entity is not mandatory in 1722.1, thus we can ignore a BadArguments status
			[[fallthrough]];
		case FailureAction::NotSupported:
			checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.4.4", "Milan mandatory dynamic info not supported by the entity: " + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
			auto const [shouldRetry, retryTimer] = entity->getQueryDynamicInfoRetryTimer();
			if (shouldRetry)
			{
				queryInformation(entity, configurationIndex, dynamicInfoType, descriptorIndex, subIndex, retryTimer);
			}
			else
			{
				// Too many retries, result depends on FailureAction and AemCommandStatus
				if (action == FailureAction::TimedOut)
				{
					checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.4.4", "Too many timeouts for Milan mandatory dynamic info: " + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
				}
				else if (action == FailureAction::Busy)
				{
					switch (status)
					{
						case entity::ControllerEntity::AemCommandStatus::LockedByOther: // Should not happen for a read operation but some devices are bugged
							[[fallthrough]];
						case entity::ControllerEntity::AemCommandStatus::AcquiredByOther: // Should not happen for a read operation but some devices are bugged
							[[fallthrough]];
						case entity::ControllerEntity::AemCommandStatus::NoResources: // Should not happen for a read operation but some devices are bugged
						{
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4", "Too many unexpected errors for dynamic info query " + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType) + ": " + entity::LocalEntity::statusToString(status));
							break;
						}
						default:
							break;
					}
				}
			}
			return true;
		}
		case FailureAction::ErrorFatal:
			return false;
		default:
			return false;
	}
}

/* This method handles non-success MvuCommandStatus returned while getting EnumerationStep::GetDynamicInfo for MVU commands */
bool ControllerImpl::processGetMvuDynamicInfoFailureStatus(entity::ControllerEntity::MvuCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex, MilanRequirements const& milanRequirements) noexcept
{
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

#if defined(IGNORE_MISMATCHING_MVU_RESPONSES)
	auto updatedStatus = status;
	// If this is a Milan 1.2 or earlier device, it might respond to any non-GET_MILAN_INFO MVU command with a GET_MILAN_INFO response (even if we sent a SET_SYSTEM_UNIQUE_ID for example) // This is a known bug in some Milan 1.2 devices (which was not tested against the spec at the time)
	if (updatedStatus == entity::ControllerEntity::MvuCommandStatus::BaseProtocolViolation)
	{
		// Check if the device is Milan 1.2 or earlier (otherwise it really did a protocol violation)
		auto const milanInfo = entity->getMilanInfo();
		if (milanInfo && milanInfo->specificationVersion >= entity::model::MilanVersion{ 1, 0 } && milanInfo->specificationVersion < entity::model::MilanVersion{ 1, 3 })
		{
			switch (dynamicInfoType)
			{
				// Only these commands are defined in Milan 1.2 or earlier and are "allowed" to respond with GET_MILAN_INFO response if we consider the device a Milan 1.0 device
				case ControlledEntityImpl::DynamicInfoType::GetSystemUniqueID:
				case ControlledEntityImpl::DynamicInfoType::GetMediaClockReferenceInfo:
				{
					updatedStatus = entity::ControllerEntity::MvuCommandStatus::NotImplemented;
					LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Entity violated MVU protocol but is Milan 1.2 or earlier, treating BaseProtocolViolation as NotImplemented for {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
					ControllerImpl::decreaseMilanCompatibilityVersion(this, *entity, entity::model::MilanVersion{ 1, 0 }, "Milan 1.2 - 5.4.3", "Not responding with the correct command_type for MVU command: " + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
					break;
				}
				default:
					break;
			}
		}
	}
	auto const action = getFailureActionForMvuCommandStatus(updatedStatus);
#else // !defined(IGNORE_MISMATCHING_MVU_RESPONSES)
	auto const action = getFailureActionForMvuCommandStatus(status);
#endif // defined(IGNORE_MISMATCHING_MVU_RESPONSES)
	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			return true;
		case FailureAction::NotAuthenticated:
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::ErrorContinue:
			[[fallthrough]];
		case FailureAction::BadArguments:
			[[fallthrough]];
		case FailureAction::NotSupported:
			checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.4.2", "Milan mandatory dynamic info not supported by the entity: " + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
			auto const [shouldRetry, retryTimer] = entity->getQueryDynamicInfoRetryTimer();
			if (shouldRetry)
			{
				queryInformation(entity, configurationIndex, dynamicInfoType, descriptorIndex, subIndex, retryTimer);
			}
			else
			{
				// Too many retries, result depends on FailureAction and AemCommandStatus
				if (action == FailureAction::TimedOut)
				{
					checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.4.2", "Too many timeouts for Milan mandatory dynamic info: " + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
				}
			}
			return true;
		}
		case FailureAction::ErrorFatal:
			return false;
		default:
			return false;
	}
}

/* This method handles non-success ControlStatus returned while getting EnumerationStep::GetDynamicInfo for ACMP commands */
bool ControllerImpl::processGetAcmpDynamicInfoFailureStatus(entity::ControllerEntity::ControlStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, MilanRequirements const& milanRequirements) noexcept
{
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const action = getFailureActionForControlStatus(status);
	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			return true;
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 8.2", "Error getting IEEE1722.1 mandatory ACMP info (" + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType) + "): " + entity::LocalEntity::statusToString(status));
			return true;
		case FailureAction::NotAuthenticated:
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::NotSupported:
			checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.5", "Milan mandatory ACMP command not supported by the entity: " + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
			auto const [shouldRetry, retryTimer] = entity->getQueryDynamicInfoRetryTimer();
			if (shouldRetry)
			{
				queryInformation(entity, configurationIndex, dynamicInfoType, descriptorIndex, 0u, retryTimer);
			}
			else
			{
				// Too many retries, result depends on FailureAction and ControlStatus
				if (action == FailureAction::TimedOut)
				{
					checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.5", "Too many timeouts for Milan mandatory ACMP command: " + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
				}
				else if (action == FailureAction::Busy)
				{
					switch (status)
					{
						case entity::ControllerEntity::ControlStatus::StateUnavailable:
							[[fallthrough]];
						case entity::ControllerEntity::ControlStatus::CouldNotSendMessage:
						{
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 8.2", "Too many unexpected errors for ACMP command " + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType) + ": " + entity::LocalEntity::statusToString(status));
							break;
						}
						default:
							break;
					}
				}
			}
			return true;
		}
		case FailureAction::ErrorFatal:
			return false;
		default:
			return false;
	}
}

/* This method handles non-success ControlStatus returned while getting EnumerationStep::GetDynamicInfo for ACMP commands with a connection index */
bool ControllerImpl::processGetAcmpDynamicInfoFailureStatus(entity::ControllerEntity::ControlStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::StreamIdentification const& talkerStream, std::uint16_t const subIndex, MilanRequirements const& milanRequirements) noexcept
{
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const action = getFailureActionForControlStatus(status);
	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			return true;
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 8.2", "Error getting IEEE1722.1 mandatory ACMP info (" + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType) + "): " + entity::LocalEntity::statusToString(status));
			return true;
		case FailureAction::NotAuthenticated:
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::NotSupported:
			checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.5", "Milan mandatory ACMP command not supported by the entity: " + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
			auto const [shouldRetry, retryTimer] = entity->getQueryDynamicInfoRetryTimer();
			if (shouldRetry)
			{
				queryInformation(entity, configurationIndex, dynamicInfoType, talkerStream, subIndex, retryTimer);
			}
			else
			{
				// Too many retries, result depends on FailureAction and ControlStatus
				if (action == FailureAction::TimedOut)
				{
					checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.5", "Too many timeouts for Milan mandatory ACMP command: " + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
				}
				else if (action == FailureAction::Busy)
				{
					switch (status)
					{
						case entity::ControllerEntity::ControlStatus::StateUnavailable:
							[[fallthrough]];
						case entity::ControllerEntity::ControlStatus::CouldNotSendMessage:
						{
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 8.2", "Too many unexpected errors for ACMP command " + ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType) + ": " + entity::LocalEntity::statusToString(status));
							break;
						}
						default:
							break;
					}
				}
			}
			return true;
		}
		case FailureAction::ErrorFatal:
			return false;
		default:
			return false;
	}
}

/* This method handles non-success AemCommandStatus returned while getting EnumerationStep::GetDescriptorDynamicInfo (AEM) */
bool ControllerImpl::processGetDescriptorDynamicInfoFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, MilanRequirements const& milanRequirements) noexcept
{
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const entityID = entity->getEntity().getEntityID();
	auto const action = getFailureActionForAemCommandStatus(status);
	auto checkScheduleRetry = false;
	auto fallbackEnumerationMode = false;

	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			fallbackEnumerationMode = true;
			break;
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE1722.1-2021 - 7.4", "Error getting IEEE1722.1 descriptor dynamic info (" + ControlledEntityImpl::descriptorDynamicInfoTypeToString(descriptorDynamicInfoType) + "): " + entity::LocalEntity::statusToString(status));
			fallbackEnumerationMode = true;
			break;
		case FailureAction::NotAuthenticated:
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::TimedOut:
			checkScheduleRetry = true;
			fallbackEnumerationMode = true;
			break;
		case FailureAction::Busy:
			checkScheduleRetry = true;
			fallbackEnumerationMode = true;
			break;
		case FailureAction::NotSupported:
			checkMilanRequirements(entity, milanRequirements, "Milan 1.3 - 5.4.2", "Milan mandatory AECP command not supported by the entity: " + ControlledEntityImpl::descriptorDynamicInfoTypeToString(descriptorDynamicInfoType));
			fallbackEnumerationMode = true;
			break;
		case FailureAction::ErrorFatal:
			return false;
		default:
			return false;
	}

	if (checkScheduleRetry)
	{
		auto const [shouldRetry, retryTimer] = entity->getQueryDescriptorDynamicInfoRetryTimer();
		if (shouldRetry)
		{
			queryInformation(entity, configurationIndex, descriptorDynamicInfoType, descriptorIndex, retryTimer);
			return true;
		}
	}

	if (fallbackEnumerationMode)
	{
		// Failed to retrieve single DescriptorDynamicInformation, retrieve the corresponding descriptor instead if possible, otherwise switch back to full StaticModel enumeration
		auto const success = fetchCorrespondingDescriptor(entity, configurationIndex, descriptorDynamicInfoType, descriptorIndex);

		// Fallback to full StaticModel enumeration
		if (!success)
		{
			// Set the entity as not using the cached EntityModel
			entity->setNotUsingCachedEntityModel();
			// Flag the entity as not able to use the cached EntityModel
			entity->setIgnoreCachedEntityModel();
			// Clear all DescriptorDynamicInfo queries
			entity->clearAllExpectedDescriptorDynamicInfo();
			// Fallback to full descriptors enumeration
			entity->addEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
			LOG_CONTROLLER_ERROR(entityID, "Failed to use cached EntityModel (too many DescriptorDynamic query retries), falling back to full StaticModel enumeration");
		}
		return true;
	}

	return false;
}

bool ControllerImpl::fetchCorrespondingDescriptor(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	auto descriptorType{ entity::model::DescriptorType::Invalid };

	switch (descriptorDynamicInfoType)
	{
		case ControlledEntityImpl::DescriptorDynamicInfoType::ConfigurationName:
			descriptorType = entity::model::DescriptorType::Configuration;
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitName:
			descriptorType = entity::model::DescriptorType::AudioUnit;
			// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
			entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitSamplingRate, descriptorIndex);
			// Clear other DescriptorDynamicInfo that will be retrieved by subtree calls
			entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName, descriptorIndex);
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitSamplingRate:
			descriptorType = entity::model::DescriptorType::AudioUnit;
			// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
			entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitName, descriptorIndex);
			// Clear other DescriptorDynamicInfo that will be retrieved by subtree calls
			entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName, descriptorIndex);
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamName:
			descriptorType = entity::model::DescriptorType::StreamInput;
			// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
			entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamFormat, descriptorIndex);
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamFormat:
			descriptorType = entity::model::DescriptorType::StreamInput;
			// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
			entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamName, descriptorIndex);
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamName:
			descriptorType = entity::model::DescriptorType::StreamOutput;
			// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
			entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamFormat, descriptorIndex);
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamFormat:
			descriptorType = entity::model::DescriptorType::StreamOutput;
			// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
			entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamName, descriptorIndex);
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::InputJackName:
			descriptorType = entity::model::DescriptorType::JackInput;
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::OutputJackName:
			descriptorType = entity::model::DescriptorType::JackOutput;
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectName:
			descriptorType = entity::model::DescriptorType::MemoryObject;
			// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
			entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectLength, descriptorIndex);
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectLength:
			descriptorType = entity::model::DescriptorType::MemoryObject;
			// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
			entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectName, descriptorIndex);
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName:
			descriptorType = entity::model::DescriptorType::AudioCluster;
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::ControlName:
			descriptorType = entity::model::DescriptorType::Control;
			// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
			entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ControlValues, descriptorIndex);
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::ControlValues:
			descriptorType = entity::model::DescriptorType::Control;
			// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
			entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ControlName, descriptorIndex);
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainName:
			descriptorType = entity::model::DescriptorType::ClockDomain;
			// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
			entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainSourceIndex, descriptorIndex);
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainSourceIndex:
			descriptorType = entity::model::DescriptorType::ClockDomain;
			// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
			entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainName, descriptorIndex);
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::TimingName:
			descriptorType = entity::model::DescriptorType::Timing;
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::PtpInstanceName:
			descriptorType = entity::model::DescriptorType::PtpInstance;
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::PtpPortName:
			descriptorType = entity::model::DescriptorType::PtpPort;
			break;
		default:
			AVDECC_ASSERT(false, "Unhandled DescriptorDynamicInfoType");
			break;
	}

	if (!!descriptorType)
	{
		LOG_CONTROLLER_DEBUG(entity->getEntity().getEntityID(), "Failed to get DescriptorDynamicInfo ({}), falling back to {} Descriptor enumeration", ControlledEntityImpl::descriptorDynamicInfoTypeToString(descriptorDynamicInfoType), entity::model::descriptorTypeToString(descriptorType));
		queryInformation(entity, configurationIndex, descriptorType, descriptorIndex);
		return true;
	}

	return false;
}

void ControllerImpl::handleListenerStreamStateNotification(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, bool const isConnected, std::optional<entity::ConnectionFlags> const flags, bool const changedByOther) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Build StreamConnectionState::State
	auto conState{ entity::model::StreamInputConnectionInfo::State::NotConnected };
	if (isConnected)
	{
		conState = entity::model::StreamInputConnectionInfo::State::Connected;
	}
	else if (flags && flags->test(entity::ConnectionFlag::FastConnect))
	{
		conState = entity::model::StreamInputConnectionInfo::State::FastConnecting;
	}

	// Build Talker StreamIdentification
	auto talkerStreamIdentification{ entity::model::StreamIdentification{} };
	if (conState != entity::model::StreamInputConnectionInfo::State::NotConnected)
	{
		if (!talkerStream.entityID)
		{
			LOG_CONTROLLER_WARN(UniqueIdentifier::getNullUniqueIdentifier(), "Listener StreamState notification advertises being connected but with no Talker Identification (ListenerID={} ListenerIndex={})", utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex);
			conState = entity::model::StreamInputConnectionInfo::State::NotConnected;
		}
		else
		{
			talkerStreamIdentification = talkerStream;
		}
	}

	// Build a StreamInputConnectionInfo
	auto const info = entity::model::StreamInputConnectionInfo{ talkerStreamIdentification, conState };

	// Check if Listener is online so we can update the StreamState
	{
		// Take a "scoped locked" shared copy of the ControlledEntity
		auto listenerEntity = getControlledEntityImplGuard(listenerStream.entityID);

		if (listenerEntity)
		{
			// Check for invalid streamIndex
			auto const maxSinks = listenerEntity->getEntity().getCommonInformation().listenerStreamSinks;
			if (listenerStream.streamIndex >= maxSinks)
			{
				// Flag the entity as "Misbehaving"
				setMisbehavingCompatibilityFlag(this, *listenerEntity, "IEEE1722.1-2021 - 6.2.2.12", "Invalid CONNECTION STATE: StreamIndex is greater than maximum declared streams in ADP");
				return;
			}
			auto const previousInfo = listenerEntity->setStreamInputConnectionInformation(listenerStream.streamIndex, info, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

			// Entity was advertised to the user, notify observers
			if (listenerEntity->wasAdvertised() && previousInfo != info)
			{
				auto& listener = *listenerEntity;
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputConnectionChanged, this, &listener, listenerStream.streamIndex, info, changedByOther);

				auto isTalkerStreamChanged = previousInfo.talkerStream != info.talkerStream;
				// If the Listener was already advertised, check if talker StreamIdentification changed (no need to do it during listener enumeration, the connections to the talker will be updated when the listener is ready to advertise)
				if (isTalkerStreamChanged)
				{
					if (previousInfo.talkerStream.entityID)
					{
						// Update the cached connection on the talker (disconnect)
						handleTalkerStreamStateNotification(previousInfo.talkerStream, listenerStream, false, entity::ConnectionFlags{}, changedByOther); // Do not pass any flags (especially not FastConnect)
					}
					if (info.talkerStream.entityID && isConnected)
					{
						// Update the cached connection on the talker (connect)
						handleTalkerStreamStateNotification(info.talkerStream, listenerStream, true, entity::ConnectionFlags{}, changedByOther); // Do not pass any flags (especially not FastConnect)
					}
				}

				// Process all other entities and update media clock / channel connection if needed
				{
					// Lock to protect _controlledEntities
					auto const lg = std::lock_guard{ _lock };

					// Detects which connection transition is happening
					auto isConnecting = info.state == entity::model::StreamInputConnectionInfo::State::Connected && previousInfo.state == entity::model::StreamInputConnectionInfo::State::NotConnected;
					auto isDisconnecting = info.state == entity::model::StreamInputConnectionInfo::State::NotConnected && previousInfo.state == entity::model::StreamInputConnectionInfo::State::Connected;
					auto isConnectingToDifferentTalker = info.state == entity::model::StreamInputConnectionInfo::State::Connected && previousInfo.state == entity::model::StreamInputConnectionInfo::State::Connected && isTalkerStreamChanged;
					auto isLegacyFastConnecting = info.state == entity::model::StreamInputConnectionInfo::State::FastConnecting || previousInfo.state == entity::model::StreamInputConnectionInfo::State::FastConnecting;

					auto updateMediaClockChain = std::function<void(ControlledEntityImpl&, model::ClockDomainNode&)>{ nullptr };

					// We are now connected and we are not changing the talker
					if (isConnecting)
					{
						updateMediaClockChain = [this, &listenerStream](ControlledEntityImpl& entity, model::ClockDomainNode& clockDomainNode)
						{
							if (AVDECC_ASSERT_WITH_RET(!clockDomainNode.mediaClockChain.empty(), "Chain should not be empty"))
							{
								// Check if the last node had a status of StreamNotConnected for that listener
								if (auto const& lastNode = clockDomainNode.mediaClockChain.back(); lastNode.status == model::MediaClockChainNode::Status::StreamNotConnected && lastNode.entityID == listenerStream.entityID)
								{
									// Save the domain/stream indexes, we'll continue from it
									auto const continueDomainIndex = lastNode.clockDomainIndex;
									auto const continueStreamOutputIndex = lastNode.streamOutputIndex;

									// Remove the node
									clockDomainNode.mediaClockChain.pop_back();

									// Update the chain starting from this entity
									computeAndUpdateMediaClockChain(entity, clockDomainNode, listenerStream.entityID, continueDomainIndex, continueStreamOutputIndex, {});
								}
							}
						};
					}
					// We are now disconnected or we are changing the talker, check for any node in the chain that had an Active status with that listener
					else if (isDisconnecting || isConnectingToDifferentTalker)
					{
						updateMediaClockChain = [this, &listenerStream](ControlledEntityImpl& entity, model::ClockDomainNode& clockDomainNode)
						{
							// Check if the chain has a node on that disconnected listener entity
							for (auto nodeIt = clockDomainNode.mediaClockChain.begin(); nodeIt != clockDomainNode.mediaClockChain.end(); ++nodeIt)
							{
								auto const& node = *nodeIt;
								if (node.status == model::MediaClockChainNode::Status::Active && node.type == model::MediaClockChainNode::Type::StreamInput && node.entityID == listenerStream.entityID && node.streamInputIndex && *node.streamInputIndex == listenerStream.streamIndex)
								{
									// Save the domain/stream indexes, we'll continue from it
									auto const continueDomainIndex = nodeIt->clockDomainIndex;
									auto const continueStreamOutputIndex = nodeIt->streamOutputIndex;

									// Remove this node and all following nodes
									clockDomainNode.mediaClockChain.erase(nodeIt, clockDomainNode.mediaClockChain.end());

									// Update the chain starting from this entity
									computeAndUpdateMediaClockChain(entity, clockDomainNode, listenerStream.entityID, continueDomainIndex, continueStreamOutputIndex, {});
									break;
								}
							}
						};
					}
					else if (isLegacyFastConnecting)
					{
						LOG_CONTROLLER_DEBUG(UniqueIdentifier::getNullUniqueIdentifier(), "Legacy FastConnect transition for listener entity {}, nothing to do", utils::toHexString(listenerStream.entityID, true));
					}
					else
					{
						AVDECC_ASSERT(false, "Unsupported connection transition");
					}

					// Run the media clock updates if needed
					if (updateMediaClockChain)
					{
						// Update all entities for which the chain has a node with a connection to that stream
						for (auto& [eid, entity] : _controlledEntities)
						{
							if (entity->wasAdvertised() && entity->getEntity().getEntityCapabilities().test(entity::EntityCapability::AemSupported) && entity->hasAnyConfiguration())
							{
								auto* const configNode = entity->getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
								if (configNode != nullptr)
								{
									for (auto& clockDomainKV : configNode->clockDomains)
									{
										auto& clockDomainNode = clockDomainKV.second;
										updateMediaClockChain(*entity, clockDomainNode);
									}
								}
							}
						}
					}

#ifdef ENABLE_AVDECC_FEATURE_CBR
					// Run the channel connection updates
					{
						auto* const configurationNode = listener.getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
						if (configurationNode != nullptr)
						{
							auto const updateChannelConnectionIdentification = [this, &talkerStream, isConnecting, isDisconnecting, isConnectingToDifferentTalker](auto& channelConnectionIdentification)
							{
								auto changed = false;
								// If we are disconnecting or changing talker, disconnect previous talker stream
								if (isDisconnecting || isConnectingToDifferentTalker)
								{
									changed |= computeAndUpdateChannelConnectionFromStreamIdentification(entity::model::StreamIdentification{}, channelConnectionIdentification);
								}
								// If we are connecting or changing talker, connect new talker stream
								if (isConnecting || isConnectingToDifferentTalker)
								{
									changed |= computeAndUpdateChannelConnectionFromStreamIdentification(talkerStream, channelConnectionIdentification);
								}
								return changed;
							};

							// Process all channel connections that could be impacted
							for (auto& [clusterIdentification, channelIdentification] : configurationNode->channelConnections)
							{
								auto changed = false;

								// Check if this channel connection is linked to that listener stream
								if (channelIdentification.channelConnectionIdentification.streamChannelIdentification.streamIndex == listenerStream.streamIndex)
								{
									changed |= updateChannelConnectionIdentification(channelIdentification.channelConnectionIdentification);
								}
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
								// Check if this secondary channel connection is linked to that listener stream
								if (channelIdentification.secondaryChannelConnectionIdentification && channelIdentification.secondaryChannelConnectionIdentification->streamChannelIdentification.streamIndex == listenerStream.streamIndex)
								{
									changed |= updateChannelConnectionIdentification(*channelIdentification.secondaryChannelConnectionIdentification);
								}
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
								if (changed)
								{
									notifyObserversMethod<Controller::Observer>(&Controller::Observer::onChannelInputConnectionChanged, this, &listener, clusterIdentification, channelIdentification);
								}
							}
						}
					}
#endif // ENABLE_AVDECC_FEATURE_CBR
				}

				// Check for Diagnostics - Latency Error - Reset Error if stream is not connected
				if (!isConnected)
				{
					updateStreamInputLatency(*listenerEntity, listenerStream.streamIndex, false);
				}
			}
		}
	}
}

void ControllerImpl::handleTalkerStreamStateNotification(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, bool const isConnected, entity::ConnectionFlags const flags, bool const changedByOther) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Build Talker StreamIdentification
	auto const isFastConnect = flags.test(entity::ConnectionFlag::FastConnect);
	auto talkerStreamIdentification{ entity::model::StreamIdentification{} };
	if (isConnected || isFastConnect)
	{
		AVDECC_ASSERT(talkerStream.entityID, "Connected or FastConnecting to an invalid TalkerID");
		talkerStreamIdentification = talkerStream;
	}

	// For non-milan devices (that might not send a GetStreamInfo notification) in case of FastConnect, update the connection state (because there will not be any other direct notification to the controller)
	if (isFastConnect)
	{
		handleListenerStreamStateNotification(talkerStream, listenerStream, isConnected, flags, changedByOther);
	}

	// Check if Talker is valid and online so we can update the StreamConnections
	if (talkerStream.entityID)
	{
		// Take a "scoped locked" shared copy of the ControlledEntity
		auto talkerEntity = getControlledEntityImplGuard(talkerStream.entityID, true);

		// Only process talkers that are already advertised. The connections list will be completed by the talker right before advertising.
		if (talkerEntity)
		{
			// Update our internal cache
			auto shouldNotify{ false }; // Only notify if we actually changed the connections list
			if (isConnected)
			{
				shouldNotify = talkerEntity->addStreamOutputConnection(talkerStream.streamIndex, listenerStream, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				shouldNotify = talkerEntity->delStreamOutputConnection(talkerStream.streamIndex, listenerStream, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			if (shouldNotify)
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputConnectionsChanged, this, talkerEntity.get(), talkerStream.streamIndex, talkerEntity->getStreamOutputConnections(talkerStream.streamIndex));
			}
		}
	}
}

void ControllerImpl::clearTalkerStreamConnections(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const
{
	talkerEntity->clearStreamOutputConnections(talkerStreamIndex, notFoundBehavior);
}

void ControllerImpl::addTalkerStreamConnection(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIdentification const& listenerStream, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const
{
	// Update our internal cache
	talkerEntity->addStreamOutputConnection(talkerStreamIndex, listenerStream, notFoundBehavior);
}

#ifdef ENABLE_AVDECC_FEATURE_JSON
ControllerImpl::SharedControlledEntityImpl ControllerImpl::loadControlledEntityFromJson(json const& object, entity::model::jsonSerializer::Flags const flags, ControlledEntityImpl::LockInformation::SharedPointer const& lockInfo)
{
	auto controlledEntity = createControlledEntityFromJson(object, flags, lockInfo);

	auto& entity = *controlledEntity;

	// Set the Entity Model for our virtual entity
	auto const isAemSupported = entity.getEntity().getEntityCapabilities().test(entity::EntityCapability::AemSupported);
	if (isAemSupported && (flags.test(entity::model::jsonSerializer::Flag::ProcessStaticModel) || flags.test(entity::model::jsonSerializer::Flag::ProcessDynamicModel)))
	{
		jsonSerializer::setEntityModel(entity, object.at(jsonSerializer::keyName::ControlledEntity_EntityModel), flags);
	}

	// Set the Entity State
	if (flags.test(entity::model::jsonSerializer::Flag::ProcessState))
	{
		jsonSerializer::setEntityState(entity, object.at(jsonSerializer::keyName::ControlledEntity_EntityState));
	}

	// Set the Statistics
	if (flags.test(entity::model::jsonSerializer::Flag::ProcessStatistics))
	{
		auto const it = object.find(jsonSerializer::keyName::ControlledEntity_Statistics);
		if (it != object.end())
		{
			jsonSerializer::setEntityStatistics(entity, *it);
		}
	}

	// Set the Diagnostics
	if (flags.test(entity::model::jsonSerializer::Flag::ProcessDiagnostics))
	{
		auto const it = object.find(jsonSerializer::keyName::ControlledEntity_Diagnostics);
		if (it != object.end())
		{
			jsonSerializer::setEntityDiagnostics(entity, *it);
		}
	}

	// Choose a locale
	if (entity.hasAnyConfiguration())
	{
		// Load locale for each configuration
		try
		{
			auto const& entityNode = entity.getEntityNode();
			for (auto const& [configurationIndex, configurationNode] : entityNode.configurations)
			{
				chooseLocale(&entity, configurationIndex, "en-US", nullptr);
			}
		}
		catch (ControlledEntity::Exception const&)
		{
		}
	}

	auto const entityID = entity.getEntity().getEntityID();
	LOG_CONTROLLER_INFO(UniqueIdentifier::getNullUniqueIdentifier(), "Successfully loaded virtual entity with ID {}", utils::toHexString(entityID, true));

	return controlledEntity;
}

std::tuple<avdecc::jsonSerializer::DeserializationError, std::string> ControllerImpl::registerVirtualControlledEntity(SharedControlledEntityImpl&& controlledEntity) noexcept
{
	auto& entity = *controlledEntity;

	// Add the entity
	auto const entityID = entity.getEntity().getEntityID();
	{
		// Lock to protect _controlledEntities
		std::lock_guard<decltype(_lock)> const lg(_lock);

		auto entityIt = _controlledEntities.find(entityID);
		if (entityIt != _controlledEntities.end())
		{
			return { avdecc::jsonSerializer::DeserializationError::DuplicateEntityID, utils::toHexString(entityID, true) };
		}
		_controlledEntities.insert(std::make_pair(entityID, controlledEntity));
	}

	// Set entity as virtual
	_controllerProxy->setVirtualEntity(entityID);

	auto const exName = _endStation->getProtocolInterface()->getExecutorName();
	auto& executor = ExecutorManager::getInstance();

	runJobOnExecutorAndWait(executor, exName,
		[this, entityID]()
		{
			auto controlledEntity = getControlledEntityImplGuard(entityID);
			if (AVDECC_ASSERT_WITH_RET(!!controlledEntity, "Entity should be in the list"))
			{
				checkEnumerationSteps(controlledEntity.get());
			}
		});

	LOG_CONTROLLER_INFO(_controller->getEntityID(), "Successfully registered virtual entity with ID {}", utils::toHexString(entityID, true));

	return { avdecc::jsonSerializer::DeserializationError::NoError, "" };
}

ControllerImpl::SharedControlledEntityImpl ControllerImpl::deregisterVirtualControlledEntity(UniqueIdentifier const entityID) noexcept
{
	auto sharedControlledEntity = decltype(_controlledEntities)::value_type::second_type{};

	// Check if entity is virtual
	{
		// Lock to protect _controlledEntities
		std::lock_guard<decltype(_lock)> const lg(_lock);

		auto entityIt = _controlledEntities.find(entityID);
		// Entity not found
		if (entityIt == _controlledEntities.end())
		{
			return sharedControlledEntity;
		}
		// Entity is not virtual
		if (!entityIt->second->isVirtual())
		{
			return sharedControlledEntity;
		}
		// Take a shared ownership on the ControlledEntity (without locking it)
		sharedControlledEntity = entityIt->second;
	}

	// Ready to remove using the network executor
	auto const exName = _endStation->getProtocolInterface()->getExecutorName();
	auto& executor = ExecutorManager::getInstance();

	runJobOnExecutorAndWait(executor, exName,
		[this, entityID]()
		{
			onEntityOffline(_controller, entityID);
		});

	// Clear entity as virtual
	_controllerProxy->clearVirtualEntity(entityID);

	return sharedControlledEntity;
}

ControllerImpl::SharedControlledEntityImpl ControllerImpl::createControlledEntityFromJson(json const& object, entity::model::jsonSerializer::Flags const flags, ControlledEntityImpl::LockInformation::SharedPointer const& lockInfo)
{
	try
	{
		// Read information of the dump itself
		auto const dumpVersion = object.at(jsonSerializer::keyName::ControlledEntity_DumpVersion).get<decltype(jsonSerializer::keyValue::ControlledEntity_DumpVersion)>();
		// Check dump version
		if (dumpVersion > jsonSerializer::keyValue::ControlledEntity_DumpVersion)
		{
			throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::IncompatibleDumpVersion, std::string("Incompatible dump version: ") + std::to_string(dumpVersion) };
		}

		auto commonInfo = entity::Entity::CommonInformation{};
		auto intfcsInfo = entity::Entity::InterfacesInformation{};

		// Read ADP information
		if (flags.test(entity::model::jsonSerializer::Flag::ProcessADP))
		{
			auto const& adp = object.at(jsonSerializer::keyName::ControlledEntity_AdpInformation);

			// Read common information
			adp.at(entity::keyName::Entity_CommonInformation_Node).get_to(commonInfo);

			// Read interfaces information
			for (auto const& j : adp.at(entity::keyName::Entity_InterfaceInformation_Node))
			{
				auto const& jIndex = j.at(entity::keyName::Entity_InterfaceInformation_AvbInterfaceIndex);
				auto const avbInterfaceIndex = jIndex.is_null() ? entity::Entity::GlobalAvbInterfaceIndex : jIndex.get<entity::model::AvbInterfaceIndex>();
				intfcsInfo.insert(std::make_pair(avbInterfaceIndex, j.get<entity::Entity::InterfaceInformation>()));
			}
		}

		auto controlledEntity = std::make_shared<ControlledEntityImpl>(entity::Entity{ commonInfo, intfcsInfo }, lockInfo, true);
		auto& entity = *controlledEntity;

		// Start Enumeration timer
		entity.setStartEnumerationTime(std::chrono::steady_clock::now());

		// Read device compatibility
		if (flags.test(entity::model::jsonSerializer::Flag::ProcessCompatibility))
		{
			auto const compatFlags = object.at(jsonSerializer::keyName::ControlledEntity_CompatibilityFlags).get<ControlledEntity::CompatibilityFlags>();
			entity.setCompatibilityFlags(compatFlags);
			if (auto const it = object.find(jsonSerializer::keyName::ControlledEntity_MilanCompatibilityVersion); it != object.end())
			{
				auto const str = it->get<std::string>();
				entity.setMilanCompatibilityVersion(entity::model::MilanVersion{ str });
			}
			else if (compatFlags.test(ControlledEntity::CompatibilityFlag::Milan))
			{
				// Fallback to Milan 1.2 compatibility if the device has the Milan flag but there is no MilanCompatibilityVersion field (older dump). The compatibility version may be downgraded later during loading
				entity.setMilanCompatibilityVersion(entity::model::MilanVersion{ 1, 2 });
			}
			if (auto const it = object.find(jsonSerializer::keyName::ControlledEntity_CompatibilityEvents); it != object.end())
			{
				// Check if the CompatibilityEvents is an array
				if (it->is_array())
				{
					auto const& events = it->get<std::vector<ControlledEntity::CompatibilityChangedEvent>>();
					for (auto const& event : events)
					{
						entity.addCompatibilityChangedEvent(event);
					}
				}
			}
		}

		// Read Milan information, if present
		if (flags.test(entity::model::jsonSerializer::Flag::ProcessMilan))
		{
			auto milanInfo = std::optional<entity::model::MilanInfo>{};
			get_optional_value(object, jsonSerializer::keyName::ControlledEntity_MilanInformation, milanInfo);
			if (milanInfo)
			{
				entity.setMilanInfo(*milanInfo);
			}
		}

		// Read Milan Dynamic State, if present
		if (flags.test(entity::model::jsonSerializer::Flag::ProcessMilan) && flags.test(entity::model::jsonSerializer::Flag::ProcessDynamicModel))
		{
			auto milanDynamicState = std::optional<entity::model::MilanDynamicState>{};
			get_optional_value(object, jsonSerializer::keyName::ControlledEntity_MilanDynamicState, milanDynamicState);
			if (milanDynamicState)
			{
				entity.setMilanDynamicState(*milanDynamicState);
			}
		}

		return controlledEntity;
	}
	catch (avdecc::Exception const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::MissingKey, e.what() };
	}
	catch (json::type_error const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InvalidValue, e.what() };
	}
	catch (json::parse_error const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::ParseError, e.what() };
	}
	catch (json::out_of_range const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::MissingKey, e.what() };
	}
	catch (json::other_error const& e)
	{
		if (e.id == 555)
		{
			throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InvalidKey, e.what() };
		}
		else
		{
			throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::OtherError, e.what() };
		}
	}
	catch (json::exception const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::OtherError, e.what() };
	}
	catch (std::invalid_argument const& e)
	{
		throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InvalidValue, e.what() };
	}
}

std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, std::vector<ControllerImpl::SharedControlledEntityImpl>> ControllerImpl::deserializeJsonNetworkState(std::string const& filePath, entity::model::jsonSerializer::Flags const flags, bool const continueOnError, ControlledEntityImpl::LockInformation::SharedPointer const& lockInfo) noexcept
{
	// Try to open the input file
	auto const mode = std::ios::binary | std::ios::in;
	auto ifs = std::ifstream{ utils::filePathFromUTF8String(filePath), mode }; // We always want to read as 'binary', we don't want the cr/lf shit to alter the size of our allocated buffer (all modern code should handle both lf and cr/lf)

	// Failed to open file for reading
	if (!ifs.is_open())
	{
		return { avdecc::jsonSerializer::DeserializationError::AccessDenied, std::strerror(errno), {} };
	}

	auto object = json{};
	auto error = avdecc::jsonSerializer::DeserializationError::NoError;
	auto errorText = std::string{};
	auto controlledEntities = std::vector<SharedControlledEntityImpl>{};

	try
	{
		// Load the JSON object from disk
		if (flags.test(entity::model::jsonSerializer::Flag::BinaryFormat))
		{
			object = json::from_msgpack(ifs);
		}
		else
		{
			ifs >> object;
		}

		// Try to deserialize
		// Read information of the dump itself
		auto const dumpVersion = object.at(jsonSerializer::keyName::Controller_DumpVersion).get<decltype(jsonSerializer::keyValue::Controller_DumpVersion)>();
		if (dumpVersion != jsonSerializer::keyValue::Controller_DumpVersion)
		{
			return { avdecc::jsonSerializer::DeserializationError::IncompatibleDumpVersion, std::string("Incompatible dump version: ") + std::to_string(dumpVersion), {} };
		}

		// Get entities
		auto const& entitiesObject = object.at(jsonSerializer::keyName::Controller_Entities);
		if (!entitiesObject.is_array())
		{
			return { avdecc::jsonSerializer::DeserializationError::InvalidValue, std::string("Unsupported value type for ") + jsonSerializer::keyName::Controller_Entities + " (array expected)", {} };
		}
		for (auto const& entityObject : entitiesObject)
		{
			try
			{
				auto controlledEntity = loadControlledEntityFromJson(entityObject, flags, lockInfo);
				controlledEntities.push_back(std::move(controlledEntity));
			}
			catch (avdecc::jsonSerializer::DeserializationException const& e)
			{
				if (continueOnError)
				{
					error = avdecc::jsonSerializer::DeserializationError::Incomplete;
					errorText = e.what();
					continue;
				}
				return { e.getError(), e.what(), {} };
			}
			// Catch json and std exceptions thrown by loadControlledEntityFromJson
			catch (std::exception const& e)
			{
				if (continueOnError)
				{
					error = avdecc::jsonSerializer::DeserializationError::Incomplete;
					errorText = e.what();
					continue;
				}
				throw; // Rethrow
			}
		}
	}
	catch (json::type_error const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::InvalidValue, e.what(), {} };
	}
	catch (json::parse_error const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::ParseError, e.what(), {} };
	}
	catch (json::out_of_range const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::MissingKey, e.what(), {} };
	}
	catch (json::other_error const& e)
	{
		if (e.id == 555)
		{
			return { avdecc::jsonSerializer::DeserializationError::InvalidKey, e.what(), {} };
		}
		else
		{
			return { avdecc::jsonSerializer::DeserializationError::OtherError, e.what(), {} };
		}
	}
	catch (json::exception const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::OtherError, e.what(), {} };
	}
	catch (std::invalid_argument const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::InvalidValue, e.what(), {} };
	}

	return std::make_tuple(error, errorText, controlledEntities);
}

std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, ControllerImpl::SharedControlledEntityImpl> ControllerImpl::deserializeJson(std::string const& filePath, entity::model::jsonSerializer::Flags const flags, ControlledEntityImpl::LockInformation::SharedPointer const& lockInfo) noexcept
{
	// Try to open the input file
	auto const mode = std::ios::binary | std::ios::in;
	auto ifs = std::ifstream{ utils::filePathFromUTF8String(filePath), mode }; // We always want to read as 'binary', we don't want the cr/lf shit to alter the size of our allocated buffer (all modern code should handle both lf and cr/lf)

	// Failed to open file for reading
	if (!ifs.is_open())
	{
		return { avdecc::jsonSerializer::DeserializationError::AccessDenied, std::strerror(errno), nullptr };
	}

	// Load the JSON object from disk
	auto object = json{};
	try
	{
		if (flags.test(entity::model::jsonSerializer::Flag::BinaryFormat))
		{
			object = json::from_msgpack(ifs);
		}
		else
		{
			ifs >> object;
		}

		// Try to deserialize
		try
		{
			auto controlledEntity = loadControlledEntityFromJson(object, flags, lockInfo);
			return { avdecc::jsonSerializer::DeserializationError::NoError, "", controlledEntity };
		}
		catch (avdecc::jsonSerializer::DeserializationException const& e)
		{
			return { e.getError(), e.what(), nullptr };
		}
	}
	catch (json::type_error const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::InvalidValue, e.what(), nullptr };
	}
	catch (json::parse_error const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::ParseError, e.what(), nullptr };
	}
	catch (json::out_of_range const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::MissingKey, e.what(), nullptr };
	}
	catch (json::other_error const& e)
	{
		if (e.id == 555)
		{
			return { avdecc::jsonSerializer::DeserializationError::InvalidKey, e.what(), nullptr };
		}
		else
		{
			return { avdecc::jsonSerializer::DeserializationError::OtherError, e.what(), nullptr };
		}
	}
	catch (json::exception const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::OtherError, e.what(), nullptr };
	}
}

std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, entity::model::EntityTree, UniqueIdentifier> ControllerImpl::deserializeJsonEntityModel(std::string const& filePath, bool const isBinaryFormat) noexcept
{
	// Try to open the input file
	auto const mode = std::ios::binary | std::ios::in;
	auto ifs = std::ifstream{ utils::filePathFromUTF8String(filePath), mode }; // We always want to read as 'binary', we don't want the cr/lf shit to alter the size of our allocated buffer (all modern code should handle both lf and cr/lf)

	// Failed to open file for reading
	if (!ifs.is_open())
	{
		return { avdecc::jsonSerializer::DeserializationError::AccessDenied, std::strerror(errno), {}, {} };
	}

	// Load the JSON object from disk
	auto object = json{};
	try
	{
		auto flags = entity::model::jsonSerializer::Flags{ entity::model::jsonSerializer::Flag::ProcessStaticModel };
		if (isBinaryFormat)
		{
			flags.set(entity::model::jsonSerializer::Flag::BinaryFormat);
			object = json::from_msgpack(ifs);
		}
		else
		{
			ifs >> object;
		}

		// Read Entity Tree
		auto entityTree = entity::model::jsonSerializer::createEntityTree(object.at(jsonSerializer::keyName::ControlledEntity_EntityModel), flags);
		auto entityModelID = object.at(jsonSerializer::keyName::ControlledEntity_EntityModelID).get<la::avdecc::UniqueIdentifier>();
		return { avdecc::jsonSerializer::DeserializationError::NoError, "", entityTree, entityModelID };
	}
	catch (json::type_error const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::InvalidValue, e.what(), {}, {} };
	}
	catch (json::parse_error const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::ParseError, e.what(), {}, {} };
	}
	catch (json::out_of_range const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::MissingKey, e.what(), {}, {} };
	}
	catch (json::other_error const& e)
	{
		if (e.id == 555)
		{
			return { avdecc::jsonSerializer::DeserializationError::InvalidKey, e.what(), {}, {} };
		}
		else
		{
			return { avdecc::jsonSerializer::DeserializationError::OtherError, e.what(), {}, {} };
		}
	}
	catch (json::exception const& e)
	{
		return { avdecc::jsonSerializer::DeserializationError::OtherError, e.what(), {}, {} };
	}
	catch (avdecc::jsonSerializer::DeserializationException const& e)
	{
		return { e.getError(), e.what(), {}, {} };
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Exception type other than avdecc::jsonSerializer::DeserializationException are not expected to be thrown here");
		return { avdecc::jsonSerializer::DeserializationError::InternalError, "Exception type other than avdecc::jsonSerializer::DeserializationException are not expected to be thrown here.", {}, {} };
	}
}

void ControllerImpl::setupDetachedVirtualControlledEntity(ControlledEntityImpl& entity) noexcept
{
	// Notify the ControlledEntity it has been fully loaded
	entity.onEntityFullyLoaded();

	// Validate the entity, now that it's fully enumerated
	validateEntity(entity);

	// Declare entity as advertised
	entity.setAdvertised(true);
}
#endif // ENABLE_AVDECC_FEATURE_JSON

void ControllerImpl::runJobOnExecutorAndWait(la::avdecc::ExecutorManager& executor, std::string const& exName, Executor::Job&& job) const noexcept
{
	// If current thread is Executor thread, directly call handler
	if (std::this_thread::get_id() == executor.getExecutorThread(exName))
	{
		job();
	}
	else
	{
		// Ready to advertise using the network executor
		executor.pushJob(exName,
			[this, job = std::move(job)]()
			{
				auto const lg = std::lock_guard{ *_controller }; // Lock the Controller itself (thus, lock it's ProtocolInterface), since we are on the Networking Thread
				job();
			});

		// Insert a special "marker" job in the queue (and wait for it to be executed) to be sure everything is loaded before returning
		auto markerPromise = std::promise<void>{};
		executor.pushJob(exName,
			[&markerPromise]()
			{
				markerPromise.set_value();
			});

		// Wait for the marker job to be executed
		[[maybe_unused]] auto const status = markerPromise.get_future().wait_for(std::chrono::seconds{ 30 });
		AVDECC_ASSERT(status == std::future_status::ready, "Timeout waiting for marker job to be executed");
	}
}

std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, std::vector<SharedControlledEntity>> LA_AVDECC_CONTROLLER_CALL_CONVENTION Controller::deserializeControlledEntitiesFromJsonNetworkState(std::string const& filePath, entity::model::jsonSerializer::Flags const flags, bool const continueOnError) noexcept
{
	return ControllerImpl::deserializeControlledEntitiesFromJsonNetworkState(filePath, flags, continueOnError);
}

std::tuple<avdecc::jsonSerializer::DeserializationError, std::string, SharedControlledEntity> LA_AVDECC_CONTROLLER_CALL_CONVENTION Controller::deserializeControlledEntityFromJson(std::string const& filePath, entity::model::jsonSerializer::Flags const flags) noexcept
{
	return ControllerImpl::deserializeControlledEntityFromJson(filePath, flags);
}

entity::model::StreamFormat LA_AVDECC_CONTROLLER_CALL_CONVENTION Controller::chooseBestStreamFormat(entity::model::StreamFormats const& availableFormats, entity::model::StreamFormat const desiredStreamFormat, std::function<bool(bool const isDesiredClockSync, bool const isAvailableClockSync)> const& clockValidator) noexcept
{
	auto const desiredStreamFormatInfo = entity::model::StreamFormatInfo::create(desiredStreamFormat);
	auto const desiredFormatType = desiredStreamFormatInfo->getType();
	auto const desiredSamplingRate = desiredStreamFormatInfo->getSamplingRate();
	auto const desiredSampleFormat = desiredStreamFormatInfo->getSampleFormat();
	auto const desiredChannelsCount = desiredStreamFormatInfo->getChannelsCount();
	auto const desiredUseSyncClock = desiredStreamFormatInfo->useSynchronousClock();

	// Loop over available formats, and search for a matching one
	for (auto const& streamFormat : availableFormats)
	{
		auto const streamFormatInfo = entity::model::StreamFormatInfo::create(streamFormat);
		auto const formatType = streamFormatInfo->getType();
		auto const samplingRate = streamFormatInfo->getSamplingRate();
		auto const sampleFormat = streamFormatInfo->getSampleFormat();
		auto const useSyncClock = streamFormatInfo->useSynchronousClock();
		// Check basic properties
		if (desiredFormatType == formatType && desiredSamplingRate == samplingRate && desiredSampleFormat == sampleFormat && clockValidator(desiredUseSyncClock, useSyncClock))
		{
			// Check channel count, with possible up-to bit
			auto const channelsCount = streamFormatInfo->getChannelsCount();
			auto const isUpTo = streamFormatInfo->isUpToChannelsCount();
			if ((isUpTo && desiredChannelsCount <= channelsCount) || (desiredChannelsCount == channelsCount))
			{
				return streamFormatInfo->getAdaptedStreamFormat(desiredChannelsCount);
			}
		}
	}

	return {};
}

bool LA_AVDECC_CONTROLLER_CALL_CONVENTION Controller::isMediaClockStreamFormat(entity::model::StreamFormat const streamFormat) noexcept
{
	auto const streamFormatInfo = entity::model::StreamFormatInfo::create(streamFormat);
	auto const type = streamFormatInfo->getType();

	// CRF is always a media clock stream format
	if (type == entity::model::StreamFormatInfo::Type::ClockReference)
	{
		return true;
	}

	// TODO: Maybe check for 1 channel stream
	return false;
}

std::optional<std::string> LA_AVDECC_CONTROLLER_CALL_CONVENTION Controller::computeEntityModelChecksum(ControlledEntity const& controlledEntity, std::uint32_t const checksumVersion) noexcept
{
	if (controlledEntity.isEntityModelValidForCaching())
	{
		auto visitor = ChecksumEntityModelVisitor{ checksumVersion, controlledEntity.getMilanInfo() };
		controlledEntity.accept(&visitor, true);
		return visitor.getHash();
	}
	return std::nullopt;
}


} // namespace controller
} // namespace avdecc
} // namespace la
