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

bool ControlledEntityImpl::isStreamInputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& descriptor = getStreamInputDescriptor(configurationIndex, streamIndex);
	return descriptor.dynamicModel.isRunning;
}

bool ControlledEntityImpl::isStreamOutputRunning(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& descriptor = getStreamOutputDescriptor(configurationIndex, streamIndex);
	return descriptor.dynamicModel.isRunning;
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

model::LocaleNodeStaticModel const* ControlledEntityImpl::findLocaleNode(entity::model::ConfigurationIndex const configurationIndex, std::string const& /*locale*/) const
{
	auto const& configDescriptor = getConfigurationDescriptor(configurationIndex);

	if (configDescriptor.localeDescriptors.empty())
		throw Exception(Exception::Type::InvalidLocaleName, "Entity has no locale");

#pragma message("TODO: Parse 'locale' parameter and find best match")
	// Right now, return the first locale
	return &configDescriptor.localeDescriptors.at(0).staticModel;
}

entity::model::AvdeccFixedString const& ControlledEntityImpl::getLocalizedString(entity::model::LocalizedStringReference const stringReference) const noexcept
{
	auto const& entityDescriptor = getEntityDescriptor();
	return getLocalizedString(entityDescriptor.dynamicModel.currentConfiguration, stringReference);
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

		auto const& configDescriptor = getConfigurationDescriptor(configurationIndex);

		return configDescriptor.dynamicModel.localizedStrings.at(entity::model::StringsIndex(globalOffset));
	}
	catch (...)
	{
		return s_noLocalizationString;
	}
}

model::StreamConnectionState const& ControlledEntityImpl::getConnectedSinkState(entity::model::StreamIndex const streamIndex) const
{
	auto const& entityDescriptor = getEntityDescriptor();
	auto const& streamDescriptor = getStreamInputDescriptor(entityDescriptor.dynamicModel.currentConfiguration, streamIndex);

	AVDECC_ASSERT(_entity.getEntityID() == streamDescriptor.dynamicModel.connectionState.listenerStream.entityID, "EntityID not correctly initialized");
	AVDECC_ASSERT(streamIndex == streamDescriptor.dynamicModel.connectionState.listenerStream.streamIndex, "StreamIndex not correctly initialized");
	return streamDescriptor.dynamicModel.connectionState;
}

entity::model::AudioMappings const& ControlledEntityImpl::getStreamPortInputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const& entityDescriptor = getEntityDescriptor();

	// Check if dynamic mappings is supported by the entity
	auto const& streamPortDescriptor = getStreamPortInputDescriptor(entityDescriptor.dynamicModel.currentConfiguration, streamPortIndex);
	if (!streamPortDescriptor.staticModel.hasDynamicAudioMap)
		throw Exception(Exception::Type::NotSupported, "Dynamic mappings not supported by this stream port");

	// Return dynamic mappings for this stream port
	return streamPortDescriptor.dynamicModel.dynamicAudioMap;
}

entity::model::AudioMappings const& ControlledEntityImpl::getStreamPortOutputAudioMappings(entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const& entityDescriptor = getEntityDescriptor();

	// Check if dynamic mappings is supported by the entity
	auto const& streamPortDescriptor = getStreamPortOutputDescriptor(entityDescriptor.dynamicModel.currentConfiguration, streamPortIndex);
	if (!streamPortDescriptor.staticModel.hasDynamicAudioMap)
		throw Exception(Exception::Type::NotSupported, "Dynamic mappings not supported by this stream port");

	// Return dynamic mappings for this stream port
	return streamPortDescriptor.dynamicModel.dynamicAudioMap;
}

