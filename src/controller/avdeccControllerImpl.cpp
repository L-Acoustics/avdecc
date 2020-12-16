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
* @file avdeccControllerImpl.cpp
* @author Christophe Calmejane
*/

#include "avdeccControllerImpl.hpp"
#include "avdeccControllerLogHelper.hpp"
#include "avdeccEntityModelCache.hpp"

#ifdef ENABLE_AVDECC_FEATURE_JSON
#	include "avdeccControllerJsonTypes.hpp"
#	include <la/avdecc/internals/jsonTypes.hpp>
#endif // ENABLE_AVDECC_FEATURE_JSON
#include <la/avdecc/internals/streamFormatInfo.hpp>

// According to clarification (from IEEE1722.1 call) a device should always send the complete, up-to-date, status in a GET/SET_STREAM_INFO response (either unsolicited or not)
// This means that we should always replace the previously stored StreamInfo data with the last one received
#define REPLACE_STREAM_INFO

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
	auto oldEntity = controlledEntity.getEntity();

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
				updateGptpInformation(controlledEntity, avbInterfaceIndex, information.macAddress, *information.gptpGrandmasterID, *information.gptpDomainNumber);
			}
		}
	}

	// Set the new AssociationID and notify if it changed
	auto const associationID = entity.getAssociationID();
	setAssociationAndNotify(controlledEntity, associationID ? *associationID : UniqueIdentifier::getNullUniqueIdentifier());

	// Only do checks if entity was advertised to the user (we already changed the values anyway)
	if (controlledEntity.wasAdvertised())
	{
		// Check if Capabilities changed
		if (oldEntity.getEntityCapabilities() != entity.getEntityCapabilities())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityCapabilitiesChanged, this, &controlledEntity);
		}
	}

	// Update the full entity info (for information not separately handled)
	controlledEntity.setEntity(entity);
}

void ControllerImpl::addCompatibilityFlag(ControlledEntityImpl& controlledEntity, ControlledEntity::CompatibilityFlag const flag) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const oldFlags = controlledEntity.getCompatibilityFlags();
	auto newFlags{ oldFlags };

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
			}
			break;
		case ControlledEntity::CompatibilityFlag::Misbehaving:
			newFlags.reset(ControlledEntity::CompatibilityFlag::IEEE17221); // A misbehaving device is not IEEE1722.1 compatible
			newFlags.reset(ControlledEntity::CompatibilityFlag::Milan); // A misbehaving is not Milan compatible
			newFlags.set(flag);
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Entity is sending incoherent values (misbehaving)");
			break;
		default:
			AVDECC_ASSERT(false, "Unknown CompatibilityFlag");
			return;
	}

	if (oldFlags != newFlags)
	{
		controlledEntity.setCompatibilityFlags(newFlags);

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onCompatibilityFlagsChanged, this, &controlledEntity, newFlags);
		}
	}
}

void ControllerImpl::removeCompatibilityFlag(ControlledEntityImpl& controlledEntity, ControlledEntity::CompatibilityFlag const flag) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const oldFlags = controlledEntity.getCompatibilityFlags();
	auto newFlags{ oldFlags };

	switch (flag)
	{
		case ControlledEntity::CompatibilityFlag::IEEE17221:
			// If device was IEEE1722.1 compliant
			if (newFlags.test(ControlledEntity::CompatibilityFlag::IEEE17221))
			{
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Entity not fully IEEE1722.1 compliant");
				newFlags.reset(ControlledEntity::CompatibilityFlag::Milan); // A non compliant IEEE1722.1 device is not Milan compliant either
				newFlags.reset(flag);
			}
			break;
		case ControlledEntity::CompatibilityFlag::Milan:
			// If device was Milan compliant
			if (newFlags.test(ControlledEntity::CompatibilityFlag::Milan))
			{
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Entity not fully Milan compliant");
				newFlags.reset(flag);
			}
			break;
		case ControlledEntity::CompatibilityFlag::Misbehaving:
			AVDECC_ASSERT(false, "Should not be possible to remove the Misbehaving flag");
			break;
		default:
			AVDECC_ASSERT(false, "Unknown CompatibilityFlag");
			return;
	}

	if (oldFlags != newFlags)
	{
		controlledEntity.setCompatibilityFlags(newFlags);

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onCompatibilityFlagsChanged, this, &controlledEntity, newFlags);
		}
	}
}

void ControllerImpl::updateUnsolicitedNotificationsSubscription(ControlledEntityImpl& controlledEntity, bool const isSubscribed) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const oldValue = controlledEntity.isSubscribedToUnsolicitedNotifications();

	if (oldValue != isSubscribed)
	{
		controlledEntity.setSubscribedToUnsolicitedNotifications(isSubscribed);

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onUnsolicitedRegistrationChanged, this, &controlledEntity, isSubscribed);
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

void ControllerImpl::updateConfiguration(entity::controller::Interface const* const controller, ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex) const noexcept
{
	controlledEntity.setCurrentConfiguration(configurationIndex);

	// Right now, simulate the entity going offline then online again - TODO: Handle multiple configurations, see https://github.com/L-Acoustics/avdecc/issues/3
	auto const e = controlledEntity.getEntity(); // Make a copy of the Entity object since it will be destroyed during onEntityOffline
	auto const entityID = e.getEntityID();
	auto* self = const_cast<ControllerImpl*>(this);
	self->onEntityOffline(controller, entityID);
	self->onEntityOnline(controller, entityID, e);
}

void ControllerImpl::updateStreamInputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto& streamDynamicModel = controlledEntity.getNodeDynamicModel(controlledEntity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamInputModels);

	if (streamDynamicModel.streamFormat != streamFormat)
	{
		streamDynamicModel.streamFormat = streamFormat;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputFormatChanged, this, &controlledEntity, streamIndex, streamFormat);
		}
	}
}

void ControllerImpl::updateStreamOutputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto& streamDynamicModel = controlledEntity.getNodeDynamicModel(controlledEntity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels);

	if (streamDynamicModel.streamFormat != streamFormat)
	{
		streamDynamicModel.streamFormat = streamFormat;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputFormatChanged, this, &controlledEntity, streamIndex, streamFormat);
		}
	}
}

void ControllerImpl::updateStreamInputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, bool const streamFormatRequired, bool const milanExtendedRequired) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto hasStreamFormat = info.streamInfoFlags.test(entity::StreamInfoFlag::StreamFormatValid);

	// Try to detect non compliant entities
	if (streamFormatRequired)
	{
		// No StreamFormatValid bit
		if (!hasStreamFormat)
		{
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "StreamFormatValid bit not set in GET_STREAM_INFO response");
			removeCompatibilityFlag(controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
			// Check if we have something that looks like a valid streamFormat in the field
			auto const formatType = entity::model::StreamFormatInfo::create(info.streamFormat)->getType();
			if (formatType != entity::model::StreamFormatInfo::Type::None && formatType != entity::model::StreamFormatInfo::Type::Unsupported)
			{
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "StreamFormatValid bit set but stream_format field appears to contain a valid value in GET_STREAM_INFO response");
			}
		}
		// Or Invalid StreamFormat
		else if (!info.streamFormat)
		{
			hasStreamFormat = false;
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "StreamFormatValid bit set but invalid stream_format field in GET_STREAM_INFO response");
			removeCompatibilityFlag(controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
		}
	}
	// If Milan Extended Information is required (for GetStreamInfo, not SetStreamInfo) and entity is Milan compatible, check if it's present
	if (milanExtendedRequired && controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
	{
		if (!info.streamInfoFlagsEx || !info.probingStatus || !info.acmpStatus)
		{
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Milan mandatory extended GetStreamInfo not found");
			removeCompatibilityFlag(controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
		}
	}

	// Update each individual part of StreamInfo
	if (hasStreamFormat)
	{
		updateStreamInputFormat(controlledEntity, streamIndex, info.streamFormat);
	}
	updateStreamInputRunningStatus(controlledEntity, streamIndex, !info.streamInfoFlags.test(entity::StreamInfoFlag::StreamingWait));

#ifdef REPLACE_STREAM_INFO
	// Create a new StreamDynamicInfo
	auto dynamicInfo = entity::model::StreamDynamicInfo{};

	// Update each field
	dynamicInfo.isClassB = info.streamInfoFlags.test(entity::StreamInfoFlag::ClassB);
	dynamicInfo.hasSavedState = info.streamInfoFlags.test(entity::StreamInfoFlag::SavedState);
	dynamicInfo.doesSupportEncrypted = info.streamInfoFlags.test(entity::StreamInfoFlag::SupportsEncrypted);
	dynamicInfo.arePdusEncrypted = info.streamInfoFlags.test(entity::StreamInfoFlag::EncryptedPdu);
	dynamicInfo.hasTalkerFailed = info.streamInfoFlags.test(entity::StreamInfoFlag::TalkerFailed);
	dynamicInfo._streamInfoFlags = info.streamInfoFlags;

	if (info.streamInfoFlags.test(entity::StreamInfoFlag::StreamIDValid))
	{
		dynamicInfo.streamID = info.streamID;
	}
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::MsrpAccLatValid))
	{
		dynamicInfo.msrpAccumulatedLatency = info.msrpAccumulatedLatency;
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
	// Milan additions
	dynamicInfo.streamInfoFlagsEx = info.streamInfoFlagsEx;
	dynamicInfo.probingStatus = info.probingStatus;
	dynamicInfo.acmpStatus = info.acmpStatus;

	// Update StreamDynamicInfo
	auto& streamDynamicModel = controlledEntity.getNodeDynamicModel(controlledEntity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamInputModels);
	streamDynamicModel.streamDynamicInfo = std::move(dynamicInfo);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputDynamicInfoChanged, this, &controlledEntity, streamIndex, *streamDynamicModel.streamDynamicInfo);
	}
