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
* @file avdeccControlledEntityImpl.cpp
* @author Christophe Calmejane
*/

#include "avdeccControlledEntityImpl.hpp"
#include "la/avdecc/logger.hpp"
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
bool ControlledEntityImpl::gotEnumerationError() const noexcept
{
	return _enumerateError;
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
	if (gotEnumerationError())
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

	if (entityNode.entityDescriptor == nullptr)
		throw Exception(Exception::Type::Internal, "EntityDescriptor not set");

	auto const it = entityNode.configurations.find(entityNode.entityDescriptor->currentConfiguration);
	if (it == entityNode.configurations.end())
		throw Exception(Exception::Type::Internal, "ConfigurationDescriptor for current_configuration not set");

	return it->second;
}

model::StreamNode const& ControlledEntityImpl::getStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& configNode = getConfigurationNode(configurationIndex);

	auto const it = configNode.streamInputs.find(streamIndex);
	if (it == configNode.streamInputs.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream index");

	return it->second;
}

model::StreamNode const& ControlledEntityImpl::getStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& configNode = getConfigurationNode(configurationIndex);

	auto const it = configNode.streamOutputs.find(streamIndex);
	if (it == configNode.streamOutputs.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream index");

	return it->second;
}

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
#if 1
	static model::AudioClusterNode s{};
	(void)configurationIndex;
	(void)audioUnitIndex;
	(void)streamPortIndex;
	(void)clusterIndex;
	return s;
#else
#endif
}

model::AudioMapNode const& ControlledEntityImpl::getAudioMapNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex) const
{
#if 1
	static model::AudioMapNode s{};
	(void)configurationIndex;
	(void)audioUnitIndex;
	(void)streamPortIndex;
	(void)mapIndex;
	return s;
#else
#endif
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

entity::model::LocaleDescriptor const* ControlledEntityImpl::findLocaleDescriptor(entity::model::ConfigurationIndex const configurationIndex, std::string const& /*locale*/) const
{
	auto const& configStaticModel = getConfigurationStaticModel(configurationIndex);

	if (configStaticModel.localeDescriptors.empty())
		throw Exception(Exception::Type::InvalidLocaleName, "Entity has no locale");

#pragma message("TODO: Parse 'locale' parameter and find best match")
	// Right now, return the first locale
	return &configStaticModel.localeDescriptors.at(0);
}

entity::model::AvdeccFixedString const& ControlledEntityImpl::getLocalizedString(entity::model::LocalizedStringReference const stringReference) const noexcept
{
	auto const& entityDescriptor = getEntityStaticModel().entityDescriptor;
	return getLocalizedString(entityDescriptor.currentConfiguration, stringReference);
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

		auto const& configStaticModel = getConfigurationStaticModel(configurationIndex);

		return configStaticModel.localizedStrings.at(entity::model::StringsIndex(globalOffset));
	}
	catch (...)
	{
		return s_noLocalizationString;
	}
}

entity::model::StreamConnectedState const& ControlledEntityImpl::getConnectedSinkState(entity::model::StreamIndex const listenerIndex) const
{
	auto const& entityDescriptor = getEntityStaticModel().entityDescriptor;
	auto const& streamDynamicModel = getStreamInputDynamicModel(entityDescriptor.currentConfiguration, listenerIndex);

	return streamDynamicModel.connectedState;
}

entity::model::AudioMappings const& ControlledEntityImpl::getStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const& entityDescriptor = getEntityStaticModel().entityDescriptor;

	// Check if dynamic mappings is supported by the entity
	auto const& streamPortNode = getStreamPortInputNode(entityDescriptor.currentConfiguration, streamPortIndex);
	if (!streamPortNode.hasDynamicAudioMap)
		throw Exception(Exception::Type::NotSupported, "Dynamic mappings not supported by this stream port");

	// Get dynamic mappings for this stream port
	auto const& streamPortDynamicModel = getStreamPortInputDynamicModel(entityDescriptor.currentConfiguration, streamPortIndex);
	return streamPortDynamicModel.dynamicAudioMap;
}

entity::model::AudioMappings const& ControlledEntityImpl::getStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const& entityDescriptor = getEntityStaticModel().entityDescriptor;

	// Check if dynamic mappings is supported by the entity
	auto const& streamPortNode = getStreamPortOutputNode(entityDescriptor.currentConfiguration, streamPortIndex);
	if (!streamPortNode.hasDynamicAudioMap)
		throw Exception(Exception::Type::NotSupported, "Dynamic mappings not supported by this stream port");

	// Get dynamic mappings for this stream port
	auto const& streamPortDynamicModel = getStreamPortOutputDynamicModel(entityDescriptor.currentConfiguration, streamPortIndex);
	return streamPortDynamicModel.dynamicAudioMap;
}