model::StreamConnections const& ControlledEntityImpl::getStreamOutputConnections(entity::model::StreamIndex const streamIndex) const
{
	auto const& entityDescriptor = getEntityDescriptor();
	auto const& streamDescriptor = getStreamOutputDescriptor(entityDescriptor.dynamicModel.currentConfiguration, streamIndex);

	return streamDescriptor.dynamicModel.connections;
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
model::EntityDescriptor const& ControlledEntityImpl::getEntityDescriptor() const
{
	if (gotEnumerationError())
		throw Exception(Exception::Type::EnumerationError, "Entity had an enumeration error");

	if (!hasFlag(_entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		throw Exception(Exception::Type::NotSupported, "EM not supported by the entity");

	return _entityModel;
}

model::ConfigurationDescriptor const& ControlledEntityImpl::getConfigurationDescriptor(entity::model::ConfigurationIndex const configurationIndex) const
{
	if (!hasFlag(_entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		throw Exception(Exception::Type::NotSupported, "EM not supported by the entity");

	auto const& entityDescriptor = getEntityDescriptor();
	auto const it = entityDescriptor.configurationDescriptors.find(configurationIndex);
	if (it == entityDescriptor.configurationDescriptors.end())
		throw Exception(Exception::Type::InvalidConfigurationIndex, "Invalid configuration index");

	return it->second;
}

model::AudioUnitDescriptor const& ControlledEntityImpl::getAudioUnitDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex) const
{
	auto const& configDescriptor = getConfigurationDescriptor(configurationIndex);

	auto const it = configDescriptor.audioUnitDescriptors.find(audioUnitIndex);
	if (it == configDescriptor.audioUnitDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid audio unit index");

	return it->second;
}

model::StreamInputDescriptor const& ControlledEntityImpl::getStreamInputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& configDescriptor = getConfigurationDescriptor(configurationIndex);

	auto const it = configDescriptor.streamInputDescriptors.find(streamIndex);
	if (it == configDescriptor.streamInputDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream index");

	return it->second;
}

model::StreamOutputDescriptor const& ControlledEntityImpl::getStreamOutputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex) const
{
	auto const& configDescriptor = getConfigurationDescriptor(configurationIndex);

	auto const it = configDescriptor.streamOutputDescriptors.find(streamIndex);
	if (it == configDescriptor.streamOutputDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream index");

	return it->second;
}

model::AvbInterfaceDescriptor const& ControlledEntityImpl::getAvbInterfaceDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex) const
{
	auto const& configDescriptor = getConfigurationDescriptor(configurationIndex);

	auto const it = configDescriptor.avbInterfaceDescriptors.find(avbInterfaceIndex);
	if (it == configDescriptor.avbInterfaceDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid avb interface index");

	return it->second;
}

model::ClockSourceDescriptor const& ControlledEntityImpl::getClockSourceDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex) const
{
	auto const& configDescriptor = getConfigurationDescriptor(configurationIndex);

	auto const it = configDescriptor.clockSourceDescriptors.find(clockSourceIndex);
	if (it == configDescriptor.clockSourceDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid clock source index");

	return it->second;
}

model::LocaleDescriptor const& ControlledEntityImpl::getLocaleDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex) const
{
	auto const& configDescriptor = getConfigurationDescriptor(configurationIndex);

	auto const it = configDescriptor.localeDescriptors.find(localeIndex);
	if (it == configDescriptor.localeDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid locale index");

	return it->second;
}

model::StringsDescriptor const& ControlledEntityImpl::getStringsDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex) const
{
	auto const& configDescriptor = getConfigurationDescriptor(configurationIndex);

	auto const it = configDescriptor.stringsDescriptors.find(stringsIndex);
	if (it == configDescriptor.stringsDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid strings index");

	return it->second;
}

model::StreamPortDescriptor const& ControlledEntityImpl::getStreamPortInputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const& configDescriptor = getConfigurationDescriptor(configurationIndex);

	auto const it = configDescriptor.streamPortInputDescriptors.find(streamPortIndex);
	if (it == configDescriptor.streamPortInputDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream port index");

	return it->second;
}

model::StreamPortDescriptor const& ControlledEntityImpl::getStreamPortOutputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex) const
{
	auto const& configDescriptor = getConfigurationDescriptor(configurationIndex);

	auto const it = configDescriptor.streamPortOutputDescriptors.find(streamPortIndex);
	if (it == configDescriptor.streamPortOutputDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid stream port index");

	return it->second;
}

model::AudioClusterDescriptor const& ControlledEntityImpl::getAudioClusterDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex) const
{
	auto const& configDescriptor = getConfigurationDescriptor(configurationIndex);

	auto const it = configDescriptor.audioClusterDescriptors.find(clusterIndex);
	if (it == configDescriptor.audioClusterDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid audio cluster index");

	return it->second;
}

model::AudioMapDescriptor const& ControlledEntityImpl::getAudioMapDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex) const
{
	auto const& configDescriptor = getConfigurationDescriptor(configurationIndex);

	auto const it = configDescriptor.audioMapDescriptors.find(mapIndex);
	if (it == configDescriptor.audioMapDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid audio map index");

	return it->second;
}

model::ClockDomainDescriptor const& ControlledEntityImpl::getClockDomainDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex) const
{
	auto const& configDescriptor = getConfigurationDescriptor(configurationIndex);

	auto const it = configDescriptor.clockDomainDescriptors.find(clockDomainIndex);
	if (it == configDescriptor.clockDomainDescriptors.end())
		throw Exception(Exception::Type::InvalidDescriptorIndex, "Invalid clock domain index");

	return it->second;
}

// Non-const getters
model::EntityDescriptor& ControlledEntityImpl::getEntityDescriptor()
{
	// Implemented over getEntityDescriptor() const overload
	return const_cast<model::EntityDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getEntityDescriptor());
}

model::ConfigurationDescriptor& ControlledEntityImpl::getConfigurationDescriptor(entity::model::ConfigurationIndex const configurationIndex)
{
	// Implemented over getConfigurationDescriptor() const overload
	return const_cast<model::ConfigurationDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getConfigurationDescriptor(configurationIndex));
}

model::AudioUnitDescriptor& ControlledEntityImpl::getAudioUnitDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex)
{
	// Implemented over getAudioUnitDescriptor() const overload
	return const_cast<model::AudioUnitDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getAudioUnitDescriptor(configurationIndex, audioUnitIndex));
}

model::StreamInputDescriptor& ControlledEntityImpl::getStreamInputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
{
	// Implemented over getStreamInputDescriptor() const overload
	return const_cast<model::StreamInputDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getStreamInputDescriptor(configurationIndex, streamIndex));
}

model::StreamOutputDescriptor& ControlledEntityImpl::getStreamOutputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
{
	// Implemented over getStreamOutputDescriptor() const overload
	return const_cast<model::StreamOutputDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getStreamOutputDescriptor(configurationIndex, streamIndex));
}

model::AvbInterfaceDescriptor& ControlledEntityImpl::getAvbInterfaceDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex)
{
	// Implemented over getAvbInterfaceDescriptor() const overload
	return const_cast<model::AvbInterfaceDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getAvbInterfaceDescriptor(configurationIndex, avbInterfaceIndex));
}

model::ClockSourceDescriptor& ControlledEntityImpl::getClockSourceDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex)
{
	// Implemented over getClockSourceDescriptor() const overload
	return const_cast<model::ClockSourceDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getClockSourceDescriptor(configurationIndex, clockSourceIndex));
}

