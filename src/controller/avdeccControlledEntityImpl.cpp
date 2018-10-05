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
static constexpr std::uint16_t MaxQueryDescriptorRetryCount = 5;
static constexpr std::uint16_t MaxQueryDynamicInfoRetryCount = 5;
static constexpr std::uint16_t MaxQueryDescriptorDynamicInfoRetryCount = 5;
static constexpr std::uint16_t QueryRetryMillisecondDelay = 500;

/* ************************************************************************** */
/* ControlledEntityImpl                                                       */
/* ************************************************************************** */
/** Constructor */
ControlledEntityImpl::ControlledEntityImpl(entity::Entity const& entity) noexcept
	: _entity(entity)
{
}

// ControlledEntity overrides
// Getters
ControlledEntity::Compatibility ControlledEntityImpl::getCompatibility() const noexcept
{
	return _compatibility;
}

bool ControlledEntityImpl::gotFatalEnumerationError() const noexcept
{
	return _gotFatalEnumerateError;
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

bool ControlledEntityImpl::isStreamInputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& dynamicModel = getNodeDynamicModel(configurationIndex, streamIndex, &model::ConfigurationDynamicTree::streamInputDynamicModels);
	return isStreamRunningFlag(dynamicModel.streamInfo.streamInfoFlags);
}

bool ControlledEntityImpl::isStreamOutputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& dynamicModel = getNodeDynamicModel(configurationIndex, streamIndex, &model::ConfigurationDynamicTree::streamOutputDynamicModels);
	return isStreamRunningFlag(dynamicModel.streamInfo.streamInfoFlags);
}

UniqueIdentifier ControlledEntityImpl::getOwningControllerID() const noexcept
{
	return _owningControllerID;
}

entity::Entity const& ControlledEntityImpl::getEntity() const noexcept
{
	return _entity;
}

model::EntityNode const& ControlledEntityImpl::getEntityNode() const
{
	if (gotFatalEnumerationError())
		throw Exception(Exception::Type::EnumerationError, "Entity had an enumeration error");

	if (!hasFlag(_entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		throw Exception(Exception::Type::NotSupported, "EM not supported by the entity");

	checkAndBuildEntityModelGraph();
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

model::LocaleNodeStaticModel const* ControlledEntityImpl::findLocaleNode(entity::model::ConfigurationIndex const configurationIndex, std::string const& /*locale*/) const
{
	auto const& configStaticTree = getConfigurationStaticTree(configurationIndex);

	if (configStaticTree.localeStaticModels.empty())
		throw Exception(Exception::Type::InvalidLocaleName, "Entity has no locale");

#pragma message("TODO: Parse 'locale' parameter and find best match")
	// Right now, return the first locale
	return &configStaticTree.localeStaticModels.at(0);
}

entity::model::AvdeccFixedString const& ControlledEntityImpl::getLocalizedString(entity::model::LocalizedStringReference const stringReference) const noexcept
{
	return getLocalizedString(getCurrentConfigurationIndex(), stringReference);
}

entity::model::AvdeccFixedString const& ControlledEntityImpl::getLocalizedString(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocalizedStringReference const stringReference) const noexcept
{
	static entity::model::AvdeccFixedString s_noLocalizationString{};
	try
	{
		// Special value meaning NO_STRING
		if (stringReference == entity::model::getNullLocalizedStringReference())
			return s_noLocalizationString;

		auto const offset = stringReference >> 3;
		auto const index = stringReference & 0x0007;
		auto const globalOffset = ((offset * 7u) + index) & 0xFFFF;

		auto const& configDynamicModel = getConfigurationNodeDynamicModel(configurationIndex);

		return configDynamicModel.localizedStrings.at(entity::model::StringsIndex(globalOffset));
	}
	catch (...)
	{
		return s_noLocalizationString;
	}
}

model::StreamConnectionState const& ControlledEntityImpl::getConnectedSinkState(entity::model::StreamIndex const streamIndex) const
{
	auto const& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &model::ConfigurationDynamicTree::streamInputDynamicModels);

	AVDECC_ASSERT(_entity.getEntityID() == dynamicModel.connectionState.listenerStream.entityID, "EntityID not correctly initialized");
	AVDECC_ASSERT(streamIndex == dynamicModel.connectionState.listenerStream.streamIndex, "StreamIndex not correctly initialized");
	return dynamicModel.connectionState;
}

entity::model::AudioMappings const& ControlledEntityImpl::getStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const currentConfiguration = getCurrentConfigurationIndex();

	// Check if dynamic mappings is supported by the entity
	auto const& staticModel = getNodeStaticModel(currentConfiguration, streamPortIndex, &model::ConfigurationStaticTree::streamPortInputStaticModels);
	if (!staticModel.hasDynamicAudioMap)
		throw Exception(Exception::Type::NotSupported, "Dynamic mappings not supported by this stream port");

	// Return dynamic mappings for this stream port
	auto const& dynamicModel = getNodeDynamicModel(currentConfiguration, streamPortIndex, &model::ConfigurationDynamicTree::streamPortInputDynamicModels);
	return dynamicModel.dynamicAudioMap;
}

entity::model::AudioMappings const& ControlledEntityImpl::getStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const currentConfiguration = getCurrentConfigurationIndex();

	// Check if dynamic mappings is supported by the entity
	auto const& staticModel = getNodeStaticModel(currentConfiguration, streamPortIndex, &model::ConfigurationStaticTree::streamPortOutputStaticModels);
	if (!staticModel.hasDynamicAudioMap)
		throw Exception(Exception::Type::NotSupported, "Dynamic mappings not supported by this stream port");

	// Return dynamic mappings for this stream port
	auto const& dynamicModel = getNodeDynamicModel(currentConfiguration, streamPortIndex, &model::ConfigurationDynamicTree::streamPortOutputDynamicModels);
	return dynamicModel.dynamicAudioMap;
}

model::StreamConnections const& ControlledEntityImpl::getStreamOutputConnections(entity::model::StreamIndex const streamIndex) const
{
	auto const& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &model::ConfigurationDynamicTree::streamOutputDynamicModels);

	return dynamicModel.connections;
}