// Visitor method
void ControlledEntityImpl::accept(model::EntityModelVisitor* const visitor) const noexcept
{
	if (_enumerateError)
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

			// If this is the active configuration, process ConfigurationNode fields
			if (configuration.isActiveConfiguration)
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

				// Loop over StreamNode for inputs
				for (auto const& streamKV : configuration.streamInputs)
				{
					auto const& stream = streamKV.second;
					// Visit StreamNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, stream);
				}

				// Loop over StreamNode for outputs
				for (auto const& streamKV : configuration.streamOutputs)
				{
					auto const& stream = streamKV.second;
					// Visit StreamNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, stream);
				}

				// Loop over RedundantStreamNode for inputs
				for (auto const& redundantStreamKV : configuration.redundantStreamInputs)
				{
					auto const& redundantStream = redundantStreamKV.second;
					// Visit RedundantStreamNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, redundantStream);

					// Loop over StreamNode
					for (auto const& streamKV : redundantStream.redundantStreams)
					{
						auto const* stream = streamKV.second;
						// Visit StreamNode (RedundantStreamNode is parent)
						visitor->visit(this, &redundantStream, *stream);
					}
				}

				// Loop over RedundantStreamNode for outputs
				for (auto const& redundantStreamKV : configuration.redundantStreamOutputs)
				{
					auto const& redundantStream = redundantStreamKV.second;
					// Visit RedundantStreamNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, redundantStream);

					// Loop over StreamNode
					for (auto const& streamKV : redundantStream.redundantStreams)
					{
						auto const* stream = streamKV.second;
						// Visit StreamNode (RedundantStreamNode is parent)
						visitor->visit(this, &redundantStream, *stream);
					}
				}

				// Loop over AvbInterfaceNode
				for (auto const& interfaceKV : configuration.avbInterfaces)
				{
					auto const& interface = interfaceKV.second;
					// Visit AvbInterfaceNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, interface);
				}

				// Loop over ClockSourceNode
				for (auto const& sourceKV : configuration.clockSources)
				{
					auto const& source = sourceKV.second;
					// Visit ClockSourceNode (ConfigurationNode is parent)
					visitor->visit(this, &configuration, source);
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
#pragma message("TODO: Add a lock to the class and use it the Lockable concept for all modifications from the controller!!")
}

void ControlledEntityImpl::unlock() noexcept
{
#pragma message("TODO: Add a lock to the class and use it the Lockable concept for all modifications from the controller!!")
}

// Const getters
model::EntityStaticModel const& ControlledEntityImpl::getEntityStaticModel() const
{
	if (gotEnumerationError())
		throw Exception(Exception::Type::EnumerationError, "Entity had an enumeration error");

	if (!hasFlag(_entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		throw Exception(Exception::Type::NotSupported, "EM not supported by the entity");

	return _entityStaticModel;
}

model::ConfigurationStaticModel const& ControlledEntityImpl::getConfigurationStaticModel(entity::model::ConfigurationIndex const configurationIndex) const
{
	if (!hasFlag(_entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		throw Exception(Exception::Type::NotSupported, "EM not supported by the entity");

	auto const& entityStaticModel = getEntityStaticModel();
	auto const it = entityStaticModel.configurationStaticModels.find(configurationIndex);
	if (it == entityStaticModel.configurationStaticModels.end())
		throw Exception(Exception::Type::InvalidConfigurationIndex, "Invalid configuration index");

	return it->second;
}

model::EntityDynamicModel const& ControlledEntityImpl::getEntityDynamicModel() const
{
	if (gotEnumerationError())
		throw Exception(Exception::Type::EnumerationError, "Entity had an enumeration error");

	if (!hasFlag(_entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		throw Exception(Exception::Type::NotSupported, "EM not supported by the entity");

	return _entityDynamicModel;
}

model::ConfigurationDynamicModel const& ControlledEntityImpl::getConfigurationDynamicModel(entity::model::ConfigurationIndex const configurationIndex) const
{
	if (!hasFlag(_entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		throw Exception(Exception::Type::NotSupported, "EM not supported by the entity");

	auto const& entityDynamicModel = getEntityDynamicModel();
	auto const it = entityDynamicModel.configurationDynamicModels.find(configurationIndex);
	if (it == entityDynamicModel.configurationDynamicModels.end())
		throw Exception(Exception::Type::InvalidConfigurationIndex, "Invalid configuration index");

	return it->second;
}

model::StreamDynamicModel const& ControlledEntityImpl::getStreamInputDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& configDynamicModel = getConfigurationDynamicModel(configurationIndex);

	auto const it = configDynamicModel.streamInputDynamicModels.find(streamIndex);
	if (it == configDynamicModel.streamInputDynamicModels.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream index");

	return it->second;
}

model::StreamDynamicModel const& ControlledEntityImpl::getStreamOutputDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& configDynamicModel = getConfigurationDynamicModel(configurationIndex);

	auto const it = configDynamicModel.streamOutputDynamicModels.find(streamIndex);
	if (it == configDynamicModel.streamOutputDynamicModels.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream index");

	return it->second;
}

model::StreamPortDynamicModel const& ControlledEntityImpl::getStreamPortInputDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const& configDynamicModel = getConfigurationDynamicModel(configurationIndex);

	auto const it = configDynamicModel.streamPortInputDynamicModels.find(streamPortIndex);
	if (it == configDynamicModel.streamPortInputDynamicModels.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream port index");

	return it->second;
}

model::StreamPortDynamicModel const& ControlledEntityImpl::getStreamPortOutputDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const& configDynamicModel = getConfigurationDynamicModel(configurationIndex);

	auto const it = configDynamicModel.streamPortOutputDynamicModels.find(streamPortIndex);
	if (it == configDynamicModel.streamPortOutputDynamicModels.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream port index");

	return it->second;
}

entity::model::AudioUnitDescriptor const& ControlledEntityImpl::getAudioUnitDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex) const
{
	auto const& configStaticModel = getConfigurationStaticModel(configurationIndex);

	auto const it = configStaticModel.audioUnitDescriptors.find(audioUnitIndex);
	if (it == configStaticModel.audioUnitDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid audio unit index");

	return it->second;
}

entity::model::StreamDescriptor const& ControlledEntityImpl::getStreamInputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& configStaticModel = getConfigurationStaticModel(configurationIndex);

	auto const it = configStaticModel.streamInputDescriptors.find(streamIndex);
	if (it == configStaticModel.streamInputDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream index");

	return it->second;
}

entity::model::StreamDescriptor const& ControlledEntityImpl::getStreamOutputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& configStaticModel = getConfigurationStaticModel(configurationIndex);

	auto const it = configStaticModel.streamOutputDescriptors.find(streamIndex);
	if (it == configStaticModel.streamOutputDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream index");

	return it->second;
}

entity::model::StreamPortDescriptor const& ControlledEntityImpl::getStreamPortInputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const& configStaticModel = getConfigurationStaticModel(configurationIndex);

	auto const it = configStaticModel.streamPortInputDescriptors.find(streamPortIndex);
	if (it == configStaticModel.streamPortInputDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream port index");

	return it->second;
}

entity::model::StreamPortDescriptor const& ControlledEntityImpl::getStreamPortOutputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const& configStaticModel = getConfigurationStaticModel(configurationIndex);

	auto const it = configStaticModel.streamPortOutputDescriptors.find(streamPortIndex);
	if (it == configStaticModel.streamPortOutputDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream port index");

	return it->second;
}

entity::model::AudioClusterDescriptor const& ControlledEntityImpl::getAudioClusterDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex) const
{
	auto const& configStaticModel = getConfigurationStaticModel(configurationIndex);

	auto const it = configStaticModel.audioClusterDescriptors.find(clusterIndex);
	if (it == configStaticModel.audioClusterDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid cluster index");

	return it->second;
}

entity::model::AudioMapDescriptor const& ControlledEntityImpl::getAudioMapDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex) const
{
	auto const& configStaticModel = getConfigurationStaticModel(configurationIndex);

	auto const it = configStaticModel.audioMapDescriptors.find(mapIndex);
	if (it == configStaticModel.audioMapDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid map index");

	return it->second;
}

entity::model::ClockDomainDescriptor const& ControlledEntityImpl::getClockDomainDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex) const
{
	auto const& configStaticModel = getConfigurationStaticModel(configurationIndex);

	auto const it = configStaticModel.clockDomainDescriptors.find(clockDomainIndex);
	if (it == configStaticModel.clockDomainDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid clock domain index");

	return it->second;
}

// Non-const getters
model::EntityStaticModel& ControlledEntityImpl::getEntityStaticModel()
{
	// Implemented over getEntityStaticModel() const overload
	return const_cast<model::EntityStaticModel&>(static_cast<ControlledEntityImpl const*>(this)->getEntityStaticModel());
}

model::ConfigurationStaticModel& ControlledEntityImpl::getConfigurationStaticModel(entity::model::ConfigurationIndex const configurationIndex)
{
	// Implemented over getConfigurationStaticModel() const overload
	return const_cast<model::ConfigurationStaticModel&>(static_cast<ControlledEntityImpl const*>(this)->getConfigurationStaticModel(configurationIndex));
}

model::EntityDynamicModel& ControlledEntityImpl::getEntityDynamicModel()
{
	// Implemented over getEntityDynamicModel() const overload
	return const_cast<model::EntityDynamicModel&>(static_cast<ControlledEntityImpl const*>(this)->getEntityDynamicModel());
}

model::ConfigurationDynamicModel& ControlledEntityImpl::getConfigurationDynamicModel(entity::model::ConfigurationIndex const configurationIndex)
{
	// Implemented over getConfigurationDynamicModel() const overload
	return const_cast<model::ConfigurationDynamicModel&>(static_cast<ControlledEntityImpl const*>(this)->getConfigurationDynamicModel(configurationIndex));
}

model::StreamDynamicModel& ControlledEntityImpl::getStreamInputDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
{
	// Implemented over getStreamInputDynamicModel() const overload
	return const_cast<model::StreamDynamicModel&>(static_cast<ControlledEntityImpl const*>(this)->getStreamInputDynamicModel(configurationIndex, streamIndex));
}

model::StreamDynamicModel& ControlledEntityImpl::getStreamOutputDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
{
	// Implemented over getStreamOutputDynamicModel() const overload
	return const_cast<model::StreamDynamicModel&>(static_cast<ControlledEntityImpl const*>(this)->getStreamOutputDynamicModel(configurationIndex, streamIndex));
}

model::StreamPortDynamicModel& ControlledEntityImpl::getStreamPortInputDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex)
{
	// Implemented over getStreamPortInputDynamicModel() const overload
	return const_cast<model::StreamPortDynamicModel&>(static_cast<ControlledEntityImpl const*>(this)->getStreamPortInputDynamicModel(configurationIndex, streamPortIndex));
}

model::StreamPortDynamicModel& ControlledEntityImpl::getStreamPortOutputDynamicModel(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex)
{
	// Implemented over getStreamPortOutputDynamicModel() const overload
	return const_cast<model::StreamPortDynamicModel&>(static_cast<ControlledEntityImpl const*>(this)->getStreamPortOutputDynamicModel(configurationIndex, streamPortIndex));
}

entity::model::AudioUnitDescriptor& ControlledEntityImpl::getAudioUnitDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex)
{
	// Implemented over getAudioUnitDescriptor() const overload
	return const_cast<entity::model::AudioUnitDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getAudioUnitDescriptor(configurationIndex, audioUnitIndex));
}

entity::model::StreamDescriptor& ControlledEntityImpl::getStreamInputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
{
	// Implemented over getStreamInputDescriptor() const overload
	return const_cast<entity::model::StreamDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getStreamInputDescriptor(configurationIndex, streamIndex));
}

entity::model::StreamDescriptor& ControlledEntityImpl::getStreamOutputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
{
	// Implemented over getStreamOutputDescriptor() const overload
	return const_cast<entity::model::StreamDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getStreamOutputDescriptor(configurationIndex, streamIndex));
}

entity::model::StreamPortDescriptor& ControlledEntityImpl::getStreamPortInputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex)
{
	// Implemented over getStreamPortInputDescriptor() const overload
	return const_cast<entity::model::StreamPortDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getStreamPortInputDescriptor(configurationIndex, streamPortIndex));
}

entity::model::StreamPortDescriptor& ControlledEntityImpl::getStreamPortOutputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex)
{
	// Implemented over getStreamPortOutputDescriptor() const overload
	return const_cast<entity::model::StreamPortDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getStreamPortOutputDescriptor(configurationIndex, streamPortIndex));
}

