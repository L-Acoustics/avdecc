/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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

#include <algorithm>
#include <cassert>
#include <typeindex>
#include <unordered_map>

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

/* ************************************************************************** */
/* ControlledEntityImpl                                                       */
/* ************************************************************************** */
/** Constructor */
ControlledEntityImpl::ControlledEntityImpl(entity::Entity const& entity, LockInformation::SharedPointer const& sharedLock, bool const isVirtual) noexcept
	: _sharedLock(sharedLock)
	, _isVirtual(isVirtual)
	, _entity(entity)
{
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

bool ControlledEntityImpl::gotFatalEnumerationError() const noexcept
{
	return _gotFatalEnumerateError;
}

bool ControlledEntityImpl::isSubscribedToUnsolicitedNotifications() const noexcept
{
	return _isSubscribedToUnsolicitedNotifications;
}

bool ControlledEntityImpl::isAcquired() const noexcept
{
	return _acquireState == model::AcquireState::Acquired;
}

bool ControlledEntityImpl::isAcquiring() const noexcept
{
	return _acquireState == model::AcquireState::TryAcquire;
}

bool ControlledEntityImpl::isAcquiredByOther() const noexcept
{
	return _acquireState == model::AcquireState::AcquiredByOther;
}

bool ControlledEntityImpl::isLocked() const noexcept
{
	return _lockState == model::LockState::Locked;
}

bool ControlledEntityImpl::isLocking() const noexcept
{
	return _lockState == model::LockState::TryLock;
}

bool ControlledEntityImpl::isLockedByOther() const noexcept
{
	return _lockState == model::LockState::LockedByOther;
}

bool ControlledEntityImpl::isStreamInputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& dynamicModel = getNodeDynamicModel(configurationIndex, streamIndex, &entity::model::ConfigurationTree::streamInputModels);
	return dynamicModel.isStreamRunning ? *dynamicModel.isStreamRunning : true;
}

bool ControlledEntityImpl::isStreamOutputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& dynamicModel = getNodeDynamicModel(configurationIndex, streamIndex, &entity::model::ConfigurationTree::streamOutputModels);
	return dynamicModel.isStreamRunning ? *dynamicModel.isStreamRunning : true;
}

ControlledEntity::InterfaceLinkStatus ControlledEntityImpl::getAvbInterfaceLinkStatus(entity::model::AvbInterfaceIndex const avbInterfaceIndex) const noexcept
{
	// AEM not supported, unknown status
	if (!_entity.getEntityCapabilities().test(entity::EntityCapability::AemSupported))
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

model::EntityNode const& ControlledEntityImpl::getEntityNode() const
{
	if (gotFatalEnumerationError())
		throw Exception(Exception::Type::EnumerationError, "Entity had an enumeration error");

	if (!_entity.getEntityCapabilities().test(entity::EntityCapability::AemSupported))
		throw Exception(Exception::Type::NotSupported, "EM not supported by the entity");

	return _entityNode;
}

model::ConfigurationNode const& ControlledEntityImpl::getConfigurationNode(entity::model::ConfigurationIndex const configurationIndex) const
{
	model::EntityNode const& entityNode = getEntityNode();

	auto const it = entityNode.configurations.find(configurationIndex);
	if (it == entityNode.configurations.end())
		throw Exception(Exception::Type::InvalidConfigurationIndex, "Invalid configuration index");

	return it->second;
}

model::ConfigurationNode const& ControlledEntityImpl::getCurrentConfigurationNode() const
{
	model::EntityNode const& entityNode = getEntityNode();

	if (!entityNode.dynamicModel)
		throw Exception(Exception::Type::Internal, "EntityNodeDynamicModel not set");

	auto const it = entityNode.configurations.find(entityNode.dynamicModel->currentConfiguration);
	if (it == entityNode.configurations.end())
		throw Exception(Exception::Type::Internal, "ConfigurationNode for current_configuration not set");

	return it->second;
}

model::StreamInputNode const& ControlledEntityImpl::getStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& configNode = getConfigurationNode(configurationIndex);

	auto const it = configNode.streamInputs.find(streamIndex);
	if (it == configNode.streamInputs.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream index");

	return it->second;
}

model::StreamOutputNode const& ControlledEntityImpl::getStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& configNode = getConfigurationNode(configurationIndex);

	auto const it = configNode.streamOutputs.find(streamIndex);
	if (it == configNode.streamOutputs.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream index");

	return it->second;
}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
model::RedundantStreamNode const& ControlledEntityImpl::getRedundantStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex) const
{
	auto const& configNode = getConfigurationNode(configurationIndex);

	auto const it = configNode.redundantStreamInputs.find(redundantStreamIndex);
	if (it == configNode.redundantStreamInputs.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid redundant stream index");

	return it->second;
}

model::RedundantStreamNode const& ControlledEntityImpl::getRedundantStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex) const
{
	auto const& configNode = getConfigurationNode(configurationIndex);

	auto const it = configNode.redundantStreamOutputs.find(redundantStreamIndex);
	if (it == configNode.redundantStreamOutputs.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid redundant stream index");

	return it->second;
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

model::AudioUnitNode const& ControlledEntityImpl::getAudioUnitNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex) const
{
	auto const& configNode = getConfigurationNode(configurationIndex);

	auto const it = configNode.audioUnits.find(audioUnitIndex);
	if (it == configNode.audioUnits.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid audio unit index");

	return it->second;
}

model::AvbInterfaceNode const& ControlledEntityImpl::getAvbInterfaceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex) const
{
	auto const& configNode = getConfigurationNode(configurationIndex);

	auto const it = configNode.avbInterfaces.find(avbInterfaceIndex);
	if (it == configNode.avbInterfaces.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid avb interface index");

	return it->second;
}

model::ClockSourceNode const& ControlledEntityImpl::getClockSourceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex) const
{
	auto const& configNode = getConfigurationNode(configurationIndex);

	auto const it = configNode.clockSources.find(clockSourceIndex);
	if (it == configNode.clockSources.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid clock source index");

	return it->second;
}

model::StreamPortNode const& ControlledEntityImpl::getStreamPortInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const& configNode = getConfigurationNode(configurationIndex);

	// Search a matching StreamPortIndex in all AudioUnits
	for (auto const& audioUnitNodeKV : configNode.audioUnits)
	{
		auto const& audioUnitNode = audioUnitNodeKV.second;

		auto const it = audioUnitNode.streamPortInputs.find(streamPortIndex);
		if (it != audioUnitNode.streamPortInputs.end())
			return it->second;
	}

	// Not found
	throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream port input index");
}

model::StreamPortNode const& ControlledEntityImpl::getStreamPortOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const& configNode = getConfigurationNode(configurationIndex);

	// Search a matching StreamPortIndex in all AudioUnits
	for (auto const& audioUnitNodeKV : configNode.audioUnits)
	{
		auto const& audioUnitNode = audioUnitNodeKV.second;

		auto const it = audioUnitNode.streamPortOutputs.find(streamPortIndex);
		if (it != audioUnitNode.streamPortOutputs.end())
			return it->second;
	}

	// Not found
	throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream port output index");
}
#if 0
model::AudioClusterNode const& ControlledEntityImpl::getAudioClusterNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex) const
{
#	if 1
	static model::AudioClusterNode s{};
	(void)configurationIndex;
	(void)audioUnitIndex;
	(void)streamPortIndex;
	(void)clusterIndex;
	return s;
#	else
#	endif
}

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