// Visitor method
void ControlledEntityImpl::accept(model::EntityModelVisitor* const visitor) const noexcept
{
	if (_gotFatalEnumerateError)
		return;

	if (visitor == nullptr)
		return;

	if (!hasFlag(_entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		return;

	try
	{
		// Visit entity model graph
		auto const& entityModel = getEntityNode();

		// Visit EntityModelNode (no parent)
		visitor->visit(this, nullptr, entityModel);

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
					auto processStreamPorts = [this, visitor](model::AudioUnitNode const& audioUnit, std::map<entity::model::StreamPortIndex, model::StreamPortNode> const& streamPorts)
					{
						for (auto const& streamPortKV : streamPorts)
						{
							auto const& streamPort = streamPortKV.second;
							// Visit StreamPortNode (AudioUnitNode is parent)
							visitor->visit(this, &audioUnit, streamPort);

							// Loop over AudioClusterNode
							for (auto const& audioClusterKV : streamPort.audioClusters)
							{
								auto const& audioCluster = audioClusterKV.second;
								// Visit AudioClusterNode (StreamPortNode is parent)
								visitor->visit(this, &streamPort, audioCluster);
							}

							// Loop over AudioMapNode
							for (auto const& audioMapKV : streamPort.audioMaps)
							{
								auto const& audioMap = audioMapKV.second;
								// Visit AudioMapNode (StreamPortNode is parent)
								visitor->visit(this, &streamPort, audioMap);
							}
						}
					};
					processStreamPorts(audioUnit, audioUnit.streamPortInputs); // streamPortInputs
					processStreamPorts(audioUnit, audioUnit.streamPortOutputs); // streamPortOutputs
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
						visitor->visit(this, &redundantStream, *stream);
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
						visitor->visit(this, &redundantStream, *stream);
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
					// TODO
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
						visitor->visit(this, &domain, *source);
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
	_lock.lock();
	if (_lockedCount == 0)
	{
		_lockingThreadID = std::this_thread::get_id();
	}
	++_lockedCount;
}

void ControlledEntityImpl::unlock() noexcept
{
	--_lockedCount;
	if (_lockedCount == 0)
	{
		_lockingThreadID = {};
	}
	_lock.unlock();
}

bool ControlledEntityImpl::isSelfLocked() const noexcept
{
	return _lockingThreadID == std::this_thread::get_id();
}

// Const Tree getters, all throw Exception::NotSupported if EM not supported by the Entity, Exception::InvalidConfigurationIndex if configurationIndex do not exist
model::EntityStaticTree const& ControlledEntityImpl::getEntityStaticTree() const
{
	if (gotFatalEnumerationError())
		throw Exception(Exception::Type::EnumerationError, "Entity had a fatal enumeration error");

	if (!hasFlag(_entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		throw Exception(Exception::Type::NotSupported, "EM not supported by the entity");

	return _entityStaticTree;
}

model::EntityDynamicTree const& ControlledEntityImpl::getEntityDynamicTree() const
{
	if (gotFatalEnumerationError())
		throw Exception(Exception::Type::EnumerationError, "Entity had a fatal enumeration error");

	if (!hasFlag(_entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		throw Exception(Exception::Type::NotSupported, "EM not supported by the entity");

	return _entityDynamicTree;
}

model::ConfigurationStaticTree const& ControlledEntityImpl::getConfigurationStaticTree(entity::model::ConfigurationIndex const configurationIndex) const
{
	auto const& entityStaticTree = getEntityStaticTree();
	auto const it = entityStaticTree.configurationStaticTrees.find(configurationIndex);
	if (it == entityStaticTree.configurationStaticTrees.end())
		throw Exception(Exception::Type::InvalidConfigurationIndex, "Invalid configuration index");

	return it->second;
}

model::ConfigurationDynamicTree const& ControlledEntityImpl::getConfigurationDynamicTree(entity::model::ConfigurationIndex const configurationIndex) const
{
	auto const& entityDynamicTree = getEntityDynamicTree();
	auto const it = entityDynamicTree.configurationDynamicTrees.find(configurationIndex);
	if (it == entityDynamicTree.configurationDynamicTrees.end())
		throw Exception(Exception::Type::InvalidConfigurationIndex, "Invalid configuration index");

	return it->second;
}

entity::model::ConfigurationIndex ControlledEntityImpl::getCurrentConfigurationIndex() const noexcept
{
	return _entityDynamicTree.dynamicModel.currentConfiguration;
}

// Const NodeModel getters, all throw Exception::NotSupported if EM not supported by the Entity, Exception::InvalidConfigurationIndex if configurationIndex do not exist, Exception::InvalidDescriptorIndex if descriptorIndex is invalid
model::EntityNodeStaticModel const& ControlledEntityImpl::getEntityNodeStaticModel() const
{
	return getEntityStaticTree().staticModel;
}

model::EntityNodeDynamicModel const& ControlledEntityImpl::getEntityNodeDynamicModel() const
{
	return getEntityDynamicTree().dynamicModel;
}

model::ConfigurationNodeStaticModel const& ControlledEntityImpl::getConfigurationNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex) const
{
	return getConfigurationStaticTree(configurationIndex).staticModel;
}

model::ConfigurationNodeDynamicModel const& ControlledEntityImpl::getConfigurationNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex) const
{
	return getConfigurationDynamicTree(configurationIndex).dynamicModel;
}

// Non-const Tree getters
model::EntityStaticTree& ControlledEntityImpl::getEntityStaticTree() noexcept
{
	return _entityStaticTree;
}

model::EntityDynamicTree& ControlledEntityImpl::getEntityDynamicTree() noexcept
{
	return _entityDynamicTree;
}

model::ConfigurationStaticTree& ControlledEntityImpl::getConfigurationStaticTree(entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	auto& entityStaticTree = getEntityStaticTree();
	return entityStaticTree.configurationStaticTrees[configurationIndex];
}

model::ConfigurationDynamicTree& ControlledEntityImpl::getConfigurationDynamicTree(entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	auto& entityDynamicTree = getEntityDynamicTree();
	return entityDynamicTree.configurationDynamicTrees[configurationIndex];
}

// Non-const NodeModel getters
model::EntityNodeStaticModel& ControlledEntityImpl::getEntityNodeStaticModel() noexcept
{
	return getEntityStaticTree().staticModel;
}

model::EntityNodeDynamicModel& ControlledEntityImpl::getEntityNodeDynamicModel() noexcept
{
	return getEntityDynamicTree().dynamicModel;
}

model::ConfigurationNodeStaticModel& ControlledEntityImpl::getConfigurationNodeStaticModel(entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	return getConfigurationStaticTree(configurationIndex).staticModel;
}

model::ConfigurationNodeDynamicModel& ControlledEntityImpl::getConfigurationNodeDynamicModel(entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	return getConfigurationDynamicTree(configurationIndex).dynamicModel;
}

// Setters of the DescriptorDynamic info, all throw Exception::NotSupported if EM not supported by the Entity, Exception::InvalidConfigurationIndex if configurationIndex do not exist, Exception::InvalidDescriptorIndex if descriptorIndex is invalid
void ControlledEntityImpl::setEntityName(entity::model::AvdeccFixedString const& name) noexcept
{
	_entityDynamicTree.dynamicModel.entityName = name;
}

void ControlledEntityImpl::setEntityGroupName(entity::model::AvdeccFixedString const& name) noexcept
{
	_entityDynamicTree.dynamicModel.groupName = name;
}

void ControlledEntityImpl::setCurrentConfiguration(entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	_entityDynamicTree.dynamicModel.currentConfiguration = configurationIndex;

	// Set isActiveConfiguration for each configuration
	for (auto& confIt : _entityDynamicTree.configurationDynamicTrees)
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
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), audioUnitIndex, &model::ConfigurationDynamicTree::audioUnitDynamicModels);
	dynamicModel.currentSamplingRate = samplingRate;
}

void ControlledEntityImpl::setStreamInputFormat(entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &model::ConfigurationDynamicTree::streamInputDynamicModels);
	dynamicModel.currentFormat = streamFormat;
}

model::StreamConnectionState ControlledEntityImpl::setStreamInputConnectionState(entity::model::StreamIndex const streamIndex, model::StreamConnectionState const& state) noexcept
{
	AVDECC_ASSERT(_entity.getEntityID() == state.listenerStream.entityID, "EntityID not correctly initialized");
	AVDECC_ASSERT(streamIndex == state.listenerStream.streamIndex, "StreamIndex not correctly initialized");

	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &model::ConfigurationDynamicTree::streamInputDynamicModels);

	// Save previous StreamConnectionState
	auto previousState = dynamicModel.connectionState;

	// Set StreamConnectionState
	dynamicModel.connectionState = state;

	return previousState;
}

entity::model::StreamInfo ControlledEntityImpl::setStreamInputInfo(entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &model::ConfigurationDynamicTree::streamInputDynamicModels);

	// Save previous StreamInfo
	auto previousInfo = dynamicModel.streamInfo;

	// Set StreamInfo
	dynamicModel.streamInfo = info;

	return previousInfo;
}

