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
* @file treeModelAccessCacheStrategy.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "avdeccControlledEntityImpl.hpp"
#include "treeModelAccessStrategy.hpp"

namespace la
{
namespace avdecc
{
namespace controller
{
class TreeModelAccessCacheStrategy : public TreeModelAccessStrategy
{
public:
	TreeModelAccessCacheStrategy(ControlledEntityImpl* const entity) noexcept
		: TreeModelAccessStrategy{ entity }
	{
		// USE THE VISITOR TO BUILD THE MAP
	}

private:
	//	/** A cache for quick descriptor access from the configuration level */
	//	struct DescriptorReferenceCache
	//	{
	//		// TODO: Add all descriptors
	//		std::map<entity::model::ControlIndex, std::reference_wrapper<ControlNode>> controls{};
	//	};

	virtual StrategyType getStrategyType() const noexcept override
	{
		return StrategyType::Cached;
	}

	virtual model::EntityNode* getEntityNode(NotFoundBehavior const notFoundBehavior) override
	{
		if (_entity->gotFatalEnumerationError())
		{
			handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::EnumerationError, "Entity had an enumeration error");
			return nullptr;
		}

		if (!_entity->getEntity().getEntityCapabilities().test(entity::EntityCapability::AemSupported))
		{
			handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::NotSupported, "EM not supported by the entity");
			return nullptr;
		}

		return &_entity->_entityNode;
	}

	virtual model::ConfigurationNode* getConfigurationNode(entity::model::ConfigurationIndex const configurationIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const entityNode = getEntityNode(notFoundBehavior);
		if (entityNode)
		{
			auto it = entityNode->configurations.find(configurationIndex);
			if (it == entityNode->configurations.end())
			{
				if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidConfigurationIndex, "Invalid configuration index"))
				{
					return nullptr;
				}
				it = entityNode->configurations.emplace(configurationIndex, model::ConfigurationNode{ configurationIndex }).first;
			}