model::ClockDomainNode const& ControlledEntityImpl::getClockDomainNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex) const
{
	auto const& configNode = getConfigurationNode(configurationIndex);

	auto const it = configNode.clockDomains.find(clockDomainIndex);
	if (it == configNode.clockDomains.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid clock domain index");

	return it->second;
}

entity::model::LocaleNodeStaticModel const* ControlledEntityImpl::findLocaleNode(entity::model::ConfigurationIndex const configurationIndex, std::string const& /*locale*/) const
{
	auto const& configTree = getConfigurationTree(configurationIndex);

	if (configTree.localeModels.empty())
		throw Exception(Exception::Type::InvalidLocaleName, "Entity has no locale");

#pragma message("TODO: Parse 'locale' parameter and find best match")
	// Right now, return the first locale
	return &configTree.localeModels.at(0).staticModel;
}

entity::model::AvdeccFixedString const& ControlledEntityImpl::getLocalizedString(entity::model::LocalizedStringReference const& stringReference) const noexcept
{
	return getLocalizedString(getCurrentConfigurationIndex(), stringReference);
}

entity::model::AvdeccFixedString const& ControlledEntityImpl::getLocalizedString(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocalizedStringReference const& stringReference) const noexcept
{
	static entity::model::AvdeccFixedString s_noLocalizationString{};
	try
	{
		// Not valid, return NO_STRING
		if (!stringReference)
			return s_noLocalizationString;

		auto const globalOffset = stringReference.getGlobalOffset();
		auto const& configDynamicModel = getConfigurationNodeDynamicModel(configurationIndex);

		return configDynamicModel.localizedStrings.at(entity::model::StringsIndex(globalOffset));
	}
	catch (...)
	{
		return s_noLocalizationString;
	}
}

entity::model::StreamConnectionState const& ControlledEntityImpl::getConnectedSinkState(entity::model::StreamIndex const streamIndex) const
{
	auto const& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamInputModels);

	AVDECC_ASSERT(_entity.getEntityID() == dynamicModel.connectionState.listenerStream.entityID, "EntityID not correctly initialized");
	AVDECC_ASSERT(streamIndex == dynamicModel.connectionState.listenerStream.streamIndex, "StreamIndex not correctly initialized");
	return dynamicModel.connectionState;
}

entity::model::AudioMappings const& ControlledEntityImpl::getStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const currentConfiguration = getCurrentConfigurationIndex();

	// Check if dynamic mappings is supported by the entity
	auto const& staticModel = getNodeStaticModel(currentConfiguration, streamPortIndex, &entity::model::ConfigurationTree::streamPortInputModels);
	if (!staticModel.hasDynamicAudioMap)
		throw Exception(Exception::Type::NotSupported, "Dynamic mappings not supported by this stream port");

	// Return dynamic mappings for this stream port
	auto const& dynamicModel = getNodeDynamicModel(currentConfiguration, streamPortIndex, &entity::model::ConfigurationTree::streamPortInputModels);
	return dynamicModel.dynamicAudioMap;
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
	auto const currentConfiguration = getCurrentConfigurationIndex();

	// Check if dynamic mappings is supported by the entity
	auto const& staticModel = getNodeStaticModel(currentConfiguration, streamPortIndex, &entity::model::ConfigurationTree::streamPortOutputModels);
	if (!staticModel.hasDynamicAudioMap)
		throw Exception(Exception::Type::NotSupported, "Dynamic mappings not supported by this stream port");

	// Return dynamic mappings for this stream port
	auto const& dynamicModel = getNodeDynamicModel(currentConfiguration, streamPortIndex, &entity::model::ConfigurationTree::streamPortOutputModels);
	return dynamicModel.dynamicAudioMap;
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

entity::model::StreamConnections const& ControlledEntityImpl::getStreamOutputConnections(entity::model::StreamIndex const streamIndex) const
{
	auto const& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels);

	return dynamicModel.connections;
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

std::chrono::milliseconds const& ControlledEntityImpl::getEnumerationTime() const noexcept
{
	return _enumerationTime;
}