entity::model::AudioClusterDescriptor& ControlledEntityImpl::getAudioClusterDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex)
{
	// Implemented over getAudioClusterDescriptor() const overload
	return const_cast<entity::model::AudioClusterDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getAudioClusterDescriptor(configurationIndex, clusterIndex));
}

entity::model::AudioMapDescriptor& ControlledEntityImpl::getAudioMapDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex)
{
	// Implemented over getAudioMapDescriptor() const overload
	return const_cast<entity::model::AudioMapDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getAudioMapDescriptor(configurationIndex, mapIndex));
}

entity::model::ClockDomainDescriptor& ControlledEntityImpl::getClockDomainDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex)
{
	// Implemented over getClockDomainDescriptor() const overload
	return const_cast<entity::model::ClockDomainDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getClockDomainDescriptor(configurationIndex, clockDomainIndex));
}

// Setters (of the model, not the physical entity)
void ControlledEntityImpl::updateEntity(entity::Entity const& entity) noexcept
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

void ControlledEntityImpl::setEntityDescriptor(entity::model::EntityDescriptor const& descriptor)
{
	// Wipe previous model::EntityStaticModel, we can have only one EntityDescriptor
	_entityStaticModel = {};

	// Initialize model::EntityStaticModel
	_entityStaticModel.entityDescriptor = descriptor;

	// Wipe previous model::EntityDynamicModel
	_entityDynamicModel = {};

	// Wipe model::EntityNode
	_entityNode = {};
}

