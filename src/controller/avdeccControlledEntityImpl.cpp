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
* @file avdeccControlledEntityImpl.cpp
* @author Christophe Calmejane
*/

#include "avdeccControlledEntityImpl.hpp"
#include "avdeccControllerLogHelper.hpp"
#include "avdeccEntityModelCache.hpp"
#include "treeModelAccessTraverseStrategy.hpp"
#include "treeModelAccessCacheStrategy.hpp"

#include <la/avdecc/internals/streamFormatInfo.hpp>
#include <la/avdecc/internals/entityModelControlValuesTraits.hpp>
#include <la/avdecc/utils.hpp>

#include <algorithm>
#include <cassert>
#include <typeindex>
#include <unordered_map>
#include <string>

#pragma message("TODO: Add check in all methods, if the class is locked or not. We want the lock to be taken for all APIs")

namespace la
{
namespace avdecc
{
namespace controller
{
static constexpr std::uint16_t MaxRegisterUnsolRetryCount = 1;
static constexpr std::uint16_t MaxQueryMilanInfoRetryCount = 2;
static constexpr std::uint16_t MaxQueryDescriptorRetryCount = 2;
static constexpr std::uint16_t MaxQueryDynamicInfoRetryCount = 2;
static constexpr std::uint16_t MaxQueryDescriptorDynamicInfoRetryCount = 2;
static constexpr std::uint16_t QueryRetryMillisecondDelay = 500;
static entity::model::AvdeccFixedString s_noLocalizationString{};

/** Returns the common part of the two strings, with excess spaces removed. */
static std::string getCommonString(std::string const& lhs, std::string const& rhs) noexcept
{
	auto const tokensL = avdecc::utils::tokenizeString(lhs, ' ', false);
	auto const tokensR = avdecc::utils::tokenizeString(rhs, ' ', false);

	if (tokensL.size() == tokensR.size())
	{
		auto result = std::string{};
		for (auto tokPos = 0u; tokPos < tokensL.size(); ++tokPos)
		{
			auto const tokL = tokensL[tokPos];
			auto const tokR = tokensR[tokPos];
			if (tokL == tokR)
			{
				if (!result.empty())
				{
					result += ' ';
				}
				result += tokL;
			}
		}
		return result;
	}

	return {};
}

/* ************************************************************************** */
/* ControlledEntityImpl                                                       */
/* ************************************************************************** */
/** Constructor */
ControlledEntityImpl::ControlledEntityImpl(entity::Entity const& entity, LockInformation::SharedPointer const& sharedLock, bool const isVirtual) noexcept
	: _sharedLock(sharedLock)
	, _isVirtual(isVirtual)
	, _entity(entity)
{
	_treeModelAccess = std::make_unique<TreeModelAccessTraverseStrategy>(this);
}

// ControlledEntity overrides
// Getters
bool ControlledEntityImpl::isVirtual() const noexcept
{
	return _isVirtual;
}
ControlledEntity::CompatibilityFlags ControlledEntityImpl::getCompatibilityFlags() const noexcept
{
	return _compatibilityFlags;
}

bool ControlledEntityImpl::isMilanRedundant() const noexcept
{
	return _isMilanRedundant;
}

bool ControlledEntityImpl::gotFatalEnumerationError() const noexcept
{
	return _gotFatalEnumerateError;
}

bool ControlledEntityImpl::isSubscribedToUnsolicitedNotifications() const noexcept
{
	return _isSubscribedToUnsolicitedNotifications;
}

bool ControlledEntityImpl::areUnsolicitedNotificationsSupported() const noexcept
{
	return _areUnsolicitedNotificationsSupported;
}

bool ControlledEntityImpl::isAcquired() const noexcept
{
	return _acquireState == model::AcquireState::Acquired;
}

bool ControlledEntityImpl::isAcquireCommandInProgress() const noexcept
{
	return _acquireState == model::AcquireState::AcquireInProgress || _acquireState == model::AcquireState::ReleaseInProgress;
}

bool ControlledEntityImpl::isAcquiredByOther() const noexcept
{
	return _acquireState == model::AcquireState::AcquiredByOther;
}

bool ControlledEntityImpl::isLocked() const noexcept
{
	return _lockState == model::LockState::Locked;
}

bool ControlledEntityImpl::isLockCommandInProgress() const noexcept
{
	return _lockState == model::LockState::LockInProgress || _lockState == model::LockState::UnlockInProgress;
}

bool ControlledEntityImpl::isLockedByOther() const noexcept
{
	return _lockState == model::LockState::LockedByOther;
}

bool ControlledEntityImpl::isStreamInputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const* const dynamicModel = _treeModelAccess->getStreamInputNodeDynamicModel(configurationIndex, streamIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
	if (dynamicModel)
	{
		return dynamicModel->isStreamRunning ? *dynamicModel->isStreamRunning : true;
	}
	return false;
}

bool ControlledEntityImpl::isStreamOutputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const* const dynamicModel = _treeModelAccess->getStreamOutputNodeDynamicModel(configurationIndex, streamIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
	if (dynamicModel)
	{
		return dynamicModel->isStreamRunning ? *dynamicModel->isStreamRunning : true;
	}
	return false;
}

ControlledEntity::InterfaceLinkStatus ControlledEntityImpl::getAvbInterfaceLinkStatus(entity::model::AvbInterfaceIndex const avbInterfaceIndex) const noexcept
{
	// AEM not supported, unknown status
	if (!_entity.getEntityCapabilities().test(entity::EntityCapability::AemSupported) || !hasAnyConfiguration())
	{
		return InterfaceLinkStatus::Unknown;
	}

	auto const it = _avbInterfaceLinkStatus.find(avbInterfaceIndex);
	if (it == _avbInterfaceLinkStatus.end())
	{
		return InterfaceLinkStatus::Unknown;
	}

	return it->second;
}

model::AcquireState ControlledEntityImpl::getAcquireState() const noexcept
{
	return _acquireState;
}

UniqueIdentifier ControlledEntityImpl::getOwningControllerID() const noexcept
{
	return _owningControllerID;
}

model::LockState ControlledEntityImpl::getLockState() const noexcept
{
	return _lockState;
}

UniqueIdentifier ControlledEntityImpl::getLockingControllerID() const noexcept
{
	return _lockingControllerID;
}

entity::Entity const& ControlledEntityImpl::getEntity() const noexcept
{
	return _entity;
}

std::optional<entity::model::MilanInfo> ControlledEntityImpl::getMilanInfo() const noexcept
{
	return _milanInfo;
}

std::optional<entity::model::ControlIndex> ControlledEntityImpl::getIdentifyControlIndex() const noexcept
{
	return _identifyControlIndex;
}

bool ControlledEntityImpl::isEntityModelValidForCaching() const noexcept
{
	if (_gotFatalEnumerateError || _entityNode.configurations.empty())
	{
		return false;
	}

	return isEntityModelComplete(_entityNode, static_cast<std::uint16_t>(_entityNode.configurations.size()));
}

bool ControlledEntityImpl::isIdentifying() const noexcept
{
	// The entity has an Identify ControlIndex
	auto const identifyControlIndex = getIdentifyControlIndex();
	if (identifyControlIndex)
	{
		// Check if identify is currently in progress
		auto const* const entityDynamicModel = _treeModelAccess->getEntityNodeDynamicModel(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
		if (entityDynamicModel)
		{
			auto const* const controlDynamicModel = _treeModelAccess->getControlNodeDynamicModel(entityDynamicModel->currentConfiguration, *identifyControlIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			if (controlDynamicModel)
			{
				try
				{
					// Get and check the control value
					auto const& values = controlDynamicModel->values;
					AVDECC_ASSERT(values.areDynamicValues() && values.getType() == entity::model::ControlValueType::Type::ControlLinearUInt8, "Doesn't look like Identify Control Value");

					if (values.size() == 1)
					{
						auto const dynamicValues = values.getValues<entity::model::LinearValues<entity::model::LinearValueDynamic<std::uint8_t>>>(); // We have to store the copy or it will go out of scope
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
				catch (std::invalid_argument const&)
				{
					AVDECC_ASSERT(false, "Identify Control Descriptor values doesn't seem valid");
				}
				catch (...)
				{
					AVDECC_ASSERT(false, "Identify Control Descriptor was validated, this should not throw");
				}
			}
		}
	}
	return false;
}

bool ControlledEntityImpl::hasAnyConfiguration() const noexcept
{
	return !_entityNode.configurations.empty();
}

entity::model::ConfigurationIndex ControlledEntityImpl::getCurrentConfigurationIndex() const
{
	auto const& entityNode = getEntityNode();
	return entityNode.dynamicModel.currentConfiguration;
}

model::EntityNode const& ControlledEntityImpl::getEntityNode() const
{
	return *_treeModelAccess->getEntityNode(TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::ConfigurationNode const& ControlledEntityImpl::getConfigurationNode(entity::model::ConfigurationIndex const configurationIndex) const
{
	return *_treeModelAccess->getConfigurationNode(configurationIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::ConfigurationNode const& ControlledEntityImpl::getCurrentConfigurationNode() const
{
	return *_treeModelAccess->getConfigurationNode(getCurrentConfigurationIndex(), TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::AudioUnitNode const& ControlledEntityImpl::getAudioUnitNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex) const
{
	return *_treeModelAccess->getAudioUnitNode(configurationIndex, audioUnitIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::StreamInputNode const& ControlledEntityImpl::getStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	return *_treeModelAccess->getStreamInputNode(configurationIndex, streamIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::StreamOutputNode const& ControlledEntityImpl::getStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	return *_treeModelAccess->getStreamOutputNode(configurationIndex, streamIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
model::RedundantStreamNode const& ControlledEntityImpl::getRedundantStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex) const
{
	return *_treeModelAccess->getRedundantStreamInputNode(configurationIndex, redundantStreamIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::RedundantStreamNode const& ControlledEntityImpl::getRedundantStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex) const
{
	return *_treeModelAccess->getRedundantStreamOutputNode(configurationIndex, redundantStreamIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

model::JackInputNode const& ControlledEntityImpl::getJackInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex) const
{
	return *_treeModelAccess->getJackInputNode(configurationIndex, jackIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::JackOutputNode const& ControlledEntityImpl::getJackOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex) const
{
	return *_treeModelAccess->getJackOutputNode(configurationIndex, jackIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::AvbInterfaceNode const& ControlledEntityImpl::getAvbInterfaceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex) const
{
	return *_treeModelAccess->getAvbInterfaceNode(configurationIndex, avbInterfaceIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::ClockSourceNode const& ControlledEntityImpl::getClockSourceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex) const
{
	return *_treeModelAccess->getClockSourceNode(configurationIndex, clockSourceIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::StreamPortNode const& ControlledEntityImpl::getStreamPortInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const
{
	return *_treeModelAccess->getStreamPortInputNode(configurationIndex, streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::StreamPortNode const& ControlledEntityImpl::getStreamPortOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const
{
	return *_treeModelAccess->getStreamPortOutputNode(configurationIndex, streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::AudioClusterNode const& ControlledEntityImpl::getAudioClusterNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex) const
{
	return *_treeModelAccess->getAudioClusterNode(configurationIndex, clusterIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}
#if 0
model::AudioMapNode const& ControlledEntityImpl::getAudioMapNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex) const
{
#	if 1
	static model::AudioMapNode s{};
	(void)configurationIndex;
	(void)audioUnitIndex;
	(void)streamPortIndex;
	(void)mapIndex;
	return s;
#	else
#	endif
}
#endif

model::ControlNode const& ControlledEntityImpl::getControlNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex) const
{
	return *_treeModelAccess->getControlNode(configurationIndex, controlIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw, TreeModelAccessStrategy::DefaultConstructLevelHint::None);
}

model::ClockDomainNode const& ControlledEntityImpl::getClockDomainNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex) const
{
	return *_treeModelAccess->getClockDomainNode(configurationIndex, clockDomainIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::TimingNode const& ControlledEntityImpl::getTimingNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex) const
{
	return *_treeModelAccess->getTimingNode(configurationIndex, timingIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::PtpInstanceNode const& ControlledEntityImpl::getPtpInstanceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex) const
{
	return *_treeModelAccess->getPtpInstanceNode(configurationIndex, ptpInstanceIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::PtpPortNode const& ControlledEntityImpl::getPtpPortNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex) const
{
	return *_treeModelAccess->getPtpPortNode(configurationIndex, ptpPortIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

model::LocaleNode const* ControlledEntityImpl::findLocaleNode(entity::model::ConfigurationIndex const configurationIndex, std::string const& /*locale*/) const
{
#pragma message("TODO: Parse 'locale' parameter and find best match")
	// Right now, return the first locale
	return _treeModelAccess->getLocaleNode(configurationIndex, entity::model::LocaleIndex{ 0u }, TreeModelAccessStrategy::NotFoundBehavior::Throw);
}

entity::model::AvdeccFixedString const& ControlledEntityImpl::getLocalizedString(entity::model::LocalizedStringReference const& stringReference) const noexcept
{
	auto const* const entityDynamicModel = _treeModelAccess->getEntityNodeDynamicModel(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
	if (entityDynamicModel)
	{
		return getLocalizedString(entityDynamicModel->currentConfiguration, stringReference);
	}
	return s_noLocalizationString;
}

entity::model::AvdeccFixedString const& ControlledEntityImpl::getLocalizedString(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocalizedStringReference const& stringReference) const noexcept
{
	// Not valid, return NO_STRING
	if (!stringReference)
	{
		return s_noLocalizationString;
	}

	auto const* const configurationDynamicModel = _treeModelAccess->getConfigurationNodeDynamicModel(configurationIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (configurationDynamicModel)
	{
		auto const globalOffset = stringReference.getGlobalOffset();
		if (globalOffset < configurationDynamicModel->localizedStrings.size())
		{
			return configurationDynamicModel->localizedStrings.at(entity::model::StringsIndex(globalOffset));
		}
		else
		{
			LOG_CONTROLLER_WARN(_entity.getEntityID(), "LocalizedStringReference out of bounds: {} (global offset: {})", stringReference.getValue(), globalOffset);
		}
	}
	return s_noLocalizationString;
}

entity::model::StreamInputConnectionInfo const& ControlledEntityImpl::getSinkConnectionInformation(entity::model::StreamIndex const streamIndex) const
{
	auto const* const streamDynamicModel = _treeModelAccess->getStreamInputNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
	AVDECC_ASSERT(!!streamDynamicModel, "Should not be null, should have thrown in case of error");
	return streamDynamicModel->connectionInfo;
}

entity::model::AudioMappings const& ControlledEntityImpl::getStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const* const streamPortNode = _treeModelAccess->getStreamPortInputNode(getCurrentConfigurationIndex(), streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
	AVDECC_ASSERT(!!streamPortNode, "Should not be null, should have thrown in case of error");

	// Check if dynamic mappings is supported by the entity
	if (!streamPortNode->staticModel.hasDynamicAudioMap)
	{
		throw Exception(Exception::Type::NotSupported, "Dynamic mappings not supported by this stream port");
	}

	// Return dynamic mappings for this stream port
	return streamPortNode->dynamicModel.dynamicAudioMap;
}

entity::model::AudioMappings ControlledEntityImpl::getStreamPortInputNonRedundantAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const
{
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	// Get the current mappings
	auto const& mappings = getStreamPortInputAudioMappings(streamPortIndex);
	auto nonRedundantMappings = decltype(mappings){};

	// For each mapping, add only if not secondary stream
	for (auto const& map : mappings)
	{
		if (!isRedundantSecondaryStreamInput(map.streamIndex))
		{
			nonRedundantMappings.push_back(map);
		}
	}

	return nonRedundantMappings;
#else // !ENABLE_AVDECC_FEATURE_REDUNDANCY
	// Return a copy of the current mappings
	return getStreamPortInputAudioMappings(streamPortIndex);
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
}

entity::model::AudioMappings const& ControlledEntityImpl::getStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const* const streamPortNode = _treeModelAccess->getStreamPortOutputNode(getCurrentConfigurationIndex(), streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
	AVDECC_ASSERT(!!streamPortNode, "Should not be null, should have thrown in case of error");

	// Check if dynamic mappings is supported by the entity
	if (!streamPortNode->staticModel.hasDynamicAudioMap)
	{
		throw Exception(Exception::Type::NotSupported, "Dynamic mappings not supported by this stream port");
	}

	// Return dynamic mappings for this stream port
	return streamPortNode->dynamicModel.dynamicAudioMap;
}

entity::model::AudioMappings ControlledEntityImpl::getStreamPortOutputNonRedundantAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const
{
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	// Get the current mappings
	auto const& mappings = getStreamPortOutputAudioMappings(streamPortIndex);
	auto nonRedundantMappings = decltype(mappings){};

	// For each mapping, add only if not secondary stream
	for (auto const& map : mappings)
	{
		if (!isRedundantSecondaryStreamOutput(map.streamIndex))
		{
			nonRedundantMappings.push_back(map);
		}
	}

	return nonRedundantMappings;
#else // !ENABLE_AVDECC_FEATURE_REDUNDANCY
	// Return a copy of the current mappings
	return getStreamPortOutputAudioMappings(streamPortIndex);
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
}

std::map<entity::model::StreamPortIndex, entity::model::AudioMappings> ControlledEntityImpl::getStreamPortInputInvalidAudioMappingsForStreamFormat(entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const
{
	auto const& configurationNode = getCurrentConfigurationNode();
	auto const maxStreams = configurationNode.streamInputs.size();
	auto const maxStreamChannels = entity::model::StreamFormatInfo::create(streamFormat)->getChannelsCount();
	auto invalidPortMappings = std::map<entity::model::StreamPortIndex, entity::model::AudioMappings>{};

	// Process all StreamPort Input, in all audio units
	for (auto const& [audioUnitIndex, audioUnitNode] : configurationNode.audioUnits)
	{
		for (auto const& [streamPortIndex, streamPortNode] : audioUnitNode.streamPortInputs)
		{
			// Check if dynamic mappings is supported by the entity
			if (streamPortNode.staticModel.hasDynamicAudioMap)
			{
				auto const maxClusters = streamPortNode.audioClusters.size();
				auto invalidMappings = entity::model::AudioMappings{};

				for (auto const& mapping : streamPortNode.dynamicModel.dynamicAudioMap)
				{
					// Only check mappings for affected StreamIndex
					if (mapping.streamIndex == streamIndex)
					{
						// Check if the mapping makes sense statically (regarding StreamIndex and ClusterOffset bounds)
						if (AVDECC_ASSERT_WITH_RET(mapping.streamIndex < maxStreams && mapping.clusterOffset < maxClusters, "Mapping stream/cluster index is out of bounds"))
						{
							// Check if the mapping makes sense statically (regarding ClusterOffset value)
							auto const clusterIndex = static_cast<entity::model::ClusterIndex>(streamPortNode.staticModel.baseCluster + mapping.clusterOffset);
							auto const clusterNodeIt = streamPortNode.audioClusters.find(clusterIndex);
							if (AVDECC_ASSERT_WITH_RET(clusterNodeIt != streamPortNode.audioClusters.end(), "Mapping cluster offset invalid for this stream port"))
							{
								// Check if the mapping makes sense statically (regarding ClusterChannel)
								auto const& clusterNode = clusterNodeIt->second;
								if (AVDECC_ASSERT_WITH_RET(mapping.clusterChannel < clusterNode.staticModel.channelCount, "Mapping cluster channel is out of bounds"))
								{
									// Check if the mapping will be out of stream bounds with the new stream format
									if (mapping.streamChannel >= maxStreamChannels)
									{
										invalidMappings.push_back(mapping);
									}
								}
							}
						}
					}
				}

				if (!invalidMappings.empty())
				{
					invalidPortMappings.insert_or_assign(streamPortIndex, std::move(invalidMappings));
				}
			}
		}
	}

	return invalidPortMappings;
}

entity::model::StreamConnections const& ControlledEntityImpl::getStreamOutputConnections(entity::model::StreamIndex const streamIndex) const
{
	auto const* const streamDynamicModel = _treeModelAccess->getStreamOutputNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
	AVDECC_ASSERT(!!streamDynamicModel, "Should not be null, should have thrown in case of error");
	return streamDynamicModel->connections;
}

// Statistics
std::uint64_t ControlledEntityImpl::getAecpRetryCounter() const noexcept
{
	return _aecpRetryCounter;
}

std::uint64_t ControlledEntityImpl::getAecpTimeoutCounter() const noexcept
{
	return _aecpTimeoutCounter;
}

std::uint64_t ControlledEntityImpl::getAecpUnexpectedResponseCounter() const noexcept
{
	return _aecpUnexpectedResponseCounter;
}

std::chrono::milliseconds const& ControlledEntityImpl::getAecpResponseAverageTime() const noexcept
{
	return _aecpResponseAverageTime;
}

std::uint64_t ControlledEntityImpl::getAemAecpUnsolicitedCounter() const noexcept
{
	return _aemAecpUnsolicitedCounter;
}

std::uint64_t ControlledEntityImpl::getAemAecpUnsolicitedLossCounter() const noexcept
{
	return _aemAecpUnsolicitedLossCounter;
}

std::chrono::milliseconds const& ControlledEntityImpl::getEnumerationTime() const noexcept
{
	return _enumerationTime;
}

// Diagnostics
ControlledEntity::Diagnostics const& ControlledEntityImpl::getDiagnostics() const noexcept
{
	return _diagnostics;
}

// Visitor methods
namespace visitorHelper
{
template<typename NodeType>
void processStreamPortNodes(ControlledEntity const* const entity, model::EntityModelVisitor* const visitor, model::ConfigurationNode const& configuration, model::AudioUnitNode const& audioUnit, std::map<entity::model::StreamPortIndex, NodeType> const& streamPorts)
{
	for (auto const& streamPortKV : streamPorts)
	{
		auto const& streamPort = streamPortKV.second;
		// Visit StreamPortNode (AudioUnitNode is parent)
		visitor->visit(entity, &configuration, &audioUnit, streamPort);

		// Loop over AudioClusterNode
		for (auto const& audioClusterKV : streamPort.audioClusters)
		{
			auto const& audioCluster = audioClusterKV.second;
			// Visit AudioClusterNode (StreamPortNode is parent)
			visitor->visit(entity, &configuration, &audioUnit, &streamPort, audioCluster);
		}

		// Loop over AudioMapNode
		for (auto const& audioMapKV : streamPort.audioMaps)
		{
			auto const& audioMap = audioMapKV.second;
			// Visit AudioMapNode (StreamPortNode is parent)
			visitor->visit(entity, &configuration, &audioUnit, &streamPort, audioMap);
		}

		// Loop over ControlNode
		for (auto const& controlKV : streamPort.controls)
		{
			auto const& control = controlKV.second;
			// Visit ControlNode (StreamPortNode is parent)
			visitor->visit(entity, &configuration, &audioUnit, &streamPort, control);
		}
	}
}

template<typename NodeType>
void processStreamNodes(ControlledEntity const* const entity, model::EntityModelVisitor* const visitor, model::ConfigurationNode const& configuration, std::map<entity::model::StreamIndex, NodeType> const& streams)
{
	for (auto const& streamKV : streams)
	{
		auto const& stream = streamKV.second;
		// Visit StreamNode (ConfigurationNode is parent)
		visitor->visit(entity, &configuration, stream);
	}
}

template<typename NodeType>
void processJackNodes(ControlledEntity const* const entity, model::EntityModelVisitor* const visitor, model::ConfigurationNode const& configuration, std::map<entity::model::JackIndex, NodeType> const& jacks)
{
	for (auto const& jackKV : jacks)
	{
		auto const& jack = jackKV.second;
		// Visit JackNode (ConfigurationNode is parent)
		visitor->visit(entity, &configuration, jack);

		// Loop over ControlNode
		for (auto const& controlKV : jack.controls)
		{
			auto const& control = controlKV.second;
			// Visit ControlNode (JackNode is parent)
			visitor->visit(entity, &configuration, &jack, control);
		}
	}
}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
template<typename NodeType, typename RedundantNodeType>
void processRedundantStreamNodes(ControlledEntity const* const entity, model::EntityModelVisitor* const visitor, model::ConfigurationNode const& configuration, std::map<model::VirtualIndex, RedundantNodeType> const& redundantStreams)
{
	for (auto const& redundantStreamKV : redundantStreams)
	{
		auto const& redundantStream = redundantStreamKV.second;
		// Visit RedundantStreamNode (ConfigurationNode is parent)
		visitor->visit(entity, &configuration, redundantStream);

		// Loop over StreamNodes
		for (auto const streamIndex : redundantStream.redundantStreams)
		{
			if constexpr (std::is_same_v<NodeType, model::StreamInputNode>)
			{
				if (auto const streamIt = configuration.streamInputs.find(streamIndex); streamIt != configuration.streamInputs.end())
				{
					auto const& stream = static_cast<NodeType const&>(streamIt->second);
					// Visit StreamNodes (RedundantStreamNodes is parent)
					visitor->visit(entity, &configuration, &redundantStream, stream);
				}
			}
			else if constexpr (std::is_same_v<NodeType, model::StreamOutputNode>)
			{
				if (auto const streamIt = configuration.streamOutputs.find(streamIndex); streamIt != configuration.streamOutputs.end())
				{
					auto const& stream = static_cast<NodeType const&>(streamIt->second);
					// Visit StreamNodes (RedundantStreamNodes is parent)
					visitor->visit(entity, &configuration, &redundantStream, stream);
				}
			}
			else
			{
				AVDECC_ASSERT(false, "Unknown NodeType");
			}
		}
	}
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
} // namespace visitorHelper

void ControlledEntityImpl::accept(model::EntityModelVisitor* const visitor, bool const visitAllConfigurations) const noexcept
{
	if (_gotFatalEnumerateError)
		return;

	if (visitor == nullptr)
		return;

	if (!_entity.getEntityCapabilities().test(entity::EntityCapability::AemSupported))
		return;

	try
	{
		// Visit entity model graph
		auto const& entityModel = getEntityNode();

		// Visit EntityModelNode (no parent)
		visitor->visit(this, entityModel);

		// Loop over all configurations
		for (auto const& configurationKV : entityModel.configurations)
		{
			auto const& configuration = configurationKV.second;

			// Visit ConfigurationNode (EntityModelNode is parent)
			visitor->visit(this, &entityModel, configuration);

			// If this is the active configuration, process ConfigurationNode fields
			if (visitAllConfigurations || configuration.dynamicModel.isActiveConfiguration)
			{
				// Loop over AudioUnitNode
				for (auto const& audioUnitKV : configuration.audioUnits)
				{
					auto const& audioUnit = audioUnitKV.second;
					// Visit AudioUnitNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, audioUnit);

					// Loop over StreamPortNodes
					visitorHelper::processStreamPortNodes(this, visitor, configuration, audioUnit, audioUnit.streamPortInputs);
					visitorHelper::processStreamPortNodes(this, visitor, configuration, audioUnit, audioUnit.streamPortOutputs);

					// Loop over ExternalPortInput
					// Loop over ExternalPortOutput
					// Loop over InternalPortInput
					// Loop over InternalPortOutput

					// Loop over ControlNode
					for (auto const& controlKV : audioUnit.controls)
					{
						auto const& control = controlKV.second;
						// Visit ControlNode (AudioUnitNode is parent)
						visitor->visit(this, &configuration, &audioUnit, control);
					}
				}

				// Loop over StreamNodes
				visitorHelper::processStreamNodes(this, visitor, configuration, configuration.streamInputs);
				visitorHelper::processStreamNodes(this, visitor, configuration, configuration.streamOutputs);

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				// Loop over RedundantStreamNodes
				visitorHelper::processRedundantStreamNodes<model::StreamInputNode>(this, visitor, configuration, configuration.redundantStreamInputs);
				visitorHelper::processRedundantStreamNodes<model::StreamOutputNode>(this, visitor, configuration, configuration.redundantStreamOutputs);
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

				// Loop over JackNodes
				visitorHelper::processJackNodes(this, visitor, configuration, configuration.jackInputs);
				visitorHelper::processJackNodes(this, visitor, configuration, configuration.jackOutputs);

				// Loop over AvbInterfaceNode
				for (auto const& interfaceKV : configuration.avbInterfaces)
				{
					auto const& intfc = interfaceKV.second;
					// Visit AvbInterfaceNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, intfc);
				}

				// Loop over ClockSourceNode
				for (auto const& sourceKV : configuration.clockSources)
				{
					auto const& source = sourceKV.second;
					// Visit ClockSourceNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, source);
				}

				// Loop over MemoryObjectNode
				for (auto const& memoryObjectKV : configuration.memoryObjects)
				{
					auto const& memoryObject = memoryObjectKV.second;
					// Visit MemoryObjectNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, memoryObject);
				}

				// Loop over LocaleNode
				for (auto const& localeKV : configuration.locales)
				{
					auto const& locale = localeKV.second;
					// Visit LocaleNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, locale);

					// Loop over StringsNode
					for (auto const& stringsKV : locale.strings)
					{
						auto const& strings = stringsKV.second;
						// Visit StringsNode (LocaleNode is parent)
						visitor->visit(this, &configuration, &locale, strings);
					}
				}

				// Loop over ControlNode
				for (auto const& controlKV : configuration.controls)
				{
					auto const& control = controlKV.second;
					// Visit ControlNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, control);
				}

				// Loop over ClockDomainNode
				for (auto const& domainKV : configuration.clockDomains)
				{
					auto const& domain = domainKV.second;
					// Visit ClockDomainNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, domain);

					// Loop over ClockSourceNode
					for (auto const sourceIndex : domain.staticModel.clockSources)
					{
						if (auto const sourceIt = configuration.clockSources.find(sourceIndex); sourceIt != configuration.clockSources.end())
						{
							auto const& source = sourceIt->second;
							// Visit ClockSourceNode (ClockDomainNode is parent)
							visitor->visit(this, &configuration, &domain, source);
						}
						else
						{
							LOG_CONTROLLER_WARN(_entity.getEntityID(), "Invalid ClockSourceIndex in ClockDomain");
						}
					}
				}

				// Loop over TimingNode
				for (auto const& timingKV : configuration.timings)
				{
					auto const& timing = timingKV.second;
					// Visit TimingNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, timing);

					// Loop over PtpInstanceNode
					for (auto const ptpInstanceIndex : timing.staticModel.ptpInstances)
					{
						if (auto const ptpInstanceIt = configuration.ptpInstances.find(ptpInstanceIndex); ptpInstanceIt != configuration.ptpInstances.end())
						{
							auto const& ptpInstance = ptpInstanceIt->second;
							// Visit PtpInstanceNode (TimingNode is parent)
							visitor->visit(this, &configuration, &timing, ptpInstance);

							// Loop over ControlNode
							for (auto const& controlKV : ptpInstance.controls)
							{
								auto const& control = controlKV.second;
								// Visit ControlNode (PtpInstanceNode is parent)
								visitor->visit(this, &configuration, &timing, &ptpInstance, control);
							}
							// Loop over PtpPortNode
							for (auto const& ptpPortKV : ptpInstance.ptpPorts)
							{
								auto const& port = ptpPortKV.second;
								// Visit PtpPortNode (PtpInstanceNode is parent)
								visitor->visit(this, &configuration, &timing, &ptpInstance, port);
							}
						}
						else
						{
							LOG_CONTROLLER_WARN(_entity.getEntityID(), "Invalid PtpInstanceIndex in Timing");
						}
					}
				}

				// Loop over PtpInstanceNode
				for (auto const& ptpInstanceKV : configuration.ptpInstances)
				{
					auto const& ptpInstance = ptpInstanceKV.second;
					// Visit PtpInstanceNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, ptpInstance);

					// Loop over ControlNode
					for (auto const& controlKV : ptpInstance.controls)
					{
						auto const& control = controlKV.second;
						// Visit ControlNode (PtpInstanceNode is parent)
						visitor->visit(this, &configuration, &ptpInstance, control);
					}
					// Loop over PtpPortNode
					for (auto const& ptpPortKV : ptpInstance.ptpPorts)
					{
						auto const& port = ptpPortKV.second;
						// Visit PtpPortNode (PtpInstanceNode is parent)
						visitor->visit(this, &configuration, &ptpInstance, port);
					}
				}
			}
		}
	}
	catch (...)
	{
		// Ignore exceptions
		AVDECC_ASSERT(false, "Should never throw");
	}
}

void ControlledEntityImpl::lock() noexcept
{
	_sharedLock->lock();
}

void ControlledEntityImpl::unlock() noexcept
{
	_sharedLock->unlock();
}

TreeModelAccessStrategy& ControlledEntityImpl::getModelAccessStrategy() noexcept
{
	return *_treeModelAccess;
}

// Non-const Node getters
model::EntityNode* ControlledEntityImpl::getEntityNode(TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	return _treeModelAccess->getEntityNode(notFoundBehavior);
}

std::optional<entity::model::ConfigurationIndex> ControlledEntityImpl::getCurrentConfigurationIndex(TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const* const entityNode = getEntityNode(notFoundBehavior);
	if (entityNode)
	{
		return entityNode->dynamicModel.currentConfiguration;
	}
	return std::nullopt;
}

model::ConfigurationNode* ControlledEntityImpl::getCurrentConfigurationNode(TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		return getConfigurationNode(*currentConfigurationIndexOpt, notFoundBehavior);
	}
	return nullptr;
}

model::ConfigurationNode* ControlledEntityImpl::getConfigurationNode(entity::model::ConfigurationIndex const configurationIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	return _treeModelAccess->getConfigurationNode(configurationIndex, notFoundBehavior);
}

entity::model::EntityCounters* ControlledEntityImpl::getEntityCounters(TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto* const dynamicModel = _treeModelAccess->getEntityNodeDynamicModel(notFoundBehavior);
	if (dynamicModel)
	{
		// Create counters if they don't exist yet
		if (!dynamicModel->counters)
		{
			dynamicModel->counters = entity::model::EntityCounters{};
		}
		return &(*dynamicModel->counters);
	}
	return nullptr;
}

entity::model::AvbInterfaceCounters* ControlledEntityImpl::getAvbInterfaceCounters(entity::model::AvbInterfaceIndex const avbInterfaceIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getAvbInterfaceNodeDynamicModel(*currentConfigurationIndexOpt, avbInterfaceIndex, notFoundBehavior);
		if (dynamicModel)
		{
			// Create counters if they don't exist yet
			if (!dynamicModel->counters)
			{
				dynamicModel->counters = entity::model::AvbInterfaceCounters{};
			}
			return &(*dynamicModel->counters);
		}
	}
	return nullptr;
}

entity::model::ClockDomainCounters* ControlledEntityImpl::getClockDomainCounters(entity::model::ClockDomainIndex const clockDomainIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getClockDomainNodeDynamicModel(*currentConfigurationIndexOpt, clockDomainIndex, notFoundBehavior);
		if (dynamicModel)
		{
			// Create counters if they don't exist yet
			if (!dynamicModel->counters)
			{
				dynamicModel->counters = entity::model::ClockDomainCounters{};
			}
			return &(*dynamicModel->counters);
		}
	}
	return nullptr;
}

entity::model::StreamInputCounters* ControlledEntityImpl::getStreamInputCounters(entity::model::StreamIndex const streamIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getStreamInputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
		if (dynamicModel)
		{
			// Create counters if they don't exist yet
			if (!dynamicModel->counters)
			{
				dynamicModel->counters = entity::model::StreamInputCounters{};
			}
			return &(*dynamicModel->counters);
		}
	}
	return nullptr;
}

entity::model::StreamOutputCounters* ControlledEntityImpl::getStreamOutputCounters(entity::model::StreamIndex const streamIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getStreamOutputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
		if (dynamicModel)
		{
			// Create counters if they don't exist yet
			if (!dynamicModel->counters)
			{
				dynamicModel->counters = entity::model::StreamOutputCounters{};
			}
			return &(*dynamicModel->counters);
		}
	}
	return nullptr;
}

// Setters of the DescriptorDynamic info, default constructing if not existing
void ControlledEntityImpl::setEntityName(entity::model::AvdeccFixedString const& name, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto* const dynamicModel = _treeModelAccess->getEntityNodeDynamicModel(notFoundBehavior);
	if (dynamicModel)
	{
		dynamicModel->entityName = name;
	}
}

void ControlledEntityImpl::setEntityGroupName(entity::model::AvdeccFixedString const& name, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto* const dynamicModel = _treeModelAccess->getEntityNodeDynamicModel(notFoundBehavior);
	if (dynamicModel)
	{
		dynamicModel->groupName = name;
	}
}

void ControlledEntityImpl::setCurrentConfiguration(entity::model::ConfigurationIndex const configurationIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto* const entityNode = _treeModelAccess->getEntityNode(notFoundBehavior);
	if (entityNode)
	{
		if (entityNode->dynamicModel.currentConfiguration != configurationIndex)
		{
			entityNode->dynamicModel.currentConfiguration = configurationIndex;

			// Set isActiveConfiguration for each configuration
			for (auto& confIt : entityNode->configurations)
			{
				confIt.second.dynamicModel.isActiveConfiguration = configurationIndex == confIt.first;
			}
		}
	}
}

void ControlledEntityImpl::setConfigurationName(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto* const dynamicModel = _treeModelAccess->getConfigurationNodeDynamicModel(configurationIndex, notFoundBehavior);
	if (dynamicModel)
	{
		dynamicModel->objectName = name;
	}
}

void ControlledEntityImpl::setSamplingRate(entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getAudioUnitNodeDynamicModel(*currentConfigurationIndexOpt, audioUnitIndex, notFoundBehavior);
		if (dynamicModel)
		{
			dynamicModel->currentSamplingRate = samplingRate;
		}
	}
}

entity::model::StreamInputConnectionInfo ControlledEntityImpl::setStreamInputConnectionInformation(entity::model::StreamIndex const streamIndex, entity::model::StreamInputConnectionInfo const& info, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getStreamInputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
		if (dynamicModel)
		{
			// Save previous StreamInputConnectionInfo
			auto const previousInfo = dynamicModel->connectionInfo;

			// Set connection information
			dynamicModel->connectionInfo = info;

			return previousInfo;
		}
	}
	return entity::model::StreamInputConnectionInfo{};
}

void ControlledEntityImpl::clearStreamOutputConnections(entity::model::StreamIndex const streamIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getStreamOutputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
		if (dynamicModel)
		{
			dynamicModel->connections.clear();
		}
	}
}

bool ControlledEntityImpl::addStreamOutputConnection(entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& listenerStream, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getStreamOutputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
		if (dynamicModel)
		{
			auto const result = dynamicModel->connections.insert(listenerStream);
			return result.second;
		}
	}
	return false;
}

bool ControlledEntityImpl::delStreamOutputConnection(entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& listenerStream, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getStreamOutputNodeDynamicModel(*currentConfigurationIndexOpt, streamIndex, notFoundBehavior);
		if (dynamicModel)
		{
			return dynamicModel->connections.erase(listenerStream) > 0;
		}
	}
	return false;
}

entity::model::AvbInterfaceInfo ControlledEntityImpl::setAvbInterfaceInfo(entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInterfaceInfo const& info, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getAvbInterfaceNodeDynamicModel(*currentConfigurationIndexOpt, avbInterfaceIndex, notFoundBehavior);
		if (dynamicModel)
		{
			// Save previous AvbInfo
			auto previousInfo = dynamicModel->avbInterfaceInfo;

			// Set AvbInterfaceInfo
			dynamicModel->avbInterfaceInfo = info;

			return previousInfo ? *previousInfo : entity::model::AvbInterfaceInfo{};
		}
	}
	return entity::model::AvbInterfaceInfo{};
}

entity::model::AsPath ControlledEntityImpl::setAsPath(entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AsPath const& asPath, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getAvbInterfaceNodeDynamicModel(*currentConfigurationIndexOpt, avbInterfaceIndex, notFoundBehavior);
		if (dynamicModel)
		{
			// Save previous AsPath
			auto previousPath = dynamicModel->asPath;

			// Set AsPath
			dynamicModel->asPath = asPath;

			return previousPath ? *previousPath : entity::model::AsPath{};
		}
	}
	return entity::model::AsPath{};
}

void ControlledEntityImpl::setSelectedLocaleStringsIndexesRange(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const baseIndex, entity::model::StringsIndex const countIndexes, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto* const dynamicModel = _treeModelAccess->getConfigurationNodeDynamicModel(configurationIndex, notFoundBehavior);
	if (dynamicModel)
	{
		dynamicModel->selectedLocaleBaseIndex = baseIndex;
		dynamicModel->selectedLocaleCountIndexes = countIndexes;
	}
}

void ControlledEntityImpl::clearStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getStreamPortInputNodeDynamicModel(*currentConfigurationIndexOpt, streamPortIndex, notFoundBehavior);
		if (dynamicModel)
		{
			dynamicModel->dynamicAudioMap.clear();
		}
	}
}

void ControlledEntityImpl::addStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getStreamPortInputNodeDynamicModel(*currentConfigurationIndexOpt, streamPortIndex, notFoundBehavior);
		if (dynamicModel)
		{
			auto& dynamicMap = dynamicModel->dynamicAudioMap;

			// Process audio mappings
			for (auto const& map : mappings)
			{
				// Search for another mapping associated to the same destination (cluster), which is not allowed except in redundancy
				auto foundIt = std::find_if(dynamicMap.begin(), dynamicMap.end(),
					[&map](entity::model::AudioMapping const& mapping)
					{
						return (map.clusterOffset == mapping.clusterOffset) && (map.clusterChannel == mapping.clusterChannel);
					});
				// Not found, add the new mapping
				if (foundIt == dynamicMap.end())
				{
					dynamicMap.push_back(map);
				}
				else // Otherwise, replace the previous mapping (or add it as well, if redundancy feature is not enabled)
				{
					// Note: Not able to check if the stream is redundant (using the redundant property of the stream or the cached Primary/Secondary indexes) since we might receive mappings before having had the time to retrieve the descriptor or build the cache
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					// StreamChannel must be the same and StreamIndex must be different, in redundancy
					if ((foundIt->streamIndex != map.streamIndex) && (foundIt->streamChannel == map.streamChannel))
					{
						dynamicMap.push_back(map);
					}
					else
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					{
						if (*foundIt != map)
						{
							LOG_CONTROLLER_WARN(_entity.getEntityID(), std::string("Duplicate StreamPortInput AudioMappings found: ") + std::to_string(foundIt->streamIndex) + ":" + std::to_string(foundIt->streamChannel) + ":" + std::to_string(foundIt->clusterOffset) + ":" + std::to_string(foundIt->clusterChannel) + " replaced by " + std::to_string(map.streamIndex) + ":" + std::to_string(map.streamChannel) + ":" + std::to_string(map.clusterOffset) + ":" + std::to_string(map.clusterChannel));
							foundIt->streamIndex = map.streamIndex;
							foundIt->streamChannel = map.streamChannel;
						}
					}
				}
			}
		}
	}
}

void ControlledEntityImpl::removeStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getStreamPortInputNodeDynamicModel(*currentConfigurationIndexOpt, streamPortIndex, notFoundBehavior);
		if (dynamicModel)
		{
			auto& dynamicMap = dynamicModel->dynamicAudioMap;

			// Process audio mappings
			for (auto const& map : mappings)
			{
				// Check if mapping exists
				auto foundIt = std::find_if(dynamicMap.begin(), dynamicMap.end(),
					[&map](entity::model::AudioMapping const& mapping)
					{
						return map == mapping;
					});
				if (foundIt != dynamicMap.end())
				{
					dynamicMap.erase(foundIt);
				}
				else
				{
					LOG_CONTROLLER_WARN(_entity.getEntityID(), std::string("Removing non-existing StreamPortInput AudioMappings: ") + std::to_string(map.streamIndex) + ":" + std::to_string(map.streamChannel) + ":" + std::to_string(map.clusterOffset) + ":" + std::to_string(map.clusterChannel));
				}
			}
		}
	}
}

void ControlledEntityImpl::clearStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getStreamPortOutputNodeDynamicModel(*currentConfigurationIndexOpt, streamPortIndex, notFoundBehavior);
		if (dynamicModel)
		{
			dynamicModel->dynamicAudioMap.clear();
		}
	}
}

void ControlledEntityImpl::addStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getStreamPortOutputNodeDynamicModel(*currentConfigurationIndexOpt, streamPortIndex, notFoundBehavior);
		if (dynamicModel)
		{
			auto& dynamicMap = dynamicModel->dynamicAudioMap;

			// Process audio mappings
			for (auto const& map : mappings)
			{
				// Search for another mapping associated to the same destination (stream), which is not allowed
				auto foundIt = std::find_if(dynamicMap.begin(), dynamicMap.end(),
					[&map](entity::model::AudioMapping const& mapping)
					{
						return (map.streamIndex == mapping.streamIndex) && (map.streamChannel == mapping.streamChannel);
					});
				// Not found, add the new mapping
				if (foundIt == dynamicMap.end())
				{
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
					// TODO: If StreamIndex is redundant and the other Stream Pair is already in the map, validate it's the same ClusterIndex and ClusterChannel
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
					dynamicMap.push_back(map);
				}
				else // Otherwise, replace the previous mapping
				{
					if (*foundIt != map)
					{
						LOG_CONTROLLER_WARN(_entity.getEntityID(), std::string("Duplicate StreamPortOutput AudioMappings found: ") + std::to_string(foundIt->streamIndex) + ":" + std::to_string(foundIt->streamChannel) + ":" + std::to_string(foundIt->clusterOffset) + ":" + std::to_string(foundIt->clusterChannel) + " replaced by " + std::to_string(map.streamIndex) + ":" + std::to_string(map.streamChannel) + ":" + std::to_string(map.clusterOffset) + ":" + std::to_string(map.clusterChannel));
						foundIt->clusterOffset = map.clusterOffset;
						foundIt->clusterChannel = map.clusterChannel;
					}
				}
			}
		}
	}
}

void ControlledEntityImpl::removeStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getStreamPortOutputNodeDynamicModel(*currentConfigurationIndexOpt, streamPortIndex, notFoundBehavior);
		if (dynamicModel)
		{
			auto& dynamicMap = dynamicModel->dynamicAudioMap;

			// Process audio mappings
			for (auto const& map : mappings)
			{
				// Check if mapping exists
				auto foundIt = std::find_if(dynamicMap.begin(), dynamicMap.end(),
					[&map](entity::model::AudioMapping const& mapping)
					{
						return map == mapping;
					});
				if (foundIt != dynamicMap.end())
				{
					dynamicMap.erase(foundIt);
				}
				else
				{
					LOG_CONTROLLER_WARN(_entity.getEntityID(), std::string("Removing non-existing StreamPortOutput AudioMappings: ") + std::to_string(map.streamIndex) + ":" + std::to_string(map.streamChannel) + ":" + std::to_string(map.clusterOffset) + ":" + std::to_string(map.clusterChannel));
				}
			}
		}
	}
}

void ControlledEntityImpl::setClockSource(entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getClockDomainNodeDynamicModel(*currentConfigurationIndexOpt, clockDomainIndex, notFoundBehavior);
		if (dynamicModel)
		{
			dynamicModel->clockSourceIndex = clockSourceIndex;
		}
	}
}

void ControlledEntityImpl::setControlValues(entity::model::ControlIndex const controlIndex, entity::model::ControlValues const& controlValues, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto const currentConfigurationIndexOpt = getCurrentConfigurationIndex(notFoundBehavior);
	if (currentConfigurationIndexOpt)
	{
		auto* const dynamicModel = _treeModelAccess->getControlNodeDynamicModel(*currentConfigurationIndexOpt, controlIndex, notFoundBehavior);
		if (dynamicModel)
		{
			dynamicModel->values = controlValues;
		}
	}
}

void ControlledEntityImpl::setMemoryObjectLength(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, TreeModelAccessStrategy::NotFoundBehavior const notFoundBehavior)
{
	auto* const dynamicModel = _treeModelAccess->getMemoryObjectNodeDynamicModel(configurationIndex, memoryObjectIndex, notFoundBehavior);
	if (dynamicModel)
	{
		dynamicModel->length = length;
	}
}

// Setters of the global state
void ControlledEntityImpl::setEntity(entity::Entity const& entity) noexcept
{
	_entity = entity;
}

ControlledEntity::InterfaceLinkStatus ControlledEntityImpl::setAvbInterfaceLinkStatus(entity::model::AvbInterfaceIndex const avbInterfaceIndex, InterfaceLinkStatus const linkStatus) noexcept
{
	// Previous link status (unknown by default)
	auto previousStatus = InterfaceLinkStatus::Unknown;

	auto it = _avbInterfaceLinkStatus.find(avbInterfaceIndex);

	// Entry existed
	if (it != _avbInterfaceLinkStatus.end())
	{
		// Get previous link status
		previousStatus = it->second;

		// Set new link status
		it->second = linkStatus;
	}
	else
	{
		// Insert a new entry
		_avbInterfaceLinkStatus.emplace(std::make_pair(avbInterfaceIndex, linkStatus));
	}
	return previousStatus;
}

void ControlledEntityImpl::setAcquireState(model::AcquireState const state) noexcept
{
	_acquireState = state;
}

void ControlledEntityImpl::setOwningController(UniqueIdentifier const controllerID) noexcept
{
	_owningControllerID = controllerID;
}

void ControlledEntityImpl::setLockState(model::LockState const state) noexcept
{
	_lockState = state;
}
void ControlledEntityImpl::setLockingController(UniqueIdentifier const controllerID) noexcept
{
	_lockingControllerID = controllerID;
}

void ControlledEntityImpl::setMilanInfo(entity::model::MilanInfo const& info) noexcept
{
	_milanInfo = info;
}

// Setters of the Statistics
void ControlledEntityImpl::setAecpRetryCounter(std::uint64_t const value) noexcept
{
	_aecpRetryCounter = value;
}

void ControlledEntityImpl::setAecpTimeoutCounter(std::uint64_t const value) noexcept
{
	_aecpTimeoutCounter = value;
}

void ControlledEntityImpl::setAecpUnexpectedResponseCounter(std::uint64_t const value) noexcept
{
	_aecpUnexpectedResponseCounter = value;
}

void ControlledEntityImpl::setAecpResponseAverageTime(std::chrono::milliseconds const& value) noexcept
{
	_aecpResponseAverageTime = value;
}

void ControlledEntityImpl::setAemAecpUnsolicitedCounter(std::uint64_t const value) noexcept
{
	_aemAecpUnsolicitedCounter = value;
}

void ControlledEntityImpl::setAemAecpUnsolicitedLossCounter(std::uint64_t const value) noexcept
{
	_aemAecpUnsolicitedLossCounter = value;
}

void ControlledEntityImpl::setEnumerationTime(std::chrono::milliseconds const& value) noexcept
{
	_enumerationTime = value;
}

// Setters of the Diagnostics
void ControlledEntityImpl::setDiagnostics(Diagnostics const& diags) noexcept
{
	_diagnostics = diags;
}

// Setters of the Model from AEM Descriptors (including DescriptorDynamic info)
bool ControlledEntityImpl::setCachedEntityNode(model::EntityNode&& cachedNode, entity::model::EntityDescriptor const& descriptor, bool const forAllConfiguration) noexcept
{
	// Check if static information in EntityDescriptor are identical
	auto const& cachedDescriptor = cachedNode.staticModel;
	if (cachedDescriptor.vendorNameString != descriptor.vendorNameString || cachedDescriptor.modelNameString != descriptor.modelNameString)
	{
		LOG_CONTROLLER_WARN(_entity.getEntityID(), "EntityModelID provided by this Entity has inconsistent data in it's EntityDescriptor, not using cached AEM");
		return false;
	}

	if (forAllConfiguration)
	{
		// Check if Cached Model is valid for ALL configurations
		if (!isEntityModelComplete(cachedNode, descriptor.configurationsCount))
		{
			LOG_CONTROLLER_WARN(_entity.getEntityID(), "Cached EntityModel does not provide model for configuration {}, not using cached AEM", descriptor.currentConfiguration);
			return false;
		}
	}
	else
	{
		auto const configNodeIt = cachedNode.configurations.find(descriptor.currentConfiguration);

		if (configNodeIt == cachedNode.configurations.end() || !EntityModelCache::isModelValidForConfiguration(configNodeIt->second))
		{
			LOG_CONTROLLER_WARN(_entity.getEntityID(), "Cached EntityModel does not provide model for configuration {}, not using cached AEM", descriptor.currentConfiguration);
			return false;
		}
	}

	// Ok the static information from EntityDescriptor are identical, we cannot check more than this so we have to assume it's correct, copy the whole model
	_entityNode = std::move(cachedNode);

	// Success, switch to Cached Model Strategy
	switchToCachedTreeModelAccessStrategy();

	// And override with the EntityDescriptor so this entity's specific fields are copied
	setEntityDescriptor(descriptor);

	return true;
}

void ControlledEntityImpl::setEntityDescriptor(entity::model::EntityDescriptor const& descriptor) noexcept
{
	if (!AVDECC_ASSERT_WITH_RET(!_advertised, "EntityDescriptor should never be set twice on an entity. Only the dynamic part should be set again."))
	{
		// Wipe everything and set as enumeration error
		_entityNode = {};
		_gotFatalEnumerateError = true;

		return;
	}

	// Copy static model
	{
		auto& m = _entityNode.staticModel;
		m.vendorNameString = descriptor.vendorNameString;
		m.modelNameString = descriptor.modelNameString;
	}

	// Copy dynamic model
	{
		auto& m = _entityNode.dynamicModel;
		// Not changeable fields
		m.firmwareVersion = descriptor.firmwareVersion;
		m.serialNumber = descriptor.serialNumber;
		// Changeable fields through commands
		m.entityName = descriptor.entityName;
		m.groupName = descriptor.groupName;
		m.currentConfiguration = descriptor.currentConfiguration;
	}
}

void ControlledEntityImpl::setConfigurationDescriptor(entity::model::ConfigurationDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	// Get or create a new ConfigurationNode for this entity
	auto* const node = _treeModelAccess->getConfigurationNode(configurationIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.descriptorCounts = descriptor.descriptorCounts;
	}

	// Copy dynamic model
	{
		auto const* const entityDynamicModel = _treeModelAccess->getEntityNodeDynamicModel(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
		if (!AVDECC_ASSERT_WITH_RET(!!entityDynamicModel, "Should not be null, parent descriptor expected to be created before this one"))
		{
			return;
		}
		auto& m = node->dynamicModel;
		// Changeable fields through commands
		m.isActiveConfiguration = configurationIndex == entityDynamicModel->currentConfiguration;
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setAudioUnitDescriptor(entity::model::AudioUnitDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex) noexcept
{
	// Get or create a new AudioUnitNode for this entity
	auto* const node = _treeModelAccess->getAudioUnitNode(configurationIndex, audioUnitIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.clockDomainIndex = descriptor.clockDomainIndex;
		m.numberOfStreamInputPorts = descriptor.numberOfStreamInputPorts;
		m.baseStreamInputPort = descriptor.baseStreamInputPort;
		m.numberOfStreamOutputPorts = descriptor.numberOfStreamOutputPorts;
		m.baseStreamOutputPort = descriptor.baseStreamOutputPort;
		m.numberOfExternalInputPorts = descriptor.numberOfExternalInputPorts;
		m.baseExternalInputPort = descriptor.baseExternalInputPort;
		m.numberOfExternalOutputPorts = descriptor.numberOfExternalOutputPorts;
		m.baseExternalOutputPort = descriptor.baseExternalOutputPort;
		m.numberOfInternalInputPorts = descriptor.numberOfInternalInputPorts;
		m.baseInternalInputPort = descriptor.baseInternalInputPort;
		m.numberOfInternalOutputPorts = descriptor.numberOfInternalOutputPorts;
		m.baseInternalOutputPort = descriptor.baseInternalOutputPort;
		m.numberOfControls = descriptor.numberOfControls;
		m.baseControl = descriptor.baseControl;
		m.numberOfSignalSelectors = descriptor.numberOfSignalSelectors;
		m.baseSignalSelector = descriptor.baseSignalSelector;
		m.numberOfMixers = descriptor.numberOfMixers;
		m.baseMixer = descriptor.baseMixer;
		m.numberOfMatrices = descriptor.numberOfMatrices;
		m.baseMatrix = descriptor.baseMatrix;
		m.numberOfSplitters = descriptor.numberOfSplitters;
		m.baseSplitter = descriptor.baseSplitter;
		m.numberOfCombiners = descriptor.numberOfCombiners;
		m.baseCombiner = descriptor.baseCombiner;
		m.numberOfDemultiplexers = descriptor.numberOfDemultiplexers;
		m.baseDemultiplexer = descriptor.baseDemultiplexer;
		m.numberOfMultiplexers = descriptor.numberOfMultiplexers;
		m.baseMultiplexer = descriptor.baseMultiplexer;
		m.numberOfTranscoders = descriptor.numberOfTranscoders;
		m.baseTranscoder = descriptor.baseTranscoder;
		m.numberOfControlBlocks = descriptor.numberOfControlBlocks;
		m.baseControlBlock = descriptor.baseControlBlock;
		m.samplingRates = descriptor.samplingRates;
	}

	// Copy dynamic model
	{
		auto& m = node->dynamicModel;
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
		m.currentSamplingRate = descriptor.currentSamplingRate;
	}
}

void ControlledEntityImpl::setStreamInputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) noexcept
{
	// Get or create a new StreamInputNode for this entity
	auto* const node = _treeModelAccess->getStreamInputNode(configurationIndex, streamIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.clockDomainIndex = descriptor.clockDomainIndex;
		m.streamFlags = descriptor.streamFlags;
		m.backupTalkerEntityID_0 = descriptor.backupTalkerEntityID_0;
		m.backupTalkerUniqueID_0 = descriptor.backupTalkerUniqueID_0;
		m.backupTalkerEntityID_1 = descriptor.backupTalkerEntityID_1;
		m.backupTalkerUniqueID_1 = descriptor.backupTalkerUniqueID_1;
		m.backupTalkerEntityID_2 = descriptor.backupTalkerEntityID_2;
		m.backupTalkerUniqueID_2 = descriptor.backupTalkerUniqueID_2;
		m.backedupTalkerEntityID = descriptor.backedupTalkerEntityID;
		m.backedupTalkerUnique = descriptor.backedupTalkerUnique;
		m.avbInterfaceIndex = descriptor.avbInterfaceIndex;
		m.bufferLength = descriptor.bufferLength;
		m.formats = descriptor.formats;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
		m.redundantStreams = descriptor.redundantStreams;
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
	}

	// Copy dynamic model
	{
		auto& m = node->dynamicModel;
		// Not changeable fields
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
		m.streamFormat = descriptor.currentFormat;
	}
}

void ControlledEntityImpl::setStreamOutputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) noexcept
{
	// Get or create a new StreamOutputNode for this entity
	auto* const node = _treeModelAccess->getStreamOutputNode(configurationIndex, streamIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.clockDomainIndex = descriptor.clockDomainIndex;
		m.streamFlags = descriptor.streamFlags;
		m.backupTalkerEntityID_0 = descriptor.backupTalkerEntityID_0;
		m.backupTalkerUniqueID_0 = descriptor.backupTalkerUniqueID_0;
		m.backupTalkerEntityID_1 = descriptor.backupTalkerEntityID_1;
		m.backupTalkerUniqueID_1 = descriptor.backupTalkerUniqueID_1;
		m.backupTalkerEntityID_2 = descriptor.backupTalkerEntityID_2;
		m.backupTalkerUniqueID_2 = descriptor.backupTalkerUniqueID_2;
		m.backedupTalkerEntityID = descriptor.backedupTalkerEntityID;
		m.backedupTalkerUnique = descriptor.backedupTalkerUnique;
		m.avbInterfaceIndex = descriptor.avbInterfaceIndex;
		m.bufferLength = descriptor.bufferLength;
		m.formats = descriptor.formats;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
		m.redundantStreams = descriptor.redundantStreams;
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
	}

	// Copy dynamic model
	{
		auto& m = node->dynamicModel;
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
		m.streamFormat = descriptor.currentFormat;
	}
}

void ControlledEntityImpl::setJackInputDescriptor(entity::model::JackDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex) noexcept
{
	// Get or create a new JackInputNode for this entity
	auto* const node = _treeModelAccess->getJackInputNode(configurationIndex, jackIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.jackFlags = descriptor.jackFlags;
		m.jackType = descriptor.jackType;
	}

	// Copy dynamic model
	{
		auto& m = node->dynamicModel;
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setJackOutputDescriptor(entity::model::JackDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex) noexcept
{
	// Get or create a new JackOutputNode for this entity
	auto* const node = _treeModelAccess->getJackOutputNode(configurationIndex, jackIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.jackFlags = descriptor.jackFlags;
		m.jackType = descriptor.jackType;
	}

	// Copy dynamic model
	{
		auto& m = node->dynamicModel;
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setAvbInterfaceDescriptor(entity::model::AvbInterfaceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const interfaceIndex) noexcept
{
	// Get or create a new AvbInterfaceNode for this entity
	auto* const node = _treeModelAccess->getAvbInterfaceNode(configurationIndex, interfaceIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.macAddress = descriptor.macAddress;
		m.interfaceFlags = descriptor.interfaceFlags;
		m.clockIdentity = descriptor.clockIdentity;
		m.priority1 = descriptor.priority1;
		m.clockClass = descriptor.clockClass;
		m.offsetScaledLogVariance = descriptor.offsetScaledLogVariance;
		m.clockAccuracy = descriptor.clockAccuracy;
		m.priority2 = descriptor.priority2;
		m.domainNumber = descriptor.domainNumber;
		m.logSyncInterval = descriptor.logSyncInterval;
		m.logAnnounceInterval = descriptor.logAnnounceInterval;
		m.logPDelayInterval = descriptor.logPDelayInterval;
		m.portNumber = descriptor.portNumber;
	}

	// Copy dynamic model
	{
		auto& m = node->dynamicModel;
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setClockSourceDescriptor(entity::model::ClockSourceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockIndex) noexcept
{
	// Get or create a new ClockSourceNode for this entity
	auto* const node = _treeModelAccess->getClockSourceNode(configurationIndex, clockIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.clockSourceType = descriptor.clockSourceType;
		m.clockSourceLocationType = descriptor.clockSourceLocationType;
		m.clockSourceLocationIndex = descriptor.clockSourceLocationIndex;
	}

	// Copy dynamic model
	{
		auto& m = node->dynamicModel;
		// Not changeable fields
		m.clockSourceFlags = descriptor.clockSourceFlags;
		m.clockSourceIdentifier = descriptor.clockSourceIdentifier;
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setMemoryObjectDescriptor(entity::model::MemoryObjectDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex) noexcept
{
	// Get or create a new MemoryObjectNode for this entity
	auto* const node = _treeModelAccess->getMemoryObjectNode(configurationIndex, memoryObjectIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.memoryObjectType = descriptor.memoryObjectType;
		m.targetDescriptorType = descriptor.targetDescriptorType;
		m.targetDescriptorIndex = descriptor.targetDescriptorIndex;
		m.startAddress = descriptor.startAddress;
		m.maximumLength = descriptor.maximumLength;
	}

	// Copy dynamic model
	{
		auto& m = node->dynamicModel;
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
		m.length = descriptor.length;
	}
}

void ControlledEntityImpl::setLocaleDescriptor(entity::model::LocaleDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex) noexcept
{
	// Get or create a new LocaleNode for this entity
	auto* const node = _treeModelAccess->getLocaleNode(configurationIndex, localeIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localeID = descriptor.localeID;
		m.numberOfStringDescriptors = descriptor.numberOfStringDescriptors;
		m.baseStringDescriptorIndex = descriptor.baseStringDescriptorIndex;
	}
}

void ControlledEntityImpl::setStringsDescriptor(entity::model::StringsDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex) noexcept
{
	// Get or create a new StringsNode for this entity
	auto* const node = _treeModelAccess->getStringsNode(configurationIndex, stringsIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.strings = descriptor.strings;
	}

	auto const* const configDynamicModel = _treeModelAccess->getConfigurationNodeDynamicModel(configurationIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
	if (!AVDECC_ASSERT_WITH_RET(!!configDynamicModel, "Should not be null, parent descriptor expected to be created before this one"))
	{
		return;
	}
	// This StringsIndex is part of the active Locale
	if (configDynamicModel->selectedLocaleCountIndexes > 0 && stringsIndex >= configDynamicModel->selectedLocaleBaseIndex && stringsIndex < (configDynamicModel->selectedLocaleBaseIndex + configDynamicModel->selectedLocaleCountIndexes))
	{
		setLocalizedStrings(configurationIndex, stringsIndex - configDynamicModel->selectedLocaleBaseIndex, descriptor.strings);
	}
}

void ControlledEntityImpl::setLocalizedStrings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const relativeStringsIndex, entity::model::AvdeccFixedStrings const& strings) noexcept
{
	auto* const configDynamicModel = _treeModelAccess->getConfigurationNodeDynamicModel(configurationIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
	if (!AVDECC_ASSERT_WITH_RET(!!configDynamicModel, "Should not be null, parent descriptor expected to be created before this one"))
	{
		return;
	}
	// Copy the strings to the ConfigurationDynamicModel for a quick access
	for (auto strIndex = 0u; strIndex < strings.size(); ++strIndex)
	{
		auto const localizedStringIndex = entity::model::LocalizedStringReference{ relativeStringsIndex, static_cast<std::uint8_t>(strIndex) }.getGlobalOffset();
		configDynamicModel->localizedStrings[localizedStringIndex] = strings.at(strIndex);
	}
}

void ControlledEntityImpl::setStreamPortInputDescriptor(entity::model::StreamPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) noexcept
{
	// Get or create a new StreamPortInputNode for this entity
	auto* const node = _treeModelAccess->getStreamPortInputNode(configurationIndex, streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.clockDomainIndex = descriptor.clockDomainIndex;
		m.portFlags = descriptor.portFlags;
		m.numberOfControls = descriptor.numberOfControls;
		m.baseControl = descriptor.baseControl;
		m.numberOfClusters = descriptor.numberOfClusters;
		m.baseCluster = descriptor.baseCluster;
		m.numberOfMaps = descriptor.numberOfMaps;
		m.baseMap = descriptor.baseMap;
		m.hasDynamicAudioMap = m.numberOfMaps == 0;
	}

	// Nothing to copy in dynamic model
}

void ControlledEntityImpl::setStreamPortOutputDescriptor(entity::model::StreamPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) noexcept
{
	// Get or create a new StreamPortOutputNode for this entity
	auto* const node = _treeModelAccess->getStreamPortOutputNode(configurationIndex, streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.clockDomainIndex = descriptor.clockDomainIndex;
		m.portFlags = descriptor.portFlags;
		m.numberOfControls = descriptor.numberOfControls;
		m.baseControl = descriptor.baseControl;
		m.numberOfClusters = descriptor.numberOfClusters;
		m.baseCluster = descriptor.baseCluster;
		m.numberOfMaps = descriptor.numberOfMaps;
		m.baseMap = descriptor.baseMap;
		m.hasDynamicAudioMap = m.numberOfMaps == 0;
	}

	// Nothing to copy in dynamic model
}

void ControlledEntityImpl::setAudioClusterDescriptor(entity::model::AudioClusterDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex) noexcept
{
	// Get or create a new AudioClusterNode for this entity
	auto* const node = _treeModelAccess->getAudioClusterNode(configurationIndex, clusterIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.signalType = descriptor.signalType;
		m.signalIndex = descriptor.signalIndex;
		m.signalOutput = descriptor.signalOutput;
		m.pathLatency = descriptor.pathLatency;
		m.blockLatency = descriptor.blockLatency;
		m.channelCount = descriptor.channelCount;
		m.format = descriptor.format;
	}

	// Copy dynamic model
	{
		auto& m = node->dynamicModel;
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setAudioMapDescriptor(entity::model::AudioMapDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex) noexcept
{
	// Get or create a new AudioMapNode for this entity
	auto* const node = _treeModelAccess->getAudioMapNode(configurationIndex, mapIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.mappings = descriptor.mappings;
	}
}

void ControlledEntityImpl::setControlDescriptor(entity::model::ControlDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex) noexcept
{
	// Get or create a new ControlNode for this entity
	auto* const node = _treeModelAccess->getControlNode(configurationIndex, controlIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct, TreeModelAccessStrategy::DefaultConstructLevelHint::None);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;

		m.blockLatency = descriptor.blockLatency;
		m.controlLatency = descriptor.controlLatency;
		m.controlDomain = descriptor.controlDomain;
		m.controlType = descriptor.controlType;
		m.resetTime = descriptor.resetTime;
		m.signalType = descriptor.signalType;
		m.signalIndex = descriptor.signalIndex;
		m.signalOutput = descriptor.signalOutput;
		m.controlValueType = descriptor.controlValueType;
		m.numberOfValues = descriptor.numberOfValues;
		m.values = descriptor.valuesStatic;
	}

	// Copy dynamic model
	{
		auto& m = node->dynamicModel;
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
		m.values = descriptor.valuesDynamic;
	}
}

void ControlledEntityImpl::setClockDomainDescriptor(entity::model::ClockDomainDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex) noexcept
{
	// Get or create a new ClockDomainNode for this entity
	auto* const node = _treeModelAccess->getClockDomainNode(configurationIndex, clockDomainIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.clockSources = descriptor.clockSources;
	}

	// Copy dynamic model
	{
		auto& m = node->dynamicModel;
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
		m.clockSourceIndex = descriptor.clockSourceIndex;
	}
}

void ControlledEntityImpl::setTimingDescriptor(entity::model::TimingDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex) noexcept
{
	// Get or create a new TimingNode for this entity
	auto* const node = _treeModelAccess->getTimingNode(configurationIndex, timingIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.algorithm = descriptor.algorithm;
		m.ptpInstances = descriptor.ptpInstances;
	}

	// Copy dynamic model
	{
		auto& m = node->dynamicModel;
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setPtpInstanceDescriptor(entity::model::PtpInstanceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex) noexcept
{
	// Get or create a new PtpInstanceNode for this entity
	auto* const node = _treeModelAccess->getPtpInstanceNode(configurationIndex, ptpInstanceIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.clockIdentity = descriptor.clockIdentity;
		m.flags = descriptor.flags;
		m.numberOfControls = descriptor.numberOfControls;
		m.baseControl = descriptor.baseControl;
		m.numberOfPtpPorts = descriptor.numberOfPtpPorts;
		m.basePtpPort = descriptor.basePtpPort;
	}

	// Copy dynamic model
	{
		auto& m = node->dynamicModel;
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setPtpPortDescriptor(entity::model::PtpPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex) noexcept
{
	// Get or create a new PtpPortNode for this entity
	auto* const node = _treeModelAccess->getPtpPortNode(configurationIndex, ptpPortIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
	AVDECC_ASSERT(!!node, "Should not be null, should be default constructed");

	// Copy static model
	{
		auto& m = node->staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.portNumber = descriptor.portNumber;
		m.portType = descriptor.portType;
		m.flags = descriptor.flags;
		m.avbInterfaceIndex = descriptor.avbInterfaceIndex;
		m.profileIdentifier = descriptor.profileIdentifier;
	}

	// Copy dynamic model
	{
		auto& m = node->dynamicModel;
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
	}
}

// Setters of statistics
std::uint64_t ControlledEntityImpl::incrementAecpRetryCounter() noexcept
{
	++_aecpRetryCounter;
	return _aecpRetryCounter;
}

std::uint64_t ControlledEntityImpl::incrementAecpTimeoutCounter() noexcept
{
	++_aecpTimeoutCounter;
	return _aecpTimeoutCounter;
}

std::uint64_t ControlledEntityImpl::incrementAecpUnexpectedResponseCounter() noexcept
{
	++_aecpUnexpectedResponseCounter;
	return _aecpUnexpectedResponseCounter;
}

std::chrono::milliseconds const& ControlledEntityImpl::updateAecpResponseTimeAverage(std::chrono::milliseconds const& responseTime) noexcept
{
	++_aecpResponsesCount;
	_aecpResponseTimeSum += responseTime;
	_aecpResponseAverageTime = _aecpResponseTimeSum / _aecpResponsesCount;

	return _aecpResponseAverageTime;
}

std::uint64_t ControlledEntityImpl::incrementAemAecpUnsolicitedCounter() noexcept
{
	++_aemAecpUnsolicitedCounter;
	return _aemAecpUnsolicitedCounter;
}

std::uint64_t ControlledEntityImpl::incrementAemAecpUnsolicitedLossCounter() noexcept
{
	++_aemAecpUnsolicitedLossCounter;
	return _aemAecpUnsolicitedLossCounter;
}

void ControlledEntityImpl::setStartEnumerationTime(std::chrono::time_point<std::chrono::steady_clock>&& startTime) noexcept
{
	_enumerationStartTime = std::move(startTime);
}

void ControlledEntityImpl::setEndEnumerationTime(std::chrono::time_point<std::chrono::steady_clock>&& endTime) noexcept
{
	_enumerationTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - _enumerationStartTime);
}

// Expected RegisterUnsol query methods
bool ControlledEntityImpl::checkAndClearExpectedRegisterUnsol() noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	// Ignore if we had a fatal enumeration error
	if (_gotFatalEnumerateError)
		return false;

	auto const wasExpected = _expectedRegisterUnsol;
	_expectedRegisterUnsol = false;

	return wasExpected;
}

void ControlledEntityImpl::setRegisterUnsolExpected() noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	_expectedRegisterUnsol = true;
}

bool ControlledEntityImpl::gotExpectedRegisterUnsol() const noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	return !_expectedRegisterUnsol;
}

std::pair<bool, std::chrono::milliseconds> ControlledEntityImpl::getRegisterUnsolRetryTimer() noexcept
{
	++_registerUnsolRetryCount;
	if (_registerUnsolRetryCount > MaxRegisterUnsolRetryCount)
	{
		return std::make_pair(false, std::chrono::milliseconds{ 0 });
	}
	return std::make_pair(true, std::chrono::milliseconds{ QueryRetryMillisecondDelay });
}

// Expected Milan info query methods
static inline ControlledEntityImpl::MilanInfoKey makeMilanInfoKey(ControlledEntityImpl::MilanInfoType const milanInfoType)
{
	return static_cast<ControlledEntityImpl::MilanInfoKey>(utils::to_integral(milanInfoType));
}

bool ControlledEntityImpl::checkAndClearExpectedMilanInfo(MilanInfoType const milanInfoType) noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	// Ignore if we had a fatal enumeration error
	if (_gotFatalEnumerateError)
		return false;

	auto const key = makeMilanInfoKey(milanInfoType);
	return _expectedMilanInfo.erase(key) == 1;
}

void ControlledEntityImpl::setMilanInfoExpected(MilanInfoType const milanInfoType) noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	auto const key = makeMilanInfoKey(milanInfoType);
	_expectedMilanInfo.insert(key);
}

bool ControlledEntityImpl::gotAllExpectedMilanInfo() const noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	return _expectedMilanInfo.empty();
}

std::pair<bool, std::chrono::milliseconds> ControlledEntityImpl::getQueryMilanInfoRetryTimer() noexcept
{
	++_queryMilanInfoRetryCount;
	if (_queryMilanInfoRetryCount >= MaxQueryMilanInfoRetryCount)
	{
		return std::make_pair(false, std::chrono::milliseconds{ 0 });
	}
	return std::make_pair(true, std::chrono::milliseconds{ QueryRetryMillisecondDelay });
}

// Expected descriptor query methods
static inline ControlledEntityImpl::DescriptorKey makeDescriptorKey(entity::model::DescriptorType descriptorType, entity::model::DescriptorIndex descriptorIndex)
{
	return (utils::to_integral(descriptorType) << (sizeof(descriptorIndex) * 8)) + descriptorIndex;
}

bool ControlledEntityImpl::checkAndClearExpectedDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	// Ignore if we had a fatal enumeration error
	if (_gotFatalEnumerateError)
		return false;

	auto const confIt = _expectedDescriptors.find(configurationIndex);

	if (confIt == _expectedDescriptors.end())
		return false;

	auto const key = makeDescriptorKey(descriptorType, descriptorIndex);

	return confIt->second.erase(key) == 1;
}

void ControlledEntityImpl::setDescriptorExpected(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	auto& conf = _expectedDescriptors[configurationIndex];

	auto const key = makeDescriptorKey(descriptorType, descriptorIndex);
	conf.insert(key);
}

bool ControlledEntityImpl::gotAllExpectedDescriptors() const noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	for (auto const& confKV : _expectedDescriptors)
	{
		if (!confKV.second.empty())
			return false;
	}
	return true;
}

std::pair<bool, std::chrono::milliseconds> ControlledEntityImpl::getQueryDescriptorRetryTimer() noexcept
{
	++_queryDescriptorRetryCount;
	if (_queryDescriptorRetryCount >= MaxQueryDescriptorRetryCount)
	{
		return std::make_pair(false, std::chrono::milliseconds{ 0 });
	}
	return std::make_pair(true, std::chrono::milliseconds{ QueryRetryMillisecondDelay });
}

// Expected dynamic info query methods
static inline ControlledEntityImpl::DynamicInfoKey makeDynamicInfoKey(ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex)
{
	return (static_cast<ControlledEntityImpl::DynamicInfoKey>(utils::to_integral(dynamicInfoType)) << ((sizeof(descriptorIndex) + sizeof(subIndex)) * 8)) + (descriptorIndex << (sizeof(subIndex) * 8)) + subIndex;
}

bool ControlledEntityImpl::checkAndClearExpectedDynamicInfo(entity::model::ConfigurationIndex const configurationIndex, DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex) noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	// Ignore if we had a fatal enumeration error
	if (_gotFatalEnumerateError)
		return false;

	auto const confIt = _expectedDynamicInfo.find(configurationIndex);

	if (confIt == _expectedDynamicInfo.end())
		return false;

	auto const key = makeDynamicInfoKey(dynamicInfoType, descriptorIndex, subIndex);

	return confIt->second.erase(key) == 1;
}

void ControlledEntityImpl::setDynamicInfoExpected(entity::model::ConfigurationIndex const configurationIndex, DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex) noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	auto& conf = _expectedDynamicInfo[configurationIndex];

	auto const key = makeDynamicInfoKey(dynamicInfoType, descriptorIndex, subIndex);
	conf.insert(key);
}

bool ControlledEntityImpl::gotAllExpectedDynamicInfo() const noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	for (auto const& confKV : _expectedDynamicInfo)
	{
		if (!confKV.second.empty())
			return false;
	}
	return true;
}

std::pair<bool, std::chrono::milliseconds> ControlledEntityImpl::getQueryDynamicInfoRetryTimer() noexcept
{
	++_queryDynamicInfoRetryCount;
	if (_queryDynamicInfoRetryCount >= MaxQueryDynamicInfoRetryCount)
	{
		return std::make_pair(false, std::chrono::milliseconds{ 0 });
	}
	return std::make_pair(true, std::chrono::milliseconds{ QueryRetryMillisecondDelay });
}

// Expected descriptor dynamic info query methods
static inline ControlledEntityImpl::DescriptorDynamicInfoKey makeDescriptorDynamicInfoKey(ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex)
{
	return (static_cast<ControlledEntityImpl::DescriptorDynamicInfoKey>(utils::to_integral(descriptorDynamicInfoType)) << (sizeof(descriptorIndex) * 8)) + descriptorIndex;
}

bool ControlledEntityImpl::checkAndClearExpectedDescriptorDynamicInfo(entity::model::ConfigurationIndex const configurationIndex, DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	// Ignore if we had a fatal enumeration error
	if (_gotFatalEnumerateError)
		return false;

	auto const confIt = _expectedDescriptorDynamicInfo.find(configurationIndex);

	if (confIt == _expectedDescriptorDynamicInfo.end())
		return false;

	auto const key = makeDescriptorDynamicInfoKey(descriptorDynamicInfoType, descriptorIndex);

	return confIt->second.erase(key) == 1;
}

void ControlledEntityImpl::setDescriptorDynamicInfoExpected(entity::model::ConfigurationIndex const configurationIndex, DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	auto& conf = _expectedDescriptorDynamicInfo[configurationIndex];

	auto const key = makeDescriptorDynamicInfoKey(descriptorDynamicInfoType, descriptorIndex);
	conf.insert(key);
}

void ControlledEntityImpl::clearAllExpectedDescriptorDynamicInfo() noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	_expectedDescriptorDynamicInfo.clear();
}

bool ControlledEntityImpl::gotAllExpectedDescriptorDynamicInfo() const noexcept
{
	AVDECC_ASSERT(_sharedLock->_lockedCount >= 0, "ControlledEntity should be locked");

	for (auto const& confKV : _expectedDescriptorDynamicInfo)
	{
		if (!confKV.second.empty())
			return false;
	}
	return true;
}

std::pair<bool, std::chrono::milliseconds> ControlledEntityImpl::getQueryDescriptorDynamicInfoRetryTimer() noexcept
{
	++_queryDescriptorDynamicInfoRetryCount;
	if (_queryDescriptorDynamicInfoRetryCount >= MaxQueryDescriptorDynamicInfoRetryCount)
	{
		return std::make_pair(false, std::chrono::milliseconds{ 0 });
	}
	return std::make_pair(true, std::chrono::milliseconds{ QueryRetryMillisecondDelay });
}

entity::Entity& ControlledEntityImpl::getEntity() noexcept
{
	return _entity;
}

void ControlledEntityImpl::setIdentifyControlIndex(entity::model::ControlIndex const identifyControlIndex) noexcept
{
	_identifyControlIndex = identifyControlIndex;
}

bool ControlledEntityImpl::shouldIgnoreCachedEntityModel() const noexcept
{
	return _ignoreCachedEntityModel;
}

void ControlledEntityImpl::setIgnoreCachedEntityModel() noexcept
{
	_ignoreCachedEntityModel = true;
}

ControlledEntityImpl::EnumerationSteps ControlledEntityImpl::getEnumerationSteps() const noexcept
{
	return _enumerationSteps;
}

void ControlledEntityImpl::setEnumerationSteps(EnumerationSteps const steps) noexcept
{
	AVDECC_ASSERT(_enumerationSteps.empty(), "EnumerationSteps were not empty");
	_enumerationSteps = steps;
}

void ControlledEntityImpl::addEnumerationStep(EnumerationStep const step) noexcept
{
	_enumerationSteps.set(step);
}

void ControlledEntityImpl::clearEnumerationStep(EnumerationStep const step) noexcept
{
	_enumerationSteps.reset(step);
}

void ControlledEntityImpl::setCompatibilityFlags(CompatibilityFlags const compatibilityFlags) noexcept
{
	_compatibilityFlags = compatibilityFlags;
}

void ControlledEntityImpl::setMilanRedundant(bool const isMilanRedundant) noexcept
{
	_isMilanRedundant = isMilanRedundant;
}

void ControlledEntityImpl::setGetFatalEnumerationError() noexcept
{
	LOG_CONTROLLER_ERROR(_entity.getEntityID(), "Got Fatal Enumeration Error");
	_gotFatalEnumerateError = true;
}

void ControlledEntityImpl::setSubscribedToUnsolicitedNotifications(bool const isSubscribed) noexcept
{
	_isSubscribedToUnsolicitedNotifications = isSubscribed;

	// If unsubscribing, reset the expected sequence id
	if (!isSubscribed)
	{
		_expectedSequenceID = std::nullopt;
	}
}

void ControlledEntityImpl::setUnsolicitedNotificationsSupported(bool const isSupported) noexcept
{
	_areUnsolicitedNotificationsSupported = isSupported;
}

bool ControlledEntityImpl::wasAdvertised() const noexcept
{
	return _advertised;
}

void ControlledEntityImpl::setAdvertised(bool const wasAdvertised) noexcept
{
	_advertised = wasAdvertised;
}

bool ControlledEntityImpl::isRedundantPrimaryStreamInput(entity::model::StreamIndex const streamIndex) const noexcept
{
	return _redundantPrimaryStreamInputs.count(streamIndex) != 0;
}

bool ControlledEntityImpl::isRedundantPrimaryStreamOutput(entity::model::StreamIndex const streamIndex) const noexcept
{
	return _redundantPrimaryStreamOutputs.count(streamIndex) != 0;
}

bool ControlledEntityImpl::isRedundantSecondaryStreamInput(entity::model::StreamIndex const streamIndex) const noexcept
{
	return _redundantSecondaryStreamInputs.count(streamIndex) != 0;
}

bool ControlledEntityImpl::isRedundantSecondaryStreamOutput(entity::model::StreamIndex const streamIndex) const noexcept
{
	return _redundantSecondaryStreamOutputs.count(streamIndex) != 0;
}

ControlledEntity::Diagnostics& ControlledEntityImpl::getDiagnostics() noexcept
{
	return _diagnostics;
}

bool ControlledEntityImpl::hasLostUnsolicitedNotification(protocol::AecpSequenceID const sequenceID) noexcept
{
	auto unmatched = false;
	if (_isSubscribedToUnsolicitedNotifications && _milanInfo && _milanInfo->protocolVersion == 1)
	{
		// Compare received sequenceID and expected one, if it's not the first one received.
		// We don't expect 0 as first value, since the controller itself might restart (with the same entityID) without properly deregistering first,
		// in which case the entity will send unsolicited continuing the previous sequence.
		if (_expectedSequenceID.has_value())
		{
			unmatched = *_expectedSequenceID != sequenceID;
		}
		// Update next expected sequence ID
		_expectedSequenceID = static_cast<protocol::AecpSequenceID>(sequenceID + 1u);
	}
	return unmatched;
}

// Static methods
std::string ControlledEntityImpl::dynamicInfoTypeToString(DynamicInfoType const dynamicInfoType) noexcept
{
	switch (dynamicInfoType)
	{
		case DynamicInfoType::AcquiredState:
			return protocol::AemCommandType::AcquireEntity;
		case DynamicInfoType::LockedState:
			return protocol::AemCommandType::LockEntity;
		case DynamicInfoType::InputStreamAudioMappings:
			return static_cast<std::string>(protocol::AemCommandType::GetAudioMap) + " (STREAM_INPUT)";
		case DynamicInfoType::OutputStreamAudioMappings:
			return static_cast<std::string>(protocol::AemCommandType::GetAudioMap) + " (STREAM_OUTPUT)";
		case DynamicInfoType::InputStreamState:
			return protocol::AcmpMessageType::GetRxStateCommand;
		case DynamicInfoType::OutputStreamState:
			return protocol::AcmpMessageType::GetTxStateCommand;
		case DynamicInfoType::OutputStreamConnection:
			return protocol::AcmpMessageType::GetTxConnectionCommand;
		case DynamicInfoType::InputStreamInfo:
			return static_cast<std::string>(protocol::AemCommandType::GetStreamInfo) + " (STREAM_INPUT)";
		case DynamicInfoType::OutputStreamInfo:
			return static_cast<std::string>(protocol::AemCommandType::GetStreamInfo) + " (STREAM_OUTPUT)";
		case DynamicInfoType::GetAvbInfo:
			return protocol::AemCommandType::GetAvbInfo;
		case DynamicInfoType::GetAsPath:
			return protocol::AemCommandType::GetAsPath;
		case DynamicInfoType::GetEntityCounters:
			return static_cast<std::string>(protocol::AemCommandType::GetCounters) + " (ENTITY)";
		case DynamicInfoType::GetAvbInterfaceCounters:
			return static_cast<std::string>(protocol::AemCommandType::GetCounters) + " (AVB_INTERFACE)";
		case DynamicInfoType::GetClockDomainCounters:
			return static_cast<std::string>(protocol::AemCommandType::GetCounters) + " (CLOCK_DOMAIN)";
		case DynamicInfoType::GetStreamInputCounters:
			return static_cast<std::string>(protocol::AemCommandType::GetCounters) + " (STREAM_INPUT)";
		case DynamicInfoType::GetStreamOutputCounters:
			return static_cast<std::string>(protocol::AemCommandType::GetCounters) + " (STREAM_OUTPUT)";
		default:
			return "Unknown DynamicInfoType";
	}
}

std::string ControlledEntityImpl::descriptorDynamicInfoTypeToString(DescriptorDynamicInfoType const descriptorDynamicInfoType) noexcept
{
	switch (descriptorDynamicInfoType)
	{
		case DescriptorDynamicInfoType::ConfigurationName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (CONFIGURATION)";
		case DescriptorDynamicInfoType::AudioUnitName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (AUDIO_UNIT)";
		case DescriptorDynamicInfoType::AudioUnitSamplingRate:
			return static_cast<std::string>(protocol::AemCommandType::GetSamplingRate) + " (AUDIO_UNIT)";
		case DescriptorDynamicInfoType::InputStreamName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (STREAM_INPUT)";
		case DescriptorDynamicInfoType::InputStreamFormat:
			return static_cast<std::string>(protocol::AemCommandType::GetStreamFormat) + " (STREAM_INPUT)";
		case DescriptorDynamicInfoType::OutputStreamName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (STREAM_OUTPUT)";
		case DescriptorDynamicInfoType::OutputStreamFormat:
			return static_cast<std::string>(protocol::AemCommandType::GetStreamFormat) + " (STREAM_OUTPUT)";
		case DescriptorDynamicInfoType::InputJackName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (JACK_INPUT)";
		case DescriptorDynamicInfoType::OutputJackName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (JACK_OUTPUT)";
		case DescriptorDynamicInfoType::AvbInterfaceName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (AVB_INTERFACE)";
		case DescriptorDynamicInfoType::ClockSourceName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (CLOCK_SOURCE)";
		case DescriptorDynamicInfoType::MemoryObjectName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (MEMORY_OBJECT)";
		case DescriptorDynamicInfoType::MemoryObjectLength:
			return protocol::AemCommandType::GetMemoryObjectLength;
		case DescriptorDynamicInfoType::AudioClusterName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (AUDIO_CLUSTER)";
		case DescriptorDynamicInfoType::ControlName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (CONTROL)";
		case DescriptorDynamicInfoType::ControlValues:
			return protocol::AemCommandType::GetControl;
		case DescriptorDynamicInfoType::ClockDomainName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (CLOCK_DOMAIN)";
		case DescriptorDynamicInfoType::ClockDomainSourceIndex:
			return protocol::AemCommandType::GetClockSource;
		case DescriptorDynamicInfoType::TimingName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (TIMING)";
		case DescriptorDynamicInfoType::PtpInstanceName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (PTP_INSTANCE)";
		case DescriptorDynamicInfoType::PtpPortName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (PTP_PORT)";
		default:
			return "Unknown DescriptorDynamicInfoType";
	}
}

// Controller restricted methods
void ControlledEntityImpl::onEntityModelEnumerated() noexcept
{
	switchToCachedTreeModelAccessStrategy();
}

void ControlledEntityImpl::onEntityFullyLoaded() noexcept
{
	auto const& e = getEntity();
	//auto const entityID = e.getEntityID();
	auto const isAemSupported = e.getEntityCapabilities().test(entity::EntityCapability::AemSupported);

	// Save the enumeration time
	setEndEnumerationTime(std::chrono::steady_clock::now());

	// If AEM is supported
	if (isAemSupported)
	{
		// Build all virtual nodes (eg. RedundantStreams, ...)
		auto* entityNode = _treeModelAccess->getEntityNode(TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
		if (entityNode)
		{
			for (auto& configKV : entityNode->configurations)
			{
				buildVirtualNodes(configKV.second);
			}
		}
	}
}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
class RedundantHelper : public ControlledEntityImpl
{
public:
	template<typename StreamNodeType, typename RedundantNodeType>
	static void buildRedundancyNodesByType(ControlledEntityImpl const& entity, UniqueIdentifier entityID, std::map<entity::model::StreamIndex, StreamNodeType>& streams, std::map<model::VirtualIndex, RedundantNodeType>& redundantStreams, RedundantStreamCategory& redundantPrimaryStreams, RedundantStreamCategory& redundantSecondaryStreams)
	{
		for (auto& [streamIndex, streamNode] : streams)
		{
			auto const& staticModel = streamNode.staticModel;

			// Check if this node has redundant stream association
			if (staticModel.redundantStreams.empty())
			{
				continue;
			}

#	ifdef ENABLE_AVDECC_STRICT_2018_REDUNDANCY
			// 2018 Redundancy specification only defines stream pairs
			if (staticModel.redundantStreams.size() != 1)
			{
				LOG_CONTROLLER_WARN(entityID, std::string("More than one StreamIndex in RedundantStreamAssociation"));
				continue;
			}
#	endif // ENABLE_AVDECC_STRICT_2018_REDUNDANCY

			// Check each stream in the association is associated back to this stream and the AVB_INTERFACE index is unique
			auto isAssociationValid = true;
			auto redundantStreamNodes = std::map<entity::model::AvbInterfaceIndex, model::StreamNode*>{};
			redundantStreamNodes.emplace(std::make_pair(staticModel.avbInterfaceIndex, &streamNode));
			for (auto const redundantIndex : staticModel.redundantStreams)
			{
				auto const redundantStreamIt = streams.find(redundantIndex);

				// Referencing self
				if (redundantIndex == streamIndex)
				{
					isAssociationValid = false;
					LOG_CONTROLLER_ERROR(entityID, std::string("RedundantStreamAssociation invalid for ") + (streamNode.descriptorType == entity::model::DescriptorType::StreamInput ? "STREAM_INPUT." : "STREAM_OUTPUT.") + std::to_string(streamNode.descriptorIndex) + ": Referencing itself in RedundantAssociation set");
					break;
				}

				// Stream does not even exist
				if (redundantStreamIt == streams.end())
				{
					isAssociationValid = false;
					LOG_CONTROLLER_ERROR(entityID, std::string("RedundantStreamAssociation invalid for ") + (streamNode.descriptorType == entity::model::DescriptorType::StreamInput ? "STREAM_INPUT." : "STREAM_OUTPUT.") + std::to_string(streamNode.descriptorIndex) + ": StreamIndex " + std::to_string(redundantIndex) + " does not exist");
					break;
				}

				auto& redundantStream = redundantStreamIt->second;
				// Index not associated back
				if (redundantStream.staticModel.redundantStreams.find(streamIndex) == redundantStream.staticModel.redundantStreams.end())
				{
					isAssociationValid = false;
					LOG_CONTROLLER_ERROR(entityID, std::string("RedundantStreamAssociation invalid for ") + (streamNode.descriptorType == entity::model::DescriptorType::StreamInput ? "STREAM_INPUT." : "STREAM_OUTPUT.") + std::to_string(streamNode.descriptorIndex) + ": StreamIndex " + std::to_string(redundantIndex) + " doesn't reference back to the stream");
					break;
				}

				auto const redundantInterfaceIndex = redundantStream.staticModel.avbInterfaceIndex;
				// AVB_INTERFACE index already used
				if (redundantStreamNodes.find(redundantInterfaceIndex) != redundantStreamNodes.end())
				{
					isAssociationValid = false;
					LOG_CONTROLLER_ERROR(entityID, std::string("RedundantStreamAssociation invalid for ") + (streamNode.descriptorType == entity::model::DescriptorType::StreamInput ? "STREAM_INPUT." : "STREAM_OUTPUT.") + std::to_string(streamNode.descriptorIndex) + ": StreamIndex " + std::to_string(redundantIndex) + " uses the same AVB_INTERFACE than another stream of the association");
					break;
				}
				redundantStreamNodes.emplace(std::make_pair(redundantInterfaceIndex, &redundantStream));
			}

#	ifdef ENABLE_AVDECC_STRICT_2018_REDUNDANCY
			// Check AVB_INTERFACE index used are 0 for primary and 1 for secondary
			if (redundantStreamNodes.find(0u) == redundantStreamNodes.end() || redundantStreamNodes.find(1) == redundantStreamNodes.end())
			{
				isAssociationValid = false;
				LOG_CONTROLLER_ERROR(entityID, std::string("RedundantStreamAssociation invalid for ") + (streamNode.descriptorType == entity::model::DescriptorType::StreamInput ? "STREAM_INPUT." : "STREAM_OUTPUT.") + std::to_string(streamNode.descriptorIndex) + ": Redundant streams do not use AVB_INTERFACE 0 and 1");
			}
#	endif // ENABLE_AVDECC_STRICT_2018_REDUNDANCY

			if (!isAssociationValid)
			{
				continue;
			}

			// Association is valid, check if the RedundantStreamNode has been created for this stream yet // Also do a sanity check on a single stream being part of multiple associations
			auto redundantNodeCreated = false;
			for (auto const& redundantStreamNodeKV : redundantStreams)
			{
				auto const& redundantStreamNode = redundantStreamNodeKV.second;
				if (redundantStreamNode.redundantStreams.find(streamIndex) != redundantStreamNode.redundantStreams.end())
				{
					// Stream found in an association, but check if it's the first time it's found
					if (redundantNodeCreated)
					{
						isAssociationValid = false;
						LOG_CONTROLLER_ERROR(entityID, std::string("RedundantStreamAssociation invalid for ") + (streamNode.descriptorType == entity::model::DescriptorType::StreamInput ? "STREAM_INPUT." : "STREAM_OUTPUT.") + std::to_string(streamNode.descriptorIndex) + ": Stream has been found in multiple RedundantAssociation sets");
						break;
					}
					redundantNodeCreated = true;
				}
			}

			if (isAssociationValid)
			{
				if (!redundantNodeCreated)
				{
					// Create it now
					auto const virtualIndex = static_cast<model::VirtualIndex>(redundantStreams.size());
					//auto& redundantStreamNode = redundantStreams[virtualIndex];
					auto& redundantStreamNode = redundantStreams.emplace(virtualIndex, RedundantNodeType{ virtualIndex }).first->second;

					// Add all streams part of this redundant association
					for (auto& redundantNodeKV : redundantStreamNodes)
					{
						auto* const redundantNode = redundantNodeKV.second;
						redundantStreamNode.redundantStreams.emplace(redundantNode->descriptorIndex);
						redundantNode->isRedundant = true; // Set this StreamNode as part of a valid redundant stream association
					}

					auto redundantStreamIt = redundantStreamNodes.begin();
					// Defined the primary stream
					redundantStreamNode.primaryStreamIndex = redundantStreamIt->second->descriptorIndex;

					// Try to create a virtual name
					if (redundantStreamNodes.size() == 2)
					{
						auto const primaryName = getStreamName<StreamNodeType>(entity, redundantStreamNodes[0]);
						auto const secondaryName = getStreamName<StreamNodeType>(entity, redundantStreamNodes[1]);

						auto const virtualName = getCommonString(primaryName.str(), secondaryName.str());
						if (!virtualName.empty())
						{
							redundantStreamNode.virtualName = virtualName;
						}
					}

					// Cache Primary and Secondary StreamIndexes
					redundantPrimaryStreams.insert(redundantStreamIt->second->descriptorIndex);
					++redundantStreamIt;
					redundantSecondaryStreams.insert(redundantStreamIt->second->descriptorIndex);
				}
			}
		}
	}

private:
	template<typename StreamNodeType>
	static avdecc::entity::model::AvdeccFixedString getStreamName(ControlledEntityImpl const& entity, model::StreamNode const* const stream) noexcept
	{
		auto const* const streamNode = static_cast<StreamNodeType const* const>(stream);
		if (!streamNode->dynamicModel.objectName.empty())
		{
			return streamNode->dynamicModel.objectName;
		}
		return entity.getLocalizedString(streamNode->staticModel.localizedDescription);
	}
};

void ControlledEntityImpl::buildVirtualNodes(model::ConfigurationNode& configNode) noexcept
{
#	ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	// Build RedundantStreamNodes
	buildRedundancyNodes(configNode);
#	endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
}

void ControlledEntityImpl::buildRedundancyNodes(model::ConfigurationNode& configNode) noexcept
{
	RedundantHelper::buildRedundancyNodesByType(*this, _entity.getEntityID(), configNode.streamInputs, configNode.redundantStreamInputs, _redundantPrimaryStreamInputs, _redundantSecondaryStreamInputs);
	RedundantHelper::buildRedundancyNodesByType(*this, _entity.getEntityID(), configNode.streamOutputs, configNode.redundantStreamOutputs, _redundantPrimaryStreamOutputs, _redundantSecondaryStreamOutputs);
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

namespace buildEntityModelHelper
{
template<class NodeType, typename = std::enable_if_t<std::is_same_v<NodeType, model::StreamPortInputNode> || std::is_same_v<NodeType, model::StreamPortOutputNode>>>
void processStreamPortNodes(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configIndex, entity::model::AudioUnitTree::StreamPortTrees const& streamPortTrees)
{
	for (auto const& [streamPortIndex, streamPortTree] : streamPortTrees)
	{
		auto* streamPortNode = static_cast<NodeType*>(nullptr);
		if constexpr (std::is_same_v<NodeType, model::StreamPortInputNode>)
		{
			streamPortNode = entity->getModelAccessStrategy().getStreamPortInputNode(configIndex, streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
		}
		else if constexpr (std::is_same_v<NodeType, model::StreamPortOutputNode>)
		{
			streamPortNode = entity->getModelAccessStrategy().getStreamPortOutputNode(configIndex, streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
		}
		streamPortNode->staticModel = streamPortTree.staticModel;
		streamPortNode->dynamicModel = streamPortTree.dynamicModel;

		// Build audio clusters (AudioClusterNode)
		for (auto const& [clusterIndex, clusterTree] : streamPortTree.audioClusterModels)
		{
			auto* const audioClusterNode = entity->getModelAccessStrategy().getAudioClusterNode(configIndex, clusterIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
			audioClusterNode->staticModel = clusterTree.staticModel;
			audioClusterNode->dynamicModel = clusterTree.dynamicModel;
		}

		// Build audio maps (AudioMapNode)
		for (auto const& [mapIndex, mapTree] : streamPortTree.audioMapModels)
		{
			auto* const audioMapNode = entity->getModelAccessStrategy().getAudioMapNode(configIndex, mapIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
			audioMapNode->staticModel = mapTree.staticModel;
		}

		// Build controls (ControlNode)
		for (auto const& [controlIndex, controlTree] : streamPortTree.controlModels)
		{
			auto* controlNode = static_cast<model::ControlNode*>(nullptr);
			if constexpr (std::is_same_v<NodeType, model::StreamPortInputNode>)
			{
				controlNode = entity->getModelAccessStrategy().getControlNode(configIndex, controlIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct, TreeModelAccessStrategy::DefaultConstructLevelHint::StreamPortInput);
			}
			else if constexpr (std::is_same_v<NodeType, model::StreamPortOutputNode>)
			{
				controlNode = entity->getModelAccessStrategy().getControlNode(configIndex, controlIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct, TreeModelAccessStrategy::DefaultConstructLevelHint::StreamPortOutput);
			}
			controlNode->staticModel = controlTree.staticModel;
			controlNode->dynamicModel = controlTree.dynamicModel;
		}
	}
}

template<class NodeType, class TreeType, typename = std::enable_if_t<std::is_same_v<NodeType, model::StreamInputNode> || std::is_same_v<NodeType, model::StreamOutputNode>>>
void processStreamNodes(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configIndex, std::map<entity::model::StreamIndex, TreeType> const& streams)
{
	for (auto& [streamIndex, streamModel] : streams)
	{
		auto* streamNode = static_cast<NodeType*>(nullptr);
		if constexpr (std::is_same_v<NodeType, model::StreamInputNode>)
		{
			streamNode = entity->getModelAccessStrategy().getStreamInputNode(configIndex, streamIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
		}
		else if constexpr (std::is_same_v<NodeType, model::StreamOutputNode>)
		{
			streamNode = entity->getModelAccessStrategy().getStreamOutputNode(configIndex, streamIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
		}
		streamNode->staticModel = streamModel.staticModel;
		streamNode->dynamicModel = streamModel.dynamicModel;
	}
}

template<class NodeType, typename = std::enable_if_t<std::is_same_v<NodeType, model::JackInputNode> || std::is_same_v<NodeType, model::JackOutputNode>>>
void processJackNodes(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configIndex, entity::model::ConfigurationTree::JackTrees const& jackTrees)
{
	for (auto const& [jackIndex, jackTree] : jackTrees)
	{
		auto* jackNode = static_cast<NodeType*>(nullptr);
		if constexpr (std::is_same_v<NodeType, model::JackInputNode>)
		{
			jackNode = entity->getModelAccessStrategy().getJackInputNode(configIndex, jackIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
		}
		else if constexpr (std::is_same_v<NodeType, model::JackOutputNode>)
		{
			jackNode = entity->getModelAccessStrategy().getJackOutputNode(configIndex, jackIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
		}
		jackNode->staticModel = jackTree.staticModel;
		jackNode->dynamicModel = jackTree.dynamicModel;

		// Build controls (ControlNode)
		for (auto const& [controlIndex, controlTree] : jackTree.controlModels)
		{
			auto* controlNode = static_cast<model::ControlNode*>(nullptr);
			if constexpr (std::is_same_v<NodeType, model::JackInputNode>)
			{
				controlNode = entity->getModelAccessStrategy().getControlNode(configIndex, controlIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct, TreeModelAccessStrategy::DefaultConstructLevelHint::JackInput);
			}
			else if constexpr (std::is_same_v<NodeType, model::JackOutputNode>)
			{
				controlNode = entity->getModelAccessStrategy().getControlNode(configIndex, controlIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct, TreeModelAccessStrategy::DefaultConstructLevelHint::JackOutput);
			}
			controlNode->staticModel = controlTree.staticModel;
			controlNode->dynamicModel = controlTree.dynamicModel;
		}
	}
}

template<typename NodeType>
void processJackNodes(ControlledEntity const* const entity, model::EntityModelVisitor* const visitor, model::ConfigurationNode const& configuration, std::map<entity::model::JackIndex, NodeType> const& jacks)
{
	for (auto const& jackKV : jacks)
	{
		auto const& jack = jackKV.second;
		// Visit JackNode (ConfigurationNode is parent)
		visitor->visit(entity, &configuration, jack);

		// Loop over ControlNode
		for (auto const& controlKV : jack.controls)
		{
			auto const& control = controlKV.second;
			// Visit ControlNode (JackNode is parent)
			visitor->visit(entity, &configuration, &jack, control);
		}
	}
}

} // namespace buildEntityModelHelper


void ControlledEntityImpl::buildEntityModelGraph(entity::model::EntityTree const& entityTree) noexcept
{
	try
	{
		// Build root node (EntityNode)
		auto* const entityNode = _treeModelAccess->getEntityNode(TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
		entityNode->staticModel = entityTree.staticModel;
		entityNode->dynamicModel = entityTree.dynamicModel;

		// Build configuration nodes (ConfigurationNode)
		for (auto& [configIndex, configTree] : entityTree.configurationTrees)
		{
			auto* const configNode = _treeModelAccess->getConfigurationNode(configIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
			configNode->staticModel = configTree.staticModel;
			configNode->dynamicModel = configTree.dynamicModel;

			// Build leaves first
			{
				// Build stream inputs and outputs (StreamInputNode / StreamOutputNode)
				buildEntityModelHelper::processStreamNodes<model::StreamInputNode>(this, configIndex, configTree.streamInputModels);
				buildEntityModelHelper::processStreamNodes<model::StreamOutputNode>(this, configIndex, configTree.streamOutputModels);

				// Build clock sources (ClockSourceNode)
				for (auto& [sourceIndex, sourceModel] : configTree.clockSourceModels)
				{
					auto* const sourceNode = _treeModelAccess->getClockSourceNode(configIndex, sourceIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
					sourceNode->staticModel = sourceModel.staticModel;
					sourceNode->dynamicModel = sourceModel.dynamicModel;
				}

				// Build memory objects (MemoryObjectNode)
				for (auto& [memoryObjectIndex, memoryObjectModel] : configTree.memoryObjectModels)
				{
					auto* const memoryObjectNode = _treeModelAccess->getMemoryObjectNode(configIndex, memoryObjectIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
					memoryObjectNode->staticModel = memoryObjectModel.staticModel;
					memoryObjectNode->dynamicModel = memoryObjectModel.dynamicModel;
				}

				// Build locales (LocaleNode)
				for (auto const& [localeIndex, localeTree] : configTree.localeTrees)
				{
					auto* const localeNode = _treeModelAccess->getLocaleNode(configIndex, localeIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
					localeNode->staticModel = localeTree.staticModel;

					// Build strings (StringsNode)
					for (auto const& [stringsIndex, stringsModel] : localeTree.stringsModels)
					{
						auto* const stringsNode = _treeModelAccess->getStringsNode(configIndex, stringsIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
						stringsNode->staticModel = stringsModel.staticModel;
					}
				}

				// Build controls (ControlNode)
				for (auto& [controlIndex, controlModel] : configTree.controlModels)
				{
					auto* const controlNode = _treeModelAccess->getControlNode(configIndex, controlIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct, TreeModelAccessStrategy::DefaultConstructLevelHint::Configuration);
					controlNode->staticModel = controlModel.staticModel;
					controlNode->dynamicModel = controlModel.dynamicModel;
				}

				// Build clock domains (ClockDomainNode)
				for (auto& [domainIndex, domainModel] : configTree.clockDomainModels)
				{
					auto* const domainNode = _treeModelAccess->getClockDomainNode(configIndex, domainIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
					domainNode->staticModel = domainModel.staticModel;
					domainNode->dynamicModel = domainModel.dynamicModel;
				}

				// Build timings (TimingNode)
				for (auto& [timingIndex, timingModel] : configTree.timingModels)
				{
					auto* const timingNode = _treeModelAccess->getTimingNode(configIndex, timingIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
					timingNode->staticModel = timingModel.staticModel;
					timingNode->dynamicModel = timingModel.dynamicModel;
				}
			}

			// Now build the trees
			{
				// Build audio units (AudioUnitNode)
				for (auto& [audioUnitIndex, audioUnitTree] : configTree.audioUnitTrees)
				{
					auto* const audioUnitNode = _treeModelAccess->getAudioUnitNode(configIndex, audioUnitIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
					audioUnitNode->staticModel = audioUnitTree.staticModel;
					audioUnitNode->dynamicModel = audioUnitTree.dynamicModel;

					// Build leaves first
					{
						// Build controls (ControlNode)
						for (auto const& [controlIndex, controlTree] : audioUnitTree.controlModels)
						{
							auto* const controlNode = _treeModelAccess->getControlNode(configIndex, controlIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct, TreeModelAccessStrategy::DefaultConstructLevelHint::AudioUnit);
							controlNode->staticModel = controlTree.staticModel;
							controlNode->dynamicModel = controlTree.dynamicModel;
						}
					}

					// Now build the trees
					{
						// Build stream port inputs and outputs (StreamPortInputNode / StreamPortOutputNode)
						buildEntityModelHelper::processStreamPortNodes<model::StreamPortInputNode>(this, configIndex, audioUnitTree.streamPortInputTrees);
						buildEntityModelHelper::processStreamPortNodes<model::StreamPortOutputNode>(this, configIndex, audioUnitTree.streamPortOutputTrees);

						//	Process ExternalPortInputTrees
						//	Process ExternalPortOutputTrees
						//	Process InternalPortInputTrees
						//	Process InternalPortOutputTrees
					}
				}

				// Build jack inputs and outputs (JackInputNode / JackOutputNode)
				buildEntityModelHelper::processJackNodes<model::JackInputNode>(this, configIndex, configTree.jackInputTrees);
				buildEntityModelHelper::processJackNodes<model::JackOutputNode>(this, configIndex, configTree.jackOutputTrees);

				// Build avb interfaces (AvbInterfaceNode)
				for (auto& [interfaceIndex, interfaceModel] : configTree.avbInterfaceModels)
				{
					auto* const interfaceNode = _treeModelAccess->getAvbInterfaceNode(configIndex, interfaceIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
					interfaceNode->staticModel = interfaceModel.staticModel;
					interfaceNode->dynamicModel = interfaceModel.dynamicModel;
#pragma message("TODO: Add Controls (AvbInterface children) - 1722.1-2021")
				}

				// Build ptp instances (PtpInstanceNode)
				for (auto& [ptpInstanceIndex, ptpInstanceTree] : configTree.ptpInstanceTrees)
				{
					auto* const ptpInstanceNode = _treeModelAccess->getPtpInstanceNode(configIndex, ptpInstanceIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
					ptpInstanceNode->staticModel = ptpInstanceTree.staticModel;
					ptpInstanceNode->dynamicModel = ptpInstanceTree.dynamicModel;

					// Build controls (ControlNode)
					for (auto const& [controlIndex, controlTree] : ptpInstanceTree.controlModels)
					{
						auto* const controlNode = _treeModelAccess->getControlNode(configIndex, controlIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct, TreeModelAccessStrategy::DefaultConstructLevelHint::PtpInstance);
						controlNode->staticModel = controlTree.staticModel;
						controlNode->dynamicModel = controlTree.dynamicModel;
					}
					// Build ptp ports (PtpPortNode)
					for (auto const& [ptpPortIndex, ptpPortTree] : ptpInstanceTree.ptpPortModels)
					{
						auto* const ptpPortNode = _treeModelAccess->getPtpPortNode(configIndex, ptpPortIndex, TreeModelAccessStrategy::NotFoundBehavior::DefaultConstruct);
						ptpPortNode->staticModel = ptpPortTree.staticModel;
						ptpPortNode->dynamicModel = ptpPortTree.dynamicModel;
					}
				}
			}
		}

		// Success, switch to Cached Model Strategy
		switchToCachedTreeModelAccessStrategy();
	}
	catch (...)
	{
		// Reset the graph
		_entityNode = {};
		AVDECC_ASSERT(false, "Should never throw");
	}
}

entity::model::EntityTree const& ControlledEntityImpl::getEntityModelTree() const noexcept
{
	class FullModelVisitor : public la::avdecc::controller::model::EntityModelVisitor
	{
	public:
		FullModelVisitor(ControlledEntityImpl const& entity) noexcept
			: _entity{ entity }
		{
		}

		// Deleted compiler auto-generated methods
		FullModelVisitor(FullModelVisitor const&) = delete;
		FullModelVisitor(FullModelVisitor&&) = delete;
		FullModelVisitor& operator=(FullModelVisitor const&) = delete;
		FullModelVisitor& operator=(FullModelVisitor&&) = delete;

	private:
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::EntityNode const& node) noexcept override
		{
			// Create tree
			auto entityTree = entity::model::EntityTree{};

			// Copy static and dynamic models
			entityTree.staticModel = node.staticModel;
			entityTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree = std::move(entityTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::EntityNode const* const /*parent*/, la::avdecc::controller::model::ConfigurationNode const& node) noexcept override
		{
			// Create tree
			auto configTree = entity::model::ConfigurationTree{};

			// Copy static and dynamic models
			configTree.staticModel = node.staticModel;
			configTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[node.descriptorIndex] = std::move(configTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::AudioUnitNode const& node) noexcept override
		{
			// Create tree
			auto audioUnitTree = entity::model::AudioUnitTree{};

			// Copy static and dynamic models
			audioUnitTree.staticModel = node.staticModel;
			audioUnitTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[parent->descriptorIndex].audioUnitTrees[node.descriptorIndex] = std::move(audioUnitTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::StreamInputNode const& node) noexcept override
		{
			// Create tree
			auto streamInputTree = entity::model::StreamInputNodeModels{};

			// Copy static and dynamic models
			streamInputTree.staticModel = node.staticModel;
			streamInputTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[parent->descriptorIndex].streamInputModels[node.descriptorIndex] = std::move(streamInputTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::StreamOutputNode const& node) noexcept override
		{
			// Create tree
			auto streamOutputTree = entity::model::StreamOutputNodeModels{};

			// Copy static and dynamic models
			streamOutputTree.staticModel = node.staticModel;
			streamOutputTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[parent->descriptorIndex].streamOutputModels[node.descriptorIndex] = std::move(streamOutputTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::JackInputNode const& node) noexcept override
		{
			// Create tree
			auto jackTree = entity::model::JackTree{};

			// Copy static and dynamic models
			jackTree.staticModel = node.staticModel;
			jackTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[parent->descriptorIndex].jackInputTrees[node.descriptorIndex] = std::move(jackTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::JackOutputNode const& node) noexcept override
		{
			// Create tree
			auto jackTree = entity::model::JackTree{};

			// Copy static and dynamic models
			jackTree.staticModel = node.staticModel;
			jackTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[parent->descriptorIndex].jackOutputTrees[node.descriptorIndex] = std::move(jackTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::JackNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			// Create tree
			auto controlTree = entity::model::ControlNodeModels{};

			// Copy static and dynamic models
			controlTree.staticModel = node.staticModel;
			controlTree.dynamicModel = node.dynamicModel;

			// Save
			if (parent->descriptorType == entity::model::DescriptorType::JackInput)
			{
				_entity._entityTree->configurationTrees[grandParent->descriptorIndex].jackInputTrees[parent->descriptorIndex].controlModels[node.descriptorIndex] = std::move(controlTree);
			}
			else if (parent->descriptorType == entity::model::DescriptorType::JackOutput)
			{
				_entity._entityTree->configurationTrees[grandParent->descriptorIndex].jackOutputTrees[parent->descriptorIndex].controlModels[node.descriptorIndex] = std::move(controlTree);
			}
			else
			{
				AVDECC_ASSERT(false, "Unsupported DescriptorType");
			}
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::AvbInterfaceNode const& node) noexcept override
		{
			// Create tree
			auto avbInterfaceTree = entity::model::AvbInterfaceNodeModels{};

			// Copy static and dynamic models
			avbInterfaceTree.staticModel = node.staticModel;
			avbInterfaceTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[parent->descriptorIndex].avbInterfaceModels[node.descriptorIndex] = std::move(avbInterfaceTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::ClockSourceNode const& node) noexcept override
		{
			// Create tree
			auto clockSourceTree = entity::model::ClockSourceNodeModels{};

			// Copy static and dynamic models
			clockSourceTree.staticModel = node.staticModel;
			clockSourceTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[parent->descriptorIndex].clockSourceModels[node.descriptorIndex] = std::move(clockSourceTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::MemoryObjectNode const& node) noexcept override
		{
			// Create tree
			auto memoryObjectTree = entity::model::MemoryObjectNodeModels{};

			// Copy static and dynamic models
			memoryObjectTree.staticModel = node.staticModel;
			memoryObjectTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[parent->descriptorIndex].memoryObjectModels[node.descriptorIndex] = std::move(memoryObjectTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::LocaleNode const& node) noexcept override
		{
			// Create tree
			auto localeTree = entity::model::LocaleTree{};

			// Copy static and dynamic models
			localeTree.staticModel = node.staticModel;

			// Save
			_entity._entityTree->configurationTrees[parent->descriptorIndex].localeTrees[node.descriptorIndex] = std::move(localeTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::LocaleNode const* const parent, la::avdecc::controller::model::StringsNode const& node) noexcept override
		{
			// Create tree
			auto stringsTree = entity::model::StringsNodeModels{};

			// Copy static and dynamic models
			stringsTree.staticModel = node.staticModel;

			// Save
			_entity._entityTree->configurationTrees[grandParent->descriptorIndex].localeTrees[parent->descriptorIndex].stringsModels[node.descriptorIndex] = std::move(stringsTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::AudioUnitNode const* const parent, la::avdecc::controller::model::StreamPortInputNode const& node) noexcept override
		{
			// Create tree
			auto streamPortTree = entity::model::StreamPortTree{};

			// Copy static and dynamic models
			streamPortTree.staticModel = node.staticModel;
			streamPortTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[grandParent->descriptorIndex].audioUnitTrees[parent->descriptorIndex].streamPortInputTrees[node.descriptorIndex] = std::move(streamPortTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::AudioUnitNode const* const parent, la::avdecc::controller::model::StreamPortOutputNode const& node) noexcept override
		{
			// Create tree
			auto streamPortTree = entity::model::StreamPortTree{};

			// Copy static and dynamic models
			streamPortTree.staticModel = node.staticModel;
			streamPortTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[grandParent->descriptorIndex].audioUnitTrees[parent->descriptorIndex].streamPortOutputTrees[node.descriptorIndex] = std::move(streamPortTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::AudioUnitNode const* const grandParent, la::avdecc::controller::model::StreamPortNode const* const parent, la::avdecc::controller::model::AudioClusterNode const& node) noexcept override
		{
			// Create tree
			auto audioClusterTree = entity::model::AudioClusterNodeModels{};

			// Copy static and dynamic models
			audioClusterTree.staticModel = node.staticModel;
			audioClusterTree.dynamicModel = node.dynamicModel;

			// Save
			if (parent->descriptorType == entity::model::DescriptorType::StreamPortInput)
			{
				_entity._entityTree->configurationTrees[grandGrandParent->descriptorIndex].audioUnitTrees[grandParent->descriptorIndex].streamPortInputTrees[parent->descriptorIndex].audioClusterModels[node.descriptorIndex] = std::move(audioClusterTree);
			}
			else if (parent->descriptorType == entity::model::DescriptorType::StreamPortOutput)
			{
				_entity._entityTree->configurationTrees[grandGrandParent->descriptorIndex].audioUnitTrees[grandParent->descriptorIndex].streamPortOutputTrees[parent->descriptorIndex].audioClusterModels[node.descriptorIndex] = std::move(audioClusterTree);
			}
			else
			{
				AVDECC_ASSERT(false, "Unsupported DescriptorType");
			}
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::AudioUnitNode const* const grandParent, la::avdecc::controller::model::StreamPortNode const* const parent, la::avdecc::controller::model::AudioMapNode const& node) noexcept override
		{
			// Create tree
			auto audioMapTree = entity::model::AudioMapNodeModels{};

			// Copy static and dynamic models
			audioMapTree.staticModel = node.staticModel;

			// Save
			if (parent->descriptorType == entity::model::DescriptorType::StreamPortInput)
			{
				_entity._entityTree->configurationTrees[grandGrandParent->descriptorIndex].audioUnitTrees[grandParent->descriptorIndex].streamPortInputTrees[parent->descriptorIndex].audioMapModels[node.descriptorIndex] = std::move(audioMapTree);
			}
			else if (parent->descriptorType == entity::model::DescriptorType::StreamPortOutput)
			{
				_entity._entityTree->configurationTrees[grandGrandParent->descriptorIndex].audioUnitTrees[grandParent->descriptorIndex].streamPortOutputTrees[parent->descriptorIndex].audioMapModels[node.descriptorIndex] = std::move(audioMapTree);
			}
			else
			{
				AVDECC_ASSERT(false, "Unsupported DescriptorType");
			}
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::AudioUnitNode const* const grandParent, la::avdecc::controller::model::StreamPortNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			// Create tree
			auto controlTree = entity::model::ControlNodeModels{};

			// Copy static and dynamic models
			controlTree.staticModel = node.staticModel;
			controlTree.dynamicModel = node.dynamicModel;

			// Save
			if (parent->descriptorType == entity::model::DescriptorType::StreamPortInput)
			{
				_entity._entityTree->configurationTrees[grandGrandParent->descriptorIndex].audioUnitTrees[grandParent->descriptorIndex].streamPortInputTrees[parent->descriptorIndex].controlModels[node.descriptorIndex] = std::move(controlTree);
			}
			else if (parent->descriptorType == entity::model::DescriptorType::StreamPortOutput)
			{
				_entity._entityTree->configurationTrees[grandGrandParent->descriptorIndex].audioUnitTrees[grandParent->descriptorIndex].streamPortOutputTrees[parent->descriptorIndex].controlModels[node.descriptorIndex] = std::move(controlTree);
			}
			else
			{
				AVDECC_ASSERT(false, "Unsupported DescriptorType");
			}
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::AudioUnitNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			// Create tree
			auto controlTree = entity::model::ControlNodeModels{};

			// Copy static and dynamic models
			controlTree.staticModel = node.staticModel;
			controlTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[grandParent->descriptorIndex].audioUnitTrees[parent->descriptorIndex].controlModels[node.descriptorIndex] = std::move(controlTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			// Create tree
			auto controlTree = entity::model::ControlNodeModels{};

			// Copy static and dynamic models
			controlTree.staticModel = node.staticModel;
			controlTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[parent->descriptorIndex].controlModels[node.descriptorIndex] = std::move(controlTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::ClockDomainNode const& node) noexcept override
		{
			// Create tree
			auto clockDomainTree = entity::model::ClockDomainNodeModels{};

			// Copy static and dynamic models
			clockDomainTree.staticModel = node.staticModel;
			clockDomainTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[parent->descriptorIndex].clockDomainModels[node.descriptorIndex] = std::move(clockDomainTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::ClockDomainNode const* const /*parent*/, la::avdecc::controller::model::ClockSourceNode const& /*node*/) noexcept override
		{
			// Ignore virtual parenting
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::TimingNode const& node) noexcept override
		{
			// Create tree
			auto timingTree = entity::model::TimingNodeModels{};

			// Copy static and dynamic models
			timingTree.staticModel = node.staticModel;
			timingTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[parent->descriptorIndex].timingModels[node.descriptorIndex] = std::move(timingTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::PtpInstanceNode const& node) noexcept override
		{
			// Create tree
			auto ptpInstanceTree = entity::model::PtpInstanceTree{};

			// Copy static and dynamic models
			ptpInstanceTree.staticModel = node.staticModel;
			ptpInstanceTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[parent->descriptorIndex].ptpInstanceTrees[node.descriptorIndex] = std::move(ptpInstanceTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::TimingNode const* const /*parent*/, la::avdecc::controller::model::PtpInstanceNode const& /*node*/) noexcept override
		{
			// Ignore virtual parenting
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::PtpInstanceNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept
		{
			// Create tree
			auto controlTree = entity::model::ControlNodeModels{};

			// Copy static and dynamic models
			controlTree.staticModel = node.staticModel;
			controlTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[grandParent->descriptorIndex].ptpInstanceTrees[parent->descriptorIndex].controlModels[node.descriptorIndex] = std::move(controlTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::PtpInstanceNode const* const parent, la::avdecc::controller::model::PtpPortNode const& node) noexcept
		{
			// Create tree
			auto ptpPortTree = entity::model::PtpPortNodeModels{};

			// Copy static and dynamic models
			ptpPortTree.staticModel = node.staticModel;
			ptpPortTree.dynamicModel = node.dynamicModel;

			// Save
			_entity._entityTree->configurationTrees[grandParent->descriptorIndex].ptpInstanceTrees[parent->descriptorIndex].ptpPortModels[node.descriptorIndex] = std::move(ptpPortTree);
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::TimingNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept override
		{
			// Ignore virtual parenting
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::TimingNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::PtpPortNode const& /*node*/) noexcept override
		{
			// Ignore virtual parenting
		}
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::RedundantStreamInputNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::RedundantStreamOutputNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::RedundantStreamNode const* const /*parent*/, la::avdecc::controller::model::StreamInputNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::RedundantStreamNode const* const /*parent*/, la::avdecc::controller::model::StreamOutputNode const& /*node*/) noexcept override {}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

		ControlledEntityImpl const& _entity;
	};

	class DynamicModelVisitor : public la::avdecc::controller::model::EntityModelVisitor
	{
	public:
		DynamicModelVisitor(ControlledEntityImpl const& entity) noexcept
			: _entity{ entity }
		{
		}

		// Deleted compiler auto-generated methods
		DynamicModelVisitor(DynamicModelVisitor const&) = delete;
		DynamicModelVisitor(DynamicModelVisitor&&) = delete;
		DynamicModelVisitor& operator=(DynamicModelVisitor const&) = delete;
		DynamicModelVisitor& operator=(DynamicModelVisitor&&) = delete;

	private:
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::EntityNode const& node) noexcept override
		{
			// Get tree
			auto& entityTree = *_entity._entityTree;

			// Update dynamic model
			entityTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::EntityNode const* const /*parent*/, la::avdecc::controller::model::ConfigurationNode const& node) noexcept override
		{
			// Get tree
			auto& configTree = _entity._entityTree->configurationTrees[node.descriptorIndex];

			// Update dynamic model
			configTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::AudioUnitNode const& node) noexcept override
		{
			// Get tree
			auto& audioUnitTree = _entity._entityTree->configurationTrees[parent->descriptorIndex].audioUnitTrees[node.descriptorIndex];

			// Update dynamic model
			audioUnitTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::StreamInputNode const& node) noexcept override
		{
			// Get tree
			auto& streamInputTree = _entity._entityTree->configurationTrees[parent->descriptorIndex].streamInputModels[node.descriptorIndex];

			// Update dynamic model
			streamInputTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::StreamOutputNode const& node) noexcept override
		{
			// Get tree
			auto& streamOutputTree = _entity._entityTree->configurationTrees[parent->descriptorIndex].streamOutputModels[node.descriptorIndex];

			// Update dynamic model
			streamOutputTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::JackInputNode const& node) noexcept override
		{
			// Get tree
			auto& jackTree = _entity._entityTree->configurationTrees[parent->descriptorIndex].jackInputTrees[node.descriptorIndex];

			// Update dynamic model
			jackTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::JackOutputNode const& node) noexcept override
		{
			// Get tree
			auto& jackTree = _entity._entityTree->configurationTrees[parent->descriptorIndex].jackOutputTrees[node.descriptorIndex];

			// Update dynamic model
			jackTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::JackNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			if (parent->descriptorType == entity::model::DescriptorType::JackInput)
			{
				// Get tree
				auto& controlTree = _entity._entityTree->configurationTrees[grandParent->descriptorIndex].jackInputTrees[parent->descriptorIndex].controlModels[node.descriptorIndex];

				// Update dynamic model
				controlTree.dynamicModel = node.dynamicModel;
			}
			else if (parent->descriptorType == entity::model::DescriptorType::JackOutput)
			{
				// Get tree
				auto& controlTree = _entity._entityTree->configurationTrees[grandParent->descriptorIndex].jackOutputTrees[parent->descriptorIndex].controlModels[node.descriptorIndex];

				// Update dynamic model
				controlTree.dynamicModel = node.dynamicModel;
			}
			else
			{
				AVDECC_ASSERT(false, "Unsupported DescriptorType");
			}
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::AvbInterfaceNode const& node) noexcept override
		{
			// Get tree
			auto& avbInterfaceTree = _entity._entityTree->configurationTrees[parent->descriptorIndex].avbInterfaceModels[node.descriptorIndex];

			// Update dynamic model
			avbInterfaceTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::ClockSourceNode const& node) noexcept override
		{
			// Get tree
			auto& clockSourceTree = _entity._entityTree->configurationTrees[parent->descriptorIndex].clockSourceModels[node.descriptorIndex];

			// Update dynamic model
			clockSourceTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::MemoryObjectNode const& node) noexcept override
		{
			// Get tree
			auto& memoryObjectTree = _entity._entityTree->configurationTrees[parent->descriptorIndex].memoryObjectModels[node.descriptorIndex];

			// Update dynamic model
			memoryObjectTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::LocaleNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::LocaleNode const* const /*parent*/, la::avdecc::controller::model::StringsNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::AudioUnitNode const* const parent, la::avdecc::controller::model::StreamPortInputNode const& node) noexcept override
		{
			// Get tree
			auto& streamPortTree = _entity._entityTree->configurationTrees[grandParent->descriptorIndex].audioUnitTrees[parent->descriptorIndex].streamPortInputTrees[node.descriptorIndex];

			// Update dynamic model
			streamPortTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::AudioUnitNode const* const parent, la::avdecc::controller::model::StreamPortOutputNode const& node) noexcept override
		{
			// Get tree
			auto& streamPortTree = _entity._entityTree->configurationTrees[grandParent->descriptorIndex].audioUnitTrees[parent->descriptorIndex].streamPortOutputTrees[node.descriptorIndex];

			// Update dynamic model
			streamPortTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::AudioUnitNode const* const grandParent, la::avdecc::controller::model::StreamPortNode const* const parent, la::avdecc::controller::model::AudioClusterNode const& node) noexcept override
		{
			if (parent->descriptorType == entity::model::DescriptorType::StreamPortInput)
			{
				// Get tree
				auto& audioClusterTree = _entity._entityTree->configurationTrees[grandGrandParent->descriptorIndex].audioUnitTrees[grandParent->descriptorIndex].streamPortInputTrees[parent->descriptorIndex].audioClusterModels[node.descriptorIndex];

				// Update dynamic model
				audioClusterTree.dynamicModel = node.dynamicModel;
			}
			else if (parent->descriptorType == entity::model::DescriptorType::StreamPortOutput)
			{
				// Get tree
				auto& audioClusterTree = _entity._entityTree->configurationTrees[grandGrandParent->descriptorIndex].audioUnitTrees[grandParent->descriptorIndex].streamPortOutputTrees[parent->descriptorIndex].audioClusterModels[node.descriptorIndex];

				// Update dynamic model
				audioClusterTree.dynamicModel = node.dynamicModel;
			}
			else
			{
				AVDECC_ASSERT(false, "Unsupported DescriptorType");
			}
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*grandParent*/, la::avdecc::controller::model::StreamPortNode const* const /*parent*/, la::avdecc::controller::model::AudioMapNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandGrandParent, la::avdecc::controller::model::AudioUnitNode const* const grandParent, la::avdecc::controller::model::StreamPortNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			if (parent->descriptorType == entity::model::DescriptorType::StreamPortInput)
			{
				// Get tree
				auto& controlTree = _entity._entityTree->configurationTrees[grandGrandParent->descriptorIndex].audioUnitTrees[grandParent->descriptorIndex].streamPortInputTrees[parent->descriptorIndex].controlModels[node.descriptorIndex];

				// Update dynamic model
				controlTree.dynamicModel = node.dynamicModel;
			}
			else if (parent->descriptorType == entity::model::DescriptorType::StreamPortOutput)
			{
				// Get tree
				auto& controlTree = _entity._entityTree->configurationTrees[grandGrandParent->descriptorIndex].audioUnitTrees[grandParent->descriptorIndex].streamPortOutputTrees[parent->descriptorIndex].controlModels[node.descriptorIndex];

				// Update dynamic model
				controlTree.dynamicModel = node.dynamicModel;
			}
			else
			{
				AVDECC_ASSERT(false, "Unsupported DescriptorType");
			}
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::AudioUnitNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			// Get tree
			auto& controlTree = _entity._entityTree->configurationTrees[grandParent->descriptorIndex].audioUnitTrees[parent->descriptorIndex].controlModels[node.descriptorIndex];

			// Update dynamic model
			controlTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			// Get tree
			auto& controlTree = _entity._entityTree->configurationTrees[parent->descriptorIndex].controlModels[node.descriptorIndex];

			// Update dynamic model
			controlTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::ClockDomainNode const& node) noexcept override
		{
			// Get tree
			auto& clockDomainTree = _entity._entityTree->configurationTrees[parent->descriptorIndex].clockDomainModels[node.descriptorIndex];

			// Update dynamic model
			clockDomainTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::ClockDomainNode const* const /*parent*/, la::avdecc::controller::model::ClockSourceNode const& /*node*/) noexcept override
		{
			// Ignore virtual parenting
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::TimingNode const& node) noexcept override
		{
			// Get tree
			auto& timingTree = _entity._entityTree->configurationTrees[parent->descriptorIndex].timingModels[node.descriptorIndex];

			// Update dynamic model
			timingTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const parent, la::avdecc::controller::model::PtpInstanceNode const& node) noexcept override
		{
			// Get tree
			auto& ptpInstanceTree = _entity._entityTree->configurationTrees[parent->descriptorIndex].ptpInstanceTrees[node.descriptorIndex];

			// Update dynamic model
			ptpInstanceTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::TimingNode const* const /*parent*/, la::avdecc::controller::model::PtpInstanceNode const& /*node*/) noexcept override
		{
			// Ignore virtual parenting
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::PtpInstanceNode const* const parent, la::avdecc::controller::model::ControlNode const& node) noexcept override
		{
			// Get tree
			auto& controlTree = _entity._entityTree->configurationTrees[grandParent->descriptorIndex].ptpInstanceTrees[parent->descriptorIndex].controlModels[node.descriptorIndex];

			// Update dynamic model
			controlTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const grandParent, la::avdecc::controller::model::PtpInstanceNode const* const parent, la::avdecc::controller::model::PtpPortNode const& node) noexcept override
		{
			// Get tree
			auto& ptpPortTree = _entity._entityTree->configurationTrees[grandParent->descriptorIndex].ptpInstanceTrees[parent->descriptorIndex].ptpPortModels[node.descriptorIndex];

			// Update dynamic model
			ptpPortTree.dynamicModel = node.dynamicModel;
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::TimingNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept override
		{
			// Ignore virtual parenting
		}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::TimingNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::PtpPortNode const& /*node*/) noexcept override
		{
			// Ignore virtual parenting
		}
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::RedundantStreamInputNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::RedundantStreamOutputNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::RedundantStreamNode const* const /*parent*/, la::avdecc::controller::model::StreamInputNode const& /*node*/) noexcept override {}
		virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::RedundantStreamNode const* const /*parent*/, la::avdecc::controller::model::StreamOutputNode const& /*node*/) noexcept override {}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

		ControlledEntityImpl const& _entity;
	};

	// First time, we need to build both static and dynamic models
	if (!_entityTree)
	{
		// Visit the complete model, create static tree if not already created, and update dynamic info
		auto visitor = FullModelVisitor{ *this };
		accept(&visitor, true);
	}
	else
	{
		// Only refresh the dynamic information
		auto visitor = DynamicModelVisitor{ *this };
		accept(&visitor, true);
	}

	return *_entityTree;
}

void ControlledEntityImpl::switchToCachedTreeModelAccessStrategy() noexcept
{
	_treeModelAccess = std::make_unique<TreeModelAccessCacheStrategy>(this);
}

bool ControlledEntityImpl::isEntityModelComplete(model::EntityNode const& entityNode, std::uint16_t const configurationsCount) const noexcept
{
	if (configurationsCount != entityNode.configurations.size())
	{
		return false;
	}

	// Check if Cached Model is valid for ALL configurations
	for (auto const& [configIndex, configNode] : entityNode.configurations)
	{
		if (!EntityModelCache::isModelValidForConfiguration(configNode))
		{
			return false;
		}
	}
	return true;
}

} // namespace controller
} // namespace avdecc
} // namespace la