void ControlledEntityImpl::setStreamOutputFormat(entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &model::ConfigurationDynamicTree::streamOutputDynamicModels);
	dynamicModel.currentFormat = streamFormat;
}

void ControlledEntityImpl::clearStreamOutputConnections(entity::model::StreamIndex const streamIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &model::ConfigurationDynamicTree::streamOutputDynamicModels);
	dynamicModel.connections.clear();
}

bool ControlledEntityImpl::addStreamOutputConnection(entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& listenerStream) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &model::ConfigurationDynamicTree::streamOutputDynamicModels);
	auto const result = dynamicModel.connections.insert(listenerStream);
	return result.second;
}

bool ControlledEntityImpl::delStreamOutputConnection(entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& listenerStream) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &model::ConfigurationDynamicTree::streamOutputDynamicModels);
	return dynamicModel.connections.erase(listenerStream) > 0;
}

entity::model::StreamInfo ControlledEntityImpl::setStreamOutputInfo(entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &model::ConfigurationDynamicTree::streamOutputDynamicModels);

	// Save previous StreamInfo
	auto previousInfo = dynamicModel.streamInfo;

	// Set StreamInfo
	dynamicModel.streamInfo = info;

	return previousInfo;
}

entity::model::AvbInfo ControlledEntityImpl::setAvbInfo(entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), avbInterfaceIndex, &model::ConfigurationDynamicTree::avbInterfaceDynamicModels);

	// Save previous AvbInfo
	auto previousInfo = dynamicModel.avbInfo;

	// Set AvbInfo
	dynamicModel.avbInfo = info;

	return previousInfo;
}

void ControlledEntityImpl::setSelectedLocaleBaseIndex(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const baseIndex) noexcept
{
	auto& dynamicModel = getConfigurationNodeDynamicModel(configurationIndex);
	dynamicModel.selectedLocaleBaseIndex = baseIndex;
}

void ControlledEntityImpl::clearStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamPortIndex, &model::ConfigurationDynamicTree::streamPortInputDynamicModels);
	dynamicModel.dynamicAudioMap.clear();
}

void ControlledEntityImpl::addStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamPortIndex, &model::ConfigurationDynamicTree::streamPortInputDynamicModels);
	auto& dynamicMap = dynamicModel.dynamicAudioMap;

	// Process audio mappings
	for (auto const& map : mappings)
	{
		// Check if mapping must be replaced
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
		else // Otherwise, replace the previous mapping
		{
#ifndef ENABLE_AVDECC_FEATURE_REDUNDANCY
			LOG_CONTROLLER_WARN(_entity.getEntityID(), std::string("Duplicate StreamPortInput AudioMappings found: ") + std::to_string(foundIt->streamIndex) + ":" + std::to_string(foundIt->streamChannel) + ":" + std::to_string(foundIt->clusterOffset) + ":" + std::to_string(foundIt->clusterChannel) + " replaced by " + std::to_string(map.streamIndex) + ":" + std::to_string(map.streamChannel) + ":" + std::to_string(map.clusterOffset) + ":" + std::to_string(map.clusterChannel));
#endif // !ENABLE_AVDECC_FEATURE_REDUNDANCY
			foundIt->streamIndex = map.streamIndex;
			foundIt->streamChannel = map.streamChannel;
		}
	}
}