// Visitor method
void ControlledEntityImpl::accept(model::EntityModelVisitor* const visitor) const noexcept
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

			if (!configuration.dynamicModel)
				continue;

			// If this is the active configuration, process ConfigurationNode fields
			if (configuration.dynamicModel->isActiveConfiguration)
			{
				// Loop over AudioUnitNode
				for (auto const& audioUnitKV : configuration.audioUnits)
				{
					auto const& audioUnit = audioUnitKV.second;
					// Visit AudioUnitNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, audioUnit);

					// Loop over StreamPortNode
					auto processStreamPorts = [this, visitor](model::ConfigurationNode const& configuration, model::AudioUnitNode const& audioUnit, std::map<entity::model::StreamPortIndex, model::StreamPortNode> const& streamPorts)
					{
						for (auto const& streamPortKV : streamPorts)
						{
							auto const& streamPort = streamPortKV.second;
							// Visit StreamPortNode (AudioUnitNode is parent)
							visitor->visit(this, &configuration, &audioUnit, streamPort);

							// Loop over AudioClusterNode
							for (auto const& audioClusterKV : streamPort.audioClusters)
							{
								auto const& audioCluster = audioClusterKV.second;
								// Visit AudioClusterNode (StreamPortNode is parent)
								visitor->visit(this, &configuration, &audioUnit, &streamPort, audioCluster);
							}

							// Loop over AudioMapNode
							for (auto const& audioMapKV : streamPort.audioMaps)
							{
								auto const& audioMap = audioMapKV.second;
								// Visit AudioMapNode (StreamPortNode is parent)
								visitor->visit(this, &configuration, &audioUnit, &streamPort, audioMap);
							}
						}
					};
					processStreamPorts(configuration, audioUnit, audioUnit.streamPortInputs); // streamPortInputs
					processStreamPorts(configuration, audioUnit, audioUnit.streamPortOutputs); // streamPortOutputs
				}

				// Loop over StreamInputNode for inputs
				for (auto const& streamKV : configuration.streamInputs)
				{
					auto const& stream = streamKV.second;
					// Visit StreamInputNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, stream);
				}

				// Loop over StreamOutputNode for outputs
				for (auto const& streamKV : configuration.streamOutputs)
				{
					auto const& stream = streamKV.second;
					// Visit StreamOutputNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, stream);
				}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				// Loop over RedundantStreamInputNode for inputs
				for (auto const& redundantStreamKV : configuration.redundantStreamInputs)
				{
					auto const& redundantStream = redundantStreamKV.second;
					// Visit RedundantStreamInputNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, redundantStream);

					// Loop over StreamInputNode
					for (auto const& streamKV : redundantStream.redundantStreams)
					{
						auto const* stream = static_cast<model::StreamInputNode const*>(streamKV.second);
						// Visit StreamInputNode (RedundantStreamInputNode is parent)
						visitor->visit(this, &configuration, &redundantStream, *stream);
					}
				}

				// Loop over RedundantStreamOutputNode for outputs
				for (auto const& redundantStreamKV : configuration.redundantStreamOutputs)
				{
					auto const& redundantStream = redundantStreamKV.second;
					// Visit RedundantStreamOutputNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, redundantStream);

					// Loop over StreamOutputNode
					for (auto const& streamKV : redundantStream.redundantStreams)
					{
						auto const* stream = static_cast<model::StreamOutputNode const*>(streamKV.second);
						// Visit StreamOutputNode (RedundantStreamOutputNode is parent)
						visitor->visit(this, &configuration, &redundantStream, *stream);
					}
				}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

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

				// Loop over ClockDomainNode
				for (auto const& domainKV : configuration.clockDomains)
				{
					auto const& domain = domainKV.second;
					// Visit ClockDomainNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, domain);

					// Loop over ClockSourceNode
					for (auto const& sourceKV : domain.clockSources)
					{
						auto const* source = sourceKV.second;
						// Visit ClockSourceNode (ClockDomainNode is parent)
						visitor->visit(this, &configuration, &domain, *source);
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

// Const Tree getters, all throw Exception::NotSupported if EM not supported by the Entity, Exception::InvalidConfigurationIndex if configurationIndex do not exist
entity::model::EntityTree const& ControlledEntityImpl::getEntityTree() const
{
	if (gotFatalEnumerationError())
		throw Exception(Exception::Type::EnumerationError, "Entity had a fatal enumeration error");

	if (!_entity.getEntityCapabilities().test(entity::EntityCapability::AemSupported))
		throw Exception(Exception::Type::NotSupported, "EM not supported by the entity");

	return _entityTree;
}

entity::model::ConfigurationTree const& ControlledEntityImpl::getConfigurationTree(entity::model::ConfigurationIndex const configurationIndex) const
{
	auto const& entityTree = getEntityTree();
	auto const it = entityTree.configurationTrees.find(configurationIndex);
	if (it == entityTree.configurationTrees.end())
		throw Exception(Exception::Type::InvalidConfigurationIndex, "Invalid configuration index");

	return it->second;
}

entity::model::ConfigurationIndex ControlledEntityImpl::getCurrentConfigurationIndex() const noexcept
{
	return _entityTree.dynamicModel.currentConfiguration;
}

// Const NodeModel getters, all throw Exception::NotSupported if EM not supported by the Entity, Exception::InvalidConfigurationIndex if configurationIndex do not exist, Exception::InvalidDescriptorIndex if descriptorIndex is invalid
entity::model::EntityNodeStaticModel const& ControlledEntityImpl::getEntityNodeStaticModel() const
{
	return getEntityTree().staticModel;
}

entity::model::EntityNodeDynamicModel const& ControlledEntityImpl::getEntityNodeDynamicModel() const
{
	return getEntityTree().dynamicModel;
}

entity::model::ConfigurationNodeStaticModel const& ControlledEntityImpl::getConfigurationNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex) const
{
	return getConfigurationTree(configurationIndex).staticModel;
}

entity::model::ConfigurationNodeDynamicModel const& ControlledEntityImpl::getConfigurationNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex) const
{
	return getConfigurationTree(configurationIndex).dynamicModel;
}

// Non-const Tree getters
entity::model::EntityTree& ControlledEntityImpl::getEntityTree() noexcept
{
	return _entityTree;
}

entity::model::ConfigurationTree& ControlledEntityImpl::getConfigurationTree(entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	auto& entityTree = getEntityTree();
	return entityTree.configurationTrees[configurationIndex];
}

// Non-const NodeModel getters
entity::model::EntityNodeStaticModel& ControlledEntityImpl::getEntityNodeStaticModel() noexcept
{
	return getEntityTree().staticModel;
}

entity::model::EntityNodeDynamicModel& ControlledEntityImpl::getEntityNodeDynamicModel() noexcept
{
	return getEntityTree().dynamicModel;
}

entity::model::ConfigurationNodeStaticModel& ControlledEntityImpl::getConfigurationNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	return getConfigurationTree(configurationIndex).staticModel;
}

entity::model::ConfigurationNodeDynamicModel& ControlledEntityImpl::getConfigurationNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	return getConfigurationTree(configurationIndex).dynamicModel;
}

entity::model::EntityCounters& ControlledEntityImpl::getEntityCounters() noexcept
{
	auto& entityTree = getEntityTree();
	// Create counters if they don't exist yet
	if (!entityTree.dynamicModel.counters)
	{
		entityTree.dynamicModel.counters = entity::model::EntityCounters{};
	}
	return *entityTree.dynamicModel.counters;
}

entity::model::AvbInterfaceCounters& ControlledEntityImpl::getAvbInterfaceCounters(entity::model::AvbInterfaceIndex const avbInterfaceIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), avbInterfaceIndex, &entity::model::ConfigurationTree::avbInterfaceModels);
	// Create counters if they don't exist yet
	if (!dynamicModel.counters)
	{
		dynamicModel.counters = entity::model::AvbInterfaceCounters{};
	}
	return *dynamicModel.counters;
}

