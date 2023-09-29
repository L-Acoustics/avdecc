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
#include <la/avdecc/internals/protocolInterface.hpp>
#include <la/avdecc/executor.hpp>

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
			}
			break;
		case ControlledEntity::CompatibilityFlag::MilanWarning:
			if (AVDECC_ASSERT_WITH_RET(newFlags.test(ControlledEntity::CompatibilityFlag::Milan), "Adding MilanWarning flag for a non Milan device"))
			{
				newFlags.set(flag);
			}
			break;
		case ControlledEntity::CompatibilityFlag::Misbehaving:
			newFlags.reset(ControlledEntity::CompatibilityFlag::IEEE17221); // A misbehaving device is not IEEE1722.1 compatible
			newFlags.reset(ControlledEntity::CompatibilityFlag::Milan); // A misbehaving is not Milan compatible
			if (!newFlags.test(ControlledEntity::CompatibilityFlag::Misbehaving))
			{
				newFlags.set(flag);
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Entity is sending incoherent values (misbehaving)");
			}
			break;
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
				controller->notifyObserversMethod<Controller::Observer>(&Controller::Observer::onCompatibilityFlagsChanged, controller, &controlledEntity, newFlags);
			}
		}
	}
}

void ControllerImpl::removeCompatibilityFlag(ControllerImpl const* const controller, ControlledEntityImpl& controlledEntity, ControlledEntity::CompatibilityFlag const flag) noexcept
{
	auto const oldFlags = controlledEntity.getCompatibilityFlags();
	auto newFlags = oldFlags;

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

	if (oldFlags != newFlags)
	{
		controlledEntity.setCompatibilityFlags(newFlags);

		if (controller)
		{
			AVDECC_ASSERT(controller->_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");
			// Entity was advertised to the user, notify observers
			if (controlledEntity.wasAdvertised())
			{
				controller->notifyObserversMethod<Controller::Observer>(&Controller::Observer::onCompatibilityFlagsChanged, controller, &controlledEntity, newFlags);
			}
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

void ControllerImpl::updateConfiguration(entity::controller::Interface const* const controller, ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	controlledEntity.setCurrentConfiguration(configurationIndex, notFoundBehavior);

	// Right now, simulate the entity going offline then online again - TODO: Handle multiple configurations, see https://github.com/L-Acoustics/avdecc/issues/3
	auto const e = static_cast<entity::Entity>(controlledEntity.getEntity()); // Make a copy of the Entity object since it will be destroyed during onEntityOffline (use static_cast to prevent warning with 'auto' not being a reference)
	auto const entityID = e.getEntityID();
	auto* self = const_cast<ControllerImpl*>(this);
	self->onEntityOffline(controller, entityID);
	self->onEntityOnline(controller, entityID, e);
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
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "StreamFormatValid bit not set in GET_STREAM_INFO response");
			removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
			// Check if we have something that looks like a valid streamFormat in the field
			auto const formatType = entity::model::StreamFormatInfo::create(info.streamFormat)->getType();
			if (formatType != entity::model::StreamFormatInfo::Type::None && formatType != entity::model::StreamFormatInfo::Type::Unsupported)
			{
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "StreamFormatValid bit not set but stream_format field appears to contain a valid value in GET_STREAM_INFO response");
			}
		}
		// Or Invalid StreamFormat
		else if (!info.streamFormat)
		{
			hasStreamFormat = false;
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "StreamFormatValid bit set but invalid stream_format field in GET_STREAM_INFO response");
			removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
		}
	}
	// If Milan Extended Information is required (for GetStreamInfo, not SetStreamInfo) and entity is Milan compatible, check if it's present
	if (milanExtendedRequired && controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
	{
		if (!info.streamInfoFlagsEx || !info.probingStatus || !info.acmpStatus)
		{
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Milan mandatory extended GetStreamInfo not found");
			removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
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
	{
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

			// Check for Diagnostics - Latency Error
			{
				// Only if the entity has been advertised, onPreAdvertiseEntity will take care of the non-advertised ones later
				if (controlledEntity.wasAdvertised())
				{
					auto isOverLatency = false;

					// Only if Latency is greater than 0
					if (info.msrpAccumulatedLatency > 0)
					{
						auto const& sink = controlledEntity.getSinkConnectionInformation(streamIndex);

						// If the Stream is Connected, search for the Talker we are connected to
						if (sink.state == entity::model::StreamInputConnectionInfo::State::Connected)
						{
							// Take a "scoped locked" shared copy of the ControlledEntity
							auto talkerEntity = getControlledEntityImplGuard(sink.talkerStream.entityID, true);

							if (talkerEntity)
							{
								auto const& talker = *talkerEntity;

								// Only process advertised entities, onPreAdvertiseEntity will take care of the non-advertised ones later
								if (talker.wasAdvertised())
								{
									try
									{
										auto const& talkerStreamOutputNode = talker.getStreamOutputNode(talker.getCurrentConfigurationIndex(), sink.talkerStream.streamIndex);
										if (talkerStreamOutputNode.dynamicModel.streamDynamicInfo)
										{
											isOverLatency = info.msrpAccumulatedLatency > (*talkerStreamOutputNode.dynamicModel.streamDynamicInfo).msrpAccumulatedLatency;
										}
									}
									catch (ControlledEntity::Exception const&)
									{
										// Ignore Exception
									}
								}
							}
						}
					}

					updateStreamInputLatency(controlledEntity, streamIndex, isOverLatency);
				}
			}
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
		auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(notFoundBehavior);
		if (currentConfigurationIndexOpt)
		{
			auto* const streamDynamicModel = controlledEntity.getModelAccessStrategy().getStreamInputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
			if (streamDynamicModel)
			{
				streamDynamicModel->streamDynamicInfo = std::move(dynamicInfo);

				// Entity was advertised to the user, notify observers
				if (controlledEntity.wasAdvertised())
				{
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputDynamicInfoChanged, this, &controlledEntity, streamIndex, *streamDynamicModel->streamDynamicInfo);
				}
			}
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
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "StreamFormatValid bit not set in GET_STREAM_INFO response");
			removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
			// Check if we have something that looks like a valid streamFormat in the field
			auto const formatType = entity::model::StreamFormatInfo::create(info.streamFormat)->getType();
			if (formatType != entity::model::StreamFormatInfo::Type::None && formatType != entity::model::StreamFormatInfo::Type::Unsupported)
			{
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "StreamFormatValid bit not set but stream_format field appears to contain a valid value in GET_STREAM_INFO response");
			}
		}
		// Or Invalid StreamFormat
		else if (!info.streamFormat)
		{
			hasStreamFormat = false;
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "StreamFormatValid bit set but invalid stream_format field in GET_STREAM_INFO response");
			removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
		}
	}
	// If Milan Extended Information is required (for GetStreamInfo, not SetStreamInfo) and entity is Milan compatible, check if it's present
	if (milanExtendedRequired && controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
	{
		if (!info.streamInfoFlagsEx || !info.probingStatus || !info.acmpStatus)
		{
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Milan mandatory extended GetStreamInfo not found");
			removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
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
	{
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
		auto const currentConfigurationIndexOpt = controlledEntity.getCurrentConfigurationIndex(notFoundBehavior);
		if (currentConfigurationIndexOpt)
		{
			auto* const streamDynamicModel = controlledEntity.getModelAccessStrategy().getStreamOutputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
			if (streamDynamicModel)
			{
				streamDynamicModel->streamDynamicInfo = std::move(dynamicInfo);

				// Entity was advertised to the user, notify observers
				if (controlledEntity.wasAdvertised())
				{
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputDynamicInfoChanged, this, &controlledEntity, streamIndex, *streamDynamicModel->streamDynamicInfo);
				}
			}
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
		LOG_CONTROLLER_WARN(entity.getEntityID(), "Entity changed its ASSOCIATION_ID but it said ASSOCIATION_ID_NOT_SUPPORTED in ADPDU");
		removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
			auto const validationResult = validateControlValues(controlledEntity.getEntity().getEntityID(), controlIndex, controlType, controlValueType, controlStaticModel->values, controlValues);
			auto isOutOfBounds = false;
			switch (validationResult)
			{
				case DynamicControlValuesValidationResult::InvalidValues:
					// Flag the entity as "Not fully IEEE1722.1 compliant"
					removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
					break;
				case DynamicControlValuesValidationResult::CurrentValueOutOfRange:
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
					if (interfaceIndex == avbInterfaceIndex || (avbInterfaceIndex == entity::Entity::GlobalAvbInterfaceIndex && macAddress == avbInterfaceNode.staticModel.macAddress))
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
		auto const* const avbInterfaceStaticModel = controlledEntity.getModelAccessStrategy().getAvbInterfaceNodeStaticModel(*currentConfigurationIndexOpt, avbInterfaceIndex, notFoundBehavior);
		if (avbInterfaceStaticModel)
		{
			updateGptpInformation(controlledEntity, avbInterfaceIndex, avbInterfaceStaticModel->macAddress, info.gptpGrandmasterID, info.gptpDomainNumber, notFoundBehavior);
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
			// LinkDown should either be equal to LinkUp or be one more (Milan-2019 Clause 6.6.3)
			// We are safe to get those counters, check for their presence during first enumeration has already been done
			auto const upValue = counters[validCounters.getPosition(entity::AvbInterfaceCounterValidFlag::LinkUp)];
			auto const downValue = counters[validCounters.getPosition(entity::AvbInterfaceCounterValidFlag::LinkDown)];
			if (upValue != downValue && upValue != (downValue + 1))
			{
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Invalid LINK_UP / LINK_DOWN counters value on AVB_INTERFACE:{} ({} / {})", avbInterfaceIndex, upValue, downValue);
				removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
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
			// Unlocked should either be equal to Locked or be one more (Milan-2019 Clause 6.11.2)
			// We are safe to get those counters, check for their presence during first enumeration has already been done
			auto const lockedValue = (*clockDomainCounters)[entity::ClockDomainCounterValidFlag::Locked];
			auto const unlockedValue = (*clockDomainCounters)[entity::ClockDomainCounterValidFlag::Unlocked];
			if (lockedValue != unlockedValue && lockedValue != (unlockedValue + 1))
			{
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Invalid LOCKED / UNLOCKED counters value on CLOCK_DOMAIN:{} ({} / {})", clockDomainIndex, lockedValue, unlockedValue);
				removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
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
			// MediaUnlocked should either be equal to MediaLocked or be one more (Milan-2019 Clause 6.8.10)
			// We are safe to get those counters, check for their presence during first enumeration has already been done
			auto const lockedValue = (*streamCounters)[entity::StreamInputCounterValidFlag::MediaLocked];
			auto const unlockedValue = (*streamCounters)[entity::StreamInputCounterValidFlag::MediaUnlocked];
			if (lockedValue != unlockedValue && lockedValue != (unlockedValue + 1))
			{
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Invalid MEDIA_LOCKED / MEDIA_UNLOCKED counters value on STREAM_INPUT:{} ({} / {})", streamIndex, lockedValue, unlockedValue);
				removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
			}
		}

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputCountersChanged, this, &controlledEntity, streamIndex, *streamCounters);
		}
	}
}

void ControllerImpl::updateStreamOutputCounters(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::StreamOutputCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior) const noexcept
{
	AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

	// Get previous counters
	auto* const streamCounters = controlledEntity.getStreamOutputCounters(streamIndex, notFoundBehavior);
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
			// StreamStop should either be equal to StreamStart or be one more (Milan-2019 Clause 6.7.7)
			// We are safe to get those counters, check for their presence during first enumeration has already been done
			auto const startValue = (*streamCounters)[entity::StreamOutputCounterValidFlag::StreamStart];
			auto const stopValue = (*streamCounters)[entity::StreamOutputCounterValidFlag::StreamStop];
			if (startValue != stopValue && startValue != (stopValue + 1))
			{
				LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Invalid STREAM_START / STREAM_STOP counters value on STREAM_OUTPUT:{} ({} / {})", streamIndex, startValue, stopValue);
				removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
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
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Invalid MemoryObject.length value (greater than MemoryObject.maximumLength value): {} > {}", length, memoryObjectNode->staticModel.maximumLength);
			ControllerImpl::removeCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
	auto& entity = controlledEntity.getEntity();
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
			LOG_CONTROLLER_WARN(entity.getEntityID(), "Entity is declared Milan Redundant but does not have AVB_INTERFACE_0 and AVB_INTERFACE_1");
			addCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::MilanWarning);
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
				LOG_CONTROLLER_WARN(entity.getEntity().getEntityID(), "Milan must not implement ACQUIRE_ENTITY");
				removeCompatibilityFlag(this, entity, ControlledEntity::CompatibilityFlag::Milan);
			}
			break;
		case entity::ControllerEntity::AemCommandStatus::AcquiredByOther:
			acquireState = model::AcquireState::AcquiredByOther;
			owningController = owningEntity;
			// Remove "Milan compatibility" as device does support a forbidden command
			if (entity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
			{
				LOG_CONTROLLER_WARN(entity.getEntity().getEntityID(), "Milan device must not implement ACQUIRE_ENTITY");
				removeCompatibilityFlag(this, entity, ControlledEntity::CompatibilityFlag::Milan);
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
				removeCompatibilityFlag(this, entity, ControlledEntity::CompatibilityFlag::Milan);
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
		{
		}

		// Deleted compiler auto-generated methods
		DynamicInfoVisitor(DynamicInfoVisitor const&) = delete;
		DynamicInfoVisitor(DynamicInfoVisitor&&) = delete;
		DynamicInfoVisitor& operator=(DynamicInfoVisitor const&) = delete;
		DynamicInfoVisitor& operator=(DynamicInfoVisitor&&) = delete;

	private:
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
			_controller->queryInformation(_entity, 0u, ControlledEntityImpl::DynamicInfoType::GetEntityCounters, 0u);
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
			_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::GetStreamInputCounters, node.descriptorIndex);

			// RX_STATE
			_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamState, node.descriptorIndex);
		}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::StreamOutputNode const& node) noexcept override
		{
			// StreamInfo
			_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamInfo, node.descriptorIndex);

			// Counters
			_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::GetStreamOutputCounters, node.descriptorIndex);

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
			_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInterfaceCounters, node.descriptorIndex);
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
				_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, node.descriptorIndex);
			}
		}
		virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::AudioUnitNode const* const /*parent*/, model::StreamPortOutputNode const& node) noexcept override
		{
			if (node.staticModel.numberOfMaps == 0)
			{
				// AudioMappings
				// TODO: IEEE1722.1-2013 Clause 7.4.44.3 recommands to Lock or Acquire the entity before getting the dynamic audio map
				_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, node.descriptorIndex);
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
			_controller->queryInformation(_entity, _currentConfigurationIndex, ControlledEntityImpl::DynamicInfoType::GetClockDomainCounters, node.descriptorIndex);
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
	};

	// Visit all known descriptor and get associated dynamic information
	auto visitor = DynamicInfoVisitor{ this, entity };
	entity->accept(&visitor, false);

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
			{
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
				_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ControlName, controlIndex);
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
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ConfigurationName, 0u);


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
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitName, audioUnitIndex);
					// Get AudioUnitSamplingRate
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitSamplingRate, audioUnitIndex);
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
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamName, streamIndex);
					// Get InputStreamFormat
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamFormat, streamIndex);
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
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamName, streamIndex);
					// Get OutputStreamFormat
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamFormat, streamIndex);
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
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputJackName, jackIndex);
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
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputJackName, jackIndex);
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
				auto const configurationIndex = parent->descriptorIndex;
				auto const avbInterfaceIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					// Get AvbInterfaceName
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AvbInterfaceName, avbInterfaceIndex);
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ClockSourceNode const& node) noexcept override
			{
				auto const configurationIndex = parent->descriptorIndex;
				auto const clockSourceIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					// Get ClockSourceName
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockSourceName, clockSourceIndex);
				}
			}
			virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::MemoryObjectNode const& node) noexcept override
			{
				auto const configurationIndex = parent->descriptorIndex;
				auto const memoryObjectIndex = node.descriptorIndex;

				// Only for active configuration
				if (configurationIndex == _currentConfigurationIndex)
				{
					// Get MemoryObjectName
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectName, memoryObjectIndex);
					// Get MemoryObjectLength
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectLength, memoryObjectIndex);
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
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName, clusterIndex);
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
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainName, clockDomainIndex);
					// Get ClockDomainSourceIndex
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainSourceIndex, clockDomainIndex);
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
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::TimingName, timingIndex);
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
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::PtpInstanceName, ptpInstanceIndex);
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
					// Get ControlName
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ControlName, controlIndex);
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
					_controller->queryInformation(_entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::PtpPortName, ptpPortIndex);
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
		};

		// Visit the model, and retrieve dynamic info
		auto visitor = DynamicInfoModelVisitor{ this, entity };
		entity->accept(&visitor, true);
	}

	// Get all expected descriptor dynamic information
	if (entity->gotAllExpectedDescriptorDynamicInfo())
	{
		// Clear this enumeration step and check for next one
		entity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
		checkEnumerationSteps(entity);
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
					controlledEntity->accept(&visitor);

					// Store EntityModel in the cache for later usevisitor
					entityModelCache.cacheEntityModel(entityModelID, visitor.getModel());
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

entity::model::AudioMappings ControllerImpl::validateMappings(ControlledEntityImpl& controlledEntity, std::uint16_t const maxStreams, std::uint16_t const maxClusters, entity::model::AudioMappings const& mappings) const noexcept
{
	auto fixedMappings = std::decay_t<decltype(mappings)>{};

	for (auto const& mapping : mappings)
	{
		if (mapping.streamIndex >= maxStreams)
		{
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Invalid Mapping received: StreamIndex is greater than maximum declared streams in ADP ({} >= {})", mapping.streamIndex, maxStreams);
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Misbehaving);
			continue;
		}
		if (mapping.clusterOffset >= maxClusters)
		{
			LOG_CONTROLLER_WARN(controlledEntity.getEntity().getEntityID(), "Invalid Mapping received: ClusterOffset is greater than cluster in the StreamPort ({} >= {})", mapping.clusterOffset, maxClusters);
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, controlledEntity, ControlledEntity::CompatibilityFlag::Misbehaving);
			continue;
		}

		fixedMappings.push_back(mapping);
	}

	return fixedMappings;
}