void ControlledEntityImpl::removeStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamPortIndex, &model::ConfigurationDynamicTree::streamPortInputDynamicModels);
	auto& dynamicMap = dynamicModel.dynamicAudioMap;

	// Process audio mappings
	for (auto const& map : mappings)
	{
		// Check if mapping exists
		auto foundIt = std::find_if(dynamicMap.begin(), dynamicMap.end(),
			[&map](entity::model::AudioMapping const& mapping)
			{
				return (map.clusterOffset == mapping.clusterOffset) && (map.clusterChannel == mapping.clusterChannel);
			});
		if (AVDECC_ASSERT_WITH_RET(foundIt != dynamicMap.end(), "Mapping not found"))
			dynamicMap.erase(foundIt);
	}
}

void ControlledEntityImpl::clearStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamPortIndex, &model::ConfigurationDynamicTree::streamPortOutputDynamicModels);
	dynamicModel.dynamicAudioMap.clear();
}

void ControlledEntityImpl::addStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamPortIndex, &model::ConfigurationDynamicTree::streamPortOutputDynamicModels);
	auto& dynamicMap = dynamicModel.dynamicAudioMap;

	// Process audio mappings
	for (auto const& map : mappings)
	{
		// Check if mapping must be replaced
		auto foundIt = std::find_if(dynamicMap.begin(), dynamicMap.end(),
			[&map](entity::model::AudioMapping const& mapping)
			{
				return (map.streamIndex == mapping.streamIndex) && (map.streamChannel == mapping.streamChannel);
			});
		// Not found, add the new mapping
		if (foundIt == dynamicMap.end())
		{
			dynamicMap.push_back(map);
		}
		else // Otherwise, replace the previous mapping
		{
#ifndef ENABLE_AVDECC_FEATURE_REDUNDANCY
			LOG_CONTROLLER_WARN(_entity.getEntityID(), std::string("Duplicate StreamPortOutput AudioMappings found: ") + std::to_string(foundIt->streamIndex) + ":" + std::to_string(foundIt->streamChannel) + ":" + std::to_string(foundIt->clusterOffset) + ":" + std::to_string(foundIt->clusterChannel) + " replaced by " + std::to_string(map.streamIndex) + ":" + std::to_string(map.streamChannel) + ":" + std::to_string(map.clusterOffset) + ":" + std::to_string(map.clusterChannel));
#endif // !ENABLE_AVDECC_FEATURE_REDUNDANCY
			foundIt->streamIndex = map.streamIndex;
			foundIt->streamChannel = map.streamChannel;
		}
	}
}

void ControlledEntityImpl::removeStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamPortIndex, &model::ConfigurationDynamicTree::streamPortOutputDynamicModels);
	auto& dynamicMap = dynamicModel.dynamicAudioMap;

	// Process audio mappings
	for (auto const& map : mappings)
	{
		// Check if mapping exists
		auto foundIt = std::find_if(dynamicMap.begin(), dynamicMap.end(),
			[&map](entity::model::AudioMapping const& mapping)
			{
				return (map.streamIndex == mapping.streamIndex) && (map.streamChannel == mapping.streamChannel);
			});
		if (AVDECC_ASSERT_WITH_RET(foundIt != dynamicMap.end(), "Mapping not found"))
			dynamicMap.erase(foundIt);
	}
}

void ControlledEntityImpl::setClockSource(entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), clockDomainIndex, &model::ConfigurationDynamicTree::clockDomainDynamicModels);
	dynamicModel.clockSourceIndex = clockSourceIndex;
}

void ControlledEntityImpl::setMemoryObjectLength(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(configurationIndex, memoryObjectIndex, &model::ConfigurationDynamicTree::memoryObjectDynamicModels);
	dynamicModel.length = length;
}

model::AvbInterfaceCounters& ControlledEntityImpl::getAvbInterfaceCounters(entity::model::AvbInterfaceIndex const avbInterfaceIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), avbInterfaceIndex, &model::ConfigurationDynamicTree::avbInterfaceDynamicModels);
	return dynamicModel.counters;
}

model::ClockDomainCounters& ControlledEntityImpl::getClockDomainCounters(entity::model::ClockDomainIndex const clockDomainIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), clockDomainIndex, &model::ConfigurationDynamicTree::clockDomainDynamicModels);
	return dynamicModel.counters;
}

model::StreamInputCounters& ControlledEntityImpl::getStreamInputCounters(entity::model::StreamIndex const streamIndex) noexcept
{
	auto& dynamicModel = getNodeDynamicModel(getCurrentConfigurationIndex(), streamIndex, &model::ConfigurationDynamicTree::streamInputDynamicModels);
	return dynamicModel.counters;
}

// Setters (of the model, not the physical entity)
void ControlledEntityImpl::setEntity(entity::Entity const& entity) noexcept
{
	_entity = entity;
}

void ControlledEntityImpl::setAcquireState(model::AcquireState const state) noexcept
{
	_acquireState = state;
}

void ControlledEntityImpl::setOwningController(UniqueIdentifier const controllerID) noexcept
{
	_owningControllerID = controllerID;
}

// Setters of the Model from AEM Descriptors (including DescriptorDynamic info)