#else
	// Get a copy of previous StreamDynamicInfo
	auto& streamDynamicModel = controlledEntity.getNodeDynamicModel(controlledEntity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamInputModels);
	auto dynamicInfo = streamDynamicModel.streamDynamicInfo ? *streamDynamicModel.streamDynamicInfo : entity::model::StreamDynamicInfo{};
	auto changed = false;

	// Update each field checking for a change
	auto const updateFieldFromFlag = [&changed, flags = info.streamInfoFlags](auto& field, auto const flag)
	{
		auto const newValue = flags.test(flag);
		if (field != newValue)
		{
			field = newValue;
			changed = true;
		}
	};
	auto const updateFieldFromValue = [&changed](auto& field, auto const newValue)
	{
		if (field != newValue)
		{
			field = newValue;
			changed = true;
		}
	};
	auto const updateOptionalFieldFromValue = [&changed](auto& field, auto const newValue)
	{
		if (field != newValue)
		{
			field = newValue;
			changed = true;
		}
	};
	updateFieldFromFlag(dynamicInfo.isClassB, entity::StreamInfoFlag::ClassB);
	updateFieldFromFlag(dynamicInfo.hasSavedState, entity::StreamInfoFlag::SavedState);
	updateFieldFromFlag(dynamicInfo.doesSupportEncrypted, entity::StreamInfoFlag::SupportsEncrypted);
	updateFieldFromFlag(dynamicInfo.arePdusEncrypted, entity::StreamInfoFlag::EncryptedPdu);
	updateFieldFromFlag(dynamicInfo.hasTalkerFailed, entity::StreamInfoFlag::TalkerFailed);
	updateFieldFromValue(dynamicInfo._streamInfoFlags, info.streamInfoFlags);
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::StreamIDValid))
	{
		updateFieldFromValue(dynamicInfo.streamID, info.streamID);
	}
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::MsrpAccLatValid))
	{
		updateFieldFromValue(dynamicInfo.msrpAccumulatedLatency, info.msrpAccumulatedLatency);
	}
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::StreamDestMacValid))
	{
		updateFieldFromValue(dynamicInfo.streamDestMac, info.streamDestMac);
	}
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::MsrpFailureValid))
	{
		updateFieldFromValue(dynamicInfo.msrpFailureCode, info.msrpFailureCode);
		updateFieldFromValue(dynamicInfo.msrpFailureBridgeID, info.msrpFailureBridgeID);
	}
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::StreamVlanIDValid))
	{
		updateFieldFromValue(dynamicInfo.streamVlanID, info.streamVlanID);
	}
	// Milan additions
	if (info.streamInfoFlagsEx)
	{
		updateOptionalFieldFromValue(dynamicInfo.streamInfoFlagsEx, *info.streamInfoFlagsEx);
	}
	if (info.probingStatus)
	{
		updateOptionalFieldFromValue(dynamicInfo.probingStatus, *info.probingStatus);
	}
	if (info.acmpStatus)
	{
		updateOptionalFieldFromValue(dynamicInfo.acmpStatus, *info.acmpStatus);
	}

	if (changed)
	{
		// Update StreamDynamicInfo
		streamDynamicModel.streamDynamicInfo = std::move(dynamicInfo);

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputDynamicInfoChanged, this, &controlledEntity, streamIndex, *streamDynamicModel.streamDynamicInfo);
		}
	}
#endif
}

void ControllerImpl::updateStreamOutputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, bool const streamFormatRequired, bool const milanExtendedRequired) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto hasStreamFormat = info.streamInfoFlags.test(entity::StreamInfoFlag::StreamFormatValid);

	// Try to detect non compliant entities
	if (streamFormatRequired)
	{
		// No StreamFormatValid bit
		if (!hasStreamFormat)
		{
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "StreamFormatValid bit not set in GET_STREAM_INFO response");
			removeCompatibilityFlag(controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
			// Check if we have something that looks like a valid streamFormat in the field
			auto const formatType = entity::model::StreamFormatInfo::create(info.streamFormat)->getType();
			if (formatType != entity::model::StreamFormatInfo::Type::None && formatType != entity::model::StreamFormatInfo::Type::Unsupported)
			{
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "StreamFormatValid bit set but stream_format field appears to contain a valid value in GET_STREAM_INFO response");
			}
		}
		// Or Invalid StreamFormat
		else if (!info.streamFormat)
		{
			hasStreamFormat = false;
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "StreamFormatValid bit set but invalid stream_format field in GET_STREAM_INFO response");
			removeCompatibilityFlag(controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
		}
	}
	// If Milan Extended Information is required (for GetStreamInfo, not SetStreamInfo) and entity is Milan compatible, check if it's present
	if (milanExtendedRequired && controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
	{
		if (!info.streamInfoFlagsEx || !info.probingStatus || !info.acmpStatus)
		{
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Milan mandatory extended GetStreamInfo not found");
			removeCompatibilityFlag(controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
		}
	}

	// Update each individual part of StreamInfo
	if (hasStreamFormat)
	{
		updateStreamOutputFormat(controlledEntity, streamIndex, info.streamFormat);
	}
	updateStreamOutputRunningStatus(controlledEntity, streamIndex, !info.streamInfoFlags.test(entity::StreamInfoFlag::StreamingWait));

#ifdef REPLACE_STREAM_INFO
	// Create a new StreamDynamicInfo
	auto dynamicInfo = entity::model::StreamDynamicInfo{};

	// Update each field
	dynamicInfo.isClassB = info.streamInfoFlags.test(entity::StreamInfoFlag::ClassB);
	dynamicInfo.hasSavedState = info.streamInfoFlags.test(entity::StreamInfoFlag::SavedState);
	dynamicInfo.doesSupportEncrypted = info.streamInfoFlags.test(entity::StreamInfoFlag::SupportsEncrypted);
	dynamicInfo.arePdusEncrypted = info.streamInfoFlags.test(entity::StreamInfoFlag::EncryptedPdu);
	dynamicInfo.hasTalkerFailed = info.streamInfoFlags.test(entity::StreamInfoFlag::TalkerFailed);
	dynamicInfo._streamInfoFlags = info.streamInfoFlags;

	if (info.streamInfoFlags.test(entity::StreamInfoFlag::StreamIDValid))
	{
		dynamicInfo.streamID = info.streamID;
	}
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::MsrpAccLatValid))
	{
		dynamicInfo.msrpAccumulatedLatency = info.msrpAccumulatedLatency;
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
	// Milan additions
	dynamicInfo.streamInfoFlagsEx = info.streamInfoFlagsEx;
	dynamicInfo.probingStatus = info.probingStatus;
	dynamicInfo.acmpStatus = info.acmpStatus;

	// Update StreamDynamicInfo
	auto& streamDynamicModel = controlledEntity.getNodeDynamicModel(controlledEntity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels);
	streamDynamicModel.streamDynamicInfo = std::move(dynamicInfo);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputDynamicInfoChanged, this, &controlledEntity, streamIndex, *streamDynamicModel.streamDynamicInfo);
	}
#else
	// Get a copy of previous StreamDynamicInfo
	auto& streamDynamicModel = controlledEntity.getNodeDynamicModel(controlledEntity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels);
	auto dynamicInfo = streamDynamicModel.streamDynamicInfo ? *streamDynamicModel.streamDynamicInfo : entity::model::StreamDynamicInfo{};
	auto changed = false;

	// Update each field checking for a change
	auto const updateFieldFromFlag = [&changed, flags = info.streamInfoFlags](auto& field, auto const flag)
	{
		auto const newValue = flags.test(flag);
		if (field != newValue)
		{
			field = newValue;
			changed = true;
		}
	};
	auto const updateFieldFromValue = [&changed](auto& field, auto const newValue)
	{
		if (field != newValue)
		{
			field = newValue;
			changed = true;
		}
	};
	auto const updateOptionalFieldFromValue = [&changed](auto& field, auto const newValue)
	{
		if (field != newValue)
		{
			field = newValue;
			changed = true;
		}
	};
	updateFieldFromFlag(dynamicInfo.isClassB, entity::StreamInfoFlag::ClassB);
	updateFieldFromFlag(dynamicInfo.hasSavedState, entity::StreamInfoFlag::SavedState);
	updateFieldFromFlag(dynamicInfo.doesSupportEncrypted, entity::StreamInfoFlag::SupportsEncrypted);
	updateFieldFromFlag(dynamicInfo.arePdusEncrypted, entity::StreamInfoFlag::EncryptedPdu);
	updateFieldFromFlag(dynamicInfo.hasTalkerFailed, entity::StreamInfoFlag::TalkerFailed);
	updateFieldFromValue(dynamicInfo._streamInfoFlags, info.streamInfoFlags);
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::StreamIDValid))
	{
		updateFieldFromValue(dynamicInfo.streamID, info.streamID);
	}
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::MsrpAccLatValid))
	{
		updateFieldFromValue(dynamicInfo.msrpAccumulatedLatency, info.msrpAccumulatedLatency);
	}
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::StreamDestMacValid))
	{
		updateFieldFromValue(dynamicInfo.streamDestMac, info.streamDestMac);
	}
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::MsrpFailureValid))
	{
		updateFieldFromValue(dynamicInfo.msrpFailureCode, info.msrpFailureCode);
		updateFieldFromValue(dynamicInfo.msrpFailureBridgeID, info.msrpFailureBridgeID);
	}
	if (info.streamInfoFlags.test(entity::StreamInfoFlag::StreamVlanIDValid))
	{
		updateFieldFromValue(dynamicInfo.streamVlanID, info.streamVlanID);
	}
	// Milan additions
	if (info.streamInfoFlagsEx)
	{
		updateOptionalFieldFromValue(dynamicInfo.streamInfoFlagsEx, *info.streamInfoFlagsEx);
	}
	if (info.probingStatus)
	{
		updateOptionalFieldFromValue(dynamicInfo.probingStatus, *info.probingStatus);
	}
	if (info.acmpStatus)
	{
		updateOptionalFieldFromValue(dynamicInfo.acmpStatus, *info.acmpStatus);
	}

	if (changed)
	{
		// Update StreamDynamicInfo
		streamDynamicModel.streamDynamicInfo = std::move(dynamicInfo);

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputDynamicInfoChanged, this, &controlledEntity, streamIndex, *streamDynamicModel.streamDynamicInfo);
		}
	}