model::LocaleDescriptor& ControlledEntityImpl::getLocaleDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex)
{
	// Implemented over getLocaleDescriptor() const overload
	return const_cast<model::LocaleDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getLocaleDescriptor(configurationIndex, localeIndex));
}

model::StringsDescriptor& ControlledEntityImpl::getStringsDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex)
{
	// Implemented over getStringsDescriptor() const overload
	return const_cast<model::StringsDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getStringsDescriptor(configurationIndex, stringsIndex));
}

model::StreamPortDescriptor& ControlledEntityImpl::getStreamPortInputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex)
{
	// Implemented over getStreamPortInputDescriptor() const overload
	return const_cast<model::StreamPortDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getStreamPortInputDescriptor(configurationIndex, streamPortIndex));
}

model::StreamPortDescriptor& ControlledEntityImpl::getStreamPortOutputDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex)
{
	// Implemented over getStreamPortOutputDescriptor() const overload
	return const_cast<model::StreamPortDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getStreamPortOutputDescriptor(configurationIndex, streamPortIndex));
}

model::AudioClusterDescriptor& ControlledEntityImpl::getAudioClusterDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex)
{
	// Implemented over getAudioClusterDescriptor() const overload
	return const_cast<model::AudioClusterDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getAudioClusterDescriptor(configurationIndex, clusterIndex));
}

model::AudioMapDescriptor& ControlledEntityImpl::getAudioMapDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex)
{
	// Implemented over getAudioMapDescriptor() const overload
	return const_cast<model::AudioMapDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getAudioMapDescriptor(configurationIndex, mapIndex));
}