bool ControlledEntityImpl::setCachedEntityStaticTree(model::EntityStaticTree const& cachedStaticTree, entity::model::EntityDescriptor const& descriptor) noexcept
{
	// Check if static information in EntityDescriptor are identical
	auto const& cachedDescriptor = cachedStaticTree.staticModel;
	if (cachedDescriptor.vendorNameString != descriptor.vendorNameString || cachedDescriptor.modelNameString != descriptor.modelNameString)
	{
		LOG_CONTROLLER_WARN(_entity.getEntityID(), "EntityModelID provided by this Entity has inconsistent data in it's EntityDescriptor, not using cached AEM");
		return false;
	}

	// Ok the static information from EntityDescriptor are identical, we cannot check more than this so we have to assume it's correct, copy the whole model
	_entityStaticTree = cachedStaticTree;

	// And override with the EntityDescriptor so this entity's specific fields are copied
	setEntityDescriptor(descriptor);

	return true;
}

void ControlledEntityImpl::setEntityDescriptor(entity::model::EntityDescriptor const& descriptor) noexcept
{
	if (!AVDECC_ASSERT_WITH_RET(!_advertised, "EntityDescriptor should never be set twice on an entity. Only the dynamic part should be set again."))
	{
		// Wipe everything and set as enumeration error
		_entityStaticTree = {};
		_entityDynamicTree = {};
		_entityNode = {};
		_gotFatalEnumerateError = true;

		return;
	}

	// Copy static model
	{
		auto& m = _entityStaticTree.staticModel;
		m.vendorNameString = descriptor.vendorNameString;
		m.modelNameString = descriptor.modelNameString;
	}

	// Copy dynamic model
	{
		auto& m = _entityDynamicTree.dynamicModel;
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
		auto& m = getNodeStaticModel(configurationIndex, audioUnitIndex, &model::ConfigurationStaticTree::audioUnitStaticModels);
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
		auto& m = getNodeDynamicModel(configurationIndex, audioUnitIndex, &model::ConfigurationDynamicTree::audioUnitDynamicModels);
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
		auto& m = getNodeStaticModel(configurationIndex, streamIndex, &model::ConfigurationStaticTree::streamInputStaticModels);
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
		auto& m = getNodeDynamicModel(configurationIndex, streamIndex, &model::ConfigurationDynamicTree::streamInputDynamicModels);
		// Not changeable fields
		m.connectionState.listenerStream = entity::model::StreamIdentification{ _entity.getEntityID(), streamIndex }; // We always are the other endpoint of a connection, initialize this now
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
		m.currentFormat = descriptor.currentFormat;
	}
}

void ControlledEntityImpl::setStreamOutputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::StreamNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, streamIndex, &model::ConfigurationStaticTree::streamOutputStaticModels);
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
		auto& m = getNodeDynamicModel(configurationIndex, streamIndex, &model::ConfigurationDynamicTree::streamOutputDynamicModels);
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
		m.currentFormat = descriptor.currentFormat;
	}
}

void ControlledEntityImpl::setAvbInterfaceDescriptor(entity::model::AvbInterfaceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const interfaceIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::AvbInterfaceNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, interfaceIndex, &model::ConfigurationStaticTree::avbInterfaceStaticModels);
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
		auto& m = getNodeDynamicModel(configurationIndex, interfaceIndex, &model::ConfigurationDynamicTree::avbInterfaceDynamicModels);
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setClockSourceDescriptor(entity::model::ClockSourceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::ClockSourceNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, clockIndex, &model::ConfigurationStaticTree::clockSourceStaticModels);
		m.localizedDescription = descriptor.localizedDescription;
		m.clockSourceType = descriptor.clockSourceType;
		m.clockSourceLocationType = descriptor.clockSourceLocationType;
		m.clockSourceLocationIndex = descriptor.clockSourceLocationIndex;
	}

	// Copy dynamic model
	{
		// Get or create a new model::ClockSourceNodeDynamicModel
		auto& m = getNodeDynamicModel(configurationIndex, clockIndex, &model::ConfigurationDynamicTree::clockSourceDynamicModels);
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
		auto& m = getNodeStaticModel(configurationIndex, memoryObjectIndex, &model::ConfigurationStaticTree::memoryObjectStaticModels);
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
		auto& m = getNodeDynamicModel(configurationIndex, memoryObjectIndex, &model::ConfigurationDynamicTree::memoryObjectDynamicModels);
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
		auto& m = getNodeStaticModel(configurationIndex, localeIndex, &model::ConfigurationStaticTree::localeStaticModels);
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
		auto& m = getNodeStaticModel(configurationIndex, stringsIndex, &model::ConfigurationStaticTree::stringsStaticModels);
		m.strings = descriptor.strings;
	}

	// Copy the strings to the ConfigurationDynamicModel for a quick access
	setLocalizedStrings(configurationIndex, stringsIndex, descriptor.strings);
}

void ControlledEntityImpl::setLocalizedStrings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex, model::AvdeccFixedStrings const& strings) noexcept
{
	auto& configDynamicModel = getConfigurationNodeDynamicModel(configurationIndex);
	// Copy the strings to the ConfigurationDynamicModel for a quick access
	for (auto strIndex = 0u; strIndex < strings.size(); ++strIndex)
	{
		auto localizedStringIndex = entity::model::StringsIndex(stringsIndex * strings.size() + strIndex);
		configDynamicModel.localizedStrings[localizedStringIndex] = strings.at(strIndex);
	}
}

void ControlledEntityImpl::setStreamPortInputDescriptor(entity::model::StreamPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::StreamPortNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, streamPortIndex, &model::ConfigurationStaticTree::streamPortInputStaticModels);
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
		auto& m = getNodeStaticModel(configurationIndex, streamPortIndex, &model::ConfigurationStaticTree::streamPortOutputStaticModels);
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
		auto& m = getNodeStaticModel(configurationIndex, clusterIndex, &model::ConfigurationStaticTree::audioClusterStaticModels);
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
		auto& m = getNodeDynamicModel(configurationIndex, clusterIndex, &model::ConfigurationDynamicTree::audioClusterDynamicModels);
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setAudioMapDescriptor(entity::model::AudioMapDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::AudioMapNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, mapIndex, &model::ConfigurationStaticTree::audioMapStaticModels);
		m.mappings = descriptor.mappings;
	}
}