#endif
}

void ControllerImpl::updateEntityName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityName) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setEntityName(entityName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityNameChanged, this, &controlledEntity, entityName);
	}
}

void ControllerImpl::updateEntityGroupName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityGroupName) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setEntityGroupName(entityGroupName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityGroupNameChanged, this, &controlledEntity, entityGroupName);
	}
}

void ControllerImpl::updateConfigurationName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setConfigurationName(configurationIndex, configurationName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onConfigurationNameChanged, this, &controlledEntity, configurationIndex, configurationName);
	}
}

void ControllerImpl::updateAudioUnitName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& audioUnitName) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, audioUnitIndex, &entity::model::ConfigurationTree::audioUnitModels, audioUnitName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAudioUnitNameChanged, this, &controlledEntity, configurationIndex, audioUnitIndex, audioUnitName);
	}
}

void ControllerImpl::updateStreamInputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, streamIndex, &entity::model::ConfigurationTree::streamInputModels, streamInputName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputNameChanged, this, &controlledEntity, configurationIndex, streamIndex, streamInputName);
	}
}

void ControllerImpl::updateStreamOutputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, streamIndex, &entity::model::ConfigurationTree::streamOutputModels, streamOutputName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputNameChanged, this, &controlledEntity, configurationIndex, streamIndex, streamOutputName);
	}
}

void ControllerImpl::updateAvbInterfaceName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& avbInterfaceName) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, avbInterfaceIndex, &entity::model::ConfigurationTree::avbInterfaceModels, avbInterfaceName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAvbInterfaceNameChanged, this, &controlledEntity, configurationIndex, avbInterfaceIndex, avbInterfaceName);
	}
}

void ControllerImpl::updateClockSourceName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& clockSourceName) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, clockSourceIndex, &entity::model::ConfigurationTree::clockSourceModels, clockSourceName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onClockSourceNameChanged, this, &controlledEntity, configurationIndex, clockSourceIndex, clockSourceName);
	}
}

void ControllerImpl::updateMemoryObjectName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& memoryObjectName) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, memoryObjectIndex, &entity::model::ConfigurationTree::memoryObjectModels, memoryObjectName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onMemoryObjectNameChanged, this, &controlledEntity, configurationIndex, memoryObjectIndex, memoryObjectName);
	}
}

void ControllerImpl::updateAudioClusterName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& audioClusterName) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, audioClusterIndex, &entity::model::ConfigurationTree::audioClusterModels, audioClusterName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAudioClusterNameChanged, this, &controlledEntity, configurationIndex, audioClusterIndex, audioClusterName);
	}
}

void ControllerImpl::updateClockDomainName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& clockDomainName) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setObjectName(configurationIndex, clockDomainIndex, &entity::model::ConfigurationTree::clockDomainModels, clockDomainName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onClockDomainNameChanged, this, &controlledEntity, configurationIndex, clockDomainIndex, clockDomainName);
	}
}

void ControllerImpl::setAssociationAndNotify(ControlledEntityImpl& controlledEntity, UniqueIdentifier const associationID) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto& entity = controlledEntity.getEntity();
	auto const previousAssociationID = entity.getAssociationID();
	entity.setAssociationID(associationID);

	// Only do checks if entity was advertised to the user (we already changed the values anyway)
	if (controlledEntity.wasAdvertised())
	{
		// Notify if AssociationID changed
		if (previousAssociationID != associationID)
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityAssociationChanged, this, &controlledEntity);
		}
	}
}

//void ControllerImpl::updateAssociationID(ControlledEntityImpl& controlledEntity, UniqueIdentifier const associationID) const noexcept
//{
//	// Set the new AssociationID and notify if it changed
//	setAssociationAndNotify(controlledEntity, associationID);
//
//	// Update the Entity as well
//	auto entity = controlledEntity.getEntity(); // Copy the entity so we can alter values in the copy and not the original
//	auto const caps = entity.getEntityCapabilities();
//
//	if (!caps.test(entity::EntityCapability::AssociationIDSupported))
//	{
//		LOG_CONTROLLER_WARN(entity.getEntityID(), "Entity changed its ASSOCIATION_ID but it said ASSOCIATION_ID_NOT_SUPPORTED in ADPDU");
//		return;
//	}
//
//	// Only update the Entity if AssociationIDValid flag was not set in ADPDU
//	if (caps.test(entity::EntityCapability::AssociationIDValid))
//	{
//		entity.setAssociationID(associationID);
//		setEntityAndNotify(controlledEntity, entity);
//	}
//}

void ControllerImpl::updateAudioUnitSamplingRate(ControlledEntityImpl& controlledEntity, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setSamplingRate(audioUnitIndex, samplingRate);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAudioUnitSamplingRateChanged, this, &controlledEntity, audioUnitIndex, samplingRate);
	}
}

void ControllerImpl::updateClockSource(ControlledEntityImpl& controlledEntity, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setClockSource(clockDomainIndex, clockSourceIndex);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onClockSourceChanged, this, &controlledEntity, clockDomainIndex, clockSourceIndex);
	}
}

void ControllerImpl::updateStreamInputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto& streamDynamicModel = controlledEntity.getNodeDynamicModel(controlledEntity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamInputModels);

	// Never initialized or changed
	if (!streamDynamicModel.isStreamRunning || *streamDynamicModel.isStreamRunning != isRunning)
	{
		streamDynamicModel.isStreamRunning = isRunning;

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

void ControllerImpl::updateStreamOutputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto& streamDynamicModel = controlledEntity.getNodeDynamicModel(controlledEntity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels);

	// Never initialized or changed
	if (!streamDynamicModel.isStreamRunning || *streamDynamicModel.isStreamRunning != isRunning)
	{
		streamDynamicModel.isStreamRunning = isRunning;

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

void ControllerImpl::updateGptpInformation(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, networkInterface::MacAddress const& macAddress, UniqueIdentifier const& gptpGrandmasterID, std::uint8_t const gptpDomainNumber) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto infoChanged = false;

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

	// Then update gPTP Info in existing AvbDescriptors (don't create if not created yet)
	auto& avbDescriptorModels = controlledEntity.getModels(controlledEntity.getCurrentConfigurationIndex(), &entity::model::ConfigurationTree::avbInterfaceModels);
	for (auto& [interfaceIndex, avbInterfaceModel] : avbDescriptorModels)
	{
		// Match with the passed AvbInterfaceIndex, or with macAddress if passed AvbInterfaceIndex is the GlobalAvbInterfaceIndex
		if (interfaceIndex == avbInterfaceIndex || (avbInterfaceIndex == entity::Entity::GlobalAvbInterfaceIndex && macAddress == avbInterfaceModel.staticModel.macAddress))
		{
			// Alter InterfaceInfo with new gPTP info
			if (avbInterfaceModel.dynamicModel.gptpGrandmasterID != gptpGrandmasterID || avbInterfaceModel.dynamicModel.gptpDomainNumber != gptpDomainNumber)
			{
				avbInterfaceModel.dynamicModel.gptpGrandmasterID = gptpGrandmasterID;
				avbInterfaceModel.dynamicModel.gptpDomainNumber = gptpDomainNumber;
				infoChanged |= true;
			}
		}
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

void ControllerImpl::updateAvbInfo(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Build AvbInterfaceInfo structure
	auto const avbInterfaceInfo = entity::model::AvbInterfaceInfo{ info.propagationDelay, info.flags, info.mappings };

	// Update AvbInterfaceInfo
	auto const previousInfo = controlledEntity.setAvbInterfaceInfo(avbInterfaceIndex, avbInterfaceInfo);

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
	auto const& macAddress = controlledEntity.getNodeStaticModel(controlledEntity.getCurrentConfigurationIndex(), avbInterfaceIndex, &entity::model::ConfigurationTree::avbInterfaceModels).macAddress;
	updateGptpInformation(controlledEntity, avbInterfaceIndex, macAddress, info.gptpGrandmasterID, info.gptpDomainNumber);
}

void ControllerImpl::updateAsPath(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AsPath const& asPath) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const previousPath = controlledEntity.setAsPath(avbInterfaceIndex, asPath);

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

void ControllerImpl::updateAvbInterfaceLinkStatus(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, ControlledEntity::InterfaceLinkStatus const linkStatus) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	auto const previousLinkStatus = controlledEntity.setAvbInterfaceLinkStatus(avbInterfaceIndex, linkStatus);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		// Changed
		if (previousLinkStatus != linkStatus)
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAvbInterfaceLinkStatusChanged, this, &controlledEntity, avbInterfaceIndex, linkStatus);
		}
	}
}

void ControllerImpl::updateEntityCounters(ControlledEntityImpl& controlledEntity, entity::EntityCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Get previous counters
	auto& entityCounters = controlledEntity.getEntityCounters();

	// Update (or set) counters
	for (auto counter : validCounters)
	{
		entityCounters[counter] = counters[validCounters.getPosition(counter)];
	}

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityCountersChanged, this, &controlledEntity, entityCounters);
	}
}