bool ControllerImpl::validateIdentifyControl(ControlledEntityImpl& controlledEntity, model::ControlNode const& identifyControlNode) noexcept
{
	AVDECC_ASSERT(entity::model::StandardControlType::Identify == identifyControlNode.staticModel.controlType.getValue(), "validateIdentifyControl should only be called on an IDENTIFY Control Descriptor Type");
	auto const& e = controlledEntity.getEntity();
	auto const entityID = e.getEntityID();
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
								LOG_CONTROLLER_WARN(entityID, "ControlDescriptor at Index {} is not a valid Identify Control: SignalType should be set to INVALID and SignalIndex to 0", controlIndex);
								// Flag the entity as "Not fully IEEE1722.1 compliant"
								removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
							}

							// All (or almost) ok
							return true;
						}
						else
						{
							LOG_CONTROLLER_WARN(entityID, "ControlDescriptor at Index {} is not a valid Identify Control: CurrentValue should either be 0 or 255 but is {}", controlIndex, dynamicValue.currentValue);
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
						}
					}
					else
					{
						LOG_CONTROLLER_WARN(entityID, "ControlDescriptor at Index {} is not a valid Identify Control: Should only contain one value but has {}", controlIndex, dynamicValues.countValues());
						// Flag the entity as "Not fully IEEE1722.1 compliant"
						removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
					}
				}
				else
				{
					LOG_CONTROLLER_WARN(entityID, "ControlDescriptor at Index {} is not a valid Identify Control: One or many fields are incorrect and should be min=0, max=255, step=255, Unit=UNITLESS/0", controlIndex);
					// Flag the entity as "Not fully IEEE1722.1 compliant"
					removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
				}
			}
			else
			{
				LOG_CONTROLLER_WARN(entityID, "ControlDescriptor at Index {} is not a valid Identify Control: Should only contain one value but has {}", controlIndex, staticValues.countValues());
				// Flag the entity as "Not fully IEEE1722.1 compliant"
				removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
			}
		}
		else
		{
			LOG_CONTROLLER_WARN(entityID, "ControlDescriptor at Index {} is not a valid Identify Control: ControlValueType should be CONTROL_LINEAR_UINT8 but is {}", controlIndex, entity::model::controlValueTypeToString(controlValueType));
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
		}
	}
	catch (std::invalid_argument const& e)
	{
		LOG_CONTROLLER_WARN(entityID, "ControlDescriptor at Index {} is not a valid Identify Control: {}", controlIndex, e.what());
		// Flag the entity as "Not fully IEEE1722.1 compliant"
		removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
	}
	catch (...)
	{
		LOG_CONTROLLER_WARN(entityID, "ControlDescriptor at Index {} is not a valid Identify Control: Unknown exception trying to read descriptor", controlIndex);
		// Flag the entity as "Not fully IEEE1722.1 compliant"
		removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
	}

	return false;
}