void ControlledEntityImpl::setClockDomainDescriptor(entity::model::ClockDomainDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex) noexcept
{
	// Copy static model
	{
		// Get or create a new model::ClockDomainNodeStaticModel
		auto& m = getNodeStaticModel(configurationIndex, clockDomainIndex, &model::ConfigurationStaticTree::clockDomainStaticModels);
		m.localizedDescription = descriptor.localizedDescription;
		m.clockSources = descriptor.clockSources;
	}

	// Copy dynamic model
	{
		// Get or create a new model::ClockDomainNodeDynamicModel
		auto& m = getNodeDynamicModel(configurationIndex, clockDomainIndex, &model::ConfigurationDynamicTree::clockDomainDynamicModels);
		// Changeable fields through commands
		m.objectName = descriptor.objectName;
		m.clockSourceIndex = descriptor.clockSourceIndex;
	}
}

// Expected descriptor query methods
static inline ControlledEntityImpl::DescriptorKey makeDescriptorKey(entity::model::DescriptorType descriptorType, entity::model::DescriptorIndex descriptorIndex)
{
	return (la::avdecc::to_integral(descriptorType) << (sizeof(descriptorIndex) * 8)) + descriptorIndex;
}

bool ControlledEntityImpl::checkAndClearExpectedDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	// Lock during access to the map
	std::lock_guard<decltype(_lock)> const lg(_lock);

	auto const confIt = _expectedDescriptors.find(configurationIndex);

	if (confIt == _expectedDescriptors.end())
		return false;

	auto const key = makeDescriptorKey(descriptorType, descriptorIndex);

	return confIt->second.erase(key) == 1;
}

void ControlledEntityImpl::setDescriptorExpected(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	// Lock during access to the map
	std::lock_guard<decltype(_lock)> const lg(_lock);

	auto& conf = _expectedDescriptors[configurationIndex];

	auto const key = makeDescriptorKey(descriptorType, descriptorIndex);
	conf.insert(key);
}