void ControllerImpl::updateAvbInterfaceCounters(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::AvbInterfaceCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Get previous counters
	auto& avbInterfaceCounters = controlledEntity.getAvbInterfaceCounters(avbInterfaceIndex);

	// Update (or set) counters
	for (auto counter : validCounters)
	{
		avbInterfaceCounters[counter] = counters[validCounters.getPosition(counter)];
	}

	// Check for link status update
	// We must not access avbInterfaceCounters through operator[] or it will create a value if it does not exist. We want the LinkStatus update to be available even for non Milan devices
	auto const upIt = avbInterfaceCounters.find(entity::AvbInterfaceCounterValidFlag::LinkUp);
	auto const downIt = avbInterfaceCounters.find(entity::AvbInterfaceCounterValidFlag::LinkDown);
	if (upIt != avbInterfaceCounters.end() && downIt != avbInterfaceCounters.end())
	{
		auto const upValue = upIt->second;
		auto const downValue = downIt->second;
		auto const isUp = upValue == (downValue + 1);
		updateAvbInterfaceLinkStatus(controlledEntity, avbInterfaceIndex, isUp ? ControlledEntity::InterfaceLinkStatus::Up : ControlledEntity::InterfaceLinkStatus::Down);
	}

	// If Milan device, validate counters values
	if (controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
	{
		// LinkDown should either be equal to LinkUp or be one more (Milan Clause 6.6.3)
		// We are safe to get those counters, check for their presence during first enumeration has already been done
		auto const upValue = counters[validCounters.getPosition(entity::AvbInterfaceCounterValidFlag::LinkUp)];
		auto const downValue = counters[validCounters.getPosition(entity::AvbInterfaceCounterValidFlag::LinkDown)];
		if (upValue != downValue && upValue != (downValue + 1))
		{
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Invalid LINK_UP / LINK_DOWN counters value on AVB_INTERFACE:{} ({} / {})", avbInterfaceIndex, upValue, downValue);
			removeCompatibilityFlag(controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
		}
	}

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAvbInterfaceCountersChanged, this, &controlledEntity, avbInterfaceIndex, avbInterfaceCounters);
	}
}

void ControllerImpl::updateClockDomainCounters(ControlledEntityImpl& controlledEntity, entity::model::ClockDomainIndex const clockDomainIndex, entity::ClockDomainCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Get previous counters
	auto& clockDomainCounters = controlledEntity.getClockDomainCounters(clockDomainIndex);

	// Update (or set) counters
	for (auto counter : validCounters)
	{
		clockDomainCounters[counter] = counters[validCounters.getPosition(counter)];
	}

	// If Milan device, validate counters values
	if (controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
	{
		// Unlocked should either be equal to Locked or be one more (Milan Clause 6.11.2)
		// We are safe to get those counters, check for their presence during first enumeration has already been done
		auto const lockedValue = clockDomainCounters[entity::ClockDomainCounterValidFlag::Locked];
		auto const unlockedValue = clockDomainCounters[entity::ClockDomainCounterValidFlag::Unlocked];
		if (lockedValue != unlockedValue && lockedValue != (unlockedValue + 1))
		{
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Invalid LOCKED / UNLOCKED counters value on CLOCK_DOMAIN:{} ({} / {})", clockDomainIndex, lockedValue, unlockedValue);
			removeCompatibilityFlag(controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
		}
	}

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onClockDomainCountersChanged, this, &controlledEntity, clockDomainIndex, clockDomainCounters);
	}
}

void ControllerImpl::updateStreamInputCounters(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::StreamInputCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Get previous counters
	auto& streamCounters = controlledEntity.getStreamInputCounters(streamIndex);

	// Update (or set) counters
	for (auto counter : validCounters)
	{
		streamCounters[counter] = counters[validCounters.getPosition(counter)];
	}

	// If Milan device, validate counters values
	if (controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
	{
		// MediaUnlocked should either be equal to MediaLocked or be one more (Milan Clause 6.8.10)
		// We are safe to get those counters, check for their presence during first enumeration has already been done
		auto const lockedValue = streamCounters[entity::StreamInputCounterValidFlag::MediaLocked];
		auto const unlockedValue = streamCounters[entity::StreamInputCounterValidFlag::MediaUnlocked];
		if (lockedValue != unlockedValue && lockedValue != (unlockedValue + 1))
		{
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Invalid MEDIA_LOCKED / MEDIA_UNLOCKED counters value on STREAM_INPUT:{} ({} / {})", streamIndex, lockedValue, unlockedValue);
			removeCompatibilityFlag(controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
		}
	}

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputCountersChanged, this, &controlledEntity, streamIndex, streamCounters);
	}
}

void ControllerImpl::updateStreamOutputCounters(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::StreamOutputCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Get previous counters
	auto& streamCounters = controlledEntity.getStreamOutputCounters(streamIndex);

	// Update (or set) counters
	for (auto counter : validCounters)
	{
		streamCounters[counter] = counters[validCounters.getPosition(counter)];
	}

	// If Milan device, validate counters values
	if (controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
	{
		// StreamStop should either be equal to StreamStart or be one more (Milan Clause 6.7.7)
		// We are safe to get those counters, check for their presence during first enumeration has already been done
		auto const startValue = streamCounters[entity::StreamOutputCounterValidFlag::StreamStart];
		auto const stopValue = streamCounters[entity::StreamOutputCounterValidFlag::StreamStop];
		if (startValue != stopValue && startValue != (stopValue + 1))
		{
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Invalid STREAM_START / STREAM_STOP counters value on STREAM_OUTPUT:{} ({} / {})", streamIndex, startValue, stopValue);
			removeCompatibilityFlag(controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
		}
	}

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputCountersChanged, this, &controlledEntity, streamIndex, streamCounters);
	}
}

void ControllerImpl::updateMemoryObjectLength(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.setMemoryObjectLength(configurationIndex, memoryObjectIndex, length);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onMemoryObjectLengthChanged, this, &controlledEntity, configurationIndex, memoryObjectIndex, length);
	}
}