entity::model::ClockDomainCounters& ControlledEntityImpl::getClockDomainCounters(entity::model::ClockDomainIndex const clockDomainIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), clockDomainIndex, &entity::model::ConfigurationTree::clockDomainModels);
	// Create counters if they don't exist yet
	if (!dynamicModel.counters)
	{
		dynamicModel.counters = entity::model::ClockDomainCounters{};
	}
	return *dynamicModel.counters;
}

entity::model::StreamInputCounters& ControlledEntityImpl::getStreamInputCounters(entity::model::StreamIndex const streamIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamInputModels);
	// Create counters if they don't exist yet
	if (!dynamicModel.counters)
	{
		dynamicModel.counters = entity::model::StreamInputCounters{};
	}
	return *dynamicModel.counters;
}

entity::model::StreamOutputCounters& ControlledEntityImpl::getStreamOutputCounters(entity::model::StreamIndex const streamIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels);
	// Create counters if they don't exist yet
	if (!dynamicModel.counters)
	{
		dynamicModel.counters = entity::model::StreamOutputCounters{};
	}
	return *dynamicModel.counters;
}

// Setters of the DescriptorDynamic info, default constructing if not existing
void ControlledEntityImpl::setEntityName(entity::model::AvdeccFixedString const& name) noexcept
{
	_entityTree.dynamicModel.entityName = name;
}

void ControlledEntityImpl::setEntityGroupName(entity::model::AvdeccFixedString const& name) noexcept
{
	_entityTree.dynamicModel.groupName = name;
}

void ControlledEntityImpl::setCurrentConfiguration(entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	_entityTree.dynamicModel.currentConfiguration = configurationIndex;

	// Set isActiveConfiguration for each configuration
	for (auto& confIt : _entityTree.configurationTrees)
	{
		confIt.second.dynamicModel.isActiveConfiguration = configurationIndex == confIt.first;
	}
}

void ControlledEntityImpl::setConfigurationName(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name) noexcept
{
	auto& dynamicModel = getConfigurationNodeDynamicModel(configurationIndex);
	dynamicModel.objectName = name;
}

void ControlledEntityImpl::setSamplingRate(entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), audioUnitIndex, &entity::model::ConfigurationTree::audioUnitModels);
	dynamicModel.currentSamplingRate = samplingRate;
}

entity::model::StreamConnectionState ControlledEntityImpl::setStreamInputConnectionState(entity::model::StreamIndex const streamIndex, entity::model::StreamConnectionState const& state) noexcept
{
	AVDECC_ASSERT(_entity.getEntityID() == state.listenerStream.entityID, "EntityID not correctly initialized");
	AVDECC_ASSERT(streamIndex == state.listenerStream.streamIndex, "StreamIndex not correctly initialized");

	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamInputModels);

	// Save previous StreamConnectionState
	auto previousState = dynamicModel.connectionState;

	// Set StreamConnectionState
	dynamicModel.connectionState = state;

	return previousState;
}

void ControlledEntityImpl::clearStreamOutputConnections(entity::model::StreamIndex const streamIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels);
	dynamicModel.connections.clear();
}

bool ControlledEntityImpl::addStreamOutputConnection(entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& listenerStream) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels);
	auto const result = dynamicModel.connections.insert(listenerStream);
	return result.second;
}

bool ControlledEntityImpl::delStreamOutputConnection(entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& listenerStream) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels);
	return dynamicModel.connections.erase(listenerStream) > 0;
}

entity::model::AvbInterfaceInfo ControlledEntityImpl::setAvbInterfaceInfo(entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInterfaceInfo const& info) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), avbInterfaceIndex, &entity::model::ConfigurationTree::avbInterfaceModels);

	// Save previous AvbInfo
	auto previousInfo = dynamicModel.avbInterfaceInfo;

	// Set AvbInterfaceInfo
	dynamicModel.avbInterfaceInfo = info;

	return previousInfo ? *previousInfo : entity::model::AvbInterfaceInfo{};
}

entity::model::AsPath ControlledEntityImpl::setAsPath(entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AsPath const& asPath) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), avbInterfaceIndex, &entity::model::ConfigurationTree::avbInterfaceModels);

	// Save previous AsPath
	auto previousPath = dynamicModel.asPath;

	// Set AsPath
	dynamicModel.asPath = asPath;

	return previousPath ? *previousPath : entity::model::AsPath{};
}

void ControlledEntityImpl::setSelectedLocaleStringsIndexesRange(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const baseIndex, entity::model::StringsIndex const countIndexes) noexcept
{
	auto& dynamicModel = getConfigurationNodeDynamicModel(configurationIndex);
	dynamicModel.selectedLocaleBaseIndex = baseIndex;
	dynamicModel.selectedLocaleCountIndexes = countIndexes;
}

void ControlledEntityImpl::clearStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamPortIndex, &entity::model::ConfigurationTree::streamPortInputModels);
	dynamicModel.dynamicAudioMap.clear();
}

void ControlledEntityImpl::addStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamPortIndex, &entity::model::ConfigurationTree::streamPortInputModels);
	auto& dynamicMap = dynamicModel.dynamicAudioMap;

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

void ControlledEntityImpl::removeStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamPortIndex, &entity::model::ConfigurationTree::streamPortInputModels);
	auto& dynamicMap = dynamicModel.dynamicAudioMap;

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

void ControlledEntityImpl::clearStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamPortIndex, &entity::model::ConfigurationTree::streamPortOutputModels);
	dynamicModel.dynamicAudioMap.clear();
}

void ControlledEntityImpl::addStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamPortIndex, &entity::model::ConfigurationTree::streamPortOutputModels);
	auto& dynamicMap = dynamicModel.dynamicAudioMap;

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

void ControlledEntityImpl::removeStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamPortIndex, &entity::model::ConfigurationTree::streamPortOutputModels);
	auto& dynamicMap = dynamicModel.dynamicAudioMap;

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

void ControlledEntityImpl::setClockSource(entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), clockDomainIndex, &entity::model::ConfigurationTree::clockDomainModels);
	dynamicModel.clockSourceIndex = clockSourceIndex;
}