void ControlledEntityImpl::setConfigurationDescriptor(entity::model::ConfigurationDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex)
{
	// Create a new model::ConfigurationStaticModel for this entity
	auto& entityStaticModel = getEntityStaticModel();
	auto& configStaticModel = entityStaticModel.configurationStaticModels[configurationIndex];

	// Initialize model::ConfigurationStaticModel
	configStaticModel.configurationDescriptor = descriptor;

	// Create a new model::ConfigurationDynamicModel for this entity
	auto& entityDynamicModel = getEntityDynamicModel();
	auto& configDynamicModel = entityDynamicModel.configurationDynamicModels[configurationIndex];

	// Pre-allocate all stream input/output dynamic models so the getStreamInputDynamicModel() const method can access them without having an exception
	{
		auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::StreamInput);
		if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
		{
			auto count = countIt->second;
			for (auto index = entity::model::StreamIndex(0); index < count; ++index)
			{
				configDynamicModel.streamInputDynamicModels[index];
			}
		}
	}
	{
		auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::StreamOutput);
		if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
		{
			auto count = countIt->second;
			for (auto index = entity::model::StreamIndex(0); index < count; ++index)
			{
				configDynamicModel.streamOutputDynamicModels[index];
			}
		}
	}
}

void ControlledEntityImpl::setAudioUnitDescriptor(entity::model::AudioUnitDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex)
{
	auto& configStaticModel = getConfigurationStaticModel(configurationIndex);

	// Insert or replace the descriptor into the configuration
#if __cpp_lib_map_try_emplace
	configStaticModel.audioUnitDescriptors.insert_or_assign(std::make_pair(audioUnitIndex, descriptor));
#else
	configStaticModel.audioUnitDescriptors.erase(audioUnitIndex);
	configStaticModel.audioUnitDescriptors.insert(std::make_pair(audioUnitIndex, descriptor));
#endif
}

void ControlledEntityImpl::setStreamInputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
{
	auto& configStaticModel = getConfigurationStaticModel(configurationIndex);

	// Insert or replace the descriptor into the configuration
#if __cpp_lib_map_try_emplace
	configStaticModel.streamInputDescriptors.insert_or_assign(std::make_pair(streamIndex, descriptor));
#else
	configStaticModel.streamInputDescriptors.erase(streamIndex);
	configStaticModel.streamInputDescriptors.insert(std::make_pair(streamIndex, descriptor));
#endif
}

void ControlledEntityImpl::setStreamOutputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
{
	auto& configStaticModel = getConfigurationStaticModel(configurationIndex);

	// Insert or replace the descriptor into the configuration
#if __cpp_lib_map_try_emplace
	configStaticModel.streamOutputDescriptors.insert_or_assign(std::make_pair(streamIndex, descriptor));
#else
	configStaticModel.streamOutputDescriptors.erase(streamIndex);
	configStaticModel.streamOutputDescriptors.insert(std::make_pair(streamIndex, descriptor));
#endif
}

void ControlledEntityImpl::setAvbInterfaceDescriptor(entity::model::AvbInterfaceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const interfaceIndex)
{
	auto& configStaticModel = getConfigurationStaticModel(configurationIndex);

	// Insert or replace the descriptor into the configuration
#if __cpp_lib_map_try_emplace
	configStaticModel.avbInterfaceDescriptors.insert_or_assign(std::make_pair(interfaceIndex, descriptor));
#else
	configStaticModel.avbInterfaceDescriptors.erase(interfaceIndex);
	configStaticModel.avbInterfaceDescriptors.insert(std::make_pair(interfaceIndex, descriptor));
#endif
}

void ControlledEntityImpl::setClockSourceDescriptor(entity::model::ClockSourceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockIndex)
{
	auto& configStaticModel = getConfigurationStaticModel(configurationIndex);

	// Insert or replace the descriptor into the configuration
#if __cpp_lib_map_try_emplace
	configStaticModel.clockSourceDescriptors.insert_or_assign(std::make_pair(clockIndex, descriptor));
#else
	configStaticModel.clockSourceDescriptors.erase(clockIndex);
	configStaticModel.clockSourceDescriptors.insert(std::make_pair(clockIndex, descriptor));
#endif
}

void ControlledEntityImpl::setSelectedLocaleDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleDescriptor const* const descriptor)
{
	auto& configStaticModel = getConfigurationStaticModel(configurationIndex);
	configStaticModel.selectedLocaleDescriptor = descriptor;
}

void ControlledEntityImpl::setLocaleDescriptor(entity::model::LocaleDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex)
{
	auto& configStaticModel = getConfigurationStaticModel(configurationIndex);

	// Insert or replace the descriptor into the configuration
#if __cpp_lib_map_try_emplace
	configStaticModel.localeDescriptors.insert_or_assign(std::make_pair(localeIndex, descriptor));
#else
	configStaticModel.localeDescriptors.erase(localeIndex);
	configStaticModel.localeDescriptors.insert(std::make_pair(localeIndex, descriptor));
#endif
}