void ControllerImpl::updateStreamPortInputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.addStreamPortInputAudioMappings(streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortInputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

void ControllerImpl::updateStreamPortInputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.removeStreamPortInputAudioMappings(streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortInputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

void ControllerImpl::updateStreamPortOutputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.addStreamPortOutputAudioMappings(streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortOutputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

void ControllerImpl::updateStreamPortOutputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	controlledEntity.removeStreamPortOutputAudioMappings(streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortOutputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

void ControllerImpl::updateOperationStatus(ControlledEntityImpl& controlledEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, std::uint16_t const percentComplete) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		if (percentComplete == 0) /* Clause 7.4.55.2 */
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

/* ************************************************************ */
/* Private methods                                              */
/* ************************************************************ */
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
				LOG_CONTROLLER_WARN(entity.getEntity().getEntityID(), "Milan must not implement ACQUIRE_ENTITY");
				removeCompatibilityFlag(entity, ControlledEntity::CompatibilityFlag::Milan);
			}
			break;
		case entity::ControllerEntity::AemCommandStatus::AcquiredByOther:
			acquireState = model::AcquireState::AcquiredByOther;
			owningController = owningEntity;
			// Remove "Milan compatibility" as device does support a forbidden command
			if (entity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
			{
				LOG_CONTROLLER_WARN(entity.getEntity().getEntityID(), "Milan device must not implement ACQUIRE_ENTITY");
				removeCompatibilityFlag(entity, ControlledEntity::CompatibilityFlag::Milan);
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
				LOG_CONTROLLER_WARN(entity.getEntity().getEntityID(), "Milan device must implement LOCK_ENTITY");
				removeCompatibilityFlag(entity, ControlledEntity::CompatibilityFlag::Milan);
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

void ControllerImpl::chooseLocale(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	entity::model::LocaleNodeStaticModel const* localeNode{ nullptr };
	localeNode = entity->findLocaleNode(configurationIndex, _preferedLocale);
	if (localeNode == nullptr)
	{
#pragma message("TODO: Split _preferedLocale into language/country, then if findLocaleDescriptor fails and language is not 'en', try to find a locale for 'en'")
		localeNode = entity->findLocaleNode(configurationIndex, "en");
	}
	if (localeNode != nullptr)
	{
		auto const& configTree = entity->getConfigurationTree(configurationIndex);

		entity->setSelectedLocaleStringsIndexesRange(configurationIndex, localeNode->baseStringDescriptorIndex, localeNode->numberOfStringDescriptors);
		for (auto index = entity::model::StringsIndex(0); index < localeNode->numberOfStringDescriptors; ++index)
		{
			// Check if we already have the Strings descriptor
			auto const stringsIndex = static_cast<decltype(index)>(localeNode->baseStringDescriptorIndex + index);
			auto const stringsModelIt = configTree.stringsModels.find(stringsIndex);
			if (stringsModelIt != configTree.stringsModels.end())
			{
				// Already in cache, no need to query (just have to copy strings to Configuration for quick access)
				auto const& stringsStaticModel = stringsModelIt->second.staticModel;
				entity->setLocalizedStrings(configurationIndex, index, stringsStaticModel.strings);
			}
			else
			{
				queryInformation(entity, configurationIndex, entity::model::DescriptorType::Strings, stringsIndex);
			}
		}
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
		case entity::model::DescriptorType::AvbInterface:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readAvbInterfaceDescriptor (ConfigurationIndex={}, AvbInterfaceIndex={})", configurationIndex, descriptorIndex);
				controller->readAvbInterfaceDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAvbInterfaceDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::ClockSource:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readClockSourceDescriptor (ConfigurationIndex={} ClockSourceIndex={})", configurationIndex, descriptorIndex);
				controller->readClockSourceDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onClockSourceDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
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
		case entity::model::DescriptorType::ClockDomain:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readClockDomainDescriptor (ConfigurationIndex={}, ClockDomainIndex={})", configurationIndex, descriptorIndex);
				controller->readClockDomainDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onClockDomainDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
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
		case ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex, subIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamPortInputAudioMap (StreamPortIndex={})", descriptorIndex);
				controller->getStreamPortInputAudioMap(entityID, descriptorIndex, subIndex, std::bind(&ControllerImpl::onGetStreamPortInputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings:
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
		case ControlledEntityImpl::DescriptorDynamicInfoType::AvbInterfaceName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getAvbInterfaceName (ConfigurationIndex={} AvbInterfaceIndex={})", configurationIndex, descriptorIndex);
				controller->getAvbInterfaceName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAvbInterfaceNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::ClockSourceName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getClockSourceName (ConfigurationIndex={} ClockSourceIndex={})", configurationIndex, descriptorIndex);
				controller->getClockSourceName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onClockSourceNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
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

void ControllerImpl::registerUnsol(ControlledEntityImpl* const entity) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();

	// Immediately set as expected
	entity->setRegisterUnsolExpected();

	// Register for unsolicited notifications
	LOG_CONTROLLER_TRACE(entityID, "registerUnsolicitedNotifications ()");
	_controller->registerUnsolicitedNotifications(entityID, std::bind(&ControllerImpl::onRegisterUnsolicitedNotificationsResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void ControllerImpl::getStaticModel(ControlledEntityImpl* const entity) noexcept
{
	// Always start with Entity Descriptor, the response from it will schedule subsequent descriptors queries
	queryInformation(entity, 0, entity::model::DescriptorType::Entity, 0);
}

void ControllerImpl::getDynamicInfo(ControlledEntityImpl* const entity) noexcept
{
	auto const caps = entity->getEntity().getEntityCapabilities();
	// Check if AEM is supported by this entity
	if (caps.test(entity::EntityCapability::AemSupported))
	{
		auto const configurationIndex = entity->getCurrentConfigurationIndex();
		auto const& configTree = entity->getConfigurationTree(configurationIndex);

		// Get AcquiredState / LockedState (global entity information not related to current configuration)
		{
			// Milan devices don't implement AcquireEntity, no need to query its state
			if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
			{
				entity->setAcquireState(model::AcquireState::NotSupported);
			}
			else
			{
				queryInformation(entity, 0u, ControlledEntityImpl::DynamicInfoType::AcquiredState, 0u);
			}
			queryInformation(entity, 0u, ControlledEntityImpl::DynamicInfoType::LockedState, 0u);
		}

		// Entity Counters
		queryInformation(entity, 0u, ControlledEntityImpl::DynamicInfoType::GetEntityCounters, 0u);

		// Get StreamInfo/Counters and RX_STATE for each StreamInput descriptors
		{
			auto const count = configTree.streamInputModels.size();
			for (auto index = entity::model::StreamIndex(0); index < count; ++index)
			{
				// StreamInfo
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamInfo, index);

				// Counters
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::GetStreamInputCounters, index);

				// RX_STATE
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamState, index);
			}
		}

		// Get StreamInfo and TX_STATE for each StreamOutput descriptors
		{
			auto const count = configTree.streamOutputModels.size();
			for (auto index = entity::model::StreamIndex(0); index < count; ++index)
			{
				// StreamInfo
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamInfo, index);

				// Counters
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::GetStreamOutputCounters, index);

				// TX_STATE
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamState, index);
			}
		}

		// Get AvbInfo/Counters for each AvbInterface descriptors
		{
			auto const count = configTree.avbInterfaceModels.size();
			for (auto index = entity::model::AvbInterfaceIndex(0); index < count; ++index)
			{
				// AvbInfo
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInfo, index);
				// AsPath
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAsPath, index);
				// Counters
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInterfaceCounters, index);
			}
		}

		// Get Counters for each ClockDomain descriptors
		{
			auto const count = configTree.clockDomainModels.size();
			for (auto index = entity::model::ClockDomainIndex(0); index < count; ++index)
			{
				// Counters
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::GetClockDomainCounters, index);
			}
		}

		// Get AudioMappings for each StreamPortInput descriptors
		{
			auto const count = configTree.streamPortInputModels.size();
			for (auto index = entity::model::StreamPortIndex(0); index < count; ++index)
			{
				auto const& staticModel = entity->getNodeStaticModel(configurationIndex, index, &entity::model::ConfigurationTree::streamPortInputModels);
				if (staticModel.numberOfMaps == 0)
				{
					// TODO: Clause 7.4.44.3 recommands to Lock or Acquire the entity before getting the dynamic audio map
					queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, index);
				}
			}
		}

		// Get AudioMappings for each StreamPortOutput descriptors
		{
			auto const count = configTree.streamPortOutputModels.size();
			for (auto index = entity::model::StreamPortIndex(0); index < count; ++index)
			{
				auto const& staticModel = entity->getNodeStaticModel(configurationIndex, index, &entity::model::ConfigurationTree::streamPortOutputModels);
				if (staticModel.numberOfMaps == 0)
				{
					// TODO: Clause 7.4.44.3 recommands to Lock or Acquire the entity before getting the dynamic audio map
					queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, index);
				}
			}
		}
	}

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
	if (caps.test(entity::EntityCapability::AemSupported))
	{
		auto const& entityTree = entity->getEntityTree();
		auto const currentConfigurationIndex = entity->getCurrentConfigurationIndex();

		// Get DynamicModel for each Configuration descriptors
		for (auto configurationIndex = entity::model::ConfigurationIndex(0u); configurationIndex < entityTree.configurationTrees.size(); ++configurationIndex)
		{
			auto const& configTree = entity->getConfigurationTree(configurationIndex);
			auto& configDynamicModel = entity->getConfigurationNodeDynamicModel(configurationIndex);

			// We can set the currentConfiguration value right now, we know it
			configDynamicModel.isActiveConfiguration = configurationIndex == currentConfigurationIndex;

			// Get ConfigurationName
			queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ConfigurationName, 0u);

			// And only for the current configuration, get DynamicModel for sub-descriptors
			if (configDynamicModel.isActiveConfiguration)
			{
				// Choose a locale
				chooseLocale(entity, configurationIndex);

				// Get DynamicModel for each AudioUnit descriptors
				{
					auto const count = configTree.audioUnitModels.size();
					for (auto index = entity::model::AudioUnitIndex(0); index < count; ++index)
					{
						// Get AudioUnitName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitName, index);
						// Get AudioUnitSamplingRate
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitSamplingRate, index);
					}
				}
				// Get DynamicModel for each StreamInput descriptors
				{
					auto const count = configTree.streamInputModels.size();
					for (auto index = entity::model::StreamIndex(0); index < count; ++index)
					{
						// Get InputStreamName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamName, index);
						// Get InputStreamFormat
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamFormat, index);
					}
				}
				// Get DynamicModel for each StreamOutput descriptors
				{
					auto const count = configTree.streamOutputModels.size();
					for (auto index = entity::model::StreamIndex(0); index < count; ++index)
					{
						// Get OutputStreamName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamName, index);
						// Get OutputStreamFormat
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamFormat, index);
					}
				}
				// Get DynamicModel for each AvbInterface descriptors
				{
					auto const count = configTree.avbInterfaceModels.size();
					for (auto index = entity::model::AvbInterfaceIndex(0); index < count; ++index)
					{
						// Get AvbInterfaceName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AvbInterfaceName, index);
					}
				}
				// Get DynamicModel for each ClockSource descriptors
				{
					auto const count = configTree.clockSourceModels.size();
					for (auto index = entity::model::ClockSourceIndex(0); index < count; ++index)
					{
						// Get ClockSourceName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockSourceName, index);
					}
				}
				// Get DynamicModel for each MemoryObject descriptors
				{
					auto const count = configTree.memoryObjectModels.size();
					for (auto index = entity::model::MemoryObjectIndex(0); index < count; ++index)
					{
						// Get MemoryObjectName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectName, index);
						// Get MemoryObjectLength
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectLength, index);
					}
				}
				// Get DynamicModel for each AudioCluster descriptors
				{
					auto const count = configTree.audioClusterModels.size();
					for (auto index = entity::model::ClusterIndex(0); index < count; ++index)
					{
						// Get AudioClusterName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName, index);
					}
				}
				// Get DynamicModel for each ClockDomain descriptors
				{
					auto const count = configTree.clockDomainModels.size();
					for (auto index = entity::model::ClockDomainIndex(0); index < count; ++index)
					{
						// Get ClockDomainName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainName, index);
						// Get ClockDomainSourceIndex
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainSourceIndex, index);
					}
				}
			}
		}
	}

	// Get all expected descriptor dynamic information
	if (entity->gotAllExpectedDescriptorDynamicInfo())
	{
		// Clear this enumeration step and check for next one
		entity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
		checkEnumerationSteps(entity);
	}
}