void ControlledEntityImpl::setMemoryObjectLength(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(configurationIndex, memoryObjectIndex, &entity::model::ConfigurationTree::memoryObjectModels);
	dynamicModel.length = length;
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

void ControlledEntityImpl::setEnumerationTime(std::chrono::milliseconds const& value) noexcept
{
	_enumerationTime = value;
}

// Setters of the Model from AEM Descriptors (including DescriptorDynamic info)
void ControlledEntityImpl::setEntityTree(entity::model::EntityTree const& entityTree) noexcept
{
	_entityTree = entityTree;
}

bool ControlledEntityImpl::setCachedEntityTree(entity::model::EntityTree const& cachedTree, entity::model::EntityDescriptor const& descriptor) noexcept
{
	// Check if static information in EntityDescriptor are identical
	auto const& cachedDescriptor = cachedTree.staticModel;
	if (cachedDescriptor.vendorNameString != descriptor.vendorNameString || cachedDescriptor.modelNameString != descriptor.modelNameString)
	{
		LOG_CONTROLLER_WARN(_entity.getEntityID(), "EntityModelID provided by this Entity has inconsistent data in it's EntityDescriptor, not using cached AEM");
		return false;
	}

	// Ok the static information from EntityDescriptor are identical, we cannot check more than this so we have to assume it's correct, copy the whole model
	_entityTree = cachedTree;

	// And override with the EntityDescriptor so this entity's specific fields are copied
	setEntityDescriptor(descriptor);

	return true;
}

void ControlledEntityImpl::setEntityDescriptor(entity::model::EntityDescriptor const& descriptor) noexcept
{
	if (!AVDECC_ASSERT_WITH_RET(!_advertised, "EntityDescriptor should never be set twice on an entity. Only the dynamic part should be set again."))
	{
		// Wipe everything and set as enumeration error
		_entityTree = {};
		_entityTree = {};
		_entityNode = {};
		_gotFatalEnumerateError = true;

		return;
	}

	// Copy static model
	{
		auto& m = _entityTree.staticModel;
		m.vendorNameString = descriptor.vendorNameString;
		m.modelNameString = descriptor.modelNameString;
	}

	// Copy dynamic model
	{
		auto& m = _entityTree.dynamicModel;
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
	auto const& entityDynamicModel = getEntityNodeDynamicModel();

	// Copy static model
	{
		// Get or create a new model::ConfigurationStaticTree for this entity
		auto& m = getConfigurationNodeStaticModel(configurationIndex);
		m.localizedDescription = descriptor.localizedDescription;
		m.descriptorCounts = descriptor.descriptorCounts;
	}

	// Copy dynamic model
	{
		// Get or create a new model::ConfigurationDynamicTree for this entity
		auto& m = getConfigurationNodeDynamicModel(configurationIndex);
		// Changeable fields through commands
		m.isActiveConfiguration = configurationIndex == entityDynamicModel.currentConfiguration;
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setAudioUnitDescriptor(entity::model::AudioUnitDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::AudioUnitNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, audioUnitIndex, &entity::model::ConfigurationTree::audioUnitModels);
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
		// Get or create a new model::AudioUnitNodeDynamicModel
		auto& m = getNodeDynamicModel(configurationIndex, audioUnitIndex, &entity::model::ConfigurationTree::audioUnitModels);
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
		m.currentSamplingRate = descriptor.currentSamplingRate;
	}
}

void ControlledEntityImpl::setStreamInputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::StreamNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, streamIndex, &entity::model::ConfigurationTree::streamInputModels);
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
		// Get or create a new model::StreamInputNodeDynamicModel
		auto& m = getNodeDynamicModel(configurationIndex, streamIndex, &entity::model::ConfigurationTree::streamInputModels);
		// Not changeable fields
		m.connectionState.listenerStream = entity::model::StreamIdentification{ _entity.getEntityID(), streamIndex }; // We always are the other endpoint of a connection, initialize this now
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
		m.streamFormat = descriptor.currentFormat;
	}
}

void ControlledEntityImpl::setStreamOutputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::StreamNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, streamIndex, &entity::model::ConfigurationTree::streamOutputModels);
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
		// Get or create a new model::StreamOutputNodeDynamicModel
		auto& m = getNodeDynamicModel(configurationIndex, streamIndex, &entity::model::ConfigurationTree::streamOutputModels);
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
		m.streamFormat = descriptor.currentFormat;
	}
}

void ControlledEntityImpl::setAvbInterfaceDescriptor(entity::model::AvbInterfaceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const interfaceIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::AvbInterfaceNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, interfaceIndex, &entity::model::ConfigurationTree::avbInterfaceModels);
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
		// Get or create a new model::AvbInterfaceNodeDynamicModel
		auto& m = getNodeDynamicModel(configurationIndex, interfaceIndex, &entity::model::ConfigurationTree::avbInterfaceModels);
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setClockSourceDescriptor(entity::model::ClockSourceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::ClockSourceNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, clockIndex, &entity::model::ConfigurationTree::clockSourceModels);
		m.localizedDescription = descriptor.localizedDescription;
		m.clockSourceType = descriptor.clockSourceType;
		m.clockSourceLocationType = descriptor.clockSourceLocationType;
		m.clockSourceLocationIndex = descriptor.clockSourceLocationIndex;
	}

	// Copy dynamic model
	{
		// Get or create a new model::ClockSourceNodeDynamicModel
		auto& m = getNodeDynamicModel(configurationIndex, clockIndex, &entity::model::ConfigurationTree::clockSourceModels);
		// Not changeable fields
		m.clockSourceFlags = descriptor.clockSourceFlags;
		m.clockSourceIdentifier = descriptor.clockSourceIdentifier;
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setMemoryObjectDescriptor(entity::model::MemoryObjectDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::MemoryObjectNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, memoryObjectIndex, &entity::model::ConfigurationTree::memoryObjectModels);
		m.localizedDescription = descriptor.localizedDescription;
		m.memoryObjectType = descriptor.memoryObjectType;
		m.targetDescriptorType = descriptor.targetDescriptorType;
		m.targetDescriptorIndex = descriptor.targetDescriptorIndex;
		m.startAddress = descriptor.startAddress;
		m.maximumLength = descriptor.maximumLength;
	}

	// Copy dynamic model
	{
		// Get or create a new model::MemoryObjectNodeDynamicModel
		auto& m = getNodeDynamicModel(configurationIndex, memoryObjectIndex, &entity::model::ConfigurationTree::memoryObjectModels);
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
		m.length = descriptor.length;
	}
}