model::ClockDomainDescriptor& ControlledEntityImpl::getClockDomainDescriptor(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex)
{
	// Implemented over getClockDomainDescriptor() const overload
	return const_cast<model::ClockDomainDescriptor&>(static_cast<ControlledEntityImpl const*>(this)->getClockDomainDescriptor(configurationIndex, clockDomainIndex));
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

void ControlledEntityImpl::setEntityDescriptor(entity::model::EntityDescriptor const& descriptor)
{
	// Wipe previous model::EntityDescriptor, we can have only one EntityDescriptor
	_entityModel = {};

	// Wipe model::EntityNode
	_entityNode = {};

	// Copy static model
	{
		auto& m = _entityModel.staticModel;
		m.vendorNameString = descriptor.vendorNameString;
		m.modelNameString = descriptor.modelNameString;
	}

	// Copy dynamic model
	{
		auto& m = _entityModel.dynamicModel;
		m.entityName = descriptor.entityName;
		m.groupName = descriptor.groupName;
		m.firmwareVersion = descriptor.firmwareVersion;
		m.serialNumber = descriptor.serialNumber;
		m.currentConfiguration = descriptor.currentConfiguration;
	}
}

template<typename ProtocolDescriptorType, typename ModelDescriptorType, typename IndexType, void (ControlledEntityImpl::*MethodPtr)(ProtocolDescriptorType const&, entity::model::ConfigurationIndex, IndexType)>
void allocateDescriptors(ControlledEntityImpl* entity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ConfigurationDescriptor const& descriptor, entity::model::DescriptorType const descriptorType)
{
	auto countIt = descriptor.descriptorCounts.find(descriptorType);
	if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
	{
		auto count = countIt->second;
		for (auto index = IndexType(0u); index < count; ++index)
		{
			(entity->*MethodPtr)(ProtocolDescriptorType{}, configurationIndex, index);
		}
	}
}

void ControlledEntityImpl::setConfigurationDescriptor(entity::model::ConfigurationDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex)
{
	auto& entityDescriptor = getEntityDescriptor();

	// Get or create a new model::ConfigurationDescriptor for this entity
	auto& configDescriptor = entityDescriptor.configurationDescriptors[configurationIndex];

	// Copy static model
	{
		auto& m = configDescriptor.staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.descriptorCounts = descriptor.descriptorCounts;
	}

	// Copy dynamic model
	{
		auto& m = configDescriptor.dynamicModel;
		m.objectName = descriptor.objectName;
		m.isActiveConfiguration = configurationIndex == entityDescriptor.dynamicModel.currentConfiguration;
	}

	// Pre-allocate all descriptors, in case we receive an unsolicited notification for the dynamic part of one of them, before we have retrieved them
	allocateDescriptors<entity::model::AudioUnitDescriptor, model::AudioUnitDescriptor, entity::model::AudioUnitIndex, &ControlledEntityImpl::setAudioUnitDescriptor>(this, configurationIndex, descriptor, entity::model::DescriptorType::AudioUnit);
	allocateDescriptors<entity::model::StreamDescriptor, model::StreamInputDescriptor, entity::model::StreamIndex, &ControlledEntityImpl::setStreamInputDescriptor>(this, configurationIndex, descriptor, entity::model::DescriptorType::StreamInput);
	allocateDescriptors<entity::model::StreamDescriptor, model::StreamInputDescriptor, entity::model::StreamIndex, &ControlledEntityImpl::setStreamOutputDescriptor>(this, configurationIndex, descriptor, entity::model::DescriptorType::StreamOutput);
	allocateDescriptors<entity::model::AvbInterfaceDescriptor, model::AvbInterfaceDescriptor, entity::model::AvbInterfaceIndex, &ControlledEntityImpl::setAvbInterfaceDescriptor>(this, configurationIndex, descriptor, entity::model::DescriptorType::AvbInterface);
	allocateDescriptors<entity::model::ClockSourceDescriptor, model::ClockSourceDescriptor, entity::model::ClockSourceIndex, &ControlledEntityImpl::setClockSourceDescriptor>(this, configurationIndex, descriptor, entity::model::DescriptorType::ClockSource);
	allocateDescriptors<entity::model::LocaleDescriptor, model::LocaleDescriptor, entity::model::LocaleIndex, &ControlledEntityImpl::setLocaleDescriptor>(this, configurationIndex, descriptor, entity::model::DescriptorType::Locale);
	allocateDescriptors<entity::model::StringsDescriptor, model::StringsDescriptor, entity::model::StringsIndex, &ControlledEntityImpl::setStringsDescriptor>(this, configurationIndex, descriptor, entity::model::DescriptorType::Strings);
	allocateDescriptors<entity::model::StreamPortDescriptor, model::StreamPortDescriptor, entity::model::StreamPortIndex, &ControlledEntityImpl::setStreamPortInputDescriptor>(this, configurationIndex, descriptor, entity::model::DescriptorType::StreamPortInput);
	allocateDescriptors<entity::model::StreamPortDescriptor, model::StreamPortDescriptor, entity::model::StreamPortIndex, &ControlledEntityImpl::setStreamPortOutputDescriptor>(this, configurationIndex, descriptor, entity::model::DescriptorType::StreamPortOutput);
	allocateDescriptors<entity::model::AudioClusterDescriptor, model::AudioClusterDescriptor, entity::model::ClusterIndex, &ControlledEntityImpl::setAudioClusterDescriptor>(this, configurationIndex, descriptor, entity::model::DescriptorType::AudioCluster);
	allocateDescriptors<entity::model::AudioMapDescriptor, model::AudioMapDescriptor, entity::model::MapIndex, &ControlledEntityImpl::setAudioMapDescriptor>(this, configurationIndex, descriptor, entity::model::DescriptorType::AudioMap);
	allocateDescriptors<entity::model::ClockDomainDescriptor, model::ClockDomainDescriptor, entity::model::ClockDomainIndex, &ControlledEntityImpl::setClockDomainDescriptor>(this, configurationIndex, descriptor, entity::model::DescriptorType::ClockDomain);
}

void ControlledEntityImpl::setAudioUnitDescriptor(entity::model::AudioUnitDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex)
{
	auto& configDescriptor = getConfigurationDescriptor(configurationIndex);

	// Get or create a new model::AudioUnitDescriptor
	auto& audioUnitDescriptor = configDescriptor.audioUnitDescriptors[audioUnitIndex];

	// Copy static model
	{
		auto& m = audioUnitDescriptor.staticModel;
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
		auto& m = audioUnitDescriptor.dynamicModel;
		m.objectName = descriptor.objectName;
		m.currentSamplingRate = descriptor.currentSamplingRate;
	}
}

void ControlledEntityImpl::setStreamInputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
{
	auto& configDescriptor = getConfigurationDescriptor(configurationIndex);

	// Get or create a new model::StreamInputDescriptor
	auto& streamInputDescriptor = configDescriptor.streamInputDescriptors[streamIndex];

	// Copy static model
	{
		auto& m = streamInputDescriptor.staticModel;
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
		auto& m = streamInputDescriptor.dynamicModel;
		m.objectName = descriptor.objectName;
		m.currentFormat = descriptor.currentFormat;
		m.connectionState.listenerStream = entity::model::StreamIdentification{ _entity.getEntityID(), streamIndex };
	}
}

void ControlledEntityImpl::setStreamOutputDescriptor(entity::model::StreamDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
{
	auto& configDescriptor = getConfigurationDescriptor(configurationIndex);

	// Get or create a new model::StreamOutputDescriptor
	auto& streamOutputDescriptor = configDescriptor.streamOutputDescriptors[streamIndex];

	// Copy static model
	{
		auto& m = streamOutputDescriptor.staticModel;
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
		auto& m = streamOutputDescriptor.dynamicModel;
		m.objectName = descriptor.objectName;
		m.currentFormat = descriptor.currentFormat;
	}
}

void ControlledEntityImpl::setAvbInterfaceDescriptor(entity::model::AvbInterfaceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const interfaceIndex)
{
	auto& configDescriptor = getConfigurationDescriptor(configurationIndex);

	// Get or create a new model::AvbInterfaceDescriptor
	auto& avbInterfaceDescriptor = configDescriptor.avbInterfaceDescriptors[interfaceIndex];

	// Copy static model
	{
		auto& m = avbInterfaceDescriptor.staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.macAddress = descriptor.macAddress;
		m.interfaceFlags = descriptor.interfaceFlags;
		m.clockIdentify = descriptor.clockIdentify;
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
		auto& m = avbInterfaceDescriptor.dynamicModel;
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setClockSourceDescriptor(entity::model::ClockSourceDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockIndex)
{
	auto& configDescriptor = getConfigurationDescriptor(configurationIndex);

	// Get or create a new model::ClockSourceDescriptor
	auto& clockSourceDescriptor = configDescriptor.clockSourceDescriptors[clockIndex];

	// Copy static model
	{
		auto& m = clockSourceDescriptor.staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.clockSourceType = descriptor.clockSourceType;
		m.clockSourceLocationType = descriptor.clockSourceLocationType;
		m.clockSourceLocationIndex = descriptor.clockSourceLocationIndex;
	}

	// Copy dynamic model
	{
		auto& m = clockSourceDescriptor.dynamicModel;
		m.objectName = descriptor.objectName;
		m.clockSourceFlags = descriptor.clockSourceFlags;
		m.clockSourceIdentifier = descriptor.clockSourceIdentifier;
	}
}

void ControlledEntityImpl::setSelectedLocaleBaseIndex(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const baseIndex)
{
	auto& configDescriptor = getConfigurationDescriptor(configurationIndex);
	configDescriptor.dynamicModel.selectedLocaleBaseIndex = baseIndex;
}

void ControlledEntityImpl::setLocaleDescriptor(entity::model::LocaleDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex)
{
	auto& configDescriptor = getConfigurationDescriptor(configurationIndex);

	// Get or create a new model::LocaleDescriptor
	auto& localeDescriptor = configDescriptor.localeDescriptors[localeIndex];

	// Copy static model
	{
		auto& m = localeDescriptor.staticModel;
		m.localeID = descriptor.localeID;
		m.numberOfStringDescriptors = descriptor.numberOfStringDescriptors;
		m.baseStringDescriptorIndex = descriptor.baseStringDescriptorIndex;
	}
}

void ControlledEntityImpl::setStringsDescriptor(entity::model::StringsDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex)
{
	auto& configDescriptor = getConfigurationDescriptor(configurationIndex);

	// Get or create a new model::StringsDescriptor
	auto& stringsDescriptor = configDescriptor.stringsDescriptors[stringsIndex];

	// Copy static model
	{
		auto& m = stringsDescriptor.staticModel;
		m.strings = descriptor.strings;
	}

	// Copy the strings to the ConfigurationNode for a quick access
	for (auto strIndex = 0u; strIndex < descriptor.strings.size(); ++strIndex)
	{
		auto localizedStringIndex = entity::model::StringsIndex(stringsIndex * descriptor.strings.size() + strIndex);
		configDescriptor.dynamicModel.localizedStrings[localizedStringIndex] = descriptor.strings.at(strIndex);
	}
}

void ControlledEntityImpl::setStreamPortInputDescriptor(entity::model::StreamPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex)
{
	auto& configDescriptor = getConfigurationDescriptor(configurationIndex);

	// Get or create a new model::StreamPortDescriptor
	auto& streamPortDescriptor = configDescriptor.streamPortInputDescriptors[streamPortIndex];

	// Copy static model
	{
		auto& m = streamPortDescriptor.staticModel;
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

void ControlledEntityImpl::setStreamPortOutputDescriptor(entity::model::StreamPortDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex)
{
	auto& configDescriptor = getConfigurationDescriptor(configurationIndex);

	// Get or create a new model::StreamPortDescriptor
	auto& streamPortDescriptor = configDescriptor.streamPortOutputDescriptors[streamPortIndex];

	// Copy static model
	{
		auto& m = streamPortDescriptor.staticModel;
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

void ControlledEntityImpl::setAudioClusterDescriptor(entity::model::AudioClusterDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex)
{
	auto& configDescriptor = getConfigurationDescriptor(configurationIndex);

	// Get or create a new model::AudioClusterDescriptor
	auto& audioClusterDescriptor = configDescriptor.audioClusterDescriptors[clusterIndex];

	// Copy static model
	{
		auto& m = audioClusterDescriptor.staticModel;
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
		auto& m = audioClusterDescriptor.dynamicModel;
		m.objectName = descriptor.objectName;
	}
}

void ControlledEntityImpl::setAudioMapDescriptor(entity::model::AudioMapDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex)
{
	auto& configDescriptor = getConfigurationDescriptor(configurationIndex);

	// Get or create a new model::AudioMapDescriptor
	auto& audioMapDescriptor = configDescriptor.audioMapDescriptors[mapIndex];

	// Copy static model
	{
		auto& m = audioMapDescriptor.staticModel;
		m.mappings = descriptor.mappings;
	}
}

void ControlledEntityImpl::setClockDomainDescriptor(entity::model::ClockDomainDescriptor const& descriptor, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex)
{
	auto& configDescriptor = getConfigurationDescriptor(configurationIndex);

	// Get or create a new model::ClockDomainDescriptor
	auto& clockDomainDescriptor = configDescriptor.clockDomainDescriptors[clockDomainIndex];

	// Copy static model
	{
		auto& m = clockDomainDescriptor.staticModel;
		m.localizedDescription = descriptor.localizedDescription;
		m.clockSources = descriptor.clockSources;
	}

	// Copy dynamic model
	{
		auto& m = clockDomainDescriptor.dynamicModel;
		m.objectName = descriptor.objectName;
		m.clockSourceIndex = descriptor.clockSourceIndex;
	}
}

void ControlledEntityImpl::setInputStreamState(model::StreamConnectionState const& state, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
{
	AVDECC_ASSERT(_entity.getEntityID() == state.listenerStream.entityID, "EntityID not correctly initialized");
	AVDECC_ASSERT(streamIndex == state.listenerStream.streamIndex, "StreamIndex not correctly initialized");

	auto& streamDescriptor = getStreamInputDescriptor(configurationIndex, streamIndex);

	// Set StreamConnectionState
	streamDescriptor.dynamicModel.connectionState = state;
}

void ControlledEntityImpl::clearPortInputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex)
{
	auto& streamPortDescriptor = getStreamPortInputDescriptor(configurationIndex, streamPortIndex);

	// Clear audio mappings
	streamPortDescriptor.dynamicModel.dynamicAudioMap.clear();
}

void ControlledEntityImpl::clearPortOutputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex)
{
	auto& streamPortDescriptor = getStreamPortOutputDescriptor(configurationIndex, streamPortIndex);

	// Clear audio mappings
	streamPortDescriptor.dynamicModel.dynamicAudioMap.clear();
}

void ControlledEntityImpl::addPortInputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
{
	auto& streamPortDescriptor = getStreamPortInputDescriptor(configurationIndex, streamPortIndex);
	auto& dynamicMap = streamPortDescriptor.dynamicModel.dynamicAudioMap;

	// Process audio mappings
	for (auto const& map : mappings)
	{
		// Check if mapping must be replaced
		auto foundIt = std::find_if(dynamicMap.begin(), dynamicMap.end(), [&map](entity::model::AudioMapping const& mapping)
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

void ControlledEntityImpl::addPortOutputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
{
	auto& streamPortDescriptor = getStreamPortOutputDescriptor(configurationIndex, streamPortIndex);
	auto& dynamicMap = streamPortDescriptor.dynamicModel.dynamicAudioMap;

	// Process audio mappings
	for (auto const& map : mappings)
	{
		// Check if mapping must be replaced
		auto foundIt = std::find_if(dynamicMap.begin(), dynamicMap.end(), [&map](entity::model::AudioMapping const& mapping)
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
			LOG_CONTROLLER_WARN(_entity.getEntityID(), std::string("Duplicate StreamPortOutput AudioMappings found: ") + std::to_string(foundIt->clusterOffset) + ":" + std::to_string(foundIt->clusterChannel) + ":" + std::to_string(foundIt->streamIndex) + ":" + std::to_string(foundIt->streamChannel) + " replaced by " + std::to_string(map.clusterOffset) + ":" + std::to_string(map.clusterChannel) + ":" + std::to_string(map.streamIndex) + ":" + std::to_string(map.streamChannel));
#endif // !ENABLE_AVDECC_FEATURE_REDUNDANCY
			foundIt->streamIndex = map.streamIndex;
			foundIt->streamChannel = map.streamChannel;
		}
	}
}

void ControlledEntityImpl::removePortInputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
{
	auto& streamPortDescriptor = getStreamPortInputDescriptor(configurationIndex, streamPortIndex);
	auto& dynamicMap = streamPortDescriptor.dynamicModel.dynamicAudioMap;

	// Process audio mappings
	for (auto const& map : mappings)
	{
		// Check if mapping exists
		auto foundIt = std::find_if(dynamicMap.begin(), dynamicMap.end(), [&map](entity::model::AudioMapping const& mapping)
		{
			return (map.clusterOffset == mapping.clusterOffset) && (map.clusterChannel == mapping.clusterChannel);
		});
		if (AVDECC_ASSERT_WITH_RET(foundIt != dynamicMap.end(), "Mapping not found"))
			dynamicMap.erase(foundIt);
	}
}

void ControlledEntityImpl::removePortOutputStreamAudioMappings(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings)
{
	auto& streamPortDescriptor = getStreamPortOutputDescriptor(configurationIndex, streamPortIndex);
	auto& dynamicMap = streamPortDescriptor.dynamicModel.dynamicAudioMap;

	// Process audio mappings
	for (auto const& map : mappings)
	{
		// Check if mapping exists
		auto foundIt = std::find_if(dynamicMap.begin(), dynamicMap.end(), [&map](entity::model::AudioMapping const& mapping)
		{
			return (map.clusterOffset == mapping.clusterOffset) && (map.clusterChannel == mapping.clusterChannel);
		});
		if (AVDECC_ASSERT_WITH_RET(foundIt != dynamicMap.end(), "Mapping not found"))
			dynamicMap.erase(foundIt);
	}
}

void ControlledEntityImpl::clearStreamOutputConnections(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex)
{
	auto& streamDescriptor = getStreamOutputDescriptor(configurationIndex, streamIndex);

	// Clear connections
	streamDescriptor.dynamicModel.connections.clear();
}

bool ControlledEntityImpl::addStreamOutputConnection(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& listenerStream)
{
	auto& streamDescriptor = getStreamOutputDescriptor(configurationIndex, streamIndex);

	// Add connection
	auto const result = streamDescriptor.dynamicModel.connections.insert(listenerStream);
	return result.second;
}

bool ControlledEntityImpl::delStreamOutputConnection(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamIdentification const& listenerStream)
{
	auto& streamDescriptor = getStreamOutputDescriptor(configurationIndex, streamIndex);

	// Remove connection
	return streamDescriptor.dynamicModel.connections.erase(listenerStream) > 0;
}

void ControlledEntityImpl::setInputStreamInfo(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info)
{
	auto& streamDescriptor = getStreamInputDescriptor(configurationIndex, streamIndex);

	// Set StreamInfo
	streamDescriptor.dynamicModel.streamInfo = info;

	// Set isRunning
	streamDescriptor.dynamicModel.isRunning = !hasFlag(info.streamInfoFlags, entity::StreamInfoFlags::StreamingWait);
}

void ControlledEntityImpl::setOutputStreamInfo(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info)
{
	auto& streamDescriptor = getStreamOutputDescriptor(configurationIndex, streamIndex);

	// Set StreamInfo
	streamDescriptor.dynamicModel.streamInfo = info;

	// Set isRunning
	streamDescriptor.dynamicModel.isRunning = !hasFlag(info.streamInfoFlags, entity::StreamInfoFlags::StreamingWait);
}

void ControlledEntityImpl::setAvbInfo(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info)
{
	auto& avbInterfaceDescriptor = getAvbInterfaceDescriptor(configurationIndex, avbInterfaceIndex);

	// Set AvbInfo
	avbInterfaceDescriptor.dynamicModel.avbInfo = info;
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
static inline ControlledEntityImpl::DynamicInfoKey makeDynamicInfoKey(ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex descriptorIndex, std::uint16_t const connectionIndex)
{
	return (static_cast<ControlledEntityImpl::DynamicInfoKey>(la::avdecc::to_integral(dynamicInfoType)) << ((sizeof(descriptorIndex) + sizeof(connectionIndex)) * 8)) + (descriptorIndex << (sizeof(connectionIndex) * 8)) + connectionIndex;
}

bool ControlledEntityImpl::checkAndClearExpectedDynamicInfo(entity::model::ConfigurationIndex const configurationIndex, DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const connectionIndex) noexcept
{
	auto const confIt = _expectedDynamicInfo.find(configurationIndex);

	if (confIt == _expectedDynamicInfo.end())
		return false;

	auto const key = makeDynamicInfoKey(dynamicInfoType, descriptorIndex, connectionIndex);

	return confIt->second.erase(key) == 1;
}

void ControlledEntityImpl::setDynamicInfoExpected(entity::model::ConfigurationIndex const configurationIndex, DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const connectionIndex) noexcept
{
	auto& conf = _expectedDynamicInfo[configurationIndex];

	auto const key = makeDynamicInfoKey(dynamicInfoType, descriptorIndex, connectionIndex);
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
	LOG_CONTROLLER_ERROR(_entity.getEntityID(), "Enumeration Error");
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
			_entityNode.staticModel = &_entityModel.staticModel;
			_entityNode.dynamicModel = &_entityModel.dynamicModel;

			// Build configuration nodes (ConfigurationNode)
			for (auto& configKV : _entityModel.configurationDescriptors)
			{
				auto const configIndex = configKV.first;
				auto& configDescriptor = configKV.second;

				auto& configNode = _entityNode.configurations[configIndex];
				initNode(configNode, entity::model::DescriptorType::Configuration, configIndex, model::AcquireState::Undefined);
				configNode.staticModel = &configDescriptor.staticModel;
				configNode.dynamicModel = &configDescriptor.dynamicModel;

				// Build audio units (AudioUnitNode)
				for (auto& audioUnitKV : configDescriptor.audioUnitDescriptors)
				{
					auto const audioUnitIndex = audioUnitKV.first;
					auto& audioUnitDescriptor = audioUnitKV.second;

					auto& audioUnitNode = configNode.audioUnits[audioUnitIndex];
					initNode(audioUnitNode, entity::model::DescriptorType::AudioUnit, audioUnitIndex, model::AcquireState::Undefined);
					audioUnitNode.staticModel = &audioUnitDescriptor.staticModel;
					audioUnitNode.dynamicModel = &audioUnitDescriptor.dynamicModel;

					// Build stream port inputs and outputs (StreamPortNode)
					auto processStreamPorts = [entity = const_cast<ControlledEntityImpl*>(this), &audioUnitNode, configIndex](entity::model::DescriptorType const descriptorType, std::uint16_t const numberOfStreamPorts, entity::model::StreamPortIndex const baseStreamPort)
					{
						for (auto streamPortIndexCounter = entity::model::StreamPortIndex(0); streamPortIndexCounter < numberOfStreamPorts; ++streamPortIndexCounter)
						{
							model::StreamPortNode* streamPortNode{ nullptr };
							model::StreamPortDescriptor* streamPortDescriptor{ nullptr };
							auto const streamPortIndex = entity::model::StreamPortIndex(streamPortIndexCounter + baseStreamPort);

							if (descriptorType == entity::model::DescriptorType::StreamPortInput)
							{
								streamPortNode = &audioUnitNode.streamPortInputs[streamPortIndex];
								streamPortDescriptor = &entity->getStreamPortInputDescriptor(configIndex, streamPortIndex);
							}
							else
							{
								streamPortNode = &audioUnitNode.streamPortOutputs[streamPortIndex];
								streamPortDescriptor = &entity->getStreamPortOutputDescriptor(configIndex, streamPortIndex);
							}

							initNode(*streamPortNode, descriptorType, streamPortIndex, model::AcquireState::Undefined);
							streamPortNode->staticModel = &streamPortDescriptor->staticModel;
							streamPortNode->dynamicModel = &streamPortDescriptor->dynamicModel;

							// Build audio clusters (AudioClusterNode)
							for (auto clusterIndexCounter = entity::model::ClusterIndex(0); clusterIndexCounter < streamPortDescriptor->staticModel.numberOfClusters; ++clusterIndexCounter)
							{
								auto const clusterIndex = entity::model::ClusterIndex(clusterIndexCounter + streamPortDescriptor->staticModel.baseCluster);
								auto& audioClusterNode = streamPortNode->audioClusters[clusterIndex];
								initNode(audioClusterNode, entity::model::DescriptorType::AudioCluster, clusterIndex, model::AcquireState::Undefined);
								auto& audioClusterDescriptor = entity->getAudioClusterDescriptor(configIndex, clusterIndex);
								audioClusterNode.staticModel = &audioClusterDescriptor.staticModel;
								audioClusterNode.dynamicModel = &audioClusterDescriptor.dynamicModel;
							}

							// Build audio maps (AudioMapNode)
							for (auto mapIndexCounter = entity::model::MapIndex(0); mapIndexCounter < streamPortDescriptor->staticModel.numberOfMaps; ++mapIndexCounter)
							{
								auto const mapIndex = entity::model::MapIndex(mapIndexCounter + streamPortDescriptor->staticModel.baseMap);
								auto& audioMapNode = streamPortNode->audioMaps[mapIndex];
								initNode(audioMapNode, entity::model::DescriptorType::AudioMap, mapIndex, model::AcquireState::Undefined);
								auto const& audioMapDescriptor = entity->getAudioMapDescriptor(configIndex, mapIndex);
								audioMapNode.staticModel = &audioMapDescriptor.staticModel;
							}
						}
					};
					processStreamPorts(entity::model::DescriptorType::StreamPortInput, audioUnitDescriptor.staticModel.numberOfStreamInputPorts, audioUnitDescriptor.staticModel.baseStreamInputPort);
					processStreamPorts(entity::model::DescriptorType::StreamPortOutput, audioUnitDescriptor.staticModel.numberOfStreamOutputPorts, audioUnitDescriptor.staticModel.baseStreamOutputPort);
				}

				// Build stream inputs (StreamNode)
				for (auto& streamKV : configDescriptor.streamInputDescriptors)
				{
					auto const streamIndex = streamKV.first;
					auto& streamDescriptor = streamKV.second;

					auto& streamNode = configNode.streamInputs[streamIndex];
					initNode(streamNode, entity::model::DescriptorType::StreamInput, streamIndex, model::AcquireState::Undefined);
					streamNode.staticModel = &streamDescriptor.staticModel;
					streamNode.dynamicModel = &streamDescriptor.dynamicModel;
				}

				// Build stream outputs (StreamNode)
				for (auto& streamKV : configDescriptor.streamOutputDescriptors)
				{
					auto const streamIndex = streamKV.first;
					auto& streamDescriptor = streamKV.second;

					auto& streamNode = configNode.streamOutputs[streamIndex];
					initNode(streamNode, entity::model::DescriptorType::StreamOutput, streamIndex, model::AcquireState::Undefined);
					streamNode.staticModel = &streamDescriptor.staticModel;
					streamNode.dynamicModel = &streamDescriptor.dynamicModel;
				}

				// Build avb interfaces (AvbInterfaceNode)
				for (auto& interfaceKV : configDescriptor.avbInterfaceDescriptors)
				{
					auto const interfaceIndex = interfaceKV.first;
					auto& interfaceDescriptor = interfaceKV.second;

					auto& interfaceNode = configNode.avbInterfaces[interfaceIndex];
					initNode(interfaceNode, entity::model::DescriptorType::AvbInterface, interfaceIndex, model::AcquireState::Undefined);
					interfaceNode.staticModel = &interfaceDescriptor.staticModel;
					interfaceNode.dynamicModel = &interfaceDescriptor.dynamicModel;
				}

				// Build clock sources (ClockSourceNode)
				for (auto& sourceKV : configDescriptor.clockSourceDescriptors)
				{
					auto const sourceIndex = sourceKV.first;
					auto& sourceDescriptor = sourceKV.second;

					auto& sourceNode = configNode.clockSources[sourceIndex];
					initNode(sourceNode, entity::model::DescriptorType::ClockSource, sourceIndex, model::AcquireState::Undefined);
					sourceNode.staticModel = &sourceDescriptor.staticModel;
					sourceNode.dynamicModel = &sourceDescriptor.dynamicModel;
				}

				// Build locales (LocaleNode)
				for (auto const& localeKV : configDescriptor.localeDescriptors)
				{
					auto const localeIndex = localeKV.first;
					auto const& localeDescriptor = localeKV.second;

					auto& localeNode = configNode.locales[localeIndex];
					initNode(localeNode, entity::model::DescriptorType::Locale, localeIndex, model::AcquireState::Undefined);
					localeNode.staticModel = &localeDescriptor.staticModel;

					// Build strings (StringsNode)
					// TODO
				}

				// Build clock domains (ClockDomainNode)
				for (auto& domainKV : configDescriptor.clockDomainDescriptors)
				{
					auto const domainIndex = domainKV.first;
					auto& domainDescriptor = domainKV.second;

					auto& domainNode = configNode.clockDomains[domainIndex];
					initNode(domainNode, entity::model::DescriptorType::ClockDomain, domainIndex, model::AcquireState::Undefined);
					domainNode.staticModel = &domainDescriptor.staticModel;
					domainNode.dynamicModel = &domainDescriptor.dynamicModel;

					// Build associated clock sources (ClockSourceNode)
					for (auto const sourceIndex: domainDescriptor.staticModel.clockSources)
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

#ifdef ENABLE_AVDECC_STRICT_2018_REDUNDANCY
			// 2018 Redundancy specification only defines stream pairs
			if (staticModel->redundantStreams.size() != 1)
			{
				LOG_CONTROLLER_WARN(entityID, std::string("More than one StreamIndex in RedundantStreamAssociation"));
				continue;
			}
#endif // ENABLE_AVDECC_STRICT_2018_REDUNDANCY

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

#ifdef ENABLE_AVDECC_STRICT_2018_REDUNDANCY
			// Check AVB_INTERFACE index used are 0 for primary and 1 for secondary
			if (redundantStreamNodes.find(0u) == redundantStreamNodes.end() || redundantStreamNodes.find(1) == redundantStreamNodes.end())
			{
				isAssociationValid = false;
				LOG_CONTROLLER_ERROR(entityID, std::string("RedundantStreamAssociation invalid for ") + (streamNode.descriptorType == entity::model::DescriptorType::StreamInput ? "STREAM_INPUT." : "STREAM_OUTPUT.") + std::to_string(streamNode.descriptorIndex) + ": Redundant streams do not use AVB_INTERFACE 0 and 1");
			}
#endif // ENABLE_AVDECC_STRICT_2018_REDUNDANCY

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