void ControlledEntityImpl::addStringsDescriptor(entity::model::StringsDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex, entity::model::StringsIndex const baseStringDescriptorIndex)
{
	auto& configStaticModel = getConfigurationStaticModel(configurationIndex);

	// Check if we have a selected locale descriptor (and correct one), we might be receiving strings after a reset of the entity (it has gone offline then online again)
	if (configStaticModel.selectedLocaleDescriptor == nullptr || configStaticModel.selectedLocaleDescriptor->baseStringDescriptorIndex != baseStringDescriptorIndex)
		return;

	// Add received localizedStrings
	for (auto strIndex = 0u; strIndex < descriptor.strings.size(); ++strIndex)
	{
		auto localizedStringIndex = entity::model::StringsIndex((stringsIndex - baseStringDescriptorIndex) * descriptor.strings.size() + strIndex);
		configStaticModel.localizedStrings[localizedStringIndex] = descriptor.strings.at(strIndex);
	}
}

void ControlledEntityImpl::setStreamPortInputDescriptor(entity::model::StreamPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex)
{
	auto& configStaticModel = getConfigurationStaticModel(configurationIndex);

	// Insert or replace the descriptor into the configuration
#if __cpp_lib_map_try_emplace
	configStaticModel.streamPortInputDescriptors.insert_or_assign(std::make_pair(streamPortIndex, descriptor));
#else
	configStaticModel.streamPortInputDescriptors.erase(streamPortIndex);
	configStaticModel.streamPortInputDescriptors.insert(std::make_pair(streamPortIndex, descriptor));
#endif
}

void ControlledEntityImpl::setStreamPortOutputDescriptor(entity::model::StreamPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex)
{
	auto& configStaticModel = getConfigurationStaticModel(configurationIndex);

	// Insert or replace the descriptor into the configuration
#if __cpp_lib_map_try_emplace
	configStaticModel.streamPortOutputDescriptors.insert_or_assign(std::make_pair(streamPortIndex, descriptor));
#else
	configStaticModel.streamPortOutputDescriptors.erase(streamPortIndex);
	configStaticModel.streamPortOutputDescriptors.insert(std::make_pair(streamPortIndex, descriptor));
#endif
}

void ControlledEntityImpl::setAudioClusterDescriptor(entity::model::AudioClusterDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex)
{
	auto& configStaticModel = getConfigurationStaticModel(configurationIndex);

	// Insert or replace the descriptor into the configuration
#if __cpp_lib_map_try_emplace
	configStaticModel.audioClusterDescriptors.insert_or_assign(std::make_pair(clusterIndex, descriptor));
#else
	configStaticModel.audioClusterDescriptors.erase(clusterIndex);
	configStaticModel.audioClusterDescriptors.insert(std::make_pair(clusterIndex, descriptor));
#endif
}

void ControlledEntityImpl::setAudioMapDescriptor(entity::model::AudioMapDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex)
{
	auto& configStaticModel = getConfigurationStaticModel(configurationIndex);

	// Insert or replace the descriptor into the configuration
#if __cpp_lib_map_try_emplace
	configStaticModel.audioMapDescriptors.insert_or_assign(std::make_pair(mapIndex, descriptor));
#else
	configStaticModel.audioMapDescriptors.erase(mapIndex);
	configStaticModel.audioMapDescriptors.insert(std::make_pair(mapIndex, descriptor));
#endif
}

void ControlledEntityImpl::setClockDomainDescriptor(entity::model::ClockDomainDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex)
{
	auto& configStaticModel = getConfigurationStaticModel(configurationIndex);

	// Insert or replace the descriptor into the configuration
#if __cpp_lib_map_try_emplace
	configStaticModel.clockDomainDescriptors.insert_or_assign(std::make_pair(clockIndex, descriptor));
#else
	configStaticModel.clockDomainDescriptors.erase(clockDomainIndex);
	configStaticModel.clockDomainDescriptors.insert(std::make_pair(clockDomainIndex, descriptor));
#endif
}

void ControlledEntityImpl::setInputStreamState(entity::model::StreamConnectedState const& state, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
{
	auto& configDynamicModel = getConfigurationDynamicModel(configurationIndex);

	// Get or create model::StreamDynamicModel
	auto& streamDynamicModel = configDynamicModel.streamInputDynamicModels[streamIndex];

	// Set StreamConnectedState
	streamDynamicModel.connectedState = state;
}

void ControlledEntityImpl::clearPortInputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex)
{
	auto& configDynamicModel = getConfigurationDynamicModel(configurationIndex);

	// Get or create model::StreamPortDynamicModel
	auto& streamPortDynamicModel = configDynamicModel.streamPortInputDynamicModels[streamPortIndex];

	// Clear audio mappings
	streamPortDynamicModel.dynamicAudioMap.clear();
}

void ControlledEntityImpl::clearPortOutputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex)
{
	auto& configDynamicModel = getConfigurationDynamicModel(configurationIndex);

	// Get or create model::StreamPortDynamicModel
	auto& streamPortDynamicModel = configDynamicModel.streamPortOutputDynamicModels[streamPortIndex];

	// Clear audio mappings
	streamPortDynamicModel.dynamicAudioMap.clear();
}

void ControlledEntityImpl::addPortInputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
{
	auto& configDynamicModel = getConfigurationDynamicModel(configurationIndex);

	// Get or create model::StreamPortDynamicModel
	auto& streamPortDynamicModel = configDynamicModel.streamPortInputDynamicModels[streamPortIndex];

	// Process audio mappings
	for (auto const& map : mappings)
	{
		// Check if mapping must be replaced
		auto foundIt = std::find_if(streamPortDynamicModel.dynamicAudioMap.begin(), streamPortDynamicModel.dynamicAudioMap.end(), [&map](entity::model::AudioMapping const& mapping)
		{
			return (map.clusterOffset == mapping.clusterOffset) && (map.clusterChannel == mapping.clusterChannel);
		});
		// Not found, add the new mapping
		if (foundIt == streamPortDynamicModel.dynamicAudioMap.end())
			streamPortDynamicModel.dynamicAudioMap.push_back(map);
		else // Otherwise, replace the previous mapping
		{
			foundIt->streamIndex = map.streamIndex;
			foundIt->streamChannel = map.streamChannel;
		}
	}
}