void ControlledEntityImpl::setLocaleDescriptor(entity::model::LocaleDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::LocaleNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, localeIndex, &entity::model::ConfigurationTree::localeModels);
		m.localeID = descriptor.localeID;
		m.numberOfStringDescriptors = descriptor.numberOfStringDescriptors;
		m.baseStringDescriptorIndex = descriptor.baseStringDescriptorIndex;
	}
}

void ControlledEntityImpl::setStringsDescriptor(entity::model::StringsDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::StringsNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, stringsIndex, &entity::model::ConfigurationTree::stringsModels);
		m.strings = descriptor.strings;
	}

	auto const& configDynamicModel = getConfigurationNodeDynamicModel(configurationIndex);
	// This StringsIndex is part of the active Locale
	if (configDynamicModel.selectedLocaleCountIndexes > 0 && stringsIndex >= configDynamicModel.selectedLocaleBaseIndex && stringsIndex < (configDynamicModel.selectedLocaleBaseIndex + configDynamicModel.selectedLocaleCountIndexes))
	{
		setLocalizedStrings(configurationIndex, stringsIndex - configDynamicModel.selectedLocaleBaseIndex, descriptor.strings);
	}
}

void ControlledEntityImpl::setLocalizedStrings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const relativeStringsIndex, entity::model::AvdeccFixedStrings const& strings) noexcept
{
	auto& configDynamicModel = getConfigurationNodeDynamicModel(configurationIndex);
	// Copy the strings to the ConfigurationDynamicModel for a quick access
	for (auto strIndex = 0u; strIndex < strings.size(); ++strIndex)
	{
		auto const localizedStringIndex = entity::model::LocalizedStringReference{ relativeStringsIndex, static_cast<std::uint8_t>(strIndex) }.getGlobalOffset();
		configDynamicModel.localizedStrings[localizedStringIndex] = strings.at(strIndex);
	}
}

void ControlledEntityImpl::setStreamPortInputDescriptor(entity::model::StreamPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::StreamPortNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, streamPortIndex, &entity::model::ConfigurationTree::streamPortInputModels);
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
	// Copy static model
	{
		// Get or create a new model::StreamPortNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, streamPortIndex, &entity::model::ConfigurationTree::streamPortOutputModels);
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
	// Copy static model
	{
		// Get or create a new model::AudioClusterNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, clusterIndex, &entity::model::ConfigurationTree::audioClusterModels);
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
		// Get or create a new model::AudioClusterNodeDynamicModel
		auto& m = getNodeDynamicModel(configurationIndex, clusterIndex, &entity::model::ConfigurationTree::audioClusterModels);
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setAudioMapDescriptor(entity::model::AudioMapDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::AudioMapNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, mapIndex, &entity::model::ConfigurationTree::audioMapModels);
		m.mappings = descriptor.mappings;
	}
}

void ControlledEntityImpl::setClockDomainDescriptor(entity::model::ClockDomainDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::ClockDomainNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, clockDomainIndex, &entity::model::ConfigurationTree::clockDomainModels);
		m.localizedDescription = descriptor.localizedDescription;
		m.clockSources = descriptor.clockSources;
	}

	// Copy dynamic model
	{
		// Get or create a new model::ClockDomainNodeDynamicModel
		auto& m = getNodeDynamicModel(configurationIndex, clockDomainIndex, &entity::model::ConfigurationTree::clockDomainModels);
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
		m.clockSourceIndex = descriptor.clockSourceIndex;
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
static inline ControlledEntityImpl::DynamicInfoKey makeDynamicInfoKey(ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex descriptorIndex, std::uint16_t const subIndex)
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
static inline ControlledEntityImpl::DescriptorDynamicInfoKey makeDescriptorDynamicInfoKey(ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex descriptorIndex)
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

void ControlledEntityImpl::setGetFatalEnumerationError() noexcept
{
	LOG_CONTROLLER_ERROR(_entity.getEntityID(), "Got Fatal Enumeration Error");
	_gotFatalEnumerateError = true;
}

void ControlledEntityImpl::setSubscribedToUnsolicitedNotifications(bool const isSubscribed) noexcept
{
	_isSubscribedToUnsolicitedNotifications = isSubscribed;
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
		case DescriptorDynamicInfoType::ClockDomainName:
			return static_cast<std::string>(protocol::AemCommandType::GetName) + " (CLOCK_DOMAIN)";
		case DescriptorDynamicInfoType::ClockDomainSourceIndex:
			return protocol::AemCommandType::GetClockSource;
		default:
			return "Unknown DescriptorDynamicInfoType";
	}
}