ControllerImpl::DynamicControlValuesValidationResult ControllerImpl::validateControlValues(UniqueIdentifier const entityID, entity::model::ControlIndex const controlIndex, UniqueIdentifier const& controlType, entity::model::ControlValueType::Type const controlValueType, entity::model::ControlValues const& staticValues, entity::model::ControlValues const& dynamicValues) noexcept
{
	if (!staticValues)
	{
		// Returning true here because uninitialized values may be due to a type unknown to the library
		LOG_CONTROLLER_WARN(entityID, "StaticValues (type {}) for ControlDescriptor at Index {} are not initialized (probably unhandled type)", entity::model::controlValueTypeToString(controlValueType), controlIndex);
		return DynamicControlValuesValidationResult::Valid;
	}

	if (staticValues.areDynamicValues())
	{
		LOG_CONTROLLER_WARN(entityID, "StaticValues for ControlDescriptor at Index {} are dynamic instead of static", controlIndex);
		return DynamicControlValuesValidationResult::InvalidValues;
	}

	if (!dynamicValues)
	{
		// Returning true here because uninitialized values may be due to a type unknown to the library
		LOG_CONTROLLER_WARN(entityID, "DynamicValues (type {}) for ControlDescriptor at Index {} are not initialized (probably unhandled type)", entity::model::controlValueTypeToString(controlValueType), controlIndex);
		return DynamicControlValuesValidationResult::InvalidValues;
	}

	if (!dynamicValues.areDynamicValues())
	{
		LOG_CONTROLLER_WARN(entityID, "DynamicValues for ControlDescriptor at Index {} are static instead of dynamic", controlIndex);
		return DynamicControlValuesValidationResult::InvalidValues;
	}

	auto const [result, errMessage] = entity::model::validateControlValues(staticValues, dynamicValues);

	// No error during validation
	if (result == entity::model::ControlValuesValidationResult::Valid)
	{
		return DynamicControlValuesValidationResult::Valid;
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
					return DynamicControlValuesValidationResult::CurrentValueOutOfRange;
				default:
					// Also return CurrentValueOutOfRange for non-standard controls
					if (controlType.getVendorID() != entity::model::StandardControlTypeVendorID)
					{
						LOG_CONTROLLER_DEBUG(entityID, "Warning for DynamicValues for Non-Standard ControlDescriptor at Index {}: {}", controlIndex, errMessage);
						return DynamicControlValuesValidationResult::CurrentValueOutOfRange;
					}
					break;
			}
		}
		default:
			break;
	}

	LOG_CONTROLLER_WARN(entityID, "DynamicValues for ControlDescriptor at Index {} are not valid: {}", controlIndex, errMessage);
	return DynamicControlValuesValidationResult::InvalidValues;
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
					LOG_CONTROLLER_WARN(_entityID, "AEM_IDENTIFY_CONTROL_INDEX_VALID bit is set in ADP but ControlIndex is invalid: CONTROL index not defined in ADP");
					// Flag the entity as "Not fully IEEE1722.1 compliant"
					removeCompatibilityFlag(nullptr, _controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
				}
			}
		}

		void validate() const noexcept
		{
			// Check we found a valid Identify Control at either Configuration or Jack level, if ADP contains a valid Identify Control Index
			if (_adpIdentifyControlIndex && !_foundADPIdentifyControlIndex)
			{
				LOG_CONTROLLER_WARN(_entityID, "AEM_IDENTIFY_CONTROL_INDEX_VALID bit is set in ADP but ControlIndex is invalid: No valid CONTROL at index {}", *_adpIdentifyControlIndex);
				// Flag the entity as "Not fully IEEE1722.1 compliant"
				removeCompatibilityFlag(nullptr, _controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
					LOG_CONTROLLER_WARN(_entityID, "AEM_IDENTIFY_CONTROL_INDEX_VALID bit is set in ADP but ControlIndex is invalid: ControlType should be IDENTIFY but is {}", entity::model::controlTypeToString(node.staticModel.controlType));
					// Flag the entity as "Not fully IEEE1722.1 compliant"
					removeCompatibilityFlag(nullptr, _controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
						LOG_CONTROLLER_WARN(_entityID, "ControlDescriptor at Index {} is a valid Identify Control but it's neither at CONFIGURATION nor JACK level", controlIndex);
						// Flag the entity as "Not fully IEEE1722.1 compliant"
						removeCompatibilityFlag(nullptr, _controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
					}
				}
				// Note: No need to remove compatibility flag or log a warning in else statement, the validateIdentifyControl method already did it
			}

			// Validate ControlType
			auto const controlType = node.staticModel.controlType;
			if (!controlType.isValid())
			{
				LOG_CONTROLLER_WARN(_entityID, "control_type for CONTROL descriptor at index {} is not a valid EUI-64: {}", controlIndex, utils::toHexString(controlType));
				// Flag the entity as "Not fully IEEE1722.1 compliant"
				removeCompatibilityFlag(nullptr, _controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
			}

			// Validate ControlValues
			auto const validationResult = validateControlValues(_entityID, controlIndex, controlType, node.staticModel.controlValueType.getType(), node.staticModel.values, node.dynamicModel.values);
			auto isOutOfBounds = false;
			switch (validationResult)
			{
				case DynamicControlValuesValidationResult::InvalidValues:
					// Flag the entity as "Not fully IEEE1722.1 compliant"
					removeCompatibilityFlag(nullptr, _controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
					break;
				case DynamicControlValuesValidationResult::CurrentValueOutOfRange:
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
				auto const& e = controlledEntity.getEntity();
				auto const entityID = e.getEntityID();

				if (hasRedundantStream)
				{
					LOG_CONTROLLER_WARN(entityID, "Redundant Streams detected, but MilanInfo features_flags does not contain REDUNDANCY bit");
				}
				else
				{
					LOG_CONTROLLER_WARN(entityID, "MilanInfo features_flags contains REDUNDANCY bit, but active Configuration does not have a single valid Redundant Stream");
				}
				addCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::MilanWarning);
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
			LOG_CONTROLLER_WARN(entityID, "[IEEE1722.1-2013 Clause 7.2.1] A device is required to have at least one Configuration Descriptor");
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
					removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
				LOG_CONTROLLER_WARN(entityID, "[IEEE1722.1-2013 Clause 7.2] Invalid Descriptor Numbering: {}", e.what());
				// Flag the entity as "Not fully IEEE1722.1 compliant"
				removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
			auto const validateStreamFormats = [](auto const& entityID, auto& controlledEntity, auto const& streams)
			{
				auto aafCapableStreams = CapableStreams{};
				auto crfCapableStreams = CapableStreams{};

				for (auto const& [streamIndex, streamNode] : streams)
				{
					auto streamHasAaf = false;
					auto streamHasCrf = false;
					for (auto const& format : streamNode.staticModel.formats)
					{
						auto const f = entity::model::StreamFormatInfo::create(format);
						switch (f->getType())
						{
							case entity::model::StreamFormatInfo::Type::AAF:
								streamHasAaf = true;
								aafCapableStreams[streamIndex] = true;
								break;
							case entity::model::StreamFormatInfo::Type::ClockReference:
								streamHasCrf = true;
								crfCapableStreams[streamIndex] = true;
								break;
							default:
								break;
						}
					}
					// [Milan-2019 Clause 6.3.4] If a STREAM_INPUT/OUTPUT supports the Avnu Pro Audio CRF Media Clock Stream Format, it shall not support the Avnu Pro Audio AAF Audio Stream Format, and vice versa
					if (streamHasAaf && streamHasCrf)
					{
						LOG_CONTROLLER_WARN(entityID, "[Milan-2019 Clause 6.3.4] If a STREAM_INPUT/OUTPUT supports the Avnu Pro Audio CRF Media Clock Stream Format, it shall not support the Avnu Pro Audio AAF Audio Stream Format, and vice versa");
						// Remove "Milan compatibility"
						removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
					}
				}
				return std::make_pair(aafCapableStreams, crfCapableStreams);
			};

			auto const countCapableStreamsForDomain = [](auto const& streams, auto const& redundantStreams, auto const& capableStreams, auto const domainIndex)
			{
				auto countStreams = 0u;
				// Process non-redundant streams
				for (auto const& [streamIndex, streamNode] : streams)
				{
					if (!streamNode.isRedundant)
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

			// Milan devices AEM validation
			if (controlledEntity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
			{
				auto const& configurationNode = controlledEntity.getCurrentConfigurationNode();

				auto aafInputStreams = CapableStreams{};
				auto crfInputStreams = CapableStreams{};
				auto aafOutputStreams = CapableStreams{};
				auto crfOutputStreams = CapableStreams{};
				auto isAafMediaListener = false;
				auto isAafMediaTalker = false;
				// Validate stream formats
				if (!configurationNode.streamInputs.empty())
				{
					auto caps = validateStreamFormats(entityID, controlledEntity, configurationNode.streamInputs);
					aafInputStreams = std::move(caps.first);
					crfInputStreams = std::move(caps.second);
					isAafMediaListener = !aafInputStreams.empty();
				}
				if (!configurationNode.streamOutputs.empty())
				{
					auto caps = validateStreamFormats(entityID, controlledEntity, configurationNode.streamOutputs);
					aafOutputStreams = std::move(caps.first);
					crfOutputStreams = std::move(caps.second);
					isAafMediaTalker = !aafOutputStreams.empty();
				}

				// Validate AAF requirements
				// [Milan Formats] A PAAD-AE shall have at least one Configuration that contains at least one Stream which advertises support for a Base format in its list of supported formats
				if (!isAafMediaListener && !isAafMediaTalker)
				{
					LOG_CONTROLLER_WARN(entityID, "[Milan Formats] A PAAD-AE shall have at least one Configuration that contains at least one Stream which advertises support for a Base format in its list of supported formats");
					// Remove "Milan compatibility"
					removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
				}

				// Validate CRF requirements for domains
				for (auto const& [domainIndex, domainNode] : configurationNode.clockDomains)
				{
					auto const aafInputStreamsForDomain = countCapableStreamsForDomain(configurationNode.streamInputs, configurationNode.redundantStreamInputs, aafInputStreams, domainIndex);
					auto const crfInputStreamsForDomain = countCapableStreamsForDomain(configurationNode.streamInputs, configurationNode.redundantStreamInputs, crfInputStreams, domainIndex);
					auto const crfOutputStreamsForDomain = countCapableStreamsForDomain(configurationNode.streamOutputs, configurationNode.redundantStreamOutputs, crfOutputStreams, domainIndex);
					if (isAafMediaListener)
					{
						if (aafInputStreamsForDomain >= 2)
						{
							// [Milan-2019 Clause 7.2.2] For each supported clock domain, an AAF Media Listener with two or more AAF Media Inputs shall implement a CRF Media Clock Input
							if (crfInputStreamsForDomain == 0u)
							{
								LOG_CONTROLLER_WARN(entityID, "[Milan-2019 Clause 7.2.2] For each supported clock domain, an AAF Media Listener with two or more AAF Media Inputs shall implement a CRF Media Clock Input");
								// Remove "Milan compatibility"
								removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
							}
							// [Milan-2019 Clause 7.2.3] For each supported clock domain, an AAF Media Listener with two or more AAF Media Inputs shall implement a CRF Media Clock Output
							if (crfOutputStreamsForDomain == 0u)
							{
								LOG_CONTROLLER_WARN(entityID, "[Milan-2019 Clause 7.2.3] For each supported clock domain, an AAF Media Listener with two or more AAF Media Inputs shall implement a CRF Media Clock Output");
								// Remove "Milan compatibility"
								removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
							}
						}
					}
					if (isAafMediaTalker)
					{
						// [Milan-2019 Clause 7.2.2] For each supported clock domain, an AAF Media Talker shall implement a CRF Media Clock Input
						if (crfInputStreamsForDomain == 0u)
						{
							LOG_CONTROLLER_WARN(entityID, "[Milan-2019 Clause 7.2.2] For each supported clock domain, an AAF Media Talker shall implement a CRF Media Clock Input");
							// Remove "Milan compatibility"
							removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
						}
						// [Milan-2019 Clause 7.2.3] For each supported clock domain, an AAF Media Talker capable of synchronizing to an external clock source (not an AVB stream) shall implement a CRF Media Clock Output
						// TODO
					}
				}
			}
		}
		catch (ControlledEntity::Exception const&)
		{
			LOG_CONTROLLER_WARN(entityID, "Invalid current CONFIGURATION descriptor");
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			removeCompatibilityFlag(nullptr, controlledEntity, ControlledEntity::CompatibilityFlag::IEEE17221);
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

/** Actions to be done on the entity, just before advertising, which require looking at other already advertised entities (only for attached entities) */
void ControllerImpl::onPreAdvertiseEntity(ControlledEntityImpl& controlledEntity) noexcept
{
	auto const& e = controlledEntity.getEntity();
	auto const entityID = e.getEntityID();
	auto const isAemSupported = e.getEntityCapabilities().test(entity::EntityCapability::AemSupported);
	auto const hasAnyConfiguration = controlledEntity.hasAnyConfiguration();
	auto const isVirtualEntity = controlledEntity.isVirtual();

	// Lock to protect _controlledEntities
	auto const lg = std::lock_guard{ _lock };

	if (isAemSupported && hasAnyConfiguration)
	{
		// Now that this entity is ready to be advertised, update states that are linked to the connection with another entity, in case it was not advertised during processing
		// States related to Talker capabilities
		if (e.getTalkerCapabilities().test(entity::TalkerCapability::Implemented))
		{
			AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

			try
			{
				auto const& talkerConfigurationNode = controlledEntity.getCurrentConfigurationNode();

				// Process all entities that are connected to any of our output streams
				for (auto const& entityKV : _controlledEntities)
				{
					auto const listenerEntityID = entityKV.first;
					auto& listenerEntity = *(entityKV.second);

					// Don't process self, not yet advertised entities, nor different virtual/physical kind
					if (listenerEntityID == entityID || !listenerEntity.wasAdvertised() || isVirtualEntity != listenerEntity.isVirtual())
					{
						continue;
					}

					// We need the AEM to check for Listener connections
					if (listenerEntity.getEntity().getEntityCapabilities().test(entity::EntityCapability::AemSupported) && listenerEntity.hasAnyConfiguration())
					{
						try
						{
							auto const& configurationNode = listenerEntity.getCurrentConfigurationNode();

							// Check each of this Listener's Input Streams
							for (auto const& [streamIndex, streamInputNode] : configurationNode.streamInputs)
							{
								// If the Stream is Connected
								if (streamInputNode.dynamicModel.connectionInfo.state == entity::model::StreamInputConnectionInfo::State::Connected)
								{
									// Check against all the Talker's Output Streams
									for (auto const& [streamOutputIndex, streamOutputNode] : talkerConfigurationNode.streamOutputs)
									{
										auto const talkerIdentification = entity::model::StreamIdentification{ entityID, streamOutputIndex };

										// Connected to our talker
										if (streamInputNode.dynamicModel.connectionInfo.talkerStream == talkerIdentification)
										{
											// We want to build an accurate list of connections, based on the known listeners (already advertised only, the other ones will update once ready to advertise themselves)
											{
												// Add this listener to our list of connected entities
												controlledEntity.addStreamOutputConnection(streamOutputIndex, { listenerEntityID, streamIndex }, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
												// Do not trigger onStreamOutputConnectionsChanged notification, we are just about to advertise the entity
											}

											// Check for Latency Error (if the Listener was advertised before this Talker, it couldn't check Talker's PresentationTime, so do it now)
											if (streamOutputNode.dynamicModel.streamDynamicInfo && streamInputNode.dynamicModel.streamDynamicInfo)
											{
												if ((*streamInputNode.dynamicModel.streamDynamicInfo).msrpAccumulatedLatency > (*streamOutputNode.dynamicModel.streamDynamicInfo).msrpAccumulatedLatency)
												{
													updateStreamInputLatency(listenerEntity, streamIndex, true);
												}
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

		// States related to Listener capabilities
		if (e.getListenerCapabilities().test(entity::ListenerCapability::Implemented))
		{
			AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

			try
			{
				auto const& listenerConfigurationNode = controlledEntity.getCurrentConfigurationNode();

				// Process all our input streams that are connected to another talker
				for (auto const& [streamIndex, streamInputNode] : listenerConfigurationNode.streamInputs)
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
								if (talkerStreamOutputNode.dynamicModel.streamDynamicInfo && streamInputNode.dynamicModel.streamDynamicInfo)
								{
									if ((*streamInputNode.dynamicModel.streamDynamicInfo).msrpAccumulatedLatency > (*talkerStreamOutputNode.dynamicModel.streamDynamicInfo).msrpAccumulatedLatency)
									{
										isOverLatency = true;
									}
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

		// Compute Media Clock Chain and update all entities for which the chain ends on this newly added entity
		{
			// Process the newly added entity and update media clock if needed
			{
				auto* const configNode = controlledEntity.getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
				if (configNode != nullptr)
				{
					for (auto& [clockDomainIndex, clockDomainNode] : configNode->clockDomains)
					{
						computeAndUpdateMediaClockChain(controlledEntity, clockDomainNode, entityID, clockDomainIndex, std::nullopt, entityID);
					}
				}
			}

			// Process all other entities and update media clock if needed
			for (auto& [eid, entity] : _controlledEntities)
			{
				if (eid != entityID && entity->wasAdvertised() && entity->getEntity().getEntityCapabilities().test(entity::EntityCapability::AemSupported) && entity->hasAnyConfiguration())
				{
					auto* const configNode = entity->getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
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
				}
			}
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

	// Update all entities for which the chain has a node for that departing entity
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
				}
			}
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

		// Cases we want to flag as error and misbehaving entity, but continue enumeration
		case entity::ControllerEntity::MvuCommandStatus::BaseProtocolViolation:
		{
			return FailureAction::MisbehaveContinue;
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

		// Cases the library do not implement
		case entity::ControllerEntity::MvuCommandStatus::PartialImplementation:
		{
			return FailureAction::WarningContinue;
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

		// Cases the library do not implement
		case entity::ControllerEntity::AemCommandStatus::PartialImplementation:
		{
			return FailureAction::WarningContinue;
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

/* This method handles non-success AemCommandStatus returned while trying to RegisterUnsolicitedNotifications */
bool ControllerImpl::processRegisterUnsolFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity) noexcept
{
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const entityID = entity->getEntity().getEntityID();
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
			LOG_CONTROLLER_WARN(entityID, "Error registering for unsolicited notifications: {}", entity::LocalEntity::statusToString(status));
			removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
				LOG_CONTROLLER_WARN(entityID, "Milan mandatory command not supported by the entity: REGISTER_UNSOLICITED_NOTIFICATION");
				removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan);
			}
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
					// Remove "Milan compatibility" as device does not respond to mandatory command
					if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
					{
						LOG_CONTROLLER_WARN(entityID, "Too many timeouts for Milan mandatory command: REGISTER_UNSOLICITED_NOTIFICATION");
						removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan);
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
							LOG_CONTROLLER_WARN(entityID, "Too many unexpected errors for AEM command: REGISTER_UNSOLICITED_NOTIFICATION ({})", entity::LocalEntity::statusToString(status));
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const entityID = entity->getEntity().getEntityID();
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
				LOG_CONTROLLER_WARN(entityID, "Milan mandatory MVU command not properly implemented by the entity");
				removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan);
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
					LOG_CONTROLLER_WARN(entityID, "Milan mandatory MVU command not supported by the entity");
					removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan);
				}
			}
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
					if (!optionalForMilan)
					{
						// Remove "Milan compatibility" as device does not respond to mandatory command
						if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
						{
							LOG_CONTROLLER_WARN(entityID, "Too many timeouts for Milan mandatory MVU command");
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan);
						}
					}
				}
				else if (action == FailureAction::Busy)
				{
					LOG_CONTROLLER_WARN(entityID, "Too many unexpected errors for AEM command: REGISTER_UNSOLICITED_NOTIFICATION ({})", entity::LocalEntity::statusToString(status));
					// Flag the entity as "Not fully IEEE1722.1 compliant"
					removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const entityID = entity->getEntity().getEntityID();
	auto const action = getFailureActionForAemCommandStatus(status);
	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			return true;
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			LOG_CONTROLLER_WARN(entityID, "Error getting IEEE1722.1 mandatory descriptor ({}): {}", entity::model::descriptorTypeToString(descriptorType), entity::LocalEntity::statusToString(status));
			removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
				LOG_CONTROLLER_WARN(entityID, "Milan mandatory descriptor not supported by the entity: {}", entity::model::descriptorTypeToString(descriptorType));
				removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan);
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
						LOG_CONTROLLER_WARN(entityID, "Too many timeouts for Milan mandatory descriptor: {}", entity::model::descriptorTypeToString(descriptorType));
						removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan);
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
							LOG_CONTROLLER_WARN(entityID, "Too many unexpected errors for READ_DESCRIPTOR on {}: {}", entity::model::descriptorTypeToString(descriptorType), entity::LocalEntity::statusToString(status));
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const entityID = entity->getEntity().getEntityID();
	auto const action = getFailureActionForAemCommandStatus(status);
	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			return true;
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			LOG_CONTROLLER_WARN(entityID, "Error getting IEEE1722.1 mandatory dynamic info ({}): {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType), entity::LocalEntity::statusToString(status));
			removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
					LOG_CONTROLLER_WARN(entityID, "Milan mandatory dynamic info not supported by the entity: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
					removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan);
				}
			}
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
					if (!optionalForMilan)
					{
						// Remove "Milan compatibility" as device does not respond to mandatory command
						if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
						{
							LOG_CONTROLLER_WARN(entityID, "Too many timeouts for Milan mandatory dynamic info: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan);
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
							LOG_CONTROLLER_WARN(entityID, "Too many unexpected errors for dynamic info query {}: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType), entity::LocalEntity::statusToString(status));
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const entityID = entity->getEntity().getEntityID();
	auto const action = getFailureActionForControlStatus(status);
	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			return true;
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			LOG_CONTROLLER_WARN(entityID, "Error getting IEEE1722.1 mandatory ACMP info ({}): {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType), entity::LocalEntity::statusToString(status));
			removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
					LOG_CONTROLLER_WARN(entityID, "Milan mandatory ACMP command not supported by the entity: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
					removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan);
				}
			}
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
					if (!optionalForMilan)
					{
						// Remove "Milan compatibility" as device does not respond to mandatory command
						if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
						{
							LOG_CONTROLLER_WARN(entityID, "Too many timeouts for Milan mandatory ACMP command: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan);
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
							LOG_CONTROLLER_WARN(entityID, "Too many unexpected errors for ACMP command {}: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType), entity::LocalEntity::statusToString(status));
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const entityID = entity->getEntity().getEntityID();
	auto const action = getFailureActionForControlStatus(status);
	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			return true;
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			LOG_CONTROLLER_WARN(entityID, "Error getting IEEE1722.1 mandatory ACMP info ({}): {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType), entity::LocalEntity::statusToString(status));
			removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
					LOG_CONTROLLER_WARN(entityID, "Milan mandatory ACMP command not supported by the entity: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
					removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan);
				}
			}
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
					if (!optionalForMilan)
					{
						// Remove "Milan compatibility" as device does not respond to mandatory command
						if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
						{
							LOG_CONTROLLER_WARN(entityID, "Too many timeouts for Milan mandatory ACMP command: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType));
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan);
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
							LOG_CONTROLLER_WARN(entityID, "Too many unexpected errors for ACMP command {}: {}", ControlledEntityImpl::dynamicInfoTypeToString(dynamicInfoType), entity::LocalEntity::statusToString(status));
							// Flag the entity as "Not fully IEEE1722.1 compliant"
							removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221);
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
	AVDECC_ASSERT(!status, "Should not call this method with a SUCCESS status");

	auto const entityID = entity->getEntity().getEntityID();
	auto const action = getFailureActionForAemCommandStatus(status);
	auto checkScheduleRetry = false;
	auto fallbackStaticModelEnumeration = false;

	switch (action)
	{
		case FailureAction::MisbehaveContinue:
			// Flag the entity as "Misbehaving"
			addCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Misbehaving);
			fallbackStaticModelEnumeration = true;
			break;
		case FailureAction::ErrorContinue:
			// Flag the entity as "Not fully IEEE1722.1 compliant"
			LOG_CONTROLLER_WARN(entityID, "Error getting IEEE1722.1 mandatory descriptor dynamic info ({}): {}", ControlledEntityImpl::descriptorDynamicInfoTypeToString(descriptorDynamicInfoType), entity::LocalEntity::statusToString(status));
			removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::IEEE17221);
			fallbackStaticModelEnumeration = true;
			break;
		case FailureAction::NotAuthenticated:
			AVDECC_ASSERT(false, "TODO: Handle authentication properly (https://github.com/L-Acoustics/avdecc/issues/49)");
			return true;
		case FailureAction::WarningContinue:
			return true;
		case FailureAction::TimedOut:
			checkScheduleRetry = true;
			fallbackStaticModelEnumeration = true;
			break;
		case FailureAction::Busy:
			checkScheduleRetry = true;
			fallbackStaticModelEnumeration = true;
			break;
		case FailureAction::NotSupported:
			if (!optionalForMilan)
			{
				// Remove "Milan compatibility" as device does not support mandatory command
				if (entity->getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
				{
					LOG_CONTROLLER_WARN(entityID, "Milan mandatory AECP command not supported by the entity: {}", ControlledEntityImpl::descriptorDynamicInfoTypeToString(descriptorDynamicInfoType));
					removeCompatibilityFlag(this, *entity, ControlledEntity::CompatibilityFlag::Milan);
				}
			}
			fallbackStaticModelEnumeration = true;
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

	if (fallbackStaticModelEnumeration)
	{
		// Failed to retrieve single DescriptorDynamicInformation, retrieve the corresponding descriptor instead if possible, otherwise switch back to full StaticModel enumeration
		auto const success = fetchCorrespondingDescriptor(entity, configurationIndex, descriptorDynamicInfoType, descriptorIndex);

		// Fallback to full StaticModel enumeration
		if (!success)
		{
			entity->setIgnoreCachedEntityModel();
			entity->clearAllExpectedDescriptorDynamicInfo();
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
				addCompatibilityFlag(this, *listenerEntity, ControlledEntity::CompatibilityFlag::Misbehaving);
				return;
			}
			auto const previousInfo = listenerEntity->setStreamInputConnectionInformation(listenerStream.streamIndex, info, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

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

				// Process all other entities and update media clock if needed
				{
					// Lock to protect _controlledEntities
					auto const lg = std::lock_guard{ _lock };

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

									// We are now connected, check if the last node had a status of StreamNotConnected for that listener
									if (conState == entity::model::StreamInputConnectionInfo::State::Connected)
									{
										if (AVDECC_ASSERT_WITH_RET(!clockDomainNode.mediaClockChain.empty(), "Chain should not be empty"))
										{
											auto& lastNode = clockDomainNode.mediaClockChain.back();
											if (lastNode.status == model::MediaClockChainNode::Status::StreamNotConnected && lastNode.entityID == listenerStream.entityID)
											{
												// Save the domain/stream indexes, we'll continue from it
												auto const continueDomainIndex = lastNode.clockDomainIndex;
												auto const continueStreamOutputIndex = lastNode.streamOutputIndex;

												// Remove the node
												clockDomainNode.mediaClockChain.pop_back();

												// Update the chain starting from this entity
												computeAndUpdateMediaClockChain(*entity, clockDomainNode, listenerStream.entityID, continueDomainIndex, continueStreamOutputIndex, {});
											}
										}
									}
									// We are now disconnected, check for any node in the chain that had an Active status with that listener
									else
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
												computeAndUpdateMediaClockChain(*entity, clockDomainNode, listenerStream.entityID, continueDomainIndex, continueStreamOutputIndex, {});
												break;
											}
										}
									}
								}
							}
						}
					}
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
		chooseLocale(&entity, entity.getCurrentConfigurationIndex(), "en-US", nullptr);
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

	// Ready to advertise using the network executor
	auto const exName = _endStation->getProtocolInterface()->getExecutorName();
	ExecutorManager::getInstance().pushJob(exName,
		[this, entityID]()
		{
			auto const lg = std::lock_guard{ *_controller }; // Lock the Controller itself (thus, lock it's ProtocolInterface), since we are on the Networking Thread

			auto controlledEntity = getControlledEntityImplGuard(entityID);

			if (AVDECC_ASSERT_WITH_RET(!!controlledEntity, "Entity should be in the list"))
			{
				checkEnumerationSteps(controlledEntity.get());
			}
		});

	// Flush executor to be sure everything is loaded before returning
	ExecutorManager::getInstance().flush(exName);

	LOG_CONTROLLER_INFO(_controller->getEntityID(), "Successfully registered virtual entity with ID {}", utils::toHexString(entityID, true));

	return { avdecc::jsonSerializer::DeserializationError::NoError, "" };
}

ControllerImpl::SharedControlledEntityImpl ControllerImpl::createControlledEntityFromJson(json const& object, entity::model::jsonSerializer::Flags const flags, ControlledEntityImpl::LockInformation::SharedPointer const& lockInfo)
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
				auto const& jIndex = j.at(entity::keyName::Entity_InterfaceInformation_AvbInterfaceIndex);
				auto const avbInterfaceIndex = jIndex.is_null() ? entity::Entity::GlobalAvbInterfaceIndex : jIndex.get<entity::model::AvbInterfaceIndex>();
				intfcsInfo.insert(std::make_pair(avbInterfaceIndex, j.get<entity::Entity::InterfaceInformation>()));
			}
		}

		auto controlledEntity = std::make_shared<ControlledEntityImpl>(entity::Entity{ commonInfo, intfcsInfo }, lockInfo, true);
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
			return { avdecc::jsonSerializer::DeserializationError::UnsupportedDumpVersion, std::string("Unsupported dump version: ") + std::to_string(dumpVersion), {} };
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
		auto visitor = ChecksumEntityModelVisitor{ checksumVersion };
		controlledEntity.accept(&visitor, true);
		return visitor.getHash();
	}
	return std::nullopt;
}


} // namespace controller
} // namespace avdecc
} // namespace la