void ControlledEntityImpl::addPortOutputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
{
	auto& configDynamicModel = getConfigurationDynamicModel(configurationIndex);

	// Get or create model::StreamPortDynamicModel
	auto& streamPortDynamicModel = configDynamicModel.streamPortOutputDynamicModels[streamPortIndex];

	// Process audio mappings
	for (auto const& map : mappings)
	{
		// Check if mapping must be replaced
		auto foundIt = std::find_if(streamPortDynamicModel.dynamicAudioMap.begin(), streamPortDynamicModel.dynamicAudioMap.end(), [&map](entity::model::AudioMapping const& mapping)
		{
			return (map.clusterOffset == mapping.clusterOffset) && (map.clusterChannel == mapping.clusterChannel);
		});
		// Not found, add the new mapping
		if (foundIt == streamPortDynamicModel.dynamicAudioMap.end())
			streamPortDynamicModel.dynamicAudioMap.push_back(map);
		else // Otherwise, replace the previous mapping
		{
			foundIt->streamIndex = map.streamIndex;
			foundIt->streamChannel = map.streamChannel;
		}
	}
}

void ControlledEntityImpl::removePortInputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
{
	auto& configDynamicModel = getConfigurationDynamicModel(configurationIndex);

	// Get or create model::StreamPortDynamicModel
	auto& streamPortDynamicModel = configDynamicModel.streamPortInputDynamicModels[streamPortIndex];

	// Process audio mappings
	for (auto const& map : mappings)
	{
		// Check if mapping exists
		auto foundIt = std::find_if(streamPortDynamicModel.dynamicAudioMap.begin(), streamPortDynamicModel.dynamicAudioMap.end(), [&map](entity::model::AudioMapping const& mapping)
		{
			return (map.clusterOffset == mapping.clusterOffset) && (map.clusterChannel == mapping.clusterChannel);
		});
		if (AVDECC_ASSERT_WITH_RET(foundIt != streamPortDynamicModel.dynamicAudioMap.end(), "Mapping not found"))
			streamPortDynamicModel.dynamicAudioMap.erase(foundIt);
	}
}

void ControlledEntityImpl::removePortOutputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
{
	auto& configDynamicModel = getConfigurationDynamicModel(configurationIndex);

	// Get or create model::StreamPortDynamicModel
	auto& streamPortDynamicModel = configDynamicModel.streamPortOutputDynamicModels[streamPortIndex];

	// Process audio mappings
	for (auto const& map : mappings)
	{
		// Check if mapping exists
		auto foundIt = std::find_if(streamPortDynamicModel.dynamicAudioMap.begin(), streamPortDynamicModel.dynamicAudioMap.end(), [&map](entity::model::AudioMapping const& mapping)
		{
			return (map.clusterOffset == mapping.clusterOffset) && (map.clusterChannel == mapping.clusterChannel);
		});
		if (AVDECC_ASSERT_WITH_RET(foundIt != streamPortDynamicModel.dynamicAudioMap.end(), "Mapping not found"))
			streamPortDynamicModel.dynamicAudioMap.erase(foundIt);
	}
}

// Expected descriptor query methods
static inline ControlledEntityImpl::DescriptorKey makeDescriptorKey(entity::model::DescriptorType descriptorType, entity::model::DescriptorIndex descriptorIndex)
{
	return (la::avdecc::to_integral(descriptorType) << (sizeof(descriptorIndex) * 8)) + descriptorIndex;
}

bool ControlledEntityImpl::checkAndClearExpectedDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	auto const confIt = _expectedDescriptors.find(configurationIndex);

	if (confIt == _expectedDescriptors.end())
		return false;

	auto const key = makeDescriptorKey(descriptorType, descriptorIndex);

	return confIt->second.erase(key) == 1;
}

void ControlledEntityImpl::setDescriptorExpected(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	auto& conf = _expectedDescriptors[configurationIndex];

	auto const key = makeDescriptorKey(descriptorType, descriptorIndex);
	conf.insert(key);
}

bool ControlledEntityImpl::gotAllExpectedDescriptors() const noexcept
{
	for (auto const& confKV : _expectedDescriptors)
	{
		if (confKV.second.size() != 0)
			return false;
	}
	return true;
}

void ControlledEntityImpl::clearExpectedDescriptors(entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	_expectedDescriptors.erase(configurationIndex);
}

// Expected dynamic info query methods
static inline ControlledEntityImpl::DescriptorKey makeDynamicInfoKey(ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex descriptorIndex)
{
	return (la::avdecc::to_integral(dynamicInfoType) << (sizeof(descriptorIndex) * 8)) + descriptorIndex;
}

bool ControlledEntityImpl::checkAndClearExpectedDynamicInfo(entity::model::ConfigurationIndex const configurationIndex, DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	auto const confIt = _expectedDynamicInfo.find(configurationIndex);

	if (confIt == _expectedDynamicInfo.end())
		return false;

	auto const key = makeDynamicInfoKey(dynamicInfoType, descriptorIndex);

	return confIt->second.erase(key) == 1;
}

void ControlledEntityImpl::setDynamicInfoExpected(entity::model::ConfigurationIndex const configurationIndex, DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	auto& conf = _expectedDynamicInfo[configurationIndex];

	auto const key = makeDynamicInfoKey(dynamicInfoType, descriptorIndex);
	conf.insert(key);
}