			return &(it->second);
		}
		return nullptr;
	}

	virtual model::AudioUnitNode* getAudioUnitNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			auto it = configurationNode->audioUnits.find(descriptorIndex);
			if (it == configurationNode->audioUnits.end())
			{
				if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid audio unit index"))
				{
					return nullptr;
				}
				it = configurationNode->audioUnits.emplace(descriptorIndex, model::AudioUnitNode{ descriptorIndex }).first;
			}

			return &(it->second);
		}
		return nullptr;
	}

	virtual model::StreamInputNode* getStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			auto it = configurationNode->streamInputs.find(descriptorIndex);
			if (it == configurationNode->streamInputs.end())
			{
				if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid stream input index"))
				{
					return nullptr;
				}
				it = configurationNode->streamInputs.emplace(descriptorIndex, model::StreamInputNode{ descriptorIndex }).first;
			}

			return &(it->second);
		}
		return nullptr;
	}
	virtual model::StreamOutputNode* getStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			auto it = configurationNode->streamOutputs.find(descriptorIndex);
			if (it == configurationNode->streamOutputs.end())
			{
				if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid stream output index"))
				{
					return nullptr;
				}
				it = configurationNode->streamOutputs.emplace(descriptorIndex, model::StreamOutputNode{ descriptorIndex }).first;
			}

			return &(it->second);
		}
		return nullptr;
	}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual model::RedundantStreamInputNode* getRedundantStreamInputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			auto it = configurationNode->redundantStreamInputs.find(redundantStreamIndex);
			if (it == configurationNode->redundantStreamInputs.end())
			{
				if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid redundant stream input index"))
				{
					return nullptr;
				}
				it = configurationNode->redundantStreamInputs.emplace(redundantStreamIndex, model::RedundantStreamInputNode{ redundantStreamIndex }).first;
			}

			return &(it->second);
		}
		return nullptr;
	}
	virtual model::RedundantStreamOutputNode* getRedundantStreamOutputNode(entity::model::ConfigurationIndex const configurationIndex, model::VirtualIndex const redundantStreamIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			auto it = configurationNode->redundantStreamOutputs.find(redundantStreamIndex);
			if (it == configurationNode->redundantStreamOutputs.end())
			{
				if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid redundant stream output index"))
				{
					return nullptr;
				}
				it = configurationNode->redundantStreamOutputs.emplace(redundantStreamIndex, model::RedundantStreamOutputNode{ redundantStreamIndex }).first;
			}

			return &(it->second);
		}
		return nullptr;
	}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY


	virtual model::JackInputNode* getJackInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			auto it = configurationNode->jackInputs.find(descriptorIndex);
			if (it == configurationNode->jackInputs.end())
			{
				if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid jack input index"))
				{
					return nullptr;
				}
				it = configurationNode->jackInputs.emplace(descriptorIndex, model::JackInputNode{ descriptorIndex }).first;
			}

			return &(it->second);
		}
		return nullptr;
	}
	virtual model::JackOutputNode* getJackOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			auto it = configurationNode->jackOutputs.find(descriptorIndex);
			if (it == configurationNode->jackOutputs.end())
			{
				if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid jack output index"))
				{
					return nullptr;
				}
				it = configurationNode->jackOutputs.emplace(descriptorIndex, model::JackOutputNode{ descriptorIndex }).first;
			}

			return &(it->second);
		}
		return nullptr;
	}

	virtual model::AvbInterfaceNode* getAvbInterfaceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			auto it = configurationNode->avbInterfaces.find(descriptorIndex);
			if (it == configurationNode->avbInterfaces.end())
			{
				if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid avbInterface index"))
				{
					return nullptr;
				}
				it = configurationNode->avbInterfaces.emplace(descriptorIndex, model::AvbInterfaceNode{ descriptorIndex }).first;
			}

			return &(it->second);
		}
		return nullptr;
	}

	virtual model::ClockSourceNode* getClockSourceNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			auto it = configurationNode->clockSources.find(descriptorIndex);
			if (it == configurationNode->clockSources.end())
			{
				if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid clockSource index"))
				{
					return nullptr;
				}
				it = configurationNode->clockSources.emplace(descriptorIndex, model::ClockSourceNode{ descriptorIndex }).first;
			}

			return &(it->second);
		}
		return nullptr;
	}

	virtual model::MemoryObjectNode* getMemoryObjectNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			auto it = configurationNode->memoryObjects.find(descriptorIndex);
			if (it == configurationNode->memoryObjects.end())
			{
				if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid memoryObject index"))
				{
					return nullptr;
				}
				it = configurationNode->memoryObjects.emplace(descriptorIndex, model::MemoryObjectNode{ descriptorIndex }).first;
			}

			return &(it->second);
		}
		return nullptr;
	}

	virtual model::LocaleNode* getLocaleNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			auto it = configurationNode->locales.find(descriptorIndex);
			if (it == configurationNode->locales.end())
			{
				if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid locale index"))
				{
					return nullptr;
				}
				it = configurationNode->locales.emplace(descriptorIndex, model::LocaleNode{ descriptorIndex }).first;
			}

			return &(it->second);
		}
		return nullptr;
	}

	virtual model::StringsNode* getStringsNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			// Search a matching StringsIndex in all Locales
			for (auto& localeNodeKV : configurationNode->locales)
			{
				auto& localeNode = localeNodeKV.second;

				if (isDescriptorIndexInRange(descriptorIndex, localeNode.staticModel.baseStringDescriptorIndex, localeNode.staticModel.numberOfStringDescriptors))
				{
					auto it = localeNode.strings.find(descriptorIndex);
					if (it == localeNode.strings.end())
					{
						if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid strings index"))
						{
							return nullptr;
						}
						it = localeNode.strings.emplace(descriptorIndex, model::StringsNode{ descriptorIndex }).first;
					}

					return &(it->second);
				}
			}
			// Not found
			handleDescriptorNotFound(NotFoundBehavior::IgnoreAndReturnNull, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid strings index");
			return nullptr;
		}
		return nullptr;
	}

	virtual model::StreamPortInputNode* getStreamPortInputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			// Search a matching StreamPortIndex in all AudioUnits
			for (auto& unitNodeKV : configurationNode->audioUnits)
			{
				auto& unitNode = unitNodeKV.second;

				if (isDescriptorIndexInRange(descriptorIndex, unitNode.staticModel.baseStreamInputPort, unitNode.staticModel.numberOfStreamInputPorts))
				{
					auto it = unitNode.streamPortInputs.find(descriptorIndex);
					if (it == unitNode.streamPortInputs.end())
					{
						if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid streamPortInput index"))
						{
							return nullptr;
						}
						it = unitNode.streamPortInputs.emplace(descriptorIndex, model::StreamPortInputNode{ descriptorIndex }).first;
					}

					return &(it->second);
				}
			}

			// Search a matching StreamPortIndex in all VideoUnits

			// Search a matching StreamPortIndex in all SensorUnits

			// Not found
			handleDescriptorNotFound(NotFoundBehavior::IgnoreAndReturnNull, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid streamPortInput index");
			return nullptr;
		}
		return nullptr;
	}
	virtual model::StreamPortOutputNode* getStreamPortOutputNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			// Search a matching StreamPortIndex in all AudioUnits
			for (auto& unitNodeKV : configurationNode->audioUnits)
			{
				auto& unitNode = unitNodeKV.second;

				if (isDescriptorIndexInRange(descriptorIndex, unitNode.staticModel.baseStreamOutputPort, unitNode.staticModel.numberOfStreamOutputPorts))
				{
					auto it = unitNode.streamPortOutputs.find(descriptorIndex);
					if (it == unitNode.streamPortOutputs.end())
					{
						if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid streamPortOutput index"))
						{
							return nullptr;
						}
						it = unitNode.streamPortOutputs.emplace(descriptorIndex, model::StreamPortOutputNode{ descriptorIndex }).first;
					}

					return &(it->second);
				}
			}

			// Search a matching StreamPortIndex in all VideoUnits

			// Search a matching StreamPortIndex in all SensorUnits

			// Not found
			handleDescriptorNotFound(NotFoundBehavior::IgnoreAndReturnNull, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid streamPortOutput index");
			return nullptr;
		}
		return nullptr;
	}

	virtual model::AudioClusterNode* getAudioClusterNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			// Search a matching ClusterIndex in all AudioUnits/StreamPorts
			for (auto& unitNodeKV : configurationNode->audioUnits)
			{
				auto& unitNode = unitNodeKV.second;

				// Search StreamPortInputs
				for (auto& streamPortNodeKV : unitNode.streamPortInputs)
				{
					auto& streamPortNode = streamPortNodeKV.second;

					if (isDescriptorIndexInRange(descriptorIndex, streamPortNode.staticModel.baseCluster, streamPortNode.staticModel.numberOfClusters))
					{
						auto it = streamPortNode.audioClusters.find(descriptorIndex);
						if (it == streamPortNode.audioClusters.end())
						{
							if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid cluster index"))
							{
								return nullptr;
							}
							it = streamPortNode.audioClusters.emplace(descriptorIndex, model::AudioClusterNode{ descriptorIndex }).first;
						}

						return &(it->second);
					}
				}

				// Search StreamPortOutputs
				for (auto& streamPortNodeKV : unitNode.streamPortOutputs)
				{
					auto& streamPortNode = streamPortNodeKV.second;

					if (isDescriptorIndexInRange(descriptorIndex, streamPortNode.staticModel.baseCluster, streamPortNode.staticModel.numberOfClusters))
					{
						auto it = streamPortNode.audioClusters.find(descriptorIndex);
						if (it == streamPortNode.audioClusters.end())
						{
							if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid cluster index"))
							{
								return nullptr;
							}
							it = streamPortNode.audioClusters.emplace(descriptorIndex, model::AudioClusterNode{ descriptorIndex }).first;
						}

						return &(it->second);
					}
				}
			}

			// Search a matching ClusterIndex in all VideoUnits/StreamPorts

			// Search a matching ClusterIndex in all SensorUnits/StreamPorts

			// Not found
			handleDescriptorNotFound(NotFoundBehavior::IgnoreAndReturnNull, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid cluster index");
			return nullptr;
		}
		return nullptr;
	}

	virtual model::AudioMapNode* getAudioMapNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			// Search a matching MapIndex in all AudioUnits/StreamPorts
			for (auto& unitNodeKV : configurationNode->audioUnits)
			{
				auto& unitNode = unitNodeKV.second;

				// Search StreamPortInputs
				for (auto& streamPortNodeKV : unitNode.streamPortInputs)
				{
					auto& streamPortNode = streamPortNodeKV.second;

					if (isDescriptorIndexInRange(descriptorIndex, streamPortNode.staticModel.baseMap, streamPortNode.staticModel.numberOfMaps))
					{
						auto it = streamPortNode.audioMaps.find(descriptorIndex);
						if (it == streamPortNode.audioMaps.end())
						{
							if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid map index"))
							{
								return nullptr;
							}
							it = streamPortNode.audioMaps.emplace(descriptorIndex, model::AudioMapNode{ descriptorIndex }).first;
						}

						return &(it->second);
					}
				}

				// Search StreamPortOutputs
				for (auto& streamPortNodeKV : unitNode.streamPortOutputs)
				{
					auto& streamPortNode = streamPortNodeKV.second;

					if (isDescriptorIndexInRange(descriptorIndex, streamPortNode.staticModel.baseMap, streamPortNode.staticModel.numberOfMaps))
					{
						auto it = streamPortNode.audioMaps.find(descriptorIndex);
						if (it == streamPortNode.audioMaps.end())
						{
							if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid map index"))
							{
								return nullptr;
							}
							it = streamPortNode.audioMaps.emplace(descriptorIndex, model::AudioMapNode{ descriptorIndex }).first;
						}

						return &(it->second);
					}
				}
			}

			// Search a matching MapIndex in all VideoUnits/StreamPorts

			// Search a matching MapIndex in all SensorUnits/StreamPorts

			// Not found
			handleDescriptorNotFound(NotFoundBehavior::IgnoreAndReturnNull, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid map index");
			return nullptr;
		}
		return nullptr;
	}

	virtual model::ControlNode* getControlNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			// Search Top Level
			if (isDescriptorIndexInRange(descriptorIndex, entity::model::ControlIndex{ 0u }, configurationNode->staticModel.descriptorCounts[entity::model::DescriptorType::Control]))
			{
				auto it = configurationNode->controls.find(descriptorIndex);
				if (it == configurationNode->controls.end())
				{
					if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid control index"))
					{
						return nullptr;
					}
					it = configurationNode->controls.emplace(descriptorIndex, model::ControlNode{ descriptorIndex }).first;
				}

				return &(it->second);
			}

			// Search a matching ControlIndex in all AudioUnits/StreamPorts/ExternalPorts/InternalPorts
			for (auto& unitNodeKV : configurationNode->audioUnits)
			{
				auto& unitNode = unitNodeKV.second;

				// Search AudioUnit
				if (isDescriptorIndexInRange(descriptorIndex, unitNode.staticModel.baseControl, unitNode.staticModel.numberOfControls))
				{
					auto it = unitNode.controls.find(descriptorIndex);
					if (it == unitNode.controls.end())
					{
						if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid control index"))
						{
							return nullptr;
						}
						it = unitNode.controls.emplace(descriptorIndex, model::ControlNode{ descriptorIndex }).first;
					}

					return &(it->second);
				}

				// Search StreamPortInputs
				for (auto& streamPortNodeKV : unitNode.streamPortInputs)
				{
					auto& streamPortNode = streamPortNodeKV.second;

					if (isDescriptorIndexInRange(descriptorIndex, streamPortNode.staticModel.baseControl, streamPortNode.staticModel.numberOfControls))
					{
						auto it = streamPortNode.controls.find(descriptorIndex);
						if (it == streamPortNode.controls.end())
						{
							if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid control index"))
							{
								return nullptr;
							}
							it = streamPortNode.controls.emplace(descriptorIndex, model::ControlNode{ descriptorIndex }).first;
						}

						return &(it->second);
					}
				}

				// Search StreamPortOutputs
				for (auto& streamPortNodeKV : unitNode.streamPortOutputs)
				{
					auto& streamPortNode = streamPortNodeKV.second;

					if (isDescriptorIndexInRange(descriptorIndex, streamPortNode.staticModel.baseControl, streamPortNode.staticModel.numberOfControls))
					{
						auto it = streamPortNode.controls.find(descriptorIndex);
						if (it == streamPortNode.controls.end())
						{
							if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid control index"))
							{
								return nullptr;
							}
							it = streamPortNode.controls.emplace(descriptorIndex, model::ControlNode{ descriptorIndex }).first;
						}

						return &(it->second);
					}
				}

				// Search ExternalPortInputs

				// Search ExternalPortOutputs

				// Search InternalPortInputs

				// Search InternalPortOutputs
			}

			// Search a matching ControlIndex in all VideoUnits/StreamPorts/ExternalPorts/InternalPorts

			// Search a matching ControlIndex in all SensorUnits/StreamPorts/ExternalPorts/InternalPorts

			// Search JackInputs
			for (auto& jackNodeKV : configurationNode->jackInputs)
			{
				auto& jackNode = jackNodeKV.second;

				if (isDescriptorIndexInRange(descriptorIndex, jackNode.staticModel.baseControl, jackNode.staticModel.numberOfControls))
				{
					auto it = jackNode.controls.find(descriptorIndex);
					if (it == jackNode.controls.end())
					{
						if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid control index"))
						{
							return nullptr;
						}
						it = jackNode.controls.emplace(descriptorIndex, model::ControlNode{ descriptorIndex }).first;
					}

					return &(it->second);
				}
			}

			// Search JackOutputs
			for (auto& jackNodeKV : configurationNode->jackOutputs)
			{
				auto& jackNode = jackNodeKV.second;

				if (isDescriptorIndexInRange(descriptorIndex, jackNode.staticModel.baseControl, jackNode.staticModel.numberOfControls))
				{
					auto it = jackNode.controls.find(descriptorIndex);
					if (it == jackNode.controls.end())
					{
						if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid control index"))
						{
							return nullptr;
						}
						it = jackNode.controls.emplace(descriptorIndex, model::ControlNode{ descriptorIndex }).first;
					}

					return &(it->second);
				}
			}