void ControllerImpl::checkEnumerationSteps(ControlledEntityImpl* const entity) noexcept
{
	auto const steps = entity->getEnumerationSteps();

	// Always start with retrieving MilanInfo from the device
	if (steps.test(ControlledEntityImpl::EnumerationStep::GetMilanInfo))
	{
		getMilanInfo(entity);
		return;
	}
	// Then register to unsolicited notifications
	if (steps.test(ControlledEntityImpl::EnumerationStep::RegisterUnsol))
	{
		registerUnsol(entity);
		return;
	}
	// Then get the static AEM
	if (steps.test(ControlledEntityImpl::EnumerationStep::GetStaticModel))
	{
		getStaticModel(entity);
		return;
	}
	// Then get descriptors dynamic information, it AEM is cached
	if (steps.test(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo))
	{
		getDescriptorDynamicInfo(entity);
		return;
	}
	// Finally retrieve all other dynamic information
	if (steps.test(ControlledEntityImpl::EnumerationStep::GetDynamicInfo))
	{
		getDynamicInfo(entity);
		return;
	}

	// Ready to advertise the entity
	if (!entity->wasAdvertised())
	{
		if (!entity->gotFatalEnumerationError())
		{
			// Do some final steps before advertising entity
			onPreAdvertiseEntity(*entity);

			auto& entityModelCache = EntityModelCache::getInstance();
			auto const& e = entity->getEntity();
			auto const& entityID = e.getEntityID();
			auto const& entityModelID = e.getEntityModelID();
			// If AEM Cache is Enabled and the entity has an EntityModelID defined
			if (entityModelCache.isCacheEnabled() && entityModelID)
			{
				if (EntityModelCache::isValidEntityModelID(entityModelID))
				{
					// Store EntityModel in the cache for later use
					entityModelCache.cacheEntityTree(entityModelID, entity->getEntityTree());
					LOG_CONTROLLER_INFO(entityID, "AEM-CACHE: Cached model for EntityModelID {}", utils::toHexString(entityModelID, true, false));
				}
				else
				{
					LOG_CONTROLLER_INFO(entityID, "AEM-CACHE: Not caching model with invalid EntityModelID {} (invalid Vendor OUI-24)", utils::toHexString(entityModelID, true, false));
				}
			}

			// Advertise the entity
			entity->setAdvertised(true);
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityOnline, this, entity);
		}
	}
}

void ControllerImpl::onPreAdvertiseEntity(ControlledEntityImpl& controlledEntity) noexcept
{
	auto const& e = controlledEntity.getEntity();
	auto const entityID = e.getEntityID();

	// Save the enumeration time
	controlledEntity.setEndEnumerationTime(std::chrono::steady_clock::now());

	// If AEM is supported, build the Entity Model Graph
	if (e.getEntityCapabilities().test(entity::EntityCapability::AemSupported))
	{
		controlledEntity.buildEntityModelGraph();
	}

	// For a Talker, we want to build an accurate list of connections, based on the known listeners (already advertised only, the other ones will update once ready to advertise themselves)
	if (e.getTalkerCapabilities().test(entity::TalkerCapability::Implemented) && e.getEntityCapabilities().test(entity::EntityCapability::AemSupported))
	{
		AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

		try
		{
			auto const& talkerConfigurationNode = controlledEntity.getCurrentConfigurationNode();

			// Lock to protect _controlledEntities
			auto const lg = std::lock_guard{ _lock };

			// For all (advertised) ControlledEntities, check if they are connected to this talker
			for (auto const& entityKV : _controlledEntities)
			{
				auto const listenerEntityID = entityKV.first;
				auto const& listenerEntity = *(entityKV.second);

				// Don't process self, nor not yet advertised entities
				if (listenerEntityID == entityID || !listenerEntity.wasAdvertised())
				{
					continue;
				}
				if (listenerEntity.getEntity().getEntityCapabilities().test(entity::EntityCapability::AemSupported))
				{
					try
					{
						auto const& configurationNode = listenerEntity.getCurrentConfigurationNode();
						for (auto const& [streamIndex, streamInputNode] : configurationNode.streamInputs)
						{
							if (streamInputNode.dynamicModel)
							{
								// If the Stream is Connected
								if (streamInputNode.dynamicModel->connectionInfo.state == entity::model::StreamInputConnectionInfo::State::Connected)
								{
									// Check against all the Talker's Output Streams
									for (auto const& streamOutputNodeKV : talkerConfigurationNode.streamOutputs)
									{
										auto const streamOutputIndex = streamOutputNodeKV.first;
										//auto const& streamOutputNode = streamOutputNodeKV.second;
										auto const talkerIdentification = entity::model::StreamIdentification{ entityID, streamOutputIndex };

										// Connected to our talker
										if (streamInputNode.dynamicModel->connectionInfo.talkerStream == talkerIdentification)
										{
											controlledEntity.addStreamOutputConnection(streamOutputIndex, { listenerEntityID, streamIndex });
											// Do not trigger any notification, we are just about to advertise the entity
										}
									}
								}
							}
						}
					}
					catch (...)
					{
						AVDECC_ASSERT(false, "Unexpected exception");
					}
				}
			}
		}
		catch (...)
		{
			AVDECC_ASSERT(false, "Unexpected exception");
		}
	}

	// For a Listener, we want to inform all the talkers we are connected to (already advertised only, the other ones will update once ready to advertise themselves)
	if (e.getListenerCapabilities().test(entity::ListenerCapability::Implemented) && e.getEntityCapabilities().test(entity::EntityCapability::AemSupported))
	{
		AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

		try
		{
			auto const& configurationNode = controlledEntity.getCurrentConfigurationNode();

			for (auto const& [streamIndex, streamInputNode] : configurationNode.streamInputs)
			{
				if (streamInputNode.dynamicModel)
				{
					// If the Stream is Connected, search for the Talker we are connected to
					if (streamInputNode.dynamicModel->connectionInfo.state == entity::model::StreamInputConnectionInfo::State::Connected)
					{
						// Take a "scoped locked" shared copy of the ControlledEntity
						auto talkerEntity = getControlledEntityImplGuard(streamInputNode.dynamicModel->connectionInfo.talkerStream.entityID, true);

						if (talkerEntity)
						{
							auto& talker = *talkerEntity;
							auto const talkerStreamIndex = streamInputNode.dynamicModel->connectionInfo.talkerStream.streamIndex;
							talker.addStreamOutputConnection(talkerStreamIndex, { entityID, streamIndex });
							notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputConnectionsChanged, this, &talker, talkerStreamIndex, talker.getStreamOutputConnections(talkerStreamIndex));
						}
					}
				}
			}
		}
		catch (...)
		{
			AVDECC_ASSERT(false, "Unexpected exception");
		}
	}
}

void ControllerImpl::onPreUnadvertiseEntity(ControlledEntityImpl& controlledEntity) noexcept
{
	auto const& e = controlledEntity.getEntity();
	auto const entityID = e.getEntityID();

	// For a Listener, we want to inform all the talkers we are connected to, that we left
	if (e.getListenerCapabilities().test(entity::ListenerCapability::Implemented) && e.getEntityCapabilities().test(entity::EntityCapability::AemSupported))
	{
		AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

		try
		{
			auto const& configurationNode = controlledEntity.getCurrentConfigurationNode();

			for (auto const& [streamIndex, streamInputNode] : configurationNode.streamInputs)
			{
				if (streamInputNode.dynamicModel)
				{
					// If the Stream is Connected, search for the Talker we are connected to
					if (streamInputNode.dynamicModel->connectionInfo.state == entity::model::StreamInputConnectionInfo::State::Connected)
					{
						// Take a "scoped locked" shared copy of the ControlledEntity
						auto talkerEntity = getControlledEntityImplGuard(streamInputNode.dynamicModel->connectionInfo.talkerStream.entityID, true);

						if (talkerEntity)
						{
							auto& talker = *talkerEntity;
							auto const talkerStreamIndex = streamInputNode.dynamicModel->connectionInfo.talkerStream.streamIndex;
							talker.delStreamOutputConnection(talkerStreamIndex, { entityID, streamIndex });
							notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputConnectionsChanged, this, &talker, talkerStreamIndex, talker.getStreamOutputConnections(talkerStreamIndex));
						}
					}
				}
			}
		}
		catch (...)
		{
			AVDECC_ASSERT(false, "Unexpected exception");
		}
	}
}