// Private methods
void ControlledEntityImpl::buildEntityModelGraph() noexcept
{
	try
	{
		// Wipe previous graph
		_entityNode = {};

		// Build a new one
		{
			// Build root node (EntityNode)
			initNode(_entityNode, entity::model::DescriptorType::Entity, 0);
			_entityNode.staticModel = &_entityTree.staticModel;
			_entityNode.dynamicModel = &_entityTree.dynamicModel;

			// Build configuration nodes (ConfigurationNode)
			for (auto& [configIndex, configTree] : _entityTree.configurationTrees)
			{
				auto& configNode = _entityNode.configurations[configIndex];
				initNode(configNode, entity::model::DescriptorType::Configuration, configIndex);
				configNode.staticModel = &configTree.staticModel;
				configNode.dynamicModel = &configTree.dynamicModel;

				// Build audio units (AudioUnitNode)
				for (auto& [audioUnitIndex, audioUnitModels] : configTree.audioUnitModels)
				{
					auto const& audioUnitStaticModel = audioUnitModels.staticModel;
					auto& audioUnitDynamicModel = audioUnitModels.dynamicModel;

					auto& audioUnitNode = configNode.audioUnits[audioUnitIndex];
					initNode(audioUnitNode, entity::model::DescriptorType::AudioUnit, audioUnitIndex);
					audioUnitNode.staticModel = &audioUnitStaticModel;
					audioUnitNode.dynamicModel = &audioUnitDynamicModel;

					// Build stream port inputs and outputs (StreamPortNode)
					auto processStreamPorts = [entity = const_cast<ControlledEntityImpl*>(this), &audioUnitNode, configIndex = configIndex /* Have to explicitly redefine configIndex due to a clang bug*/](entity::model::DescriptorType const descriptorType, std::uint16_t const numberOfStreamPorts, entity::model::StreamPortIndex const baseStreamPort)
					{
						for (auto streamPortIndexCounter = entity::model::StreamPortIndex(0); streamPortIndexCounter < numberOfStreamPorts; ++streamPortIndexCounter)
						{
							model::StreamPortNode* streamPortNode{ nullptr };
							entity::model::StreamPortNodeStaticModel const* streamPortStaticModel{ nullptr };
							entity::model::StreamPortNodeDynamicModel* streamPortDynamicModel{ nullptr };
							auto const streamPortIndex = entity::model::StreamPortIndex(streamPortIndexCounter + baseStreamPort);

							if (descriptorType == entity::model::DescriptorType::StreamPortInput)
							{
								streamPortNode = &audioUnitNode.streamPortInputs[streamPortIndex];
								streamPortStaticModel = &entity->getNodeStaticModel(configIndex, streamPortIndex, &entity::model::ConfigurationTree::streamPortInputModels);
								streamPortDynamicModel = &entity->getNodeDynamicModel(configIndex, streamPortIndex, &entity::model::ConfigurationTree::streamPortInputModels);
							}
							else
							{
								streamPortNode = &audioUnitNode.streamPortOutputs[streamPortIndex];
								streamPortStaticModel = &entity->getNodeStaticModel(configIndex, streamPortIndex, &entity::model::ConfigurationTree::streamPortOutputModels);
								streamPortDynamicModel = &entity->getNodeDynamicModel(configIndex, streamPortIndex, &entity::model::ConfigurationTree::streamPortOutputModels);
							}

							initNode(*streamPortNode, descriptorType, streamPortIndex);
							streamPortNode->staticModel = streamPortStaticModel;
							streamPortNode->dynamicModel = streamPortDynamicModel;

							// Build audio clusters (AudioClusterNode)
							for (auto clusterIndexCounter = entity::model::ClusterIndex(0); clusterIndexCounter < streamPortStaticModel->numberOfClusters; ++clusterIndexCounter)
							{
								auto const clusterIndex = entity::model::ClusterIndex(clusterIndexCounter + streamPortStaticModel->baseCluster);
								auto& audioClusterNode = streamPortNode->audioClusters[clusterIndex];
								initNode(audioClusterNode, entity::model::DescriptorType::AudioCluster, clusterIndex);

								auto const& audioClusterStaticModel = entity->getNodeStaticModel(configIndex, clusterIndex, &entity::model::ConfigurationTree::audioClusterModels);
								auto& audioClusterDynamicModel = entity->getNodeDynamicModel(configIndex, clusterIndex, &entity::model::ConfigurationTree::audioClusterModels);
								audioClusterNode.staticModel = &audioClusterStaticModel;
								audioClusterNode.dynamicModel = &audioClusterDynamicModel;
							}

							// Build audio maps (AudioMapNode)
							for (auto mapIndexCounter = entity::model::MapIndex(0); mapIndexCounter < streamPortStaticModel->numberOfMaps; ++mapIndexCounter)
							{
								auto const mapIndex = entity::model::MapIndex(mapIndexCounter + streamPortStaticModel->baseMap);
								auto& audioMapNode = streamPortNode->audioMaps[mapIndex];
								initNode(audioMapNode, entity::model::DescriptorType::AudioMap, mapIndex);

								auto const& audioMapStaticModel = entity->getNodeStaticModel(configIndex, mapIndex, &entity::model::ConfigurationTree::audioMapModels);
								audioMapNode.staticModel = &audioMapStaticModel;
							}
						}
					};
					processStreamPorts(entity::model::DescriptorType::StreamPortInput, audioUnitStaticModel.numberOfStreamInputPorts, audioUnitStaticModel.baseStreamInputPort);
					processStreamPorts(entity::model::DescriptorType::StreamPortOutput, audioUnitStaticModel.numberOfStreamOutputPorts, audioUnitStaticModel.baseStreamOutputPort);
				}

				// Build stream inputs (StreamNode)
				for (auto& [streamIndex, streamModels] : configTree.streamInputModels)
				{
					auto const& streamStaticModel = streamModels.staticModel;
					auto& streamDynamicModel = streamModels.dynamicModel;

					auto& streamNode = configNode.streamInputs[streamIndex];
					initNode(streamNode, entity::model::DescriptorType::StreamInput, streamIndex);
					streamNode.staticModel = &streamStaticModel;
					streamNode.dynamicModel = &streamDynamicModel;
				}

				// Build stream outputs (StreamNode)
				for (auto& [streamIndex, streamModels] : configTree.streamOutputModels)
				{
					auto const& streamStaticModel = streamModels.staticModel;
					auto& streamDynamicModel = streamModels.dynamicModel;

					auto& streamNode = configNode.streamOutputs[streamIndex];
					initNode(streamNode, entity::model::DescriptorType::StreamOutput, streamIndex);
					streamNode.staticModel = &streamStaticModel;
					streamNode.dynamicModel = &streamDynamicModel;
				}

				// Build avb interfaces (AvbInterfaceNode)
				for (auto& [interfaceIndex, interfaceModels] : configTree.avbInterfaceModels)
				{
					auto const& interfaceStaticModel = interfaceModels.staticModel;
					auto& interfaceDynamicModel = interfaceModels.dynamicModel;

					auto& interfaceNode = configNode.avbInterfaces[interfaceIndex];
					initNode(interfaceNode, entity::model::DescriptorType::AvbInterface, interfaceIndex);
					interfaceNode.staticModel = &interfaceStaticModel;
					interfaceNode.dynamicModel = &interfaceDynamicModel;
				}

				// Build clock sources (ClockSourceNode)
				for (auto& [sourceIndex, sourceModels] : configTree.clockSourceModels)
				{
					auto const& sourceStaticModel = sourceModels.staticModel;
					auto& sourceDynamicModel = sourceModels.dynamicModel;

					auto& sourceNode = configNode.clockSources[sourceIndex];
					initNode(sourceNode, entity::model::DescriptorType::ClockSource, sourceIndex);
					sourceNode.staticModel = &sourceStaticModel;
					sourceNode.dynamicModel = &sourceDynamicModel;
				}

				// Build memory objects (MemoryObjectNode)
				for (auto& [memoryObjectIndex, memoryObjectModels] : configTree.memoryObjectModels)
				{
					auto const& memoryObjectStaticModel = memoryObjectModels.staticModel;
					auto& memoryObjectDynamicModel = memoryObjectModels.dynamicModel;

					auto& memoryObjectNode = configNode.memoryObjects[memoryObjectIndex];
					initNode(memoryObjectNode, entity::model::DescriptorType::MemoryObject, memoryObjectIndex);
					memoryObjectNode.staticModel = &memoryObjectStaticModel;
					memoryObjectNode.dynamicModel = &memoryObjectDynamicModel;
				}

				// Build locales (LocaleNode)
				for (auto const& [localeIndex, localeModels] : configTree.localeModels)
				{
					auto const& localeStaticModel = localeModels.staticModel;

					auto& localeNode = configNode.locales[localeIndex];
					initNode(localeNode, entity::model::DescriptorType::Locale, localeIndex);
					localeNode.staticModel = &localeStaticModel;

					// Build strings (StringsNode)
					for (auto stringsIndexCounter = entity::model::StringsIndex(0); stringsIndexCounter < localeStaticModel.numberOfStringDescriptors; ++stringsIndexCounter)
					{
						auto const stringsIndex = entity::model::StringsIndex(stringsIndexCounter + localeStaticModel.baseStringDescriptorIndex);
						auto& stringsNode = localeNode.strings[stringsIndex];
						initNode(stringsNode, entity::model::DescriptorType::Strings, stringsIndex);

						// Manually searching the Strings to improve performance (not throwing if Strings not loaded for this Locale), ignoring not loaded strings
						auto const stringsIt = configTree.stringsModels.find(stringsIndex);
						if (stringsIt != configTree.stringsModels.end())
						{
							stringsNode.staticModel = &stringsIt->second.staticModel;
						}
					}
				}

				// Build clock domains (ClockDomainNode)
				for (auto& [domainIndex, domainModels] : configTree.clockDomainModels)
				{
					auto const& domainStaticModel = domainModels.staticModel;
					auto& domainDynamicModel = domainModels.dynamicModel;

					auto& domainNode = configNode.clockDomains[domainIndex];
					initNode(domainNode, entity::model::DescriptorType::ClockDomain, domainIndex);
					domainNode.staticModel = &domainStaticModel;
					domainNode.dynamicModel = &domainDynamicModel;

					// Build associated clock sources (ClockSourceNode)
					for (auto const sourceIndex : domainStaticModel.clockSources)
					{
						auto const sourceIt = configNode.clockSources.find(sourceIndex);
						if (sourceIt != configNode.clockSources.end())
						{
							auto const& sourceNode = sourceIt->second;
							domainNode.clockSources[sourceIndex] = &sourceNode;
						}
					}
				}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				// Build redundancy nodes
				buildRedundancyNodes(configNode);
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
			}
		}
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Should never throw");
		_entityNode = {};
	}
}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
class RedundantHelper : public ControlledEntityImpl
{
public:
	template<typename StreamNodeType>
	static void buildRedundancyNodesByType(la::avdecc::UniqueIdentifier entityID, std::map<entity::model::StreamIndex, StreamNodeType>& streams, std::map<model::VirtualIndex, model::RedundantStreamNode>& redundantStreams, RedundantStreamCategory& redundantPrimaryStreams, RedundantStreamCategory& redundantSecondaryStreams)
	{
		for (auto& streamNodeKV : streams)
		{
			auto const streamIndex = streamNodeKV.first;
			auto& streamNode = streamNodeKV.second;
			auto const* const staticModel = streamNode.staticModel;

			// Check if this node as redundant stream association
			if (staticModel->redundantStreams.empty())
				continue;

#	ifdef ENABLE_AVDECC_STRICT_2018_REDUNDANCY
			// 2018 Redundancy specification only defines stream pairs
			if (staticModel->redundantStreams.size() != 1)
			{
				LOG_CONTROLLER_WARN(entityID, std::string("More than one StreamIndex in RedundantStreamAssociation"));
				continue;
			}
#	endif // ENABLE_AVDECC_STRICT_2018_REDUNDANCY

			// Check each stream in the association is associated back to this stream and the AVB_INTERFACE index is unique
			auto isAssociationValid{ true };
			std::map<entity::model::AvbInterfaceIndex, model::StreamNode*> redundantStreamNodes{};
			redundantStreamNodes.emplace(std::make_pair(staticModel->avbInterfaceIndex, &streamNode));
			for (auto const redundantIndex : staticModel->redundantStreams)
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
				if (redundantStream.staticModel->redundantStreams.find(streamIndex) == redundantStream.staticModel->redundantStreams.end())
				{
					isAssociationValid = false;
					LOG_CONTROLLER_ERROR(entityID, std::string("RedundantStreamAssociation invalid for ") + (streamNode.descriptorType == entity::model::DescriptorType::StreamInput ? "STREAM_INPUT." : "STREAM_OUTPUT.") + std::to_string(streamNode.descriptorIndex) + ": StreamIndex " + std::to_string(redundantIndex) + " doesn't reference back to the stream");
					break;
				}

				auto const redundantInterfaceIndex{ redundantStream.staticModel->avbInterfaceIndex };
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
			auto redundantNodeCreated{ false };
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
					auto& redundantStreamNode = redundantStreams[virtualIndex];
					initNode(redundantStreamNode, streamNode.descriptorType, virtualIndex);

					// Add all streams part of this redundant association
					for (auto& redundantNodeKV : redundantStreamNodes)
					{
						auto* const redundantNode = redundantNodeKV.second;
						redundantStreamNode.redundantStreams.emplace(std::make_pair(redundantNode->descriptorIndex, redundantNode));
						redundantNode->isRedundant = true; // Set this StreamNode as part of a valid redundant stream association
					}

					auto redundantStreamIt = redundantStreamNodes.begin();
					// Defined the primary stream
					redundantStreamNode.primaryStream = redundantStreamIt->second;

					// Cache Primary and Secondary StreamIndexes
					redundantPrimaryStreams.insert(redundantStreamIt->second->descriptorIndex);
					++redundantStreamIt;
					redundantSecondaryStreams.insert(redundantStreamIt->second->descriptorIndex);
				}
			}
		}
	}
};

void ControlledEntityImpl::buildRedundancyNodes(model::ConfigurationNode& configNode) noexcept
{
	RedundantHelper::buildRedundancyNodesByType(_entity.getEntityID(), configNode.streamInputs, configNode.redundantStreamInputs, _redundantPrimaryStreamInputs, _redundantSecondaryStreamInputs);
	RedundantHelper::buildRedundancyNodesByType(_entity.getEntityID(), configNode.streamOutputs, configNode.redundantStreamOutputs, _redundantPrimaryStreamOutputs, _redundantSecondaryStreamOutputs);
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

} // namespace controller
} // namespace avdecc
} // namespace la