bool ControlledEntityImpl::gotAllExpectedDynamicInfo() const noexcept
{
	for (auto const& confKV : _expectedDynamicInfo)
	{
		if (confKV.second.size() != 0)
			return false;
	}
	return true;
}

void ControlledEntityImpl::clearExpectedDynamicInfo(entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	_expectedDynamicInfo.erase(configurationIndex);
}

void ControlledEntityImpl::setEnumerationError(bool const gotEnumerationError) noexcept
{
	_enumerateError = gotEnumerationError;
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
#pragma message("TODO: Use a std::optional when available, instead of detecting the non-initialized value from configurations count")
		if (_entityNode.configurations.size() == 0)
		{
			// Build root node (EntityNode)
			initNode(_entityNode, entity::model::DescriptorType::Entity, 0, model::AcquireState::Undefined);
			_entityNode.entityDescriptor = &_entityStaticModel.entityDescriptor;

			// Build configuration nodes (ConfigurationNode)
			for (auto const& configKV : _entityStaticModel.configurationStaticModels)
			{
				auto const configIndex = configKV.first;
				auto const& configStaticModel = configKV.second;

				auto& configNode = _entityNode.configurations[configIndex];
				initNode(configNode, entity::model::DescriptorType::Configuration, configIndex, model::AcquireState::Undefined);
				configNode.configurationDescriptor = &configStaticModel.configurationDescriptor;
				configNode.isActiveConfiguration = configIndex == _entityStaticModel.entityDescriptor.currentConfiguration;

				// Build audio units (AudioUnitNode)
				for (auto const& audioUnitKV : configStaticModel.audioUnitDescriptors)
				{
					auto const audioUnitIndex = audioUnitKV.first;
					auto const& audioUnitDescriptor = audioUnitKV.second;

					auto& audioUnitNode = configNode.audioUnits[audioUnitIndex];
					initNode(audioUnitNode, entity::model::DescriptorType::AudioUnit, audioUnitIndex, model::AcquireState::Undefined);
					audioUnitNode.audioUnitDescriptor = &audioUnitDescriptor;

					// Build stream port inputs and outputs (StreamPortNode)
					auto processStreamPorts = [this, &audioUnitNode, configIndex](entity::model::DescriptorType const descriptorType, std::uint16_t const numberOfStreamPorts, entity::model::StreamPortIndex const baseStreamPort)
					{
						for (auto streamPortIndexCounter = entity::model::StreamPortIndex(0); streamPortIndexCounter < numberOfStreamPorts; ++streamPortIndexCounter)
						{
							model::StreamPortNode* streamPortNode{ nullptr };
							entity::model::StreamPortDescriptor const* streamPortDescriptor{ nullptr };
							auto const streamPortIndex = entity::model::StreamPortIndex(streamPortIndexCounter + baseStreamPort);

							if (descriptorType == entity::model::DescriptorType::StreamPortInput)
							{
								streamPortNode = &audioUnitNode.streamPortInputs[streamPortIndex];
								streamPortDescriptor = &getStreamPortInputDescriptor(configIndex, streamPortIndex);
							}
							else
							{
								streamPortNode = &audioUnitNode.streamPortOutputs[streamPortIndex];
								streamPortDescriptor = &getStreamPortOutputDescriptor(configIndex, streamPortIndex);
							}

							initNode(*streamPortNode, descriptorType, streamPortIndex, model::AcquireState::Undefined);
							streamPortNode->streamPortDescriptor = streamPortDescriptor;
							if (streamPortDescriptor->numberOfMaps == 0)
							{
#if IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS
								try
								{
#endif // IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS
									if (descriptorType == entity::model::DescriptorType::StreamPortInput)
									{
										streamPortNode->dynamicAudioMap = &getStreamPortInputDynamicModel(configIndex, streamPortIndex).dynamicAudioMap;
									}
									else
									{
										streamPortNode->dynamicAudioMap = &getStreamPortOutputDynamicModel(configIndex, streamPortIndex).dynamicAudioMap;
									}
									streamPortNode->hasDynamicAudioMap = true;
#if IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS
								}
								catch (Exception const&)
								{
								}
#endif // IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS
							}

							// Build audio clusters (AudioClusterNode)
							for (auto clusterIndexCounter = entity::model::ClusterIndex(0); clusterIndexCounter < streamPortDescriptor->numberOfClusters; ++clusterIndexCounter)
							{
								auto const clusterIndex = entity::model::ClusterIndex(clusterIndexCounter + streamPortDescriptor->baseCluster);
								auto& audioClusterNode = streamPortNode->audioClusters[clusterIndex];
								initNode(audioClusterNode, entity::model::DescriptorType::AudioCluster, clusterIndex, model::AcquireState::Undefined);
								auto const& audioClusterDescriptor = getAudioClusterDescriptor(configIndex, clusterIndex);
								audioClusterNode.audioClusterDescriptor = &audioClusterDescriptor;
							}

							// Build audio maps (AudioMapNode)
							for (auto mapIndexCounter = entity::model::MapIndex(0); mapIndexCounter < streamPortDescriptor->numberOfMaps; ++mapIndexCounter)
							{
								auto const mapIndex = entity::model::MapIndex(mapIndexCounter + streamPortDescriptor->baseMap);
								auto& audioMapNode = streamPortNode->audioMaps[mapIndex];
								initNode(audioMapNode, entity::model::DescriptorType::AudioMap, mapIndex, model::AcquireState::Undefined);
								auto const& audioMapDescriptor = getAudioMapDescriptor(configIndex, mapIndex);
								audioMapNode.audioMapDescriptor = &audioMapDescriptor;
							}
						}
					};
					processStreamPorts(entity::model::DescriptorType::StreamPortInput, audioUnitDescriptor.numberOfStreamInputPorts, audioUnitDescriptor.baseStreamInputPort);
					processStreamPorts(entity::model::DescriptorType::StreamPortOutput, audioUnitDescriptor.numberOfStreamOutputPorts, audioUnitDescriptor.baseStreamOutputPort);
				}

				// Build stream inputs (StreamNode)
				for (auto const& streamKV : configStaticModel.streamInputDescriptors)
				{
					auto const streamIndex = streamKV.first;
					auto const& streamDescriptor = streamKV.second;

					auto& streamNode = configNode.streamInputs[streamIndex];
					initNode(streamNode, entity::model::DescriptorType::StreamInput, streamIndex, model::AcquireState::Undefined);
					streamNode.streamDescriptor = &streamDescriptor;
					streamNode.connectedState = &getConnectedSinkState(streamIndex);

					// Build redundant input (RedundantStreamNode)
					if (!streamDescriptor.redundantStreams.empty())
					{
						// Search for an already created redundant association with the first stream index in the list (possible primary stream)
						model::RedundantStreamNode* redundantStreamNode{ nullptr };
						auto const firstStreamIndex = *streamDescriptor.redundantStreams.begin();
						for (auto& redundantNode : configNode.redundantStreamInputs)
						{
							auto const it = redundantNode.second.redundantStreams.find(firstStreamIndex);
							if (it != redundantNode.second.redundantStreams.end())
							{
								redundantStreamNode = &redundantNode.second;
								break;
							}
						}
						// Not created yet, do it now
						if (redundantStreamNode == nullptr)
						{
							auto const virtualIndex = static_cast<model::VirtualIndex>(configNode.redundantStreamInputs.size());
							redundantStreamNode = &configNode.redundantStreamInputs[virtualIndex];
							initNode(*redundantStreamNode, entity::model::DescriptorType::StreamInput, virtualIndex);
						}
						redundantStreamNode->redundantStreams[streamIndex] = &streamNode;
						streamNode.isRedundant = true;
					}
				}

				// Build stream outputs (StreamNode)
				for (auto const& streamKV : configStaticModel.streamOutputDescriptors)
				{
					auto const streamIndex = streamKV.first;
					auto const& streamDescriptor = streamKV.second;

					auto& streamNode = configNode.streamOutputs[streamIndex];
					initNode(streamNode, entity::model::DescriptorType::StreamOutput, streamIndex, model::AcquireState::Undefined);
					streamNode.streamDescriptor = &streamDescriptor;

					// Build redundant output (RedundantStreamNode)
					if (!streamDescriptor.redundantStreams.empty())
					{
						// Search for an already created redundant association with the first stream index in the list (possible primary stream)
						model::RedundantStreamNode* redundantStreamNode{ nullptr };
						auto const firstStreamIndex = *streamDescriptor.redundantStreams.begin();
						for (auto& redundantNode : configNode.redundantStreamOutputs)
						{
							auto const it = redundantNode.second.redundantStreams.find(firstStreamIndex);
							if (it != redundantNode.second.redundantStreams.end())
							{
								redundantStreamNode = &redundantNode.second;
								break;
							}
						}
						// Not created yet, do it now
						if (redundantStreamNode == nullptr)
						{
							auto const virtualIndex = static_cast<model::VirtualIndex>(configNode.redundantStreamOutputs.size());
							redundantStreamNode = &configNode.redundantStreamOutputs[virtualIndex];
							initNode(*redundantStreamNode, entity::model::DescriptorType::StreamOutput, virtualIndex);
						}
						redundantStreamNode->redundantStreams[streamIndex] = &streamNode;
						streamNode.isRedundant = true;
					}
				}

				// Build avb interfaces (AvbInterfaceNode)
				for (auto const& interfaceKV : configStaticModel.avbInterfaceDescriptors)
				{
					auto const interfaceIndex = interfaceKV.first;
					auto const& interfaceDescriptor = interfaceKV.second;

					auto& interfaceNode = configNode.avbInterfaces[interfaceIndex];
					initNode(interfaceNode, entity::model::DescriptorType::AvbInterface, interfaceIndex, model::AcquireState::Undefined);
					interfaceNode.avbInterfaceDescriptor = &interfaceDescriptor;
				}

				// Build clock sources (ClockSourceNode)
				for (auto const& sourceKV : configStaticModel.clockSourceDescriptors)
				{
					auto const sourceIndex = sourceKV.first;
					auto const& sourceDescriptor = sourceKV.second;

					auto& sourceNode = configNode.clockSources[sourceIndex];
					initNode(sourceNode, entity::model::DescriptorType::ClockSource, sourceIndex, model::AcquireState::Undefined);
					sourceNode.clockSourceDescriptor = &sourceDescriptor;
				}

				// Build locales (LocaleNode)
				for (auto const& localeKV : configStaticModel.localeDescriptors)
				{
					auto const localeIndex = localeKV.first;
					auto const& localeDescriptor = localeKV.second;

					auto& localeNode = configNode.locales[localeIndex];
					initNode(localeNode, entity::model::DescriptorType::Locale, localeIndex, model::AcquireState::Undefined);
					localeNode.localeDescriptor = &localeDescriptor;

					// Build strings (StringsNode)
					// TODO
				}

				// Build clock domains (ClockDomainNode)
				for (auto const& domainKV : configStaticModel.clockDomainDescriptors)
				{
					auto const domainIndex = domainKV.first;
					auto const& domainDescriptor = domainKV.second;

					auto& domainNode = configNode.clockDomains[domainIndex];
					initNode(domainNode, entity::model::DescriptorType::ClockDomain, domainIndex, model::AcquireState::Undefined);
					domainNode.clockDomainDescriptor = &domainDescriptor;

					// Build associated clock sources (ClockSourceNode)
					for (auto const sourceIndex: domainDescriptor.clockSources)
					{
						auto const sourceIt = configNode.clockSources.find(sourceIndex);
						if (sourceIt != configNode.clockSources.end())
						{
							auto const& sourceNode = sourceIt->second;
							domainNode.clockSources[sourceIndex] = &sourceNode;
						}
					}
				}
			}
		}
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Should never throw");
		_entityNode = {};
	}
}

} // namespace controller
} // namespace avdecc
} // namespace la