ControllerImpl::FailureAction ControllerImpl::getFailureActionForMvuCommandStatus(entity::ControllerEntity::MvuCommandStatus const status) const noexcept
{
	switch (status)
	{
		// Query timed out
		case entity::ControllerEntity::MvuCommandStatus::TimedOut:
		{
			return FailureAction::TimedOut;
		}

		// Cases we want to flag as error (should not have happened, we have a possible non certified entity) but continue enumeration
		case entity::ControllerEntity::MvuCommandStatus::ProtocolError:
		{
			return FailureAction::ErrorContinue;
		}

		// Case inbetween NotSupported and actual device error that shoud not happen
		case entity::ControllerEntity::MvuCommandStatus::BadArguments:
		{
			return FailureAction::BadArguments;
		}

		// Cases the caller should decide whether to continue enumeration or not
		case entity::ControllerEntity::MvuCommandStatus::NotImplemented:
		{
			return FailureAction::NotSupported;
		}

		// Cases that are errors and we want to discard this entity
		case entity::ControllerEntity::MvuCommandStatus::UnknownEntity:
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

		// Case inbetween NotSupported and actual device error that shoud not happen
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

		// Cases that are errors and we want to discard this entity
		case entity::ControllerEntity::AemCommandStatus::UnknownEntity:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::EntityMisbehaving:
			[[fallthrough]];
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

/* This method handles non-success AemCommandStatus returned while trying to RegisterUnsolicitedNotifications */
bool ControllerImpl::processRegisterUnsolFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity) noexcept
{
	auto const action = getFailureActionForAemCommandStatus(status);
	switch (action)
	{
		case FailureAction::BadArguments:
			[[fallthrough]];
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::IEEE17221);
			return true;
		case FailureAction::NotAuthenticated:
			AVDECC_ASSERT(false, "TODO: Handle authentication properly (https://github.com/L-Acoustics/avdecc/issues/49)");
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::NotSupported:
			// Remove "Milan compatibility" as device does not support mandatory command
			if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
			{
				LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Milan mandatory command not supported by the entity: REGISTER_UNSOLICITED_NOTIFICATION");
				removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::Milan);
			}
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
#ifdef __cpp_structured_bindings
			auto const [shouldRetry, retryTimer] = entity->getRegisterUnsolRetryTimer();
#else // !__cpp_structured_bindings
			auto const result = entity->getRegisterUnsolRetryTimer();
			auto const shouldRetry = std::get<0>(result);
			auto const retryTimer = std::get<1>(result);
#endif // __cpp_structured_bindings
			if (shouldRetry)
			{
				registerUnsol(entity);
			}
			else
			{
				// Too many retries, result depends on FailureAction and AemCommandStatus
				if (action == FailureAction::TimedOut)
				{
					// Remove "Milan compatibility" as device does not respond to mandatory command
					if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
					{
						LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Too many timeouts for Milan mandatory command: REGISTER_UNSOLICITED_NOTIFICATION");
						removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::Milan);
					}
				}
				else if (action == FailureAction::Busy)
				{
					switch (status)
					{
						case entity::ControllerEntity::AemCommandStatus::LockedByOther: // Should not happen for a read operation but some devices are bugged
							[[fallthrough]];
						case entity::ControllerEntity::AemCommandStatus::AcquiredByOther: // Should not happen for a read operation but some devices are bugged
						{
							LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Too many unexpected errors for AEM command: REGISTER_UNSOLICITED_NOTIFICATION ({})", entity::LocalEntity::statusToString(status));
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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

/* This method handles non-success AemCommandStatus returned while getting EnumerationSteps::GetMilanModel (MVU) */
bool ControllerImpl::processGetMilanModelFailureStatus(entity::ControllerEntity::MvuCommandStatus const status, ControlledEntityImpl* const entity, ControlledEntityImpl::MilanInfoType const milanInfoType, bool const optionalForMilan) noexcept
{
	auto const action = getFailureActionForMvuCommandStatus(status);
	switch (action)
	{
		case FailureAction::BadArguments:
			[[fallthrough]];
		case FailureAction::ErrorContinue:
			// Remove "Milan compatibility" as device does not properly implement mandatory MVU
			if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
			{
				LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Milan mandatory MVU command not properly implemented by the entity");
				removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::Milan);
			}
			return true;
		case FailureAction::NotAuthenticated:
			AVDECC_ASSERT(false, "TODO: Handle authentication properly (https://github.com/L-Acoustics/avdecc/issues/49)");
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::NotSupported:
			if (!optionalForMilan)
			{
				// Remove "Milan compatibility" as device does not support mandatory MVU
				if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
				{
					LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Milan mandatory MVU command not supported by the entity");
					removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::Milan);
				}
			}
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
#ifdef __cpp_structured_bindings
			auto const [shouldRetry, retryTimer] = entity->getQueryMilanInfoRetryTimer();
#else // !__cpp_structured_bindings
			auto const result = entity->getQueryMilanInfoRetryTimer();
			auto const shouldRetry = std::get<0>(result);
			auto const retryTimer = std::get<1>(result);
#endif // __cpp_structured_bindings
			if (shouldRetry)
			{
				queryInformation(entity, milanInfoType, retryTimer);
			}
			else
			{
				// Too many retries, result depends on FailureAction and MvuCommandStatus
				if (action == FailureAction::TimedOut)
				{
					if (!optionalForMilan)
					{
						// Remove "Milan compatibility" as device does not respond to mandatory command
						if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
						{
							LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Too many timeouts for Milan mandatory MVU command");
							removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::Milan);
						}
					}
				}
				else if (action == FailureAction::Busy)
				{
					LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Too many unexpected errors for AEM command: REGISTER_UNSOLICITED_NOTIFICATION ({})", entity::LocalEntity::statusToString(status));
					// Flag the entity as "Not fully IEEE1722.1 compliant"
					removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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

/* This method handles non-success AemCommandStatus returned while getting EnumerationSteps::GetStaticModel (AEM) */
bool ControllerImpl::processGetStaticModelFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	auto const action = getFailureActionForAemCommandStatus(status);
	switch (action)
	{
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::IEEE17221);
			return true;
		case FailureAction::NotAuthenticated:
			AVDECC_ASSERT(false, "TODO: Handle authentication properly (https://github.com/L-Acoustics/avdecc/issues/49)");
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::BadArguments: // Getting the static model of an entity is not mandatory in 1722.1, thus we can ignore a BadArguments status
			[[fallthrough]];
		case FailureAction::NotSupported:
			// Remove "Milan compatibility" as device does not support mandatory descriptor
			if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
			{
				LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Milan mandatory descriptor not supported by the entity: {}", entity::model::descriptorTypeToString(descriptorType));
				removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::Milan);
			}
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
#ifdef __cpp_structured_bindings
			auto const [shouldRetry, retryTimer] = entity->getQueryDescriptorRetryTimer();
#else // !__cpp_structured_bindings
			auto const result = entity->getQueryDescriptorRetryTimer();
			auto const shouldRetry = std::get<0>(result);
			auto const retryTimer = std::get<1>(result);
#endif // __cpp_structured_bindings
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
						LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Too many timeouts for Milan mandatory descriptor: {}", entity::model::descriptorTypeToString(descriptorType));
						removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::Milan);
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
							LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Too many unexpected errors for READ_DESCRIPTOR on {}: {}", entity::model::descriptorTypeToString(descriptorType), entity::LocalEntity::statusToString(status));
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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

/* This method handles non-success AemCommandStatus returned while getting EnumerationSteps::GetDynamicInfo for AECP commands */
bool ControllerImpl::processGetAecpDynamicInfoFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex, bool const optionalForMilan) noexcept
{
	auto const action = getFailureActionForAemCommandStatus(status);
	switch (action)
	{
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::IEEE17221);
			return true;
		case FailureAction::NotAuthenticated:
			AVDECC_ASSERT(false, "TODO: Handle authentication properly (https://github.com/L-Acoustics/avdecc/issues/49)");
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::BadArguments: // Getting the AECP dynamic info of an entity is not mandatory in 1722.1, thus we can ignore a BadArguments status
			[[fallthrough]];
		case FailureAction::NotSupported:
			if (!optionalForMilan)
			{
				// Remove "Milan compatibility" as device does not support mandatory descriptor
				if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
				{
					LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Milan mandatory dynamic info not supported by the entity: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
					removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::Milan);
				}
			}
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
#ifdef __cpp_structured_bindings
			auto const [shouldRetry, retryTimer] = entity->getQueryDynamicInfoRetryTimer();
#else // !__cpp_structured_bindings
			auto const result = entity->getQueryDynamicInfoRetryTimer();
			auto const shouldRetry = std::get<0>(result);
			auto const retryTimer = std::get<1>(result);
#endif // __cpp_structured_bindings
			if (shouldRetry)
			{
				queryInformation(entity, configurationIndex, dynamicInfoType, descriptorIndex, subIndex, retryTimer);
			}
			else
			{
				// Too many retries, result depends on FailureAction and AemCommandStatus
				if (action == FailureAction::TimedOut)
				{
					if (!optionalForMilan)
					{
						// Remove "Milan compatibility" as device does not respond to mandatory command
						if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
						{
							LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Too many timeouts for Milan mandatory dynamic info: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
							removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::Milan);
						}
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
							LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Too many unexpected errors for dynamic info query {}: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType), entity::LocalEntity::statusToString(status));
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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

/* This method handles non-success ControlStatus returned while getting EnumerationSteps::GetDynamicInfo for ACMP commands */
bool ControllerImpl::processGetAcmpDynamicInfoFailureStatus(entity::ControllerEntity::ControlStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, bool const optionalForMilan) noexcept
{
	auto const action = getFailureActionForControlStatus(status);
	switch (action)
	{
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::IEEE17221);
			return true;
		case FailureAction::NotAuthenticated:
			AVDECC_ASSERT(false, "TODO: Handle authentication properly (https://github.com/L-Acoustics/avdecc/issues/49)");
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::NotSupported:
			if (!optionalForMilan)
			{
				// Remove "Milan compatibility" as device does not support mandatory descriptor
				if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
				{
					LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Milan mandatory ACMP command not supported by the entity: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
					removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::Milan);
				}
			}
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
#ifdef __cpp_structured_bindings
			auto const [shouldRetry, retryTimer] = entity->getQueryDynamicInfoRetryTimer();
#else // !__cpp_structured_bindings
			auto const result = entity->getQueryDynamicInfoRetryTimer();
			auto const shouldRetry = std::get<0>(result);
			auto const retryTimer = std::get<1>(result);
#endif // __cpp_structured_bindings
			if (shouldRetry)
			{
				queryInformation(entity, configurationIndex, dynamicInfoType, descriptorIndex, 0u, retryTimer);
			}
			else
			{
				// Too many retries, result depends on FailureAction and ControlStatus
				if (action == FailureAction::TimedOut)
				{
					if (!optionalForMilan)
					{
						// Remove "Milan compatibility" as device does not respond to mandatory command
						if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
						{
							LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Too many timeouts for Milan mandatory ACMP command: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
							removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::Milan);
						}
					}
				}
				else if (action == FailureAction::Busy)
				{
					switch (status)
					{
						case entity::ControllerEntity::ControlStatus::StateUnavailable:
							[[fallthrough]];
						case entity::ControllerEntity::ControlStatus::CouldNotSendMessage:
						{
							LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Too many unexpected errors for ACMP command {}: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType), entity::LocalEntity::statusToString(status));
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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

/* This method handles non-success ControlStatus returned while getting EnumerationSteps::GetDynamicInfo for ACMP commands with a connection index */
bool ControllerImpl::processGetAcmpDynamicInfoFailureStatus(entity::ControllerEntity::ControlStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::StreamIdentification const& talkerStream, std::uint16_t const subIndex, bool const optionalForMilan) noexcept
{
	auto const action = getFailureActionForControlStatus(status);
	switch (action)
	{
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::IEEE17221);
			return true;
		case FailureAction::NotAuthenticated:
			AVDECC_ASSERT(false, "TODO: Handle authentication properly (https://github.com/L-Acoustics/avdecc/issues/49)");
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::NotSupported:
			if (!optionalForMilan)
			{
				// Remove "Milan compatibility" as device does not support mandatory descriptor
				if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
				{
					LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Milan mandatory ACMP command not supported by the entity: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
					removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::Milan);
				}
			}
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
#ifdef __cpp_structured_bindings
			auto const [shouldRetry, retryTimer] = entity->getQueryDynamicInfoRetryTimer();
#else // !__cpp_structured_bindings
			auto const result = entity->getQueryDynamicInfoRetryTimer();
			auto const shouldRetry = std::get<0>(result);
			auto const retryTimer = std::get<1>(result);
#endif // __cpp_structured_bindings
			if (shouldRetry)
			{
				queryInformation(entity, configurationIndex, dynamicInfoType, talkerStream, subIndex, retryTimer);
			}
			else
			{
				// Too many retries, result depends on FailureAction and ControlStatus
				if (action == FailureAction::TimedOut)
				{
					if (!optionalForMilan)
					{
						// Remove "Milan compatibility" as device does not respond to mandatory command
						if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
						{
							LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Too many timeouts for Milan mandatory ACMP command: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
							removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::Milan);
						}
					}
				}
				else if (action == FailureAction::Busy)
				{
					switch (status)
					{
						case entity::ControllerEntity::ControlStatus::StateUnavailable:
							[[fallthrough]];
						case entity::ControllerEntity::ControlStatus::CouldNotSendMessage:
						{
							LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Too many unexpected errors for ACMP command {}: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType), entity::LocalEntity::statusToString(status));
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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

/* This method handles non-success AemCommandStatus returned while getting EnumerationSteps::GetDescriptorDynamicInfo (AEM) */
bool ControllerImpl::processGetDescriptorDynamicInfoFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, bool const optionalForMilan) noexcept
{
	auto const action = getFailureActionForAemCommandStatus(status);
	switch (action)
	{
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::IEEE17221);
			[[fallthrough]];
		case FailureAction::NotAuthenticated:
			AVDECC_ASSERT(false, "TODO: Handle authentication properly (https://github.com/L-Acoustics/avdecc/issues/49)");
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::TimedOut:
			[[fallthrough]];
		case FailureAction::Busy:
		{
#ifdef __cpp_structured_bindings
			auto const [shouldRetry, retryTimer] = entity->getQueryDescriptorDynamicInfoRetryTimer();
#else // !__cpp_structured_bindings
			auto const result = entity->getQueryDescriptorDynamicInfoRetryTimer();
			auto const shouldRetry = std::get<0>(result);
			auto const retryTimer = std::get<1>(result);
#endif // __cpp_structured_bindings
			if (shouldRetry)
			{
				queryInformation(entity, configurationIndex, descriptorDynamicInfoType, descriptorIndex, retryTimer);
				return true;
			}
			[[fallthrough]];
		}
		case FailureAction::NotSupported:
		{
			if (action == FailureAction::NotSupported && !optionalForMilan)
			{
				// Remove "Milan compatibility" as device does not support mandatory command
				if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
				{
					LOG_CONTROLLER_WARN(entity->getEntity().getEntityID(), "Milan mandatory AECP command not supported by the entity: {}", ControlledEntityImpl::descriptorDynamicInfoTypeToString(descriptorDynamicInfoType));
					removeCompatibilityFlag(*entity, ControlledEntity::CompatibilityFlag::Milan);
				}
			}

			// Failed to retrieve single DescriptorDynamicInformation, retrieve the corresponding descriptor instead if possible, otherwise switch back to full StaticModel enumeration
			auto const success = fetchCorrespondingDescriptor(entity, configurationIndex, descriptorDynamicInfoType, descriptorIndex);

			// Fallback to full StaticModel enumeration
			if (!success)
			{
				entity->setIgnoreCachedEntityModel();
				entity->clearAllExpectedDescriptorDynamicInfo();
				entity->addEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				LOG_CONTROLLER_ERROR(entity->getEntity().getEntityID(), "Failed to use cached EntityModel (too many DescriptorDynamic query retries), falling back to full StaticModel enumeration");
			}
			return true;
		}
		case FailureAction::ErrorFatal:
			return false;
		default:
			return false;
	}
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
		case ControlledEntityImpl::DescriptorDynamicInfoType::AvbInterfaceName:
			descriptorType = entity::model::DescriptorType::AvbInterface;
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::ClockSourceName:
			descriptorType = entity::model::DescriptorType::ClockSource;
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
		default:
			AVDECC_ASSERT(false, "Unhandled DescriptorDynamicInfoType");
			break;
	}

	if (!!descriptorType)
	{
		LOG_CONTROLLER_DEBUG(entity->getEntity().getEntityID(), "Failed to get DescriptorDynamicInfo, trying to get the corresponding Descriptor");
		queryInformation(entity, configurationIndex, descriptorType, descriptorIndex);
		return true;
	}

	return false;
}

void ControllerImpl::handleListenerStreamStateNotification(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, bool const isConnected, entity::ConnectionFlags const flags, bool const changedByOther) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Build StreamConnectionState::State
	auto conState{ entity::model::StreamInputConnectionInfo::State::NotConnected };
	if (isConnected)
	{
		conState = entity::model::StreamInputConnectionInfo::State::Connected;
	}
	else if (flags.test(entity::ConnectionFlag::FastConnect))
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
				LOG_CONTROLLER_WARN(UniqueIdentifier::getNullUniqueIdentifier(), "Listener entity {} sent an invalid CONNECTION STATE (with status=SUCCESS) for StreamIndex={} although it only has {} sinks", utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex, maxSinks);
				// Flag the entity as "Misbehaving"
				addCompatibilityFlag(*listenerEntity, ControlledEntity::CompatibilityFlag::Misbehaving);
				return;
			}
			auto const previousInfo = listenerEntity->setStreamInputConnectionInformation(listenerStream.streamIndex, info);

			// Entity was advertised to the user, notify observers
			if (listenerEntity->wasAdvertised() && previousInfo != info)
			{
				auto& listener = *listenerEntity;
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputConnectionChanged, this, &listener, listenerStream.streamIndex, info, changedByOther);

				// If the Listener was already advertised, check if talker StreamIdentification changed (no need to do it during listener enumeration, the connections to the talker will be updated when the listener is ready to advertise)
				if (previousInfo.talkerStream != info.talkerStream)
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
				shouldNotify = talkerEntity->addStreamOutputConnection(talkerStream.streamIndex, listenerStream);
			}
			else
			{
				shouldNotify = talkerEntity->delStreamOutputConnection(talkerStream.streamIndex, listenerStream);
			}
			if (shouldNotify)
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputConnectionsChanged, this, talkerEntity.get(), talkerStream.streamIndex, talkerEntity->getStreamOutputConnections(talkerStream.streamIndex));
			}
		}
	}
}