#if 0 // IEEE 1722.1-2021

			// Search AvbInterfaces
			for (auto& avbInterfaceNodeKV : configurationNode->avbInterfaces)
			{
				auto& avbInterfaceNode = avbInterfaceNodeKV.second;

				if (isDescriptorIndexInRange(descriptorIndex, avbInterfaceNode.staticModel.baseControl, avbInterfaceNode.staticModel.numberOfControls))
				{
					auto it = avbInterfaceNode.controls.find(descriptorIndex);
					if (it == avbInterfaceNode.controls.end())
					{
						if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid control index"))
						{
							return nullptr;
						}
						it = avbInterfaceNode.controls.emplace(descriptorIndex, model::ControlNode{descriptorIndex}).first;
					}

					return &(it->second);
				}
			}
#endif

			// Search ControlBlocks

#if 0 // IEEE 1722.1-2021

			// Search PtpInstances
			for (auto& ptpInstanceNodeKV : configurationNode->ptpInstances)
			{
				auto& ptpInstanceNode = ptpInstanceNodeKV.second;

				if (isDescriptorIndexInRange(descriptorIndex, ptpInstanceNode.staticModel.baseControl, ptpInstanceNode.staticModel.numberOfControls))
				{
					auto it = ptpInstanceNode.controls.find(descriptorIndex);
					if (it == ptpInstanceNode.controls.end())
					{
						if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid control index"))
						{
							return nullptr;
						}
						it = ptpInstanceNode.controls.emplace(descriptorIndex, model::ControlNode{descriptorIndex}).first;
					}

					return &(it->second);
				}
			}
#endif

			// Not found
			handleDescriptorNotFound(NotFoundBehavior::IgnoreAndReturnNull, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid control index");
			return nullptr;
		}
		return nullptr;
	}

	virtual model::ClockDomainNode* getClockDomainNode(entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const descriptorIndex, NotFoundBehavior const notFoundBehavior) override
	{
		auto* const configurationNode = getConfigurationNode(configurationIndex, notFoundBehavior);
		if (configurationNode)
		{
			auto it = configurationNode->clockDomains.find(descriptorIndex);
			if (it == configurationNode->clockDomains.end())
			{
				if (!handleDescriptorNotFound(notFoundBehavior, ControlledEntity::Exception::Type::InvalidDescriptorIndex, "Invalid clockDomain index"))
				{
					return nullptr;
				}
				it = configurationNode->clockDomains.emplace(descriptorIndex, model::ClockDomainNode{ descriptorIndex }).first;
			}

			return &(it->second);
		}
		return nullptr;
	}
};

} // namespace controller
} // namespace avdecc
} // namespace la