bool ControlledEntityImpl::gotAllExpectedDescriptors() const noexcept
{
	// Lock during access to the map
	std::lock_guard<decltype(_lock)> const lg(_lock);

	for (auto const& confKV : _expectedDescriptors)
	{
		if (confKV.second.size() != 0)
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
	return (static_cast<ControlledEntityImpl::DynamicInfoKey>(la::avdecc::to_integral(dynamicInfoType)) << ((sizeof(descriptorIndex) + sizeof(subIndex)) * 8)) + (descriptorIndex << (sizeof(subIndex) * 8)) + subIndex;
}

bool ControlledEntityImpl::checkAndClearExpectedDynamicInfo(entity::model::ConfigurationIndex const configurationIndex, DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex) noexcept
{
	// Lock during access to the map
	std::lock_guard<decltype(_lock)> const lg(_lock);

	auto const confIt = _expectedDynamicInfo.find(configurationIndex);

	if (confIt == _expectedDynamicInfo.end())
		return false;

	auto const key = makeDynamicInfoKey(dynamicInfoType, descriptorIndex, subIndex);

	return confIt->second.erase(key) == 1;
}

void ControlledEntityImpl::setDynamicInfoExpected(entity::model::ConfigurationIndex const configurationIndex, DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const subIndex) noexcept
{
	// Lock during access to the map
	std::lock_guard<decltype(_lock)> const lg(_lock);

	auto& conf = _expectedDynamicInfo[configurationIndex];

	auto const key = makeDynamicInfoKey(dynamicInfoType, descriptorIndex, subIndex);
	conf.insert(key);
}

bool ControlledEntityImpl::gotAllExpectedDynamicInfo() const noexcept
{
	// Lock during access to the map
	std::lock_guard<decltype(_lock)> const lg(_lock);

	for (auto const& confKV : _expectedDynamicInfo)
	{
		if (confKV.second.size() != 0)
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
	return (static_cast<ControlledEntityImpl::DescriptorDynamicInfoKey>(la::avdecc::to_integral(descriptorDynamicInfoType)) << (sizeof(descriptorIndex) * 8)) + descriptorIndex;
}

bool ControlledEntityImpl::checkAndClearExpectedDescriptorDynamicInfo(entity::model::ConfigurationIndex const configurationIndex, DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	// Lock during access to the map
	std::lock_guard<decltype(_lock)> const lg(_lock);

	auto const confIt = _expectedDescriptorDynamicInfo.find(configurationIndex);

	if (confIt == _expectedDescriptorDynamicInfo.end())
		return false;

	auto const key = makeDescriptorDynamicInfoKey(descriptorDynamicInfoType, descriptorIndex);

	return confIt->second.erase(key) == 1;
}

void ControlledEntityImpl::setDescriptorDynamicInfoExpected(entity::model::ConfigurationIndex const configurationIndex, DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	// Lock during access to the map
	std::lock_guard<decltype(_lock)> const lg(_lock);

	auto& conf = _expectedDescriptorDynamicInfo[configurationIndex];

	auto const key = makeDescriptorDynamicInfoKey(descriptorDynamicInfoType, descriptorIndex);
	conf.insert(key);
}

void ControlledEntityImpl::clearAllExpectedDescriptorDynamicInfo() noexcept
{
	// Lock during access to the map
	std::lock_guard<decltype(_lock)> const lg(_lock);

	_expectedDescriptorDynamicInfo.clear();
}

bool ControlledEntityImpl::gotAllExpectedDescriptorDynamicInfo() const noexcept
{
	// Lock during access to the map
	std::lock_guard<decltype(_lock)> const lg(_lock);

	for (auto const& confKV : _expectedDescriptorDynamicInfo)
	{
		if (confKV.second.size() != 0)
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

void ControlledEntityImpl::addEnumerationSteps(EnumerationSteps const steps) noexcept
{
	addFlag(_enumerationSteps, steps);
}

void ControlledEntityImpl::clearEnumerationSteps(EnumerationSteps const steps) noexcept
{
	clearFlag(_enumerationSteps, steps);
}

void ControlledEntityImpl::setCompatibility(Compatibility const compatibility) noexcept
{
	_compatibility = compatibility;
}

void ControlledEntityImpl::setGetFatalEnumerationError() noexcept
{
	LOG_CONTROLLER_ERROR(_entity.getEntityID(), "Got Fatal Enumeration Error");
	_gotFatalEnumerateError = true;
}

bool ControlledEntityImpl::wasAdvertised() const noexcept
{
	return _advertised;
}

void ControlledEntityImpl::setAdvertised(bool const wasAdvertised) noexcept
{
	_advertised = wasAdvertised;
}

// Private methods
void ControlledEntityImpl::checkAndBuildEntityModelGraph() const noexcept
{
	try
	{
#pragma message("TODO: Use a std::optional when available, instead of detecting the non-initialized value from configurations count (xcode clang is still missing optional as of this date)")
		if (_entityNode.configurations.size() == 0)
		{
			// Build root node (EntityNode)
			initNode(_entityNode, entity::model::DescriptorType::Entity, 0, model::AcquireState::Undefined);
			_entityNode.staticModel = &_entityStaticTree.staticModel;
			_entityNode.dynamicModel = &_entityDynamicTree.dynamicModel;

			// Build configuration nodes (ConfigurationNode)
			for (auto& configKV : _entityStaticTree.configurationStaticTrees)
			{
				auto const configIndex = configKV.first;
				auto const& configStaticTree = configKV.second;
				auto& configDynamicTree = _entityDynamicTree.configurationDynamicTrees[configIndex];

				auto& configNode = _entityNode.configurations[configIndex];
				initNode(configNode, entity::model::DescriptorType::Configuration, configIndex, model::AcquireState::Undefined);
				configNode.staticModel = &configStaticTree.staticModel;
				configNode.dynamicModel = &configDynamicTree.dynamicModel;

				// Build audio units (AudioUnitNode)
				for (auto& audioUnitKV : configStaticTree.audioUnitStaticModels)
				{
					auto const audioUnitIndex = audioUnitKV.first;
					auto const& audioUnitStaticModel = audioUnitKV.second;
					auto& audioUnitDynamicModel = configDynamicTree.audioUnitDynamicModels[audioUnitIndex];

					auto& audioUnitNode = configNode.audioUnits[audioUnitIndex];
					initNode(audioUnitNode, entity::model::DescriptorType::AudioUnit, audioUnitIndex, model::AcquireState::Undefined);
					audioUnitNode.staticModel = &audioUnitStaticModel;
					audioUnitNode.dynamicModel = &audioUnitDynamicModel;

					// Build stream port inputs and outputs (StreamPortNode)
					auto processStreamPorts = [entity = const_cast<ControlledEntityImpl*>(this), &audioUnitNode, configIndex](entity::model::DescriptorType const descriptorType, std::uint16_t const numberOfStreamPorts, entity::model::StreamPortIndex const baseStreamPort)
					{
						for (auto streamPortIndexCounter = entity::model::StreamPortIndex(0); streamPortIndexCounter < numberOfStreamPorts; ++streamPortIndexCounter)
						{
							model::StreamPortNode* streamPortNode{ nullptr };
							model::StreamPortNodeStaticModel const* streamPortStaticModel{ nullptr };
							model::StreamPortNodeDynamicModel* streamPortDynamicModel{ nullptr };
							auto const streamPortIndex = entity::model::StreamPortIndex(streamPortIndexCounter + baseStreamPort);

							if (descriptorType == entity::model::DescriptorType::StreamPortInput)
							{
								streamPortNode = &audioUnitNode.streamPortInputs[streamPortIndex];
								streamPortStaticModel = &entity->getNodeStaticModel(configIndex, streamPortIndex, &model::ConfigurationStaticTree::streamPortInputStaticModels);
								streamPortDynamicModel = &entity->getNodeDynamicModel(configIndex, streamPortIndex, &model::ConfigurationDynamicTree::streamPortInputDynamicModels);
							}
							else
							{
								streamPortNode = &audioUnitNode.streamPortOutputs[streamPortIndex];
								streamPortStaticModel = &entity->getNodeStaticModel(configIndex, streamPortIndex, &model::ConfigurationStaticTree::streamPortOutputStaticModels);
								streamPortDynamicModel = &entity->getNodeDynamicModel(configIndex, streamPortIndex, &model::ConfigurationDynamicTree::streamPortOutputDynamicModels);
							}

							initNode(*streamPortNode, descriptorType, streamPortIndex, model::AcquireState::Undefined);
							streamPortNode->staticModel = streamPortStaticModel;
							streamPortNode->dynamicModel = streamPortDynamicModel;

							// Build audio clusters (AudioClusterNode)
							for (auto clusterIndexCounter = entity::model::ClusterIndex(0); clusterIndexCounter < streamPortStaticModel->numberOfClusters; ++clusterIndexCounter)
							{
								auto const clusterIndex = entity::model::ClusterIndex(clusterIndexCounter + streamPortStaticModel->baseCluster);
								auto& audioClusterNode = streamPortNode->audioClusters[clusterIndex];
								initNode(audioClusterNode, entity::model::DescriptorType::AudioCluster, clusterIndex, model::AcquireState::Undefined);

								auto const& audioClusterStaticModel = entity->getNodeStaticModel(configIndex, clusterIndex, &model::ConfigurationStaticTree::audioClusterStaticModels);
								auto& audioClusterDynamicModel = entity->getNodeDynamicModel(configIndex, clusterIndex, &model::ConfigurationDynamicTree::audioClusterDynamicModels);
								audioClusterNode.staticModel = &audioClusterStaticModel;
								audioClusterNode.dynamicModel = &audioClusterDynamicModel;
							}

							// Build audio maps (AudioMapNode)
							for (auto mapIndexCounter = entity::model::MapIndex(0); mapIndexCounter < streamPortStaticModel->numberOfMaps; ++mapIndexCounter)
							{
								auto const mapIndex = entity::model::MapIndex(mapIndexCounter + streamPortStaticModel->baseMap);
								auto& audioMapNode = streamPortNode->audioMaps[mapIndex];
								initNode(audioMapNode, entity::model::DescriptorType::AudioMap, mapIndex, model::AcquireState::Undefined);

								auto const& audioMapStaticModel = entity->getNodeStaticModel(configIndex, mapIndex, &model::ConfigurationStaticTree::audioMapStaticModels);
								audioMapNode.staticModel = &audioMapStaticModel;
							}
						}
					};
					processStreamPorts(entity::model::DescriptorType::StreamPortInput, audioUnitStaticModel.numberOfStreamInputPorts, audioUnitStaticModel.baseStreamInputPort);
					processStreamPorts(entity::model::DescriptorType::StreamPortOutput, audioUnitStaticModel.numberOfStreamOutputPorts, audioUnitStaticModel.baseStreamOutputPort);
				}

				// Build stream inputs (StreamNode)
				for (auto& streamKV : configStaticTree.streamInputStaticModels)
				{
					auto const streamIndex = streamKV.first;
					auto const& streamStaticModel = streamKV.second;
					auto& streamDynamicModel = configDynamicTree.streamInputDynamicModels[streamIndex];

					auto& streamNode = configNode.streamInputs[streamIndex];
					initNode(streamNode, entity::model::DescriptorType::StreamInput, streamIndex, model::AcquireState::Undefined);
					streamNode.staticModel = &streamStaticModel;
					streamNode.dynamicModel = &streamDynamicModel;
				}

				// Build stream outputs (StreamNode)
				for (auto& streamKV : configStaticTree.streamOutputStaticModels)
				{
					auto const streamIndex = streamKV.first;
					auto const& streamStaticModel = streamKV.second;
					auto& streamDynamicModel = configDynamicTree.streamOutputDynamicModels[streamIndex];

					auto& streamNode = configNode.streamOutputs[streamIndex];
					initNode(streamNode, entity::model::DescriptorType::StreamOutput, streamIndex, model::AcquireState::Undefined);
					streamNode.staticModel = &streamStaticModel;
					streamNode.dynamicModel = &streamDynamicModel;
				}

				// Build avb interfaces (AvbInterfaceNode)
				for (auto& interfaceKV : configStaticTree.avbInterfaceStaticModels)
				{
					auto const interfaceIndex = interfaceKV.first;
					auto const& interfaceStaticModel = interfaceKV.second;
					auto& interfaceDynamicModel = configDynamicTree.avbInterfaceDynamicModels[interfaceIndex];

					auto& interfaceNode = configNode.avbInterfaces[interfaceIndex];
					initNode(interfaceNode, entity::model::DescriptorType::AvbInterface, interfaceIndex, model::AcquireState::Undefined);
					interfaceNode.staticModel = &interfaceStaticModel;
					interfaceNode.dynamicModel = &interfaceDynamicModel;
				}

				// Build clock sources (ClockSourceNode)
				for (auto& sourceKV : configStaticTree.clockSourceStaticModels)
				{
					auto const sourceIndex = sourceKV.first;
					auto const& sourceStaticModel = sourceKV.second;
					auto& sourceDynamicModel = configDynamicTree.clockSourceDynamicModels[sourceIndex];

					auto& sourceNode = configNode.clockSources[sourceIndex];
					initNode(sourceNode, entity::model::DescriptorType::ClockSource, sourceIndex, model::AcquireState::Undefined);
					sourceNode.staticModel = &sourceStaticModel;
					sourceNode.dynamicModel = &sourceDynamicModel;
				}

				// Build memory objects (MemoryObjectNode)
				for (auto& memoryObjectKV : configStaticTree.memoryObjectStaticModels)
				{
					auto const memoryObjectIndex = memoryObjectKV.first;
					auto const& memoryObjectStaticModel = memoryObjectKV.second;
					auto& memoryObjectDynamicModel = configDynamicTree.memoryObjectDynamicModels[memoryObjectIndex];

					auto& memoryObjectNode = configNode.memoryObjects[memoryObjectIndex];
					initNode(memoryObjectNode, entity::model::DescriptorType::MemoryObject, memoryObjectIndex, model::AcquireState::Undefined);
					memoryObjectNode.staticModel = &memoryObjectStaticModel;
					memoryObjectNode.dynamicModel = &memoryObjectDynamicModel;
				}

				// Build locales (LocaleNode)
				for (auto const& localeKV : configStaticTree.localeStaticModels)
				{
					auto const localeIndex = localeKV.first;
					auto const& localeStaticModel = localeKV.second;

					auto& localeNode = configNode.locales[localeIndex];
					initNode(localeNode, entity::model::DescriptorType::Locale, localeIndex, model::AcquireState::Undefined);
					localeNode.staticModel = &localeStaticModel;

					// Build strings (StringsNode)
					// TODO
				}

				// Build clock domains (ClockDomainNode)
				for (auto& domainKV : configStaticTree.clockDomainStaticModels)
				{
					auto const domainIndex = domainKV.first;
					auto const& domainStaticModel = domainKV.second;
					auto& domainDynamicModel = configDynamicTree.clockDomainDynamicModels[domainIndex];

					auto& domainNode = configNode.clockDomains[domainIndex];
					initNode(domainNode, entity::model::DescriptorType::ClockDomain, domainIndex, model::AcquireState::Undefined);
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
class RedundantHelper : ControlledEntityImpl
{
public:
	template<typename StreamNodeType>
	static void buildRedundancyNodesByType(la::avdecc::UniqueIdentifier entityID, std::map<entity::model::StreamIndex, StreamNodeType>& streams, std::map<model::VirtualIndex, model::RedundantStreamNode>& redundantStreams)
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

					// Defined the primary stream
					redundantStreamNode.primaryStream = redundantStreamNodes.begin()->second;
				}
			}
		}
	}
};

void ControlledEntityImpl::buildRedundancyNodes(model::ConfigurationNode& configNode) const noexcept
{
	RedundantHelper::buildRedundancyNodesByType(_entity.getEntityID(), configNode.streamInputs, configNode.redundantStreamInputs);
	RedundantHelper::buildRedundancyNodesByType(_entity.getEntityID(), configNode.streamOutputs, configNode.redundantStreamOutputs);
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

} // namespace controller
} // namespace avdecc
} // namespace la