void ControllerImpl::clearTalkerStreamConnections(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex) const noexcept
{
	talkerEntity->clearStreamOutputConnections(talkerStreamIndex);
}

void ControllerImpl::addTalkerStreamConnection(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIdentification const& listenerStream) const noexcept
{
	// Update our internal cache
	talkerEntity->addStreamOutputConnection(talkerStreamIndex, listenerStream);
}

#ifdef ENABLE_AVDECC_FEATURE_JSON
ControllerImpl::SharedControlledEntityImpl ControllerImpl::createControlledEntityFromJson(json const& object, entity::model::jsonSerializer::Flags const flags)
{
	try
	{
		// Read information of the dump itself
		auto const dumpVersion = object.at(jsonSerializer::keyName::ControlledEntity_DumpVersion).get<decltype(jsonSerializer::keyValue::ControlledEntity_DumpVersion)>();
		if (dumpVersion != jsonSerializer::keyValue::ControlledEntity_DumpVersion)
		{
			throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::UnsupportedDumpVersion, std::string("Unsupported dump version: ") + std::to_string(dumpVersion) };
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
				auto const jIndex = j.at(entity::keyName::Entity_InterfaceInformation_AvbInterfaceIndex);
				auto const avbInterfaceIndex = jIndex.is_null() ? entity::Entity::GlobalAvbInterfaceIndex : jIndex.get<entity::model::AvbInterfaceIndex>();
				intfcsInfo.insert(std::make_pair(avbInterfaceIndex, j.get<entity::Entity::InterfaceInformation>()));
			}
		}

		auto controlledEntity = std::make_shared<ControlledEntityImpl>(entity::Entity{ commonInfo, intfcsInfo }, _entitiesSharedLockInformation, true);
		auto& entity = *controlledEntity;

		// Start Enumeration timer
		entity.setStartEnumerationTime(std::chrono::steady_clock::now());

		// Read device compatibility flags
		if (flags.test(entity::model::jsonSerializer::Flag::ProcessCompatibility))
		{
			entity.setCompatibilityFlags(object.at(jsonSerializer::keyName::ControlledEntity_CompatibilityFlags).get<ControlledEntity::CompatibilityFlags>());
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
#endif // ENABLE_AVDECC_FEATURE_JSON

} // namespace controller
} // namespace avdecc
} // namespace la
